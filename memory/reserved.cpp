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

#include "reserved.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(Reserved)
#endif

/**
 * @brief Main command entry point for reserved memory analysis
 *
 * Parses command line arguments and dispatches to appropriate handler functions:
 * -a: Display all reserved memory regions with detailed information
 */
void Reserved::cmd_main(void) {
    int c;

    // Check minimum argument count
    if (argcnt < 2) cmd_usage(pc->curcmd, SYNOPSIS);

    // Parse reserved memory regions if not already done
    if (mem_list.size() == 0){
        parser_reserved_mem();
    }

    // Parse command line options
    while ((c = getopt(argcnt, args, "a")) != EOF) {
        switch(c) {
            case 'a':
                print_reserved_mem();
                break;
            default:
                argerrs++;
                break;
        }
    }

    // Handle argument errors
    if (argerrs)
        cmd_usage(pc->curcmd, SYNOPSIS);
}

/**
 * @brief Initialize kernel structure field offsets
 *
 * Sets up field offsets for reserved_mem structure fields used in reserved memory analysis.
 * These offsets are essential for reading reserved memory data from kernel memory.
 */
void Reserved::init_offset(void) {
    // Initialize reserved_mem structure field offsets
    field_init(reserved_mem,name);      // Reserved memory region name
    field_init(reserved_mem,base);      // Physical base address
    field_init(reserved_mem,size);      // Size of reserved region
    struct_init(reserved_mem);          // Initialize reserved_mem structure size
}

/**
 * @brief Initialize command help and usage information
 *
 * Sets up the command name, description, and detailed help text including
 * usage examples and expected output formats for the reserved memory plugin.
 */
void Reserved::init_command(void) {
    cmd_name = "reserved";
    help_str_list={
        "reserved",                                    /* command name */
        "display reserved memory regions information", /* short description */
        "[-a]\n"
        "  This command displays information about reserved memory regions.\n"
        "\n"
        "    -a              display all reserved memory regions with detailed information\n",
        "\n",
        "EXAMPLES",
        "  List all reserved memory regions:",
        "    %s> reserved -a",
        "    Reserved Memory Regions Overview",
        "    ┌────┬──────────────────────────────────┬──────────────────┬───────────────────────────┬──────────┬──────────┐",
        "    │   #│Region Name                       │Base Address      │Physical Memory Range      │Total Size│Status    │",
        "    ├────┼──────────────────────────────────┼──────────────────┼───────────────────────────┼──────────┼──────────┤",
        "    │  1 │gunyah_hyp_region@80000000        │0x0000000080000000│[0x080000000~0x080e00000]  │      14MB│NO-MAP    │",
        "    │  2 │xbl_sc_region@81800000            │0x0000000081800000│[0x081800000~0x081840000]  │     256KB│NO-MAP    │",
        "    │  3 │cpucp_fw_region@81840000          │0x0000000081840000│[0x081840000~0x081a00000]  │    1.75MB│NO-MAP    │",
        "    ├────┴──────────────────────────────────┴──────────────────┴───────────────────────────┴──────────┴──────────┤",
        "    │ Regions: 3 | Total: 403.57MB | NO-MAP: 168.57MB | REUSABLE: 192MB | UNKNOWN: 43MB                          │",
        "    └────────────────────────────────────────────────────────────────────────────────────────────────────────────┘",
        "\n",
    };
}

/**
 * @brief Constructor - initializes device tree parser
 *
 * Creates a shared device tree parser instance for analyzing reserved memory
 * region properties and types from the device tree information.
 */
Reserved::Reserved(){
    dts = std::make_shared<Devicetree>();
}

/**
 * @brief Parse and collect reserved memory region information from kernel memory
 *
 * Reads the kernel's reserved memory array and extracts detailed information
 * about each reserved memory region including size, location, and type classification
 * by analyzing device tree properties.
 */
