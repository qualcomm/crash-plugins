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

#ifndef PROP_DEFS_H_
#define PROP_DEFS_H_

#include "property/propinfo.h"

/**
 * class Prop - Command-line interface for Android property parser
 *
 * This class extends PropInfo to provide a command-line interface for
 * querying and displaying Android system properties from crash dumps.
 * It implements the getprop command functionality with support for:
 * - Loading symbol files for parsing
 * - Displaying all properties
 * - Querying specific properties by name
 *
 * The class handles command-line argument parsing and delegates the
 * actual property parsing to the base PropInfo class.
 */
class Prop : public PropInfo {
private:
    /**
     * print_propertys - Display all properties with formatted output
     *
     * Overrides the base class implementation to provide enhanced
     * formatting with headers and property count information.
     */
    void print_propertys();

    /**
     * handle_symbol_option - Process symbol loading command option
     * @optarg: Path to directory containing symbol files
     *
     * Validates the symbol path and loads symbol files needed for
     * property parsing. Symbol files (e.g., libc.so) contain debug
     * information required for symbol-based parsing.
     *
     * Returns: true if symbols loaded successfully, false otherwise
     */
    bool handle_symbol_option(const char* optarg);

    /**
     * handle_all_properties_option - Process display all properties option
     *
     * Ensures properties are loaded and displays all of them in a
     * formatted manner with alignment and indexing.
     *
     * Returns: true if properties displayed successfully, false otherwise
     */
    bool handle_all_properties_option();

    /**
     * handle_single_property_option - Process single property query option
     * @optarg: Name of the property to query
     *
     * Retrieves and displays the value of a specific property by name.
     * If the property doesn't exist, displays an appropriate message.
     *
     * Returns: true if operation completed successfully, false on error
     */
    bool handle_single_property_option(const char* optarg);

    /**
     * load_symbol_files - Load symbol files from specified directory
     * @symbol_path: Directory path containing symbol files
     *
     * Iterates through the symbol_list and attempts to load each symbol
     * file from the specified directory. Provides feedback on success/failure
     * for each symbol file.
     *
     * Returns: true if at least one symbol loaded, false if all failed
     */
    bool load_symbol_files(const std::string& symbol_path);

    /**
     * ensure_properties_loaded - Ensure properties are loaded (lazy init)
     *
     * Implements lazy initialization for property loading. Attempts
     * symbol-based parsing first, falls back to VMA-based parsing if needed.
     * Properties are cached after first load for efficiency.
     *
     * Returns: true if properties loaded successfully, false otherwise
     */
    bool ensure_properties_loaded();

public:
    /**
     * Prop - Default constructor
     *
     * Creates a Prop instance with a new Swapinfo handler.
     * Used when no existing Swapinfo is available.
     */
    Prop();

    /**
     * Prop - Constructor with Swapinfo
     * @swap: Shared pointer to existing Swapinfo handler
     *
     * Creates a Prop instance using an existing Swapinfo handler.
     * Allows sharing of swap information across multiple parsers.
     */
    Prop(std::shared_ptr<Swapinfo> swap);

    /**
     * cmd_main - Main command entry point
     *
     * Processes command-line arguments and dispatches to appropriate
     * handlers. Supports options for:
     * -s <path>  : Load symbol files from directory
     * -a         : Display all properties
     * -p <name>  : Query specific property by name
     */
    void cmd_main(void) override;

    /**
     * init_offset - Initialize structure offsets (no-op for Prop)
     *
     * Offset initialization is handled by the base PropInfo class.
     * This override provides a no-op implementation.
     */
    void init_offset(void) override;

    /**
     * init_command - Initialize command metadata
     *
     * Sets up command name, description, usage information, and examples
     * for the getprop command. This information is displayed when the
     * user requests help or provides invalid arguments.
     */
    void init_command(void) override;

    // Plugin instance definition macro
    DEFINE_PLUGIN_INSTANCE(Prop)
};

#endif // PROP_DEFS_H_
