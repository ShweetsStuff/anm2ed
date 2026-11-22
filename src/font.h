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
