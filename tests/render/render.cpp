// Animations drawn by the editor's OpenGL renderer, compared with golden PNGs (ANM2ED_UPDATE_GOLDEN=1 rewrites them).
// Every render is also written to ANM2ED_RENDER_DIR (build/renders) so it can be looked at. Skipped without OpenGL.

#include <SDL3/SDL.h>
#include <doctest.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>

#include "audio.h"
#include "file.h"
#include "settings.h"
#include "canvas.h"
#include "embed.h"
#include "document.h"
#include "image.h"
#include "instance.h"
#include "render.h"

const std::filesystem::path RENDER_TEST_DIR = std::filesystem::path(ANM2ED_TEST_DIR) / "render";
const std::filesystem::path GOLDEN_DIR = RENDER_TEST_DIR / "golden";
const std::filesystem::path OUTPUT_DIR = ANM2ED_RENDER_DIR;
constexpr int CHANNEL_TOLERANCE = 8;
constexpr double MISMATCH_RATIO_MAX = 0.01;
constexpr float SCALE = 3.0f;
constexpr Vec4 BACKGROUND = {0.2f, 0.2f, 0.2f, 1.0f};

// Different OpenGL implementations round a little differently: a few pixels may differ, each only slightly.
static void golden_image_check(const std::string& name)
{
    auto actualPath = OUTPUT_DIR / name;
    auto goldenPath = GOLDEN_DIR / name;
    INFO("render: ", actualPath.string());
    if (std::getenv("ANM2ED_UPDATE_GOLDEN"))
    {
        std::filesystem::copy_file(actualPath, goldenPath, std::filesystem::copy_options::overwrite_existing);
        return;
    }
    Image actual(actualPath);
    Image golden(goldenPath);
    REQUIRE(golden.is_valid());
    REQUIRE(actual.size == golden.size);
    int mismatches = 0;
    for (std::size_t i = 0; i < actual.pixels.size(); ++i)
        mismatches += std::abs((int)actual.pixels[i] - (int)golden.pixels[i]) > CHANNEL_TOLERANCE;
    CHECK((double)mismatches / (double)actual.pixels.size() <= MISMATCH_RATIO_MAX);
}

struct RenderFixture
{
    HeadlessGL gl = {};
    std::optional<Document> document;
    std::optional<ShaderProgram> shader;
    std::optional<Canvas> canvas;
    const Animation* animation = nullptr;
    Vec4 bounds{};

    explicit RenderFixture(const char* file)
    {
        if (!gl.is_valid())
            return;
        std::filesystem::create_directories(OUTPUT_DIR);
        document.emplace(RENDER_TEST_DIR / file);
        auto& info = SHADERS[SHADER_TEXTURE];
        shader.emplace(info.vertex, info.fragment);
        canvas.emplace(Vec2(1.0f));
        animation = document->model.animation_get(0);
        bounds = animation_bounds_get(*document, *animation, true);
    }

    std::vector<uint8_t> frame_get(float time)
    {
        auto pixels =
            animation_frame_render(*canvas, *shader, *document, *animation, time, bounds, SCALE, BACKGROUND, true);
        pixels_unpremultiply(pixels);
        return pixels;
    }

    TVec4<uint8_t> pixel_get(const std::vector<uint8_t>& pixels, Vec2 point)
    {
        auto position = IVec2((point - Vec2(bounds.x, bounds.y)) * SCALE);
        auto index = ((std::size_t)position.y * (std::size_t)canvas->size.x + (std::size_t)position.x) * 4;
        return {pixels[index], pixels[index + 1], pixels[index + 2], pixels[index + 3]};
    }
};

