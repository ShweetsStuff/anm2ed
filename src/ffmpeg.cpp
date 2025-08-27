#include "ffmpeg.h"

static std::string ffmpeg_log_path_get(void)
{
    return preferences_path_get() + FFMPEG_LOG_PATH;
}

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
        case RENDER_MP4:
            command = std::format(FFMPEG_MP4_FORMAT, ffmpegPath, size.x, size.y, fps, outputPath);
            break;
        default:
            break;
    }

    // ffmpeg output will be piped into the log
    std::string logOutput = " 2>> \"" + ffmpeg_log_path_get() + "\"";

#if _WIN32
    command = string_quote(command) + logOutput;
#else
    command += logOutput;
#endif

    log_command(command);

    FILE* fp = POPEN(command.c_str(), PWRITE_MODE);

    if (!fp) 
    {
        log_error(std::format(FFMPEG_POPEN_ERROR, strerror(errno)));
        return false;
    }

    
    size_t frameBytes = size.x * size.y * TEXTURE_CHANNELS;

// supposedly, might help with video corruption issues on windows?
#if _WIN32
    _setmode(_fileno(stdout), _O_BINARY);
#endif

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