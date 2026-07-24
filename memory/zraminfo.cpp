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

#include "zraminfo.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

/**
 * @brief Main command handler (empty in base class)
 *
 * This is overridden by derived classes to implement command functionality.
 * Base class provides no implementation as it's meant to be extended.
 */
void Zraminfo::cmd_main(void) {

}

/**
 * @brief Initialize command information (empty in base class)
 *
 * This is overridden by derived classes to set up command metadata.
 * Base class provides no implementation as it's meant to be extended.
 */
void Zraminfo::init_command(void) {

}

/**
 * @brief Constructor - initializes ZRAM parser
 *
 * Initializes structure offsets, calculates flag bit positions based on
 * kernel version, and sets up ARM64-specific swap entry parameters if needed.
 *
 * The flag shift calculation is critical for correctly parsing ZRAM table entries:
 * - Kernel 6.1+: Uses dynamic page shift (PAGESHIFT() + 1)
 * - Older kernels: Uses fixed 24-bit shift
 *
 * Flag bits are used to identify special page types:
 * - SAME_BIT: Pages with identical content
 * - WB_BIT: Pages written back to backing device
 * - COMP_PRIORITY: Compression priority level
 */
Zraminfo::Zraminfo() {
    // Initialize all structure field offsets
    init_offset();

    // Calculate ZRAM flag shift based on kernel version
    // Kernel 6.1+ uses dynamic page shift, older versions use fixed 24-bit shift
    if (THIS_KERNEL_VERSION >= LINUX(6, 1, 0)) {
        ZRAM_FLAG_SHIFT = PAGESHIFT() + 1;
    } else {
        ZRAM_FLAG_SHIFT = 24;
    }

    // Calculate flag bit positions for different page states
    ZRAM_FLAG_SAME_BIT = 1 << (ZRAM_FLAG_SHIFT + 1);
    ZRAM_FLAG_WB_BIT = 1 << (ZRAM_FLAG_SHIFT + 2);
    ZRAM_COMP_PRIORITY_BIT1 = ZRAM_FLAG_SHIFT + 7;
    ZRAM_COMP_PRIORITY_MASK = 0x3;

    LOGD("Flag bits calculated:\n");
    LOGD("  ZRAM_FLAG_SAME_BIT = 0x%x\n", ZRAM_FLAG_SAME_BIT);
    LOGD("  ZRAM_FLAG_WB_BIT = 0x%x\n", ZRAM_FLAG_WB_BIT);
    LOGD("  ZRAM_COMP_PRIORITY_BIT1 = %d\n", ZRAM_COMP_PRIORITY_BIT1);

    // Calculate number of fullness groups for zspage organization
    group_cnt = field_size(size_class, fullness_list) / sizeof(struct kernel_list_head);
    LOGD("Fullness group count = %d\n", group_cnt);

#if defined(ARM64)
    // ARM64-specific swap entry configuration for kernel 6.10+
    // Reference: https://lore.kernel.org/all/20240503144604.151095-4-ryan.roberts@arm.com/
    struct machine_specific *ms = machdep->machspec;
    if (THIS_KERNEL_VERSION >= LINUX(6, 10, 0)) {
        ms->__SWP_TYPE_SHIFT = 6;
        ms->__SWP_TYPE_BITS = 5;
        ms->__SWP_TYPE_MASK = ((1 << ms->__SWP_TYPE_BITS) - 1);
        ms->__SWP_OFFSET_SHIFT = 12;
        ms->__SWP_OFFSET_BITS = 50;
        ms->__SWP_OFFSET_MASK = ((1UL << ms->__SWP_TYPE_BITS) - 1);
        ms->PTE_PROT_NONE = (1UL << 58);
        ms->PTE_FILE = 0;  /* unused */
    }
#endif

    LOGD("Zraminfo constructor completed successfully\n");
}

/**
 * @brief Initialize structure field offsets
 *
 * Initializes all field offsets for ZRAM-related kernel structures.
 * This must be called before any parsing operations to ensure correct
 * memory access to kernel structures.
 *
 * Structures initialized:
 * - zram: Main ZRAM device structure
 * - zs_pool: ZSmalloc memory pool
 * - size_class: Object size class
 * - zspage: ZSmalloc page
 * - zram_stats: ZRAM statistics
 * - gendisk: Generic disk structure
 * - zcomp: Compression backend
 * - link_free: Free object link
 * - idr: ID-to-pointer mapping
 */
void Zraminfo::init_offset(void) {
    // Initialize ZRAM device structure fields
    field_init(zram, table);
    field_init(zram, mem_pool);
    field_init(zram, comp);
    field_init(zram, comps);
    field_init(zram, disk);
    field_init(zram, limit_pages);
    field_init(zram, stats);
    field_init(zram, disksize);
    field_init(zram, compressor);
    field_init(zram, comp_algs);
    field_init(zram, claim);
    struct_init(zram);

    // Initialize zs_pool structure fields
    field_init(zs_pool, name);
    field_init(zs_pool, size_class);
    field_init(zs_pool, pages_allocated);
    field_init(zs_pool, stats);
    field_init(zs_pool, isolated_pages);
    field_init(zs_pool, destroying);
    struct_init(zs_pool);

    // Initialize size_class structure fields
    field_init(size_class, fullness_list);
    field_init(size_class, size);
    field_init(size_class, objs_per_zspage);
    field_init(size_class, pages_per_zspage);
    field_init(size_class, index);
    field_init(size_class, stats);
    struct_init(size_class);

    // Initialize zspage structure fields
    field_init(zspage, inuse);
    field_init(zspage, freeobj);
    field_init(zspage, first_page);
    field_init(zspage, list);
    field_init(zspage, huge);
    struct_init(zspage);

    // Initialize zs_size_stat structure fields
    field_init(zs_size_stat, objs);

    // Initialize zram_stats structure fields
    field_init(zram_stats, compr_data_size);
    field_init(zram_stats, num_reads);
    field_init(zram_stats, num_writes);
    field_init(zram_stats, failed_reads);
    field_init(zram_stats, failed_writes);
    field_init(zram_stats, invalid_io);
    field_init(zram_stats, notify_free);
    field_init(zram_stats, same_pages);
    field_init(zram_stats, huge_pages);
    field_init(zram_stats, huge_pages_since);
    field_init(zram_stats, pages_stored);
    field_init(zram_stats, max_used_pages);
    field_init(zram_stats, writestall);
    field_init(zram_stats, miss_free);
    struct_init(zram_stats);

    // Initialize remaining structure fields
    field_init(gendisk, disk_name);
    field_init(zcomp, name);
    field_init(link_free, handle);
    field_init(idr, idr_rt);
}

