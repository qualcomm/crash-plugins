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

#include "memblock.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(Memblock)
#endif

/**
 * @brief Main command entry point for memblock analysis
 *
 * Parses command line arguments and dispatches to appropriate handler functions.
 * Currently supports:
 * -a: Display all memblock regions including memory and reserved types
 */
void Memblock::cmd_main(void) {
    int c;

    // Check minimum argument count
    if (argcnt < 2) {
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }

    // Parse memblock information if not already done
    if (block.get() == nullptr) {
        parser_memblock();
    }

    // Parse command line options
    while ((c = getopt(argcnt, args, "a")) != EOF) {
        switch(c) {
            case 'a':
                print_memblock();
                break;
            default:
                argerrs++;
                break;
        }
    }

    // Handle argument errors
    if (argerrs) {
        cmd_usage(pc->curcmd, SYNOPSIS);
    }
}

/**
 * @brief Initialize kernel structure field offsets
 *
 * Sets up field offsets for memblock-related structures. These offsets are
 * essential for reading memblock data from kernel memory dumps and must be
 * initialized before any parsing operations.
 */
void Memblock::init_offset(void) {
    // Initialize memblock structure field offsets
    field_init(memblock, bottom_up);        // Allocation direction flag
    field_init(memblock, current_limit);    // Current allocation limit
    field_init(memblock, memory);           // Memory regions structure
    field_init(memblock, reserved);         // Reserved regions structure

    // Initialize memblock_type structure field offsets
    field_init(memblock_type, cnt);         // Number of regions
    field_init(memblock_type, max);         // Maximum regions allowed
    field_init(memblock_type, total_size);  // Total size of all regions
    field_init(memblock_type, regions);     // Regions array pointer
    field_init(memblock_type, name);        // Type name string

    // Initialize memblock_region structure field offsets
    field_init(memblock_region, base);      // Physical base address
    field_init(memblock_region, size);      // Region size
    field_init(memblock_region, flags);     // Region flags

    // Initialize structure sizes
    struct_init(memblock);
    struct_init(memblock_type);
    struct_init(memblock_region);
}

/**
 * @brief Initialize command help and usage information
 *
 * Sets up the command name, description, and detailed help text including
 * usage examples and expected output formats for the memblock plugin.
 */
