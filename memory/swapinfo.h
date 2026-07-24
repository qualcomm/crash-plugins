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

#ifndef SWAP_INFO_DEFS_H_
#define SWAP_INFO_DEFS_H_

#include "plugin.h"
#include "memory/zraminfo.h"

/**
 * @struct swap_extent
 * @brief Swap extent structure for mapping swap pages to disk blocks
 *
 * Represents a contiguous range of swap pages and their corresponding
 * disk block locations. Swap extents are organized in a red-black tree
 * for efficient lookup.
 */
struct swap_extent {
    struct rb_node rb_node;          ///< Red-black tree node
    unsigned long start_page;        ///< Starting swap page number
    unsigned long nr_pages;          ///< Number of pages in this extent
    unsigned long long start_block;  ///< Starting disk block number
};

/**
 * @struct swap_info
 * @brief Swap device information structure
 *
 * Contains metadata about a swap device including size, usage,
 * and associated file/device information.
 */
struct swap_info {
    ulong addr;                ///< Kernel swap_info_struct address
    ulong swap_space;          ///< Address space structure pointer
    unsigned int pages;        ///< Total number of swap pages
    unsigned int inuse_pages;  ///< Number of pages currently in use
    ulong bdev;                ///< Block device pointer
    std::string swap_file;     ///< Swap file/device path
};

/**
 * @class Swapinfo
 * @brief Base class for swap information parsing and user space memory access
 *
 * Provides core functionality for:
 * - Parsing swap device information from kernel structures
 * - Reading user space memory (including swapped-out pages)
 * - Accessing pages in physical memory, swap cache, and ZRAM devices
 * - Converting between PTEs, swap entries, and physical locations
 *
 * This class integrates with ZRAM to handle compressed swap pages and
 * provides utilities for reading data across page boundaries.
 */
class Swapinfo : public ParserPlugin {
private:
    std::shared_ptr<Zraminfo> zram_ptr;  ///< ZRAM information handler

    /**
     * @brief Handle swap page fault and retrieve page data
     *
     * Resolves a virtual address to its page data, handling:
     * - Pages in physical memory
     * - Pages in swap cache
     * - Pages swapped to ZRAM devices
     * - Pages swapped to disk
     *
     * @param task_addr Task structure address
     * @param uvaddr User virtual address
     * @return Pointer to page data buffer, or nullptr on failure
     */
    char* do_swap_page(ulonglong task_addr, ulonglong uvaddr);

    /**
     * @brief Get swap_info structure for a given PTE value
     *
     * Extracts swap type from PTE and locates the corresponding
     * swap_info_struct in the kernel.
     *
     * @param pte_val Page table entry value
     * @return Shared pointer to swap_info structure, or nullptr if not found
     */
    std::shared_ptr<swap_info> get_swap_info(ulonglong pte_val);

    /**
     * @brief Get ZRAM device address from swap_info
     *
     * Retrieves the ZRAM device address associated with a swap device
     * by following the block device -> disk -> private_data chain.
     *
     * @param swap_ptr Pointer to swap_info structure
     * @return ZRAM device address, or 0 if not a ZRAM device
     */
    ulong get_zram_addr(std::shared_ptr<swap_info> swap_ptr);

    /**
     * @brief Look up page in swap cache
     *
     * Searches the swap cache (address_space) for a page corresponding
     * to the given PTE value. Uses radix tree or xarray depending on
     * kernel version.
     *
     * @param pte_val Page table entry value
     * @return Page structure address if found in cache, 0 otherwise
     */
    ulong lookup_swap_cache(ulonglong pte_val);

    /**
     * @brief Read boolean value from user space
     * @param task_addr Task structure address
     * @param uvaddr User virtual address
     * @param note Description for debugging
     * @return Boolean value read from user space
     */
    bool uread_bool(ulonglong task_addr, ulonglong uvaddr, const std::string& note);

    /**
     * @brief Read integer value from user space
     * @param task_addr Task structure address
     * @param uvaddr User virtual address
     * @param note Description for debugging
     * @return Integer value read from user space
     */
    int uread_int(ulonglong task_addr, ulonglong uvaddr, const std::string& note);

    /**
     * @brief Read unsigned integer value from user space
     * @param task_addr Task structure address
     * @param uvaddr User virtual address
     * @param note Description for debugging
     * @return Unsigned integer value read from user space
     */
    uint uread_uint(ulonglong task_addr, ulonglong uvaddr, const std::string& note);

    /**
     * @brief Read long value from user space
     * @param task_addr Task structure address
     * @param uvaddr User virtual address
     * @param note Description for debugging
     * @return Long value read from user space
     */
    long uread_long(ulonglong task_addr, ulonglong uvaddr, const std::string& note);

    /**
     * @brief Read unsigned long value from user space
     * @param task_addr Task structure address
     * @param uvaddr User virtual address
     * @param note Description for debugging
     * @return Unsigned long value read from user space
     */
    ulong uread_ulong(ulonglong task_addr, ulonglong uvaddr, const std::string& note);

