#pragma once

#include "COMMON.h"

enum RenderType
{
    RENDER_PNG,
    RENDER_GIF,
    RENDER_WEBM,
    RENDER_COUNT
};

const inline std::string RENDER_TYPE_STRINGS[] = 
{
    "PNG Images",
    "GIF image",
    "WebM video",
};

const inline std::string RENDER_EXTENSIONS[RENDER_COUNT] =
{
    ".png",
    ".gif",
    ".webm"
};