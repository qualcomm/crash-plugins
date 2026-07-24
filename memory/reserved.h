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

#ifndef RESERVED_DEFS_H_
#define RESERVED_DEFS_H_

#include "plugin.h"
#include "devicetree/devicetree.h"

/**
 * @brief Enumeration for reserved memory region types
 *
 * Defines the different types of reserved memory regions based on
 * their usage characteristics and kernel accessibility.
 */
enum class Type{
    NO_MAP,     ///< Memory region is not mapped into kernel virtual address space
    REUSABLE,   ///< Memory region can be reused by the kernel when not in use
    UNKNOW,     ///< Memory region type is unknown or not specified
};

/**
 * @brief Structure representing a reserved memory region
 *
 * Contains information about a single reserved memory region including
 * its location, size, type, and kernel accessibility status.
 */
struct reserved_mem {
    ulong addr;         ///< Kernel address of the reserved_mem structure
    std::string name;   ///< Name identifier of the reserved memory region
    ulonglong base;     ///< Physical base address of the reserved region
    ulonglong size;     ///< Size of the reserved memory region in bytes
    bool status;        ///< Status flag indicating if region is active
    Type type;          ///< Type classification of the reserved memory region
};

/**
 * @brief Reserved memory analyzer plugin for crash utility
 *
 * This plugin provides comprehensive analysis of reserved memory regions
 * in the Linux kernel, including memory reservation status, usage types,
 * and detailed region information for debugging memory layout issues.
 *
 * Main features:
 * - Display all reserved memory regions with detailed information
 * - Show memory region types (no-map, reusable, unknown)
 * - Provide memory usage statistics and region classification
 * - Integrate with device tree information for region properties
 * - Generate formatted reports with visual table layouts
 */
class Reserved : public ParserPlugin {
private:
    // Core components
    std::shared_ptr<Devicetree> dts;                            ///< Device tree parser for region property analysis
    std::vector<std::shared_ptr<reserved_mem>> mem_list;        ///< List of all reserved memory regions in the system

    // Core functionality methods
    void parser_reserved_mem();                                 ///< Parse and collect reserved memory region information from kernel
    void print_reserved_mem();                                  ///< Display overview of all reserved memory regions
    std::string get_region_status(const std::shared_ptr<reserved_mem> &mem_ptr);  ///< Get human-readable status string for region type

public:
    /**
     * @brief Constructor - initializes device tree parser
     */
    Reserved();

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

    DEFINE_PLUGIN_INSTANCE(Reserved)
};


#endif // RESERVED_DEFS_H_
