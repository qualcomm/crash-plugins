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

#include "pageowner.h"
#include <chrono>
#include "plugin.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(Pageowner)
#endif


Pageowner::Pageowner() {
    // Constructor implementation - currently empty
}

void Pageowner::init_offset(void) {
    // Offset initialization - currently empty
}

void Pageowner::init_command(void) {
    cmd_name = "pageowner";
    help_str_list = {
        "pageowner",                            /* command name */
        "dump page owner information",          /* short description */
        "-a                                display all allocated page owner information\n"
        "  pageowner -f                                display all freed page owner information\n"
        "  pageowner -s <address>                      display page owner info for specific address\n"
        "  pageowner -t                                display allocation statistics by call stack\n"
        "  pageowner -p                                display allocation statistics by process ID\n"
        "  pageowner -P <pid>                          display detailed statistics for specific PID\n"
        "  pageowner -H <handle>                       display detailed stack information for specific handle\n"
        "  \n"
        "  This command dumps the pageowner info and provides advanced statistics analysis.\n"
        "  The address parameter supports automatic type detection (PFN, physical, virtual, page).\n"
        "  Statistics features require page_owner to be enabled with stack tracing support.",
        "\n",
        "EXAMPLES",
        "  Display alloc stack for every page:",
        "    %s> pageowner -a",
        "    [1/387729] ALLOC: PFN:0xbffff~0xc0000 (1 page, 4KB) Page:0xfffffffe01ffffc0 PID:68 [kswapd0] 00:09:46.363353298 (uptime)",
        "          [<ffffffde2edb039c>] post_alloc_hook+0x20c",
        "          [<ffffffde2edb3064>] prep_new_page+0x28",
        "          [<ffffffde2edb46a4>] get_page_from_freelist+0x12ac",
        "          [<ffffffde2edb320c>] __alloc_pages+0xd8",
        "          [<ffffffde2bf61b9c>] zs_malloc+0x200",
        "          [<ffffffde2bf718e4>] zram_bvec_rw+0x2a8",
        "          [<ffffffde2bf7140c>] zram_rw_page.4e8b0154c58fc8baa75c3124f9a25b1c+0x9c",
        "          [<ffffffde2f0ddba4>] bdev_write_page+0x88",
        "          [<ffffffde2edc21cc>] __swap_writepage+0x64",
        "          [<ffffffde2edc2120>] swap_writepage+0x50",
        "          [<ffffffde2ed55dbc>] shrink_page_list+0xd18",
        "          [<ffffffde2ed56cb0>] reclaim_pages+0x1fc",
        "          [<ffffffde2edc132c>] madvise_cold_or_pageout_pte_range.50c4f95024e08bb75653a011da8190a2+0x79c",
        "          [<ffffffde2eda4f44>] walk_pgd_range+0x324",
        "          [<ffffffde2eda4a34>] walk_page_range+0x1cc",
        "          [<ffffffde2edbfe40>] madvise_vma_behavior.50c4f95024e08bb75653a011da8190a2+0x900",
        "\n",
        "  Display free stack for every page:",
        "    %s> pageowner -f",
        "    [1/944] FREE: PFN:0x99073~0x99074 (1 page, 4KB) Page:0xfffffffe01641cc0 PID:504 [lmkd] 00:00:12.745040350 (uptime)",
        "          [<ffffffde2edb11a0>] free_unref_page_prepare+0x2d8",
        "          [<ffffffde2edb1634>] free_unref_page_list+0xa0",
        "          [<ffffffde2ed56528>] shrink_page_list+0x1484",
        "          [<ffffffde2ed56cb0>] reclaim_pages+0x1fc",
        "          [<ffffffde2edc132c>] madvise_cold_or_pageout_pte_range.50c4f95024e08bb75653a011da8190a2+0x79c",
        "          [<ffffffde2eda4f44>] walk_pgd_range+0x324",
        "          [<ffffffde2eda4a34>] walk_page_range+0x1cc",
        "          [<ffffffde2edbfe40>] madvise_vma_behavior.50c4f95024e08bb75653a011da8190a2+0x900",
        "          [<ffffffde2edbf354>] do_madvise+0x168",
        "          [<ffffffde2edc015c>] __arm64_sys_process_madvise+0x150",
        "          [<ffffffde2eab6ad4>] invoke_syscall+0x5c",
        "          [<ffffffde2eab69d8>] el0_svc_common+0x94",
        "          [<ffffffde2eab68e4>] do_el0_svc+0x24",
        "          [<ffffffde2fa5dd2c>] el0_svc+0x30",
        "          [<ffffffde2fa5dcb0>] el0t_64_sync_handler+0x68",
        "          [<ffffffde2ea11624>] el0t_64_sync+0x1b4",
        "\n",
        "  Display the alloc and free stack for specific virtual address:",
        "    %s> pageowner -s 0x7fff12345000",
        "    Virtual address translation: 0x7fff12345000 -> PFN:0x4000e",
        "    ═══════════════════════════════════════════════════════════════",
        "           PAGE OWNER INFO FOR VIRTUAL ADDRESS 0x7fff12345000",
        "    ═══════════════════════════════════════════════════════════════",
        "    PFN Range:       0x4000e - 0x4000e",
        "    Page Address:    0xfffffffe00000380",
        "    Order:           0 (1 page, 4KB)",
        "    Current Status:  ALLOCATED",
        "    ═══════════════════════════════════════════════════════════════",
        "",
        "    ALLOCATION HISTORY:",
        "    Action:          ALLOC",
        "    PID:             1 [init]",
        "    Timestamp:       1d 00:53:20.156",
        "    GFP Mask:        0x400dc0",
        "",
        "    [<ffffffde2edb039c>] post_alloc_hook+0x20c",
        "    [<ffffffde2edb3064>] prep_new_page+0x28",
        "\n",
        "  Display the alloc memory size for every stack:",
        "    %s> pageowner -t",
        "    Allocated 19147 times, Total memory: 74.79MB",
        "        [<ffffffd4d55b039c>] post_alloc_hook+0x20c",
        "        [<ffffffd4d55b3064>] prep_new_page+0x28",
        "        [<ffffffd4d55b46a4>] get_page_from_freelist+0x12ac",
        "        [<ffffffd4d55b320c>] __alloc_pages+0xd8",
        "        [<ffffffd4d5549210>] page_cache_ra_unbounded+0x130",
        "        [<ffffffd4d5549754>] do_page_cache_ra+0x3c",
        "        [<ffffffd4d553b718>] do_sync_mmap_readahead+0x188",
        "        [<ffffffd4d553abc0>] filemap_fault+0x280",
        "        [<ffffffd4d5598b7c>] __do_fault+0x6c",
        "        [<ffffffd4d5598288>] handle_pte_fault+0x1b4",
        "        [<ffffffd4d5594820>] do_handle_mm_fault+0x4a0",
        "        [<ffffffd4d6297488>] do_page_fault.edea7eadbbe8ee1d4acc94c9444fd9d5+0x520",
        "        [<ffffffd4d6296f50>] do_translation_fault.edea7eadbbe8ee1d4acc94c9444fd9d5+0x44",
        "        [<ffffffd4d52cbd90>] do_mem_abort+0x64",
        "        [<ffffffd4d625ddf4>] el0_da+0x48",
        "        [<ffffffd4d625ebe0>] el0t_32_sync_handler+0x78",
        "        -------------------------------------------------",
        "        PID      Comm                 Times      Size",
        "        3867     binder:1067_18       3920       15.31MB",
        "        712      main                 2188       8.55MB",
        "        3791     unknow               1324       5.17MB",
        "        2023     ndroid.systemui      1035       4.04MB",
        "\n",
        "  Display allocation statistics by call stack (similar to SLUB):",
        "    %s> pageowner -t",
        "    ================================================================================",
        "                        PAGE OWNER ALLOCATION STATISTICS",
        "    ================================================================================",
        "    Call Stacks: 1247     | Total Allocations: 2847291    | Total Memory: 1.2GB",
        "    [1]Stack Handle:0x12856162 - Allocations: 164, Memory: 41.00KB",
        "       [<ffffffff811d4c5a>] __alloc_pages+0x12a/0x1b0",
        "       [<ffffffff812a8f3c>] alloc_pages+0x2c/0x60",
        "      Per-Process Breakdown:",
        "         PID        Comm                 Count      Memory",
        "         1234       init                 100        25.00KB",
        "         5678       kswapd0              64         16.00KB",
        "\n",
        "  Display allocation statistics by process ID:",
        "    %s> pageowner -p",
        "    ================================================================================",
        "                    PAGE OWNER ALLOCATION STATISTICS (Top 20 by memory usage)",
        "    ================================================================================",
        "    Process: 1247     | Total Allocations: 2847291    | Total Memory: 1.2GB",
        "    PID        Comm                 Allocations     Total Memory    Stacks",
        "    1234       init                 50110           17.20MB         25",
        "    5678       kswapd0              25055           8.60MB          12",
        "\n",
        "  Display detailed statistics for specific PID:",
        "    %s> pageowner -P 1234",
        "    ================================================================================",
        "                           Detailed PID Analysis",
        "    PID: 1234 [init] | Total Allocations: 50110 | Total Memory: 17.20MB | Stack count: 25",
        "    Stack Handle    Memory              Percentage",
        "    0x12856162      5.20MB              30.2%",
        "    0x98765432      3.80MB              22.1%",
        "    Use 'pageowner -H <handle>' to view detailed stack information.",
        "\n",
        "  Display detailed stack information for specific handle:",
        "    %s> pageowner -H 0x12856162",
        "    ================================================================================",
        "    Stack Information - Handle: 0x12856162",
        "    Total Allocations: 164 | Total Memory: 41.00KB",
        "      [<ffffffff811d4c5a>] __alloc_pages+0x12a",
        "      [<ffffffff812a8f3c>] alloc_pages+0x2c",
        "    PID        Comm                 Allocations     Memory",
        "    1234       init                 100             25.00KB",
        "    5678       kswapd0              64              16.00KB",
        "\n",
    };
}

