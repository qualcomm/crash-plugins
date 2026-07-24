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

#include "heap.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

/**
 * @brief Main command handler (not used for Heap base class)
 *
 * This is an abstract base class used by DmaHeap and IonHeap, so cmd_main
 * is not implemented. Derived classes handle their own command processing
 * through the DmaIon plugin.
 */
void Heap::cmd_main(void) {

}

/**
 * @brief Initialize structure offsets (not used for Heap base class)
 *
 * This is an abstract base class, so structure initialization is handled
 * by derived classes (DmaHeap, IonHeap) which know their specific
 * structure requirements.
 */
void Heap::init_offset(void) {

}

/**
 * @brief Initialize command metadata (not used for Heap base class)
 *
 * This is an abstract base class, so command initialization is handled
 * by the DmaIon plugin which provides the user interface.
 */
void Heap::init_command(void) {

}

/**
 * @brief Constructor - initializes heap parser with DMA buffer reference
 *
 * Stores a reference to the DMA buffer parser, which allows heap parsers
 * to correlate buffers with their source heaps. This is essential for
 * tracking which buffers were allocated from which heaps.
 *
 * @param dmabuf Shared pointer to Dmabuf parser instance
 */
Heap::Heap(std::shared_ptr<Dmabuf> dmabuf) : dmabuf_ptr(dmabuf) {

}

#pragma GCC diagnostic pop
