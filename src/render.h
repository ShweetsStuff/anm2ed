#pragma once

#include "texture.h"

namespace anm2ed::render
{
#define RENDER_LIST                                                                                                    \
  X(PNGS, "PNGs")                                                                                                      \
  X(GIF, "GIF")                                                                                                        \
  X(WEBM, "WebM")                                                                                                      \
  X(MP4, "MP4")

  enum Type
  {
#define X(symbol, string) symbol,
    RENDER_LIST
#undef X
        COUNT
  };

  constexpr const char* STRINGS[] = {
#define X(symbol, string) string,
      RENDER_LIST
#undef X
  };
}

namespace anm2ed
{
  bool animation_render(const std::string&, const std::string&, std::vector<resource::Texture>&, render::Type,
                        glm::ivec2, int);
}