/**
 * @brief Main entry point for the pageowner command
 *
 * This function processes command-line arguments and dispatches to the appropriate
 * handler based on the options provided. It supports various operations like
 * displaying allocated/freed pages, searching by address, and memory statistics.
 */
void Pageowner::cmd_main(void) {
    // Validate minimum argument count
    if (argcnt < 2) {
        LOGD("Insufficient arguments: argcnt=%d", argcnt);
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }

    // Verify page_owner is enabled in the kernel
    if (!is_enable_pageowner()) {
        LOGE("Page owner is not enabled in kernel");
        return;
    }

    // Lazy initialization: parse page owner data if not already done
    if (owner_map.empty()) {
        parser_pageowners();
    } else {
        LOGD("Using cached owner_map with %zu entries", owner_map.size());
    }

    // Process command-line options
    int c;
    int argerrs = 0;

    while ((c = getopt(argcnt, args, "afs:tpP:H:")) != EOF) {
        switch(c) {
            case 'a':   // Display all allocated page_owner info
                print_all_pages(false);
                break;

            case 'f':   // Display all freed page_owner info
                print_all_pages(true);
                break;

            case 's':   // Search page_owner info by address (auto-detect type)
                print_page_owner(std::string(optarg));
                break;

            case 't':   // Display allocation statistics by call stack
                print_alloc_mem_by_stack();
                break;

            case 'p':   // Display allocation statistics by process ID
                print_alloc_mem_by_pid();
                break;

            case 'P':   // Display detailed statistics for specific PID
                if (optarg) {
                    try {
                        size_t pid = std::stoull(optarg);
                        print_pid_details(pid);
                    } catch (const std::exception& e) {
                        LOGE("Invalid PID: %s", optarg);
                        argerrs++;
                    }
                } else {
                    LOGE("PID argument required for -P option");
                    argerrs++;
                }
                break;

            case 'H':   // Display detailed stack information for specific handle
                if (optarg) {
                    try {
                        unsigned int handle = std::stoul(optarg, nullptr, 16);
                        print_stack_info(handle);
                    } catch (const std::exception& e) {
                        LOGE("Invalid handle: %s", optarg);
                        argerrs++;
                    }
                } else {
                    LOGE("Handle argument required for -H option");
                    argerrs++;
                }
                break;

            default:
                LOGE("Unknown option: %c", c);
                argerrs++;
                break;
        }
    }

    // Display usage if there were argument errors
    if (argerrs) {
        cmd_usage(pc->curcmd, SYNOPSIS);
    }
}

