#pragma once

#include "localization.h"

#define THEMES                                                                                                         \
  X(THEME_LIGHT, LABEL_THEME_LIGHT)                                                                                          \
  X(THEME_DARK, LABEL_THEME_DARK)                                                                                            \
  X(THEME_CLASSIC, LABEL_THEME_CLASSIC)

  enum Theme
  {
#define X(symbol, string) symbol,
    THEMES
#undef X
        THEME_COUNT
  };

  constexpr StringType THEME_STRINGS[] = {
#define X(symbol, string) string,
      THEMES
#undef X
  };

#undef THEMES

  void theme_set(Theme);
