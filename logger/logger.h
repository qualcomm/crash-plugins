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

#ifndef LOGGER_H
#define LOGGER_H

#include "plugin.h"

class Logger : public ParserPlugin {
public:
    DEFINE_PLUGIN_INSTANCE(Logger)

    void init_command() override;
    void init_offset() override {}
    void cmd_main() override;

private:
    void showStatus();
    void showHelp();
    void setPluginState(const std::string& plugin_name, const std::string& state);
    void setPluginLevel(const std::string& plugin_name, const std::string& level_str);
    void resetLogger();
    bool parseState(const std::string& state);
    LoggerLevel parseLoggerLevel(const std::string& level_str);
};

#endif // LOGGER_H