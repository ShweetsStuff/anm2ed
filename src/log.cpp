#include "log.h"

#include <algorithm>
#include <format>
#include <print>
#include <vector>

#include "localization.h"
#include "path.h"
#include "timestamp.h"

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

constexpr std::size_t SESSION_LIMIT = 10;

// Removes the oldest session logs, so that with the new one there are SESSION_LIMIT. Their names sort by time.
static void sessions_prune(const std::filesystem::path& directory)
{
    std::vector<std::filesystem::path> paths;
    std::error_code ec;
    for (auto& entry : std::filesystem::directory_iterator(directory, ec))
        if (entry.path().extension() == ".log")
            paths.push_back(entry.path());
    std::ranges::sort(paths);
    for (std::size_t i = 0; i + SESSION_LIMIT <= paths.size(); ++i)
        std::filesystem::remove(paths[i], ec);
}

std::filesystem::path Logger::path()
{
    return logPath;
}

void Logger::session_open(const std::filesystem::path& preferences)
{
    auto directory = preferences / "logs";
    std::error_code ec;
    std::filesystem::create_directories(directory, ec);
    sessions_prune(directory);
    auto name = std::format("anm2ed_{}_{}.log", timestamp_get("%Y-%m-%d_%H-%M-%S"), (int)getpid());
    logPath = directory / path_from_utf8(name);
#ifdef _WIN32
    file = _wfopen(logPath.c_str(), L"ab");
#else
    file = std::fopen(logPath.c_str(), "ab");
#endif
    info(std::format("Initializing {}", localize.get(LABEL_APPLICATION_NAME, ENGLISH)));
}

void Logger::write(Level level, const std::string& message)
{
    auto line = std::format("{} {} {}", LEVEL_STRINGS[level], timestamp_get("(%d-%B-%Y %I:%M:%S)"), message);
    std::lock_guard lock(mutex);
    std::println("{}", line);
    if (!file)
        return;
    std::println(file, "{}", line);
    std::fflush(file);
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

Logger::~Logger()
{
    if (!file)
        return;
    info("Exiting Anm2Ed");
    std::fclose(file);
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

Logger logger;