/**
 * @brief Check if ZRAM is enabled in the kernel
 *
 * Verifies that:
 * 1. Required structure offsets are available (zram.ko and zsmalloc.ko loaded)
 * 2. zram_index_idr symbol exists in the kernel
 *
 * @return true if ZRAM is enabled and accessible, false otherwise
 */
bool Zraminfo::is_zram_enable() {
    // Check if required structure offsets are available
    if (field_offset(zram, mem_pool) == -1 || field_offset(size_class, size) == -1) {
        LOGD("structure offsets not available,Please run mod -s zram.ko and zsmalloc.ko at first, then reload the plugins\n");
        return false;
    }

    // Check if zram_index_idr symbol exists
    if (!csymbol_exists("zram_index_idr")) {
        LOGD("zram_index_idr symbol not found\n");
        return false;
    }
    LOGD("ZRAM is enabled and accessible\n");
    return true;
}

/**
 * @brief Parse all ZRAM devices in the system
 *
 * Iterates through the zram_index_idr xarray to find and parse all
 * registered ZRAM devices. Each device is parsed and added to zram_list.
 */
void Zraminfo::parser_zrams() {
    LOGD("Starting to parse all ZRAM devices\n");
    ulong zram_idr_addr = csymbol_value("zram_index_idr");
    LOGD("zram_index_idr address = 0x%lx\n", zram_idr_addr);

    if (!is_kvaddr(zram_idr_addr)) {
        LOGE("Invalid zram_index_idr address, aborting\n");
        return;
    }

    ulong xarray_addr = zram_idr_addr + field_offset(idr, idr_rt);
    for (const auto& addr : for_each_xarray(xarray_addr)) {
        parser_zram(addr);
    }
}

/**
 * @brief Read a ZRAM table entry
 *
 * Reads the table entry at the specified index from the ZRAM device's
 * page table. The table entry contains the handle to the compressed data
 * and flags indicating the page state.
 *
 * @param zram_ptr Pointer to ZRAM device structure
 * @param index Index of the table entry to read
 * @param entry Output parameter for the table entry
 * @return true if read successful, false otherwise
 */
bool Zraminfo::read_table_entry(std::shared_ptr<zram> zram_ptr, ulonglong index,
                                struct zram_table_entry* entry) {
    LOGD("Reading ZRAM table entry at index %lld\n", index);
    ulong table_entry_addr = zram_ptr->table + index * sizeof(zram_table_entry);
    LOGD("zram_table_entry address=0x%lx\n", table_entry_addr);

    if (!read_struct(table_entry_addr, entry, sizeof(zram_table_entry), "zram_table_entry")) {
        LOGE("read zram_table_entry fail at %lx\n", table_entry_addr);
        return false;
    }
    LOGD("zram_table_entry handle=0x%lx, flags=0x%lx\n", entry->handle, entry->flags);
    return true;
}

/**
 * @brief Get size class ID from zspage structure
 *
 * Extracts the class ID from a zspage structure, handling different
 * kernel versions that have different structure layouts.
 *
 * Kernel version handling:
 * - >= 6.6.0: Uses v6_6 layout
 * - >= 5.17.0: Uses v5_17 layout
 * - < 5.17.0: Uses v0 layout
 *
 * @param zspage_s Reference to zspage structure
 * @return Size class ID
 */
int Zraminfo::get_class_id(struct zspage& zspage_s) {
    int class_idx = 0;
    if (field_offset(zspage, huge) != -1) {
        if (THIS_KERNEL_VERSION >= LINUX(6, 6, 0)) {
            class_idx = zspage_s.v6_6.class_id;
            LOGD("Using v6_6 layout, class_id =%d\n", class_idx);
        } else {
            class_idx = zspage_s.v5_17.class_id;
            LOGD("Using v5_17 layout, class_id =%d\n", class_idx);
        }
    } else {
        class_idx = zspage_s.v0.class_id;
        LOGD("Using v0 layout, class_id =%d\n", class_idx);
    }
    return class_idx;
}

/**
 * @brief Get zspage structure from a page
 *
 * Retrieves and validates the zspage structure associated with a given page.
 * The zspage address is stored in the page's private field.
 *
 * Validation includes:
 * 1. Checking if zspage address is valid kernel address
 * 2. Reading the zspage structure
 * 3. Verifying the magic number (ZSPAGE_MAGIC)
 *
 * @param page Page address
 * @param zp Output parameter for zspage structure
 * @return true if zspage is valid, false otherwise
 */
