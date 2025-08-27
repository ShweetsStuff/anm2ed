#pragma once

#include "render.h"
#include "texture.h"

#define FFMPEG_POPEN_ERROR "popen() (for FFmpeg) failed!\n{}"
#define FFMPEG_LOG_BUFFER_SIZE 256
#define FFMPEG_LOG_PATH "ffmpeg.txt"

static constexpr const char* FFMPEG_GIF_FORMAT =
"\"{0}\" -y "
"-f rawvideo -pix_fmt rgba -s {1}x{2} -r {3} -i pipe:0 "
"-lavfi \"split[s0][s1];"
"[s0]palettegen=stats_mode=full[p];"
"[s1][p]paletteuse=dither=floyd_steinberg\" "
"-loop 0 \"{4}\"";

static constexpr const char* FFMPEG_WEBM_FORMAT =
"\"{0}\" -y "
"-f rawvideo -pix_fmt rgba -s {1}x{2} -r {3} -i pipe:0 "
"-c:v libvpx-vp9 -crf 30 -b:v 0 -pix_fmt yuva420p -row-mt 1 -threads 0 -speed 2 "
"-auto-alt-ref 0 -an \"{4}\"";

static constexpr const char* FFMPEG_MP4_FORMAT =
"\"{0}\" -y "
"-f rawvideo -pix_fmt rgba -s {1}x{2} -r {3} -i pipe:0 "
"-vf \"format=yuv420p,scale=trunc(iw/2)*2:trunc(ih/2)*2\" "
"-c:v libx265 -crf 20 -preset slow "
"-tag:v hvc1 -movflags +faststart -an \"{4}\"";

bool 
ffmpeg_render
(
    const std::string& ffmpegPath, 
    const std::string& outputPath, 
    const std::vector<Texture>& frames,
    ivec2 size, 
    s32 fps,
    enum RenderType type
);