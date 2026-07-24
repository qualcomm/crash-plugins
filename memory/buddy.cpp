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

#include "buddy.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(Buddy)
#endif

/**
 * @brief Main command handler for buddy allocator commands
 *
 * Processes command-line arguments and dispatches to appropriate handlers:
 * The buddy allocator information is parsed on first use and cached.
 *
 * @note Requires at least one argument, otherwise displays usage information
 */
void Buddy::cmd_main(void) {
    int c;
    std::string cppString;

    // Validate minimum argument count
    if (argcnt < 2) {
        LOGD("Insufficient arguments, displaying usage\n");
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }
    // Parse buddy information if not already cached
    if (node_list.size() == 0) {
        LOGD("Node list empty, parsing buddy information\n");
        parser_buddy_info();
    }

    // Parse command-line options
    while ((c = getopt(argcnt, args, "anz:")) != EOF) {
        switch(c) {
            case 'a':
                print_buddy_info();
                break;

            case 'n':
                print_memory_node();
                break;

            case 'z':
                cppString.assign(optarg);
                print_memory_zone(cppString);
                break;

            default:
                LOGD("Invalid option encountered\n");
                argerrs++;
                break;
        }
    }

    // Display usage if there were argument errors
    if (argerrs) {
        LOGD("Argument errors detected, displaying usage\n");
        cmd_usage(pc->curcmd, SYNOPSIS);
    }
}

/**
 * @brief Initialize structure field offsets
 *
 * Initializes all field offsets for buddy allocator kernel structures.
 * This must be called before any parsing operations to ensure correct
 * memory access to kernel structures.
 */
void Buddy::init_offset(void) {
    // Initialize pglist_data (NUMA node) fields
    field_init(pglist_data, node_zones);
    field_init(pglist_data, node_start_pfn);
    field_init(pglist_data, node_present_pages);
    field_init(pglist_data, node_spanned_pages);
    field_init(pglist_data, node_id);
    field_init(pglist_data, totalreserve_pages);
    field_init(pglist_data, vm_stat);
    struct_init(pglist_data);

    // Initialize zone fields
    field_init(zone, _watermark);
    field_init(zone, watermark_boost);
    field_init(zone, lowmem_reserve);
    field_init(zone, zone_start_pfn);
    field_init(zone, managed_pages);
    field_init(zone, spanned_pages);
    field_init(zone, present_pages);
    field_init(zone, cma_pages);
    field_init(zone, name);
    field_init(zone, free_area);
    field_init(zone, vm_stat);
    struct_init(zone);

    // Initialize free_area fields
    field_init(free_area, free_list);
    field_init(free_area, nr_free);
    struct_init(free_area);

    // Initialize page fields (for free list traversal)
    field_init(page, buddy_list);
    field_init(page, lru);

    // Initialize atomic_long_t for statistics
    struct_init(atomic_long_t);
}

/**
 * @brief Initialize command metadata and help information
 *
 * Sets up the command name, description, and detailed help text including:
 * - Command synopsis
 * - Usage examples with sample output
 * - Option descriptions
 *
 * This information is displayed when the user requests help or provides
 * invalid arguments.
 */
