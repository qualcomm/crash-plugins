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

#include "prop.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(Prop)
#endif

/**
 * cmd_main - Main entry point for the getprop command
 *
 * This function processes command-line arguments and dispatches to the appropriate
 * handler based on the options provided. It supports operations like loading symbols,
 * displaying all properties, or querying specific property values.
 */
void Prop::cmd_main(void) {
    // Check if sufficient arguments are provided
    if (argcnt < 2) {
        LOGD("Insufficient arguments: argcnt=%d", argcnt);
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }

    int c;
    int argerrs = 0;
    bool has_valid_operation = false;

    // Process command-line options
    while ((c = getopt(argcnt, args, "s:ap:")) != EOF) {
        switch(c) {
            case 's': // Load symbol files from specified directory
                if (handle_symbol_option(optarg)) {
                    has_valid_operation = true;
                }
                break;
            case 'a': // Display all properties
                if (handle_all_properties_option()) {
                    has_valid_operation = true;
                }
                break;
            case 'p': // Query specific property by name
                if (handle_single_property_option(optarg)) {
                    has_valid_operation = true;
                }
                break;
            default:
                LOGE("Unknown option: %c", c);
                argerrs++;
                break;
        }
    }
    // Display usage if there were argument errors or no valid operation
    if (argerrs || !has_valid_operation) {
        cmd_usage(pc->curcmd, SYNOPSIS);
    }
}

void Prop::init_offset(void) {}

Prop::Prop(std::shared_ptr<Swapinfo> swap) : PropInfo(swap){}

Prop::Prop() : PropInfo(std::make_shared<Swapinfo>()) {}

void Prop::init_command(void){
    cmd_name = "getprop";
    help_str_list={
        "getprop",                                /* command name */
        "display Android system property information and analysis",  /* short description */
        "[-s symbol_dir] [-a] [-p prop_name]\n"
        "  This command analyzes Android system properties from crash dumps,\n"
        "  providing comprehensive property information extraction and display.\n"
        "  It supports symbol-based parsing and fallback VMA parsing methods.\n"
        "\n"
        "    -s symbol_dir   load symbol files from specified directory for parsing\n"
        "    -a              display all system properties with formatted output\n"
        "    -p prop_name    query and display value of specific property by name\n",
        "\n",
        "EXAMPLES",
        "  Load symbol for property:",
        "    %s> getprop -s <libc.so symbol path>",
        "    Add symbol table from file \"path/libc.so\"",
        "    Reading symbols from path/libc.so...",
        "\n",
        "  Display all propertys:",
        "    %s> getprop -a",
        "    [0001]wifi.aware.interface                                                   wifi-aware0",
        "    [0002]ro.crypto.state                                                        encrypted",
        "    [0003]ro.crypto.type                                                         file",
        "\n",
        "  Display specified property's value:",
        "    %s> getprop -p ro.crypto.state",
        "    encrypted",
        "\n",
    };
}

/**
 * print_propertys - Display all Android system properties
 *
 * This function formats and prints all properties stored in prop_map.
 * Properties are displayed with index numbers and aligned for readability.
 */
void Prop::print_propertys(){
    LOGD("Printing %zu properties", prop_map.size());
    // Calculate maximum property name length for alignment
    size_t max_len = 0;
    for (const auto& pair : prop_map) {
        max_len = std::max(max_len, pair.first.size());
    }
    // Print header
    PRINT("═══════════════════════════════════════════════════════════════\n");
    PRINT("                    ANDROID SYSTEM PROPERTIES\n");
    PRINT("═══════════════════════════════════════════════════════════════\n");
    PRINT("Total properties: %zu\n", prop_map.size());
    PRINT("═══════════════════════════════════════════════════════════════\n");
    // Format and print each property
    size_t index = 1;
    std::ostringstream oss;
    for (const auto& pair : prop_map) {
        oss << "[" << std::setw(4) << std::right << std::setfill('0') << std::dec << index << "]"
            << std::left << std::setw(max_len) << std::setfill(' ') << pair.first << " "
            << std::left << pair.second
            << "\n";
        index++;
    }
    PRINT("%s", oss.str().c_str());
}

/**
 * handle_symbol_option - Process the symbol loading option
 * @optarg: Path to the directory containing symbol files
 *
 * This function validates the symbol path and attempts to load symbol files
 * from the specified directory. Symbol files are needed to parse property
 * information from the crash dump.
 *
 * Returns: true if symbols were loaded successfully, false otherwise
 */
bool Prop::handle_symbol_option(const char* optarg) {
    // Validate input parameter
    if (!optarg || strlen(optarg) == 0) {
        LOGE("Symbol path cannot be empty");
        return false;
    }
    try {
        std::string symbol_path(optarg);
        if (symbol_path.empty()) {
            LOGE("Invalid symbol path: %s", optarg);
            return false;
        }
        bool result = load_symbol_files(symbol_path);
        if (result) {
            LOGD("Successfully loaded symbols from: %s", symbol_path.c_str());
        } else {
            LOGE("Failed to load symbols from: %s", symbol_path.c_str());
        }

        return result;
    } catch (const std::exception& e) {
        LOGE("Exception while processing symbol path %s: %s", optarg, e.what());
        return false;
    } catch (...) {
        LOGE("Unknown exception while processing symbol path: %s", optarg);
        return false;
    }
}

