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

#ifndef BUDDY_DEFS_H_
#define BUDDY_DEFS_H_

#include "plugin.h"

/**
 * @enum zone_watermarks
 * @brief Zone watermark levels for memory allocation
 *
 * Watermarks control when the kernel starts reclaiming memory:
 * - WMARK_MIN: Minimum threshold, direct reclaim starts
 * - WMARK_LOW: Low threshold, kswapd wakes up
 * - WMARK_HIGH: High threshold, kswapd goes back to sleep
 */
enum zone_watermarks {
    WMARK_MIN,   ///< Minimum watermark level
    WMARK_LOW,   ///< Low watermark level
    WMARK_HIGH,  ///< High watermark level
    NR_WMARK     ///< Number of watermark levels
};

/**
 * @struct free_area
 * @brief Free area structure for a specific order in buddy allocator
 *
 * Represents free pages of a specific order (size) in the buddy system.
 * Pages are organized by migration type to reduce fragmentation.
 */
struct free_area {
    ulong addr;                                      ///< Kernel address of free_area structure
    std::vector<std::vector<ulong>> free_list;      ///< Free page lists by migration type
    unsigned long nr_free;                           ///< Total number of free pages in this order
};

/**
 * @struct zone
 * @brief Memory zone structure
 *
 * Represents a memory zone (DMA, DMA32, Normal, HighMem, Movable).
 * Each zone has its own buddy allocator and watermarks.
 */
struct zone {
    ulong addr;                                      ///< Kernel address of zone structure
    unsigned long _watermark[3];                     ///< Watermark levels (MIN, LOW, HIGH)
    unsigned long watermark_boost;                   ///< Temporary watermark boost
    long lowmem_reserve[3];                          ///< Reserved pages for lower zones
    unsigned long start_pfn;                         ///< Starting page frame number
    unsigned long managed_pages;                     ///< Pages managed by buddy allocator
    unsigned long spanned_pages;                     ///< Total pages spanned by zone
    unsigned long present_pages;                     ///< Pages present in zone
    unsigned long cma_pages;                         ///< Contiguous Memory Allocator pages
    std::string name;                                ///< Zone name (e.g., "DMA32", "Normal")
    std::vector<std::shared_ptr<free_area>> free_areas;  ///< Free areas by order
    std::vector<ulong> vm_stat;                      ///< VM statistics for this zone
};

/**
 * @struct pglist_data
 * @brief NUMA node structure
 *
 * Represents a NUMA node containing one or more memory zones.
 * Each node has its own set of zones and statistics.
 */
struct pglist_data {
    ulong addr;                                      ///< Kernel address of pglist_data structure
    std::vector<std::shared_ptr<zone>> zone_list;   ///< List of zones in this node
    unsigned long start_pfn;                         ///< Starting page frame number
    unsigned long present_pages;                     ///< Pages present in node
    unsigned long spanned_pages;                     ///< Total pages spanned by node
    int id;                                          ///< Node ID
    unsigned long totalreserve_pages;                ///< Total reserved pages
    std::vector<ulong> vm_stat;                      ///< VM statistics for this node
};

/**
 * @class Buddy
 * @brief Buddy allocator information parser and display
 *
 * Provides functionality for:
 * - Parsing buddy allocator structures from kernel memory
 * - Displaying free page information by order and migration type
 * - Showing memory node and zone configuration
 * - Analyzing memory fragmentation
 *
 * The buddy allocator is the kernel's primary physical page allocator,
 * managing free pages in power-of-2 sized blocks.
 */
class Buddy : public ParserPlugin {
private:
    std::vector<std::shared_ptr<pglist_data>> node_list;  ///< List of NUMA nodes
    std::vector<std::string> migratetype_names;            ///< Migration type names
    int min_free_kbytes;                                   ///< Minimum free memory (KB)
    int user_min_free_kbytes;                              ///< User-configured minimum (KB)
    int watermark_scale_factor;                            ///< Watermark scaling factor

    /**
     * @brief Parse all buddy allocator information
     *
     * Reads and parses all NUMA nodes, zones, and free areas from kernel memory.
     */
    void parser_buddy_info();

    /**
     * @brief Parse NUMA node information
     *
     * @param addr Address of pglist_data structure
     * @return Shared pointer to parsed node structure
     */
    std::shared_ptr<pglist_data> parser_node_info(ulong addr);

    /**
     * @brief Parse memory zone information
     *
     * @param addr Address of zone structure
     * @return Shared pointer to parsed zone structure
     */
    std::shared_ptr<zone> parser_zone_info(ulong addr);

    /**
     * @brief Parse free areas for all orders
     *
     * @param addr Address of first free_area structure
     * @return Vector of parsed free_area structures
     */
    std::vector<std::shared_ptr<free_area>> parser_free_area(ulong addr);

    /**
     * @brief Parse free page lists for all migration types
     *
     * @param addr Address of free_list array
     * @return 2D vector of page addresses [migration_type][page_index]
     */
    std::vector<std::vector<ulong>> parser_free_list(ulong addr);

    /**
     * @brief Get migration type names from kernel
     *
     * Reads the migratetype_names array to get human-readable names
     * for each migration type (Unmovable, Movable, Reclaimable, etc.).
     */
    void get_migratetype_names();

    /**
     * @brief Print buddy allocator summary
     *
     * Displays free pages organized by order and migration type for all zones.
     */
    void print_buddy_info();

    /**
     * @brief Print memory node configuration
     *
     * Displays detailed information about all NUMA nodes and their zones.
     */
    void print_memory_node();

    /**
     * @brief Print detailed zone information
     *
     * @param addr Zone address as hexadecimal string
     */
    void print_memory_zone(std::string addr);

    /**
     * @brief Print node information
     *
     * @param node_ptr Pointer to node structure
     */
    void print_node_info(std::shared_ptr<pglist_data> node_ptr);

    /**
     * @brief Print zone information
     *
     * @param zone_ptr Pointer to zone structure
     */
    void print_zone_info(std::shared_ptr<zone> zone_ptr);

public:
    /**
     * @brief Constructor
     *
     * Initializes the Buddy allocator parser.
     */
    Buddy();

    /**
     * @brief Main command handler
     *
     * Processes command-line arguments and dispatches to appropriate handlers.
     */
    void cmd_main(void) override;

    /**
     * @brief Initialize structure field offsets
     *
     * Initializes offsets for buddy allocator kernel structures.
     */
    void init_offset(void) override;

    /**
     * @brief Initialize command metadata
     *
     * Sets up command name, description, and help text.
     */
    void init_command(void) override;

    DEFINE_PLUGIN_INSTANCE(Buddy)
};

#endif // BUDDY_DEFS_H_
