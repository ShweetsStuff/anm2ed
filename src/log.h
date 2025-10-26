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
    void write(const Level, const std::string&);
    void info(const std::string&);
    void warning(const std::string&);
    void error(const std::string&);
    void fatal(const std::string&);
    void open(const std::filesystem::path&);
    Logger();
    ~Logger();
  };

  extern Logger logger;
}
