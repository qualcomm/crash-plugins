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

#include "dma_heap.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

/**
 * @brief Constructor - initializes DMA heap parser
 *
 * Registers all necessary kernel structure definitions for parsing DMA heaps
 * and their associated structures (plist_node, dynamic_page_pool, etc.).
 *
 * @param dmabuf Shared pointer to Dmabuf parser instance
 */
DmaHeap::DmaHeap(std::shared_ptr<Dmabuf> dmabuf) : Heap(dmabuf) {
    // Initialize plist_node structure (used in heap lists)
    field_init(plist_node, node_list);

    // Initialize dma_heap structure fields
    field_init(dma_heap, name);
    field_init(dma_heap, refcount);
    field_init(dma_heap, priv);
    field_init(dma_heap, list);
    field_init(dma_heap, ops);
    struct_init(dma_heap);

    // Initialize qcom_system_heap structure
    struct_init(qcom_system_heap);
    field_init(qcom_system_heap, uncached);
    field_init(qcom_system_heap, pool_list);

    // Initialize dynamic_page_pool structure
    struct_init(dynamic_page_pool);
    field_init(dynamic_page_pool, high_count);
    field_init(dynamic_page_pool, low_count);
    field_init(dynamic_page_pool, count);
    field_init(dynamic_page_pool, order);
    field_init(dynamic_page_pool, high_items);
    field_init(dynamic_page_pool, low_items);
}

/**
 * @brief Print summary of all DMA heaps
 *
 * Displays a formatted table showing all DMA heaps with their:
 *
 * The table columns are dynamically sized based on content.
 */
void DmaHeap::print_heaps() {
    if (dma_heaps.empty()) {
        LOGE("No DMA heaps to display");
        return;
    }
    // Calculate maximum column widths for better formatting
    size_t name_max_len = 10;
    size_t ops_max_len = 10;
    for (auto& heap_ptr : dma_heaps) {
        name_max_len = std::max(name_max_len, heap_ptr->name.size());
        ops_max_len = std::max(ops_max_len, heap_ptr->ops.size());
    }
    // Print table header
    std::ostringstream oss;
    oss << std::left << std::setw(VADDR_PRLEN + 2) << "dma_heap" << " "
        << std::left << std::setw(name_max_len + 2) << "Name" << " "
        << std::left << std::setw(4) << "ref" << " "
        << std::left << std::setw(ops_max_len + 2) << "ops" << " "
        << std::left << std::setw(VADDR_PRLEN + 2) << "priv" << " "
        << std::left << std::setw(7) << "buf_cnt" << " "
        << std::left << "total_size" << " \n";

    // Print each heap's information
    for (const auto& heap_ptr : dma_heaps) {
        // Calculate total size of all buffers from this heap
        size_t total_size = 0;
        for (const auto& dma_buf : heap_ptr->bufs) {
            total_size += dma_buf->size;
        }
        oss << std::left << std::hex << std::setw(VADDR_PRLEN + 2) << heap_ptr->addr << " "
            << std::left << std::setw(name_max_len + 2) << heap_ptr->name << " "
            << std::left << std::dec << std::setw(4) << heap_ptr->refcount << " "
            << std::left << std::setw(ops_max_len + 2) << heap_ptr->ops << " "
            << std::left << std::hex << std::setw(VADDR_PRLEN + 2) << heap_ptr->priv_addr << " "
            << std::left << std::dec << std::setw(7) << heap_ptr->bufs.size() << " "
            << std::left << csize(total_size) << " \n";
    }
    PRINT("%s", oss.str().c_str());
}

/**
 * @brief Print detailed information for a specific heap
 *
 * Searches for a heap with the given name and displays all buffers
 * allocated from it with full details.
 *
 * @param name Name of the heap to print
 */
void DmaHeap::print_heap(std::string name) {
    bool found = false;
    for (const auto& heap_ptr : dma_heaps) {
        if (heap_ptr->name == name) {
            found = true;
            // Print detailed information for each buffer from this heap
            for (const auto& buf_ptr : heap_ptr->bufs) {
                dmabuf_ptr->print_dma_buf(buf_ptr);
            }
        }
    }
    if (!found) {
        PRINT("Heap '%s' not found", name.c_str());
    }
}

