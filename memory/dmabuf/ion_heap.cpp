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

#include "ion_heap.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

/**
 * @brief Constructor - initializes ION heap parser
 *
 * Registers all necessary kernel structure definitions for parsing ION heaps
 * and their associated structures. Also initializes the heap type map and
 * automatically parses all heaps.
 *
 * @param dmabuf Shared pointer to Dmabuf parser instance
 */
IonHeap::IonHeap(std::shared_ptr<Dmabuf> dmabuf) : Heap(dmabuf) {
    // Initialize ion_device structure fields
    field_init(ion_device, heaps);

    // Initialize ion_heap structure fields
    field_init(ion_heap, node);
    field_init(ion_heap, name);
    field_init(ion_heap, id);
    field_init(ion_heap, flags);
    field_init(ion_heap, ops);
    field_init(ion_heap, num_of_buffers);
    field_init(ion_heap, num_of_alloc_bytes);
    field_init(ion_heap, type);
    struct_init(ion_heap);

    // Initialize plist_node structure (used in heap lists)
    field_init(plist_node, node_list);

    // Initialize page structure field
    field_init(page, lru);

    struct_init(ion_system_heap);
    // Initialize ion_system_heap structure fields
    field_init(ion_system_heap, heap);
    field_init(ion_system_heap, pools);

    struct_init(ion_msm_system_heap);
    // Initialize ion_msm_system_heap structure fields
    field_init(ion_msm_system_heap, heap);
    field_init(ion_msm_system_heap, uncached_pools);
    field_init(ion_msm_system_heap, cached_pools);
    field_init(ion_msm_system_heap, secure_pools);
    field_init(msm_ion_heap, ion_heap);

    // Initialize ion_page_pool structure
    struct_init(ion_page_pool);
    field_init(ion_page_pool, high_count);
    field_init(ion_page_pool, low_count);
    field_init(ion_page_pool, order);
    field_init(ion_page_pool, high_items);
    field_init(ion_page_pool, low_items);

    // Initialize ion_msm_page_pool structure
    struct_init(ion_msm_page_pool);
    field_init(ion_msm_page_pool, high_count);
    field_init(ion_msm_page_pool, low_count);
    field_init(ion_msm_page_pool, count);
    field_init(ion_msm_page_pool, cached);
    field_init(ion_msm_page_pool, order);
    field_init(ion_msm_page_pool, high_items);
    field_init(ion_msm_page_pool, low_items);

    // Initialize heap type map from kernel enum values
    heap_type[read_enum_val("ION_HEAP_TYPE_SYSTEM")] = "ION_HEAP_TYPE_SYSTEM";
    heap_type[read_enum_val("ION_HEAP_TYPE_DMA")] = "ION_HEAP_TYPE_DMA";
    heap_type[read_enum_val("ION_HEAP_TYPE_CUSTOM")] = "ION_HEAP_TYPE_CUSTOM";
    heap_type[read_enum_val("ION_HEAP_TYPE_MAX")] = "ION_HEAP_TYPE_MAX";

    // Automatically parse all heaps during construction
    parser_heaps();
}

/**
 * @brief Print summary of all ION heaps
 *
 * Displays a formatted table showing all ION heaps with their:
 *
 * The table columns are dynamically sized based on content.
 */