/**
 * @brief Parse all page owner information from crash dump
 *
 * This function iterates through all physical page frames in the system and
 * extracts page owner information for pages that have tracking enabled.
 * It builds the owner_map which maps PFN to page_owner structures.
 *
 * This is a performance-critical function that may take significant time
 * for systems with large amounts of memory.
 */
void Pageowner::parser_pageowners() {
    LOGD("Starting to parse page owners from PFN range: 0x%lx - 0x%lx", min_low_pfn, max_pfn);

    size_t total_pfns = max_pfn - min_low_pfn;
    size_t processed = 0;
    size_t valid_owners = 0;
    size_t progress_interval = total_pfns / 10; // Log progress every 10%

    // Clear previous statistics and add deduplication set
    stack_statistics.clear();
    global_pid_statistics.clear();
    std::unordered_set<ulong> processed_po_addrs;  // For po_addr deduplication

    if (!is_enable_pageowner()) {
        PRINT("page_owner not enable !");
        return;
    }
    // Iterate through all page frame numbers in the system
    for (size_t pfn = min_low_pfn; pfn < max_pfn; pfn++) {
        processed++;

        // Log progress periodically for large memory systems
        if (progress_interval > 0 && processed % progress_interval == 0) {
            PRINT("Parsing progress: %zu/%zu PFNs (%.1f%%) \n",
                  processed, total_pfns, (processed * 100.0) / total_pfns);
        }
        // Convert PFN to page structure address
        ulong page = pfn_to_page(pfn);
        if (!is_kvaddr(page)) {
            LOGD("Invalid page address: 0x%lx\n", page);
            continue;
        }

        // Lookup page_ext structure
        ulong page_ext = lookup_page_ext(page);
        if (!is_kvaddr(page_ext)) {
            LOGD("Cannot find page_ext for PFN: 0x%lx\n", pfn);
            continue;
        }
        ulong po_addr = page_ext + page_ext_ops_offset;
        // Check for po_addr deduplication before collecting statistics
        if (processed_po_addrs.find(po_addr) != processed_po_addrs.end()) {
            LOGD("PFN 0x%zx: po_addr 0x%lx already processed, skipping statistics collection", pfn, po_addr);
            continue;  // Skip statistics collection for duplicate po_addr
        }
        // Mark this po_addr as processed
        processed_po_addrs.insert(po_addr);

        // Parse page_owner structure for this PFN
        std::shared_ptr<page_owner> owner_ptr = parse_page_owner_by_pfn(pfn);
        if (owner_ptr == nullptr) {
            LOGD("PFN 0x%zx: failed to parse page_owner", pfn);
            continue;
        }

        LOGD("pfn:%zu page_ext:%lx page_owner:%lx handle:%ld free_handle:%ld\n",
             pfn, owner_ptr->page_ext, owner_ptr->addr, (ulong)owner_ptr->handle, (ulong)owner_ptr->free_handle);

        // Track memory usage by internal structures
        page_owner_page_list.insert(owner_ptr->addr & page_mask);

        if (owner_ptr->stack_ptr != nullptr) {
            stack_record_page_list.insert(owner_ptr->stack_ptr->record_addr & page_mask);
        }

        // Store in the owner map for later retrieval
        owner_map[pfn] = owner_ptr;
        valid_owners++;

        // Collect statistics for this page owner (only for unique po_addr)
        collect_stack_statistics(owner_ptr);
    }

    LOGD("Parsing complete: processed %zu PFNs, found %zu valid page owners", processed, valid_owners);
    LOGD("page_owner structures use %zu pages (%s)",
         page_owner_page_list.size(), csize(page_owner_page_list.size() * page_size).c_str());
    LOGD("stack_record structures use %zu pages (%s)",
         stack_record_page_list.size(), csize(stack_record_page_list.size() * page_size).c_str());
}