void Buddy::init_command(void) {
    cmd_name = "buddy";
    help_str_list = {
        "buddy",                                  /* command name */
        "display buddy allocator information",    /* short description */
        "[-a] [-n] [-z zone_addr]\n"
        "  This command displays buddy allocator information.\n"
        "\n"
        "    -a              display buddy info\n"
        "    -n              display memory node info\n"
        "    -z zone_addr    display page info of zone\n",
        "\n",
        "EXAMPLES",
        "\n",
        "  Display buddy info:",
        "   %s> buddy -a",
        "       Node(0)",
        "       ---------------------------------------------------------------------------------------------------------------------------",
        "                                                         zone DMA32",
        "       ---------------------------------------------------------------------------------------------------------------------------",
        "             Order       4K       8K      16K      32K      64K     128K     256K     512K    1024K    2048K    4096K      Total",
        "         Unmovable     1645     2367      958      712       37       11        2        0        0        0        0    66.32Mb",
        "           Movable     5496     3435     1998      764      145       33        5        0        0        0        0   117.84Mb",
        "       Reclaimable      135       39       94       56        0        0        0        0        0        0        0     4.05Mb",
        "               CMA        0        0        0        0        0        0        0        0        0        0        0         0b",
        "        HighAtomic      109       31       25       17        1        1        0        0        0        0        0     1.78Mb",
        "           Isolate        0        0        0        0        0        0        0        0        0        0        0         0b",
        "             Total  28.85Mb  45.88Mb  48.05Mb  48.41Mb  11.44Mb   5.62Mb   1.75Mb       0b       0b       0b       0b   189.99Mb",
        "       ---------------------------------------------------------------------------------------------------------------------------",
        "\n",
        "  Display memory node info:",
        "   %s> buddy -n",
        "    Config:",
        "    ---------------------------------------",
        "    min_free_kbytes          : 5792kb",
        "    user_min_free_kbytes     : 5792kb",
        "    watermark_scale_factor   : 150",
        "    ---------------------------------------",
        "    ",
        "    Node:",
        "    =======================================",
        "    pglist_data(0) : 0xffffffde3102c340",
        "       spanned        : 524288(2.00Gb)",
        "       present        : 519680(1.98Gb)",
        "       hole           : 4608(18.00Mb)",
        "       start_pfn      : 40000",
        "       start_paddr    : 0x40000000",
        "    ",
        "      DMA32 zone: 0xffffffde3102c340",
        "         spanned           : 524288(2.00Gb)",
        "         present           : 519680(1.98Gb)",
        "         hole              : 4608(18.00Mb)",
        "         managed           : 455177(1.74Gb)",
        "         reserved          : 64503(251.96Mb)",
        "         cma_pages         : 67584(264.00Mb)",
        "         start_pfn         : 40000",
        "         start_paddr       : 0x40000000",
        "         watermark_boost   : 0",
        "         WMARK_HIGH        : 15102(58.99Mb)",
        "         WMARK_LOW         : 8275(32.32Mb)",
        "         WMARK_MIN         : 1448(5.66Mb)",
        "\n",
        "  Display page info of zone:",
        "   %s> buddy -z 0xffffffde3102c340",
        "    Order[0] 4K",
        "       migratetype:Unmovable Order[0]",
        "           [1]Page:0xfffffffe0152bfc0 PA:0x94aff000",
        "           [2]Page:0xfffffffe0152c380 PA:0x94b0e000",
        "\n",
    };
}

/**
 * @brief Constructor
 *
 * Initializes the Buddy allocator parser. The actual parsing is deferred
 * until first use to avoid unnecessary overhead.
 */
Buddy::Buddy() {

}

/**
 * @brief Parse free page lists for all migration types
 *
 * For a given free_area (representing a specific order), this function
 * parses all the free page lists organized by migration type. Each
 * migration type has its own list of free pages to reduce fragmentation.
 *
 * Migration types include:
 * - Unmovable: Pages that cannot be moved (kernel allocations)
 * - Movable: Pages that can be moved (user space, page cache)
 * - Reclaimable: Pages that can be reclaimed (slab caches)
 * - CMA: Contiguous Memory Allocator pages
 * - HighAtomic: High-priority atomic allocations
 * - Isolate: Isolated pages (for memory hotplug, etc.)
 *
 * @param addr Address of the free_list array
 * @return 2D vector of page addresses [migration_type][page_index]
 */
