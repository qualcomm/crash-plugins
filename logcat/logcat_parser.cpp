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

#include "logcat_parser.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(Logcat_Parser)
#endif

static const std::unordered_map<std::string, LOG_ID> stringToLogID = {
    {"main", MAIN},
    {"radio", RADIO},
    {"events", EVENTS},
    {"system", SYSTEM},
    {"crash", CRASH},
    {"stats", STATS},
    {"security", SECURITY},
    {"kernel", KERNEL},
    {"all", ALL}
};

/**
 * Initialize offset information
 * Currently not used for logcat parser
 */
void Logcat_Parser::init_offset(void) {
    LOGD("Logcat_Parser::init_offset() called\n");
}

/**
 * Main command execution function
 * Parses command line arguments and dispatches to appropriate handlers
 */
void Logcat_Parser::cmd_main(void) {
    // Check if minimum arguments are provided
    if (argcnt < 2) {
        LOGE("Insufficient arguments provided (argcnt=%d)\n", argcnt);
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }

    // Parse command line options
    int c;
    while ((c = getopt(argcnt, args, "b:s:")) != EOF) {
        switch(c) {
            case 'b':
                LOGD("Processing buffer option: %s\n", optarg);
                handle_buffer_option(optarg);
                break;
            case 's':
                LOGD("Processing symbol option: %s\n", optarg);
                handle_symbol_option(optarg);
                break;
            default:
                LOGE("Unknown option encountered: %c\n", c);
                argerrs++;
                break;
        }
    }
    // Display usage if there were argument errors
    if (argerrs) {
        LOGE("Argument errors detected (argerrs=%d)\n", argerrs);
        cmd_usage(pc->curcmd, SYNOPSIS);
    }
}

/**
 * Handle buffer option to parse and print specific log buffer
 * @param buffer_name Name of the log buffer to display
 */
void Logcat_Parser::handle_buffer_option(const char* buffer_name) {
    LOGD("handle_buffer_option() entry, buffer_name=%s\n",
         buffer_name ? buffer_name : "null");
    // Validate buffer name parameter
    if (!buffer_name || strlen(buffer_name) == 0) {
        LOGE("Invalid buffer name: %s\n", buffer_name ? buffer_name : "null");
        return;
    }
    // Initialize logcat if not already initialized
    if (!logcat_ptr) {
        if (!initialize_logcat()) {
            LOGE("Failed to initialize logcat\n");
            return;
        }
        LOGD("Logcat initialized successfully\n");
    }
    // Search for logd symbol in the symbol list
    LOGD("Searching for logd symbol in symbol_list (size=%zu)\n", symbol_list.size());
    bool found_logd_symbol = false;
    for (const auto& symbol : symbol_list) {
        LOGD("Checking symbol: name=%s, path=%s\n",
             symbol.name.c_str(), symbol.path.c_str());
        if (symbol.name == "logd" && !symbol.path.empty()) {
            found_logd_symbol = true;
            logcat_ptr->logd_symbol = symbol.path;
            LOGD("Found and set logd symbol: %s\n", symbol.path.c_str());
            break;
        }
    }
    if (!found_logd_symbol) {
        LOGD("No logd symbol found in symbol_list, proceeding without it\n");
    }

    // Parse logcat logs from memory
    LOGD("Calling parser_logcat_log() to parse logs from memory\n");
    logcat_ptr->parser_logcat_log();
    // Convert buffer name string to LOG_ID enum and print
    auto it = stringToLogID.find(buffer_name);
    if (it != stringToLogID.end()) {
        LOGD("Found valid LOG_ID for buffer '%s', printing logs\n", buffer_name);
        logcat_ptr->print_logcat_log(it->second);
    } else {
        LOGE("Invalid buffer name for LOG_ID: %s\n", buffer_name);
    }
}

/**
 * Handle symbol option to load symbol files from specified directory
 * @param symbol_path Path to directory containing symbol files
 */
void Logcat_Parser::handle_symbol_option(const char* symbol_path) {
    LOGD("handle_symbol_option() entry, symbol_path=%s\n",
         symbol_path ? symbol_path : "null");
    // Validate symbol path parameter
    if (!symbol_path || strlen(symbol_path) == 0) {
        LOGE("Invalid symbol path: %s\n", symbol_path ? symbol_path : "null");
        return;
    }

    try {
        std::string symbol_path_str(symbol_path);
        LOGD("Processing symbol path: %s\n", symbol_path_str.c_str());

        // Iterate through each symbol in the symbol list
        for (auto& symbol : symbol_list) {
            LOGD("Attempting to load symbol: %s from path: %s\n",
                 symbol.name.c_str(), symbol_path_str.c_str());

            std::string sym_path = symbol_path_str;

            // Try to load symbols for this symbol name
            if (load_symbols(sym_path, symbol.name)) {
                symbol.path = sym_path;
                LOGD("Successfully loaded symbol: %s, path: %s\n",
                     symbol.name.c_str(), sym_path.c_str());

                // Update corresponding symbol in property info symbol list
                for (auto& prop_symbol : prop_ptr->symbol_list) {
                    if (prop_symbol.name == symbol.name) {
                        prop_symbol.path = sym_path;
                        LOGD("Updated prop_symbol: %s with path: %s\n",
                             prop_symbol.name.c_str(), sym_path.c_str());
                        break;
                    }
                }
            } else {
                LOGD("Failed to load symbol: %s from path: %s\n",
                     symbol.name.c_str(), sym_path.c_str());
            }
        }

        LOGD("Symbol loading completed\n");
    } catch (const std::exception& e) {
        LOGE("Exception while processing symbol path %s: %s\n", symbol_path, e.what());
    } catch (...) {
        LOGE("Unknown exception while processing symbol path: %s\n", symbol_path);
    }
}