bool Zraminfo::get_zspage(ulong page, struct zspage* zp) {
    LOGD("Getting zspage from page 0x%lx\n", page);
    int zs_magic = 0;

    // Find the zspage address from page's private field
    ulong zspage_addr = read_ulong(page + field_offset(page, private), "page private");
    LOGD("zspage address=0x%lx\n", zspage_addr);
    if (!is_kvaddr(zspage_addr)) {
        LOGD("Invalid zspage address: 0x%lx\n", zspage_addr);
        return false;
    }

    // Read the zspage structure
    if (!read_struct(zspage_addr, zp, sizeof(struct zspage), "zspage")) {
        LOGD("Failed to read zspage structure at 0x%lx\n", zspage_addr);
        return false;
    }

    // Extract and verify magic number based on kernel version
    if (field_offset(zspage, huge) != -1) {
        if (THIS_KERNEL_VERSION >= LINUX(6, 12, 0)) {
            zs_magic = zp->v6_12.magic;
        } else if (THIS_KERNEL_VERSION >= LINUX(6, 6, 0)) {
            zs_magic = zp->v6_6.magic;
        } else {
            zs_magic = zp->v5_17.magic;
        }
    } else {
        zs_magic = zp->v0.magic;
    }

    if (zs_magic != ZSPAGE_MAGIC) {
        LOGD("Invalid zspage magic: 0x%x (expected 0x%x)\n",
                    zs_magic, ZSPAGE_MAGIC);
        return false;
    }
    LOGD("zspage validated successfully (magic=0x%x)\n", zs_magic);
    return true;
}

/**
 * @brief Read a compressed object from memory
 *
 * Reads a compressed object from the zsmalloc pool based on the table entry.
 * The process involves:
 * 1. Extracting PFN and object index from handle
 * 2. Finding the page and zspage
 * 3. Determining object size from size class
 * 4. Reading object data (may span multiple pages)
 * 5. Validating handle for non-huge objects
 *
 * @param zram_ptr Pointer to ZRAM device
 * @param entry Table entry containing handle and flags
 * @param read_len Output parameter for bytes read
 * @param huge_obj Output parameter indicating if object is huge
 * @return Pointer to allocated buffer containing object data, or nullptr on failure
 */
char* Zraminfo::read_object(std::shared_ptr<zram> zram_ptr, struct zram_table_entry entry,
                            int& read_len, bool& huge_obj) {
    ulong pfn = 0;
    int obj_idx = 0;
    if (!is_kvaddr(entry.handle)) {
        LOGE("Invalid handle address: 0x%lx\n", entry.handle);
        return nullptr;
    }
    // Read the actual handle value
    ulong handle = read_ulong(entry.handle, "handle");
    LOGD("Read handle value: 0x%lx at 0x%lx\n", handle, entry.handle);
    handle_to_location(handle, &pfn, &obj_idx);
    // Find the page address from PFN
    ulong page = pfn_to_page(pfn);
    if (!is_kvaddr(page)) {
        LOGE("Invalid page address: 0x%lx\n", page);
        return nullptr;
    }
    LOGD("Page address=0x%lx\n", page);
    // Get and validate zspage
    struct zspage zspage_s;
    if (!get_zspage(page, &zspage_s)) {
        LOGE("Failed to get valid zspage\n");
        return nullptr;
    }
    // Get size class to determine object size
    int class_idx = get_class_id(zspage_s);
    std::shared_ptr<size_class> class_ptr = zram_ptr->mem_pool->class_list[class_idx];
    size_t obj_size = class_ptr->size;
    LOGD("Object size=%zd bytes\n", obj_size);

    // Allocate buffer for object data
    char* obj_buf = (char*)std::malloc(obj_size);
    BZERO(obj_buf, obj_size);

    physaddr_t paddr;
    void* tmpbuf;
    size_t offset = (obj_size * obj_idx) & (page_size - 1);
    LOGD("Object offset=%zd in page:0x%lx\n", offset, page);

    // Read object data (may span one or two pages)
    if (offset + obj_size <= page_size) {
        // Object fits in one page
        paddr = page_to_phy(page);
        if (!paddr) {
            LOGE("Can't convert to physaddr of page:%lx\n", page);
            std::free(obj_buf);
            return nullptr;
        }
        LOGD("Object:[0x%llx~0x%llx]\n",
                    (ulonglong)(paddr + offset), (ulonglong)(paddr + offset + obj_size));
        tmpbuf = read_memory(paddr + offset, obj_size, "zram obj", false);
        memcpy(obj_buf, tmpbuf, obj_size);
        FREEBUF(tmpbuf);
    } else {
        // Object spans two pages
        LOGD("Object spans two pages\n");
        ulong pages[2], sizes[2];
        pages[0] = page;

        // Get second page address
        if (field_offset(page, freelist) != -1) {
            pages[1] = read_pointer(page + field_offset(page, freelist), "page freelist");
        } else {
            pages[1] = read_pointer(page + field_offset(page, index), "page index");
        }
        sizes[0] = page_size - offset;
        sizes[1] = obj_size - sizes[0];
        // Read first part
        paddr = page_to_phy(pages[0]);
        if (!paddr) {
            LOGE("Can't convert to physaddr of page:%lx\n", pages[0]);
            std::free(obj_buf);
            return nullptr;
        }
        LOGD("Object Part0:[0x%llx~0x%llx]\n",(ulonglong)(paddr + offset), (ulonglong)(paddr + offset + sizes[0]));
        tmpbuf = read_memory(paddr + offset, sizes[0], "zram obj part0", false);
        memcpy(obj_buf, tmpbuf, sizes[0]);
        FREEBUF(tmpbuf);

        // Read second part
        paddr = page_to_phy(pages[1]);
        if (!paddr) {
            LOGE("Can't convert to physaddr of page:%lx\n", pages[1]);
            std::free(obj_buf);
            return nullptr;
        }
        LOGD("Object Part1:[0x%llx~0x%llx]\n",(ulonglong)paddr, (ulonglong)(paddr + sizes[1]));
        tmpbuf = read_memory(paddr, sizes[1], "zram obj part1", false);
        memcpy(obj_buf + sizes[0], tmpbuf, sizes[1]);
        FREEBUF(tmpbuf);
    }
    // Validate handle for non-huge objects
    if (!(class_ptr->objs_per_zspage == 1 && class_ptr->pages_per_zspage == 1)) {
        // Not a huge object - validate handle
        ulong handle_addr = ULONG(obj_buf);
        // Handle address in object has bit0 set, so clear it
        handle_addr = handle_addr & ~1;
        LOGD("Handle validation:\n");
        LOGD("  Handle in table: 0x%lx\n", entry.handle);
        LOGD("  Handle in object: 0x%lx\n", handle_addr);
        if (entry.handle != handle_addr) {
            LOGE("Handle mismatch detected\n");
            std::free(obj_buf);
            return nullptr;
        }
        huge_obj = false;
        LOGD("Normal object validated\n");
    } else {
        // Huge object
        huge_obj = true;
        LOGD("Huge object detected\n");
    }
    read_len = obj_size;
    LOGD("Object read successfully, size=%d bytes\n", read_len);
    return obj_buf;
}