std::vector<std::vector<ulong>> Buddy::parser_free_list(ulong addr) {
    std::vector<std::vector<ulong>> free_list;
    size_t free_list_cnt = field_size(free_area, free_list) / struct_size(list_head);
    // Iterate through each migration type
    for (size_t i = 0; i < free_list_cnt; i++) {
        ulong list_head_addr = addr + i * struct_size(list_head);
        if (!is_kvaddr(list_head_addr)) {
            LOGE("Invalid list_head address for migration type %zu\n", i);
            continue;
        }
        LOGD("Parsing migration type free_list#%d at address 0x%lx\n", i, addr);
        // Determine the correct offset based on kernel version
        // Kernel 5.10+ uses buddy_list, older versions use lru
        int offset = 0;
        if (THIS_KERNEL_VERSION >= LINUX(5, 10, 0)) {
            offset = field_offset(page, buddy_list);
        } else {
            offset = field_offset(page, lru);
        }
        // Traverse the linked list of free pages
        std::vector<ulong> page_list = for_each_list(list_head_addr, offset);
        free_list.push_back(page_list);
    }
    return free_list;
}

/**
 * @brief Parse free areas for all orders
 *
 * The buddy allocator organizes free pages by order (power-of-2 sizes).
 * This function parses all free_area structures for a zone, where each
 * free_area represents a specific order (0 = 4KB, 1 = 8KB, 2 = 16KB, etc.).
 *
 * @param addr Address of the first free_area structure in the zone
 * @return Vector of parsed free_area structures, one per order
 */
std::vector<std::shared_ptr<free_area>> Buddy::parser_free_area(ulong addr) {
    std::vector<std::shared_ptr<free_area>> area_list;
    size_t free_area_cnt = field_size(zone, free_area) / struct_size(free_area);
    LOGD("Number of free_area: %zu\n", free_area_cnt);
    // Iterate through each order
    for (size_t i = 0; i < free_area_cnt; i++) {
        ulong area_addr = addr + field_offset(zone, free_area) + i * struct_size(free_area);
        LOGD("Parsing order %zu at address 0x%lx (block size: %s)\n",
                i, area_addr, csize((1U << i) * page_size).c_str());

        void *area_buf = read_struct(area_addr, "free_area");
        if (area_buf == nullptr) {
            LOGE("Failed to read free_area structure for order %zu\n", i);
            continue;
        }

        std::shared_ptr<free_area> area_ptr = std::make_shared<free_area>();
        area_ptr->addr = area_addr;
        area_ptr->nr_free = ULONG(area_buf + field_offset(free_area, nr_free));
        // Parse free lists for all migration types
        area_ptr->free_list = parser_free_list(addr + field_offset(free_area, free_list) +
                                               i * struct_size(free_area));

        area_list.push_back(area_ptr);
        FREEBUF(area_buf);
    }
    return area_list;
}

/**
 * @brief Parse NUMA node information
 *
 * Reads and parses a pglist_data structure representing a NUMA node.
 * A NUMA node contains one or more memory zones and associated statistics.
 *
 * For each node, extracts:
 * - Node ID and address
 * - Memory statistics (start PFN, present/spanned pages)
 * - VM statistics array
 * - All zones within the node
 *
 * @param addr Address of pglist_data structure
 * @return Shared pointer to parsed node structure, or nullptr on failure
 */
std::shared_ptr<pglist_data> Buddy::parser_node_info(ulong addr) {
    void *node_buf = read_struct(addr, "pglist_data");
    if (node_buf == nullptr) {
        LOGE("Failed to read pglist_data structure\n");
        return nullptr;
    }
    std::shared_ptr<pglist_data> node_ptr = std::make_shared<pglist_data>();
    node_ptr->addr = addr;
    node_ptr->id = INT(node_buf + field_offset(pglist_data, node_id));
    node_ptr->start_pfn = ULONG(node_buf + field_offset(pglist_data, node_start_pfn));
    node_ptr->present_pages = ULONG(node_buf + field_offset(pglist_data, node_present_pages));
    node_ptr->spanned_pages = ULONG(node_buf + field_offset(pglist_data, node_spanned_pages));
    node_ptr->totalreserve_pages = ULONG(node_buf + field_offset(pglist_data, totalreserve_pages));
    LOGD("Node metadata:\n");
    LOGD("  id=%d\n", node_ptr->id);
    LOGD("  start_pfn=0x%lx\n", node_ptr->start_pfn);
    LOGD("  present_pages=%lu (%s)\n", node_ptr->present_pages, csize(node_ptr->present_pages * page_size).c_str());
    LOGD("  spanned_pages=%lu (%s)\n", node_ptr->spanned_pages, csize(node_ptr->spanned_pages * page_size).c_str());

    // Read VM statistics
    size_t vm_stat_cnt = field_size(pglist_data, vm_stat) / struct_size(atomic_long_t);
    for (size_t i = 0; i < vm_stat_cnt; i++) {
        node_ptr->vm_stat.push_back(ULONG(node_buf + field_offset(pglist_data, vm_stat) +
                                         i * struct_size(atomic_long_t)));
    }
    // Parse all zones in this node
    ulong node_zones = addr + field_offset(pglist_data, node_zones);
    for (int i = 0; i < vt->nr_zones; i++) {
        ulong zone_addr = (node_zones + (i * struct_size(zone)));
        LOGD("Parsing zone#%d at address 0x%lx\n", i, addr);
        std::shared_ptr<zone> zone_ptr = parser_zone_info(zone_addr);
        if (zone_ptr == nullptr) {
            LOGE("Failed to parse zone %d\n", i);
            continue;
        }
        node_ptr->zone_list.push_back(zone_ptr);
    }
    FREEBUF(node_buf);
    return node_ptr;
}