/**
 * @brief Print system heap memory pool information
 *
 * Finds all heaps using system_heap_ops and displays their memory pool
 * statistics including page pools at different orders.
 */
void DmaHeap::print_system_heap_pool() {
    for (const auto& heap_ptr : dma_heaps) {
        if (heap_ptr->ops == "system_heap_ops") {
            parser_ion_system_heap(heap_ptr);
        }
    }
}

/**
 * @brief Parse ION system heap memory pools
 *
 * Analyzes the memory pool structure for system heaps. System heaps typically
 * maintain multiple page pools at different orders (e.g., order 9, 4, 0) for
 * efficient allocation. This function extracts and displays statistics for
 * each pool including high/low watermarks.
 *
 * @param heap_ptr Pointer to the heap to analyze
 */
void DmaHeap::parser_ion_system_heap(std::shared_ptr<dma_heap> heap_ptr) {
    // Validate private data pointer
    if (!is_kvaddr(heap_ptr->priv_addr)) {
        LOGE("Invalid priv_addr 0x%lx of heap 0x%lx", heap_ptr->priv_addr, heap_ptr->addr);
        return;
    }

    // Read pool list address
    ulong pools_addr = read_pointer(heap_ptr->priv_addr + field_offset(qcom_system_heap, pool_list), "pools");
    if (!is_kvaddr(pools_addr)) {
        LOGE("Invalid pools_addr 0x%lx of heap 0x%lx", pools_addr, heap_ptr->addr);
        return;
    }
    // Print heap name and table header
    PRINT("%s: \n", heap_ptr->name.c_str());
    std::ostringstream oss;
    oss << std::left << std::setw(VADDR_PRLEN + 2) << "page_pool" << " "
        << std::left << std::setw(5) << "order" << " "
        << std::left << std::setw(10) << "high" << " "
        << std::left << std::setw(10) << "low" << " "
        << std::left << std::setw(10) << "total";
    PRINT("   %s \n", oss.str().c_str());

    // Parse up to 3 page pools (typical configuration: order 9, 4, 0)
    int pool_count = 0;
    for (size_t i = 0; i < 3; i++) {
        ulong pool_addr = read_pointer(pools_addr + i * sizeof(void *), "pool");
        if (!is_kvaddr(pool_addr)) {
            LOGE("Pool %zu: invalid address 0x%lx, skipping", i, pool_addr);
            continue;
        }
        parser_dynamic_page_pool(pool_addr);
        pool_count++;
    }
    PRINT("\n");
}

/**
 * @brief Parse a dynamic page pool structure
 *
 * Extracts detailed information about a page pool including:
 * - Order (size = 2^order pages)
 * - High watermark count
 * - Low watermark count
 * - Current total count
 *
 * The counts are converted to bytes for easier interpretation.
 *
 * @param addr Address of dynamic_page_pool structure
 */
void DmaHeap::parser_dynamic_page_pool(ulong addr) {
    // Read pool structure
    void *pool_buf = read_struct(addr, "dynamic_page_pool");
    if (!pool_buf) {
        LOGE("Failed to read dynamic_page_pool at 0x%lx", addr);
        return;
    }

    // Extract pool parameters
    uInt order = UINT(pool_buf + field_offset(dynamic_page_pool, order));
    int buf_size = power(2, order) * page_size;

    // Extract and sanitize high count
    int high_count = INT(pool_buf + field_offset(dynamic_page_pool, high_count));
    if (high_count < 0) {
        LOGE("Negative high_count: %d, setting to 0", high_count);
        high_count = 0;
    }
    high_count = high_count * buf_size;

    // Extract and sanitize low count
    int low_count = INT(pool_buf + field_offset(dynamic_page_pool, low_count));
    if (low_count < 0) {
        LOGE("Negative low_count: %d, setting to 0", low_count);
        low_count = 0;
    }
    low_count = low_count * buf_size;

    // Extract and sanitize current count
    int count = INT(pool_buf + field_offset(dynamic_page_pool, count));
    if (count < 0) {
        LOGE("Negative count: %d, setting to 0", count);
        count = 0;
    }
    count = count * buf_size;
    // Note: The following code for walking page lists is commented out
    // as it's not currently needed, but kept for potential future use
    // int offset = field_offset(page,lru);
    // ulong high_items_head = addr + field_offset(dynamic_page_pool,high_items);
    // std::vector<ulong> high_page_list = for_each_list(high_items_head,offset);
    // ulong low_items_head = addr + field_offset(dynamic_page_pool,low_items);
    // std::vector<ulong> low_page_list = for_each_list(low_items_head,offset);
    // Print pool information
    std::ostringstream oss;
    oss << std::left << std::hex << std::setw(VADDR_PRLEN + 2) << addr << " "
        << std::left << std::setw(5) << order << " "
        << std::left << std::setw(10) << csize(high_count) << " "
        << std::left << std::setw(10) << csize(low_count) << " "
        << std::left << std::setw(10) << csize(count);
    PRINT("   %s \n", oss.str().c_str());
    FREEBUF(pool_buf);
}

