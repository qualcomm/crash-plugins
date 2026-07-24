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

#ifndef PROP_INFO_DEFS_H_
#define PROP_INFO_DEFS_H_

#include "plugin.h"
#include "memory/swapinfo.h"
#include "../utils/utask.h"

/**
 * struct prop_area - Android property area header structure
 * @bytes_used_: Number of bytes used in the data section
 * @serial_: Serial number for property updates
 * @magic_: Magic number (0x504f5250 = "PROP") for validation
 * @version_: Property area format version
 * @reserved_: Reserved space for future use
 * @data_: Flexible array member for property data (binary tree)
 *
 * This structure represents the header of a memory-mapped property area.
 * It contains metadata about the property storage and is followed by
 * the actual property data organized as a binary tree.
 */
struct prop_area{
    uint32_t bytes_used_;       // Bytes used in data section
    unsigned int serial_;       // Serial number for updates
    uint32_t magic_;           // Magic number: 0x504f5250 ("PROP")
    uint32_t version_;         // Format version
    uint32_t reserved_[28];    // Reserved for future use
    char data_[0];             // Property data (flexible array)
};

/**
 * struct prop_bt - Property binary tree node structure
 * @namelen: Length of the property name
 * @prop: Offset to the prop_info structure
 * @left: Offset to the left child node
 * @right: Offset to the right child node
 * @children: Offset to child nodes (for hierarchical properties)
 * @name: Flexible array member for property name prefix
 *
 * This structure represents a node in the property binary tree.
 * Properties are organized in a tree structure for efficient lookup.
 * Each node contains offsets to related nodes and property data.
 */
struct prop_bt{
    uint32_t namelen;          // Length of property name
    unsigned int prop;         // Offset to prop_info
    unsigned int left;         // Offset to left child
    unsigned int right;        // Offset to right child
    unsigned int children;     // Offset to children nodes
    char name[0];              // Property name prefix (flexible array)
};

/**
 * struct prop_info - Property information structure
 * @serial: Serial number for this property
 * @value: Property value (up to 92 bytes, PROP_VALUE_MAX)
 * @long_property: Alternative structure for long properties
 * @error_message: Error message for long property failures
 * @offset: Offset to actual long property data
 * @name: Flexible array member for property name
 *
 * This structure contains the actual property data including its value.
 * For properties with values exceeding 92 bytes, the long_property
 * union member is used to store an offset to the actual data.
 */
struct prop_info {
    unsigned int serial;       // Property serial number
    union {
        char value[92];        // Property value (PROP_VALUE_MAX)
        struct {
            char error_message[56];  // Error message for long properties
            uint32_t offset;         // Offset to long property data
        } long_property;
    };
    char name[0];              // Property name (flexible array)
};

/**
 * struct symbol - Symbol file information
 * @name: Name of the symbol file (e.g., "libc.so")
 * @path: Full path to the loaded symbol file
 *
 * This structure tracks symbol files that have been loaded for
 * parsing property information from the crash dump.
 */
struct symbol {
    std::string name;          // Symbol file name
    std::string path;          // Full path to symbol file
};

/**
 * struct offset_list - Structure field offsets for property parsing
 *
 * This structure stores the byte offsets of various fields within
 * Android property system structures. These offsets are either obtained
 * from debug symbols or use hardcoded values based on architecture.
 *
 * The offsets are critical for correctly parsing property data structures
 * from crash dump memory, as structure layouts may vary between Android
 * versions and architectures (32-bit vs 64-bit).
 */
struct offset_list {
    int SystemProperties_contexts_;              // Offset to contexts_ field
    int ContextsSerialized_context_nodes_;       // Offset to context_nodes_ array
    int ContextsSerialized_num_context_nodes_;   // Offset to node count
    int ContextsSerialized_serial_prop_area_;    // Offset to serial prop area
    int ContextNode_pa_;                         // Offset to prop_area pointer
    int ContextNode_filename_;                   // Offset to filename string
    int ContextNode_context_;                    // Offset to context string
    int prop_bt_prop;                            // Offset to prop field in prop_bt
    int prop_area_data_;                         // Offset to data_ in prop_area
    int prop_info_name;                          // Offset to name in prop_info
};

/**
 * struct size_list - Structure sizes for property parsing
 *
 * This structure stores the sizes of various structures used in
 * Android property system. These sizes are needed for iterating
 * through arrays of structures in memory.
 */
struct size_list {
    int ContextNode;           // Size of ContextNode structure
};

/**
 * class PropInfo - Android system property parser
 *
 * This class provides functionality to parse and retrieve Android system
 * properties from crash dumps. It supports two parsing methods:
 * 1. Symbol-based parsing (preferred): Uses debug symbols from libc.so
 * 2. VMA-based parsing (fallback): Scans init process memory areas
 *
 * The class inherits from ParserPlugin and provides a base implementation
 * that can be extended by derived classes like Prop for command-line interface.
 */