/**
 * Initialize the appropriate logcat implementation based on Android version
 * Detects whether the system is Linux Embedded (LE) or Android (R/S)
 * @return true if initialization successful, false otherwise
 */
bool Logcat_Parser::initialize_logcat() {
    // Get task context for PID 1 to determine system type
    struct task_context* tc = pid_to_context(1);
    if (!tc) {
        LOGE("Failed to get task context for PID 1\n");
        return false;
    }
    LOGD("PID 1 process name: %s\n", tc->comm);
    int android_ver = 30; // Default to Android R (version 11)
    // Check if this is a Linux Embedded system (systemd as init)
    if (strstr(tc->comm, "systemd")) {
        LOGD("Detected Linux Embedded system (systemd)\n");
        Logcat::is_LE = true;
        logcat_ptr = std::make_unique<LogcatLE>(swap_ptr);
        PRINT("Android version is: LE!\n");
        LOGD("LogcatLE instance created successfully\n");
        return true;
    }
    // This is an Android system, determine the version
    LOGD("Detected Android system, determining version\n");
    // Try to get Android SDK version from system properties
    std::string version = prop_ptr->get_prop("ro.build.version.sdk");
    if (version.empty()) {
        LOGD("ro.build.version.sdk not found, trying ro.vndk.version\n");
        version = prop_ptr->get_prop("ro.vndk.version");
    }
    if (!version.empty()) {
        try {
            android_ver = std::stoi(version);
            LOGD("Parsed Android version: %d\n", android_ver);
        } catch (const std::exception& e) {
            LOGE("Exception while parsing Android version: %s\n", e.what());
        }
    } else {
        LOGE("Can't get Android version from this dump!\n");
    }
    PRINT("Android version is: %d!\n", android_ver);
    // Create appropriate logcat implementation based on version
    if (android_ver >= 31) {
        // Android 12 (S) and above
        LOGD("Creating LogcatS instance for Android %d\n", android_ver);
        logcat_ptr = std::make_unique<LogcatS>(swap_ptr);
    } else {
        // Android 11 (R) and below
        LOGD("Creating LogcatR instance for Android %d\n", android_ver);
        logcat_ptr = std::make_unique<LogcatR>(swap_ptr);
    }
    return true;
}

/**
 * Constructor with dependency injection
 * @param swap Shared pointer to Swapinfo instance
 * @param prop Shared pointer to PropInfo instance
 */
Logcat_Parser::Logcat_Parser(std::shared_ptr<Swapinfo> swap, std::shared_ptr<PropInfo> prop)
    : swap_ptr(swap), prop_ptr(prop) {
}

/**
 * Default constructor
 * Creates new instances of Swapinfo and PropInfo
 */
Logcat_Parser::Logcat_Parser() {
    swap_ptr = std::make_shared<Swapinfo>();
    prop_ptr = std::make_shared<PropInfo>(swap_ptr);
}

/**
 * Initialize command metadata
 * Sets up command name and help text for the logcat parser plugin
 */
void Logcat_Parser::init_command(void) {
    // Set command name
    cmd_name = "logcat";
    LOGD("Command name set to: %s\n", cmd_name.c_str());

    // Set help text list with usage examples
    help_str_list = {
        "logcat",                                  /* command name */
        "dump logcat log information",            /* short description */
        "[-b buffer_name] [-s symbol_directory]\n"
        "  This command dumps logcat log information from kernel memory.\n"
        "\n"
        "    -b buffer_name  display logs from specified buffer (main/radio/events/system/crash/stats/security/kernel/all)\n"
        "    -s symbol_dir   load symbol files from specified directory (required for some features)\n",
        "\n",
        "EXAMPLES",
        "  Load logd symbol file from directory:",
        "    %s> logcat -s /path/to/symbols",
        "    Add symbol table from file \"/path/to/symbols/logd\"",
        "    Reading symbols from /path/to/symbols/logd...",
        "\n",
        "  Display all logcat logs:",
        "    %s> logcat -b all",
        "    01-17 10:15:23.456  1234  1234 I ActivityManager: Start proc com.example.app",
        "    01-17 10:15:23.789  1234  1234 D WindowManager: Window added",
        "\n",
        "  Display main buffer logs:",
        "    %s> logcat -b main",
        "    01-17 10:15:23.456  1234  1234 I ActivityManager: Start proc com.example.app",
        "\n",
        "  Display system buffer logs:",
        "    %s> logcat -b system",
        "    01-17 10:15:24.123  5678  5678 W SystemServer: Service timeout",
        "\n",
        "  Display radio buffer logs:",
        "    %s> logcat -b radio",
        "    01-17 10:15:25.456  9012  9012 D RIL: Signal strength updated",
        "\n",
        "  Display crash buffer logs:",
        "    %s> logcat -b crash",
        "    01-17 10:15:26.789  3456  3456 E AndroidRuntime: FATAL EXCEPTION",
        "\n",
        "  Display events buffer logs:",
        "    %s> logcat -b events",
        "    01-17 10:15:27.012  7890  7890 I am_proc_start: [0,1234,10001,com.example.app]",
        "\n",
    };
}

#pragma GCC diagnostic pop