/**
 * @brief Collect allocation statistics for stack and PID analysis
 * @param owner_ptr page_owner structure containing allocation information
 */
void Pageowner::collect_stack_statistics(std::shared_ptr<page_owner>& owner_ptr) {
    if (!owner_ptr || owner_ptr->handle <= 0 || owner_ptr->pid <= 0) {
        return;
    }

    // Only collect statistics for the first page of each allocation
    // to avoid counting the same allocation multiple times
    size_t alignment = 1UL << owner_ptr->order;
    if ((owner_ptr->pfn % alignment) != 0) {
        // This is not the first page of the allocation, skip statistics
        LOGD("Skipping statistics for PFN 0x%lx (not first page of order %d allocation)",
             owner_ptr->pfn, owner_ptr->order);
        return;
    }

    // Calculate page size for this allocation (full allocation size)
    size_t page_count = 1UL << owner_ptr->order;
    size_t alloc_size = page_count * page_size;

    // Update stack_statistics
    auto& stack_stat = stack_statistics[owner_ptr->handle];
    stack_stat.handle = owner_ptr->handle;
    stack_stat.total_allocations++;
    stack_stat.total_memory += alloc_size;

    // Update PID-level statistics within this stack
    auto& pid_stat = stack_stat.pid_stats[owner_ptr->pid];
    pid_stat.allocation_count++;
    pid_stat.total_memory += alloc_size;

    // Update global_pid_statistics
    auto& global_pid_stat = global_pid_statistics[owner_ptr->pid];
    global_pid_stat.allocation_count++;
    global_pid_stat.total_memory += alloc_size;

    // Track which stacks this PID uses
    global_pid_stat.stack_memory[owner_ptr->handle] += alloc_size;

    LOGD("Statistics updated: handle=%u, pid=%zu, size=%zu (first page of order %d allocation)",
         owner_ptr->handle, owner_ptr->pid, alloc_size, owner_ptr->order);
}

/**
 * @brief Determine the type of a given address
 * @param addr Address to analyze
 *
 * This function uses various heuristics to determine whether an address is:
 * - A Page Frame Number (PFN)
 * - A physical memory address
 * - A kernel page structure address
 * - A virtual address (kernel or user space)
 *
 * @return AddressType enum value indicating the detected type
 */
Pageowner::AddressType Pageowner::detect_address_type(ulonglong addr) {
    // Check if address is in valid PFN range
    // PFNs are typically small numbers representing page frame indices
    if (addr >= min_low_pfn && addr <= max_pfn) {
        ulong page = pfn_to_page(addr);
        if (is_kvaddr(page)) {
            LOGD("Address in PFN range [0x%lx, 0x%lx]", min_low_pfn, max_pfn);
            return ADDR_PFN;
        }
    }

    // Check if it's a kernel virtual address
    if (is_kvaddr(addr)) {
        // Distinguish between page structure address and other kernel addresses
        if (is_page_address(addr)) {
            LOGD("Address is a page address");
            return ADDR_PAGE;
        }
        LOGD("Address is a generic kernel virtual address");
        return ADDR_VIRTUAL;
    }

    // Check if it's a physical address
    if (is_physical_address(addr)) {
        LOGD("Address is a physical address");
        return ADDR_PHYSICAL;
    }

    // Try to translate as user virtual address from current task
    physaddr_t paddr = 0;
    if (CURRENT_TASK() && uvtop(CURRENT_CONTEXT(), addr, &paddr, 0)) {
        LOGD("Address translates in current task context to phys: 0x%llx", (ulonglong)paddr);
        return ADDR_VIRTUAL;
    }

    // Search through all tasks to find if this is a user virtual address
    struct task_context *tc;
    for (ulong i = 0; i < RUNNING_TASKS(); i++) {
        tc = FIRST_CONTEXT() + i;
        if (tc->task && uvtop(tc, addr, &paddr, 0)) {
            LOGD("Address found in task PID:%ld [%s], phys: 0x%llx",
                 task_to_pid(tc->task), tc->comm, (ulonglong)paddr);
            return ADDR_VIRTUAL;
        }
    }

    // Unable to determine address type
    LOGD("Unable to determine address type");
    return ADDR_UNKNOWN;
}

/**
 * @brief Check if address is a kernel page structure address
 * @param addr Address to check
 * @return true if address is a valid page structure
 */
bool Pageowner::is_page_address(ulonglong addr) {
    try {
        ulong pfn = page_to_pfn(addr);
        if (pfn >= min_low_pfn && pfn <= max_pfn) {
            ulong back_page = pfn_to_page(pfn);
            return (back_page == addr);
        }
    } catch (...) {
        // Ignore exceptions during validation
    }
    return false;
}

/**
 * @brief Check if address is a physical memory address
 * @param addr Address to check
 * @return true if address is in valid physical memory range
 */
bool Pageowner::is_physical_address(ulonglong addr) {
    ulong pfn = phy_to_pfn(addr);
    return (pfn >= min_low_pfn && pfn <= max_pfn);
}

/**
 * @brief Automatically detect address type and print page owner info
 * @param addr_str Address string in hexadecimal format
 *
 * This function automatically detects whether the provided address is a PFN,
 * physical address, page structure address, or virtual address, then displays
 * the corresponding page owner information.
 */