void Reserved::parser_reserved_mem(){
    // Check if reserved_mem symbol exists in kernel
    if (!csymbol_exists("reserved_mem")){
        LOGE("reserved_mem doesn't exist in this kernel!\n");
        return;
    }

    // Get reserved memory array address
    ulong reserved_mem_addr = csymbol_value("reserved_mem");
    if (!is_kvaddr(reserved_mem_addr)) {
        LOGE("reserved_mem address is invalid!\n");
        return;
    }

    // Calculate number of reserved memory regions
    int cnt = get_symbol_length(TO_CONST_STRING("reserved_mem")) / struct_size(reserved_mem);

    // Process each reserved memory region
    for (int i = 0; i < cnt; ++i) {
        ulong reserved_addr = reserved_mem_addr + i * struct_size(reserved_mem);
        if (!is_kvaddr(reserved_addr)) continue;

        // Read reserved memory structure from kernel
        void *reserved_mem_buf = read_struct(reserved_addr,"reserved_mem");
        if (!reserved_mem_buf) continue;

        // Extract base address and size (handle 32-bit vs 64-bit physical addresses)
        uint64_t base = 0;
        uint64_t size = 0;
        if(get_config_val("CONFIG_PHYS_ADDR_T_64BIT") == "y"){
            // 64-bit physical addresses
            base = ULONGLONG(reserved_mem_buf + field_offset(reserved_mem,base));
            size = ULONGLONG(reserved_mem_buf + field_offset(reserved_mem,size));
        }else{
            // 32-bit physical addresses
            base = ULONG(reserved_mem_buf + field_offset(reserved_mem,base));
            size = ULONG(reserved_mem_buf + field_offset(reserved_mem,size));
        }

        // Skip invalid regions (zero base or size)
        if (base ==0 || size == 0) {
            FREEBUF(reserved_mem_buf);
            continue;
        }

        // Create new reserved memory region structure
        std::shared_ptr<reserved_mem> mem_ptr = std::make_shared<reserved_mem>();

        // Read region name
        std::string name = read_cstring(ULONG(reserved_mem_buf + field_offset(reserved_mem,name)),64, "reserved_mem_name");
        mem_ptr->addr = reserved_addr;
        mem_ptr->name = name;

        // Find corresponding device tree node to determine region type
        std::vector<std::shared_ptr<device_node>> nodes = dts->find_node_by_name(name);
        if (nodes.size() == 0)continue;

        // Analyze device tree properties to classify region type
        for (const auto& node : nodes) {
            mem_ptr->type = Type::UNKNOW;  // Default to unknown type

            // Check for "no-map" property (region not mapped to kernel virtual space)
            std::shared_ptr<Property> prop = dts->getprop(node->addr,"no-map");
            if (prop.get() != nullptr){
                mem_ptr->type = Type::NO_MAP;
            }

            // Check for "reusable" property (region can be reused by kernel)
            prop = dts->getprop(node->addr,"reusable");
            if (prop.get() != nullptr){
                mem_ptr->type = Type::REUSABLE;
            }
        }

        // Set region properties
        mem_ptr->base = base;
        mem_ptr->size = size;

        FREEBUF(reserved_mem_buf);
        mem_list.push_back(mem_ptr);
    }
}

/**
 * @brief Display overview of all reserved memory regions in formatted table
 *
 * Generates a comprehensive table showing all reserved memory regions with their
 * addresses, sizes, types, and summary statistics. Uses dynamic column width
 * calculation for optimal display formatting and sorts regions by base address.
 */
