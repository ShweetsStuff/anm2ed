#include "render.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <format>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <string_view>

#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include "gl.h"

#include "audio.h"
#include "canvas.h"
#include "document.h"
#include "file.h"
#include "log.h"
#include "model/frames.h"
#include "pack.h"
#include "path.h"
#include "subprocess.h"
#include "texture.h"
#include "ui/toast.h"

bool animation_render(const std::filesystem::path& ffmpegPath, const std::filesystem::path& path,
                      const std::vector<std::filesystem::path>& framePaths, const std::vector<float>& audio,
                      RenderType type, int fps)
{
    if (framePaths.empty() || ffmpegPath.empty() || path.empty())
        return false;
    fps = std::max(fps, 1);

    auto pathString = path_to_utf8(path);
    auto ffmpegPathString = path_to_utf8(ffmpegPath);

    std::filesystem::path audioPath;
    std::string audioInputArguments;
    std::string audioOutputArguments = "-an";
    std::string command;
    auto temporaryDirectory = framePaths.front().parent_path();
    if (temporaryDirectory.empty())
        temporaryDirectory = std::filesystem::temp_directory_path();

    auto audio_remove = [&]() {
        if (!audioPath.empty())
        {
            std::error_code ec;
            std::filesystem::remove(audioPath, ec);
        }
    };

    if (type != RENDER_GIF && !audio.empty())
    {
        auto tempFilenameUtf8 =
            std::format("anm2ed_audio_{}_{}.f32", std::hash<std::string>{}(pathString), SDL_GetTicks());
        audioPath = temporaryDirectory / path_from_utf8(tempFilenameUtf8);

        auto data = (const char*)audio.data();
        auto byteCount = audio.size() * sizeof(float);
        if (file_text_save(audioPath, std::string_view(data, byteCount)))
        {
            auto audioDurationFilter = std::string{};
            auto frameCount = (double)std::max((int)framePaths.size(), 1);
            auto expectedDurationSeconds = frameCount / (double)fps;
            if (expectedDurationSeconds > 0.0)
                audioDurationFilter += std::format("apad,atrim=duration={:.9f}", expectedDurationSeconds);

            audioInputArguments = std::format("-f f32le -ar {0} -ac {1} -i \"{2}\"", AUDIO_FREQUENCY, AUDIO_CHANNELS,
                                              path_to_utf8(audioPath));

            switch (type)
            {
                case RENDER_WEBM:
                    audioOutputArguments = "-c:a libopus -b:a 160k -shortest";
                    break;
                case RENDER_OGV:
                    audioOutputArguments = "-c:a libvorbis -q:a 4 -shortest";
                    break;
                case RENDER_MP4:
                    audioOutputArguments = "-c:a aac -b:a 192k -shortest";
                    break;
                default:
                    break;
            }

            if (!audioDurationFilter.empty())
                audioOutputArguments = std::format("-af \"{}\" {}", audioDurationFilter, audioOutputArguments);
        }
        else
        {
            logger.warning("Failed to open temporary audio file; exporting video without audio.");
            audio_remove();
        }
    }

    auto framePattern = temporaryDirectory / path_from_utf8("frame_%06d.png");
    auto framePatternString = path_to_utf8(framePattern);
    command = std::format("\"{0}\" -y -hide_banner -loglevel warning -framerate {1} -start_number 0 -i \"{2}\"",
                          ffmpegPathString, fps, framePatternString);

    if (!audioInputArguments.empty())
        command += " " + audioInputArguments;
    command += std::format(" -fps_mode cfr -r {}", fps);

    switch (type)
    {
        case RENDER_GIF:
            command += " -lavfi \"split[s0][s1];[s0]palettegen=stats_mode=full:reserve_transparent=1[p];"
                       "[s1][p]paletteuse=dither=floyd_steinberg:alpha_threshold=128\""
                       " -loop 0";
            command += std::format(" \"{}\"", pathString);
            break;
        case RENDER_WEBM:
            command +=
                " -c:v libvpx-vp9 -crf 30 -b:v 0 -pix_fmt yuva420p -row-mt 1 -threads 0 -speed 2 -auto-alt-ref 0";
            if (!audioOutputArguments.empty())
                command += " " + audioOutputArguments;
            command += std::format(" \"{}\"", pathString);
            break;
        case RENDER_OGV:
            command += " -vf \"format=yuv420p,scale=trunc(iw/2)*2:trunc(ih/2)*2\" -c:v libtheora -q:v 7";
            if (!audioOutputArguments.empty())
                command += " " + audioOutputArguments;
            command += std::format(" \"{}\"", pathString);
            break;
        case RENDER_MP4:
            command += " -vf \"format=yuv420p,scale=trunc(iw/2)*2:trunc(ih/2)*2\" -c:v libx265 -crf 20 -preset slow"
                       " -tag:v hvc1 -movflags +faststart";
            if (!audioOutputArguments.empty())
                command += " " + audioOutputArguments;
            command += std::format(" \"{}\"", pathString);
            break;
        default:
        {
            return false;
        }
    }

    // FFmpeg's warnings and errors go to the end of the log.
    if (auto logPath = logger.path(); !logPath.empty())
        command += std::format(" 2>> \"{}\"", path_to_utf8(logPath));
#if _WIN32
    command = "\"" + command + "\"";
#endif

    logger.command(command);

#if _WIN32
    Process process(command.c_str(), "wb");
#else
    Process process(command.c_str(), "w");
#endif

    if (!process.get())
    {
        audio_remove();
        return false;
    }

    auto ffmpegExitCode = process.close();
    if (ffmpegExitCode != 0)
    {
        logger.error(std::format("FFmpeg exited with code {} while exporting {}", ffmpegExitCode, pathString));
        audio_remove();
        return false;
    }

    audio_remove();
    return true;
}