TEST_CASE("frames draw where the animation puts them")
{
    RenderFixture fixture("spin.anm2");
    if (!fixture.gl.is_valid())
    {
        MESSAGE("skipped: no OpenGL context");
        return;
    }
    REQUIRE(fixture.document->is_valid());

    // At time 0 the red square is centered at (-16, 0) with its white dot up and left. The half-transparent blue
    // square (32 x 32, at the origin) covers the middle.
    auto pixels = fixture.frame_get(0.0f);
    auto red = fixture.pixel_get(pixels, {-20.0f, 4.0f});
    CHECK(red.r > 200);
    CHECK(red.g < 80);
    auto dot = fixture.pixel_get(pixels, {-18.5f, -2.5f});
    CHECK(dot.g > 200);
    auto blue = fixture.pixel_get(pixels, {12.0f, 12.0f});
    CHECK(blue.b > blue.r + 40);

    auto framePath = OUTPUT_DIR / "spin_frame_0.png";
    REQUIRE(Image::write_pixels_png(framePath, IVec2(fixture.canvas->size), pixels.data()));
    golden_image_check("spin_frame_0.png");
}

TEST_CASE("a recorded spritesheet holds every frame")
{
    RenderFixture fixture("spin.anm2");
    if (!fixture.gl.is_valid())
    {
        MESSAGE("skipped: no OpenGL context");
        return;
    }

    RecorderOptions options{.type = RENDER_SPRITESHEET,
                            .path = OUTPUT_DIR / "spin_sheet.png",
                            .start = 0,
                            .end = fixture.animation->frameNum - 1,
                            .animationFps = fixture.document->model.info.fps,
                            .fps = fixture.document->model.info.fps};
    REQUIRE(animation_record(options, *fixture.canvas, *fixture.shader, *fixture.document, *fixture.animation,
                             fixture.bounds, SCALE, BACKGROUND, true));
    Image sheet(options.path);
    auto layout = spritesheet_layout_get(0, 0, fixture.animation->frameNum);
    CHECK(sheet.size == IVec2(fixture.canvas->size) * IVec2(layout.columns, layout.rows));
    golden_image_check("spin_sheet.png");
}

TEST_CASE("a PNG sequence without a placeholder numbers each frame")
{
    RenderFixture fixture("spin.anm2");
    if (!fixture.gl.is_valid())
    {
        MESSAGE("skipped: no OpenGL context");
        return;
    }

    auto directory = OUTPUT_DIR / "spin_sequence";
    std::filesystem::remove_all(directory);
    RecorderOptions options{.type = RENDER_PNGS,
                            .path = directory,
                            .format = "spin.png",
                            .start = 0,
                            .end = fixture.animation->frameNum - 1,
                            .animationFps = fixture.document->model.info.fps,
                            .fps = fixture.document->model.info.fps};
    REQUIRE(animation_record(options, *fixture.canvas, *fixture.shader, *fixture.document, *fixture.animation,
                             fixture.bounds, SCALE, BACKGROUND, true));
    auto count = std::ranges::distance(std::filesystem::directory_iterator(directory));
    CHECK(count == fixture.animation->frameNum);
    CHECK(std::filesystem::exists(directory / "spin_000000.png"));
}

TEST_CASE("animation layers draw their child animations")
{
    RenderFixture fixture("nested.anm2");
    if (!fixture.gl.is_valid())
    {
        MESSAGE("skipped: no OpenGL context");
        return;
    }
    REQUIRE(fixture.document->is_valid());

    // The library's red square at half size starts on its fifth frame, centered at (48, 0). This file's Inner holds its
    // red square at (-40, 0).
    auto pixels = fixture.frame_get(0.0f);
    for (auto point : {Vec2(48.0f, 0.0f), Vec2(-40.0f, 0.0f)})
    {
        auto red = fixture.pixel_get(pixels, point);
        CHECK(red.r > 200);
        CHECK(red.g < 80);
    }

    REQUIRE(Image::write_pixels_png(OUTPUT_DIR / "nested_frame_0.png", IVec2(fixture.canvas->size), pixels.data()));
    golden_image_check("nested_frame_0.png");
}

