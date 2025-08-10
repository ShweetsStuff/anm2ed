#include "ffmpeg.h"

bool 
ffmpeg_render
(
    const std::string& ffmpegPath, 
    const std::string& outputPath, 
    const std::vector<Texture>& frames,
    ivec2 size, 
    s32 fps,
    enum RenderType type
)
{
    if (frames.empty() || size.x <= 0 || size.y <= 0 || fps <= 0 || ffmpegPath.empty() || outputPath.empty()) return false;
 
    std::string command{};

    switch (type)
    {
        case RENDER_GIF:
            command = std::format(FFMPEG_GIF_FORMAT, ffmpegPath, size.x, size.y, fps, outputPath);
            break;
        case RENDER_WEBM:
            command = std::format(FFMPEG_WEBM_FORMAT, ffmpegPath, size.x, size.y, fps, outputPath);
            break;
        default:
            return false;
    }

    FILE* fp = POPEN(command.c_str(), PWRITE_MODE);

    if (!fp) 
    {
        log_info(std::format(FFMPEG_POPEN_ERROR, strerror(errno)));
        return false;
    }

    size_t frameBytes = size.x * size.y * TEXTURE_CHANNELS;

    for (const auto& frame : frames) 
    {
        std::vector<u8> rgba = texture_download(&frame);

        if (rgba.size() != frameBytes) 
        { 
            PCLOSE(fp);
            return false; 
        }

        if (fwrite(rgba.data(), 1, frameBytes, fp) != frameBytes) 
        {
            PCLOSE(fp);
            return false;
        }
    }

    const int code = PCLOSE(fp);

    return (code == 0);
}