/**
 * handle_all_properties_option - Display all Android system properties
 *
 * This function ensures properties are loaded from the crash dump and then
 * displays all of them in a formatted manner.
 *
 * Returns: true if properties were displayed successfully, false otherwise
 */
bool Prop::handle_all_properties_option() {
    try {
        // Ensure properties are loaded (lazy initialization)
        if (!ensure_properties_loaded()) {
            LOGE("Failed to load properties");
            return false;
        }
        LOGD("Properties loaded successfully, count: %zu", prop_map.size());
        // Display all properties
        print_propertys();
        return true;
    } catch (const std::exception& e) {
        LOGE("Exception while displaying all properties: %s", e.what());
        return false;
    } catch (...) {
        LOGE("Unknown exception while displaying all properties");
        return false;
    }
}

/**
 * handle_single_property_option - Query and display a specific property value
 * @optarg: Name of the property to query
 *
 * This function retrieves and displays the value of a specific Android system
 * property by name. If the property doesn't exist, an appropriate message is shown.
 *
 * Returns: true if the operation completed successfully, false on error
 */
bool Prop::handle_single_property_option(const char* optarg) {
    // Validate input parameter
    if (!optarg || strlen(optarg) == 0) {
        LOGE("Property name cannot be empty");
        return false;
    }
    try {
        std::string prop_name(optarg);
        if (prop_name.empty()) {
            LOGE("Invalid property name: %s", optarg);
            return false;
        }
        LOGD("Querying property: %s", prop_name.c_str());
        // Retrieve property value
        std::string result = get_prop(prop_name);
        if (result.empty()) {
            LOGD("Property '%s' not found or has empty value", prop_name.c_str());
            return true;
        }
        LOGD("Property '%s' value: %s", prop_name.c_str(), result.c_str());
        PRINT("%s\n", result.c_str());
        return true;
    } catch (const std::exception& e) {
        LOGE("Exception while getting property %s: %s", optarg, e.what());
        return false;
    } catch (...) {
        LOGE("Unknown exception while getting property: %s", optarg);
        return false;
    }
}

/**
 * load_symbol_files - Load symbol files from the specified directory
 * @symbol_path: Directory path containing symbol files
 *
 * This function iterates through the symbol_list and attempts to load each
 * symbol file from the specified directory. Symbol files are required to
 * parse property information from the crash dump.
 *
 * Returns: true if at least one symbol file was loaded successfully, false otherwise
 */
bool Prop::load_symbol_files(const std::string& symbol_path) {
    LOGD("Loading symbol files from directory: %s", symbol_path.c_str());
    bool loaded_any = false;
    // Iterate through all symbols in the list
    for (auto& symbol : symbol_list) {
        // Construct full path to symbol file
        std::string full_path = symbol_path;
        if (!full_path.empty() && full_path.back() != '/') {
            full_path += "/";
        }
        full_path += symbol.name;
        LOGD("Attempting to load symbol: %s from %s", symbol.name.c_str(), full_path.c_str());
        // Attempt to load the symbol file
        if (load_symbols(full_path, symbol.name)) {
            symbol.path = full_path;
            loaded_any = true;
            LOGD("Successfully loaded symbol: %s from %s", symbol.name.c_str(), full_path.c_str());
        } else {
            LOGE("Failed to load symbol: %s from %s", symbol.name.c_str(), full_path.c_str());
        }
    }
    return loaded_any;
}

/**
 * ensure_properties_loaded - Ensure properties are loaded from crash dump
 *
 * This function implements lazy initialization for property loading. It first
 * attempts to parse properties using symbol information. If that fails, it
 * falls back to parsing from init process VMA (Virtual Memory Area).
 *
 * Returns: true if properties were loaded successfully, false otherwise
 */
bool Prop::ensure_properties_loaded() {
    // Check if properties are already loaded (cached)
    if (!prop_map.empty()) {
        LOGD("Properties already loaded, count: %zu", prop_map.size());
        return true;
    }
    LOGD("Properties not loaded, attempting to parse from crash dump");
    if (parser_propertys_by_sym()) {
        LOGD("Successfully parsed properties using symbols, count: %zu", prop_map.size());
    } else {
        LOGE("Failed to parse properties using symbols, falling back to VMA parsing");
        parser_prop_by_init_vma();
        if (!prop_map.empty()) {
            LOGD("Successfully parsed properties from VMA, count: %zu", prop_map.size());
        } else {
            LOGE("Failed to parse properties from VMA");
        }
    }
    // Clean up task pointer after parsing
    if (task_ptr != nullptr) {
        task_ptr.reset();
    }
    // Verify that properties were loaded
    if (prop_map.empty()) {
        LOGE("Failed to load any properties");
        return false;
    }
    LOGD("Properties loaded successfully, total count: %zu", prop_map.size());
    return true;
}

#pragma GCC diagnostic pop
