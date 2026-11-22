#include "cli.h"

#include <algorithm>
#include <charconv>
#include <cstdio>
#include <format>
#include <map>
#include <optional>
#include <print>
#include <string>

#include "canvas.h"
#include "document.h"
#include "image.h"
#include "model/xml.h"
#include "path.h"
#include "render.h"
#include "sdl.h"
#include "settings.h"
#include "working_directory.h"

constexpr const char* USAGE = R"(usage: anm2ed <command> <file.anm2> [options]

commands:
  info                  animations, items, spritesheets and Isaac compatibility, as text
  frame                 one frame as a PNG                       (--out, --time)
  convert               the document as --format, to --out (default: the file itself)
  render                the animation as --type:
                          sheet  all frames in a grid PNG (default; --rows, --columns)
                          pngs   one PNG per frame in the --out folder (--format, default "{}.png")
                          gif, webm, mp4, ogv  through FFmpeg (--ffmpeg, default: the editor's setting)

options:
  --format json|xml       the document format for "convert" (default xml: the game's, with the editor's copy)
  --animation NAME|INDEX  which animation (default: the default animation)
  --out PATH              where to write
  --time T                frame time for "frame" (default 0)
  --start N, --end N      frame range for "render" (default: the whole animation)
  --fps N                 frames per second to sample (default: the animation's)
  --scale S               output scale (default 1)
  --background RRGGBB     background color, or "transparent" (default)
  --no-root-transform     draw without the root's transform
)";

constexpr float SCALE_DEFAULT = 1.0f;
constexpr int HEX_BASE = 16;
constexpr int HEX_RGB_LENGTH = 6;
constexpr float CHANNEL_MAX = 255.0f;

struct Arguments
{
    std::string command;
    std::filesystem::path file;
    std::map<std::string, std::string> options;

    std::optional<std::string> get(const std::string& name) const
    {
        auto it = options.find(name);
        return it == options.end() ? std::nullopt : std::optional(it->second);
    }

    template <class T> T number_get(const std::string& name, T value) const
    {
        if (auto text = get(name))
            std::from_chars(text->data(), text->data() + text->size(), value);
        return value;
    }

    bool is(const std::string& name) const
    {
        return options.contains(name);
    }
};

// PNG sequences go into a folder. Everything else is a file.
static bool type_is_folder(const Arguments& arguments)
{
    return arguments.command == "render" && arguments.get("type").value_or("") == "pngs";
}

static std::optional<Arguments> arguments_get(int argc, const char** argv)
{
    if (argc < 3)
        return {};
    // Absolute, so a bare file name still has a folder for the paths in it.
    Arguments arguments = {.command = argv[1], .file = std::filesystem::absolute(path_from_utf8(argv[2]))};
    for (int i = 3; i < argc; ++i)
    {
        std::string name = argv[i];
        if (!name.starts_with("--"))
            return {};
        name.erase(0, 2);
        auto isFlag = i + 1 >= argc || std::string_view(argv[i + 1]).starts_with("--");
        arguments.options[name] = isFlag ? std::string{} : std::string(argv[++i]);
    }
    return arguments;
}

// RRGGBB (opaque) or "transparent".
static Vec4 background_get(const Arguments& arguments)
{
    auto text = arguments.get("background").value_or("transparent");
    if (text.starts_with('#'))
        text.erase(0, 1);
    unsigned value = 0;
    if (text.size() != HEX_RGB_LENGTH ||
        std::from_chars(text.data(), text.data() + text.size(), value, HEX_BASE).ec != std::errc{})
        return {};
    return {(float)((value >> 16) & 0xFF) / CHANNEL_MAX, (float)((value >> 8) & 0xFF) / CHANNEL_MAX,
            (float)(value & 0xFF) / CHANNEL_MAX, 1.0f};
}

// --ffmpeg, or else the FFmpeg the editor's Render Animation uses (or "ffmpeg" on the PATH).
static std::filesystem::path ffmpeg_path_get(const Arguments& arguments)
{
    if (auto ffmpeg = arguments.get("ffmpeg"))
        return path_from_utf8(*ffmpeg);
    auto settingsPath = preferences_directory_get() / "settings.ini";
    auto settingsFfmpeg =
        is_path_exist(settingsPath) ? Settings(settingsPath).renderFFmpegPath : std::filesystem::path{};
    return settingsFfmpeg.empty() ? std::filesystem::path("ffmpeg") : settingsFfmpeg;
}

// By index or name. The default animation otherwise.
static std::pair<int, const Animation*> animation_get(const Model& model, const Arguments& arguments)
{
    auto animations = model.animations_get();
    auto name = arguments.get("animation").value_or(model.animations.defaultAnimation);
    int index = -1;
    if (std::from_chars(name.data(), name.data() + name.size(), index).ec == std::errc{} && index >= 0 &&
        index < (int)animations.size())
        return animations[index];
    for (auto [i, animation] : animations)
        if (animation->name == name)
            return {i, animation};
    if (!arguments.is("animation") && !animations.empty())
        return animations.front();
    return {-1, nullptr};
}

static int info_print(const Arguments& arguments)
{
    Model model = {};
    std::string error;
    if (!model_load(model, arguments.file, &error))
    {
        std::println(stderr, "could not load {}: {}", path_to_utf8(arguments.file), error);
        return 1;
    }
    auto& content = model.content;
    std::println("file: {}", path_to_utf8(arguments.file));
    std::println("fps: {}, created by: {}", model.info.fps, model.info.createdBy);

    std::println("animations (default: {}):", model.animations.defaultAnimation);
    for (auto [index, animation] : model.animations_get())
    {
        int layers{}, nulls{};
        items_each(animation->layers, [&](const Track&, auto*) { ++layers; });
        items_each(animation->nulls, [&](const Track&, auto*) { ++nulls; });
        std::println("  [{}] {}: {} frames{}, {} layer tracks, {} null tracks, {} triggers", index, animation->name,
                     animation->frameNum, animation->isLoop ? ", loops" : "", layers, nulls,
                     animation->triggers.frames.size());
    }

    WorkingDirectory workingDirectory(arguments.file.parent_path());
    std::println("spritesheets:");
    for (auto& spritesheet : content.spritesheets)
    {
        auto image = Image(path_case_insensitive_find(spritesheet.path));
        auto status = image.is_valid() ? std::format("{}x{}", image.size.x, image.size.y) : std::string("missing");
        std::println("  #{} {} ({}), {} regions", spritesheet.id, path_to_utf8(spritesheet.path), status,
                     spritesheet.regions.size());
    }
    for (auto& library : content.libraries)
        std::println("library #{} {}", library.id, path_to_utf8(library.path));
    for (auto& layer : content.layers)
        if (layer.is_animation())
            std::println("layer #{} {} (animation {}, library #{})", layer.id, layer.name, layer.animation,
                         layer.libraryId);
        else
            std::println("layer #{} {} (spritesheet #{})", layer.id, layer.name, layer.spritesheetId);
    for (auto& null : content.nulls)
        std::println("null #{} {}", null.id, null.name);
    for (auto& event : content.events)
        std::println("event #{} {}", event.id, event.name);
    for (auto& sound : content.sounds)
        std::println("sound #{} {}", sound.id, path_to_utf8(sound.path));

    auto issues = isaac_issues_get(model);
    std::println("isaac issues: {} eased frames, {} group transforms, {} skewed frames, {} shader frames, {} depth "
                 "frames, {} alpha offset frames, {} blurred frames, {} extra trigger events, {} trigger sounds, "
                 "{} animation layers, {} vector spritesheets",
                 issues.easedFrames, issues.groupTransforms, issues.skewFrames, issues.shaderFrames, issues.depthFrames,
                 issues.alphaOffsetFrames, issues.blurFrames, issues.extraEvents, issues.triggerSounds,
                 issues.animationLayers, issues.vectorSpritesheets);
    return 0;
}

static int draw_run(const Arguments& arguments)
{
    auto out = arguments.get("out");
    if (!out)
    {
        std::println(stderr, "--out is required");
        return 2;
    }

    HeadlessGL gl = {};
    if (!gl.is_valid())
    {
        std::println(stderr, "could not create an OpenGL 3.3 context");
        return 1;
    }

    std::string error;
    Document document(arguments.file, DOCUMENT_OPEN, &error);
    if (!document.is_valid())
    {
        std::println(stderr, "could not load {}: {}", path_to_utf8(arguments.file), error);
        return 1;
    }
    auto [index, animation] = animation_get(document.model, arguments);
    if (!animation)
    {
        std::println(stderr, "no such animation: {}", arguments.get("animation").value_or(""));
        return 1;
    }

    auto& textureInfo = SHADERS[SHADER_TEXTURE];
    ShaderProgram shader(textureInfo.vertex, textureInfo.fragment);
    Canvas canvas(Vec2(1.0f));
    auto isRootTransform = !arguments.is("no-root-transform");
    auto bounds = animation_bounds_get(document, *animation, isRootTransform);
    auto scale = arguments.number_get("scale", SCALE_DEFAULT);
    auto background = background_get(arguments);
    auto frame_render = [&](float time) {
        return animation_frame_render(canvas, shader, document, *animation, time, bounds, scale, background,
                                      isRootTransform);
    };
    auto outPath = path_from_utf8(*out);
    std::error_code ec;
    std::filesystem::create_directories(type_is_folder(arguments) ? outPath : outPath.parent_path(), ec);

    if (arguments.command == "frame")
    {
        auto pixels = frame_render(arguments.number_get("time", 0.0f));
        pixels_unpremultiply(pixels);
        if (!Image::write_pixels_png(outPath, IVec2(canvas.size), pixels.data()))
        {
            std::println(stderr, "could not write {}", *out);
            return 1;
        }
        std::println("{}: \"{}\" at time {}, {}x{}", *out, animation->name, arguments.number_get("time", 0.0f),
                     (int)canvas.size.x, (int)canvas.size.y);
        return 0;
    }

    auto typeName = arguments.get("type").value_or("sheet");
    auto type = typeName == "sheet" ? RENDER_SPRITESHEET : RENDER_PNGS;
    for (int i = 0; i < RENDER_COUNT; ++i)
        if (RENDER_EXTENSIONS[i] == "." + typeName)
            type = (RenderType)i;
    if (typeName != "sheet" && typeName != "pngs" && type == RENDER_PNGS)
    {
        std::println(stderr, "unknown --type: {}", typeName);
        return 2;
    }

    auto fps = std::max(document.model.info.fps, 1);
    RecorderOptions options{.type = type,
                            .path = outPath,
                            .format = path_from_utf8(arguments.get("format").value_or("{}.png")),
                            .ffmpegPath = ffmpeg_path_get(arguments),
                            .start = arguments.number_get("start", 0),
                            .end = arguments.number_get("end", animation->frameNum - 1),
                            .animationFps = fps,
                            .fps = arguments.number_get("fps", fps),
                            .rows = arguments.number_get("rows", 0),
                            .columns = arguments.number_get("columns", 0)};
    auto count = frame_count_get(options.start, options.end, options.animationFps, options.fps);
    if (!animation_record(options, canvas, shader, document, *animation, bounds, scale, background, isRootTransform))
    {
        std::println(stderr, "could not write {} (FFmpeg: {})", *out, path_to_utf8(options.ffmpegPath));
        return 1;
    }

    std::print("{}: \"{}\", {} frames ({} to {} at {} fps), each {}x{}", *out, animation->name, count, options.start,
               options.end, options.fps, (int)canvas.size.x, (int)canvas.size.y);
    if (type == RENDER_SPRITESHEET)
    {
        auto layout = spritesheet_layout_get(options.rows, options.columns, count);
        std::print(", {} rows x {} columns, left to right", layout.rows, layout.columns);
    }
    std::println("");
    return 0;
}

// Saved as the editor saves it: XML (the game's copy, with the editor's embedded) or the editor's JSON.
static int convert_run(const Arguments& arguments)
{
    auto format = arguments.get("format").value_or("xml");
    if (format != "xml" && format != "json")
    {
        std::println(stderr, "--format must be json or xml");
        return 2;
    }
    // The game's copy bakes what it can't draw with OpenGL (and the document's shaders compile only with it).
    std::optional<HeadlessGL> gl;
    if (format == "xml")
        gl.emplace();
    std::string error;
    Document document(arguments.file, DOCUMENT_OPEN, &error);
    auto fileUtf8 = path_to_utf8(arguments.file);
    if (!document.is_valid())
    {
        std::println(stderr, "could not load {}: {}", fileUtf8, error);
        return 1;
    }
    if (!error.empty())
        std::println(stderr, "{}:\n{}", fileUtf8, error);
    auto out = std::filesystem::absolute(arguments.get("out").transform(path_from_utf8).value_or(arguments.file));
    Options options = {};
    options.isJson = format == "json";
    if (document.save(out, &error, options))
        return 0;
    std::println(stderr, "could not write {}: {}", path_to_utf8(out), error);
    return 1;
}

constexpr std::string_view COMMANDS[] = {"info", "frame", "render", "convert"};

bool is_cli(int argc, const char** argv)
{
    return argc >= 2 && std::ranges::contains(COMMANDS, std::string_view(argv[1])) && !is_path_exist(argv[1]);
}

int cli_run(int argc, const char** argv)
{
    auto arguments = arguments_get(argc, argv);
    if (!arguments)
    {
        std::print(stderr, "{}", USAGE);
        return 2;
    }
    if (arguments->command == "info")
        return info_print(*arguments);
    if (arguments->command == "convert")
        return convert_run(*arguments);
    return draw_run(*arguments);
}
