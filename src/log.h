#pragma once

#include <filesystem>
#include <fstream>

namespace anm2ed::log
{
#define LEVELS                                                                                                         \
  X(INFO, "[INFO]")                                                                                                    \
  X(WARNING, "[WARNING]")                                                                                              \
  X(ERROR, "[ERROR]")                                                                                                  \
  X(FATAL, "[FATAL]")

  enum Level
  {
#define X(symbol, string) symbol,
    LEVELS
#undef X
  };

  constexpr std::string_view LEVEL_STRINGS[] = {
#define X(symbol, string) string,
      LEVELS
#undef X
  };
#undef LEVELS

  class Logger
  {
    std::ofstream file{};

  public:
    void write(const Level level, const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void fatal(const std::string& message);
    void open(const std::filesystem::path& path);
    Logger();
    ~Logger();
  };

  extern Logger logger;
}