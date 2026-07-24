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

#ifndef LOGCAT_PARSER_DEFS_H_
#define LOGCAT_PARSER_DEFS_H_

#include "plugin.h"
#include "logcatR.h"
#include "logcatS.h"
#include "logcatLE.h"
#include "logcat.h"
#include <dirent.h> // for directory operations
#include <sys/stat.h> // for file status
#include "memory/swapinfo.h"
#include "property/propinfo.h"

class Logcat_Parser : public ParserPlugin {
private:
    // Pointer to the logcat implementation (R/S/LE version)
    std::unique_ptr<Logcat> logcat_ptr;

    // Shared pointer to swap information handler
    std::shared_ptr<Swapinfo> swap_ptr;

    // Shared pointer to property information handler
    std::shared_ptr<PropInfo> prop_ptr;

    // List of symbols required for logcat parsing
    // libc.so: C library symbols (libc.so.6 for LE)
    // logd: Android log daemon symbols
    std::vector<symbol> symbol_list = {
        {"libc.so", ""}, // for LE, the lib is libc.so.6
        {"logd", ""},
    };

    /**
     * Initialize the appropriate logcat implementation based on Android version
     * @return true if initialization successful, false otherwise
     */
    bool initialize_logcat();

    /**
     * Handle the buffer option (-b) to parse and print specific log buffer
     * @param buffer_name Name of the log buffer (main/radio/events/system/crash/etc.)
     */
    void handle_buffer_option(const char *buffer_name);

    /**
     * Handle the symbol option (-s) to load symbol files
     * @param symbol_path Path to the directory containing symbol files
     */
    void handle_symbol_option(const char *symbol_path);

public:
    // Default constructor
    Logcat_Parser();

    // Constructor with swap and property info dependencies
    Logcat_Parser(std::shared_ptr<Swapinfo> swap, std::shared_ptr<PropInfo> prop);

    // Main command execution entry point
    void cmd_main(void) override;

    // Initialize offset information (currently not used)
    void init_offset(void) override;

    // Initialize command metadata (name, help text, etc.)
    void init_command(void) override;

    DEFINE_PLUGIN_INSTANCE(Logcat_Parser)
};

#endif // LOGCAT_PARSER_DEFS_H_
