#pragma once

#include <imgui/imgui.h>
#include "embed.h"

constexpr auto FONT_SIZE = 16;
constexpr auto FONT_SIZE_LARGE = 32;

enum FontStyle
{
    FONT_REGULAR,
    FONT_ITALICS,
    FONT_BOLD,
    FONT_BOLD_ITALICS,
    FONT_COUNT
};

constexpr const Embed* FONTS[FONT_COUNT] = {&FONT_REGULAR_TTF, &FONT_ITALICS_TTF, &FONT_BOLD_TTF,
                                            &FONT_BOLD_ITALICS_TTF};

class Font
{
    ImFont* pointer = nullptr;

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