TEST_CASE("a blurred layer spreads past its crop like Flash's blur filter")
{
    RenderFixture fixture("spin.anm2");
    if (!fixture.gl.is_valid())
    {
        MESSAGE("skipped: no OpenGL context");
        return;
    }
    REQUIRE(fixture.document->is_valid());

    // The red square (16 x 16 at (-16, 0)) sharp leaves the canvas clear 2 above its top edge and solid 1 inside it. An
    // 8 pixel box spreads a third of it out and takes as much from inside.
    constexpr Vec2 OUTSIDE{-20.0f, -10.0f}, INSIDE{-20.0f, -7.0f};
    auto sharp = fixture.frame_get(0.0f);
    fixture.document->model.frame_edit({0, LAYER, 0, 0})->blur = 8.0f;
    fixture.animation = fixture.document->model.animation_get(0);
    auto blurred = fixture.frame_get(0.0f);
    CHECK(fixture.pixel_get(blurred, OUTSIDE).r > fixture.pixel_get(sharp, OUTSIDE).r + 40);
    CHECK(fixture.pixel_get(blurred, INSIDE).r + 40 < fixture.pixel_get(sharp, INSIDE).r);

    REQUIRE(Image::write_pixels_png(OUTPUT_DIR / "spin_blur_frame_0.png", IVec2(fixture.canvas->size), blurred.data()));
}

TEST_CASE("SVG spritesheets draw sharp at any scale")
{
    RenderFixture fixture("vector.anm2");
    if (!fixture.gl.is_valid())
    {
        MESSAGE("skipped: no OpenGL context");
        return;
    }
    REQUIRE(fixture.document->is_valid());

    // Drawn at 600% (200% frames at a 3x render), the circle's edge stays within a pixel or two: just inside it is
    // solid red, just outside it clear.
    auto pixels = fixture.frame_get(0.0f);
    auto inside = fixture.pixel_get(pixels, {-20.0f + 13.0f, 0.0f});
    auto outside = fixture.pixel_get(pixels, {-20.0f + 14.6f, 0.0f});
    CHECK(inside.r > 200);
    CHECK(outside.r < 80);

    REQUIRE(Image::write_pixels_png(OUTPUT_DIR / "vector_frame_0.png", IVec2(fixture.canvas->size), pixels.data()));
    golden_image_check("vector_frame_0.png");
}

TEST_CASE("converting SVG spritesheets writes PNGs beside them")
{
    auto directory = std::filesystem::temp_directory_path() / "anm2ed_vector_convert";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);
    for (auto name : {"vector.anm2", "vector.svg"})
        std::filesystem::copy_file(RENDER_TEST_DIR / name, directory / name);

    Document document(directory / "vector.anm2");
    REQUIRE(document.is_valid());
    document.spritesheets_vector_convert({0});
    CHECK(document.model.content.spritesheets.front().path == "vector.png");
    CHECK(std::filesystem::exists(directory / "vector.png"));
    REQUIRE(document.texture_get(0));
    CHECK_FALSE(document.texture_get(0)->is_vector());
    CHECK(spritesheets_vector_used_get(document.model).empty());

    document.undo();
    CHECK(document.model.content.spritesheets.front().path == "vector.svg");
    CHECK(document.texture_get(0)->is_vector());
    std::filesystem::remove_all(directory);
}

TEST_CASE("a Rebirth save flattens animation layers for the game")
{
    auto directory = std::filesystem::temp_directory_path() / "anm2ed_flatten";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory / "library");
    std::filesystem::copy_file(RENDER_TEST_DIR / "nested.anm2", directory / "nested.anm2");
    for (auto name : {"spin.anm2", "sheet.png"})
        std::filesystem::copy_file(RENDER_TEST_DIR / name, directory / "library" / name);
    auto text = std::string{};
    {
        std::ifstream file(directory / "nested.anm2");
        text.assign(std::istreambuf_iterator<char>(file), {});
    }
    text.replace(text.find("Path=\"spin.anm2\""), std::string("Path=\"spin.anm2\"").size(),
                 "Path=\"library/spin.anm2\"");
    std::ofstream(directory / "nested.anm2") << text;

    Document document(directory / "nested.anm2");
    REQUIRE(document.is_valid());
    REQUIRE(document.save(directory / "rebirth.anm2"));
    std::ifstream file(directory / "rebirth.anm2");
    std::string saved(std::istreambuf_iterator<char>(file), {});
    auto game = saved.substr(0, saved.find("<SourceDocument"));
    CHECK(game.find("Name=\"spin/red\"") != std::string::npos);
    CHECK(game.find("Path=\"library/sheet.png\"") != std::string::npos);
    CHECK(game.find(" Animation=\"") == std::string::npos);
    CHECK(saved.find(" Animation=\"Spin\"") != std::string::npos);
    std::filesystem::remove_all(directory);
}

