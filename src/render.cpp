#include "render.h"

#include <cstring>
#include <format>

#ifdef _WIN32
  #include "util.h"
  #define POPEN _popen
  #define PCLOSE _pclose
  #define PWRITE_MODE "wb"
  #define PREAD_MODE "r"
#else
  #define POPEN popen
  #define PCLOSE pclose
  #define PWRITE_MODE "w"
  #define PREAD_MODE "r"
#endif

#include "log.h"

using namespace anm2ed::resource;
using namespace glm;

namespace anm2ed
{
  constexpr auto FFMPEG_POPEN_ERROR = "popen() (for FFmpeg) failed!\n{}";

  constexpr auto GIF_FORMAT = "\"{0}\" -y "
                              "-f rawvideo -pix_fmt rgba -s {1}x{2} -r {3} -i pipe:0 "
                              "-lavfi \"split[s0][s1];"
                              "[s0]palettegen=stats_mode=full[p];"
                              "[s1][p]paletteuse=dither=floyd_steinberg\" "
                              "-loop 0 \"{4}\"";

  constexpr auto WEBM_FORMAT = "\"{0}\" -y "
                               "-f rawvideo -pix_fmt rgba -s {1}x{2} -r {3} -i pipe:0 "
                               "-c:v libvpx-vp9 -crf 30 -b:v 0 -pix_fmt yuva420p -row-mt 1 -threads 0 -speed 2 "
                               "-auto-alt-ref 0 -an \"{4}\"";

  constexpr auto* MP4_FORMAT = "\"{0}\" -y "
                               "-f rawvideo -pix_fmt rgba -s {1}x{2} -r {3} -i pipe:0 "
                               "-vf \"format=yuv420p,scale=trunc(iw/2)*2:trunc(ih/2)*2\" "
                               "-c:v libx265 -crf 20 -preset slow "
                               "-tag:v hvc1 -movflags +faststart -an \"{4}\"";

  bool animation_render(const std::string& ffmpegPath, const std::string& path, std::vector<Texture>& frames,
                        render::Type type, ivec2 size, int fps)
  {
    if (frames.empty() || size.x <= 0 || size.y <= 0 || fps <= 0 || ffmpegPath.empty() || path.empty()) return false;

    std::string command{};

    switch (type)
    {
      case render::GIF:
        command = std::format(GIF_FORMAT, ffmpegPath, size.x, size.y, fps, path);
        break;
      case render::WEBM:
        command = std::format(WEBM_FORMAT, ffmpegPath, size.x, size.y, fps, path);
        break;
      case render::MP4:
        command = std::format(MP4_FORMAT, ffmpegPath, size.x, size.y, fps, path);
        break;
      default:
        break;
    }

#if _WIN32
    command = string::quote(command);
#endif

    logger.command(command);

    FILE* fp = POPEN(command.c_str(), PWRITE_MODE);

    if (!fp)
    {
      logger.error(std::format(FFMPEG_POPEN_ERROR, strerror(errno)));
      return false;
    }

    for (auto& frame : frames)
    {
      auto frameSize = frame.pixel_size_get();

      if (fwrite(frame.pixels.data(), 1, frameSize, fp) != frameSize)
      {
        PCLOSE(fp);
        return false;
      }
    }

    auto code = PCLOSE(fp);
    return (code == 0);
  }
}