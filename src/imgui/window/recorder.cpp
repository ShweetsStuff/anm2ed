#include "recorder.hpp"

#include <chrono>
#include <cmath>
#include <format>

#include "audio.hpp"
#include "log.hpp"
#include "path.hpp"
#include "toast.hpp"

using namespace anm2ed::resource;
using namespace anm2ed::util;

namespace anm2ed::imgui
{
  constexpr int TEMP_DIRECTORY_ATTEMPTS = 1000;
  constexpr int ALPHA_OPAQUE = 255;
  constexpr float CHANNEL_MAX = 255.0f;

  std::filesystem::path frame_filename_get(const std::filesystem::path& format, int index, render::Type type)
  {
    auto fallback = path::from_utf8(std::format("frame_{:06}.png", index));
    if (type != render::PNGS) return fallback;
    try
    {
      auto filename = path::from_utf8(std::vformat(path::to_utf8(format), std::make_format_args(index))).filename();
      if (filename.empty()) return fallback;
      if (filename.extension().empty()) filename.replace_extension(render::EXTENSIONS[render::SPRITESHEET]);
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
    if (options.type == render::PNGS)
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
    count = render::frame_count_get(options.start, options.end, options.animationFps, options.fps);
    frames.clear();
    soundIds.clear();
    soundTime = -1;
    directory = frames_directory_create(options);
    return !directory.empty();
  }

  bool Recorder::is_done() const { return index >= count; }

  float Recorder::time_get() const
  {
    return render::frame_time_get(options.start, index, options.animationFps, options.fps);
  }

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
    if (options.type == render::PNGS)
      toast_log(frames.empty() ? Level::ERROR : Level::INFO,
                frames.empty() ? TOAST_EXPORT_RENDERED_FRAMES_FAILED : TOAST_EXPORT_RENDERED_FRAMES, pathString);
    else if (options.type == render::SPRITESHEET)
    {
      auto layout = render::spritesheet_layout_get(options.rows, options.columns, (int)frames.size());
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
      if (options.isSound && options.type != render::GIF &&
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
    if (options.type != render::PNGS && !directory.empty()) std::filesystem::remove_all(directory, ec);
    directory.clear();
    frames.clear();
    soundIds.clear();
    index = count = 0;
  }
}
