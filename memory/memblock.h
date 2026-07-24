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

#ifndef MEMBLOCK_DEFS_H_
#define MEMBLOCK_DEFS_H_

#include "plugin.h"
#include "devicetree/devicetree.h"

/**
 * @brief Enumeration for memblock region flags
 *
 * These flags define the characteristics and usage restrictions
 * of memory regions managed by the memblock allocator.
 */
enum memblock_flags {
    MEMBLOCK_NONE = 0,      ///< No special flags - normal memory region
    MEMBLOCK_HOTPLUG = 1,   ///< Memory region supports hotplug operations
    MEMBLOCK_MIRROR = 2,    ///< Memory region is mirrored for reliability
    MEMBLOCK_NOMAP = 4,     ///< Memory region should not be mapped by kernel
};

/**
 * @brief Structure representing a single memblock memory region
 *
 * Each memblock region represents a contiguous block of physical memory
 * with specific characteristics defined by flags. Regions can be either
 * available memory or reserved memory depending on the memblock type.
 */
struct memblock_region {
    ulong addr;                     ///< Kernel virtual address of this region structure
    physaddr_t base;                ///< Physical base address of the memory region
    physaddr_t size;                ///< Size of the memory region in bytes
    enum memblock_flags flags;      ///< Flags defining region characteristics
};

/**
 * @brief Structure representing a memblock type (memory or reserved)
 *
 * The memblock allocator manages two types of memory regions:
 * - Memory regions: Available physical memory for allocation
 * - Reserved regions: Memory reserved for specific purposes (kernel, initrd, etc.)
 */
struct memblock_type {
    ulong addr;                                             ///< Kernel address of the memblock_type structure
    unsigned long cnt;                                      ///< Current number of regions in this type
    unsigned long max;                                      ///< Maximum number of regions allowed
    uint64_t total_size;                                    ///< Total size of all regions in bytes
    std::vector<std::shared_ptr<memblock_region>> regions;  ///< Vector of memory regions
    std::string name;                                       ///< Human-readable name ("memory" or "reserved")
};

/**
 * @brief Main memblock allocator structure
 *
 * The memblock allocator is used during early boot before the buddy allocator
 * is initialized. It manages both available memory and reserved memory regions,
 * providing a simple interface for early memory allocation and reservation.
 */
struct memblock {
    ulong addr;                         ///< Kernel address of the memblock structure
    bool bottom_up;                     ///< Allocation direction flag (bottom-up vs top-down)
    unsigned long current_limit;        ///< Current allocation limit address
    struct memblock_type memory;        ///< Available memory regions
    struct memblock_type reserved;      ///< Reserved memory regions
};

/**
 * @brief Memblock analyzer plugin for crash utility
 *
 * This plugin provides comprehensive analysis of the Linux kernel's memblock
 * memory allocator, which is used during early boot phase before the buddy
 * allocator becomes available. It displays information about memory layout,
 * reserved regions, and allocation patterns.
 *
 * Key features:
 * - Display all memory and reserved regions with detailed information
 * - Show memory region flags and their meanings
 * - Provide memory layout visualization with addresses and sizes
 * - Support both 32-bit and 64-bit physical address configurations
 * - Generate formatted reports with visual table layouts
 */
class Memblock : public ParserPlugin {
private:
    // Core data storage
    std::shared_ptr<memblock> block;    ///< Main memblock structure containing all region information

    // Core parsing methods
    void parser_memblock();             ///< Parse main memblock structure from kernel memory
    void parser_memblock_type(ulong addr, memblock_type* type);  ///< Parse memblock_type (memory/reserved)
    std::vector<std::shared_ptr<memblock_region>> parser_memblock_region(ulong addr, int cnt);  ///< Parse region array

    // Display methods
    void print_memblock();              ///< Print complete memblock information with summary
    void print_memblock_type(memblock_type* type);  ///< Print detailed information for a memblock type

    // Utility methods
    std::string get_memblock_flags_name(enum memblock_flags flags);  ///< Convert flags enum to readable string

public:
    /**
     * @brief Default constructor
     */
    Memblock();

    /**
     * @brief Main command entry point - handles command line arguments
     */
    void cmd_main(void) override;

    /**
     * @brief Initialize kernel structure field offsets
     */
    void init_offset(void) override;

    /**
     * @brief Initialize command help and usage information
     */
    void init_command(void) override;

    DEFINE_PLUGIN_INSTANCE(Memblock)
};

#endif // MEMBLOCK_DEFS_H_
