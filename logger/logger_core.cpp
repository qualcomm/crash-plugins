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

#include "logger_core.h"
#include <cstdio>
#include <cstdarg>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

extern FILE *fp;

const char* LOG_LEVEL_NAMES[] = {"DEBUG", "INFO", "WARN", "ERROR"};
const char* LOG_LEVEL_PREFIX[] = {"D", "I", "W", "E"};

std::unordered_map<std::string, SimpleLogger::PluginState> SimpleLogger::plugin_states_;
std::string SimpleLogger::current_context_ = "";

void SimpleLogger::set_context(const std::string& name) {
    current_context_ = name;
}

void SimpleLogger::clear_context() {
    current_context_.clear();
}

std::string SimpleLogger::get_context() {
    if (current_context_.empty()) {
        // static bool warned = false;
        // if (!warned) {
        //     fprintf(fp, "[LOGGER WARNING] No context set, using 'system'.\n");
        //     warned = true;
        // }
        return "system";
    }
    return current_context_;
}

void SimpleLogger::enablePlugin(const std::string& plugin_name) {
    plugin_states_[plugin_name].enabled = true;
    plugin_states_[plugin_name].last_update = time(nullptr);
}

void SimpleLogger::disablePlugin(const std::string& plugin_name) {
    plugin_states_[plugin_name].enabled = false;
    plugin_states_[plugin_name].last_update = time(nullptr);
}

bool SimpleLogger::isPluginEnabled(const std::string& plugin_name) {
    auto it = plugin_states_.find(plugin_name);
    return (it != plugin_states_.end()) ? it->second.enabled : false;
}

void SimpleLogger::setPluginLevel(const std::string& plugin_name, LoggerLevel level) {
    plugin_states_[plugin_name].level = level;
    plugin_states_[plugin_name].last_update = time(nullptr);
}

LoggerLevel SimpleLogger::getPluginLevel(const std::string& plugin_name) {
    auto it = plugin_states_.find(plugin_name);
    return (it != plugin_states_.end()) ? it->second.level : LoggerLevel::LEVEL_ERROR;
}

bool SimpleLogger::shouldLog(const std::string& plugin_name, LoggerLevel level) {
    auto it = plugin_states_.find(plugin_name);
    if (it != plugin_states_.end()) {
        if (!it->second.enabled) {
            return false;
        }
        return level >= it->second.level;
    }
    return level >= LoggerLevel::LEVEL_ERROR;
}

void SimpleLogger::log(const std::string& plugin_name, LoggerLevel level, const char* format, ...) {
    if (!shouldLog(plugin_name, level)) return;
    if (fp) {
        fprintf(fp, "[%s:%s] ", plugin_name.c_str(), LOG_LEVEL_PREFIX[static_cast<int>(level)]);
        va_list args;
        va_start(args, format);
        vfprintf(fp, format, args);
        va_end(args);
        fflush(fp);
    }
}

void SimpleLogger::showStatus() {
    if (!fp) return;
    fprintf(fp, "\n");
    fprintf(fp, "================================================================\n");
    fprintf(fp, "                    Logger Status Report                       \n");
    fprintf(fp, "================================================================\n");
    fprintf(fp, "\n");
    if (!plugin_states_.empty()) {
        fprintf(fp, "Configured Plugins:\n");
        fprintf(fp, "  %-15s  %-8s  %-7s  %s\n", "Plugin", "Status", "Level", "Last Update");
        fprintf(fp, "  %-15s  %-8s  %-7s  %s\n", "------", "------", "-----", "-----------");
        for (const auto& pair : plugin_states_) {
            const char* status = pair.second.enabled ? "ON" : "OFF";
            const char* level = LOG_LEVEL_NAMES[static_cast<int>(pair.second.level)];
            char time_str[32] = "Never";
            if (pair.second.last_update > 0) {
                struct tm* tm_info = localtime(&pair.second.last_update);
                strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);
            }
            fprintf(fp, "  %-15s  %-8s  %-7s  %s\n",
                   pair.first.c_str(), status, level, time_str);
        }
        fprintf(fp, "\n");

        int enabled_count = 0;
        for (const auto& pair : plugin_states_) {
            if (pair.second.enabled) enabled_count++;
        }
        fprintf(fp, "Total: %zu configured, %d enabled\n", plugin_states_.size(), enabled_count);
    } else {
        fprintf(fp, "No plugins configured\n");
        fprintf(fp, "\n");
    }
    fprintf(fp, "\n");
    fprintf(fp, "Default Behavior:\n");
    fprintf(fp, "  - Unconfigured plugins: WARN/ERROR auto-enabled\n");
    fprintf(fp, "  - Configured plugins: use specified level\n");
    fprintf(fp, "\n");
    fprintf(fp, "Log Levels: DEBUG < INFO < WARN < ERROR\n");
    fprintf(fp, "Output Format: [plugin:level] message\n");
    fprintf(fp, "  Level Codes: D=DEBUG, I=INFO, W=WARN, E=ERROR\n");
    fprintf(fp, "================================================================\n");
    if (plugin_states_.empty()) {
        fprintf(fp, "\nTIP: Configure plugin logging with 'logger <plugin> level <LEVEL>'\n");
        fprintf(fp, "     Example: logger coredump level DEBUG\n");
    }
    fprintf(fp, "\n");
}

void SimpleLogger::forceCleanup() {
    plugin_states_.clear();
}

void SimpleLogger::print(const char* fmt, ...) {
    if (!fmt || !fp) return;
    va_list args;
    va_start(args, fmt);
    vfprintf(fp, fmt, args);
    va_end(args);
    fflush(fp);
}

#pragma GCC diagnostic pop