// The game's copy draws what the editor does: skew, blur and a skewed root baked into a spritesheet beside the save.
TEST_CASE("the game's copy bakes skew and blur into a spritesheet that draws the same")
{
    HeadlessGL gl = {};
    if (!gl.is_valid())
        return;
    auto directory = std::filesystem::temp_directory_path() / "anm2ed_bake";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);
    std::filesystem::copy_file(RENDER_TEST_DIR / "sheet.png", directory / "sheet.png");
    std::ofstream(directory / "bake.anm2") << R"({ "sheets": [ { "path": "sheet.png" } ],
    "layers": [ { "name": "skewed" }, { "name": "blurred" } ],
    "animations": {
      "Layers": { "length": 6, "layers": [
        { "layer": 0, "frames": [
          { "size": [16, 16], "pivot": [8, 8], "pos": [-12, 0], "skew": [40, 0], "delay": 3, "ease": "lerp" },
          { "size": [16, 16], "pivot": [8, 8], "pos": [-12, 4], "skew": [0, 30], "rotation": 45, "delay": 3 } ] },
        { "layer": 1, "frames": [
          { "crop": [16, 0], "size": [16, 16], "pivot": [8, 8], "pos": [12, 0], "blur": 4, "delay": 6 } ] } ] },
      "Root": { "length": 2,
        "root": [ { "pos": [2, 3], "skew": [25, 10], "scale": [150, 100], "delay": 2 } ],
        "layers": [ { "layer": 1, "frames": [ { "crop": [16, 0], "size": [16, 16], "pivot": [8, 8], "delay": 2 } ] } ] } } })";

    Document document(directory / "bake.anm2");
    REQUIRE(document.is_valid());
    REQUIRE(document.save(directory / "saved.anm2"));
    CHECK(std::filesystem::exists(directory / "saved.baked.png"));
    std::string saved;
    {
        std::ifstream file(directory / "saved.anm2");
        saved.assign(std::istreambuf_iterator<char>(file), {});
    }
    auto game = saved.substr(0, saved.find("    <SourceDocument")) + "</AnimatedActor>\n";
    CHECK(game.find("SkewX") == std::string::npos);
    CHECK(game.find("Blur") == std::string::npos);
    std::ofstream(directory / "game.anm2") << game;
    Document gameDocument(directory / "game.anm2");
    REQUIRE(gameDocument.is_valid());

    auto& info = SHADERS[SHADER_TEXTURE];
    ShaderProgram shader(info.vertex, info.fragment);
    Canvas canvas(Vec2(1.0f));
    for (int index = 0; index < document.model.animations_count_get(); ++index)
    {
        auto& animation = *document.model.animation_get(index);
        auto& gameAnimation = *gameDocument.model.animation_get(index);
        INFO("animation: ", animation.name);
        // On whole pixels, so the baked images land on the pixels they were drawn on.
        auto bounds = animation_bounds_get(document, animation, true);
        bounds = Vec4(vec_floor(Vec2(bounds)) - 8.0f, vec_ceil(Vec2(bounds.z, bounds.w)) + 16.0f);
        for (int time = 0; time < animation.frameNum; ++time)
        {
            INFO("time: ", time);
            auto expected = animation_frame_render(canvas, shader, document, animation, (float)time, bounds, 1.0f,
                                                   Vec4(0.0f), true);
            auto actual = animation_frame_render(canvas, shader, gameDocument, gameAnimation, (float)time, bounds, 1.0f,
                                                 Vec4(0.0f), true);
            REQUIRE(actual.size() == expected.size());
            CHECK(std::ranges::any_of(expected, [](uint8_t channel) { return channel != 0; }));
            int mismatches = 0;
            for (std::size_t i = 0; i < actual.size(); ++i)
                mismatches += std::abs((int)actual[i] - (int)expected[i]) > CHANNEL_TOLERANCE;
            CHECK((double)mismatches / (double)actual.size() <= MISMATCH_RATIO_MAX);
        }
    }

    // Ignore and Save leaves them out: no baked layers or spritesheet.
    document.model.info.isIsaacIgnored = true;
    REQUIRE(document.save(directory / "ignored.anm2"));
    CHECK_FALSE(std::filesystem::exists(directory / "ignored.baked.png"));
    std::ifstream ignored(directory / "ignored.anm2");
    CHECK(std::string(std::istreambuf_iterator<char>(ignored), {}).find("(baked)") == std::string::npos);
    if (!std::getenv("ANM2ED_KEEP"))
        std::filesystem::remove_all(directory);
}

