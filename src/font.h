#pragma once

#include <imgui/imgui.h>
#include "embed.h"

namespace anm2ed::resource::font
{
  constexpr auto SIZE = 16;
  constexpr auto SIZE_LARGE = 32;

  enum Type
  {
    REGULAR,
    ITALICS,
    BOLD,
    BOLD_ITALICS,
    COUNT
  };

  constexpr const Embed* FONTS[COUNT] = {&FONT_REGULAR_TTF, &FONT_ITALICS_TTF, &FONT_BOLD_TTF, &FONT_BOLD_ITALICS_TTF};
}

namespace anm2ed::resource
{
  class Font
  {
    ImFont* pointer{};

  public:
    Font();
    Font(void*, size_t, int);
    Font(const char* path, int size, int number = 0);
    void append(void*, size_t, int);
    bool append(const char* path, int size, int number = 0);
    ~Font();
    ImFont* get();
    Font& operator=(Font&&) noexcept;
  };
}