constexpr int TEMP_DIRECTORY_ATTEMPTS = 1000;
constexpr Vec4 BOUNDS_DEFAULT = {-32.0f, -32.0f, 64.0f, 64.0f};
constexpr int ALPHA_OPAQUE = 255;
constexpr float CHANNEL_MAX = 255.0f;

static std::filesystem::path frame_filename_get(const std::filesystem::path& format, int index, RenderType type)
{
    auto fallback = path_from_utf8(std::format("frame_{:06}.png", index));
    if (type != RENDER_PNGS)
        return fallback;
    // A format without a placeholder numbers its frames after its name.
    auto formatUtf8 = path_to_utf8(format);
    if (formatUtf8.find('{') == std::string::npos)
        formatUtf8 = path_to_utf8(format.stem()) + "_{:06}" + path_to_utf8(format.extension());
    try
    {
        auto filename = path_from_utf8(std::vformat(formatUtf8, std::make_format_args(index))).filename();
        if (filename.empty())
            return fallback;
        if (filename.extension().empty())
            filename.replace_extension(RENDER_EXTENSIONS[RENDER_SPRITESHEET]);
        return filename;
    }
    catch (...)
    {
        return fallback;
    }
}

// PNG sequences go straight to their folder. Other outputs collect frames in a temporary folder beside the output.
static std::filesystem::path frames_directory_create(const RecorderOptions& options)
{
    std::error_code ec;
    if (options.type == RENDER_PNGS)
    {
        std::filesystem::create_directories(options.path, ec);
        return options.path;
    }
    auto directory = options.path.parent_path();
    if (directory.empty())
        directory = std::filesystem::current_path();
    std::filesystem::create_directories(directory, ec);
    auto timestamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    for (int suffix = 0; suffix < TEMP_DIRECTORY_ATTEMPTS; ++suffix)
    {
        auto temporary = directory / path_from_utf8(std::format(".anm2ed_render_tmp_{}_{}", timestamp, suffix));
        if (std::filesystem::create_directories(temporary, ec))
            return temporary;
    }
    return {};
}

// Rendered pixels are premultiplied. Outputs on a transparent background want straight alpha.
void pixels_unpremultiply(std::vector<uint8_t>& pixels)
{
    for (std::size_t index = 0; index + 3 < pixels.size(); index += IMAGE_CHANNELS)
    {
        auto alpha = pixels[index + 3];
        if (alpha == ALPHA_OPAQUE)
            continue;
        for (int channel = 0; channel < 3; ++channel)
            pixels[index + channel] =
                alpha == 0
                    ? 0
                    : (uint8_t)vec_clamp(std::round(pixels[index + channel] * CHANNEL_MAX / alpha), 0.0f, CHANNEL_MAX);
    }
}