/**
 * @brief Read and decompress a ZRAM page
 *
 * Reads a page from ZRAM device at the specified index and decompresses it.
 * Handles different page types:
 * - ZRAM_SAME: Pages with identical content (filled with single value)
 * - Compressed: Pages compressed with configured algorithm
 * - Uncompressed: Pages stored without compression
 *
 * @param zram_addr ZRAM device address
 * @param index Page index in ZRAM device
 * @return Pointer to decompressed page data, or nullptr on failure
 */
char* Zraminfo::read_zram_page(ulong zram_addr, ulonglong index) {
    if (!is_zram_enable()) {
        LOGE("ZRAM not enabled, aborting\n");
        return nullptr;
    }
    // Find or parse ZRAM device
    std::shared_ptr<zram> zram_ptr;
    bool is_found = false;
    for (const auto &ptr : zram_list) {
        if (ptr->addr == zram_addr) {
            is_found = true;
            zram_ptr = ptr;
            break;
        }
    }
    if (is_found == false) {
        LOGD("zram not in cache, parsing now\n");
        zram_ptr = parser_zram(zram_addr);
    }
    if (zram_ptr == nullptr) {
        LOGE("Failed to parse ZRAM device:%lx\n",zram_addr);
        return nullptr;
    }
    // Validate index
    ulonglong total_pages = zram_ptr->disksize >> PAGESHIFT();
    LOGD("ZRAM device: disksize=%lld, total_pages=%lld\n",zram_ptr->disksize, total_pages);

    if (index > total_pages) {
        LOGE("Index %lld exceeds max %lld\n", index, total_pages);
        return nullptr;
    }
    // Read table entry
    struct zram_table_entry entry;
    if (!read_table_entry(zram_ptr, index, &entry)) {
        return nullptr;
    }

    // Allocate page buffer
    char* page_data = (char *)GETBUF(page_size);
    BZERO(page_data, page_size);

    // Handle ZRAM_SAME pages (pages with identical content)
    if (entry.flags & ZRAM_FLAG_SAME_BIT) {
        unsigned long val = entry.handle ? entry.element : 0;
        LOGD("ZRAM_SAME page detected, Filling page with value 0x%lx\n", val);
        memset(page_data, val, page_size / sizeof(unsigned long));
        return page_data;
    } else {
        // Handle compressed/uncompressed pages
        bool is_huge = false;
        int read_len = 0;
        size_t comp_len = entry.flags & ((1 << ZRAM_FLAG_SHIFT) - 1);
        LOGD("Compressed page: comp_len=%zd bytes\n", comp_len);
        char *obj_data = read_object(zram_ptr, entry, read_len, is_huge);
        if (obj_data == nullptr) {
            LOGE("Failed to read object\n");
            FREEBUF(page_data);
            return nullptr;
        }
        // Determine data pointer (skip handle for non-huge objects)
        char* data_ptr;
        if (is_huge) {
            data_ptr = obj_data;
            LOGD("Huge object, using data directly\n");
        } else {
            data_ptr = obj_data + ZS_HANDLE_SIZE;
            LOGD("Normal object, skipping %d byte handle\n", ZS_HANDLE_SIZE);
        }
        // Decompress or copy data
        if (comp_len == page_size) {
            LOGD("Uncompressed page, copying directly\n");
            memcpy(page_data, data_ptr, page_size);
        } else {
            std::string compress_name = (zram_ptr->compressor.empty() ?
                                        zram_ptr->zcomp_name : zram_ptr->compressor);
            LOGD("Decompressing with algorithm: %s, compressed size: %zd, target size: %lu\n", compress_name.c_str(),comp_len, page_size);
            decompress(compress_name, data_ptr, page_data, comp_len, page_size);
        }
        std::free(obj_data);
        return page_data;
    }
}

/**
 * @brief Decompress data using specified compression algorithm
 *
 * Supports multiple compression algorithms:
 * - LZ4: Fast compression with good ratio
 * - LZO: Lightweight compression
 *
 * @param comp_name Compression algorithm name
 * @param source Source compressed data buffer
 * @param dest Destination buffer for decompressed data
 * @param compressedSize Size of compressed data
 * @param maxDecompressedSize Maximum size of decompressed data
 * @return 0 on success, error code otherwise
 */
