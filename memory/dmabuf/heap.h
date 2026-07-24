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

#ifndef HEAP_DEFS_H_
#define HEAP_DEFS_H_

#include "plugin.h"
#include "dmabuf.h"

/**
 * @class Heap
 * @brief Abstract base class for heap parsers
 *
 * This class provides a common interface for parsing different types of
 * memory heaps (DMA heaps, ION heaps, etc.). It serves as the base class
 * for DmaHeap and IonHeap implementations.
 *
 * The class follows the Template Method pattern, where derived classes
 * implement specific heap parsing logic while sharing common infrastructure.
 *
 * Key responsibilities:
 * - Maintain reference to DMA buffer parser
 * - Define interface for heap discovery and parsing
 * - Provide common functionality for heap analysis
 */
class Heap : public ParserPlugin {
protected:
    /**
     * @brief Shared pointer to DMA buffer parser
     *
     * This provides access to the parsed DMA buffers, allowing heap parsers
     * to correlate buffers with their source heaps.
     */
    std::shared_ptr<Dmabuf> dmabuf_ptr;

public:
    /**
     * @brief Constructor - initializes heap parser with DMA buffer reference
     *
     * @param dmabuf Shared pointer to Dmabuf parser instance
     */
    Heap(std::shared_ptr<Dmabuf> dmabuf);

    /**
     * @brief Main command handler (not used for Heap base class)
     *
     * This class is used as a base for DmaHeap and IonHeap, so cmd_main
     * is not implemented at this level.
     */
    void cmd_main(void) override;

    /**
     * @brief Initialize structure offsets (not used for Heap base class)
     *
     * Derived classes implement their own structure initialization.
     */
    void init_offset(void) override;

    /**
     * @brief Initialize command metadata (not used for Heap base class)
     *
     * Derived classes implement their own command initialization.
     */
    void init_command(void) override;

    /**
     * @brief Get list of all heap addresses (pure virtual)
     *
     * Derived classes must implement this to discover heaps in their
     * specific format (DMA heap list, ION heap list, etc.).
     *
     * @return Vector of heap addresses
     */
    virtual std::vector<ulong> get_heaps() = 0;

    /**
     * @brief Parse all heaps (pure virtual)
     *
     * Derived classes must implement this to parse heap structures
     * and extract relevant information.
     */
    virtual void parser_heaps() = 0;

    /**
     * @brief Print summary of all heaps (pure virtual)
     *
     * Derived classes must implement this to display heap information
     * in a formatted table.
     */
    virtual void print_heaps() = 0;

    /**
     * @brief Print system heap memory pool information (pure virtual)
     *
     * Derived classes must implement this to display memory pool
     * statistics for system heaps.
     */
    virtual void print_system_heap_pool() = 0;

    /**
     * @brief Print detailed information for a specific heap (pure virtual)
     *
     * Derived classes must implement this to display all buffers
     * allocated from a specific heap.
     *
     * @param name Name of the heap to print
     */
    virtual void print_heap(std::string name) = 0;
};

#endif // HEAP_DEFS_H_
