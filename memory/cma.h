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

#ifndef CMA_DEFS_H_
#define CMA_DEFS_H_

#include "plugin.h"

/**
 * @brief Structure representing a CMA (Contiguous Memory Allocator) area
 *
 * Contains information about a single CMA area including its location,
 * size, allocation bitmap, and usage statistics.
 */
struct cma_mem {
    ulong addr;             ///< Kernel address of the CMA structure
    ulong base_pfn;         ///< Starting page frame number of the CMA area
    ulong count;            ///< Number of pages in this CMA area
    ulong bitmap;           ///< Address of the allocation bitmap
    int order_per_bit;      ///< Number of pages represented by each bit in bitmap
    std::string name;       ///< Name identifier of the CMA area
    ulong allocated_size;   ///< Currently allocated size in bytes
};

/**
 * @brief Structure defining column widths for CMA table display
 *
 * Used to calculate optimal column widths for formatted table output
 * based on the actual content length of each field.
 */
struct ColumnWidths {
    int index_width = 4;        ///< Width for index column
    int name_width = 25;        ///< Width for CMA area name column
    int addr_width = 18;        ///< Width for CMA address column
    int range_width = 27;       ///< Width for physical memory range column
    int size_width = 10;        ///< Width for total size column
    int used_width = 10;        ///< Width for used size column
    int percent_width = 8;      ///< Width for usage percentage column
    int order_width = 6;        ///< Width for order per bit column
};

/**
 * @brief Structure containing overall CMA system statistics
 *
 * Aggregated statistics across all CMA areas in the system,
 * including total size, usage, and efficiency metrics.
 */
struct CMAStatistics {
    size_t total_areas;             ///< Total number of CMA areas
    ulonglong total_size;           ///< Total size of all CMA areas combined
    ulonglong total_used;           ///< Total allocated size across all areas
    double overall_usage_percent;   ///< Overall usage percentage (0-100)
};

/**
 * @brief CMA (Contiguous Memory Allocator) analyzer plugin for crash utility
 *
 * This plugin provides comprehensive analysis of CMA areas in the Linux kernel,
 * including memory allocation status, usage statistics, and detailed page-level
 * information for debugging memory allocation issues.
 *
 * Main features:
 * - Display all CMA areas with allocation statistics
 * - Show detailed page allocation status for specific CMA areas
 * - Provide usage analytics and memory efficiency metrics
 * - Support both allocated and free page analysis
 * - Generate formatted reports with visual table layouts
 */
class Cma : public ParserPlugin {
private:
    // Core data storage
    std::vector<std::shared_ptr<cma_mem>> mem_list;     ///< List of all CMA areas in the system

    // Core functionality methods
    void parser_cma_areas();                            ///< Parse and collect CMA area information from kernel
    size_t get_cma_used_size(const std::shared_ptr<cma_mem>& cma);  ///< Calculate allocated size for a CMA area
    void print_cma_page_status(const std::string &addr_str);
    bool is_cma_page_allocated(const std::shared_ptr<cma_mem> &cma, ulong pfn);
    void print_cma_areas(); ///< Display overview of all CMA areas
    void print_cma_alloc_page_stack(const std::string& cma_name); ///< Print allocated CMA pages with pageowner stack traces for specific area
    // Statistics and analysis methods
    CMAStatistics calculate_cma_statistics();           ///< Calculate overall CMA system statistics
    ColumnWidths calculate_optimal_column_widths();     ///< Determine optimal column widths for table display

    // Display formatting methods
    void print_cma_table_header(const ColumnWidths& widths);       ///< Print formatted table header
    void print_table_separator_line(const ColumnWidths& widths, int total_width);  ///< Print table separator lines
    void print_cma_table_content(const ColumnWidths& widths);      ///< Print main table content
    std::string format_address_range(ulong base_pfn, ulong count); ///< Format physical address range string
    void print_cma_statistics(const ColumnWidths& widths);         ///< Print summary statistics

public:
    /**
     * @brief Default constructor
     */
    Cma();

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

    DEFINE_PLUGIN_INSTANCE(Cma)
};

#endif // CMA_DEFS_H_
