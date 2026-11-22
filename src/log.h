#pragma once

#include <cstdio>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>

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
    X(INFO, "[INFO]")                                                                                                  \
    X(WARNING, "[WARNING]")                                                                                            \
    X(ERROR, "[ERROR]")                                                                                                \
    X(FATAL, "[FATAL]")                                                                                                \
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

// Writes to stdout and, once a session is open, to its own file in the preferences' logs folder.
// The last SESSION_LIMIT sessions' files are kept.
class Logger
{
    std::filesystem::path logPath;
    std::FILE* file = nullptr;
    std::mutex mutex;

public:
    std::filesystem::path path();
    void session_open(const std::filesystem::path&);
    void write(Level, const std::string&);
    void info(const std::string&);
    void warning(const std::string&);
    void error(const std::string&);
    void fatal(const std::string&);
    void command(const std::string&);
    ~Logger();
};

int log_exceptions(const std::function<int()>&);

extern Logger logger;