HeadlessGL::HeadlessGL()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
        return;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    window = SDL_CreateWindow("anm2ed", 1, 1, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (window)
        context = SDL_GL_CreateContext(window);
    if (!context || !gl_load())
    {
        if (context)
            SDL_GL_DestroyContext(context);
        context = nullptr;
        return;
    }
    gl.Enable(GL_BLEND);
    gl.Disable(GL_DEPTH_TEST);
}

HeadlessGL::~HeadlessGL()
{
    if (context)
        SDL_GL_DestroyContext(context);
    if (window)
        SDL_DestroyWindow(window);
    SDL_Quit();
}

bool HeadlessGL::is_valid() const
{
    return context != nullptr;
}

Vec4 animation_bounds_get(const Document& document, const Animation& animation, bool isRootTransform)
{
    auto bounds = animation_rect(document.model, animation, isRootTransform, document.library_resolver_get());
    return bounds != Vec4(-1.0f) ? bounds : BOUNDS_DEFAULT;
}

std::vector<uint8_t> animation_frame_render(Canvas& canvas, ShaderProgram& shader, Document& document,
                                            const Animation& animation, float time, Vec4 bounds, float scale,
                                            Vec4 background, bool isRootTransform)
{
    canvas.size_set(Vec2(vec_max(IVec2(vec_ceil(Vec2(bounds.z, bounds.w) * scale)), IVec2(1))));
    float zoom = 0;
    Vec2 pan{};
    canvas.set_to_rect(zoom, pan, bounds);
    canvas.bind();
    canvas.viewport_set();
    canvas.clear(Vec4(Vec3(background) * background.a, background.a));
    canvas.layers_render(
        shader, document.model, animation,
        {.time = time, .isRootTransform = isRootTransform, .library_get = document.library_resolver_get()},
        canvas.transform_get(zoom, pan),
        [&](const Draw& draw) { return document.draw_image_get(draw, scale * draw_scale_get(draw)); },
        [&](int id) { return document.shader_get(id); });
    auto pixels = canvas.pixels_get();
    canvas.unbind();
    return pixels;
}

bool Recorder::start(const RecorderOptions& recorderOptions)
{
    options = recorderOptions;
    index = 0;
    count = frame_count_get(options.start, options.end, options.animationFps, options.fps);
    frames.clear();
    sounds.clear();
    soundCopies.clear();
    soundTime = -1;
    directory = frames_directory_create(options);
    return !directory.empty();
}

bool Recorder::is_done() const
{
    return index >= count;
}

float Recorder::time_get() const
{
    return frame_time_get(options.start, index, options.animationFps, options.fps);
}

// A trigger sound is taken once per animation frame, however many render frames it spans.
bool Recorder::is_sound_frame() const
{
    return (int)std::floor(time_get()) != soundTime;
}

bool Recorder::frame_capture(std::vector<uint8_t> pixels, IVec2 size, std::vector<const AudioData*> frameSounds)
{
    soundTime = (int)std::floor(time_get());
    for (auto& sound : frameSounds)
        sound = &soundCopies.try_emplace(sound->uid, *sound).first->second;
    sounds.push_back(std::move(frameSounds));
    if (options.isIsolated || options.isBounded)
        pixels_unpremultiply(pixels);
    auto framePath = directory / frame_filename_get(options.format, index, options.type);
    if (!Image::write_pixels_png(framePath, size, pixels.data()))
        return false;
    frames.push_back(framePath);
    ++index;
    return true;
}

bool Recorder::finish()
{
    auto pathString = path_to_utf8(options.path);
    auto isFinished = !frames.empty();
    if (options.type == RENDER_PNGS)
        toast_log(isFinished ? Level::INFO : Level::ERROR,
                  isFinished ? TOAST_EXPORT_RENDERED_FRAMES : TOAST_EXPORT_RENDERED_FRAMES_FAILED, pathString);
    else if (options.type == RENDER_SPRITESHEET)
    {
        auto layout = spritesheet_layout_get(options.rows, options.columns, (int)frames.size());
        auto first = frames.empty() ? Image{} : Image(frames.front());
        if (frames.empty())
            toast_log(Level::WARNING, TOAST_SPRITESHEET_NO_FRAMES);
        else if (!first.is_valid())
        {
            isFinished = false;
            toast_log(Level::ERROR, TOAST_SPRITESHEET_EMPTY);
        }
        else
        {
            Image sheet = {};
            sheet.size = first.size * IVec2(layout.columns, layout.rows);
            sheet.pixels.resize((std::size_t)sheet.size.x * sheet.size.y * IMAGE_CHANNELS);
            for (int i = 0; i < (int)frames.size() && i < layout.rows * layout.columns; ++i)
                if (auto frame = Image(frames[i]); frame.size == first.size)
                    sheet.paste(frame, first.size * IVec2(i % layout.columns, i / layout.columns));
            isFinished = sheet.write_png(options.path);
            toast_log(isFinished ? Level::INFO : Level::ERROR,
                      isFinished ? TOAST_EXPORT_SPRITESHEET : TOAST_EXPORT_SPRITESHEET_FAILED, pathString);
        }
    }
    else
    {
        auto audio =
            options.isSound && options.type != RENDER_GIF ? audio_mix(sounds, options.fps) : std::vector<float>();
        isFinished = animation_render(options.ffmpegPath, options.path, frames, audio, options.type, options.fps);
        toast_log(isFinished ? Level::INFO : Level::ERROR,
                  isFinished ? TOAST_EXPORT_RENDERED_ANIMATION : TOAST_EXPORT_RENDERED_ANIMATION_FAILED, pathString);
    }
    cancel();
    return isFinished;
}