void IonHeap::print_heaps() {
    if (ion_heaps.empty()) {
        PRINT("No ION heaps to display");
        return;
    }
    // Calculate maximum column widths for better formatting
    size_t name_max_len = 10;
    size_t ops_max_len = 10;
    for (auto& heap_ptr : ion_heaps) {
        name_max_len = std::max(name_max_len, heap_ptr->name.size());
        ops_max_len = std::max(ops_max_len, heap_ptr->ops.size());
    }
    // Print table header
    std::ostringstream oss;
    oss << std::left << std::setw(3) << "Id" << " "
        << std::left << std::setw(VADDR_PRLEN + 2) << "ion_heap" << " "
        << std::left << std::setw(22) << "type" << " "
        << std::left << std::setw(name_max_len + 2) << "Name" << " "
        << std::left << std::setw(6) << "flags" << " "
        << std::left << std::setw(ops_max_len + 2) << "ops" << " "
        << std::left << std::setw(7) << "buf_cnt" << " "
        << std::left << "total_size"
        << "\n";

    // Print each heap's information
    for (const auto& heap_ptr : ion_heaps) {
        oss << std::left << std::dec << std::setw(3) << heap_ptr->id << " "
            << std::left << std::hex << std::setw(VADDR_PRLEN + 2) << heap_ptr->addr << " "
            << std::left << std::setw(22) << heap_type[heap_ptr->type] << " "
            << std::left << std::setw(name_max_len + 2) << heap_ptr->name << " "
            << std::left << std::dec << std::setw(6) << heap_ptr->flags << " "
            << std::left << std::setw(ops_max_len + 2) << heap_ptr->ops << " "
            << std::left << std::dec << std::setw(7) << heap_ptr->buf_cnt << " "
            << std::left << csize(heap_ptr->total_allocated)
            << "\n";
    }
    PRINT("%s \n", oss.str().c_str());
}

/**
 * @brief Print system heap memory pool information
 *
 * Finds all heaps using system_heap_ops and displays their memory pool
 * statistics. Supports both standard ion_system_heap and Qualcomm's
 * ion_msm_system_heap structures.
 */
void IonHeap::print_system_heap_pool() {
    for (const auto& heap_ptr : ion_heaps) {
        if (heap_ptr->ops == "system_heap_ops") {
            // Try standard ion_system_heap first
            if (struct_size(ion_system_heap) != -1) {
                parser_ion_system_heap(heap_ptr->addr);
            }
            // Fall back to Qualcomm MSM variant
            else if (struct_size(ion_msm_system_heap) != -1) {
                parser_ion_msm_system_heap(heap_ptr->addr);
            } else {
                LOGE("No supported system heap structure found");
            }
        }
    }
}

/**
 * @brief Parse standard ION system heap memory pools
 *
 * Analyzes memory pools for standard ion_system_heap. System heaps typically
 * maintain multiple page pools at different orders for efficient allocation.
 *
 * @param addr Address of ion_heap structure
 */
void IonHeap::parser_ion_system_heap(ulong addr) {
    // Calculate heap address (ion_heap is embedded in ion_system_heap)
    ulong heap_addr = addr - field_offset(ion_system_heap, heap);
    // Print table header
    PRINT("pools: \n");
    std::ostringstream oss;
    oss << std::left << std::setw(VADDR_PRLEN + 2) << "page_pool" << " "
        << std::left << std::setw(5) << "order" << " "
        << std::left << std::setw(10) << "high" << " "
        << std::left << std::setw(10) << "low" << " "
        << std::left << std::setw(10) << "total";
    PRINT("   %s \n", oss.str().c_str());

    // Parse each pool in the pools array
    size_t pools_cnt = field_size(ion_system_heap, pools) / sizeof(void *);
    ulong pools_addr = heap_addr + field_offset(ion_system_heap, pools);
    for (size_t i = 0; i < pools_cnt; i++) {
        ulong pool_addr = read_pointer(pools_addr + i * sizeof(void *), "pool");
        if (!is_kvaddr(pool_addr)) {
            LOGE("Pool %zu: invalid address 0x%lx, skipping", i, pool_addr);
            continue;
        }
        parser_ion_page_pool(pool_addr);
    }
}

/**
 * @brief Parse Qualcomm MSM ION system heap memory pools
 *
 * Analyzes memory pools for Qualcomm's ion_msm_system_heap, which has
 * separate pools for uncached, cached, and secure memory. This provides
 * more granular control over memory attributes.
 *
 * @param addr Address of ion_heap structure
 */
