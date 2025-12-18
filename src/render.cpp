#include "render.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>

#if _WIN32
  #include <windows.h>
#endif

#include "log.h"
#include "process_.h"
#include "sdl.h"
#include "string_.h"
#include "path_.h"

using namespace anm2ed::resource;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed
{
  bool animation_render(const std::filesystem::path& ffmpegPath, const std::filesystem::path& path,
                        std::vector<Texture>& frames, AudioStream& audioStream, render::Type type, ivec2 size, int fps)
  {
    if (frames.empty() || size.x <= 0 || size.y <= 0 || fps <= 0 || ffmpegPath.empty() || path.empty()) return false;

    auto pathString = path::to_utf8(path);
    auto ffmpegPathString = path::to_utf8(ffmpegPath);
    auto loggerPath = Logger::path();
    auto loggerPathString = path::to_utf8(loggerPath);
#if _WIN32
    auto ffmpegOutputPath = loggerPath.parent_path() / "ffmpeg-output.log";
    auto ffmpegOutputPathString = path::to_utf8(ffmpegOutputPath);
    std::error_code ffmpegOutputRemoveError;
    std::filesystem::remove(ffmpegOutputPath, ffmpegOutputRemoveError);
    auto flush_ffmpeg_output = [&]()
    {
      std::ifstream teeFile(ffmpegOutputPath, std::ios::binary);
      if (!teeFile) return;

      std::string line;
      bool isFirstLine = true;
      while (std::getline(teeFile, line))
      {
        if (isFirstLine && line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF &&
            static_cast<unsigned char>(line[1]) == 0xBB && static_cast<unsigned char>(line[2]) == 0xBF)
          line.erase(0, 3);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        logger.write_raw(line);
        isFirstLine = false;
      }

      teeFile.close();
      std::error_code removeEc;
      std::filesystem::remove(ffmpegOutputPath, removeEc);
    };
#endif

    std::filesystem::path audioPath{};
    std::string audioInputArguments{};
    std::string audioOutputArguments{"-an"};
    std::string command{};

    auto audio_remove = [&]()
    {
      if (!audioPath.empty())
      {
        std::error_code ec;
        std::filesystem::remove(audioPath, ec);
      }
    };

    if (type != render::GIF && !audioStream.stream.empty() && audioStream.spec.freq > 0 &&
        audioStream.spec.channels > 0)
    {
      auto tempFilenameUtf8 = std::format("{}.f32", pathString);
      audioPath = std::filesystem::temp_directory_path() / path::from_utf8(tempFilenameUtf8);

      std::ofstream audioFile(audioPath, std::ios::binary);

      if (audioFile)
      {
        auto data = (const char*)audioStream.stream.data();
        auto byteCount = audioStream.stream.size() * sizeof(float);
        audioFile.write(data, byteCount);
        audioFile.close();

        audioInputArguments = std::format("-f f32le -ar {0} -ac {1} -i \"{2}\"", audioStream.spec.freq,
                                          audioStream.spec.channels, path::to_utf8(audioPath));

        switch (type)
        {
          case render::WEBM:
            audioOutputArguments = "-c:a libopus -b:a 160k -shortest";
            break;
          case render::MP4:
            audioOutputArguments = "-c:a aac -b:a 192k -shortest";
            break;
          default:
            break;
        }
      }
      else
      {
        logger.warning("Failed to open temporary audio file; exporting video without audio.");
        audio_remove();
      }
    }

    command = std::format("\"{0}\" -y -f rawvideo -pix_fmt rgba -s {1}x{2} -r {3} -i pipe:0", ffmpegPathString,
                          size.x, size.y, fps);

    if (!audioInputArguments.empty()) command += " " + audioInputArguments;

    switch (type)
    {
      case render::GIF:
        command +=
            " -lavfi \"split[s0][s1];[s0]palettegen=stats_mode=full[p];[s1][p]paletteuse=dither=floyd_steinberg\""
            " -loop 0";
        command += std::format(" \"{}\"", pathString);
        break;
      case render::WEBM:
        command += " -c:v libvpx-vp9 -crf 30 -b:v 0 -pix_fmt yuva420p -row-mt 1 -threads 0 -speed 2 -auto-alt-ref 0";
        if (!audioOutputArguments.empty()) command += " " + audioOutputArguments;
        command += std::format(" \"{}\"", pathString);
        break;
      case render::MP4:
        command += " -vf \"format=yuv420p,scale=trunc(iw/2)*2:trunc(ih/2)*2\" -c:v libx265 -crf 20 -preset slow"
                   " -tag:v hvc1 -movflags +faststart";
        if (!audioOutputArguments.empty()) command += " " + audioOutputArguments;
        command += std::format(" \"{}\"", pathString);
        break;
      default:
        return false;
    }

    command += " 2>&1";

#if _WIN32
    auto logCommand = std::string("& ") + command + " | Tee-Object -FilePath " + string::quote(ffmpegOutputPathString) +
                      " -Encoding UTF8 -Append";
    logger.command(logCommand);

    auto utf8_to_wstring = [](const std::string& value)
    {
      if (value.empty()) return std::wstring{};
      auto length = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
      if (length <= 0) return std::wstring{};
      std::wstring wide(static_cast<std::size_t>(length - 1), L'\0');
      MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, wide.data(), length);
      return wide;
    };

    auto base64_encode_utf16 = [](const std::wstring& input)
    {
      const auto* bytes = reinterpret_cast<const unsigned char*>(input.c_str());
      auto length = input.size() * sizeof(wchar_t);
      static constexpr char alphabet[] =
          "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
      std::string encoded{};
      encoded.reserve(((length + 2) / 3) * 4);

      for (std::size_t i = 0; i < length;)
      {
        auto remaining = length - i;
        auto octetA = bytes[i++];
        auto octetB = remaining > 1 ? bytes[i++] : 0;
        auto octetC = remaining > 2 ? bytes[i++] : 0;

        auto triple = (static_cast<uint32_t>(octetA) << 16) | (static_cast<uint32_t>(octetB) << 8) |
                      static_cast<uint32_t>(octetC);

        encoded.push_back(alphabet[(triple >> 18) & 0x3F]);
        encoded.push_back(alphabet[(triple >> 12) & 0x3F]);
        encoded.push_back(remaining > 1 ? alphabet[(triple >> 6) & 0x3F] : '=');
        encoded.push_back(remaining > 2 ? alphabet[triple & 0x3F] : '=');
      }

      return encoded;
    };

    auto script = std::wstring(L"& { ") + utf8_to_wstring(logCommand) + L" }";
    auto encodedCommand = base64_encode_utf16(script);
    command = "powershell -NoProfile -EncodedCommand " + string::quote(encodedCommand);
#else
    command += " | tee -a " + string::quote(loggerPathString);
    logger.command(command);
#endif

    Process process(command.c_str(), "w");

    if (!process.get())
    {
      audio_remove();
      return false;
    }

    for (auto& frame : frames)
    {
      auto frameSize = frame.pixel_size_get();

      if (fwrite(frame.pixels.data(), 1, frameSize, process.get()) != frameSize)
      {
        audio_remove();
        return false;
      }
    }

    process.close();
#if _WIN32
    flush_ffmpeg_output();
#endif

    audio_remove();
    return true;
  }
}