/**
 * @brief Get list of all DMA heap addresses
 *
 * Walks the kernel's heap_list to find all registered DMA heaps.
 * The heap_list is a global list maintained by the DMA-BUF heap framework.
 *
 * @return Vector of heap addresses, empty if heap_list doesn't exist
 */
std::vector<ulong> DmaHeap::get_heaps() {
    std::vector<ulong> heap_list;
    // Check if heap_list symbol exists
    if (!csymbol_exists("heap_list")) {
        LOGE("heap_list symbol doesn't exist in this kernel");
        return heap_list;
    }
    // Get heap_list address
    ulong list_head = csymbol_value("heap_list");
    if (!is_kvaddr(list_head)) {
        LOGE("Invalid heap_list address: 0x%lx", list_head);
        return heap_list;
    }
    LOGD("Found heap_list at address: 0x%lx", list_head);
    // Walk the list to get all heap addresses
    heap_list = for_each_list(list_head, field_offset(dma_heap, list));
    LOGD("Found %zu DMA heaps", heap_list.size());
    return heap_list;
}

/**
 * @brief Parse all DMA heaps
 *
 * Walks the heap list and parses each heap's structure, extracting:
 * - Heap address
 * - Heap name
 * - Reference count
 * - Private data pointer
 * - Operations name
 * - Associated buffers
 *
 * This function also correlates heaps with buffers by matching heap addresses.
 */
void DmaHeap::parser_heaps() {
    // Get list of all heap addresses
    std::vector<ulong> heaps = get_heaps();
    if (heaps.empty()) {
        LOGE("No heaps found");
        return;
    }
    int parsed_count = 0;
    for (const auto& addr : heaps) {
        LOGD("Parsing heap#%d at address: 0x%lx", parsed_count, addr);
        // Read heap structure
        void *heap_buf = read_struct(addr, "dma_heap");
        if (!heap_buf) {
            LOGE("Failed to read dma_heap at 0x%lx, skipping", addr);
            continue;
        }
        // Create and populate heap object
        std::shared_ptr<dma_heap> heap_ptr = std::make_shared<dma_heap>();
        heap_ptr->addr = addr;
        // Parse heap name
        ulong name_addr = ULONG(heap_buf + field_offset(dma_heap, name));
        if (is_kvaddr(name_addr)) {
            heap_ptr->name = read_cstring(name_addr, 64, "dma_heap_name");
        }
        // Parse reference count
        heap_ptr->refcount = INT(heap_buf + field_offset(dma_heap, refcount));
        // Parse private data pointer
        heap_ptr->priv_addr = ULONG(heap_buf + field_offset(dma_heap, priv));
        // Parse operations name
        heap_ptr->ops = "";
        ulong ops_addr = ULONG(heap_buf + field_offset(dma_heap, ops));
        heap_ptr->ops = to_symbol(ops_addr);
        // Find all buffers allocated from this heap
        int buf_count = 0;
        for (const auto& dmabuf_ptr : dmabuf_ptr->buf_list) {
            if (dmabuf_ptr->heap == addr) {
                heap_ptr->bufs.push_back(dmabuf_ptr);
                buf_count++;
            }
        }
        LOGD("  %d buffers for heap '%s'", buf_count, heap_ptr->name.c_str());
        FREEBUF(heap_buf);
        dma_heaps.push_back(heap_ptr);
        parsed_count++;
    }
}

#pragma GCC diagnostic pop