void IonHeap::parser_ion_msm_system_heap(ulong addr) {
    // Calculate heap address (ion_heap is embedded in msm_ion_heap which is embedded in ion_msm_system_heap)
    ulong heap_addr = addr - field_offset(msm_ion_heap, ion_heap) - field_offset(ion_msm_system_heap, heap);
    LOGD("Calculated heap_addr: 0x%lx", heap_addr);

    // Parse uncached pools
    PRINT("uncached_pools: \n");
    std::ostringstream oss;
    oss << std::left << std::setw(VADDR_PRLEN + 2) << "page_pool" << " "
        << std::left << std::setw(5) << "order" << " "
        << std::left << std::setw(10) << "high" << " "
        << std::left << std::setw(10) << "low" << " "
        << std::left << std::setw(10) << "total" << " "
        << std::left << "cached"
        << "\n";
    PRINT("   %s \n", oss.str().c_str());

    size_t uncached_pools_cnt = field_size(ion_msm_system_heap, uncached_pools) / sizeof(void *);
    ulong uncached_pools_addr = heap_addr + field_offset(ion_msm_system_heap, uncached_pools);

    int uncached_count = 0;
    for (size_t i = 0; i < uncached_pools_cnt; i++) {
        ulong pools_addr = read_pointer(uncached_pools_addr + i * sizeof(void *), "uncached_pool");
        if (!is_kvaddr(pools_addr)) {
            LOGE("Uncached pool %zu: invalid address, skipping", i);
            continue;
        }
        parser_ion_msm_page_pool(pools_addr);
        uncached_count++;
    }
    // Parse cached pools
    PRINT("\n\ncached_pools: \n");
    oss.str("");
    oss << std::left << std::setw(VADDR_PRLEN + 2) << "page_pool" << " "
        << std::left << std::setw(5) << "order" << " "
        << std::left << std::setw(10) << "high" << " "
        << std::left << std::setw(10) << "low" << " "
        << std::left << std::setw(10) << "total" << " "
        << std::left << "cached"
        << "\n";
    PRINT("   %s \n", oss.str().c_str());

    size_t cached_pools_cnt = field_size(ion_msm_system_heap, cached_pools) / sizeof(void *);
    ulong cached_pools_addr = heap_addr + field_offset(ion_msm_system_heap, cached_pools);
    for (size_t i = 0; i < cached_pools_cnt; i++) {
        ulong pools_addr = read_pointer(cached_pools_addr + i * sizeof(void *), "cached_pools");
        if (!is_kvaddr(pools_addr)) {
            LOGE("Cached pool %zu: invalid address, skipping", i);
            continue;
        }
        parser_ion_msm_page_pool(pools_addr);
    }

    // Parse secure pools
    PRINT("\n\nsecure_pools: \n");
    oss.str("");
    oss << std::left << std::setw(VADDR_PRLEN + 2) << "page_pool" << " "
        << std::left << std::setw(5) << "order" << " "
        << std::left << std::setw(10) << "high" << " "
        << std::left << std::setw(10) << "low" << " "
        << std::left << std::setw(10) << "total" << " "
        << std::left << "cached"
        << "\n";
    PRINT("   %s \n", oss.str().c_str());

    size_t secure_pools_cnt = field_size(ion_msm_system_heap, secure_pools) / sizeof(void *) / cached_pools_cnt;
    ulong secure_pools_addr = heap_addr + field_offset(ion_msm_system_heap, secure_pools);
    for (size_t i = 0; i < secure_pools_cnt; i++) {
        for (size_t j = 0; j < cached_pools_cnt; j++) {
            ulong pools_addr = read_pointer(secure_pools_addr + i * j * sizeof(void *), "secure_pools");
            if (!is_kvaddr(pools_addr)) {
                LOGE("Secure pool %zu: invalid address, skipping", i);
                continue;
            }
            parser_ion_msm_page_pool(pools_addr);
        }
    }
}

/**
 * @brief Parse a standard ION page pool
 *
 * Extracts detailed information about a page pool including:
 * - Order (size = 2^order pages)
 * - High watermark count
 * - Low watermark count
 * - Total count (high + low)
 *
 * The counts are converted to bytes for easier interpretation.
 *
 * @param addr Address of ion_page_pool structure
 */
