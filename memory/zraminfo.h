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

#ifndef ZRAM_DRV_DEFS_H_
#define ZRAM_DRV_DEFS_H_

#include "plugin.h"
extern "C" {
#include "lib/lzo/lzo.h"
#include "lib/lz4/lz4.h"
}

// Object allocation and structure bit field definitions
#define OBJ_ALLOCATED_TAG   1    ///< Tag bit indicating object is allocated
#define FULLNESS_BITS       2    ///< Bits for zspage fullness ratio
#define CLASS_BITS          8    ///< Bits for size class ID
#define ISOLATED_BITS       3    ///< Bits for isolated page count
#define MAGIC_VAL_BITS      8    ///< Bits for magic value validation

/**
 * @struct zram_table_entry
 * @brief ZRAM table entry structure
 *
 * Represents an entry in the ZRAM table that maps to compressed pages.
 */
struct zram_table_entry {
    union {
        unsigned long handle;
        unsigned long element;
    };
    unsigned long flags;
};

/**
 * @struct zram_stats
 * @brief ZRAM device statistics
 *
 * Contains comprehensive statistics for ZRAM device operations
 * including compression, I/O operations, and memory usage.
 */
struct zram_stats {
    ulonglong compr_data_size;      ///< Compressed data size in bytes
    ulonglong num_reads;            ///< Number of read operations
    ulonglong num_writes;           ///< Number of write operations
    ulonglong failed_reads;         ///< Number of failed reads
    ulonglong failed_writes;        ///< Number of failed writes
    ulonglong invalid_io;           ///< Number of invalid I/O operations
    ulonglong notify_free;          ///< Number of free notifications
    ulonglong same_pages;           ///< Number of same/duplicate pages
    ulonglong huge_pages;           ///< Number of huge pages
    ulonglong huge_pages_since;     ///< Huge pages since last reset
    ulonglong pages_stored;         ///< Total pages stored
    ulonglong max_used_pages;       ///< Maximum pages used
    ulonglong writestall;           ///< Write stall count
    ulonglong miss_free;            ///< Missed free operations
};

/**
 * @struct zs_pool_stats
 * @brief ZSmalloc pool statistics
 *
 * Statistics for the zsmalloc memory pool.
 */
struct zs_pool_stats {
    atomic_long_t pages_compacted;  ///< Number of compacted pages
};

/**
 * @struct zobj
 * @brief ZSmalloc object information
 *
 * Represents a single object within a zsmalloc page.
 */
struct zobj {
    int id;                         ///< Object ID
    ulong start;                    ///< Start physical address
    ulong end;                      ///< End physical address
    ulong handle_addr;              ///< Handle address
    ulong pfn;                      ///< Page frame number
    union{
        int next;                   ///< Next free object (if free)
        int index;                  ///< Object index (if allocated)
    };
    bool is_free;                   ///< Allocation status
};

/**
 * @struct pageinfo
 * @brief Page information structure
 *
 * Contains information about a page and its objects.
 */
struct pageinfo {
    ulong addr;                                     ///< Page address
    std::vector<std::shared_ptr<zobj>> obj_list;   ///< List of objects in page
};

/**
 * @struct zpage
 * @brief ZSmalloc page structure
 *
 * Represents a zspage with its metadata and page list.
 */
struct zpage {
    ulong addr;                                         ///< ZSpage address
    struct zspage zspage;                               ///< ZSpage metadata
    int obj_index;                                      ///< Current object index
    std::vector<std::shared_ptr<pageinfo>> page_list;  ///< List of pages
};

/**
 * @struct size_class
 * @brief ZSmalloc size class structure
 *
 * Represents a size class in zsmalloc allocator with its configuration
 * and list of zspages organized by fullness.
 */
struct size_class {
    ulong addr;                                                 ///< Size class address
    std::vector<std::vector<std::shared_ptr<zpage>>> fullness_list;  ///< ZSpages by fullness
    bool zspage_parser;                                         ///< Parser status flag
    int size;                                                   ///< Object size
    int objs_per_zspage;                                        ///< Objects per zspage
    int pages_per_zspage;                                       ///< Pages per zspage
    unsigned int index;                                         ///< Size class index
    std::vector<ulong> stats;                                   ///< Statistics array
};

