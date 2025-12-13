#include "log.h"

#include <print>

#include "filesystem_.h"
#include "time_.h"

using namespace anm2ed::util;

namespace anm2ed
{
  void Logger::write(const Level level, const std::string& message)
  {
    std::string formatted = std::format("{} {} {}", time::get("(%d-%B-%Y %I:%M:%S)"), LEVEL_STRINGS[level], message);
    std::println("{}", formatted);
    if (file.is_open()) file << formatted << '\n' << std::flush;
  }

  void Logger::info(const std::string& message)
  {
    write(INFO, message);
  }

  void Logger::warning(const std::string& message)
  {
    write(WARNING, message);
  }

  void Logger::error(const std::string& message)
  {
    write(ERROR, message);
  }

  void Logger::fatal(const std::string& message)
  {
    write(FATAL, message);
  }

  void Logger::command(const std::string& message)
  {
    write(COMMAND, message);
  }

  void Logger::open(const std::filesystem::path& path)
  {
    file.open(path, std::ios::out | std::ios::app);
  }

  Logger::Logger()
  {
    open(filesystem::path_preferences_get() / "log.txt");
    info("Initializing Anm2Ed");
  }

  Logger::~Logger()
  {
    info("Exiting Anm2Ed");
    if (file.is_open()) file.close();
  }

}

anm2ed::Logger logger;
