#pragma once

#include "COMMON.h"

#define LOG_WARNING_FORMAT "[WARNING] "
#define LOG_ERROR_FORMAT "[ERROR] "
#define LOG_INFO_FORMAT "[INFO] "
#define LOG_IMGUI_FORMAT "[IMGUI] "
#define LOG_INIT_ERROR "[ERROR] Failed to open log file: {}"
#define LOG_PATH "log.txt"

void log_init(const std::string& file);
void log_error(const std::string& error);
void log_info(const std::string& info);
void log_warning(const std::string& warning);
void log_imgui(const std::string& imgui);
void log_free(void);