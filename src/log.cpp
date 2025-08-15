#include "log.h"

inline std::ofstream logFile;

static void _log_write(const std::string& string)
{
    std::println("{}", string);
    
    if (logFile.is_open())
    {
        logFile << string << '\n';
        logFile.flush();
    }
}

void log_init(const std::string& file)
{
    logFile.open(file, std::ios::out | std::ios::trunc);
    if (!logFile) std::println("{}", std::format(LOG_INIT_ERROR, file));
}

void log_error(const std::string& error)
{
    _log_write(LOG_ERROR_FORMAT + error);
}

void log_info(const std::string& info)
{
    _log_write(LOG_INFO_FORMAT + info);
}

void log_warning(const std::string& warning)
{
    _log_write(LOG_WARNING_FORMAT + warning);
}

void log_imgui(const std::string& imgui)
{
    _log_write(LOG_IMGUI_FORMAT + imgui);
}

void log_free(void)
{
    logFile.close();
}