/**
 * @brief Parse memory zone information
 *
 * Reads and parses a zone structure representing a memory zone.
 * Memory zones divide physical memory into regions with different
 * properties (DMA, DMA32, Normal, HighMem, Movable).
 *
 * @param addr Address of zone structure
 * @return Shared pointer to parsed zone structure, or nullptr on failure
 */
std::shared_ptr<zone> Buddy::parser_zone_info(ulong addr) {
    void *zone_buf = read_struct(addr, "zone");
    if (zone_buf == nullptr) {
        LOGE("Failed to read zone structure\n");
        return nullptr;
    }
    std::shared_ptr<zone> zone_ptr = std::make_shared<zone>();
    zone_ptr->addr = addr;
    zone_ptr->start_pfn = ULONG(zone_buf + field_offset(zone, zone_start_pfn));
    zone_ptr->present_pages = ULONG(zone_buf + field_offset(zone, present_pages));
    zone_ptr->spanned_pages = ULONG(zone_buf + field_offset(zone, spanned_pages));
    zone_ptr->managed_pages = ULONG(zone_buf + field_offset(zone, managed_pages));

    // Read CMA pages if available (kernel version dependent)
    if (field_offset(zone, cma_pages) > 0) {
        zone_ptr->cma_pages = ULONG(zone_buf + field_offset(zone, cma_pages));
    }
    zone_ptr->name = read_cstring(ULONG(zone_buf + field_offset(zone, name)), 64, "zone_name");
    zone_ptr->watermark_boost = ULONG(zone_buf + field_offset(zone, watermark_boost));
    LOGD("Zone metadata:\n");
    LOGD("  name=%s\n", zone_ptr->name.c_str());
    LOGD("  start_pfn=0x%lx\n", zone_ptr->start_pfn);
    LOGD("  managed_pages=%lu (%s)\n", zone_ptr->managed_pages, csize(zone_ptr->managed_pages * page_size).c_str());
    LOGD("  present_pages=%lu (%s)\n", zone_ptr->present_pages, csize(zone_ptr->present_pages * page_size).c_str());
    LOGD("  spanned_pages=%lu (%s)\n", zone_ptr->spanned_pages, csize(zone_ptr->spanned_pages * page_size).c_str());
    // Read watermark levels and lowmem reserves
    for (size_t i = 0; i < 3; i++) {
        zone_ptr->_watermark[i] = ULONG(zone_buf + (field_offset(zone, _watermark) + i * sizeof(unsigned long)));
        zone_ptr->lowmem_reserve[i] = ULONG(zone_buf + field_offset(zone, lowmem_reserve) + i * sizeof(long));
    }
    LOGD("Watermarks:\n");
    LOGD("  WMARK_MIN=%lu (%s)\n", zone_ptr->_watermark[WMARK_MIN], csize(zone_ptr->_watermark[WMARK_MIN] * page_size).c_str());
    LOGD("  WMARK_LOW=%lu (%s)\n", zone_ptr->_watermark[WMARK_LOW], csize(zone_ptr->_watermark[WMARK_LOW] * page_size).c_str());
    LOGD("  WMARK_HIGH=%lu (%s)\n", zone_ptr->_watermark[WMARK_HIGH], csize(zone_ptr->_watermark[WMARK_HIGH] * page_size).c_str());

    // Read VM statistics
    size_t vm_stat_cnt = field_size(zone, vm_stat) / struct_size(atomic_long_t);
    for (size_t i = 0; i < vm_stat_cnt; i++) {
        zone_ptr->vm_stat.push_back(ULONG(zone_buf + field_offset(zone, vm_stat) + i * struct_size(atomic_long_t)));
    }

    // Parse free areas for all orders
    zone_ptr->free_areas = parser_free_area(addr + field_offset(zone, free_area));
    FREEBUF(zone_buf);
    return zone_ptr;
}

