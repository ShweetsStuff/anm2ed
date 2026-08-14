#include "log.hpp"

#include <algorithm>
#include <array>
#include <exception>
#include <filesystem>
#include <format>
#include <print>
#include <vector>

#include "file.hpp"
#include "path.hpp"
#include "sdl.hpp"
#include "strings.hpp"
#include "time.hpp"

#if _WIN32
  #include <fcntl.h>
  #include <io.h>
  #include <process.h>
#else
  #include <unistd.h>
#endif

using namespace anm2ed::util;

namespace anm2ed
{
  namespace log
  {
    constexpr auto SESSION_LIMIT = 10;
    constexpr std::uintmax_t DIRECTORY_SIZE_LIMIT = 100ull * 1024ull * 1024ull;
    constexpr std::uintmax_t SESSION_SIZE_LIMIT = 25ull * 1024ull * 1024ull;

    struct SessionFile
    {
      std::filesystem::path path{};
      std::filesystem::file_time_type time{};
      std::uintmax_t size{};
    };

    std::uint32_t process_id_get()
    {
#if _WIN32
      return (std::uint32_t)_getpid();
#else
      return (std::uint32_t)getpid();
#endif
    }

    std::filesystem::path directory_path_get() { return sdl::preferences_directory_get() / "logs"; }

    std::filesystem::path session_path_get()
    {
      auto name = std::format("anm2ed_{}_{}.log", time::get("%Y-%m-%d_%H-%M-%S"), process_id_get());
      return directory_path_get() / path::from_utf8(name);
    }

    std::vector<SessionFile> session_files_get(const std::filesystem::path& directory,
                                               const std::filesystem::path& currentPath)
    {
      std::vector<SessionFile> files{};
      std::error_code ec{};
      if (!std::filesystem::is_directory(directory, ec)) return files;

      for (std::filesystem::directory_iterator it(directory, ec), end{}; !ec && it != end; it.increment(ec))
      {
        auto path = it->path();
        if (path == currentPath || path.extension() != ".log") continue;
        std::error_code fileEc{};
        if (!std::filesystem::is_regular_file(path, fileEc)) continue;

        auto size = std::filesystem::file_size(path, fileEc);
        if (fileEc) continue;

        auto time = std::filesystem::last_write_time(path, fileEc);
        if (fileEc)
        {
          time = std::filesystem::file_time_type::min();
        }

        files.push_back({path, time, size});
      }

      return files;
    }

    void session_file_remove(const SessionFile& file, std::uintmax_t& totalSize)
    {
      std::error_code ec{};
      std::filesystem::remove(file.path, ec);
      if (!ec && totalSize >= file.size) totalSize -= file.size;
    }

    void sessions_prune(const std::filesystem::path& directory, const std::filesystem::path& currentPath)
    {
      auto files = session_files_get(directory, currentPath);
      std::sort(files.begin(), files.end(), [](const SessionFile& left, const SessionFile& right)
                { return left.time > right.time; });

      std::uintmax_t totalSize{};
      for (const auto& file : files)
        totalSize += file.size;

      while (files.size() >= SESSION_LIMIT)
      {
        session_file_remove(files.back(), totalSize);
        files.pop_back();
      }

      while (totalSize > DIRECTORY_SIZE_LIMIT && !files.empty())
      {
        session_file_remove(files.back(), totalSize);
        files.pop_back();
      }
    }

    void legacy_log_remove(const std::filesystem::path& currentPath)
    {
      auto directory = sdl::preferences_directory_get();
      if (directory.empty()) return;

      auto legacyPath = directory / "log.txt";
      if (legacyPath == currentPath) return;

      std::error_code ec{};
      std::filesystem::remove(legacyPath, ec);
    }
  }

  void Logger::write_raw(const std::string& message)
  {
    std::lock_guard lock(mutex);
    try
    {
      std::println("{}", message);
    }
    catch (...)
    {
    }
    if (!logPath.empty())
    {
      try
      {
        auto lineSize = (std::uintmax_t)message.size() + 1;
        auto isLineWithinLimit = logSize <= log::SESSION_SIZE_LIMIT && lineSize <= log::SESSION_SIZE_LIMIT - logSize;
        if (isLineWithinLimit)
        {
          auto line = message + '\n';
          if (file::write_string(logPath, line, "ab")) logSize += line.size();
        }
        else if (!isLogLimitReached)
        {
          std::string line =
              "[WARNING] Session log size limit reached; suppressing further file logs for this session.\n";
          if (logSize + line.size() <= log::SESSION_SIZE_LIMIT)
            if (file::write_string(logPath, line, "ab")) logSize += line.size();
          isLogLimitReached = true;
        }
      }
      catch (...)
      {
      }
    }
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
    logPath = path;
    logSize = 0;
    isLogLimitReached = false;
    if (!logPath.empty())
    {
      std::error_code ec{};
      auto directory = logPath.parent_path();
      if (!directory.empty()) std::filesystem::create_directories(directory, ec);
      log::legacy_log_remove(logPath);
      log::sessions_prune(directory, logPath);
      logSize = std::filesystem::file_size(logPath, ec);
      if (ec) logSize = 0;
      isLogLimitReached = logSize >= log::SESSION_SIZE_LIMIT;
    }
    stderr_redirect_start();
  }

  std::filesystem::path Logger::path()
  {
    static auto path = log::session_path_get();
    return path;
  }

  Logger::Logger()
  {
    open(path());
    info(std::format("Initializing {}", localize.get(LABEL_APPLICATION_NAME, ENGLISH)));
  }

  Logger::~Logger()
  {
    info("Exiting Anm2Ed");
    stderr_redirect_stop();
  }

  int log_exceptions(const std::function<int()>& callback)
  {
    try
    {
      return callback();
    }
    catch (const std::exception& e)
    {
      logger.fatal(std::format("Unhandled exception: {}", e.what()));
    }
    catch (...)
    {
      logger.fatal("Unhandled non-standard exception");
    }

    return EXIT_FAILURE;
  }
}

anm2ed::Logger logger;