void Pageowner::print_page_owner(const std::string& addr_str) {
    // Validate input string
    if (addr_str.empty()) {
        LOGE("Empty address string provided");
        return;
    }

    // Parse hexadecimal address string
    ulonglong addr;
    try {
        addr = std::stoull(addr_str, nullptr, 16);
    } catch (const std::exception& e) {
        LOGE("Failed to parse address: %s", addr_str.c_str());
        return;
    }

    // Reject null address
    if (addr == 0) {
        LOGE("Null address provided");
        return;
    }

    // Detect address type and dispatch to appropriate handler
    AddressType addr_type = detect_address_type(addr);
    LOGD("Detected address type: %d", addr_type);

    switch (addr_type) {
        case ADDR_PFN:
            LOGD("Address 0x%llx identified as PFN", addr);
            print_page_owner(addr_str, INPUT_PFN);
            break;

        case ADDR_PHYSICAL:
            LOGD("Address 0x%llx identified as Physical Address", addr);
            print_page_owner(addr_str, INPUT_PHYS);
            break;

        case ADDR_PAGE:
            LOGD("Address 0x%llx identified as Page Structure Address", addr);
            print_page_owner(addr_str, INPUT_PAGE);
            break;

        case ADDR_VIRTUAL:
            LOGD("Address 0x%llx identified as Virtual Address", addr);
            print_page_owner(addr_str, INPUT_VADDR);
            break;

        case ADDR_UNKNOWN:
        default:
            LOGE("Cannot determine address type for 0x%llx", addr);
            break;
    }
}

/**
 * @brief Print page owner info for specific address with type flags
 * @param addr Address string in hexadecimal format
 * @param flags Input type flags (INPUT_PFN, INPUT_PHYS, etc.)
 */
void Pageowner::print_page_owner(const std::string& addr, int flags) {
    if (addr.empty()) {
        return;
    }
    ulonglong number;
    try {
        number = std::stoull(addr, nullptr, 16);
    } catch (const std::exception&) {
        LOGE("Invalid address format\n");
        return;
    }
    if (number == 0) {
        return;
    }
    std::shared_ptr<page_owner> owner_ptr;
    switch (flags) {
        case INPUT_PFN:
            owner_ptr = parse_page_owner_by_pfn(number);
            break;

        case INPUT_PHYS:
            owner_ptr = parse_page_owner_by_phys(number);
            break;

        case INPUT_PAGE:
            owner_ptr = parse_page_owner_by_page(number);
            break;

        case INPUT_VADDR:
            owner_ptr = parse_page_owner_by_vaddr(number);
            // PFN will be determined by the parsing function
            break;

        default:
            LOGE("Invalid input type\n");
            return;
    }

    if (!owner_ptr) {
        LOGE("Cannot parse page owner for address: 0x%llx\n", number);
        return;
    }

    bool currently_allocated = is_page_allocated(owner_ptr);

    // Print header
    PRINT("═══════════════════════════════════════════════════════════════\n");
    if (flags == INPUT_VADDR) {
        PRINT("           PAGE OWNER INFO FOR VIRTUAL ADDRESS 0x%llx\n", number);
    } else {
        PRINT("                    PAGE OWNER INFO FOR PFN:0x%lx\n", owner_ptr->pfn);
    }
    PRINT("═══════════════════════════════════════════════════════════════\n");

    ulong page_count = 1UL << owner_ptr->order;
    ulong total_size = page_count * page_size;

    PRINT("Order          :   %d (%lu page%s, %s)\n",
          owner_ptr->order, page_count, (page_count > 1) ? "s" : "", csize(total_size).c_str());
    PRINT("Current Status :   %s\n", currently_allocated ? "ALLOCATED" : "FREED");
    PRINT("═══════════════════════════════════════════════════════════════\n");

    // Print allocation history if available
    if (owner_ptr->handle > 0) {
        PRINT("ALLOCATION HISTORY:\n");
        print_page_owner(owner_ptr, false);
    }

    // Print free history if available
    if (owner_ptr->free_handle > 0) {
        PRINT("FREE HISTORY:\n");
        print_page_owner(owner_ptr, true);
    }

    PRINT("═══════════════════════════════════════════════════════════════\n");
}

/**
 * @brief Print detailed page owner information
 * @param owner_ptr page_owner structure to display
 * @param is_free true if showing free operation, false for allocation
 */
void Pageowner::print_page_owner(std::shared_ptr<page_owner> owner_ptr, bool is_free) {
    const unsigned int handle = is_free ? owner_ptr->free_handle : owner_ptr->handle;
    const ulong timestamp = is_free ? owner_ptr->free_ts_nsec : owner_ptr->ts_nsec;
    const char* action = is_free ? "FREE" : "ALLOC";

    std::string comm = owner_ptr->comm;
    if (comm.empty()) {
        struct task_context *tc = pid_to_context(owner_ptr->pid);
        if (tc) {
            comm = std::string(tc->comm);
        } else {
            comm = "unknown";
        }
    }

    std::string time_str = formatTimestamp(timestamp);
    std::shared_ptr<stack_record_t> record_ptr = get_stack_record(handle);

    if (record_ptr != nullptr) {
        PRINT("Status     :   %s\n", action);
        PRINT("PID        :   %zu [%s]\n", owner_ptr->pid, comm.c_str());
        PRINT("Timestamp  :   %s\n", time_str.c_str());
        PRINT("GFP Mask   :   0x%x\n", owner_ptr->gfp_mask);

        if (owner_ptr->last_migrate_reason >= 0) {
            PRINT("Migrate Reason:  %d\n", owner_ptr->last_migrate_reason);
        }

        std::string stack = get_call_stack(record_ptr);
        PRINT("%s \n", stack.c_str());
    }
}

