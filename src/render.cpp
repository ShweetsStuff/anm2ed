#include "render.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>

#include <SDL3/SDL_timer.h>

#include "audio.hpp"
#include "document.hpp"
#include "file.hpp"
#include "log.hpp"
#include "path.hpp"
#include "process.hpp"
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
    for (auto& [id, _] : document.sounds)
      if (auto sound = document.sound_get(id)) audio::track_detach(*sound, mixer);
  }

  // Mixes each frame's trigger sound into one stream at the render frame rate.
  bool audio_stream_generate(AudioStream& audioStream, const Document& document, const std::vector<int>& soundIds,
                             int fps)
  {
    audioStream.stream.clear();
    if (soundIds.empty() || fps <= 0) return true;

    SDL_AudioSpec mixSpec = audioStream.spec;
    mixSpec.format = SDL_AUDIO_F32;
    auto mixer = MIX_CreateMixer(&mixSpec);
    if (!mixer) return false;

    auto channels = std::max(mixSpec.channels, 1);
    auto framesPerStep = (double)std::max(mixSpec.freq, 1) / (double)fps;
    auto accumulator = 0.0;
    std::vector<float> buffer{};
    auto isGenerated = true;
    for (auto soundId : soundIds)
    {
      if (auto sound = document.sound_get(soundId)) audio::play(*sound, false, mixer);
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

  bool Recorder::start(const RecorderOptions& recorderOptions)
  {
    options = recorderOptions;
    index = 0;
    count = frame_count_get(options.start, options.end, options.animationFps, options.fps);
    frames.clear();
    soundIds.clear();
    soundTime = -1;
    directory = frames_directory_create(options);
    return !directory.empty();
  }

  bool Recorder::is_done() const { return index >= count; }

  float Recorder::time_get() const { return frame_time_get(options.start, index, options.animationFps, options.fps); }

  // A trigger sound is taken once per animation frame, however many render frames it spans.
  bool Recorder::is_sound_frame() const { return (int)std::floor(time_get()) != soundTime; }

  bool Recorder::frame_capture(std::vector<uint8_t> pixels, glm::ivec2 size, int soundId)
  {
    soundTime = (int)std::floor(time_get());
    soundIds.push_back(soundId);
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
      if (options.isSound && options.type != GIF &&
          !audio_stream_generate(audioStream, document, soundIds, options.fps))
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
    soundIds.clear();
    index = count = 0;
  }
}
