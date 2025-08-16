#include "log.h"

inline std::ofstream logFile;

std::string log_path_get(void)
{
    return preferences_path_get() + LOG_PATH;
}

void log_write(const std::string& string)
{
    std::println("{}", string);
    
    if (logFile.is_open())
    {
        logFile << string << '\n';
        logFile.flush();
    }
}

void log_init(void)
{
    std::string logFilepath = log_path_get();
    logFile.open(logFilepath, std::ios::out | std::ios::trunc);
    if (!logFile) std::println("{}", std::format(LOG_INIT_ERROR, logFilepath));
}

void log_error(const std::string& error)
{
    log_write(LOG_ERROR_FORMAT + error);
}

void log_info(const std::string& info)
{
    log_write(LOG_INFO_FORMAT + info);
}

void log_warning(const std::string& warning)
{
    log_write(LOG_WARNING_FORMAT + warning);
}

void log_imgui(const std::string& imgui)
{
    log_write(LOG_IMGUI_FORMAT + imgui);
}

void log_command(const std::string& command)
{
    log_write(LOG_COMMAND_FORMAT + command);
}

void log_free(void)
{
    logFile.close();
}