/**
 * @brief Print all allocated or freed pages
 * @param show_freed true to show freed pages, false for allocated pages
 */
void Pageowner::print_all_pages(bool show_freed) {
    size_t page_count = 0;
    size_t total_pages = 0;
    size_t total_size = 0;

    for (const auto& pair : owner_map) {
        const auto& owner_ptr = pair.second;
        bool is_allocated = is_page_allocated(owner_ptr);
        // PRINT("pfn:%lx \n",pair.first);
        // PRINT("  page_ext:%llx \n",owner_ptr->page_ext);
        // PRINT("  page_owner:%llx \n",owner_ptr->addr);
        // PRINT("  status:%s \n",is_allocated ? "alloc":"free");
        // PRINT("  handle:%lx \n",owner_ptr->handle);
        // PRINT("  free_handle:%lx \n",owner_ptr->free_handle);
        // Check if this page matches our filter criteria
        bool should_count = show_freed ? !is_allocated : is_allocated;

        if (should_count) {
            page_count++;
            ulong pages_in_order = 1UL << owner_ptr->order;
            total_pages += pages_in_order;
            total_size += pages_in_order * page_size;
        }
    }

    // Print header with appropriate title
    PRINT("═══════════════════════════════════════════════════════════════\n");
    if (show_freed) {
        PRINT("                      FREED PAGES SUMMARY\n");
    } else {
        PRINT("                    ALLOCATED PAGES SUMMARY\n");
    }
    PRINT("═══════════════════════════════════════════════════════════════\n");

    // Print summary statistics
    if (show_freed) {
        PRINT("Total freed entries: %zu\n", page_count);
        PRINT("Total freed pages:   %zu\n", total_pages);
        PRINT("Total freed memory:  %s\n", csize(total_size).c_str());
    } else {
        PRINT("Total allocated entries: %zu\n", page_count);
        PRINT("Total allocated pages:   %zu\n", total_pages);
        PRINT("Total allocated memory:  %s\n", csize(total_size).c_str());
    }
    PRINT("═══════════════════════════════════════════════════════════════\n");

    // Second pass: print matching entries
    size_t entry_num = 0;
    for (const auto& pair : owner_map) {
        const auto& owner_ptr = pair.second;
        bool is_allocated = is_page_allocated(owner_ptr);

        // Check if this page matches our filter criteria
        bool should_print = show_freed ? !is_allocated : is_allocated;

        if (should_print) {
            entry_num++;
            print_page_owner(owner_ptr, show_freed, entry_num, page_count);
        }
    }
}

/**
 * @brief Print single page owner entry in list format
 * @param owner_ptr page_owner structure to display
 * @param is_free true if showing free operation, false for allocation
 * @param entry_num Current entry number in the list
 * @param total_entries Total number of entries in the list
 */
void Pageowner::print_page_owner(std::shared_ptr<page_owner> owner_ptr, bool is_free,
                                 size_t entry_num, size_t total_entries) {
    const ulong end_pfn = owner_ptr->pfn + (1UL << owner_ptr->order);
    const ulong page = pfn_to_page(owner_ptr->pfn);
    const ulong timestamp = is_free ? owner_ptr->free_ts_nsec : owner_ptr->ts_nsec;
    const char* action = is_free ? "FREE" : "ALLOC";
    ulong page_count = 1UL << owner_ptr->order;
    ulong total_size = page_count * page_size;

    std::string comm = owner_ptr->comm;
    if (comm.empty()) {
        struct task_context *tc = pid_to_context(owner_ptr->pid);
        if (tc) {
            comm = std::string(tc->comm);
        } else {
            comm = "unknown";
        }
    }

    std::string time_str = formatTimestamp(timestamp);
    std::shared_ptr<stack_record_t> record_ptr = owner_ptr->stack_ptr;
    if (record_ptr != nullptr) {
        PRINT("[%zu/%zu] %s: PFN:0x%lx~0x%lx (%lu page%s, %s) Page:0x%lx PID:%zu[%s] timestamp:(%ld) %s\n",
              entry_num, total_entries, action, owner_ptr->pfn, end_pfn, page_count,
              (page_count > 1) ? "s" : "", csize(total_size).c_str(),
              page, owner_ptr->pid, comm.c_str(), timestamp, time_str.c_str());

        std::string stack = get_call_stack(record_ptr);
        PRINT("%s \n", stack.c_str());
    }else{
        PRINT("record_ptr is null \n");
    }
}

/**
 * @brief Helper function to print formatted stack statistics
 * @param sorted_stacks Vector of sorted stack statistics
 * @param total_allocations Total allocation count across all stacks
 * @param total_memory Total memory usage across all stacks
 */
