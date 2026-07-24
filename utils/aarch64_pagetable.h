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

#ifndef PAGETABLE_H_
#define PAGETABLE_H_

#include "plugin.h"
#include <map>
#include <string>
#include <cstdint>

// Page size constants
constexpr uint64_t SZ_4K = 0x1000ULL;
constexpr uint64_t SZ_64K = 0x10000ULL;
constexpr uint64_t SZ_2M = 0x200000ULL;
constexpr uint64_t SZ_32M = 0x2000000ULL;
constexpr uint64_t SZ_1G = 0x40000000ULL;
constexpr uint64_t SZ_256G = 0x4000000000ULL;

// Page table entry type masks
constexpr uint32_t FLSL_PTE_TYPE_MASK = 0x3;
constexpr uint32_t FLSL_TYPE_BLOCK = 0x1;
constexpr uint32_t FLSL_TYPE_TABLE = 0x3;
constexpr uint64_t FLSL_BASE_MASK = 0xFFFFFFFFF000ULL;

// Last level page table entry bits
constexpr uint64_t LL_TYPE_PAGE = 0x3;
constexpr uint64_t LL_AP_BITS = 0xC0;
constexpr uint64_t LL_AP_RO = 0xC0;
constexpr uint64_t LL_AP_PR_RW = 0x00;
constexpr uint64_t LL_AP_PR_RO = 0x80;
constexpr uint64_t LL_SH_BITS = 0x300;
constexpr uint64_t LL_ATTR_INDX = 0x1C;
constexpr uint64_t LL_XN = 0x40000000000000ULL;
constexpr uint64_t LL_CH = 0x80000000000000ULL;

// Shareability attributes
constexpr int SH_NON_SHARE = 0x0;
constexpr int SH_RESERVED = 0x100;
constexpr int SH_OUTER_SHARE = 0x200;
constexpr int SH_INNER_SHARE = 0x300;

// Memory attribute indices
constexpr int ATTR_IDX_NONCACHED = 0x0;
constexpr int ATTR_IDX_CACHE = 0x1;
constexpr int ATTR_IDX_DEV = 0x2;
constexpr int ATTR_IDX_UPST = 0x3;
constexpr int ATTR_IDX_LLC_NWA = 0x4;

// Page table entry counts
constexpr int NUM_FL_PTE = 512;
constexpr int NUM_SL_PTE = 512;
constexpr int NUM_TL_PTE = 512;
constexpr int NUM_LL_PTE = 512;

/**
 * Flat mapping structure
 * Represents a single page table entry mapping
 */
struct FlatMapping {
    uint64_t virt;                  // Virtual address
    int64_t phys;                   // Physical address
    std::string type;               // Mapping type (R/W, RO, etc.)
    uint64_t map_size;              // Page size
    std::string attr_indx_str;      // Memory attribute string
    std::string shareability_str;   // Shareability attribute string
    std::string execute_never_str;  // Execute never attribute string
    bool mapped;                    // Whether this entry is mapped

    FlatMapping(uint64_t v, int64_t p, const std::string& t, uint64_t ms,
                const std::string& ai, const std::string& sh,
                const std::string& xn, bool m)
        : virt(v), phys(p), type(t), map_size(ms), attr_indx_str(ai),
          shareability_str(sh), execute_never_str(xn), mapped(m) {}
};

/**
 * Collapsed mapping structure
 * Represents a contiguous range of page table entries
 */
struct CollapsedMapping {
    uint64_t virt_start;            // Virtual start address
    uint64_t virt_end;              // Virtual end address
    int64_t phys_start;             // Physical start address
    int64_t phys_end;               // Physical end address
    std::string map_type;           // Mapping type
    uint64_t map_size;              // Page size
    std::string attr_indx_str;      // Memory attribute string
    std::string shareability_str;   // Shareability attribute string
    std::string execute_never_str;  // Execute never attribute string
    bool mapped;                    // Whether this range is mapped

    CollapsedMapping(uint64_t vs, uint64_t ve, int64_t ps, int64_t pe,
                     const std::string& mt, uint64_t ms, const std::string& ai,
                     const std::string& sh, const std::string& xn, bool m)
        : virt_start(vs), virt_end(ve - 1), phys_start(ps), phys_end(pe - 1),
          map_type(mt), map_size(ms), attr_indx_str(ai), shareability_str(sh),
          execute_never_str(xn), mapped(m) {}
};

/**
 * Mapping information structure
 * Contains details about a page table entry
 */
struct MappingInfo {
    int64_t phy_addr;       // Physical address
    uint64_t page_size;     // Page size
    uint32_t map_type;      // Mapping type
    bool status;            // Whether the mapping is valid
    int skip_count;         // Number of contiguous entries to skip
    int attr_indx;          // Memory attribute index
    int shareability;       // Shareability attribute
    int xn_bit;             // Execute never bit
};

