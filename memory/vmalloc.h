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

#ifndef VMALLOC_DEFS_H_
#define VMALLOC_DEFS_H_

#include "plugin.h"
#include "devicetree/devicetree.h"

/**
 * @struct vm_struct
 * @brief Represents a virtual memory structure in the kernel
 *
 * This structure contains information about a single vmalloc allocation,
 * including its virtual address, size, flags, and associated physical pages.
 */
struct vm_struct {
    ulong addr;                          // Address of vm_struct in kernel memory
    ulong kaddr;                         // Kernel virtual address of the allocation
    ulong size;                          // Size of the allocation in bytes
    std::string flags;                   // Allocation flags (ioremap, vmalloc, vmap, etc.)
    std::vector<ulong> page_list;        // List of physical page addresses
    int nr_pages;                        // Number of pages in the allocation
    ulonglong phys_addr;                 // Physical address (for ioremap)
    std::string caller;                  // Function that allocated this memory
};

/**
 * @struct vmap_area
 * @brief Represents a virtual memory area in the vmalloc address space
 *
 * This structure represents a contiguous virtual address range that may
 * contain one or more vm_struct allocations.
 */
struct vmap_area {
    ulong addr;                          // Address of vmap_area in kernel memory
    ulong va_start;                      // Start of virtual address range
    ulong va_end;                        // End of virtual address range
    std::vector<std::shared_ptr<vm_struct>> vm_list;  // List of vm_struct in this area
};

/**
 * @struct vmalloc_info
 * @brief Summary information for vmalloc statistics
 *
 * Used to aggregate vmalloc information by caller function or allocation type.
 */
struct vmalloc_info {
    std::string func;                    // Function name or allocation type
    ulong virt_size;                     // Total virtual memory size
    ulong page_cnt;                      // Total number of physical pages
};

/**
 * @class Vmalloc
 * @brief Plugin for analyzing kernel vmalloc memory allocations
 *
 * This plugin parses and displays information about vmalloc allocations in the kernel,
 * including vmap areas, vm_struct details, and memory usage statistics.
 */
class Vmalloc : public ParserPlugin {
private:
    // VM allocation flags (from Linux kernel)
    static const int VM_IOREMAP  = 0x00000001;  // ioremap() and friends
    static const int VM_ALLOC    = 0x00000002;  // vmalloc()
    static const int VM_MAP      = 0x00000004;  // vmap()
    static const int VM_USERMAP  = 0x00000008;  // suitable for remap_vmalloc_range
    static const int VM_VPAGES   = 0x00000010;  // buffer for pages was vmalloc'ed
    static const int VM_UNLIST   = 0x00000020;  // vm_struct is not listed in vmlist

    std::vector<std::shared_ptr<vmap_area>> area_list;  // Cached list of vmap areas

    /**
     * @brief Parse vmap_nodes structure (newer kernel versions)
     *
     * Parses the vmap_nodes structure used in newer kernels for managing
     * vmalloc address space. Falls back to vmap_area_list if not available.
     */
    void parser_vmap_nodes();

    /**
     * @brief Parse a single vmap_area structure
     * @param addr Kernel address of the vmap_area structure
     *
     * Reads and parses a vmap_area structure and all associated vm_struct entries.
     */
    void parser_vmap_area(ulong addr);

    /**
     * @brief Parse all vmap areas in the system
     *
     * Entry point for parsing vmalloc information. Automatically detects
     * whether to use vmap_nodes or vmap_area_list.
     */
    void parser_vmap_area_list();

    /**
     * @brief Print detailed information for all vmap areas
     *
     * Displays comprehensive information including vmap_area, vm_struct,
     * and individual page details.
     */
    void print_vmap_area_list();

    /**
     * @brief Print summary of all vmap areas
     *
     * Displays a concise list of all vmap areas with their address ranges.
     */
    void print_vmap_area();

    /**
     * @brief Print all vm_struct information
     *
     * Displays detailed information about all vm_struct allocations.
     */
    void print_vm_struct();

    /**
     * @brief Print summary statistics
     *
     * Displays aggregated statistics by caller function and allocation type.
     */
    void print_summary_info();

    /**
     * @brief Print summary statistics grouped by caller function
     */
    void print_summary_caller();

    /**
     * @brief Print summary statistics grouped by allocation type
     */
    void print_summary_type();

    /**
     * @brief Print page information for a specific caller function
     * @param func Function name to filter by (partial match supported)
     */
    void print_vm_info_caller(std::string func);

    /**
     * @brief Print page information for a specific allocation type
     * @param type Allocation type to filter by (partial match supported)
     */
    void print_vm_info_type(std::string type);

public:
    /**
     * @brief Constructor
     */
    Vmalloc();

    /**
     * @brief Main command handler
     *
     * Processes command-line arguments and dispatches to appropriate functions.
     */
    void cmd_main(void) override;

    /**
     * @brief Initialize structure field offsets
     *
     * Initializes all kernel structure offsets needed for parsing vmalloc data.
     */
    void init_offset(void) override;

    /**
     * @brief Initialize command metadata
     *
     * Sets up command name, help text, and usage examples.
     */
    void init_command(void) override;

    DEFINE_PLUGIN_INSTANCE(Vmalloc)
};


#endif // VMALLOC_DEFS_H_