void IonHeap::parser_ion_page_pool(ulong addr) {
    // Read pool structure
    void *pool_buf = read_struct(addr, "ion_page_pool");
    if (!pool_buf) {
        LOGE("Failed to read ion_page_pool at 0x%lx", addr);
        return;
    }
    // Extract pool parameters
    uInt order = UINT(pool_buf + field_offset(ion_page_pool, order));
    size_t buf_size = power(2, order) * page_size;

    // Extract and sanitize high count
    size_t high_count = INT(pool_buf + field_offset(ion_page_pool, high_count));
    if (high_count < 0) {
        LOGE("Negative high_count: %zu, setting to 0", high_count);
        high_count = 0;
    }
    high_count = high_count * buf_size;

    // Extract and sanitize low count
    size_t low_count = INT(pool_buf + field_offset(ion_page_pool, low_count));
    if (low_count < 0) {
        LOGE("Negative low_count: %zu, setting to 0", low_count);
        low_count = 0;
    }
    low_count = low_count * buf_size;
    // Note: The following code for walking page lists is commented out
    // as it's not currently needed, but kept for potential future use
    // int offset = field_offset(page,lru);
    // ulong high_items_head = addr + field_offset(ion_page_pool,high_items);
    // std::vector<ulong> high_page_list = for_each_list(high_items_head,offset);
    // ulong low_items_head = addr + field_offset(ion_page_pool,low_items);
    // std::vector<ulong> low_page_list = for_each_list(low_items_head,offset);

    // Print pool information
    std::ostringstream oss;
    oss << std::left << std::hex << std::setw(VADDR_PRLEN + 2) << addr << " "
        << std::left << std::setw(5) << order << " "
        << std::left << std::setw(10) << csize(high_count) << " "
        << std::left << std::setw(10) << csize(low_count) << " "
        << std::left << std::setw(10) << csize(high_count + low_count);
    PRINT("   %s \n", oss.str().c_str());
    FREEBUF(pool_buf);
}

/**
 * @brief Parse a Qualcomm MSM ION page pool
 *
 * Extracts detailed information about an MSM page pool including:
 * - Order (size = 2^order pages)
 * - High watermark count
 * - Low watermark count
 * - Current total count
 * - Cached flag (whether pages are cached or uncached)
 *
 * The counts are converted to bytes for easier interpretation.
 *
 * @param addr Address of ion_msm_page_pool structure
 */
void IonHeap::parser_ion_msm_page_pool(ulong addr) {
    // Read pool structure
    void *pool_buf = read_struct(addr, "ion_msm_page_pool");
    if (!pool_buf) {
        LOGE("Failed to read ion_msm_page_pool at 0x%lx", addr);
        return;
    }

    // Extract pool parameters
    uInt order = UINT(pool_buf + field_offset(ion_msm_page_pool, order));
    int buf_size = power(2, order) * page_size;
    // Extract and sanitize high count
    int high_count = INT(pool_buf + field_offset(ion_msm_page_pool, high_count));
    if (high_count < 0) {
        LOGE("Negative high_count: %d, setting to 0", high_count);
        high_count = 0;
    }
    high_count = high_count * buf_size;

    // Extract and sanitize low count
    int low_count = INT(pool_buf + field_offset(ion_msm_page_pool, low_count));
    if (low_count < 0) {
        LOGE("Negative low_count: %d, setting to 0", low_count);
        low_count = 0;
    }
    low_count = low_count * buf_size;

    // Extract and sanitize current count
    int count = INT(pool_buf + field_offset(ion_msm_page_pool, count));
    if (count < 0) {
        LOGE("Negative count: %d, setting to 0", count);
        count = 0;
    }
    count = count * buf_size;

    // Extract cached flag
    bool cached = UINT(pool_buf + field_offset(ion_msm_page_pool, cached));
    // Note: The following code for walking page lists is commented out
    // as it's not currently needed, but kept for potential future use
    // int offset = field_offset(page,lru);
    // ulong high_items_head = addr + field_offset(ion_msm_page_pool,high_items);
    // std::vector<ulong> high_page_list = for_each_list(high_items_head,offset);
    // ulong low_items_head = addr + field_offset(ion_msm_page_pool,low_items);
    // std::vector<ulong> low_page_list = for_each_list(low_items_head,offset);

    // Print pool information
    std::ostringstream oss;
    oss << std::left << std::hex << std::setw(VADDR_PRLEN + 2) << addr << " "
        << std::left << std::setw(5) << order << " "
        << std::left << std::setw(10) << csize(high_count) << " "
        << std::left << std::setw(10) << csize(low_count) << " "
        << std::left << std::setw(10) << csize(count) << " "
        << std::left << (cached ? "True" : "False");
    PRINT("   %s \n", oss.str().c_str());
    FREEBUF(pool_buf);
}