void Reserved::print_reserved_mem() {
    // Check if we have any reserved memory regions to display
    if (mem_list.empty()) {
        LOGE("No reserved memory regions found.\n");
        return;
    }

    // Initialize statistics counters
    uint64_t total_size = 0;
    uint64_t nomap_size = 0;
    uint64_t reusable_size = 0;
    uint64_t other_size = 0;

    // Define column widths for table formatting
    int col_num_width = 4;
    int col_name_width = 30;
    int col_base_width = 18;
    int col_range_width = 27;
    int col_size_width = 10;
    int col_status_width = 10;

    // Calculate statistics and determine optimal column widths
    for (const auto& region : mem_list) {
        total_size += region->size;

        // Categorize regions by type for statistics
        switch (region->type) {
            case Type::NO_MAP:
                nomap_size += region->size;
                break;
            case Type::REUSABLE:
                reusable_size += region->size;
                break;
            default:
                other_size += region->size;
                break;
        }

        // Adjust column width based on region name length
        col_name_width = std::max(col_name_width,static_cast<int>(region->name.length()) + 2);
    }

    // Sort regions by base address for logical display order
    std::sort(mem_list.begin(), mem_list.end(),
              [](const std::shared_ptr<reserved_mem>& a, const std::shared_ptr<reserved_mem>& b) {
                  return a->base < b->base;
              });

    // Calculate total table width
    int total_width = col_num_width + col_name_width + col_base_width +
                      col_range_width + col_size_width + col_status_width + 7;

    // Print table header with Unicode box drawing characters
    PRINT("Reserved Memory Regions Overview\n");
    PRINT("┌");
    for (int i = 0; i < col_num_width; ++i) PRINT("─");
    PRINT("┬");
    for (int i = 0; i < col_name_width; ++i) PRINT("─");
    PRINT("┬");
    for (int i = 0; i < col_base_width; ++i) PRINT("─");
    PRINT("┬");
    for (int i = 0; i < col_range_width; ++i) PRINT("─");
    PRINT("┬");
    for (int i = 0; i < col_size_width; ++i) PRINT("─");
    PRINT("┬");
    for (int i = 0; i < col_status_width; ++i) PRINT("─");
    PRINT("┐\n");

    // Print column headers
    PRINT("│%*s│%-*s│%-*s│%-*s│%-*s│%-*s│\n",
           col_num_width, "  #",
           col_name_width, "Region Name",
           col_base_width, "Base Address",
           col_range_width, "Physical Memory Range",
           col_size_width, "Total Size",
           col_status_width, "Status");

    // Print header separator
    PRINT("├");
    for (int i = 0; i < col_num_width; ++i) PRINT("─");
    PRINT("┼");
    for (int i = 0; i < col_name_width; ++i) PRINT("─");
    PRINT("┼");
    for (int i = 0; i < col_base_width; ++i) PRINT("─");
    PRINT("┼");
    for (int i = 0; i < col_range_width; ++i) PRINT("─");
    PRINT("┼");
    for (int i = 0; i < col_size_width; ++i) PRINT("─");
    PRINT("┼");
    for (int i = 0; i < col_status_width; ++i) PRINT("─");
    PRINT("┤\n");

    // Print each reserved memory region
    for (size_t i = 0; i < mem_list.size(); ++i) {
        const auto& region = mem_list[i];
        uint64_t base_addr = static_cast<uint64_t>(region->base);
        uint64_t region_size = static_cast<uint64_t>(region->size);
        uint64_t end_addr = base_addr + region_size;

        // Format size and status strings
        std::string size_str = csize(region_size);
        std::string status = get_region_status(region);

        // Format address range string
        char range_str[col_range_width + 1];
        snprintf(range_str, sizeof(range_str), "[0x%09" PRIx64 "~0x%09" PRIx64 "]",
                 base_addr, end_addr);

        // Print region information row
        PRINT("│%*zu │%-*s│0x%016" PRIx64 "│%-*s│%*s│%-*s│\n",
               col_num_width - 1, i + 1,
               col_name_width, region->name.c_str(),
               base_addr,
               col_range_width, range_str,
               col_size_width, size_str.c_str(),
               col_status_width, status.c_str());
    }

    // Print statistics separator
    PRINT("├");
    for (int i = 0; i < col_num_width; ++i) PRINT("─");
    PRINT("┴");
    for (int i = 0; i < col_name_width; ++i) PRINT("─");
    PRINT("┴");
    for (int i = 0; i < col_base_width; ++i) PRINT("─");
    PRINT("┴");
    for (int i = 0; i < col_range_width; ++i) PRINT("─");
    PRINT("┴");
    for (int i = 0; i < col_size_width; ++i) PRINT("─");
    PRINT("┴");
    for (int i = 0; i < col_status_width; ++i) PRINT("─");
    PRINT("┤\n");

    // Print summary statistics
    std::string stats = "Regions: " + std::to_string(mem_list.size()) +
                       " | Total: " + csize(total_size) +
                       " | Nomap: " + csize(nomap_size) +
                       " | Reuse: " + csize(reusable_size) +
                       " | Unknow: " + csize(other_size);
    PRINT("│ %-*s│\n", total_width - 3, stats.c_str());

    // Print table footer
    PRINT("└");
    for (int i = 0; i < total_width - 2; ++i) PRINT("─");
    PRINT("┘\n");
}

/**
 * @brief Get human-readable status string for reserved memory region type
 *
 * Converts the internal region type enumeration to a user-friendly string
 * representation for display purposes.
 *
 * @param mem_ptr Shared pointer to reserved memory region structure
 * @return String representation of the region type ("NO-MAP", "REUSABLE", or "Unknow")
 */
std::string Reserved::get_region_status(const std::shared_ptr<reserved_mem>& mem_ptr) {
    if (mem_ptr->type == Type::NO_MAP) {
        return "NO-MAP";        // Region is not mapped into kernel virtual address space
    } else if (mem_ptr->type == Type::REUSABLE) {
        return "REUSABLE";      // Region can be reused by kernel when not in use
    } else {
        return "Unknow";        // Region type is unknown or not specified
    }
}
#pragma GCC diagnostic pop