/**
 * @brief Get migration type names from kernel
 *
 * Reads the migratetype_names array from kernel memory to get human-readable
 * names for each migration type. These names are used when displaying
 * buddy allocator information.
 *
 * Migration types typically include:
 * - Unmovable
 * - Movable
 * - Reclaimable
 * - CMA (Contiguous Memory Allocator)
 * - HighAtomic
 * - Isolate
 */
void Buddy::get_migratetype_names() {
    size_t migratetype_cnt = get_array_length(TO_CONST_STRING("migratetype_names"), nullptr, 0);
    ulong migratetype_names_addr = csymbol_value("migratetype_names");

    LOGD("Number of migration types: %zu\n", migratetype_cnt);
    for (size_t i = 0; i < migratetype_cnt; i++) {
        ulong addr = migratetype_names_addr + i * sizeof(void *);
        if (!is_kvaddr(addr)) {
            LOGE("Invalid address for migration type %zu\n", i);
            continue;
        }
        addr = read_pointer(addr, "migratetype_names addr");
        std::string name = read_cstring(addr, 64, "migratetype_names");
        migratetype_names.push_back(name);

        LOGD("Migration type %zu: %s\n", i, name.c_str());
    }
}

/**
 * @brief Parse all buddy allocator information
 *
 * Main parsing function that reads all buddy allocator structures from
 * kernel memory. This includes:
 * The parsed information is cached in node_list for subsequent use.
 *
 * @note This function performs extensive validation and will report
 *       errors if required structures are not available.
 */
void Buddy::parser_buddy_info() {
    struct node_table *nt;
    // Validate required flags
    if (!(vt->flags & (NODES | ZONES))) {
        LOGE("Required flags (NODES|ZONES) not set\n");
        return;
    }
    // Validate required structures
    if (!struct_size(zone)) {
        LOGE("zone structure not available\n");
        return;
    }

    if (!struct_size(free_area)) {
        LOGE("free_area structure not available\n");
        return;
    }

    if (!csymbol_exists("migratetype_names") ||
        (get_symbol_type(TO_CONST_STRING("migratetype_names"), nullptr, nullptr) != TYPE_CODE_ARRAY)) {
        LOGE("migratetype_names array not available\n");
        return;
    }
    LOGD("Number of NUMA nodes: %d\n", vt->numnodes);
    // Parse all NUMA nodes
    for (int n = 0; n < vt->numnodes; n++) {
        nt = &vt->node_table[n];
        LOGD("Parsing NUMA node#%d at address 0x%lx\n", n, nt->pgdat);
        std::shared_ptr<pglist_data> node_ptr = parser_node_info(nt->pgdat);
        if (node_ptr == nullptr) {
            LOGE("Failed to parse node %d\n", n);
            continue;
        }
        node_list.push_back(node_ptr);
    }
    // Get migration type names
    get_migratetype_names();
    // Read watermark configuration parameters
    if (csymbol_exists("min_free_kbytes")) {
        min_free_kbytes = read_int(csymbol_value("min_free_kbytes"), "min_free_kbytes");
        LOGD("min_free_kbytes = %d KB\n", min_free_kbytes);
    }

    if (csymbol_exists("user_min_free_kbytes")) {
        user_min_free_kbytes = read_int(csymbol_value("user_min_free_kbytes"),
                                       "user_min_free_kbytes");
        LOGD("user_min_free_kbytes = %d KB\n", user_min_free_kbytes);
    }

    if (csymbol_exists("watermark_scale_factor")) {
        watermark_scale_factor = read_int(csymbol_value("watermark_scale_factor"),
                                         "watermark_scale_factor");
        LOGD("watermark_scale_factor = %d\n", watermark_scale_factor);
    }
}