int Zraminfo::decompress(std::string comp_name, char* source, char* dest,
                         int compressedSize, int maxDecompressedSize) {
    if (comp_name.find("lz4") != std::string::npos) {
        LOGD("Using LZ4 decompression\n");
        try {
            lz4_decompress(source, dest, compressedSize, maxDecompressedSize);
            LOGD("LZ4 decompression successful\n");
        } catch(const std::exception& e) {
            LOGE("Exception in zram lz4: %s \n", e.what());
        }
    } else if (comp_name.find("lzo") != std::string::npos) {
        LOGD("Using LZO decompression\n");
        try {
            lzo1x_decompress(source, dest, compressedSize, maxDecompressedSize);
            LOGD("LZO decompression successful\n");
        } catch(const std::exception& e) {
            LOGE("Exception in zram lzo: %s \n", e.what());
        }
    } else {
        LOGD("Unsupported compression algorithm: %s\n", comp_name.c_str());
    }
    return 0;
}

/**
 * @brief Decompress LZO compressed data
 *
 * Uses the LZO1X algorithm for decompression. LZO provides fast
 * decompression speed with reasonable compression ratios.
 *
 * @param source Source compressed data buffer
 * @param dest Destination buffer for decompressed data
 * @param compressedSize Size of compressed data
 * @param maxDecompressedSize Maximum size of decompressed data
 * @return 0 on success, error code otherwise
 */
int Zraminfo::lzo1x_decompress(char *source, char *dest,
                               int compressedSize, int maxDecompressedSize) {
    size_t tmp_len = maxDecompressedSize;
    int result = lzo1x_decompress_safe((unsigned char*)source, compressedSize,
                                       (unsigned char*)dest, &tmp_len);
    return result;
}

/**
 * @brief Decompress LZ4 compressed data
 *
 * Uses the LZ4 algorithm for decompression. LZ4 provides extremely fast
 * decompression speed, making it ideal for ZRAM use cases.
 *
 * @param source Source compressed data buffer
 * @param dest Destination buffer for decompressed data
 * @param compressedSize Size of compressed data
 * @param maxDecompressedSize Maximum size of decompressed data
 * @return Decompressed size on success, negative value on error
 */
int Zraminfo::lz4_decompress(char *source, char *dest,
                             int compressedSize, int maxDecompressedSize) {
    int result = LZ4_decompress_safe((const char *)source, (char *)dest,
                                     compressedSize, maxDecompressedSize);
    return result;
}

/**
 * @brief Convert object handle to PFN and object index
 *
 * Decodes a zsmalloc object handle into its components:
 * - PFN (Page Frame Number): Identifies the physical page
 * - Object index: Position within the page
 *
 * Handle format: [PFN][OBJ_INDEX][TAG_BITS]
 *
 * @param handle Object handle to decode
 * @param pfn Output parameter for page frame number
 * @param obj_idx Output parameter for object index
 */
void Zraminfo::handle_to_location(ulong handle, ulong* pfn, int* obj_idx) {
    // Remove tag bits
    handle >>= OBJ_TAG_BITS;
    // Extract PFN and object index
    *pfn = handle >> OBJ_INDEX_BITS;
    *obj_idx = (handle & OBJ_INDEX_MASK);
    LOGD("Decoded: pfn=0x%lx, obj_idx=%d\n", *pfn, *obj_idx);
}

/**
 * @brief Parse object information from handle address
 *
 * Creates a zobj structure containing object metadata including:
 * - Physical address range
 * - Allocation status (free or allocated)
 * - Handle information
 * - PFN and index (for allocated objects)
 * - Next free object (for free objects)
 *
 * @param obj_id Object ID within the page
 * @param handle_addr Address of the object handle
 * @param start Start physical address
 * @param end End physical address
 * @return Shared pointer to parsed zobj structure
 */
std::shared_ptr<zobj> Zraminfo::parser_obj(int obj_id, ulong handle_addr,
                                           physaddr_t start, physaddr_t end) {
    LOGD("Parsing object: id = %d, handle_addr:0x%lx, range=0x%llx-0x%llx\n",
                obj_id, handle_addr, (ulonglong)start, (ulonglong)end);
    std::shared_ptr<zobj> obj_ptr = std::make_shared<zobj>();
    obj_ptr->start = start;
    obj_ptr->end = end;
    obj_ptr->id = obj_id;

    // Check if object is allocated (bit 0 set)
    if (handle_addr & OBJ_ALLOCATED_TAG) {
        // Object is allocated
        obj_ptr->is_free = false;
        obj_ptr->handle_addr = handle_addr & ~OBJ_TAG_BITS; // Clear tag bits
        ulong handle = read_ulong(obj_ptr->handle_addr, "obj handle");
        handle_to_location(handle, &obj_ptr->pfn, &obj_ptr->index);

        LOGD("Allocated object: handle=0x%lx, pfn=0x%lx, index=%d\n",
                    handle, obj_ptr->pfn, obj_ptr->index);
    } else {
        // Object is free
        obj_ptr->is_free = true;
        obj_ptr->handle_addr = handle_addr;
        obj_ptr->next = handle_addr >> OBJ_TAG_BITS;
        LOGD("Free object: next=%d\n", obj_ptr->next);
    }

    return obj_ptr;
}

/**
 * @brief Parse all objects in a page
 *
 * Iterates through all objects in a page and parses their metadata.
 * Handles two cases:
 * 1. Huge objects: Single object per page
 * 2. Normal objects: Multiple objects per page with offset
 *
 * Objects may span page boundaries, requiring special handling.
 *
 * @param page_addr Page address
 * @param class_ptr Pointer to size class
 * @param zspage_ptr Pointer to zspage structure
 */