void Memblock::init_command(void) {
    cmd_name = "memblock";
    help_str_list = {
        "memblock",                                      /* command name */
        "display memblock memory allocator information", /* short description */
        "[-a]\n"
        "  This command displays information about the memblock memory allocator.\n"
        "\n"
        "    -a              display all memblock regions including memory and reserved types\n",
        "\n",
        "EXAMPLES",
        "  Display memblock memory info:",
        "    %s> memblock -a",
        "    ┌──────────────────────────────────────────────────────────────────────────────┐",
        "    │  Type:                memory                                                 │",
        "    │  memblock_type:       0xffffffdd7acdc4f8                                     │",
        "    │  Total Size:          5.75GB                                                 │",
        "    │  Region Count:        14                                                     │",
        "    ├─────┬────────────────────┬────────────────────────────┬────────────┬─────────┤",
        "    │    #│memblock_region     │Physical Range              │Size        │Flags    │",
        "    ├─────┼────────────────────┼────────────────────────────┼────────────┼─────────┤",
        "    │    0│0xffffffdd7b2b3e58  │[0x080e00000~0x081800000]   │        10MB│NONE     │",
        "    │    1│0xffffffdd7b2b3e70  │[0x081cf5000~0x081cff000]   │        40KB│NONE     │",
        "    │    2│0xffffffdd7b2b3e88  │[0x081f20000~0x0824a0000]   │      5.50MB│NONE     │",
        "    │    3│0xffffffdd7b2b3ea0  │[0x082800000~0x099280000]   │    362.50MB│NONE     │",
        "    │    4│0xffffffdd7b2b3eb8  │[0x09ea9c000~0x09eb00000]   │       400KB│NONE     │",
        "    │    5│0xffffffdd7b2b3ed0  │[0x09f300000~0x0a6400000]   │       113MB│NONE     │",
        "    │    6│0xffffffdd7b2b3ee8  │[0x0a7000000~0x0e0600000]   │       918MB│NONE     │",
        "    │    7│0xffffffdd7b2b3f00  │[0x0e0a00000~0x0e8900000]   │       127MB│NONE     │",
        "    │    8│0xffffffdd7b2b3f18  │[0x0ea700000~0x0fc800000]   │       289MB│NONE     │",
        "    │    9│0xffffffdd7b2b3f30  │[0x0fca00000~0x100000000]   │        54MB│NONE     │",
        "    │   10│0xffffffdd7b2b3f48  │[0x880000000~0x8afbff000]   │    764.00MB│NONE     │",
        "    │   11│0xffffffdd7b2b3f60  │[0x8b0000000~0x8ba700000]   │       167MB│NONE     │",
        "    │   12│0xffffffdd7b2b3f78  │[0x8bf800000~0x97ffff000]   │      3.01GB│NONE     │",
        "    │   13│0xffffffdd7b2b3f90  │[0x97ffff000~0x980000000]   │         4KB│NO-MAP   │",
        "    └─────┴────────────────────┴────────────────────────────┴────────────┴─────────┘",
        "    ",
        "    ┌──────────────────────────────────────────────────────────────────────────────┐",
        "    │  Type:                reserved                                               │",
        "    │  memblock_type:       0xffffffdd7acdc520                                     │",
        "    │  Total Size:          429.90MB                                               │",
        "    │  Region Count:        55                                                     │",
        "    ├─────┬────────────────────┬────────────────────────────┬────────────┬─────────┤",
        "    │    #│memblock_region     │Physical Range              │Size        │Flags    │",
        "    ├─────┼────────────────────┼────────────────────────────┼────────────┼─────────┤",
        "    │    0│0xffffffdd7b2b9e58  │[0x082800000~0x084800000]   │        32MB│NONE     │",
        "    │    1│0xffffffdd7b2b9e70  │[0x0a8010000~0x0ab257000]   │     50.28MB│NONE     │",
        "    │    2│0xffffffdd7b2b9e88  │[0x0ab25a000~0x0ab260000]   │        24KB│NONE     │",
        "    │    3│0xffffffdd7b2b9ea0  │[0x0affffb80~0x0b0000000]   │      1.12KB│NONE     │",
        "    │    4│0xffffffdd7b2b9eb8  │[0x0b729f000~0x0b7327cbf]   │    547.19KB│NONE     │",
        "    │    5│0xffffffdd7b2b9ed0  │[0x0e3940000~0x0e6440000]   │        43MB│NONE     │",
        "    │    6│0xffffffdd7b2b9ee8  │[0x0f6800000~0x0fc800000]   │        96MB│NONE     │",
        "    │    7│0xffffffdd7b2b9f00  │[0x0fd400000~0x0ffc00000]   │        40MB│NONE     │",
        "    │    8│0xffffffdd7b2b9f18  │[0x9757d0000~0x97d800000]   │    128.19MB│NONE     │",
        "    │   ..│...                 │...                         │         ...│...      │",
        "    │   54│0xffffffdd7b2ba368  │[0x97d8c5a58~0x980000000]   │     39.23MB│NONE     │",
        "    └─────┴────────────────────┴────────────────────────────┴────────────┴─────────┘",
        "    ",
        "    Summary:",
        "    ┌──────────────────────────────────────────────────────────────────────────────┐",
        "    │  Total Memory Regions:    69                                                 │",
        "    │  Memory Size:             5.75GB                                             │",
        "    │  Reserved Size:           429.90MB                                           │",
        "    │  Bottom Up:               No                                                 │",
        "    └──────────────────────────────────────────────────────────────────────────────┘",
        "\n",
    };
}

/**
 * @brief Default constructor
 *
 * Initializes the Memblock plugin with default settings.
 */
Memblock::Memblock() {}

/**
 * @brief Parse and collect memblock information from kernel memory
 *
 * Reads the main memblock structure from kernel memory and extracts
 * information about both memory and reserved regions. This function
 * handles both 32-bit and 64-bit physical address configurations.
 */
void Memblock::parser_memblock() {
    // Check if memblock symbol exists in kernel
    if (!csymbol_exists("memblock")) {
        LOGE("memblock doesn't exist in this kernel!\n");
        return;
    }

    // Get memblock structure address
    ulong memblock_addr = csymbol_value("memblock");
    if (!is_kvaddr(memblock_addr)) {
        LOGE("memblock address is invalid!\n");
        return;
    }

    // Read memblock structure from kernel memory
    void *buf = read_struct(memblock_addr, "memblock");
    if (!buf) {
        LOGE("Failed to read memblock structure at address %lx\n", memblock_addr);
        return;
    }

    // Create and initialize memblock structure
    block = std::make_shared<memblock>();
    block->addr = memblock_addr;
    block->bottom_up = BOOL(buf + field_offset(memblock, bottom_up));
    block->current_limit = ULONG(buf + field_offset(memblock, current_limit));

    // Calculate offsets for memory and reserved types
    ulong memory_offset = field_offset(memblock, memory);
    ulong reserved_offset = field_offset(memblock, reserved);
    FREEBUF(buf);

    // Parse both memory and reserved memblock types
    parser_memblock_type(memblock_addr + memory_offset, &block->memory);
    parser_memblock_type(memblock_addr + reserved_offset, &block->reserved);
}

