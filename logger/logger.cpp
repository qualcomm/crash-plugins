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

#include "logger.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(Logger)
#endif

LoggerLevel Logger::parseLoggerLevel(const std::string& level_str) {
    std::string upper_level = level_str;
    std::transform(upper_level.begin(), upper_level.end(), upper_level.begin(), ::toupper);
    if (upper_level == "DEBUG" || upper_level == "D") return LoggerLevel::LEVEL_DEBUG;
    if (upper_level == "INFO" || upper_level == "I")  return LoggerLevel::LEVEL_INFO;
    if (upper_level == "WARN" || upper_level == "W")  return LoggerLevel::LEVEL_WARN;
    if (upper_level == "ERROR" || upper_level == "E") return LoggerLevel::LEVEL_ERROR;
    return LoggerLevel::LEVEL_ERROR;
}

void Logger::init_command() {
    cmd_name = "logger";
    help_str_list = {
        "logger - Dynamic logging control for crash plugins",
        "",
        "USAGE:",
        "  logger                              - Show current logging status",
        "  logger help                         - Show this help message",
        "  logger <plugin> on|off              - Enable/disable plugin logging",
        "  logger <plugin> level <LEVEL>       - Set plugin log level (auto-enables)",
        "  logger reset                        - Reset all logging settings",
        "",
        "LOG LEVELS:",
        "  DEBUG (D)   - Debugging information (lowest level)",
        "  INFO  (I)   - General information",
        "  WARN  (W)   - Warning messages",
        "  ERROR (E)   - Error messages (default for unconfigured plugins)",
        "",
        "EXAMPLES:",
        "  logger                              - Display status report",
        "  logger coredump level DEBUG         - Set coredump to DEBUG (auto-enables)",
        "  logger coredump level D             - Same as above (short form)",
        "  logger coredump off                 - Disable coredump logging",
        "  logger reset                        - Reset all logging settings",
        "",
        "OUTPUT FORMAT:",
        "  [plugin:L] message",
        "  Example: [coredump:E] PID 0 is a kernel thread (no VM)",
        "",
        "DEFAULT BEHAVIOR:",
        "  - Unconfigured plugins automatically show ERROR logs",
        "  - To see DEBUG/INFO logs, explicitly set the level",
        "  - To disable all logs from a plugin, use 'logger <plugin> off'",
        "",
        "NOTES:",
        "  - Setting log level automatically enables the plugin",
        "  - Log levels filter messages: DEBUG < INFO < WARN < ERROR",
        "  - Plugin settings persist during crash session",
        "  - Use 'logger reset' to clear all settings"
    };
}

void Logger::cmd_main() {
    if (argcnt < 2) {
        showStatus();
        return;
    }
    std::string first_arg = args[1];
    if (first_arg == "help") {
        showHelp();
    } else if (first_arg == "reset") {
        resetLogger();
    } else if (first_arg == "status") {
        showStatus();
    } else {
        if (argcnt < 3) {
            fprintf(fp, "ERROR: Missing argument. Usage: logger %s on|off|level <LEVEL>\n", first_arg.c_str());
            return;
        }
        std::string second_arg = args[2];
        if (second_arg == "level") {
            if (argcnt >= 4) {
                setPluginLevel(first_arg, args[3]);
            } else {
                fprintf(fp, "ERROR: Missing level. Usage: logger %s level <LEVEL>\n", first_arg.c_str());
            }
        } else {
            setPluginState(first_arg, second_arg);
        }
    }
}

void Logger::setPluginLevel(const std::string& plugin_name, const std::string& level_str) {
    LoggerLevel level = parseLoggerLevel(level_str);
    if (!SimpleLogger::isPluginEnabled(plugin_name)) {
        SimpleLogger::enablePlugin(plugin_name);
        fprintf(fp, "Plugin '%s' automatically enabled\n", plugin_name.c_str());
    }
    SimpleLogger::setPluginLevel(plugin_name, level);
    fprintf(fp, "Plugin '%s' log level set to: %s\n",
            plugin_name.c_str(),
            LOG_LEVEL_NAMES[static_cast<int>(level)]);
}

void Logger::showStatus() {
    SimpleLogger::showStatus();
}

void Logger::showHelp() {
    fprintf(fp, "\n");
    for (const auto& help_line : help_str_list) {
        fprintf(fp, "%s\n", help_line.c_str());
    }
    fprintf(fp, "\n");
}

void Logger::setPluginState(const std::string& plugin_name, const std::string& state) {
    if (parseState(state)) {
        SimpleLogger::enablePlugin(plugin_name);
        fprintf(fp, "Plugin '%s' logging ENABLED\n", plugin_name.c_str());
    } else {
        SimpleLogger::disablePlugin(plugin_name);
        fprintf(fp, "Plugin '%s' logging DISABLED\n", plugin_name.c_str());
    }
}

void Logger::resetLogger() {
    fprintf(fp, "\n");
    fprintf(fp, "RESETTING LOGGER SYSTEM...\n");
    fprintf(fp, "- Clearing all plugin settings\n");
    SimpleLogger::forceCleanup();
    fprintf(fp, "Logger system reset completed\n");
    fprintf(fp, "All plugins reverted to default behavior\n");
    fprintf(fp, "(WARN/ERROR logs will still be shown)\n\n");
}

bool Logger::parseState(const std::string& state) {
    return (state == "on" || state == "ON" || state == "1" ||
            state == "true" || state == "TRUE" || state == "enable");
}

#pragma GCC diagnostic pop