void Zraminfo::parser_obj(ulong page_addr, std::shared_ptr<size_class> class_ptr,
                          std::shared_ptr<zpage> zspage_ptr) {
    void* buf;
    int offset = read_int(page_addr + field_offset(page, units), "page units");
    physaddr_t page_start = page_to_phy(page_addr);
    physaddr_t page_end = page_start + page_size;
    LOGD("Page range: 0x%llx-0x%llx, offset = %d\n",(ulonglong)page_start, (ulonglong)page_end, offset);
    std::shared_ptr<pageinfo> page_ptr = std::make_shared<pageinfo>();
    page_ptr->addr = page_addr;
    ulong handle_addr = 0;
    // Handle huge objects (one object per page)
    if (class_ptr->objs_per_zspage == 1 && class_ptr->pages_per_zspage == 1) {
        LOGD("Huge object page\n");
        handle_addr = read_ulong(page_addr + field_offset(page, index), "page index");
        std::shared_ptr<zobj> zsobj = parser_obj(zspage_ptr->obj_index, handle_addr,
                                                 page_start, page_end);
        page_ptr->obj_list.push_back(zsobj);
        zspage_ptr->obj_index += 1;
    } else {
        // Handle normal objects (multiple per page)
        LOGD("Normal object page, obj_size = %d\n", class_ptr->size);
        physaddr_t obj_start = page_start + offset;
        physaddr_t obj_end = obj_start + class_ptr->size;
        // Parse all complete objects in the page
        while (obj_end < page_end) {
            buf = read_memory(obj_start + field_offset(link_free, handle), sizeof(unsigned long), "link_free handle", false);
            if (buf == nullptr){
                continue;
            }
            handle_addr = ULONG(buf);
            FREEBUF(buf);
            std::shared_ptr<zobj> zsobj = parser_obj(zspage_ptr->obj_index, handle_addr, obj_start, obj_end);
            page_ptr->obj_list.push_back(zsobj);
            zspage_ptr->obj_index += 1;

            obj_start = obj_end;
            obj_end += class_ptr->size;
        }
        // Parse partial last object
        obj_end = page_end;
        buf = read_memory(obj_start + field_offset(link_free, handle), sizeof(unsigned long), "link_free handle", false);
        if (buf != nullptr){
            handle_addr = ULONG(buf);
            FREEBUF(buf);
        }
        std::shared_ptr<zobj> zsobj = parser_obj(zspage_ptr->obj_index, handle_addr, obj_start, obj_end);
        page_ptr->obj_list.push_back(zsobj);
        zspage_ptr->obj_index += 1;
        LOGD("Parsed %zu objects in page\n", page_ptr->obj_list.size());
    }
    zspage_ptr->page_list.push_back(page_ptr);
}

/**
 * @brief Parse all pages in a zspage
 *
 * Iterates through the linked list of pages in a zspage and parses
 * each page's objects. Pages are linked via the freelist field.
 *
 * @param first_page Address of the first page
 * @param class_ptr Pointer to size class
 * @param zspage_ptr Pointer to zspage structure
 */
void Zraminfo::parser_pages(ulong first_page, std::shared_ptr<size_class> class_ptr,
                           std::shared_ptr<zpage> zspage_ptr) {
    ulong page_addr = first_page;
    int page_count = 0;
    while (is_kvaddr(page_addr)) {
        LOGD("Parsing page #%d at 0x%lx\n", page_count, page_addr);
        parser_obj(page_addr, class_ptr, zspage_ptr);
        page_count += 1;
        if (page_count == class_ptr->pages_per_zspage) {
            LOGD("Reached expected page count, stopping\n");
            break;
        }
        page_addr = read_ulong(page_addr + field_offset(page, freelist), "page freelist");
    }
}

/**
 * @brief Parse a zspage structure
 *
 * Reads and parses a zspage structure, including all its pages and objects.
 * A zspage is a collection of pages managed as a single unit by zsmalloc.
 *
 * @param addr ZSpage address
 * @param class_ptr Pointer to parent size class
 * @return Shared pointer to parsed zpage structure, or nullptr on failure
 */
std::shared_ptr<zpage> Zraminfo::parser_zpage(ulong addr, std::shared_ptr<size_class> class_ptr) {
    LOGD("Parsing zspage at 0x%lx\n", addr);
    if (!is_kvaddr(addr)) {
        LOGE("Invalid zspage address\n");
        return nullptr;
    }

    void *zspage_buf = read_struct(addr, "zspage");
    if (zspage_buf == nullptr) {
        LOGE("Failed to read zspage structure\n");
        return nullptr;
    }

    std::shared_ptr<zpage> zspage_ptr = std::make_shared<zpage>();
    zspage_ptr->addr = addr;
    zspage_ptr->obj_index = 0;
    zspage_ptr->zspage.flag_bits = UINT(zspage_buf);
    zspage_ptr->zspage.inuse = UINT(zspage_buf + field_offset(zspage, inuse));
    zspage_ptr->zspage.freeobj = UINT(zspage_buf + field_offset(zspage, freeobj));
    ulong first_page = ULONG(zspage_buf + field_offset(zspage, first_page));
    FREEBUF(zspage_buf);
    parser_pages(first_page, class_ptr, zspage_ptr);
    return zspage_ptr;
}

/**
 * @brief Parse a size class structure
 *
 * Reads and parses a size class structure. A size class defines:
 * - Object size
 * - Number of objects per zspage
 * - Number of pages per zspage
 * - Statistics
 *
 * @param addr Size class address
 * @return Shared pointer to parsed size_class structure, or nullptr on failure
 */
