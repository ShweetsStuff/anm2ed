#pragma once

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <vector>

#include "audio_stream.hpp"
#include <glm/glm.hpp>

namespace anm2ed
{
  class Document;
}

namespace anm2ed::render
{
#define RENDER_LIST                                                                                                    \
  X(PNGS, "PNGs", "")                                                                                                  \
  X(SPRITESHEET, "Spritesheet (PNG)", ".png")                                                                          \
  X(GIF, "GIF", ".gif")                                                                                                \
  X(WEBM, "WebM", ".webm")                                                                                             \
  X(OGV, "OGV", ".ogv")                                                                                                \
  X(MP4, "MP4", ".mp4")

  enum Type
  {
#define X(symbol, string, extension) symbol,
    RENDER_LIST
#undef X
        COUNT
  };

  enum FpsMode
  {
    FPS_ANIMATION,
    FPS_PLAYBACK_RATE,
    FPS_COUNT
  };

  constexpr const char* STRINGS[] = {
#define X(symbol, string, extension) string,
      RENDER_LIST
#undef X
  };

  constexpr const char* EXTENSIONS[] = {
#define X(symbol, string, extension) extension,
      RENDER_LIST
#undef X
  };

  struct SpritesheetLayout
  {
    int rows{};
    int columns{};
  };

  inline int frame_count_get(int start, int end) { return std::max(end - start + 1, 1); }

  inline int frame_count_get(int start, int end, int animationFps, int renderFps)
  {
    animationFps = std::max(animationFps, 1);
    renderFps = std::max(renderFps, 1);
    auto duration = (double)frame_count_get(start, end) / (double)animationFps;
    return std::max((int)std::ceil(duration * (double)renderFps), 1);
  }

  inline float frame_time_get(int start, int frameIndex, int animationFps, int renderFps)
  {
    animationFps = std::max(animationFps, 1);
    renderFps = std::max(renderFps, 1);
    return (float)start + ((float)frameIndex * ((float)animationFps / (float)renderFps));
  }

  inline int fps_get(int mode, int animationFps, int playbackRate)
  {
    switch (mode)
    {
      case FPS_PLAYBACK_RATE:
        return std::max(playbackRate, 1);
      case FPS_ANIMATION:
      default:
        return std::max(animationFps, 1);
    }
  }

  inline SpritesheetLayout spritesheet_layout_get(int rows, int columns, int frameCount)
  {
    frameCount = std::max(frameCount, 1);
    rows = std::max(rows, 0);
    columns = std::max(columns, 0);

    if (rows == 0 && columns == 0)
    {
      columns = std::max((int)std::ceil(std::sqrt((float)frameCount)), 1);
      rows = (frameCount + columns - 1) / columns;
    }
    else if (rows == 0)
      rows = (frameCount + columns - 1) / columns;
    else if (columns == 0)
      columns = (frameCount + rows - 1) / rows;
    else if (rows * columns < frameCount)
    {
      if (rows <= columns)
        columns = (frameCount + rows - 1) / rows;
      else
        rows = (frameCount + columns - 1) / columns;
    }

    return {rows, columns};
  }

  struct RecorderOptions
  {
    Type type{};
    std::filesystem::path path{};
    std::filesystem::path format{};
    std::filesystem::path ffmpegPath{};
    int start{};
    int end{};
    int animationFps{1};
    int fps{1};
    int rows{};
    int columns{};
    bool isSound{};
    bool isIsolated{};
    bool isBounded{};
  };

  // A render in progress: which animation times to capture, the frames captured so far (written to disk) and the
  // trigger sound of each, and assembling the output when done.
  class Recorder
  {
  public:
    RecorderOptions options{};
    int index{};
    int count{};

    bool start(const RecorderOptions&);
    bool is_done() const;
    float time_get() const;
    bool is_sound_frame() const;
    bool frame_capture(std::vector<uint8_t>, glm::ivec2, int);
    void finish(const Document&, AudioStream&);
    void cancel();

  private:
    std::filesystem::path directory{};
    std::vector<std::filesystem::path> frames{};
    std::vector<int> soundIds{};
    int soundTime{-1};
  };
}

namespace anm2ed
{
  std::filesystem::path ffmpeg_log_path();
  bool animation_render(const std::filesystem::path&, const std::filesystem::path&,
                        const std::vector<std::filesystem::path>&, AudioStream&, render::Type, int);
}