/**
 * @brief Print buddy allocator summary
 *
 * Displays a comprehensive summary of the buddy allocator state for all
 * nodes and zones. For each zone, shows:
 * - Free pages organized by order (4KB, 8KB, 16KB, etc.)
 * - Breakdown by migration type (Unmovable, Movable, Reclaimable, etc.)
 * - Total free memory per order and per migration type
 *
 * This provides a detailed view of memory fragmentation and availability.
 */
void Buddy::print_buddy_info() {
    for (const auto& node_ptr : node_list) {
        PRINT("Node(%d) \n", node_ptr->id);
        for (const auto& zone_ptr : node_ptr->zone_list) {
            // Skip empty zones
            if (zone_ptr->managed_pages == 0 || zone_ptr->spanned_pages == 0 ||
                zone_ptr->present_pages == 0) {
                LOGE("Skipping empty zone: %s\n", zone_ptr->name.c_str());
                continue;
            }
            // Print zone header
            PRINT("---------------------------------------------------------------------------------------------------------------------------\n");
            PRINT("                                             zone %s \n", zone_ptr->name.c_str());
            PRINT("---------------------------------------------------------------------------------------------------------------------------\n");

            // Print order headers
            PRINT("%12s ", "Order");
            for (int o = 0; o < vt->nr_free_areas; o++) {
                PRINT("%8s ", csize((1U << o) * page_size).c_str());
            }
            PRINT("%10s\n", "Total");

            // Calculate migration type count
            size_t free_list_cnt = field_size(free_area, free_list) / struct_size(list_head);
            if (free_list_cnt > migratetype_names.size()) {
                free_list_cnt = migratetype_names.size();
            }

            uint64_t total_size = 0;
            uint64_t total_by_order[vt->nr_free_areas] = {0};

            // Print each migration type
            for (size_t m = 0; m < free_list_cnt; m++) {
                PRINT("%12s ", migratetype_names[m].c_str());
                uint64_t total_per_type = 0;

                // Print free page count for each order
                for (int o = 0; o < vt->nr_free_areas; o++) {
                    int free_cnt = zone_ptr->free_areas[o]->free_list[m].size();
                    PRINT("%8d ", free_cnt);

                    uint64_t per_size = power(2, o) * page_size;
                    total_per_type += (per_size * free_cnt);
                    total_by_order[o] += (per_size * free_cnt);
                }

                total_size += total_per_type;
                PRINT("%10s\n", csize(total_per_type).c_str());
            }

            // Print totals
            PRINT("%12s ", "Total");
            for (int o = 0; o < vt->nr_free_areas; o++) {
                PRINT("%8s ", csize(total_by_order[o]).c_str());
            }
            PRINT("%10s\n", csize(total_size).c_str());
            PRINT("---------------------------------------------------------------------------------------------------------------------------\n\n\n");
        }
        PRINT("\n");
    }
}

/**
 * @brief Print node information
 *
 * Displays detailed information about a NUMA node including:
 * - Node ID and address
 * - Memory statistics (spanned, present, holes)
 * - Starting PFN and physical address
 *
 * @param node_ptr Pointer to node structure
 */
