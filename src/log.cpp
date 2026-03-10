#include "log.hpp"

#include <array>
#include <print>

#include "path_.hpp"
#include "sdl.hpp"
#include "time_.hpp"

#if _WIN32
  #include <fcntl.h>
  #include <io.h>
#else
  #include <unistd.h>
#endif

using namespace anm2ed::util;

namespace anm2ed
{
  void Logger::write_raw(const std::string& message)
  {
    std::lock_guard lock(mutex);
    std::println("{}", message);
    if (file.is_open()) file << message << '\n' << std::flush;
  }

  void Logger::write(const Level level, const std::string& message)
  {
    std::string formatted = std::format("{} {} {}", LEVEL_STRINGS[level], time::get("(%d-%B-%Y %I:%M:%S)"), message);
    write_raw(formatted);
  }

  void Logger::info(const std::string& message) { write(INFO, message); }
  void Logger::warning(const std::string& message) { write(WARNING, message); }
  void Logger::error(const std::string& message) { write(ERROR, message); }
  void Logger::fatal(const std::string& message) { write(FATAL, message); }
  void Logger::command(const std::string& message) { write(COMMAND, message); }

  void Logger::stderr_pump()
  {
    std::string pending{};
    std::array<char, 512> buffer{};
    while (isStderrRedirecting)
    {
      int readBytes{};
#if _WIN32
      readBytes = _read(stderrPipeReadFd, buffer.data(), (unsigned int)buffer.size());
#else
      readBytes = (int)read(stderrPipeReadFd, buffer.data(), buffer.size());
#endif
      if (readBytes <= 0) break;

      pending.append(buffer.data(), (std::size_t)readBytes);

      std::size_t lineEnd{};
      while ((lineEnd = pending.find('\n')) != std::string::npos)
      {
        auto line = pending.substr(0, lineEnd);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) write_raw(line);
        pending.erase(0, lineEnd + 1);
      }
    }

    if (!pending.empty()) write_raw(pending);
  }

  void Logger::stderr_redirect_start()
  {
    if (isStderrRedirecting) return;

    int pipeFds[2]{-1, -1};
#if _WIN32
    if (_pipe(pipeFds, 4096, _O_BINARY) != 0) return;
    stderrOriginalFd = _dup(_fileno(stderr));
    if (stderrOriginalFd < 0 || _dup2(pipeFds[1], _fileno(stderr)) != 0)
    {
      _close(pipeFds[0]);
      _close(pipeFds[1]);
      return;
    }
    _close(pipeFds[1]);
#else
    if (pipe(pipeFds) != 0) return;
    stderrOriginalFd = dup(fileno(stderr));
    if (stderrOriginalFd < 0 || dup2(pipeFds[1], fileno(stderr)) < 0)
    {
      close(pipeFds[0]);
      close(pipeFds[1]);
      return;
    }
    close(pipeFds[1]);
#endif

    std::setvbuf(stderr, nullptr, _IONBF, 0);

    stderrPipeReadFd = pipeFds[0];
    isStderrRedirecting = true;
    stderrThread = std::thread([this]() { stderr_pump(); });
  }

  void Logger::stderr_redirect_stop()
  {
    if (!isStderrRedirecting) return;
    isStderrRedirecting = false;

    if (stderrOriginalFd >= 0)
    {
#if _WIN32
      _dup2(stderrOriginalFd, _fileno(stderr));
      _close(stderrOriginalFd);
#else
      dup2(stderrOriginalFd, fileno(stderr));
      close(stderrOriginalFd);
#endif
      stderrOriginalFd = -1;
    }

    if (stderrPipeReadFd >= 0)
    {
#if _WIN32
      _close(stderrPipeReadFd);
#else
      close(stderrPipeReadFd);
#endif
      stderrPipeReadFd = -1;
    }

    if (stderrThread.joinable()) stderrThread.join();
  }

  void Logger::open(const std::filesystem::path& path)
  {
    file.open(path, std::ios::out | std::ios::app);
    stderr_redirect_start();
  }

  std::filesystem::path Logger::path() { return sdl::preferences_directory_get() / "log.txt"; }

  Logger::Logger()
  {
    open(path());
    info("Initializing Anm2Ed");
  }

  Logger::~Logger()
  {
    info("Exiting Anm2Ed");
    stderr_redirect_stop();
    if (file.is_open()) file.close();
  }
}

anm2ed::Logger logger;