// Sounds decode from WAV and Ogg Vorbis, and a render's audio starts each one on its frame.
TEST_CASE("audio decodes and mixes")
{
    AudioData music(MUSIC_ABOUT_OGG.data, MUSIC_ABOUT_OGG.size);
    auto musicMix = audio_mix({{&music}}, 30);
    CHECK(musicMix.size() > (std::size_t)AUDIO_FREQUENCY * AUDIO_CHANNELS);
    CHECK(std::ranges::any_of(musicMix, [](float sample) { return sample != 0.0f; }));

    constexpr int FREQUENCY = 22050;
    constexpr int FRAMES = 100;
    std::vector<unsigned char> wav = {'R', 'I', 'F', 'F', 0,  0, 0,   0,   'W', 'A', 'V', 'E', 'f', 'm', 't',
                                      ' ', 16,  0,   0,   0,  1, 0,   1,   0,   0,   0,   0,   0,   0,   0,
                                      0,   0,   2,   0,   16, 0, 'd', 'a', 't', 'a', 0,   0,   0,   0};
    auto put = [&](int offset, std::uint32_t value, int size) {
        for (int i = 0; i < size; ++i)
            wav[offset + i] = (unsigned char)(value >> (i * 8));
    };
    put(24, FREQUENCY, 4);
    put(28, FREQUENCY * 2, 4);
    put(40, FRAMES * 2, 4);
    for (int i = 0; i < FRAMES; ++i)
    {
        wav.push_back(0);
        wav.push_back(0x40);
    }
    put(4, (std::uint32_t)wav.size() - 8, 4);
    AudioData sound(wav.data(), wav.size());
    auto mix = audio_mix({{}, {&sound}}, 30);
    auto start = (std::size_t)(AUDIO_FREQUENCY / 30) * AUDIO_CHANNELS;
    REQUIRE(mix.size() > start + 2);
    CHECK(mix[start - 2] == 0.0f);
    CHECK(mix[start + 2] == doctest::Approx(0.5f).epsilon(0.05));
}

// A second editor hands its paths to the first, which has them queued once it answers, and wakes its event loop.
// Skipped when the port is already taken.
TEST_CASE("a second instance hands its paths to the first")
{
    if (!SDL_Init(SDL_INIT_EVENTS))
        return;
    if (instance_start({}) != INSTANCE_FIRST)
        return;
    CHECK(instance_start({"a.anm2", "b.anm2"}) == INSTANCE_FORWARDED);
    CHECK(instance_paths_take() == std::vector<std::string>{"a.anm2", "b.anm2"});
    CHECK_FALSE(instance_paths_take());
    SDL_Event event = {};
    CHECK((SDL_WaitEventTimeout(&event, 1000) && event.type == SDL_EVENT_USER));
    CHECK(instance_start({}) == INSTANCE_FORWARDED);
    CHECK(instance_paths_take() == std::vector<std::string>{});
    instance_stop();
}

TEST_CASE("hand-edited settings out of an enum's range load as their defaults")
{
    auto path = std::filesystem::temp_directory_path() / "anm2ed_settings_test.ini";
    REQUIRE(file_text_save(path, "[Settings]\ntool=99\nrenderType=-3\ntheme=1\n"));
    Settings settings(path);
    CHECK(settings.tool == Settings{}.tool);
    CHECK(settings.renderType == Settings{}.renderType);
    CHECK(settings.theme == 1);
    std::filesystem::remove(path);
}
