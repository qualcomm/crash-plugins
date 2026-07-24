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

#ifndef DMA_ION_DEFS_H_
#define DMA_ION_DEFS_H_

#include "plugin.h"
#include "dmabuf.h"
#include "heap.h"
#include "dma_heap.h"
#include "ion_heap.h"

/**
 * @class DmaIon
 * @brief Plugin for analyzing DMA buffer and ION heap information from kernel crash dumps
 *
 * This plugin provides comprehensive analysis of DMA buffers and heap allocations,
 * supporting both modern DMA-BUF framework and legacy ION allocator. It can:
 * - Display all DMA buffers with their metadata
 * - Show heap information and statistics
 * - Track buffer usage per process
 * - Export buffer contents to files
 * - Analyze memory pool states
 */
class DmaIon : public ParserPlugin {
private:
    /** @brief Shared pointer to DMA buffer parser instance */
    std::shared_ptr<Dmabuf> dmabuf_ptr;

    /** @brief Shared pointer to heap parser instance (DMA heap or ION heap) */
    std::shared_ptr<Heap> heap_ptr;

public:
    /**
     * @brief Constructor - initializes the DmaIon plugin
     */
    DmaIon();

    /**
     * @brief Main command handler - processes user commands and arguments
     *
     * Parses command-line options and dispatches to appropriate handlers:
     */
    void cmd_main(void) override;

    /**
     * @brief Initialize structure offsets for DMA heap and ION heap
     *
     * Registers kernel structure definitions needed for parsing:
     * - dma_heap: Modern DMA-BUF heap structure
     * - ion_heap: Legacy ION heap structure
     */
    void init_offset(void) override;

    /**
     * @brief Initialize command metadata and help information
     *
     * Sets up command name, description, and detailed usage examples
     * for the dmabuf command interface.
     */
    void init_command(void) override;

    /** @brief Macro to define plugin instance and wrapper function */
    DEFINE_PLUGIN_INSTANCE(DmaIon)
};

#endif // DMA_ION_DEFS_H_
