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

#ifndef PAGEOWNER_H_
#define PAGEOWNER_H_

#include "plugin.h"

/**
 * @brief Per-process allocation statistics for pageowner
 *
 * Tracks memory allocation patterns for individual processes,
 * including total allocations and per-stack breakdowns.
 */
struct PageownerPidStatistics {
    size_t allocation_count = 0;                                    ///< Total number of allocations by this PID
    size_t total_memory = 0;                                        ///< Total memory allocated by this PID
    std::unordered_map<unsigned int, size_t> stack_memory;          ///< Stack handle -> memory allocated by this stack
};

/**
 * @brief Per-call-stack allocation statistics for pageowner
 *
 * Tracks memory allocation patterns for specific call stacks,
 * enabling identification of memory-intensive code paths.
 */
struct PageownerStackStatistics {
    unsigned int handle = 0;                                        ///< Stack handle ID
    size_t total_allocations = 0;                                   ///< Total allocations from this call stack
    size_t total_memory = 0;                                        ///< Total memory allocated from this call stack
    std::unordered_map<size_t, PageownerPidStatistics> pid_stats;   ///< Per-PID statistics within this stack
};

/**
 * @class Pageowner
 * @brief Plugin for analyzing kernel page owner information
 *
 * This plugin provides comprehensive functionality to analyze page allocation
 * and deallocation tracking in Linux kernel crash dumps. It supports:
 * - Display of allocation/deallocation stacks
 * - Memory usage statistics by process and call stack
 * - Address type auto-detection (PFN, physical, virtual, page structure)
 * - Advanced filtering and analysis capabilities
 */
class Pageowner : public ParserPlugin {
public:
    // Constructor and plugin interface methods
    Pageowner();
    void cmd_main(void) override;
    void init_offset(void) override;
    void init_command(void) override;
    DEFINE_PLUGIN_INSTANCE(Pageowner)

private:
    enum AddressType {
        ADDR_PFN,           ///< Page Frame Number
        ADDR_PHYSICAL,      ///< Physical memory address
        ADDR_PAGE,          ///< Kernel page structure address
        ADDR_VIRTUAL,       ///< Virtual memory address
        ADDR_UNKNOWN        ///< Unknown or invalid address type
    };

    /**
     * @brief Input type flags for address interpretation
     */
    static const int INPUT_PFN    = 0x0001;    ///< Input is a Page Frame Number
    static const int INPUT_PHYS   = 0x0002;    ///< Input is a Physical address
    static const int INPUT_PAGE   = 0x0004;    ///< Input is a Page structure address
    static const int INPUT_VADDR  = 0x0008;    ///< Input is a Virtual address

    /**
     * @brief Default display limits for statistics output
     */
    const size_t DEFAULT_TOP_COUNT    = 20;     ///< Default number of top entries to display
    const size_t DEFAULT_DETAIL_COUNT = 20;     ///< Default number of detailed entries to display

    /// Main data cache: PFN -> page_owner mapping
    std::unordered_map<size_t, std::shared_ptr<page_owner>> owner_map;

    /// Memory usage tracking for internal structures
    std::set<ulong> page_owner_page_list;       ///< Pages used by page_owner structures
    std::set<ulong> stack_record_page_list;     ///< Pages used by stack_record structures

    /// Statistics collections
    std::unordered_map<unsigned int, PageownerStackStatistics> stack_statistics;    ///< Stack-based allocation statistics
    std::unordered_map<size_t, PageownerPidStatistics> global_pid_statistics;       ///< Global per-process statistics

    /**
     * @brief Parse all page owner information from crash dump
     *
     * This is the main parsing function that iterates through all physical
     * page frames and extracts page owner information. It's performance-critical
     * and may take significant time for systems with large memory.
     */
    void parser_pageowners();

    /**
     * @brief Collect allocation statistics for stack and PID analysis
     * @param owner_ptr page_owner structure containing allocation information
     */
    void collect_stack_statistics(std::shared_ptr<page_owner>& owner_ptr);

    /**
     * @brief Detect the type of a given address using various heuristics
     * @param addr Address to analyze
     * @return AddressType enum value indicating the detected type
     */
    AddressType detect_address_type(ulonglong addr);

    /**
     * @brief Check if address is a kernel page structure address
     * @param addr Address to check
     * @return true if address is a valid page structure
     */
    bool is_page_address(ulonglong addr);

    /**
     * @brief Check if address is a physical memory address
     * @param addr Address to check
     * @return true if address is in valid physical memory range
     */
    bool is_physical_address(ulonglong addr);

    /**
     * @brief Print page owner info for specific address with type flags
     * @param addr Address string in hexadecimal format
     * @param flags Input type flags (INPUT_PFN, INPUT_PHYS, etc.)
     */
    void print_page_owner(const std::string& addr, int flags);

    /**
     * @brief Auto-detect address type and print page owner info
     * @param addr_str Address string in hexadecimal format
     */
    void print_page_owner(const std::string& addr_str);

    /**
     * @brief Print all allocated or freed pages
     * @param show_freed true to show freed pages, false for allocated pages
     */
    void print_all_pages(bool show_freed);

    /**
     * @brief Print single page owner entry in list format
     * @param owner_ptr page_owner structure to display
     * @param is_free true if showing free operation, false for allocation
     * @param entry_num Current entry number in the list
     * @param total_entries Total number of entries in the list
     */
    void print_page_owner(std::shared_ptr<page_owner> owner_ptr, bool is_free,
                         size_t entry_num, size_t total_entries);

    /**
     * @brief Print detailed page owner information
     * @param owner_ptr page_owner structure to display
     * @param is_free true if showing free operation, false for allocation
     */
    void print_page_owner(std::shared_ptr<page_owner> owner_ptr, bool is_free);

    /**
     * @brief Print allocation statistics sorted by call stack memory usage
     */
    void print_alloc_mem_by_stack();

    /**
     * @brief Print global allocation statistics sorted by process memory usage
     */
    void print_alloc_mem_by_pid();

    /**
     * @brief Print detailed memory allocation analysis for a specific process
     * @param pid Process ID to analyze
     */
    void print_pid_details(size_t pid);

    /**
     * @brief Print detailed call stack information for a specific handle
     * @param handle Stack handle ID to display
     */
    void print_stack_info(unsigned int handle);

    /**
     * @brief Helper function to print formatted stack statistics
     * @param sorted_stacks Vector of sorted stack statistics
     * @param total_allocations Total allocation count across all stacks
     * @param total_memory Total memory usage across all stacks
     */
    void print_stack_statistics_info(
        const std::vector<std::pair<unsigned int, PageownerStackStatistics>>& sorted_stacks,
        size_t total_allocations,
        size_t total_memory);
};

#endif // PAGEOWNER_H_