// Stops. The temporary frame folder is removed (a PNG sequence's own folder is kept).
void Recorder::cancel()
{
    std::error_code ec;
    if (options.type != RENDER_PNGS && !directory.empty())
        std::filesystem::remove_all(directory, ec);
    directory.clear();
    frames.clear();
    sounds.clear();
    soundCopies.clear();
    index = count = 0;
}

bool animation_record(const RecorderOptions& recorderOptions, Canvas& canvas, ShaderProgram& shader, Document& document,
                      const Animation& animation, Vec4 bounds, float scale, Vec4 background, bool isRootTransform)
{
    auto options = recorderOptions;
    options.isIsolated = options.isBounded = true;
    Recorder recorder = {};
    if (!recorder.start(options))
        return false;
    while (!recorder.is_done())
    {
        auto pixels = animation_frame_render(canvas, shader, document, animation, recorder.time_get(), bounds, scale,
                                             background, isRootTransform);
        if (recorder.frame_capture(std::move(pixels), IVec2(canvas.size)))
            continue;
        recorder.cancel();
        return false;
    }
    return recorder.finish();
}

constexpr float BAKE_MARGIN = 1.0f;
constexpr int BAKED_SIZE_MAX = 4096;

// A layer draw alone at canvas pixels, cut to what it can cover (its blur's reach too): its image (straight alpha) and
// the canvas position of the image's top-left. Its tint and color offset go into the pixels only when `isColored`.
static std::optional<std::pair<Image, Vec2>> draw_bake(Canvas& canvas, ShaderProgram& shader, Document& document,
                                                       const Draw& draw, bool isColored)
{
    auto image = document.draw_image_get(draw, draw_scale_get(draw));
    if (!image || !image->is_valid() || draw.frame.size == Vec2())
        return {};
    auto quad = draw.parent * draw_quad_model_get(draw);
    Vec2 low{std::numeric_limits<float>::max()};
    Vec2 high{std::numeric_limits<float>::lowest()};
    for (auto corner : {Vec2(0.0f), Vec2(1.0f, 0.0f), Vec2(0.0f, 1.0f), Vec2(1.0f)})
    {
        auto point = Vec2(quad * Vec4(corner, 0.0f, 1.0f));
        low = vec_min(low, point);
        high = vec_max(high, point);
    }
    auto reach = draw.frame.blur + BAKE_MARGIN;
    low = vec_floor(low - reach);
    high = vec_ceil(high + reach);
    auto size = IVec2(high - low);

    canvas.size_set(Vec2(size));
    float zoom = 0;
    Vec2 pan{};
    canvas.set_to_rect(zoom, pan, Vec4(low, Vec2(size)));
    canvas.bind();
    canvas.viewport_set();
    canvas.clear(Vec4(0.0f));
    auto custom = document.shader_get(draw.frame.shaderId);
    canvas.layer_render(custom ? *custom : shader, texture_id_get(*image), image->unit_size_get(),
                        canvas.transform_get(zoom, pan), draw, isColored ? draw.frame.tint : Vec4(1.0f),
                        isColored ? draw.frame.colorOffset : Vec4(0.0f));
    auto pixels = canvas.pixels_get();
    canvas.unbind();
    pixels_unpremultiply(pixels);
    return std::pair{Image(pixels.data(), size), low};
}

