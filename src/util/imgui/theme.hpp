#pragma once

#include "strings.hpp"

namespace anm2ed::types::theme
{
#define THEMES                                                                                                         \
  X(LIGHT, LABEL_THEME_LIGHT)                                                                                          \
  X(DARK, LABEL_THEME_DARK)                                                                                            \
  X(CLASSIC, LABEL_THEME_CLASSIC)

  enum Type
  {
#define X(symbol, string) symbol,
    THEMES
#undef X
        COUNT
  };

  constexpr StringType STRINGS[] = {
#define X(symbol, string) string,
      THEMES
#undef X
  };

#undef THEMES
}

namespace anm2ed::imgui
{
  void theme_set(types::theme::Type);
}
