#pragma once

#include <algorithm>
#include <cmath>

#include "audio_stream.hpp"
#include "texture.hpp"

namespace anm2ed::render
{
#define RENDER_LIST                                                                                                    \
  X(PNGS, "PNGs", "")                                                                                                  \
  X(SPRITESHEET, "Spritesheet (PNG)", ".png")                                                                          \
  X(GIF, "GIF", ".gif")                                                                                                \
  X(WEBM, "WebM", ".webm")                                                                                             \
  X(MP4, "MP4", ".mp4")

  enum Type
  {
#define X(symbol, string, extension) symbol,
    RENDER_LIST
#undef X
        COUNT
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
}

namespace anm2ed
{
  std::filesystem::path ffmpeg_log_path();
  bool animation_render(const std::filesystem::path&, const std::filesystem::path&,
                        const std::vector<std::filesystem::path>&, const std::vector<double>&, AudioStream&,
                        render::Type, int);
}
