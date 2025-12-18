#pragma once

#include "audio_stream.h"
#include "texture.h"

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
}

namespace anm2ed
{
  std::filesystem::path ffmpeg_log_path();
  bool animation_render(const std::filesystem::path&, const std::filesystem::path&, std::vector<resource::Texture>&,
                        AudioStream&, render::Type, glm::ivec2, int);
}