/**
 * @brief Parse a memblock_type structure (memory or reserved)
 *
 * Reads a memblock_type structure from kernel memory and extracts information
 * about the regions it contains. Handles both memory and reserved types.
 *
 * @param addr Address of the memblock_type structure in kernel memory
 * @param type Pointer to memblock_type structure to populate
 */
void Memblock::parser_memblock_type(ulong addr, memblock_type* type) {
    if (!type) {
        LOGE("Invalid memblock_type pointer!\n");
        return;
    }

    // Read memblock_type structure from kernel memory
    void *buf = read_struct(addr, "memblock_type");
    if (!buf) {
        LOGE("Failed to read memblock_type structure at address %lx\n", addr);
        return;
    }

    // Extract basic information
    type->addr = addr;
    type->cnt = ULONG(buf + field_offset(memblock_type, cnt));
    type->max = ULONG(buf + field_offset(memblock_type, max));

    // Handle both 32-bit and 64-bit physical address configurations
    if (get_config_val("CONFIG_PHYS_ADDR_T_64BIT") == "y") {
        type->total_size = (ulong)ULONGLONG(buf + field_offset(memblock_type, total_size));
    } else {
        type->total_size = ULONG(buf + field_offset(memblock_type, total_size));
    }

    // Read type name and regions array address
    type->name = read_cstring(ULONG(buf + field_offset(memblock_type, name)), 64, "memblock_type_name");
    ulong regions = ULONG(buf + field_offset(memblock_type, regions));
    FREEBUF(buf);

    // Parse all regions for this type
    type->regions = parser_memblock_region(regions, type->cnt);
}

/**
 * @brief Parse an array of memblock_region structures
 *
 * Reads an array of memblock_region structures from kernel memory and
 * creates corresponding region objects. Handles both 32-bit and 64-bit
 * physical address configurations for optimal compatibility.
 *
 * @param addr Address of the regions array in kernel memory
 * @param cnt Number of regions to parse
 * @return Vector of parsed memblock_region structures
 */
std::vector<std::shared_ptr<memblock_region>> Memblock::parser_memblock_region(ulong addr, int cnt) {
    std::vector<std::shared_ptr<memblock_region>> res;
    res.reserve(cnt);

    // Check for 64-bit physical address configuration
    bool is_64bit = (get_config_val("CONFIG_PHYS_ADDR_T_64BIT") == "y");

    // Pre-calculate structure offsets for efficiency
    size_t region_size = struct_size(memblock_region);
    size_t base_offset = field_offset(memblock_region, base);
    size_t size_offset = field_offset(memblock_region, size);
    size_t flags_offset = field_offset(memblock_region, flags);

    // Parse each region in the array
    for (int i = 0; i < cnt; ++i) {
        ulong reg_addr = addr + i * region_size;
        void *buf = read_struct(reg_addr, "memblock_region");
        if (!buf) {
            LOGE("Failed to read memblock_region structure at address %lx\n", reg_addr);
            return res;
        }

        // Create new region structure
        auto region = std::make_shared<memblock_region>();
        region->addr = reg_addr;

        // Handle different physical address sizes
        if (is_64bit) {
            region->base = (ulong)ULONGLONG(buf + base_offset);
            region->size = (ulong)ULONGLONG(buf + size_offset);
        } else {
            region->base = ULONG(buf + base_offset);
            region->size = ULONG(buf + size_offset);
        }
        region->flags = (enum memblock_flags)INT(buf + flags_offset);

        FREEBUF(buf);
        res.push_back(std::move(region));
    }

    return res;
}

/**
 * @brief Display complete memblock information with summary
 *
 * Prints detailed information about both memory and reserved memblock types,
 * followed by a comprehensive summary of the entire memblock allocator state.
 */
void Memblock::print_memblock() {
    if (block.get() == nullptr) {
        LOGE("Parser memblock fail!\n");
        return;
    }

    // Print memory regions
    print_memblock_type(&block->memory);
    PRINT("\n");

    // Print reserved regions
    print_memblock_type(&block->reserved);

    // Print comprehensive summary
    PRINT("\n");
    PRINT("Summary:\n");
    PRINT("┌──────────────────────────────────────────────────────────────────────────────┐\n");
    PRINT("│  Total Memory Regions:    %-21lu                              │\n",
            block->memory.cnt + block->reserved.cnt);
    PRINT("│  Memory Size:             %-21s                              │\n",
            csize(block->memory.total_size).c_str());
    PRINT("│  Reserved Size:           %-21s                              │\n",
            csize(block->reserved.total_size).c_str());
    PRINT("│  Bottom Up:               %-21s                              │\n",
            block->bottom_up ? "Yes" : "No");
    PRINT("└──────────────────────────────────────────────────────────────────────────────┘\n");
    PRINT("\n");
}