class PropInfo : public ParserPlugin {
private:
    // Task context for init process (PID 1)
    struct task_context *tc_init;

    // Compatibility mode flag (32-bit process on 64-bit system)
    bool is_compat;

    // Property area size (from libc.so BSS section)
    size_t pa_size;

    // Property data size (from libc.so BSS section)
    size_t pa_data_size;

    // Structure field offsets for parsing
    struct offset_list g_offset;

    // Structure sizes for parsing
    struct size_list g_size;

    /**
     * get_symbol_file - Retrieve path of a loaded symbol file by name
     * @name: Symbol file name to search for
     *
     * Returns: Full path to symbol file, or empty string if not found
     */
    std::string get_symbol_file(std::string name);

    /**
     * print_propertys - Display all parsed properties (base implementation)
     *
     * Formats and prints all properties in prop_map with alignment.
     */
    void print_propertys();

    /**
     * parser_prop_area - Parse a property area structure
     * @vaddr: Virtual address of prop_area structure
     *
     * Reads and validates prop_area, then parses the property tree within it.
     * Returns: true on success, false on failure
     */
    bool parser_prop_area(size_t vaddr);

    /**
     * parser_prop_bt - Recursively parse property binary tree node
     * @root: Base address of property data area
     * @prop_bt_addr: Address of current prop_bt node
     *
     * Traverses the property binary tree and extracts all properties.
     */
    void parser_prop_bt(size_t root, size_t prop_bt_addr);

    /**
     * parser_prop_info - Parse property info structure
     * @prop_info_addr: Virtual address of prop_info structure
     *
     * Extracts property name and value, adds to prop_map.
     */
    void parser_prop_info(size_t prop_info_addr);

    /**
     * for_each_prop - Recursively traverse property tree in VMA data
     * @prop_bt_off: Offset of prop_bt node within VMA
     * @vma_len: Total length of VMA data
     * @vma_data: Pointer to VMA data buffer
     *
     * Parses properties from a contiguous memory buffer (VMA-based method).
     * Returns: true on success, false on failure
     */
    bool for_each_prop(uint32_t prop_bt_off, size_t vma_len, char *vma_data);

    /**
     * cleanString - Clean and validate property string
     * @str: Input string to clean
     *
     * Removes null terminators, trims whitespace, validates content.
     * Returns: Cleaned string, or empty string if invalid
     */
    std::string cleanString(const std::string &str);

    /**
     * init_task - Initialize init process task context
     *
     * Locates init process (PID 1) and creates task context for it.
     * Returns: true on success, false on failure
     */
    bool init_task();

protected:
    // Swap information handler for memory operations
    std::shared_ptr<Swapinfo> swap_ptr;

    // User task handler for reading init process memory
    std::shared_ptr<UTask> task_ptr;

    // Property map: property name -> property value
    std::unordered_map<std::string, std::string> prop_map;

public:
    /**
     * Symbol files required for property parsing
     * Currently only libc.so is needed for symbol-based parsing
     */
    std::vector<symbol> symbol_list = {
        {"libc.so", ""},  // libc.so contains property system symbols
    };

    /**
     * get_prop - Retrieve value of a specific property
     * @name: Property name to query
     *
     * Implements lazy loading: parses properties on first access.
     * Returns: Property value if found, empty string otherwise
     */
    std::string get_prop(std::string name);

    /**
     * parser_prop_by_init_vma - Parse properties from init VMA (fallback)
     *
     * Scans init process VMAs for property files and parses them.
     * Used when symbol-based parsing is not available.
     */
    void parser_prop_by_init_vma();

    /**
     * parser_propertys_by_sym - Parse properties using symbols (preferred)
     *
     * Uses debug symbols from libc.so to locate and parse properties.
     * Returns: true on success, false on failure
     */
    bool parser_propertys_by_sym();

    /**
     * PropInfo - Constructor
     * @swap: Shared pointer to Swapinfo for memory operations
     *
     * Initializes the property parser with swap information handler.
     */
    PropInfo(std::shared_ptr<Swapinfo> swap);

    /**
     * ~PropInfo - Destructor
     *
     * Cleans up resources and releases shared pointers.
     */
    ~PropInfo();

    /**
     * cmd_main - Command entry point (base implementation)
     *
     * Placeholder for derived classes to implement command handling.
     */
    void cmd_main(void) override;

    /**
     * init_offset - Initialize structure field offsets
     *
     * Initializes offsets for property structures, either from debug
     * symbols or using hardcoded values based on architecture.
     */
    void init_offset(void) override;

    /**
     * init_command - Initialize command metadata (base implementation)
     *
     * Placeholder for derived classes to set up command information.
     */
    void init_command(void) override;
};

#endif // PROP_INFO_DEFS_H_