void Pageowner::print_stack_statistics_info(
    const std::vector<std::pair<unsigned int, PageownerStackStatistics>>& sorted_stacks,
    size_t total_allocations,
    size_t total_memory) {

    for (size_t i = 0; i < sorted_stacks.size(); i++) {
        const auto& stack_pair = sorted_stacks[i];
        const auto& handle = stack_pair.first;
        const auto& stats = stack_pair.second;

        PRINT("[%zu]Stack Handle:0x%x - Allocations: %zu, Memory: %s\n",
              i + 1, handle, stats.total_allocations, csize(stats.total_memory).c_str());

        // Show call stack
        std::shared_ptr<stack_record_t> record_ptr = get_stack_record(handle);
        if (record_ptr != nullptr) {
            std::string stack = get_call_stack(record_ptr);
            PRINT("%s", stack.c_str());
        }

        // Sort PIDs by memory usage within this stack
        std::vector<std::pair<size_t, PageownerPidStatistics>> sorted_pids;
        sorted_pids.reserve(stats.pid_stats.size());
        for (const auto& pid_pair : stats.pid_stats) {
            sorted_pids.emplace_back(pid_pair.first, pid_pair.second);
        }

        std::sort(sorted_pids.begin(), sorted_pids.end(),
                  [](const auto& a, const auto& b) {
                      return a.second.total_memory > b.second.total_memory;
                  });

        PRINT("  Per-Process Breakdown:\n");
        PRINT("     %-10s %-20s %-10s %-15s\n", "PID", "Comm", "Count", "Memory");
        PRINT("     %-10s %-20s %-10s %-15s\n", "---", "----", "-----", "------");

        // Show top 5 PIDs for this stack (or all if less than 5)
        size_t pid_display_count = std::min(sorted_pids.size(), static_cast<size_t>(5));
        for (size_t j = 0; j < pid_display_count; j++) {
            const auto& pid_pair = sorted_pids[j];
            const auto& pid = pid_pair.first;
            const auto& pid_stats = pid_pair.second;

            std::string comm = "unknown";
            struct task_context *tc = pid_to_context(pid);
            if (tc) {
                comm = std::string(tc->comm);
            }

            PRINT("     %-10zu %-20s %-10zu %-15s\n",
                  pid, comm.c_str(), pid_stats.allocation_count, csize(pid_stats.total_memory).c_str());
        }

        if (sorted_pids.size() > 5) {
            PRINT("     ... and %zu more processes\n", sorted_pids.size() - 5);
        }
        PRINT("\n");
    }
}

/**
 * @brief Display stack allocation statistics sorted by total memory usage
 */
void Pageowner::print_alloc_mem_by_stack() {
    if (stack_statistics.empty()) {
        PRINT("No stack statistics available. Ensure page_owner is enabled and data is parsed.\n");
        return;
    }

    // Convert to vector for sorting
    std::vector<std::pair<unsigned int, PageownerStackStatistics>> sorted_stacks;
    sorted_stacks.reserve(stack_statistics.size());

    for (const auto& stack_pair : stack_statistics) {
        sorted_stacks.emplace_back(stack_pair.first, stack_pair.second);
    }

    // Sort by total memory usage (descending)
    std::sort(sorted_stacks.begin(), sorted_stacks.end(),
              [](const auto& a, const auto& b) {
                  return a.second.total_memory > b.second.total_memory;
              });

    // Limit output if requested
    if (DEFAULT_TOP_COUNT > 0 && sorted_stacks.size() > DEFAULT_TOP_COUNT) {
        sorted_stacks.resize(DEFAULT_TOP_COUNT);
    }

    // Calculate totals
    size_t total_allocations = 0;
    size_t total_memory = 0;
    for (const auto& stack_pair : stack_statistics) {
        const auto& stats = stack_pair.second;
        total_allocations += stats.total_allocations;
        total_memory += stats.total_memory;
    }

    // Print enhanced header
    PRINT("================================================================================\n");
    PRINT("                        PAGE OWNER ALLOCATION STATISTICS\n");
    PRINT("================================================================================\n");
    PRINT("Call Stacks: %-8zu | Total Allocations: %-10zu | Total Memory: %s\n",
          stack_statistics.size(), total_allocations, csize(total_memory).c_str());
    PRINT("================================================================================\n");

    // Use the helper function to print stack statistics
    print_stack_statistics_info(sorted_stacks, total_allocations, total_memory);
}

/**
 * @brief Display global PID memory allocation statistics
 */
void Pageowner::print_alloc_mem_by_pid() {
    if (global_pid_statistics.empty()) {
        PRINT("No global PID statistics available. Ensure page_owner is enabled and data is parsed.\n");
        return;
    }

    // Convert to vector for sorting
    std::vector<std::pair<size_t, PageownerPidStatistics>> sorted_pids;
    sorted_pids.reserve(global_pid_statistics.size());

    for (const auto& pid_pair : global_pid_statistics) {
        sorted_pids.emplace_back(pid_pair.first, pid_pair.second);
    }

    // Sort by total memory usage (descending)
    std::sort(sorted_pids.begin(), sorted_pids.end(),
              [](const auto& a, const auto& b) {
                  return a.second.total_memory > b.second.total_memory;
              });

    // Limit output if requested
    if (DEFAULT_TOP_COUNT > 0 && sorted_pids.size() > DEFAULT_TOP_COUNT) {
        sorted_pids.resize(DEFAULT_TOP_COUNT);
    }

    // Calculate totals
    size_t total_allocations = 0;
    size_t total_memory = 0;
    for (const auto& pid_pair : global_pid_statistics) {
        const auto& stats = pid_pair.second;
        total_allocations += stats.allocation_count;
        total_memory += stats.total_memory;
    }

    // Print enhanced header
    PRINT("================================================================================\n");
    PRINT("                    PAGE OWNER ALLOCATION STATISTICS (Top %zu by memory usage)\n", DEFAULT_TOP_COUNT);
    PRINT("================================================================================\n");
    PRINT("Process: %-8zu | Total Allocations: %-10zu | Total Memory: %s\n",
          global_pid_statistics.size(), total_allocations, csize(total_memory).c_str());
    PRINT("================================================================================\n");

    // Print header
    PRINT("%-10s %-20s %-15s %-15s %-10s\n",
          "PID", "Comm", "Allocations", "Total Memory", "Stacks");
    PRINT("%-10s %-20s %-15s %-15s %-10s\n",
          "---", "----", "-----------", "------------", "------");

    // Print data rows
    for (const auto& pid_pair : sorted_pids) {
        size_t pid = pid_pair.first;
        const auto& stats = pid_pair.second;

        std::string comm = "unknown";
        struct task_context *tc = pid_to_context(pid);
        if (tc) {
            comm = std::string(tc->comm);
        }

        PRINT("%-10zu %-20s %-15zu %-15s %-10zu\n",
              pid,
              comm.c_str(),
              stats.allocation_count,
              csize(stats.total_memory).c_str(),
              stats.stack_memory.size());
    }
}