/**
 * @brief Display detailed information for a memblock type
 *
 * Prints a formatted table showing all regions within a memblock type
 * (either memory or reserved), including addresses, ranges, sizes, and flags.
 * Uses Unicode box drawing characters for professional table formatting.
 *
 * @param type Pointer to memblock_type structure to display
 */
void Memblock::print_memblock_type(memblock_type* type) {
    if (!type || type->regions.empty()) {
        LOGE("No regions found.\n");
        return;
    }

    // Define column widths for consistent formatting
    const int col_num_width = 5;
    const int col_addr_width = 20;
    const int col_range_width = 28;
    const int col_size_width = 12;
    const int col_flags_width = 9;
    const int info_width = 78;

    // Print header section with type information
    PRINT("┌");
    for (int i = 0; i < info_width; ++i) PRINT("─");
    PRINT("┐\n");

    PRINT("│  Type:                %-55s│\n", type->name.c_str());
    PRINT("│  memblock_type:       0x%016lx%-37s│\n", type->addr, "");
    PRINT("│  Total Size:          %-55s│\n", csize(type->total_size).c_str());
    PRINT("│  Region Count:        %-55lu│\n", type->cnt);

    // Print table header separator
    PRINT("├");
    for (int i = 0; i < col_num_width; ++i) PRINT("─");
    PRINT("┬");
    for (int i = 0; i < col_addr_width; ++i) PRINT("─");
    PRINT("┬");
    for (int i = 0; i < col_range_width; ++i) PRINT("─");
    PRINT("┬");
    for (int i = 0; i < col_size_width; ++i) PRINT("─");
    PRINT("┬");
    for (int i = 0; i < col_flags_width; ++i) PRINT("─");
    PRINT("┤\n");

    // Print column headers
    PRINT("│%*s│%-*s│%-*s│%-*s│%-*s│\n",
            col_num_width, "    #",
            col_addr_width, "memblock_region",
            col_range_width, "Physical Range",
            col_size_width, "Size",
            col_flags_width, "Flags");

    // Print header-content separator
    PRINT("├");
    for (int i = 0; i < col_num_width; ++i) PRINT("─");
    PRINT("┼");
    for (int i = 0; i < col_addr_width; ++i) PRINT("─");
    PRINT("┼");
    for (int i = 0; i < col_range_width; ++i) PRINT("─");
    PRINT("┼");
    for (int i = 0; i < col_size_width; ++i) PRINT("─");
    PRINT("┼");
    for (int i = 0; i < col_flags_width; ++i) PRINT("─");
    PRINT("┤\n");

    // Print each region's information
    for (size_t i = 0; i < type->cnt; ++i) {
        auto& region = type->regions[i];

        // Format physical address range
        uint64_t base_addr = static_cast<uint64_t>(region->base);
        uint64_t end_addr = static_cast<uint64_t>(region->base + region->size);

        char range_str[64];
        snprintf(range_str, sizeof(range_str), "[0x%09" PRIx64 "~0x%09" PRIx64 "]",
                 base_addr, end_addr);

        // Format region address and size
        char addr_str[32];
        snprintf(addr_str, sizeof(addr_str), "0x%016lx", region->addr);
        std::string size_str = csize(region->size);
        std::string flags_name = get_memblock_flags_name(region->flags);

        // Print region row
        PRINT("│%*zu│%-*s│%-*s│%*s│%-*s│\n",
                col_num_width, i,
                col_addr_width, addr_str,
                col_range_width, range_str,
                col_size_width, size_str.c_str(),
                col_flags_width, flags_name.c_str());
    }

    // Print table footer
    PRINT("└");
    for (int i = 0; i < col_num_width; ++i) PRINT("─");
    PRINT("┴");
    for (int i = 0; i < col_addr_width; ++i) PRINT("─");
    PRINT("┴");
    for (int i = 0; i < col_range_width; ++i) PRINT("─");
    PRINT("┴");
    for (int i = 0; i < col_size_width; ++i) PRINT("─");
    PRINT("┴");
    for (int i = 0; i < col_flags_width; ++i) PRINT("─");
    PRINT("┘\n");
}

/**
 * @brief Convert memblock flags enum to human-readable string
 *
 * Translates memblock region flags into descriptive strings for display
 * purposes. Handles all defined memblock flag types including combinations.
 *
 * @param flags Memblock flags enum value
 * @return Human-readable string representation of the flags
 */
std::string Memblock::get_memblock_flags_name(enum memblock_flags flags) {
    switch (flags) {
        case MEMBLOCK_NONE:    return "NONE";
        case MEMBLOCK_HOTPLUG: return "HOTPLUG";
        case MEMBLOCK_MIRROR:  return "MIRROR";
        case MEMBLOCK_NOMAP:   return "NO-MAP";
        default:               return "UNKNOWN";
    }
}

#pragma GCC diagnostic pop