std::shared_ptr<size_class> Zraminfo::parser_size_class(ulong addr) {
    LOGD("Parsing size_class at 0x%lx\n", addr);
    if (!is_kvaddr(addr)) {
        LOGE("Invalid size_class address\n");
        return nullptr;
    }
    void *class_buf = read_struct(addr, "size_class");
    if (class_buf == nullptr) {
        LOGE("Failed to read size_class structure\n");
        return nullptr;
    }
    std::shared_ptr<size_class> class_ptr = std::make_shared<size_class>();
    class_ptr->addr = addr;
    class_ptr->size = INT(class_buf + field_offset(size_class, size));
    class_ptr->objs_per_zspage = INT(class_buf + field_offset(size_class, objs_per_zspage));
    class_ptr->pages_per_zspage = INT(class_buf + field_offset(size_class, pages_per_zspage));
    class_ptr->index = UINT(class_buf + field_offset(size_class, index));
    class_ptr->zspage_parser = false;
    FREEBUF(class_buf);

    // Read statistics
    int stats_cnt = field_size(zs_size_stat, objs) / sizeof(unsigned long);
    ulong stats_addr = addr + field_offset(size_class, stats);
    for (int i = 0; i < stats_cnt; i++) {
        ulong stat_val = read_ulong(stats_addr + i * sizeof(unsigned long), "size_class_stats");
        class_ptr->stats.push_back(stat_val);
    }
    return class_ptr;
}

/**
 * @brief Parse all zspages in a size class
 *
 * Iterates through all fullness groups in a size class and parses
 * each zspage. Fullness groups organize zspages by how full they are:
 * - EMPTY: No objects allocated
 * - ALMOST_EMPTY: Few objects allocated
 * - ALMOST_FULL: Most objects allocated
 * - FULL: All objects allocated
 *
 * @param class_ptr Pointer to size class
 */
void Zraminfo::parser_zpage(std::shared_ptr<size_class> class_ptr) {
    LOGD("Parsing zspages for size_class 0x%lx\n", class_ptr->addr);
    LOGD("Number of fullness groups: %d\n", group_cnt);

    for (int i = 0; i < group_cnt; i++) {
        ulong group_addr = class_ptr->addr + field_offset(size_class, fullness_list) +
                          i * sizeof(struct kernel_list_head);
        int offset = field_offset(zspage, list);
        LOGD("Processing fullness group#%d at 0x%lx\n", i, group_addr);

        std::vector<std::shared_ptr<zpage>> zspage_list;
        int zspage_count = 0;

        for (const auto& zspage_addr : for_each_list(group_addr, offset)) {
            LOGD("Parsing zspage#%d in group#%d\n", zspage_count++, i);
            zspage_list.push_back(parser_zpage(zspage_addr, class_ptr));
        }
        LOGD("Group#%d contains %zu zspages\n", i, zspage_list.size());

        class_ptr->fullness_list.push_back(zspage_list);
    }

    class_ptr->zspage_parser = true;
    LOGD("All zspages parsed for size_class\n");
}

/**
 * @brief Parse a zsmalloc memory pool
 *
 * Reads and parses a zs_pool structure, including all its size classes.
 * The memory pool manages multiple size classes for efficient allocation.
 *
 * @param addr Memory pool address
 * @return Shared pointer to parsed zs_pool structure, or nullptr on failure
 */
std::shared_ptr<zs_pool> Zraminfo::parser_mem_pool(ulong addr) {
    LOGD("Parsing zs_pool at 0x%lx\n", addr);
    if (!is_kvaddr(addr)) {
        LOGE("Invalid zs_pool address\n");
        return nullptr;
    }
    void *pool_buf = read_struct(addr, "zs_pool");
    if (pool_buf == nullptr) {
        LOGE("Failed to read zs_pool structure\n");
        return nullptr;
    }
    std::shared_ptr<zs_pool> pool_ptr = std::make_shared<zs_pool>();
    pool_ptr->addr = addr;
    pool_ptr->name = read_cstring(ULONG(pool_buf + field_offset(zs_pool, name)), 64, "pool_name");
    pool_ptr->pages_allocated = INT(pool_buf + field_offset(zs_pool, pages_allocated));
    if (field_offset(zs_pool, isolated_pages) != -1) {
        pool_ptr->isolated_pages = INT(pool_buf + field_offset(zs_pool, isolated_pages));
    }
    if (field_offset(zs_pool, destroying) != -1) {
        pool_ptr->destroying = BOOL(pool_buf + field_offset(zs_pool, destroying));
    }
    read_struct(addr + field_offset(zs_pool, stats), &pool_ptr->stats, sizeof(struct zs_pool_stats), "zs_pool_stats");
    // Parse all size classes
    int class_cnt = field_size(zs_pool, size_class) / sizeof(void *);
    for (int i = 0; i < class_cnt; i++) {
        ulong class_addr = read_pointer(addr + field_offset(zs_pool, size_class) + i * sizeof(void *), "size_class addr");
        if (!is_kvaddr(class_addr)) {
            continue;
        }
        pool_ptr->class_list.push_back(parser_size_class(class_addr));
    }
    FREEBUF(pool_buf);
    return pool_ptr;
}

/**
 * @brief Parse a ZRAM device structure
 *
 * Reads and parses a complete ZRAM device, including:
 * - Device configuration (disk name, size, limits)
 * - Compression settings
 * - Memory pool
 * - Statistics
 *
 * The parsed device is added to the global zram_list.
 *
 * @param addr ZRAM device address
 * @return Shared pointer to parsed zram structure, or nullptr on failure
 */