/**
 * @struct zs_pool
 * @brief ZSmalloc memory pool structure
 *
 * Represents the zsmalloc memory pool with its size classes
 * and allocation statistics.
 */
struct zs_pool {
    ulong addr;                                         ///< Pool address
    std::string name;                                   ///< Pool name
    std::vector<std::shared_ptr<size_class>> class_list;  ///< Size class list
    ulonglong pages_allocated;                          ///< Total allocated pages
    struct zs_pool_stats stats;                         ///< Pool statistics
    int isolated_pages;                                 ///< Isolated page count
    bool destroying;                                    ///< Destruction flag
};

/**
 * @struct zram
 * @brief ZRAM device structure
 *
 * Main structure representing a ZRAM block device with its
 * configuration, memory pool, and statistics.
 */
struct zram {
    ulong addr;                         ///< ZRAM device address
    ulong table;                        ///< Table address
    std::shared_ptr<zs_pool> mem_pool;  ///< Memory pool pointer
    std::string zcomp_name;             ///< Compressor name
    std::string disk_name;              ///< Disk name
    ulong limit_pages;                  ///< Page limit
    struct zram_stats stats;            ///< Device statistics
    ulonglong disksize;                 ///< Disk size in bytes
    std::string compressor;             ///< Compressor algorithm
    bool claim;                         ///< Claim status
};

/**
 * @enum zs_stat_type
 * @brief ZSmalloc statistics type enumeration
 *
 * Defines the types of statistics tracked for size classes.
 */
enum zs_stat_type {
    CLASS_EMPTY,
    CLASS_ALMOST_EMPTY,
    CLASS_ALMOST_FULL,
    CLASS_FULL,
    OBJ_ALLOCATED,
    OBJ_USED,
    NR_ZS_STAT_TYPE,
};

/**
 * @class Zraminfo
 * @brief ZRAM information parser and analyzer
 *
 * Base class for ZRAM analysis providing parsing functionality for
 * ZRAM devices, memory pools, and compressed page data.
 */
class Zraminfo : public ParserPlugin {
private:
    int ZRAM_FLAG_SHIFT;               ///< Bit shift for ZRAM flags
    int ZRAM_FLAG_SAME_BIT;            ///< Bit flag for same pages
    int ZRAM_FLAG_WB_BIT;              ///< Bit flag for writeback
    int ZRAM_COMP_PRIORITY_BIT1;       ///< Compression priority bit 1
    int ZRAM_COMP_PRIORITY_MASK;       ///< Compression priority mask

    /**
     * @brief Parse ZRAM device structure
     * @param addr ZRAM device address
     * @return Shared pointer to parsed zram structure
     */
    std::shared_ptr<zram> parser_zram(ulong addr);

    /**
     * @brief Parse zsmalloc memory pool
     * @param addr Memory pool address
     * @return Shared pointer to parsed zs_pool structure
     */
    std::shared_ptr<zs_pool> parser_mem_pool(ulong addr);

    /**
     * @brief Parse size class structure
     * @param addr Size class address
     * @return Shared pointer to parsed size_class structure
     */
    std::shared_ptr<size_class> parser_size_class(ulong addr);

    /**
     * @brief Parse zspage structure
     * @param addr ZSpage address
     * @param class_ptr Pointer to parent size class
     * @return Shared pointer to parsed zpage structure
     */
    std::shared_ptr<zpage> parser_zpage(ulong addr,std::shared_ptr<size_class> class_ptr);

    /**
     * @brief Parse pages in a zspage
     * @param first_page Address of first page
     * @param class_ptr Pointer to size class
     * @param page_ptr Pointer to zpage structure
     */
    void parser_pages(ulong first_page,std::shared_ptr<size_class> class_ptr,std::shared_ptr<zpage> page_ptr);

    /**
     * @brief Parse objects in a page
     * @param page_addr Page address
     * @param class_ptr Pointer to size class
     * @param zspage_ptr Pointer to zspage structure
     */
    void parser_obj(ulong page_addr,std::shared_ptr<size_class> class_ptr,std::shared_ptr<zpage> zspage_ptr);