    /**
     * @brief Read unsigned long long value from user space
     * @param task_addr Task structure address
     * @param uvaddr User virtual address
     * @param note Description for debugging
     * @return Unsigned long long value read from user space
     */
    ulonglong uread_ulonglong(ulonglong task_addr, ulonglong uvaddr, const std::string& note);

    /**
     * @brief Read unsigned short value from user space
     * @param task_addr Task structure address
     * @param uvaddr User virtual address
     * @param note Description for debugging
     * @return Unsigned short value read from user space
     */
    ushort uread_ushort(ulonglong task_addr, ulonglong uvaddr, const std::string& note);

    /**
     * @brief Read short value from user space
     * @param task_addr Task structure address
     * @param uvaddr User virtual address
     * @param note Description for debugging
     * @return Short value read from user space
     */
    short uread_short(ulonglong task_addr, ulonglong uvaddr, const std::string& note);

    /**
     * @brief Read pointer value from user space
     * @param task_addr Task structure address
     * @param uvaddr User virtual address
     * @param note Description for debugging
     * @return Pointer value read from user space
     */
    ulong uread_pointer(ulonglong task_addr, ulonglong uvaddr, const std::string& note);

    /**
     * @brief Read byte value from user space
     * @param task_addr Task structure address
     * @param uvaddr User virtual address
     * @param note Description for debugging
     * @return Byte value read from user space
     */
    unsigned char uread_byte(ulonglong task_addr, ulonglong uvaddr, const std::string& note);

    /**
     * @brief Convert PTE value to ZRAM page index
     *
     * Extracts swap offset from PTE, looks up the swap extent to find
     * the corresponding disk block, and calculates the ZRAM page index.
     *
     * @param swap_ptr Pointer to swap_info structure
     * @param pte_val Page table entry value
     * @return ZRAM page index
     */
    ulonglong pte_handle_index(std::shared_ptr<swap_info> swap_ptr, ulonglong pte_val);

public:
    std::vector<std::shared_ptr<swap_info>> swap_list;  ///< List of swap devices
    int nr_swap;                                         ///< Number of swap devices

    /**
     * @brief Default constructor
     *
     * Initializes Swapinfo with a new ZRAM information object.
     */
    Swapinfo();

    /**
     * @brief Check if ZRAM is enabled in the kernel
     *
     * Delegates to the ZRAM information handler to check if ZRAM
     * support is available.
     *
     * @return true if ZRAM is enabled, false otherwise
     */
    bool is_zram_enable();

    /**
     * @brief Constructor with ZRAM information pointer
     *
     * Initializes Swapinfo with an existing ZRAM information object.
     *
     * @param zram Shared pointer to Zraminfo object
     */
    Swapinfo(std::shared_ptr<Zraminfo> zram);

    /**
     * @brief Destructor
     */
    ~Swapinfo();

    /**
     * @brief Main command handler (empty in base class)
     *
     * Overridden by derived classes to implement command functionality.
     */
    void cmd_main(void) override;

    /**
     * @brief Initialize structure field offsets
     *
     * Initializes offsets for swap-related kernel structures including:
     * - swap_info_struct
     * - block_device
     * - gendisk
     * - swap_extent
     * - address_space
     */
    void init_offset(void) override;

    /**
     * @brief Initialize command information (empty in base class)
     *
     * Overridden by derived classes to set up command metadata.
     */
    void init_command(void) override;

    /**
     * @brief Read C-string from user space
     *
     * Reads a null-terminated string from user space memory.
     *
     * @param task_addr Task structure address
     * @param uvaddr User virtual address
     * @param len Maximum length to read
     * @param note Description for debugging
     * @return String read from user space
     */
    std::string uread_cstring(ulonglong task_addr, ulonglong uvaddr, int len,
                             const std::string& note);

    /**
     * @brief Read memory buffer from user space
     *
     * Reads arbitrary data from user space, handling pages that may be:
     * - In physical memory
     * - In swap cache
     * - Swapped to ZRAM or disk
     *
     * Automatically handles reads that span multiple pages.
     *
     * @param task_addr Task structure address
     * @param uvaddr User virtual address
     * @param len Number of bytes to read
     * @param note Description for debugging
     * @return Vector containing read data
     */
    std::vector<char> uread_memory(ulonglong task_addr, ulonglong uvaddr, int len,
                                   const std::string& note);

    /**
     * @brief Check if PTE represents a swap entry
     *
     * Determines if a page table entry indicates a swapped-out page
     * by checking the present bit and other architecture-specific flags.
     *
     * @param pte Page table entry value
     * @return true if PTE is a swap entry, false otherwise
     */
    bool is_swap_pte(ulong pte);

    /**
     * @brief Parse all swap device information
     *
     * Reads swap device information from kernel structures and populates
     * the swap_list with details about each swap device including:
     * - Device size and usage
     * - File path
     * - Block device information
     * - Address space pointers
     */
    void parser_swap_info();
};

#endif // SWAP_INFO_DEFS_H_
