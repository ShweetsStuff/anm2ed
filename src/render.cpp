#include "render.hpp"

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
#include <glad/glad.h>

#include "audio.hpp"
#include "canvas.hpp"
#include "document.hpp"
#include "file.hpp"
#include "log.hpp"
#include "model/frames.hpp"
#include "pack.hpp"
#include "path.hpp"
#include "process.hpp"
#include "texture.hpp"
#include "toast.hpp"

using namespace anm2ed::imgui;
using namespace anm2ed::resource;
using namespace anm2ed::util;

namespace anm2ed
{
  bool animation_render(const std::filesystem::path& ffmpegPath, const std::filesystem::path& path,
                        const std::vector<std::filesystem::path>& framePaths, AudioStream& audioStream,
                        render::Type type, int fps)
  {
    if (framePaths.empty() || ffmpegPath.empty() || path.empty()) return false;
    fps = std::max(fps, 1);

    auto pathString = path::to_utf8(path);
    auto ffmpegPathString = path::to_utf8(ffmpegPath);
    auto loggerPath = logger.path();
    auto loggerPathString = path::to_utf8(loggerPath);
#if _WIN32
    auto ffmpegTempPath = loggerPath.parent_path() / "ffmpeg_log.temp.txt";
    auto ffmpegTempPathString = path::to_utf8(ffmpegTempPath);
    std::error_code ffmpegTempError;
    std::filesystem::remove(ffmpegTempPath, ffmpegTempError);
#endif

    std::filesystem::path audioPath{};
    std::string audioInputArguments{};
    std::string audioOutputArguments{"-an"};
    std::string command{};
    auto temporaryDirectory = framePaths.front().parent_path();
    if (temporaryDirectory.empty()) temporaryDirectory = std::filesystem::temp_directory_path();

    auto audio_remove = [&]()
    {
      if (!audioPath.empty())
      {
        std::error_code ec;
        std::filesystem::remove(audioPath, ec);
      }
    };

    if (type != render::GIF && !audioStream.stream.empty() && audioStream.spec.freq > 0 &&
        audioStream.spec.channels > 0)
    {
      auto tempFilenameUtf8 =
          std::format("anm2ed_audio_{}_{}.f32", std::hash<std::string>{}(pathString), SDL_GetTicks());
      audioPath = temporaryDirectory / path::from_utf8(tempFilenameUtf8);

      auto data = (const char*)audioStream.stream.data();
      auto byteCount = audioStream.stream.size() * sizeof(float);
      if (file::write_string(audioPath, std::string_view(data, byteCount), "wb"))
      {
        auto sampleRate = std::max(audioStream.spec.freq, 1);
        auto channels = std::max(audioStream.spec.channels, 1);
        auto audioDurationFilter = std::string{};
        auto frameCount = (double)std::max((int)framePaths.size(), 1);
        auto expectedDurationSeconds = frameCount / (double)fps;
        if (expectedDurationSeconds > 0.0)
          audioDurationFilter += std::format("apad,atrim=duration={:.9f}", expectedDurationSeconds);

        audioInputArguments =
            std::format("-f f32le -ar {0} -ac {1} -i \"{2}\"", sampleRate, channels, path::to_utf8(audioPath));

        switch (type)
        {
          case render::WEBM:
            audioOutputArguments = "-c:a libopus -b:a 160k -shortest";
            break;
          case render::OGV:
            audioOutputArguments = "-c:a libvorbis -q:a 4 -shortest";
            break;
          case render::MP4:
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

    auto framePattern = temporaryDirectory / path::from_utf8("frame_%06d.png");
    auto framePatternString = path::to_utf8(framePattern);
    command =
        std::format("\"{0}\" -y -framerate {1} -start_number 0 -i \"{2}\"", ffmpegPathString, fps, framePatternString);

    if (!audioInputArguments.empty()) command += " " + audioInputArguments;
    command += std::format(" -fps_mode cfr -r {}", fps);

    switch (type)
    {
      case render::GIF:
        command += " -lavfi \"split[s0][s1];[s0]palettegen=stats_mode=full:reserve_transparent=1[p];"
                   "[s1][p]paletteuse=dither=floyd_steinberg:alpha_threshold=128\""
                   " -loop 0";
        command += std::format(" \"{}\"", pathString);
        break;
      case render::WEBM:
        command += " -c:v libvpx-vp9 -crf 30 -b:v 0 -pix_fmt yuva420p -row-mt 1 -threads 0 -speed 2 -auto-alt-ref 0";
        if (!audioOutputArguments.empty()) command += " " + audioOutputArguments;
        command += std::format(" \"{}\"", pathString);
        break;
      case render::OGV:
        command += " -vf \"format=yuv420p,scale=trunc(iw/2)*2:trunc(ih/2)*2\" -c:v libtheora -q:v 7";
        if (!audioOutputArguments.empty()) command += " " + audioOutputArguments;
        command += std::format(" \"{}\"", pathString);
        break;
      case render::MP4:
        command += " -vf \"format=yuv420p,scale=trunc(iw/2)*2:trunc(ih/2)*2\" -c:v libx265 -crf 20 -preset slow"
                   " -tag:v hvc1 -movflags +faststart";
        if (!audioOutputArguments.empty()) command += " " + audioOutputArguments;
        command += std::format(" \"{}\"", pathString);
        break;
      default:
      {
        return false;
      }
    }

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
}

namespace anm2ed::render
{
  constexpr int TEMP_DIRECTORY_ATTEMPTS = 1000;
  constexpr glm::vec4 BOUNDS_DEFAULT = {-32.0f, -32.0f, 64.0f, 64.0f};
  constexpr int ALPHA_OPAQUE = 255;
  constexpr float CHANNEL_MAX = 255.0f;

  std::filesystem::path frame_filename_get(const std::filesystem::path& format, int index, Type type)
  {
    auto fallback = path::from_utf8(std::format("frame_{:06}.png", index));
    if (type != PNGS) return fallback;
    try
    {
      auto filename = path::from_utf8(std::vformat(path::to_utf8(format), std::make_format_args(index))).filename();
      if (filename.empty()) return fallback;
      if (filename.extension().empty()) filename.replace_extension(EXTENSIONS[SPRITESHEET]);
      return filename;
    }
    catch (...)
    {
      return fallback;
    }
  }

  // PNG sequences go straight to their folder; other outputs collect frames in a temporary folder beside the output.
  std::filesystem::path frames_directory_create(const RecorderOptions& options)
  {
    std::error_code ec{};
    if (options.type == PNGS)
    {
      std::filesystem::create_directories(options.path, ec);
      return options.path;
    }
    auto directory = options.path.parent_path();
    if (directory.empty()) directory = std::filesystem::current_path();
    std::filesystem::create_directories(directory, ec);
    auto timestamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    for (int suffix = 0; suffix < TEMP_DIRECTORY_ATTEMPTS; ++suffix)
    {
      auto temporary = directory / path::from_utf8(std::format(".anm2ed_render_tmp_{}_{}", timestamp, suffix));
      if (std::filesystem::create_directories(temporary, ec)) return temporary;
    }
    return {};
  }

  // Rendered pixels are premultiplied; outputs on a transparent background want straight alpha.
  void pixels_unpremultiply(std::vector<uint8_t>& pixels)
  {
    for (std::size_t index = 0; index + 3 < pixels.size(); index += image::CHANNELS)
    {
      auto alpha = pixels[index + 3];
      if (alpha == ALPHA_OPAQUE) continue;
      for (int channel = 0; channel < 3; ++channel)
        pixels[index + channel] =
            alpha == 0
                ? 0
                : (uint8_t)glm::clamp(std::round(pixels[index + channel] * CHANNEL_MAX / alpha), 0.0f, CHANNEL_MAX);
    }
  }

  void sounds_detach(const Document& document, MIX_Mixer* mixer)
  {
    for (auto sound : document.sounds_get())
      audio::track_detach(*sound, mixer);
  }

  // Mixes each frame's trigger sounds into one stream at the render frame rate.
  bool audio_stream_generate(AudioStream& audioStream, const Document& document,
                             const std::vector<std::vector<const resource::AudioData*>>& frameSounds, int fps)
  {
    audioStream.stream.clear();
    if (frameSounds.empty() || fps <= 0) return true;

    SDL_AudioSpec mixSpec = audioStream.spec;
    mixSpec.format = SDL_AUDIO_F32;
    auto mixer = MIX_CreateMixer(&mixSpec);
    if (!mixer) return false;

    auto channels = std::max(mixSpec.channels, 1);
    auto framesPerStep = (double)std::max(mixSpec.freq, 1) / (double)fps;
    auto accumulator = 0.0;
    std::vector<float> buffer{};
    auto isGenerated = true;
    for (const auto& sounds : frameSounds)
    {
      for (auto sound : sounds)
        audio::play(*sound, false, mixer);
      accumulator += framesPerStep;
      auto sampleFrames = std::max((int)std::floor(accumulator), 1);
      accumulator -= (double)sampleFrames;
      buffer.resize((std::size_t)sampleFrames * (std::size_t)channels);
      if (!(isGenerated = MIX_Generate(mixer, buffer.data(), (int)(buffer.size() * sizeof(float))))) break;
      audioStream.stream.insert(audioStream.stream.end(), buffer.begin(), buffer.end());
    }
    sounds_detach(document, mixer);
    MIX_DestroyMixer(mixer);
    if (!isGenerated) audioStream.stream.clear();
    return isGenerated;
  }

  HeadlessGL::HeadlessGL()
  {
    if (!SDL_Init(SDL_INIT_VIDEO)) return;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    window = SDL_CreateWindow("anm2ed", 1, 1, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (window) context = SDL_GL_CreateContext(window);
    if (!context || !gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
      if (context) SDL_GL_DestroyContext(context);
      context = nullptr;
      return;
    }
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
  }

  HeadlessGL::~HeadlessGL()
  {
    if (context) SDL_GL_DestroyContext(context);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
  }

  bool HeadlessGL::is_valid() const { return context != nullptr; }

  glm::vec4 animation_bounds_get(const Document& document, const model::Animation& animation, bool isRootTransform)
  {
    auto bounds = model::animation_rect(document.model, animation, isRootTransform, document.library_resolver_get());
    return bounds != glm::vec4(-1.0f) ? bounds : BOUNDS_DEFAULT;
  }

  std::vector<uint8_t> animation_frame_render(Canvas& canvas, Shader& shader, Document& document,
                                              const model::Animation& animation, float time, glm::vec4 bounds,
                                              float scale, glm::vec4 background, bool isRootTransform)
  {
    canvas.size_set(glm::vec2(glm::max(glm::ivec2(glm::ceil(glm::vec2(bounds.z, bounds.w) * scale)), glm::ivec2(1))));
    float zoom{};
    glm::vec2 pan{};
    canvas.set_to_rect(zoom, pan, bounds);
    canvas.bind();
    canvas.viewport_set();
    canvas.clear(glm::vec4(glm::vec3(background) * background.a, background.a));
    canvas.layers_render(
        shader, document.model, animation,
        {.time = time, .isRootTransform = isRootTransform, .library_get = document.library_resolver_get()},
        canvas.transform_get(zoom, pan),
        [&](const model::Draw& draw) { return document.draw_image_get(draw, scale * model::draw_scale_get(draw)); },
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
    soundTime = -1;
    directory = frames_directory_create(options);
    return !directory.empty();
  }

  bool Recorder::is_done() const { return index >= count; }

  float Recorder::time_get() const { return frame_time_get(options.start, index, options.animationFps, options.fps); }

  // A trigger sound is taken once per animation frame, however many render frames it spans.
  bool Recorder::is_sound_frame() const { return (int)std::floor(time_get()) != soundTime; }

  bool Recorder::frame_capture(std::vector<uint8_t> pixels, glm::ivec2 size,
                               std::vector<const resource::AudioData*> frameSounds)
  {
    soundTime = (int)std::floor(time_get());
    sounds.push_back(std::move(frameSounds));
    if (options.isIsolated || options.isBounded) pixels_unpremultiply(pixels);
    auto framePath = directory / frame_filename_get(options.format, index, options.type);
    if (!Image::write_pixels_png(framePath, size, pixels.data())) return false;
    frames.push_back(framePath);
    ++index;
    return true;
  }

  void Recorder::finish(const Document& document, AudioStream& audioStream)
  {
    auto pathString = path::to_utf8(options.path);
    if (options.type == PNGS)
      toast_log(frames.empty() ? Level::ERROR : Level::INFO,
                frames.empty() ? TOAST_EXPORT_RENDERED_FRAMES_FAILED : TOAST_EXPORT_RENDERED_FRAMES, pathString);
    else if (options.type == SPRITESHEET)
    {
      auto layout = spritesheet_layout_get(options.rows, options.columns, (int)frames.size());
      auto first = frames.empty() ? Image{} : Image(frames.front());
      if (frames.empty())
        toast_log(Level::WARNING, TOAST_SPRITESHEET_NO_FRAMES);
      else if (!first.is_valid())
        toast_log(Level::ERROR, TOAST_SPRITESHEET_EMPTY);
      else
      {
        Image sheet{};
        sheet.size = first.size * glm::ivec2(layout.columns, layout.rows);
        sheet.pixels.resize((std::size_t)sheet.size.x * sheet.size.y * image::CHANNELS);
        for (int i = 0; i < (int)frames.size() && i < layout.rows * layout.columns; ++i)
          if (auto frame = Image(frames[i]); frame.size == first.size)
            sheet.paste(frame, first.size * glm::ivec2(i % layout.columns, i / layout.columns));
        auto isSaved = sheet.write_png(options.path);
        toast_log(isSaved ? Level::INFO : Level::ERROR,
                  isSaved ? TOAST_EXPORT_SPRITESHEET : TOAST_EXPORT_SPRITESHEET_FAILED, pathString);
      }
    }
    else
    {
      audioStream.stream.clear();
      if (options.isSound && options.type != GIF && !audio_stream_generate(audioStream, document, sounds, options.fps))
      {
        toasts.push(localize.get(TOAST_EXPORT_RENDERED_ANIMATION_FAILED));
        logger.error("Failed to generate deterministic render audio stream; exporting without audio.");
      }
      auto isRendered =
          animation_render(options.ffmpegPath, options.path, frames, audioStream, options.type, options.fps);
      toast_log(isRendered ? Level::INFO : Level::ERROR,
                isRendered ? TOAST_EXPORT_RENDERED_ANIMATION : TOAST_EXPORT_RENDERED_ANIMATION_FAILED, pathString);
    }
    cancel();
  }

  // Stops; the temporary frame folder is removed (a PNG sequence's own folder is kept).
  void Recorder::cancel()
  {
    std::error_code ec{};
    if (options.type != PNGS && !directory.empty()) std::filesystem::remove_all(directory, ec);
    directory.clear();
    frames.clear();
    sounds.clear();
    index = count = 0;
  }

  bool animation_record(const RecorderOptions& recorderOptions, Canvas& canvas, Shader& shader, Document& document,
                        const model::Animation& animation, glm::vec4 bounds, float scale, glm::vec4 background,
                        bool isRootTransform)
  {
    auto options = recorderOptions;
    options.isIsolated = options.isBounded = true;
    Recorder recorder{};
    if (!recorder.start(options)) return false;
    while (!recorder.is_done())
    {
      auto pixels = animation_frame_render(canvas, shader, document, animation, recorder.time_get(), bounds, scale,
                                           background, isRootTransform);
      if (recorder.frame_capture(std::move(pixels), glm::ivec2(canvas.size))) continue;
      recorder.cancel();
      return false;
    }
    AudioStream audioStream(nullptr);
    recorder.finish(document, audioStream);
    return path::is_exist(options.path);
  }

  constexpr float BAKE_MARGIN = 1.0f;
  constexpr int BAKED_SIZE_MAX = 4096;

  // A layer draw alone at canvas pixels, cut to what it can cover (its blur's reach too): its image (straight alpha) and
  // the canvas position of the image's top-left. Its tint and color offset go into the pixels only when `isColored`.
  std::optional<std::pair<Image, glm::vec2>> draw_bake(Canvas& canvas, Shader& shader, Document& document,
                                                       const model::Draw& draw, bool isColored)
  {
    auto image = document.draw_image_get(draw, model::draw_scale_get(draw));
    if (!image || !image->is_valid() || draw.frame.size == glm::vec2()) return {};
    auto quad = draw.parent * model::draw_quad_model_get(draw);
    glm::vec2 low{std::numeric_limits<float>::max()};
    glm::vec2 high{std::numeric_limits<float>::lowest()};
    for (auto corner : {glm::vec2(0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec2(1.0f)})
    {
      auto point = glm::vec2(quad * glm::vec4(corner, 0.0f, 1.0f));
      low = glm::min(low, point);
      high = glm::max(high, point);
    }
    auto reach = draw.frame.blur + BAKE_MARGIN;
    low = glm::floor(low - reach);
    high = glm::ceil(high + reach);
    auto size = glm::ivec2(high - low);

    canvas.size_set(glm::vec2(size));
    float zoom{};
    glm::vec2 pan{};
    canvas.set_to_rect(zoom, pan, glm::vec4(low, glm::vec2(size)));
    canvas.bind();
    canvas.viewport_set();
    canvas.clear(glm::vec4(0.0f));
    auto custom = document.shader_get(draw.frame.shaderId);
    canvas.layer_render(custom ? *custom : shader, texture::id_get(*image), image->unit_size_get(),
                        canvas.transform_get(zoom, pan), draw, isColored ? draw.frame.tint : glm::vec4(1.0f),
                        isColored ? draw.frame.colorOffset : glm::vec4(0.0f));
    auto pixels = canvas.pixels_get();
    canvas.unbind();
    pixels_unpremultiply(pixels);
    return std::pair{Image(pixels.data(), size), low};
  }

  // The game's copy with the layer frames it can't draw baked into images: skewed, blurred or shaded ones, and every
  // layer frame under a skewed root or group root, which then goes into the images (the root becomes plain). A layer has one spritesheet in the game, so baked frames stand on a copy of their layer
  // ("name (baked)") right after it, one frame per step, while the layer hides them. The images (each once) are packed
  // into one spritesheet written to `path`, beside the document; when it can't be, `result` keeps its frames.
  void frames_bake(Document& document, model::Model& result, const std::filesystem::path& path)
  {
    auto game = result;
    auto is_baked = [](const model::Frame& frame)
    { return frame.skew != glm::vec2() || frame.blur > 0.0f || frame.shaderId != -1; };
    auto is_skewed = [](const model::Track& root)
    { return std::ranges::any_of(root.frames, [](const model::Frame& frame) { return frame.skew != glm::vec2(); }); };

    std::vector<int> indices{};
    for (int i = 0; i < game.animations_count_get(); ++i)
    {
      auto& animation = *game.animation_get(i);
      auto isNeeded = is_skewed(animation.root);
      model::tracks_each(
          animation.layers, [&](const model::Track& track, const model::TrackGroup* group)
          { isNeeded |= (group && is_skewed(group->root)) || std::ranges::any_of(track.frames, is_baked); });
      if (isNeeded) indices.push_back(i);
    }
    auto pathUtf8 = path::to_utf8(path);
    if (indices.empty()) return;
    if (!SDL_GL_GetCurrentContext()) return toast_log(Level::WARNING, TOAST_BAKE_NO_OPENGL, pathUtf8);

    auto& textureInfo = shader::SHADERS[shader::TEXTURE];
    Shader shader(textureInfo.vertex, textureInfo.fragment);
    Canvas canvas(glm::vec2(1.0f));
    auto& content = game.content;
    auto spritesheetId = model::item_next_id_get(content.spritesheets);

    std::vector<Image> images{};
    std::map<std::size_t, std::vector<int>> imageHashes{};
    auto image_index_get = [&](Image image)
    {
      auto hash =
          std::hash<std::string_view>{}(std::string_view((const char*)image.pixels.data(), image.pixels.size()));
      for (auto index : imageHashes[hash])
        if (images[index].size == image.size && images[index].pixels == image.pixels) return index;
      imageHashes[hash].push_back((int)images.size());
      images.push_back(std::move(image));
      return (int)images.size() - 1;
    };
    std::map<int, int> bakedLayerIds{};
    auto baked_layer_id_get = [&](int layerId)
    {
      auto [it, isNew] = bakedLayerIds.try_emplace(layerId);
      if (!isNew) return it->second;
      auto layer = model::item_get(content.layers, layerId);
      it->second = model::item_next_id_get(content.layers);
      content.layers.push_back({.id = it->second,
                                .name = (layer ? layer->name : std::string{}) + " (baked)",
                                .spritesheetId = spritesheetId});
      return it->second;
    };
    // Steps alike join into one frame.
    auto step_add = [](model::Track& track, model::Frame frame)
    {
      frame.duration = FRAME_DURATION_MIN;
      frame.interpolation = Interpolation::NONE;
      if (!track.frames.empty())
      {
        auto last = track.frames.back();
        last.uid = frame.uid;
        last.duration = frame.duration;
        if (last == frame) return (void)++track.frames.back().duration;
      }
      track.frames.push_back(frame);
    };

    for (auto index : indices)
    {
      auto& animation = *game.animation_edit(index);
      auto isRootFolded = is_skewed(animation.root);
      // A layer track's baked copy, the track left with the rest; nothing when no frame of it bakes.
      auto track_bake = [&](model::Track& track, const model::Track* groupRoot) -> std::optional<model::Track>
      {
        auto isForced = isRootFolded || groupRoot;
        auto layer = model::item_get(content.layers, track.id);
        if (!layer || (!isForced && std::ranges::none_of(track.frames, is_baked))) return {};
        model::Track plain{track};
        plain.frames.clear();
        model::Track baked{.type = ItemType::LAYER, .id = -1, .isVisible = track.isVisible};
        auto isBaked = false;
        auto length = std::max(model::track_length_get(track), animation.frameNum);
        for (int time = 0; time < length; ++time)
        {
          auto frame = model::frame_generate(track, (float)time);
          std::optional<std::pair<Image, glm::vec2>> bakedImage{};
          model::Draw draw{.type = model::DrawType::LAYER,
                           .id = track.id,
                           .time = (float)time,
                           .frame = frame,
                           .source = &document.model,
                           .spritesheetId = layer->spritesheetId};
          if (frame.isVisible && (isForced || is_baked(frame)))
          {
            for (auto* root :
                 std::initializer_list<const model::Track*>{isRootFolded ? &animation.root : nullptr, groupRoot})
              if (root)
              {
                auto rootFrame = model::frame_generate(*root, (float)time);
                draw.parent *= model::frame_parent_model_get(rootFrame);
                draw.frame.tint *= rootFrame.tint;
                draw.frame.colorOffset += rootFrame.colorOffset;
              }
            bakedImage = draw_bake(canvas, shader, document, draw, draw.frame.shaderId != -1);
          }
          if (!bakedImage)
          {
            step_add(plain, frame);
            step_add(baked, model::Frame{.isVisible = false});
            continue;
          }
          // The image stands where it was drawn, its region found once the images are packed (kept in regionId).
          auto step = draw.frame;
          if (step.shaderId != -1)
          {
            step.tint = glm::vec4(1.0f);
            step.colorOffset = {};
          }
          step.crop = {};
          step.size = glm::vec2(bakedImage->first.size);
          step.pivot = {};
          step.position = bakedImage->second;
          step.scale = model::Frame{}.scale;
          step.skew = {};
          step.rotation = 0.0f;
          step.blur = 0.0f;
          step.shaderId = -1;
          step.regionId = image_index_get(std::move(bakedImage->first));
          step_add(baked, step);
          step_add(plain, model::Frame{.isVisible = false});
          isBaked = true;
        }
        if (!isBaked) return {};
        baked.id = baked_layer_id_get(track.id);
        track = std::move(plain);
        return baked;
      };

      std::vector<model::TrackEntry> entries{};
      for (auto& entry : animation.layers)
        if (auto track = std::get_if<model::Track>(&entry))
        {
          auto baked = track_bake(*track, nullptr);
          entries.emplace_back(std::move(*track));
          if (baked) entries.emplace_back(std::move(*baked));
        }
        else
        {
          auto group = std::get<model::TrackGroup>(entry);
          auto isGroupFolded = is_skewed(group.root);
          std::vector<model::Track> tracks{};
          for (auto& groupTrack : group.tracks)
          {
            auto baked = track_bake(groupTrack, isGroupFolded ? &group.root : nullptr);
            tracks.push_back(std::move(groupTrack));
            if (baked) tracks.push_back(std::move(*baked));
          }
          group.tracks = std::move(tracks);
          if (isGroupFolded) group.root = model::TrackGroup{}.root;
          entries.emplace_back(std::move(group));
        }
      animation.layers = std::move(entries);

      // A folded root's transform goes into the nulls too (their group roots first), and the root becomes plain.
      if (!isRootFolded) continue;
      for (auto& entry : animation.nulls)
        if (auto track = std::get_if<model::Track>(&entry))
          model::track_group_root_bake(*track, animation.root, animation.frameNum);
        else
        {
          auto& group = std::get<model::TrackGroup>(entry);
          for (auto& groupTrack : group.tracks)
          {
            model::track_group_root_bake(groupTrack, group.root, animation.frameNum);
            model::track_group_root_bake(groupTrack, animation.root, animation.frameNum);
          }
          group.root = model::TrackGroup{}.root;
        }
      animation.root = model::Animation{}.root;
    }
    if (images.empty()) return;

    std::vector<glm::ivec2> sizes{};
    for (const auto& image : images)
      sizes.push_back(image.size);
    glm::ivec2 packedSize{};
    std::vector<glm::ivec2> positions{};
    pack::rects_pack(sizes, packedSize, positions);
    std::vector<ImagePart> parts{};
    for (int i = 0; i < (int)images.size(); ++i)
      parts.push_back({.image = &images[i], .size = images[i].size, .destination = positions[i]});
    if (!Image::compose(parts, packedSize, false).write_png(path))
      return toast_log(Level::ERROR, TOAST_BAKE_WRITE_FAILED, pathUtf8);
    if (packedSize.x > BAKED_SIZE_MAX || packedSize.y > BAKED_SIZE_MAX)
      toast_log(Level::WARNING, TOAST_BAKE_LARGE, pathUtf8, packedSize.x, packedSize.y, BAKED_SIZE_MAX);
    content.spritesheets.push_back({.id = spritesheetId, .path = path.filename()});

    std::set<int> bakedIds{};
    for (auto [layerId, bakedId] : bakedLayerIds)
      bakedIds.insert(bakedId);
    for (auto index : indices)
      model::tracks_each(game.animation_edit(index)->layers,
                         [&](model::Track& track, auto*)
                         {
                           if (!bakedIds.contains(track.id)) return;
                           for (auto& frame : track.frames)
                             if (frame.regionId != -1)
                             {
                               frame.crop = glm::vec2(positions[frame.regionId]);
                               frame.regionId = -1;
                             }
                         });
    result = std::move(game);
  }
}