    /**
     * @brief Parse single object information
     * @param obj_id Object ID
     * @param handle_addr Handle address
     * @param start Start physical address
     * @param end End physical address
     * @return Shared pointer to parsed zobj structure
     */
    std::shared_ptr<zobj> parser_obj(int obj_id, ulong handle_addr,physaddr_t start,physaddr_t end);

    /**
     * @brief Convert handle to PFN and object index
     * @param handle Object handle
     * @param pfn Output page frame number
     * @param obj_idx Output object index
     */
    void handle_to_location(ulong handle, ulong* pfn, int* obj_idx);

    /**
     * @brief Read ZRAM table entry
     * @param zram_ptr Pointer to ZRAM device
     * @param index Table entry index
     * @param entry Output table entry structure
     * @return true if successful, false otherwise
     */
    bool read_table_entry(std::shared_ptr<zram> zram_ptr, ulonglong index, struct zram_table_entry* entry);

    /**
     * @brief Read compressed object from memory
     * @param zram_ptr Pointer to ZRAM device
     * @param entry Table entry
     * @param read_len Output read length
     * @param huge_obj Output huge object flag
     * @return Pointer to object data buffer
     */
    char* read_object(std::shared_ptr<zram> zram_ptr,struct zram_table_entry entry,int& read_len, bool& huge_obj);

    /**
     * @brief Get zspage structure from page
     * @param page Page address
     * @param zp Output zspage structure
     * @return true if successful, false otherwise
     */
    bool get_zspage(ulong page,struct zspage* zp);

    /**
     * @brief Get size class ID from zspage
     * @param zspage_s ZSpage structure
     * @return Size class ID
     */
    int get_class_id(struct zspage& zspage_s);

    /**
     * @brief Decompress data using specified algorithm
     * @param comp_name Compression algorithm name
     * @param source Source compressed data
     * @param dest Destination buffer
     * @param compressedSize Compressed data size
     * @param maxDecompressedSize Maximum decompressed size
     * @return 0 on success, error code otherwise
     */
    int decompress(std::string comp_name,char* source, char* dest,int compressedSize, int maxDecompressedSize);

    /**
     * @brief Decompress LZO compressed data
     * @param source Source compressed data
     * @param dest Destination buffer
     * @param compressedSize Compressed data size
     * @param maxDecompressedSize Maximum decompressed size
     * @return 0 on success, error code otherwise
     */
    int lzo1x_decompress(char *source, char *dest, int compressedSize, int maxDecompressedSize);

    /**
     * @brief Decompress LZ4 compressed data
     * @param source Source compressed data
     * @param dest Destination buffer
     * @param compressedSize Compressed data size
     * @param maxDecompressedSize Maximum decompressed size
     * @return Decompressed size on success, negative on error
     */
    int lz4_decompress(char *source, char *dest, int compressedSize, int maxDecompressedSize);

public:
    std::vector<std::shared_ptr<zram>> zram_list;  ///< List of parsed ZRAM devices
    int group_cnt;                                  ///< Number of fullness groups

    /**
     * @brief Constructor - initializes ZRAM parser
     */
    Zraminfo();

    /**
     * @brief Parse all ZRAM devices in the system
     */
    void parser_zrams();

    /**
     * @brief Parse all zspages in a size class
     * @param class_ptr Pointer to size class
     */
    void parser_zpage(std::shared_ptr<size_class> class_ptr);

    /**
     * @brief Read and decompress a ZRAM page
     * @param zram_addr ZRAM device address
     * @param index Page index
     * @return Pointer to decompressed page data
     */
    char* read_zram_page(ulong zram_addr, ulonglong index);

    /**
     * @brief Check if ZRAM is enabled in the kernel
     * @return true if enabled, false otherwise
     */
    bool is_zram_enable();

    /**
     * @brief Main command handler (empty in base class)
     */
    void cmd_main(void) override;

    /**
     * @brief Initialize structure field offsets
     */
    void init_offset(void) override;

    /**
     * @brief Initialize command information (empty in base class)
     */
    void init_command(void) override;
};

#endif // ZRAM_DRV_DEFS_H_