void Buddy::print_node_info(std::shared_ptr<pglist_data> node_ptr) {
    uint64_t spanned_size = node_ptr->spanned_pages * page_size;
    uint64_t present_size = node_ptr->present_pages * page_size;
    uint64_t hole_size = spanned_size - present_size;

    std::ostringstream oss;
    oss << std::left << "pglist_data(" << node_ptr->id << ")" << ": "
        << std::hex << node_ptr->addr << "\n"
        << std::left << std::setw(20) << "  spanned     : " << std::dec
        << node_ptr->spanned_pages << "(" << csize(spanned_size) << ") \n"
        << std::left << std::setw(20) << "  present     : " << std::dec
        << node_ptr->present_pages << "(" << csize(present_size) << ") \n"
        << std::left << std::setw(20) << "  hole        : " << std::dec
        << (node_ptr->spanned_pages - node_ptr->present_pages) << "(" << csize(hole_size) << ") \n"
        << std::left << std::setw(20) << "  start_pfn   : " << std::hex << node_ptr->start_pfn << "\n"
        << std::left << std::setw(20) << "  start_paddr : " << std::hex << (node_ptr->start_pfn << 12);

    PRINT("%s \n", oss.str().c_str());
}

/**
 * @brief Print zone information
 *
 * Displays detailed information about a memory zone including:
 * - Zone name and address
 * - Memory statistics (spanned, present, managed, reserved, holes)
 * - CMA pages (if available)
 * - Starting PFN and physical address
 * - Watermark levels (MIN, LOW, HIGH)
 * - Watermark boost value
 *
 * @param zone_ptr Pointer to zone structure
 */
void Buddy::print_zone_info(std::shared_ptr<zone> zone_ptr) {
    std::ostringstream oss;
    oss << std::left << "  " << zone_ptr->name << " zone:" << std::hex << zone_ptr->addr << "\n"
        << std::left << std::setw(20) << "    spanned   : " << std::dec << zone_ptr->spanned_pages
        << "(" << csize((uint64_t)zone_ptr->spanned_pages * page_size) << ") \n"
        << std::left << std::setw(20) << "    present   : " << std::dec << zone_ptr->present_pages
        << "(" << csize((uint64_t)zone_ptr->present_pages * page_size) << ") \n"
        << std::left << std::setw(20) << "    hole      : " << std::dec
        << zone_ptr->spanned_pages - zone_ptr->present_pages
        << "(" << csize((uint64_t)(zone_ptr->spanned_pages - zone_ptr->present_pages) * page_size) << ") \n"
        << std::left << std::setw(20) << "    managed   : " << std::dec << zone_ptr->managed_pages
        << "(" << csize(zone_ptr->managed_pages * page_size) << ") \n"
        << std::left << std::setw(20) << "    reserved  : " << std::dec
        << (zone_ptr->present_pages - zone_ptr->managed_pages)
        << "(" << csize((uint64_t)(zone_ptr->present_pages - zone_ptr->managed_pages) * page_size) << ") \n";

    if (field_offset(zone, cma_pages) > 0) {
        oss << std::left << std::setw(20) << "    cma_pages  : " << std::dec << zone_ptr->cma_pages
            << "(" << csize((uint64_t)zone_ptr->cma_pages * page_size) << ") \n";
    }

    oss << std::left << std::setw(20) << "    start_pfn  : " << std::hex << zone_ptr->start_pfn << "\n"
        << std::left << std::setw(20) << "    start_paddr: " << std::hex << (zone_ptr->start_pfn << 12) << "\n";

    if (field_offset(zone, watermark_boost) > 0) {
        oss << std::left << std::setw(20) << "    watermark_boost: " << std::dec
            << zone_ptr->watermark_boost << "\n";
    }

    oss << std::left << std::setw(20) << "    WMARK_HIGH: " << std::dec
        << zone_ptr->_watermark[zone_watermarks::WMARK_HIGH]
        << "(" << csize((uint64_t)zone_ptr->_watermark[zone_watermarks::WMARK_HIGH] * page_size) << ") \n"
        << std::left << std::setw(20) << "    WMARK_LOW : " << std::dec
        << zone_ptr->_watermark[zone_watermarks::WMARK_LOW]
        << "(" << csize((uint64_t)zone_ptr->_watermark[zone_watermarks::WMARK_LOW] * page_size) << ") \n"
        << std::left << std::setw(20) << "    WMARK_MIN : " << std::dec
        << zone_ptr->_watermark[zone_watermarks::WMARK_MIN]
        << "(" << csize((uint64_t)zone_ptr->_watermark[zone_watermarks::WMARK_MIN] * page_size) << ")";

    PRINT("%s \n", oss.str().c_str());
}

