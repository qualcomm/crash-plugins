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

#ifndef ION_HEAP_DEFS_H_
#define ION_HEAP_DEFS_H_

#include "heap.h"
#include "dmabuf.h"

/**
 * @enum ion_heap_type
 * @brief ION heap type enumeration
 *
 * Defines the types of ION heaps available in the kernel.
 * ION is the legacy memory allocator replaced by DMA-BUF heaps in kernel 5.6+.
 */
enum ion_heap_type {
    ION_HEAP_TYPE_SYSTEM = 0,   /**< System heap - allocates from system memory */
    ION_HEAP_TYPE_DMA = 2,       /**< DMA heap - allocates from DMA memory */
    ION_HEAP_TYPE_CUSTOM = 16,   /**< Custom heap - vendor-specific implementation */
    ION_HEAP_TYPE_MAX = 31,      /**< Maximum heap type value */
};

/**
 * @struct ion_heap
 * @brief Represents an ION heap allocator
 *
 * Contains information about an ION heap including its type, operations,
 * flags, and all buffers allocated from it. ION heaps are the legacy
 * memory allocator used before DMA-BUF heaps.
 */
struct ion_heap {
    ulong addr;                                 /**< Address of ion_heap structure */
    enum ion_heap_type type;                    /**< Heap type (system, DMA, custom, etc.) */
    std::string ops;                            /**< Heap operations name */
    unsigned long flags;                        /**< Heap flags */
    unsigned int id;                            /**< Heap ID */
    std::string name;                           /**< Heap name */
    uint64_t buf_cnt;                           /**< Number of buffers allocated */
    uint64_t total_allocated;                   /**< Total bytes allocated */
    std::vector<std::shared_ptr<dma_buf>> bufs; /**< List of buffers from this heap */
};

/**
 * @class IonHeap
 * @brief Parser and analyzer for ION heap allocators
 *
 * This class provides comprehensive analysis of ION heaps from kernel crash dumps.
 * It supports both standard ION heaps and Qualcomm-specific MSM ION heaps.
 *
 * Features:
 * - Parse all ION heaps in the system
 * - Track buffers allocated from each heap
 * - Analyze system heap memory pools (both standard and MSM variants)
 * - Display heap statistics and buffer information
 * - Support multiple heap discovery methods (internal_dev, heaps array)
 */
class IonHeap : public Heap {
private:
    /** @brief Map of heap type enum values to their string names */
    std::unordered_map<int, std::string> heap_type;

    /** @brief List of all parsed ION heaps */
    std::vector<std::shared_ptr<ion_heap>> ion_heaps;

    /**
     * @brief Get ION heaps via internal_dev structure
     *
     * Walks the heap list from the internal_dev global variable.
     * This is the method used in older ION implementations.
     *
     * @return Vector of heap addresses
     */
    std::vector<ulong> get_ion_heaps_by_internal_dev();

    /**
     * @brief Get ION heaps via heaps array
     *
     * Reads the heaps array and num_heaps global variables.
     * This is the method used in newer ION implementations.
     *
     * @return Vector of heap addresses
     */
    std::vector<ulong> get_ion_heaps_by_heaps();

    /**
     * @brief Get list of all ION heap addresses
     *
     * Automatically selects the appropriate method based on available symbols.
     *
     * @return Vector of heap addresses
     */
    std::vector<ulong> get_heaps() override;

    /**
     * @brief Parse all ION heaps
     *
     * Walks the heap list and parses each heap's structure.
     */
    void parser_heaps() override;

    /**
     * @brief Print summary of all ION heaps
     *
     * Displays a table with heap information.
     */
    void print_heaps() override;

    /**
     * @brief Print system heap memory pool information
     *
     * Displays memory pool statistics for system heaps.
     */
    void print_system_heap_pool() override;

    /**
     * @brief Parse standard ION system heap memory pools
     *
     * Analyzes memory pools for standard ion_system_heap.
     *
     * @param addr Address of ion_heap structure
     */
    void parser_ion_system_heap(ulong addr);

    /**
     * @brief Parse Qualcomm MSM ION system heap memory pools
     *
     * Analyzes memory pools for Qualcomm's ion_msm_system_heap,
     * which has separate pools for uncached, cached, and secure memory.
     *
     * @param addr Address of ion_heap structure
     */
    void parser_ion_msm_system_heap(ulong addr);

    /**
     * @brief Parse a page pool structure (unused, kept for compatibility)
     *
     * @param addr Address of page_pool structure
     */
    void parser_page_pool(ulong addr);

    /**
     * @brief Parse a standard ION page pool
     *
     * Extracts information about a page pool including order and counts.
     *
     * @param addr Address of ion_page_pool structure
     */
    void parser_ion_page_pool(ulong addr);

    /**
     * @brief Parse a Qualcomm MSM ION page pool
     *
     * Extracts information about an MSM page pool including cached flag.
     *
     * @param addr Address of ion_msm_page_pool structure
     */
    void parser_ion_msm_page_pool(ulong addr);

    /**
     * @brief Print detailed information for a specific heap
     *
     * Displays all buffers allocated from the specified heap.
     *
     * @param name Name of the heap to print
     */
    void print_heap(std::string name);

public:
    /**
     * @brief Constructor - initializes ION heap parser
     *
     * Registers kernel structure definitions, initializes heap type map,
     * and automatically parses all heaps.
     *
     * @param dmabuf Shared pointer to Dmabuf parser instance
     */
    IonHeap(std::shared_ptr<Dmabuf> dmabuf);
};

#endif // ION_HEAP_DEFS_H_