/**
 * @brief Print detailed information for a specific heap
 *
 * Searches for a heap with the given name and displays all buffers
 * allocated from it with full details.
 *
 * @param name Name of the heap to print
 */
void IonHeap::print_heap(std::string name) {
    bool found = false;
    for (const auto& heap_ptr : ion_heaps) {
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
 * @brief Parse all ION heaps
 *
 * Walks the heap list and parses each heap's structure, extracting:
 *
 * This function also correlates heaps with buffers by matching heap addresses.
 */
void IonHeap::parser_heaps() {
    // Get list of all heap addresses
    std::vector<ulong> heaps = get_heaps();
    if (heaps.empty()) {
        LOGE("No heaps found");
        return;
    }
    LOGD("Parsing %zu ION heaps", heaps.size());
    for (const auto& addr : heaps) {
        LOGD("Parsing heap at address: 0x%lx", addr);
        // Read heap structure
        void *heap_buf = read_struct(addr, "ion_heap");
        if (!heap_buf) {
            LOGE("Failed to read ion_heap at 0x%lx, skipping", addr);
            continue;
        }
        // Create and populate heap object
        std::shared_ptr<ion_heap> heap_ptr = std::make_shared<ion_heap>();
        heap_ptr->addr = addr;

        // Parse heap type
        heap_ptr->type = (enum ion_heap_type)INT(heap_buf + field_offset(ion_heap, type));

        // Parse heap name
        ulong name_addr = ULONG(heap_buf + field_offset(ion_heap, name));
        if (is_kvaddr(name_addr)) {
            heap_ptr->name = read_cstring(name_addr, 64, "ion_heap_name");
        }

        // Parse heap ID and flags
        heap_ptr->id = UINT(heap_buf + field_offset(ion_heap, id));
        heap_ptr->flags = ULONG(heap_buf + field_offset(ion_heap, flags));

        // Parse operations name
        heap_ptr->ops = "";
        ulong ops_addr = ULONG(heap_buf + field_offset(ion_heap, ops));
        heap_ptr->ops = to_symbol(ops_addr);

        // Parse buffer count (if field exists)
        if (field_offset(ion_heap, num_of_buffers) != -1) {
            heap_ptr->buf_cnt = ULONG(heap_buf + field_offset(ion_heap, num_of_buffers));
        }

        // Parse total allocated bytes
        heap_ptr->total_allocated = ULONG(heap_buf + field_offset(ion_heap, num_of_alloc_bytes));
        FREEBUF(heap_buf);

        // Find all buffers allocated from this heap
        int buf_count = 0;
        for (const auto& dmabuf_ptr : dmabuf_ptr->buf_list) {
            if (dmabuf_ptr->heap == addr) {
                heap_ptr->bufs.push_back(dmabuf_ptr);
                buf_count++;
            }
        }
        ion_heaps.push_back(heap_ptr);
        // Print detailed heap information
        LOGD("========== Heap Information ==========");
        LOGD("Heap Address    : 0x%lx", heap_ptr->addr);
        LOGD("Heap Name       : %s", heap_ptr->name.c_str());
        LOGD("Heap ID         : %u", heap_ptr->id);
        LOGD("Heap Type       : %s (%d)", heap_type[heap_ptr->type].c_str(), heap_ptr->type);
        LOGD("Heap Flags      : 0x%lx", heap_ptr->flags);
        LOGD("Heap Ops        : %s", heap_ptr->ops.c_str());
        LOGD("Buffer Count    : %lu", heap_ptr->buf_cnt);
        LOGD("Total Allocated : %s (%lu bytes)", csize(heap_ptr->total_allocated).c_str(), heap_ptr->total_allocated);
        LOGD("Buffers Found   : %d", buf_count);
        LOGD("======================================");
    }
}

/**
 * @brief Get list of all ION heap addresses
 *
 * Automatically selects the appropriate method based on available kernel symbols:
 * - If "heaps" symbol exists: use get_ion_heaps_by_heaps() (newer ION)
 * - Otherwise: use get_ion_heaps_by_internal_dev() (older ION)
 *
 * @return Vector of heap addresses
 */
std::vector<ulong> IonHeap::get_heaps() {
    if (csymbol_exists("heaps")) {
        return get_ion_heaps_by_heaps();
    } else {
        return get_ion_heaps_by_internal_dev();
    }
}

/**
 * @brief Get ION heaps via internal_dev structure
 *
 * Walks the heap list from the internal_dev global variable.
 * This is the method used in older ION implementations where heaps
 * are maintained in a linked list within the ion_device structure.
 *
 * @return Vector of heap addresses
 */
std::vector<ulong> IonHeap::get_ion_heaps_by_internal_dev() {
    std::vector<ulong> heap_list;

    // Check if internal_dev symbol exists
    if (!csymbol_exists("internal_dev")) {
        LOGE("internal_dev symbol doesn't exist in this kernel");
        return heap_list;
    }

    // Read internal_dev pointer
    ulong internal_dev_addr = read_pointer(csymbol_value("internal_dev"), "internal_dev");
    if (!is_kvaddr(internal_dev_addr)) {
        LOGE("Invalid internal_dev address: 0x%lx", internal_dev_addr);
        return heap_list;
    }

    LOGD("Found internal_dev at address: 0x%lx", internal_dev_addr);

    // Walk the heap list
    ulong list_head = internal_dev_addr + field_offset(ion_device, heaps);
    if (!is_kvaddr(list_head)) {
        LOGE("Invalid heap list head: 0x%lx", list_head);
        return heap_list;
    }

    int offset = field_offset(ion_heap, node) + field_offset(plist_node, node_list);
    heap_list = for_each_list(list_head, offset);

    LOGD("Found %zu ION heaps via internal_dev", heap_list.size());
    return heap_list;
}

/**
 * @brief Get ION heaps via heaps array
 *
 * Reads the heaps array and num_heaps global variables.
 * This is the method used in newer ION implementations where heaps
 * are maintained in a simple array.
 *
 * @return Vector of heap addresses
 */
std::vector<ulong> IonHeap::get_ion_heaps_by_heaps() {
    std::vector<ulong> heap_list;

    // Check if heaps symbol exists
    if (!csymbol_exists("heaps")) {
        LOGE("heaps symbol doesn't exist in this kernel");
        return heap_list;
    }

    // Read number of heaps
    size_t num_heaps = read_int(csymbol_value("num_heaps"), "num_heaps");
    LOGD("Number of heaps: %zu", num_heaps);

    // Read heaps array pointer
    ulong heaps = read_pointer(csymbol_value("heaps"), "heaps");
    if (!is_kvaddr(heaps)) {
        LOGE("Invalid heaps array address: 0x%lx", heaps);
        return heap_list;
    }
    LOGD("Found heaps array at address: 0x%lx", heaps);

    // Read each heap pointer from the array
    for (size_t i = 0; i < num_heaps; i++) {
        ulong heap_addr = read_pointer(heaps + (i * sizeof(void *)), "heap");
        if (!is_kvaddr(heap_addr)) {
            LOGE("Heap %zu: invalid address 0x%lx, skipping", i, heap_addr);
            continue;
        }
        LOGD("Heap %zu at address: 0x%lx", i, heap_addr);
        heap_list.push_back(heap_addr);
    }
    LOGD("Found %zu ION heaps via heaps array", heap_list.size());
    return heap_list;
}

#pragma GCC diagnostic pop