/**
 * @brief Print detailed zone information
 *
 * Displays detailed page-level information for a specific zone, including:
 * - All free pages organized by order and migration type
 * - Physical addresses of each free page
 *
 * This is useful for detailed analysis of memory fragmentation and
 * understanding exactly which pages are free.
 *
 * @param addr Zone address as hexadecimal string
 */
void Buddy::print_memory_zone(std::string addr) {
    std::ostringstream oss;
    unsigned long number = std::stoul(addr, nullptr, 16);

    if (number <= 0) {
        LOGE("Invalid zone address\n");
        return;
    }
    // Find the matching zone
    for (const auto& node_ptr : node_list) {
        for (const auto& zone_ptr : node_ptr->zone_list) {
            if (zone_ptr->addr != number) {
                continue;
            }
            // Iterate through all orders
            for (size_t o = 0; o < zone_ptr->free_areas.size(); o++) {
                std::shared_ptr<free_area> area_ptr = zone_ptr->free_areas[o];
                oss << "\nOrder[" << o << "] " << csize((1U << o) * page_size) << "\n";

                size_t free_list_cnt = area_ptr->free_list.size();
                if (free_list_cnt > migratetype_names.size()) {
                    free_list_cnt = migratetype_names.size();
                }

                // Iterate through all migration types
                for (size_t m = 0; m < free_list_cnt; m++) {
                    std::vector<ulong> page_list = area_ptr->free_list[m];
                    if (page_list.size() > 0) {
                        oss << "   migratetype:" << migratetype_names[m] << " Order[" << o << "]\n";
                    }

                    int index = 1;
                    for (const auto& page_addr : page_list) {
                        physaddr_t paddr = page_to_phy(page_addr);
                        oss << "     [" << std::setw(5) << std::setfill('0') << std::dec << index << "]"
                            << "Page:" << std::hex << page_addr << " "
                            << "PA:" << paddr << " \n";
                        index += 1;
                    }
                }
            }
        }
    }
    PRINT("%s \n", oss.str().c_str());
}

/**
 * @brief Print memory node configuration
 *
 * Displays comprehensive information about all NUMA nodes including:
 * - Global watermark configuration (min_free_kbytes, etc.)
 * - Detailed information for each node
 * - Detailed information for each zone within each node
 *
 * This provides a complete overview of the memory subsystem configuration.
 */
void Buddy::print_memory_node() {
    // Print configuration
    PRINT("\nConfig:\n");
    PRINT("---------------------------------------\n");
    std::ostringstream oss;
    oss << std::left << std::setw(25) << "min_free_kbytes       : " << min_free_kbytes << "kB \n"
        << std::left << std::setw(25) << "user_min_free_kbytes  : " << user_min_free_kbytes << "kB  \n"
        << std::left << std::setw(25) << "watermark_scale_factor: " << watermark_scale_factor;
    PRINT("%s \n", oss.str().c_str());
    PRINT("---------------------------------------\n\n");

    // Print node information
    PRINT("Node:\n");
    for (const auto& node_ptr : node_list) {
        PRINT("=======================================\n");
        print_node_info(node_ptr);
        PRINT("\n");

        // Print zone information for this node
        for (const auto& zone_ptr : node_ptr->zone_list) {
            // Skip empty zones
            if (zone_ptr->managed_pages == 0 || zone_ptr->spanned_pages == 0 ||
                zone_ptr->present_pages == 0) {
                LOGD("Skipping empty zone: %s\n", zone_ptr->name.c_str());
                continue;
            }

            print_zone_info(zone_ptr);
            PRINT("\n");
        }
    }
}

#pragma GCC diagnostic pop