/**
 * ARM64 page table parser utility class
 * Provides methods for parsing and displaying AArch64 page tables
 */
class AArch64PTParser : public ParserPlugin {
public:
    /**
     * Parse and print AArch64 page tables
     * @param pg_table Physical address of the page table base
     * @param level Page table level (3 or 4)
     * @param client_name Name of the client/domain
     */
    void parse_and_print_tables(ulong pg_table, uint level, const std::string& client_name);
    void init_offset(void) override;
    void init_command(void) override;
    void cmd_main(void) override;
private:
    /**
     * Create flat mappings from page table
     * @param pg_table Physical address of the page table base
     * @param level Page table level
     * @return Map of virtual addresses to flat mappings
     */
    std::map<uint64_t, FlatMapping> create_flat_mappings(ulong pg_table, int level);

    /**
     * Create collapsed mappings from flat mappings
     * @param flat_mapping Map of flat mappings
     * @return Map of virtual addresses to collapsed mappings
     */
    std::map<uint64_t, CollapsedMapping> create_collapsed_mapping(
        const std::map<uint64_t, FlatMapping>& flat_mapping);

    /**
     * Add a collapsed mapping
     * @param mappings Map to add the collapsed mapping to
     * @param first First flat mapping in the range
     * @param last Last flat mapping in the range
     */
    void add_collapsed_mapping(std::map<uint64_t, CollapsedMapping>& mappings,
                                     const FlatMapping& first, const FlatMapping& last);

    /**
     * Add a flat mapping (internal helper)
     * @param mappings Map to add the flat mapping to
     * @param virt Virtual address
     * @param phy_addr Physical address
     * @param map_type_str Mapping type string
     * @param page_size Page size
     * @param attr_indx_str Memory attribute string
     * @param shareability_str Shareability attribute string
     * @param execute_never_str Execute never attribute string
     * @param mapped Whether the entry is mapped
     */
    void add_flat_mapping_internal(std::map<uint64_t, FlatMapping>& mappings,
                                         uint64_t virt, int64_t phy_addr,
                                         const std::string& map_type_str, uint64_t page_size,
                                         const std::string& attr_indx_str,
                                         const std::string& shareability_str,
                                         const std::string& execute_never_str, bool mapped);

    /**
     * Add a flat mapping with attribute parsing
     * @param mappings Map to add the flat mapping to
     * @param fl_idx First level index
     * @param sl_idx Second level index
     * @param tl_idx Third level index
     * @param ll_idx Last level index
     * @param phy_addr Physical address
     * @param map_type Mapping type
     * @param page_size Page size
     * @param attr_indx Memory attribute index
     * @param shareability Shareability attribute
     * @param xn_bit Execute never bit
     * @param mapped Whether the entry is mapped
     */
    void add_flat_mapping(std::map<uint64_t, FlatMapping>& mappings,
                                int fl_idx, int sl_idx, int tl_idx, int ll_idx,
                                int64_t phy_addr, uint32_t map_type, uint64_t page_size,
                                int attr_indx, int shareability, int xn_bit, bool mapped);

    /**
     * Get mapping information for a 4KB/64KB page
     * @param pg_table Physical address of the page table
     * @param index Page table entry index
     * @return Mapping information
     */
    MappingInfo get_mapping_info(ulong pg_table, int index);

    /**
     * Get mapping information for a 2MB/32MB section
     * @param pg_table Physical address of the page table
     * @param index Page table entry index
     * @return Mapping information
     */
    MappingInfo get_section_mapping_info(ulong pg_table, int index);

    /**
     * Get mapping information for a 1GB super section
     * @param pg_table Physical address of the page table
     * @param index Page table entry index
     * @return Mapping information
     */
    MappingInfo get_super_section_mapping_info(ulong pg_table, int index);

    /**
     * Parse second level page table (3rd and 4th level entries)
     * @param sl_pg_table_entry Second level page table entry
     * @param fl_index First level index
     * @param sl_index Second level index
     * @param tmp_mapping Temporary mapping map
     * @return Updated mapping map
     */
    std::map<uint64_t, FlatMapping> parse_2nd_level_table(
        uint64_t sl_pg_table_entry, int fl_index, int sl_index,
        std::map<uint64_t, FlatMapping> tmp_mapping);

    /**
     * Read first level page table entry
     * @param fl_pte Physical address of the first level PTE
     * @param skip_fl Whether to skip first level (for 3-level tables)
     * @return Pair of (entry value, second level PTE address)
     */
    std::pair<uint64_t, ulong> fl_entry(ulong fl_pte, int skip_fl);

    /**
     * Get page size order string
     * @param size Page size
     * @return String representation of the page size
     */
    static std::string get_order_string(uint64_t size);
};

#endif // PAGETABLE_H_
