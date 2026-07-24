/**
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef LOGGER_CORE_H_
#define LOGGER_CORE_H_

#include <string>
#include <unordered_map>
#include <ctime>
#include <cstdio>
#include <cstdarg>
#include <cstring>

enum class LoggerLevel {
    LEVEL_DEBUG = 0,
    LEVEL_INFO  = 1,
    LEVEL_WARN  = 2,
    LEVEL_ERROR = 3
};

extern const char* LOG_LEVEL_NAMES[];
extern const char* LOG_LEVEL_PREFIX[];

/**
 * @brief Simple logging system for crash plugins
 *
 * Provides context-aware logging with configurable levels per plugin.
 * Automatically enables WARN/ERROR logs for unconfigured plugins.
 */
class SimpleLogger {
public:
    struct PluginState {
        bool enabled = false;
        LoggerLevel level = LoggerLevel::LEVEL_ERROR;
        time_t last_update = 0;
    };

private:
    static std::unordered_map<std::string, PluginState> plugin_states_;
    static std::string current_context_;

public:
    static void set_context(const std::string& name);
    static void clear_context();
    static std::string get_context();
    static void log(const std::string& plugin_name, LoggerLevel level, const char* format, ...);
    static void print(const char* fmt, ...);
    static void enablePlugin(const std::string& plugin_name);
    static void disablePlugin(const std::string& plugin_name);
    static bool isPluginEnabled(const std::string& plugin_name);
    static void setPluginLevel(const std::string& plugin_name, LoggerLevel level);
    static LoggerLevel getPluginLevel(const std::string& plugin_name);
    /**
     * @brief Determine if a log message should be output
     * @param plugin_name Plugin context
     * @param level Log level of the message
     * @return true if message should be logged
     *
     * Logic:
     * - Explicitly enabled plugins: use configured level
     * - Explicitly disabled plugins: no output
     * - Unconfigured plugins: auto-enable WARN/ERROR
     */
    static bool shouldLog(const std::string& plugin_name, LoggerLevel level);
    static void showStatus();
    static void forceCleanup();
};

inline std::string __logger_get_context() {
    return SimpleLogger::get_context();
}

inline std::string __logger_auto_newline(const char* fmt) {
    std::string result(fmt);
    if (!result.empty() && result.back() != '\n') {
        result += '\n';
    }
    return result;
}

#define PRINT(fmt, ...) SimpleLogger::print(fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...)  SimpleLogger::log(__logger_get_context(), LoggerLevel::LEVEL_DEBUG, __logger_auto_newline(fmt).c_str(), ##__VA_ARGS__)
#define LOGI(fmt, ...)  SimpleLogger::log(__logger_get_context(), LoggerLevel::LEVEL_INFO,  __logger_auto_newline(fmt).c_str(), ##__VA_ARGS__)
#define LOGW(fmt, ...)  SimpleLogger::log(__logger_get_context(), LoggerLevel::LEVEL_WARN,  __logger_auto_newline(fmt).c_str(), ##__VA_ARGS__)
#define LOGE(fmt, ...)  SimpleLogger::log(__logger_get_context(), LoggerLevel::LEVEL_ERROR, __logger_auto_newline(fmt).c_str(), ##__VA_ARGS__)

#endif // LOGGER_CORE_H_