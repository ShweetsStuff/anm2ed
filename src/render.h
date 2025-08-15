#pragma once

#include "COMMON.h"

enum RenderType
{
    RENDER_PNG,
    RENDER_GIF,
    RENDER_WEBM,
    RENDER_MP4
};

constexpr inline s32 RENDER_COUNT = RENDER_MP4 + 1;

const inline std::string RENDER_TYPE_STRINGS[] = 
{
    "PNG Images",
    "GIF image",
    "WebM video",
    "MP4 video"
};

const inline std::string RENDER_EXTENSIONS[RENDER_COUNT] =
{
    ".png",
    ".gif",
    ".webm",
    ".mp4"
};