/**
 * @brief Display detailed memory allocation for a specific PID
 * @param pid Process ID to analyze
 */
void Pageowner::print_pid_details(size_t pid) {
    auto it = global_pid_statistics.find(pid);
    if (it == global_pid_statistics.end()) {
        PRINT("PID %zu not found in global statistics\n", pid);
        return;
    }

    const auto& stats = it->second;

    std::string comm = "unknown";
    struct task_context *tc = pid_to_context(pid);
    if (tc) {
        comm = std::string(tc->comm);
    }

    PRINT("================================================================================\n");
    PRINT("                           Detailed PID Analysis\n");
    PRINT("PID: %zu [%s] | Total Allocations: %zu | Total Memory: %s | Stack count: %zu\n",
          pid, comm.c_str(), stats.allocation_count, csize(stats.total_memory).c_str(), stats.stack_memory.size());
    PRINT("================================================================================\n");

    // Display stack breakdown sorted by memory usage
    if (!stats.stack_memory.empty()) {
        PRINT("%-12s    %-18s    %-12s\n", "Stack Handle", "Memory", "Percentage");
        PRINT("%-12s    %-18s    %-12s\n", "------------", "------", "----------");

        // Sort stacks by memory usage
        std::vector<std::pair<unsigned int, size_t>> sorted_stacks;
        sorted_stacks.reserve(stats.stack_memory.size());

        for (const auto& stack_pair : stats.stack_memory) {
            sorted_stacks.emplace_back(stack_pair.first, stack_pair.second);
        }

        std::sort(sorted_stacks.begin(), sorted_stacks.end(),
                  [](const auto& a, const auto& b) {
                      return a.second > b.second;
                  });

        // Show top stacks
        size_t display_count = std::min(sorted_stacks.size(), DEFAULT_DETAIL_COUNT);
        for (size_t i = 0; i < display_count; i++) {
            const auto& stack_pair = sorted_stacks[i];
            unsigned int handle = stack_pair.first;
            size_t stack_memory = stack_pair.second;

            double percentage = stats.total_memory > 0 ?
                               (static_cast<double>(stack_memory) / stats.total_memory) * 100.0 : 0.0;

            PRINT("0x%-10x    %-18s    %.1f%%\n",
                  handle,
                  csize(stack_memory).c_str(),
                  percentage);
        }

        if (sorted_stacks.size() > DEFAULT_DETAIL_COUNT) {
            PRINT("... and %zu more stacks\n", sorted_stacks.size() - DEFAULT_DETAIL_COUNT);
        }

        PRINT("\nUse 'pageowner -H <handle>' to view detailed stack information.\n");
    }

    PRINT("================================================================================\n");
}

/**
 * @brief Display detailed stack information for a specific handle
 * @param handle Stack handle ID to display
 */
void Pageowner::print_stack_info(unsigned int handle) {
    // Find the stack by handle
    auto it = stack_statistics.find(handle);
    if (it == stack_statistics.end()) {
        PRINT("Stack handle 0x%x not found in statistics\n", handle);
        return;
    }

    const auto& stats = it->second;

    PRINT("================================================================================\n");
    PRINT("Stack Information - Handle: 0x%x\n", handle);
    PRINT("Total Allocations: %zu | Total Memory: %s\n", stats.total_allocations, csize(stats.total_memory).c_str());
    PRINT("================================================================================\n");

    // Show call stack
    std::shared_ptr<stack_record_t> record_ptr = get_stack_record(handle);
    if (record_ptr != nullptr) {
        std::string stack = get_call_stack(record_ptr);
        PRINT("%s", stack.c_str());
    }

    // Sort PIDs by memory usage
    std::vector<std::pair<size_t, PageownerPidStatistics>> sorted_pids;
    sorted_pids.reserve(stats.pid_stats.size());

    for (const auto& pid_pair : stats.pid_stats) {
        sorted_pids.emplace_back(pid_pair.first, pid_pair.second);
    }

    std::sort(sorted_pids.begin(), sorted_pids.end(),
              [](const auto& a, const auto& b) {
                  return a.second.total_memory > b.second.total_memory;
              });

    PRINT("%-10s %-20s %-15s %-15s\n", "PID", "Comm", "Allocations", "Memory");
    PRINT("%-10s %-20s %-15s %-15s\n", "---", "----", "-----------", "------");

    for (const auto& pid_pair : sorted_pids) {
        size_t pid = pid_pair.first;
        const auto& pid_stats = pid_pair.second;

        std::string comm = "unknown";
        struct task_context *tc = pid_to_context(pid);
        if (tc) {
            comm = std::string(tc->comm);
        }

        PRINT("%-10zu %-20s %-15zu %-15s\n",
              pid,
              comm.c_str(),
              pid_stats.allocation_count,
              csize(pid_stats.total_memory).c_str());
    }

    PRINT("================================================================================\n");
}

#pragma GCC diagnostic pop