// The game's copy with the layer frames it can't draw baked into images: skewed, blurred or shaded ones, and every
// layer frame under a skewed root or group root, which then goes into the images (the root becomes plain). A layer has
// one spritesheet in the game, so baked frames stand on a copy of their layer
// ("name (baked)") right after it, one frame per step, while the layer hides them. The images (each once) are packed
// into one spritesheet written to `path`, beside the document. When it can't be, `result` keeps its frames.
void frames_bake(Document& document, Model& result, const std::filesystem::path& path)
{
    auto game = result;
    auto is_baked = [](const Frame& frame) {
        return frame.skew != Vec2() || frame.blur > 0.0f || frame.shaderId != -1;
    };
    auto is_skewed = [](const Track& root) {
        return std::ranges::any_of(root.frames, [](const Frame& frame) { return frame.skew != Vec2(); });
    };

    std::vector<int> indices;
    for (int i = 0; i < game.animations_count_get(); ++i)
    {
        auto& animation = *game.animation_get(i);
        auto isNeeded = is_skewed(animation.root);
        items_each(animation.layers, [&](const Track& track, const TrackGroup* group) {
            isNeeded |= (group && is_skewed(group->root)) || std::ranges::any_of(track.frames, is_baked);
        });
        if (isNeeded)
            indices.push_back(i);
    }
    auto pathUtf8 = path_to_utf8(path);
    if (indices.empty())
        return;
    if (!SDL_GL_GetCurrentContext())
        return toast_log(Level::WARNING, TOAST_BAKE_NO_OPENGL, pathUtf8);

    auto& textureInfo = SHADERS[SHADER_TEXTURE];
    ShaderProgram shader(textureInfo.vertex, textureInfo.fragment);
    Canvas canvas(Vec2(1.0f));
    auto& content = game.content;
    auto spritesheetId = item_next_id_get(content.spritesheets);

    std::vector<Image> images;
    std::map<std::size_t, std::vector<int>> imageHashes;
    auto image_index_get = [&](Image image) {
        auto hash =
            std::hash<std::string_view>{}(std::string_view((const char*)image.pixels.data(), image.pixels.size()));
        for (auto index : imageHashes[hash])
            if (images[index].size == image.size && images[index].pixels == image.pixels)
                return index;
        imageHashes[hash].push_back((int)images.size());
        images.push_back(std::move(image));
        return (int)images.size() - 1;
    };
    std::map<int, int> bakedLayerIds;
    auto baked_layer_id_get = [&](int layerId) {
        auto [it, isNew] = bakedLayerIds.try_emplace(layerId);
        if (!isNew)
            return it->second;
        auto layer = item_get(content.layers, layerId);
        it->second = item_next_id_get(content.layers);
        content.layers.push_back({.id = it->second,
                                  .name = (layer ? layer->name : std::string{}) + " (baked)",
                                  .spritesheetId = spritesheetId});
        return it->second;
    };
    // Steps alike join into one frame.
    auto step_add = [](Track& track, Frame frame) {
        frame.duration = FRAME_DURATION_MIN;
        frame.interpolation = Interpolation::NONE;
        if (!track.frames.empty())
        {
            auto last = track.frames.back();
            last.uid = frame.uid;
            last.duration = frame.duration;
            if (last == frame)
                return (void)++track.frames.back().duration;
        }
        track.frames.push_back(frame);
    };

    for (auto index : indices)
    {
        auto& animation = *game.animation_edit(index);
        auto isRootFolded = is_skewed(animation.root);
        // A layer track's baked copy, the track left with the rest. Nothing when no frame of it bakes.
        auto track_bake = [&](Track& track, const Track* groupRoot) -> std::optional<Track> {
            auto isForced = isRootFolded || groupRoot;
            auto layer = item_get(content.layers, track.id);
            if (!layer || (!isForced && std::ranges::none_of(track.frames, is_baked)))
                return {};
            Track plain = track;
            plain.frames.clear();
            Track baked = {.type = ItemType::LAYER, .id = -1, .isVisible = track.isVisible};
            auto isBaked = false;
            auto length = std::max(track_length_get(track), animation.frameNum);
            for (int time = 0; time < length; ++time)
            {
                auto frame = frame_generate(track, (float)time);
                std::optional<std::pair<Image, Vec2>> bakedImage{};
                Draw draw{.type = DrawType::LAYER,
                          .id = track.id,
                          .time = (float)time,
                          .frame = frame,
                          .source = &document.model,
                          .spritesheetId = layer->spritesheetId};
                if (frame.isVisible && (isForced || is_baked(frame)))
                {
                    for (auto* root :
                         std::initializer_list<const Track*>{isRootFolded ? &animation.root : nullptr, groupRoot})
                        if (root)
                        {
                            auto rootFrame = frame_generate(*root, (float)time);
                            draw.parent *= frame_parent_model_get(rootFrame);
                            draw.frame.tint *= rootFrame.tint;
                            draw.frame.colorOffset += rootFrame.colorOffset;
                        }
                    bakedImage = draw_bake(canvas, shader, document, draw, draw.frame.shaderId != -1);
                }
                if (!bakedImage)
                {
                    step_add(plain, frame);
                    step_add(baked, Frame{.isVisible = false});
                    continue;
                }
                // The image stands where it was drawn, its region found once the images are packed (kept in regionId).
                auto step = draw.frame;
                if (step.shaderId != -1)
                {
                    step.tint = Vec4(1.0f);
                    step.colorOffset = {};
                }
                step.crop = {};
                step.size = Vec2(bakedImage->first.size);
                step.pivot = {};
                step.position = bakedImage->second;
                step.scale = Frame{}.scale;
                step.skew = {};
                step.rotation = 0.0f;
                step.blur = 0.0f;
                step.shaderId = -1;
                step.regionId = image_index_get(std::move(bakedImage->first));
                step_add(baked, step);
                step_add(plain, Frame{.isVisible = false});
                isBaked = true;
            }
            if (!isBaked)
                return {};
            baked.id = baked_layer_id_get(track.id);
            track = std::move(plain);
            return baked;
        };

        // Each layer track is followed by its baked copy, if any. A skewed group root is folded into its tracks' bakes.
        animation.layers = entries_expand(std::move(animation.layers), [&](Track& track, TrackGroup* group) {
            auto baked = track_bake(track, group && is_skewed(group->root) ? &group->root : nullptr);
            std::vector<Track> tracks = {std::move(track)};
            if (baked)
                tracks.push_back(std::move(*baked));
            return tracks;
        });
        groups_each(animation.layers, [&](TrackGroup& group) {
            if (is_skewed(group.root))
                group.root = TrackGroup{}.root;
        });

        // A folded root's transform goes into the nulls too (their group roots first), and the root becomes plain.
        if (!isRootFolded)
            continue;
        items_each(animation.nulls, [&](Track& track, TrackGroup* group) {
            if (group)
                track_group_root_bake(track, group->root, animation.frameNum);
            track_group_root_bake(track, animation.root, animation.frameNum);
        });
        groups_each(animation.nulls, [](TrackGroup& group) { group.root = TrackGroup{}.root; });
        animation.root = Animation{}.root;
    }
    if (images.empty())
        return;

    std::vector<IVec2> sizes{};
    for (auto& image : images)
        sizes.push_back(image.size);
    IVec2 packedSize{};
    std::vector<IVec2> positions{};
    rects_pack(sizes, packedSize, positions);
    std::vector<ImagePart> parts;
    for (int i = 0; i < (int)images.size(); ++i)
        parts.push_back({.image = &images[i], .size = images[i].size, .destination = positions[i]});
    if (!Image::compose(parts, packedSize, false).write_png(path))
        return toast_log(Level::ERROR, TOAST_BAKE_WRITE_FAILED, pathUtf8);
    if (packedSize.x > BAKED_SIZE_MAX || packedSize.y > BAKED_SIZE_MAX)
        toast_log(Level::WARNING, TOAST_BAKE_LARGE, pathUtf8, packedSize.x, packedSize.y, BAKED_SIZE_MAX);
    content.spritesheets.push_back({.id = spritesheetId, .path = path.filename()});

    std::set<int> bakedIds;
    for (auto [layerId, bakedId] : bakedLayerIds)
        bakedIds.insert(bakedId);
    for (auto index : indices)
        items_each(game.animation_edit(index)->layers, [&](Track& track, auto*) {
            if (!bakedIds.contains(track.id))
                return;
            for (auto& frame : track.frames)
                if (frame.regionId != -1)
                {
                    frame.crop = Vec2(positions[frame.regionId]);
                    frame.regionId = -1;
                }
        });
    result = std::move(game);
}
