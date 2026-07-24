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

#ifndef DMA_HEAP_DEFS_H_
#define DMA_HEAP_DEFS_H_

#include "heap.h"
#include "dmabuf.h"

/**
 * @struct dma_heap
 * @brief Represents a DMA heap allocator
 *
 * Contains information about a DMA heap including its operations,
 * private data, reference count, and all buffers allocated from it.
 * DMA heaps are the modern replacement for ION heaps (kernel 5.6+).
 */
struct dma_heap {
    ulong addr;                                 /**< Address of dma_heap structure */
    std::string ops;                            /**< Heap operations name (e.g., system_heap_ops) */
    ulong priv_addr;                            /**< Private data pointer */
    int refcount;                               /**< Reference count */
    std::string name;                           /**< Heap name (e.g., "system", "reserved") */
    uint64_t buf_cnt;                           /**< Number of buffers allocated */
    uint64_t total_allocated;                   /**< Total bytes allocated */
    std::vector<std::shared_ptr<dma_buf>> bufs; /**< List of buffers from this heap */
};

/**
 * @class DmaHeap
 * @brief Parser and analyzer for DMA heap allocators
 *
 * This class provides comprehensive analysis of DMA heaps from kernel crash dumps.
 * It can:
 * - Parse all DMA heaps in the system
 * - Track buffers allocated from each heap
 * - Analyze system heap memory pools
 * - Display heap statistics and buffer information
 * - Support both standard and Qualcomm-specific heap implementations
 */
class DmaHeap : public Heap {
private:
    /** @brief List of all parsed DMA heaps */
    std::vector<std::shared_ptr<dma_heap>> dma_heaps;

    /**
     * @brief Parse ION system heap memory pools
     *
     * Analyzes the memory pool structure for system heaps, including
     * high/low watermarks and page pool statistics.
     *
     * @param heap_ptr Pointer to the heap to analyze
     */
    void parser_ion_system_heap(std::shared_ptr<dma_heap> heap_ptr);

    /**
     * @brief Parse a dynamic page pool structure
     *
     * Extracts information about a page pool including order, counts,
     * and memory usage statistics.
     *
     * @param addr Address of dynamic_page_pool structure
     */
    void parser_dynamic_page_pool(ulong addr);

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
     * @brief Constructor - initializes DMA heap parser
     *
     * Registers kernel structure definitions and initializes the parser
     * with a reference to the DMA buffer parser.
     *
     * @param dmabuf Shared pointer to Dmabuf parser instance
     */
    DmaHeap(std::shared_ptr<Dmabuf> dmabuf);

    /**
     * @brief Get list of all DMA heap addresses
     *
     * Walks the kernel's heap_list to find all registered DMA heaps.
     *
     * @return Vector of heap addresses
     */
    std::vector<ulong> get_heaps() override;

    /**
     * @brief Parse all DMA heaps
     *
     * Walks the heap list and parses each heap's structure, including
     * name, operations, reference count, and associated buffers.
     */
    void parser_heaps() override;

    /**
     * @brief Print summary of all DMA heaps
     *
     * Displays a table showing all heaps with their addresses, names,
     * operations, buffer counts, and total allocated sizes.
     */
    void print_heaps() override;

    /**
     * @brief Print system heap memory pool information
     *
     * Displays detailed information about system heap memory pools,
     * including page pool statistics and watermarks.
     */
    void print_system_heap_pool() override;
};

#endif // DMA_HEAP_DEFS_H_
