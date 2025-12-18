#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace anm2ed
{
#if defined(INFO)
  #undef INFO
#endif
#if defined(WARNING)
  #undef WARNING
#endif
#if defined(ERROR)
  #undef ERROR
#endif
#if defined(FATAL)
  #undef FATAL
#endif

#define LEVELS                                                                                                         \
  X(INFO, "[INFO]")                                                                                                    \
  X(WARNING, "[WARNING]")                                                                                              \
  X(ERROR, "[ERROR]")                                                                                                  \
  X(FATAL, "[FATAL]")                                                                                                  \
  X(COMMAND, "[COMMAND]")

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
    static std::filesystem::path path();
    void write_raw(const std::string&);
    void write(const Level, const std::string&);
    void info(const std::string&);
    void warning(const std::string&);
    void error(const std::string&);
    void fatal(const std::string&);
    void command(const std::string&);
    void open(const std::filesystem::path&);
    Logger();
    ~Logger();
  };

}

extern anm2ed::Logger logger;