std::shared_ptr<zram> Zraminfo::parser_zram(ulong addr) {
    LOGD("Parsing struct zram at 0x%lx\n", addr);
    void *zram_buf = read_struct(addr, "zram");
    if (zram_buf == nullptr) {
        LOGE("Failed to read zram structure\n");
        return nullptr;
    }
    ulong pool_addr = ULONG(zram_buf + field_offset(zram, mem_pool));
    if (!is_kvaddr(pool_addr)) {
        LOGE("Invalid memory pool address: 0x%lx\n", pool_addr);
        FREEBUF(zram_buf);
        return nullptr;
    }
    std::shared_ptr<zram> zram_ptr = std::make_shared<zram>();
    zram_ptr->addr = addr;
    zram_ptr->table = ULONG(zram_buf + field_offset(zram, table));
    // Parse memory pool
    zram_ptr->mem_pool = parser_mem_pool(ULONG(zram_buf + field_offset(zram, mem_pool)));
    // Parse compression backend (kernel version dependent)
    if (field_offset(zram, comp) != -1) {
        ulong zcomp_addr = ULONG(zram_buf + field_offset(zram, comp));
        if (is_kvaddr(zcomp_addr)) {
            ulong zcomp_name_addr = read_pointer(zcomp_addr + field_offset(zcomp, name), "zcomp_name_addr");
            if (is_kvaddr(zcomp_name_addr)) {
                zram_ptr->zcomp_name = read_cstring(zcomp_name_addr, 64, "zcomp_name");
            }
        }
    } else if (field_offset(zram, comps) != -1) {
        ulong zcomp_addr = ULONG(zram_buf + field_offset(zram, comps));
        if (is_kvaddr(zcomp_addr)) {
            ulong zcomp_name_addr = read_pointer(zcomp_addr + field_offset(zcomp, name), "zcomp_name_addr");
            if (is_kvaddr(zcomp_name_addr)) {
                zram_ptr->zcomp_name = read_cstring(zcomp_name_addr, 64, "zcomp_name");
            }
        }
    }
    // Read disk name
    ulong disk_name_addr = ULONG(zram_buf + field_offset(zram, disk)) + field_offset(gendisk, disk_name);
    zram_ptr->disk_name = read_cstring(disk_name_addr, 32, "disk_name");
    // Read compressor name (kernel version dependent)
    if (field_offset(zram, compressor) != -1) {
        char compressor_name[128];
        memcpy(&compressor_name, (void *)zram_buf + field_offset(zram, compressor), 128);
        zram_ptr->compressor = extract_string(compressor_name);
    } else if (field_offset(zram, comp_algs) != -1) {
        ulong name_addr = ULONG(zram_buf + field_offset(zram, comp_algs));
        if (is_kvaddr(name_addr)) {
            zram_ptr->compressor = read_cstring(name_addr, 64, "compressor name");
        }
    }
    // Read device configuration
    zram_ptr->limit_pages = ULONG(zram_buf + field_offset(zram, limit_pages));
    zram_ptr->disksize = ULONGLONG(zram_buf + field_offset(zram, disksize));
    zram_ptr->claim = BOOL(zram_buf + field_offset(zram, claim));
    FREEBUF(zram_buf);

    // Read statistics
    void *stats_buf = read_struct(addr + field_offset(zram, stats), "zram_stats");
    if (stats_buf == nullptr) {
        LOGE("Failed to read zram_stats\n");
        return nullptr;
    }
    zram_ptr->stats.compr_data_size = ULONGLONG(stats_buf + field_offset(zram_stats, compr_data_size));
    if (field_offset(zram_stats, num_reads) != -1) {
        zram_ptr->stats.num_reads = ULONGLONG(stats_buf + field_offset(zram_stats, num_reads));
    }
    if (field_offset(zram_stats, num_writes) != -1) {
        zram_ptr->stats.num_writes = ULONGLONG(stats_buf + field_offset(zram_stats, num_writes));
    }

    zram_ptr->stats.failed_reads = ULONGLONG(stats_buf + field_offset(zram_stats, failed_reads));
    zram_ptr->stats.failed_writes = ULONGLONG(stats_buf + field_offset(zram_stats, failed_writes));
    if (field_offset(zram_stats, invalid_io) != -1) {
        zram_ptr->stats.invalid_io = ULONGLONG(stats_buf + field_offset(zram_stats, invalid_io));
    }
    zram_ptr->stats.notify_free = ULONGLONG(stats_buf + field_offset(zram_stats, notify_free));
    zram_ptr->stats.same_pages = ULONGLONG(stats_buf + field_offset(zram_stats, same_pages));
    zram_ptr->stats.huge_pages = ULONGLONG(stats_buf + field_offset(zram_stats, huge_pages));
    if (field_offset(zram_stats, huge_pages_since) != -1) {
        zram_ptr->stats.huge_pages_since = ULONGLONG(stats_buf + field_offset(zram_stats, huge_pages_since));
    }
    zram_ptr->stats.pages_stored = ULONGLONG(stats_buf + field_offset(zram_stats, pages_stored));
    zram_ptr->stats.max_used_pages = ULONGLONG(stats_buf + field_offset(zram_stats, max_used_pages));
    zram_ptr->stats.writestall = ULONGLONG(stats_buf + field_offset(zram_stats, writestall));
    zram_ptr->stats.miss_free = ULONGLONG(stats_buf + field_offset(zram_stats, miss_free));
    FREEBUF(stats_buf);
    // Add to global list
    zram_list.push_back(zram_ptr);
    return zram_ptr;
}

#pragma GCC diagnostic pop
