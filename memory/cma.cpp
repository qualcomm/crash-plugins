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

#include "cma.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(Cma)
#endif

/**
 * @brief Main command entry point for CMA analysis
 *
 * Parses command line arguments and dispatches to appropriate handler functions:
 * -a: Display all CMA areas with allocation statistics
 * -u <name>: Show allocated pages for specific CMA area
 * -f <name>: Show free pages for specific CMA area
 */
void Cma::cmd_main(void) {
    // Check minimum argument count
    if (argcnt < 2) {
        LOGD("Insufficient arguments provided, showing usage\n");
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }

    // Parse CMA areas if not already done
    if (mem_list.empty()) {
        LOGD("CMA areas list is empty, parsing CMA areas from kernel\n");
        parser_cma_areas();
    } else {
        LOGD("Using cached CMA areas list with %zu areas\n", mem_list.size());
    }
    int argerrs = 0;
    int c;
    // Parse command line options
    while ((c = getopt(argcnt, args, "ap:b:")) != EOF) {
        switch(c) {
            case 'a':
                LOGD("Executing print_cma_areas() - display all CMA areas\n");
                print_cma_areas();
                break;
            case 'p':
                print_cma_page_status(std::string(optarg));
                break;
            case 'b':
                print_cma_alloc_page_stack(std::string(optarg));
                break;
            default:
                LOGD("Unknown option: -%c\n", c);
                argerrs++;
                break;
        }
    }
    // Handle argument errors
    if (argerrs) {
        LOGE("Command line argument errors detected: %d\n", argerrs);
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }
}

/**
 * @brief Initialize kernel structure field offsets
 *
 * Sets up field offsets for CMA structure fields used in CMA analysis.
 * These offsets are essential for reading CMA data from kernel memory.
 */
void Cma::init_offset(void) {
    // Initialize CMA structure field offsets
    field_init(cma,base_pfn);       // Starting page frame number
    field_init(cma,count);          // Number of pages in CMA area
    field_init(cma,bitmap);         // Allocation bitmap address
    field_init(cma,order_per_bit);  // Pages per bitmap bit
    field_init(cma,name);           // CMA area name
    struct_init(cma);               // Initialize CMA structure size
}

/**
 * @brief Initialize command help and usage information
 *
 * Sets up the command name, description, and detailed help text including
 * usage examples and expected output formats for the CMA plugin.
 */
void Cma::init_command(void) {
    cmd_name = "cma";
    help_str_list={
        "cma",                                /* command name */
        "display CMA (Contiguous Memory Allocator) information",  /* short description */
        "[-a] [-p physical_addr] [-b cma_name]\n"
        "  This command displays CMA memory allocation information.\n"
        "\n"
        "    -a              display all CMA areas with allocation statistics\n"
        "    -p physical_addr  check the status of a specific page by physical address\n"
        "    -b cma_name     display allocated CMA pages with pageowner stack traces\n",
        "\n",
        "EXAMPLES",
        "  List all CMA areas with detailed information:",
        "    %s> cma -a",
        "    CMA (Contiguous Memory Allocator) Areas Overview",
        "    ┌────┬───────────────────────────┬──────────────────┬───────────────────────────┬──────────┬──────────┬────────┬──────┐",
        "    │  # │CMA Area Name              │CMA Address       │Physical Memory Range      │Total Size│ Used Size│  Usage%│ Order│",
        "    ├────┼───────────────────────────┼──────────────────┼───────────────────────────┼──────────┼──────────┼────────┼──────┤",
        "    │  1 │qdss_apps_region@82800000  │0xffffffdd7b88e298│[0x082800000~0x084800000]  │      32MB│        0B│    0.0%│     0│",
        "    │  2 │linux,cma                  │0xffffffdd7b88e188│[0x0f6800000~0x0f8800000]  │      32MB│   17.66MB│   55.2%│     0│",
        "    │  3 │qseecom_ta_region          │0xffffffdd7b88e078│[0x0f8800000~0x0f9800000]  │      16MB│        0B│    0.0%│     0│",
        "    │  4 │adsp_heap_region           │0xffffffdd7b88df68│[0x0f9800000~0x0fa400000]  │      12MB│    5.20MB│   43.3%│     0│",
        "    │  5 │qseecom_region             │0xffffffdd7b88de58│[0x0fa400000~0x0fb800000]  │      20MB│    1.06MB│    5.3%│     0│",
        "    │  6 │va_md_mem_region           │0xffffffdd7b88dc38│[0x0fb800000~0x0fc800000]  │      16MB│        0B│    0.0%│     0│",
        "    │  7 │secure_cdsp_region         │0xffffffdd7b88db28│[0x0fd400000~0x0ffc00000]  │      40MB│        0B│    0.0%│     0│",
        "    │  8 │mem_dump_region            │0xffffffdd7b88dd48│[0x97e400000~0x97fc00000]  │      24MB│   20.66MB│   86.1%│     0│",
        "    ├────┴───────────────────────────┴──────────────────┴───────────────────────────┴──────────┴──────────┴────────┴──────┤",
        "    │ Areas: 8 | Total: 192MB | Used: 44.57MB (23.2%)                                                                     │",
        "    └─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘",
        "\n",
        "  Check page status by physical addr:",
        "    %s> cma -p f9805000",
        "    ┌──────────────────────────────────────────────────────────────────────────────┐",
        "    │  CMA Region:              adsp_heap_region                                   │",
        "    │  Total Size:              12 MB                                              │",
        "    │  Address Range:           0x00000000f9800000 ~ 0x00000000fa400000            │",
        "    └──────────────────────────────────────────────────────────────────────────────┘",
        "    Page Information:",
        "    ┌──────────────────────────────────────────────────────────────────────────────┐",
        "    │  Bit Index:               5                                                  │",
        "    │  Page Range:              0x00000000f9805000 ~ 0x00000000f9806000            │",
        "    │  Page Size:               4 KB                                               │",
        "    │  Status:                  ALLOCATED                                          │",
        "    └──────────────────────────────────────────────────────────────────────────────┘",
        "\n",
        "  Display allocated pages with stack traces for specific CMA area:",
        "    %s> cma -b linux,cma",
        "    ================================================================================",
        "                CMA ALLOCATED PAGES WITH STACK TRACES - linux,cma",
        "    ================================================================================",
        "    CMA Area: linux,cma | Total Allocated Pages: 4608 | Total Memory: 17.66MB",
        "    ================================================================================",
        "      [1/4608] PFN: 0xf6800~0xf6801 (4KB) - PID: 1234 [kswapd0] - 1d 00:53:20.156 - GFP:0x400dc0",
        "        [<ffffffde2edb039c>] post_alloc_hook+0x20c",
        "        [<ffffffde2edb3064>] prep_new_page+0x28",
        "\n",
    };
}

/**
 * @brief Default constructor
 *
 * Initializes the CMA plugin with default settings.
 */
Cma::Cma(){

}

/**
 * @brief Parse and collect CMA area information from kernel memory
 *
 * Reads the kernel's CMA areas array and extracts detailed information
 * about each CMA area including size, location, allocation bitmap, and
 * current usage statistics.
 */
void Cma::parser_cma_areas(){
    if (!csymbol_exists("cma_areas")){
        PRINT("cma_areas doesn't exist in this kernel!\n");
        return;
    }
    ulong cma_areas_addr = csymbol_value("cma_areas");
    if (!is_kvaddr(cma_areas_addr)) {
        PRINT("cma_areas address is invalid!\n");
        return;
    }
    ulong cma_area_count = read_ulong(csymbol_value("cma_area_count"),"cma_area_count");
    if (cma_area_count == 0) {
        PRINT("cma_area_count is zero!\n");
        return;
    }
    for (ulong i = 0; i < cma_area_count; ++i) {
        ulong cma_addr = cma_areas_addr + i * struct_size(cma);
        void *cma_buf = read_struct(cma_addr,"cma");
        if (!cma_buf) {
            PRINT("Failed to read cma structure at address %lx\n", cma_addr);
            continue;
        }
        std::shared_ptr<cma_mem> cma_ptr = std::make_shared<cma_mem>();
        cma_ptr->addr = cma_addr;
        cma_ptr->base_pfn = ULONG(cma_buf + field_offset(cma,base_pfn));
        cma_ptr->count = ULONG(cma_buf + field_offset(cma,count));
        cma_ptr->bitmap = ULONG(cma_buf + field_offset(cma,bitmap));
        cma_ptr->order_per_bit = UINT(cma_buf + field_offset(cma,order_per_bit));
        // read cma name
        if (THIS_KERNEL_VERSION >= LINUX(5,10,0)){
            char cma_name[64];
            memcpy(cma_name,cma_buf + field_offset(cma,name),64);
            cma_ptr->name = cma_name;
        }else{
            ulong name_addr = ULONG(cma_buf + field_offset(cma,name));
            cma_ptr->name = read_cstring(name_addr,64, "cma_name");
        }
        cma_ptr->allocated_size = get_cma_used_size(cma_ptr);
        FREEBUF(cma_buf);
        mem_list.push_back(cma_ptr);
    }
}

CMAStatistics Cma::calculate_cma_statistics() {
    CMAStatistics stats = {};
    stats.total_areas = mem_list.size();
    for (size_t i = 0; i < mem_list.size(); ++i) {
        const std::shared_ptr<cma_mem>& cma = mem_list[i];
        ulonglong area_size = cma->count * page_size;
        stats.total_size += area_size;
        stats.total_used += cma->allocated_size;
    }
    stats.overall_usage_percent = stats.total_size > 0 ?
        (stats.total_used * 100.0 / stats.total_size) : 0.0;
    return stats;
}

std::string Cma::format_address_range(ulong base_pfn, ulong count) {
    char buffer[64];
    ulong start_addr = base_pfn << 12;
    ulong end_addr = (base_pfn + count) << 12;
    snprintf(buffer, sizeof(buffer), "[0x%09lx~0x%09lx]", start_addr, end_addr);
    return std::string(buffer);
}

ColumnWidths Cma::calculate_optimal_column_widths() {
    ColumnWidths widths;
    for (const std::shared_ptr<cma_mem>& cma : mem_list) {
        // Name column
        widths.name_width = std::max(widths.name_width,
                                   static_cast<int>(cma->name.length()) + 2);
        // Range column (address range)
        std::string range_str = format_address_range(cma->base_pfn, cma->count);
        widths.range_width = std::max(widths.range_width,
                                    static_cast<int>(range_str.length()) + 2);
        // Size column
        std::string size_str = csize(cma->count * page_size);
        widths.size_width = std::max(widths.size_width,
                                   static_cast<int>(size_str.length()) + 1);
        // Used column
        std::string used_str = csize(cma->allocated_size);
        widths.used_width = std::max(widths.used_width,
                                   static_cast<int>(used_str.length()) + 1);
    }
    return widths;
}

void Cma::print_cma_table_header(const ColumnWidths& widths) {
    PRINT("CMA (Contiguous Memory Allocator) Areas Overview\n");
    PRINT("┌");
    PRINT("────┬"); // index
    for (int i = 0; i < widths.name_width; i++) PRINT("─");
    PRINT("┬");
    for (int i = 0; i < widths.addr_width; i++) PRINT("─");
    PRINT("┬");
    for (int i = 0; i < widths.range_width; i++) PRINT("─");
    PRINT("┬");
    for (int i = 0; i < widths.size_width; i++) PRINT("─");
    PRINT("┬");
    for (int i = 0; i < widths.used_width; i++) PRINT("─");
    PRINT("┬");
    for (int i = 0; i < widths.percent_width; i++) PRINT("─");
    PRINT("┬");
    for (int i = 0; i < widths.order_width; i++) PRINT("─");
    PRINT("┐\n");
    PRINT("│%3s │%-*s│%-*s│%-*s│%*s│%*s│%*s│%*s│\n",
            "#",
            widths.name_width, "CMA Area Name",
            widths.addr_width, "CMA Address",
            widths.range_width, "Physical Memory Range",
            widths.size_width, "Total Size",
            widths.used_width, "Used Size",
            widths.percent_width, "Usage%",
            widths.order_width, "Order");
    PRINT("├────┼");
    for (int i = 0; i < widths.name_width; i++) PRINT("─");
    PRINT("┼");
    for (int i = 0; i < widths.addr_width; i++) PRINT("─");
    PRINT("┼");
    for (int i = 0; i < widths.range_width; i++) PRINT("─");
    PRINT("┼");
    for (int i = 0; i < widths.size_width; i++) PRINT("─");
    PRINT("┼");
    for (int i = 0; i < widths.used_width; i++) PRINT("─");
    PRINT("┼");
    for (int i = 0; i < widths.percent_width; i++) PRINT("─");
    PRINT("┼");
    for (int i = 0; i < widths.order_width; i++) PRINT("─");
    PRINT("┤\n");
}

void Cma::print_cma_table_content(const ColumnWidths& widths) {
    std::vector<std::shared_ptr<cma_mem>> sorted_cmas = mem_list;
    std::sort(sorted_cmas.begin(), sorted_cmas.end(),
              [](const std::shared_ptr<cma_mem>& a, const std::shared_ptr<cma_mem>& b) {
                  return a->base_pfn < b->base_pfn;
              });
    for (size_t i = 0; i < sorted_cmas.size(); ++i) {
        const std::shared_ptr<cma_mem>& cma = sorted_cmas[i];
        ulonglong total_size = cma->count * page_size;
        double usage_percent = total_size > 0 ?
            (cma->allocated_size * 100.0 / total_size) : 0.0;
        std::string range_str = format_address_range(cma->base_pfn, cma->count);
        std::string size_str = csize(total_size);
        std::string used_str = csize(cma->allocated_size);
        PRINT("│%3zu │%-*s│0x%014lx│%-*s│%*s│%*s│%*.1f%%│%*d│\n",
                i + 1,
                widths.name_width, cma->name.c_str(),
                cma->addr,
                widths.range_width, range_str.c_str(),
                widths.size_width, size_str.c_str(),
                widths.used_width, used_str.c_str(),
                widths.percent_width-1, usage_percent,
                widths.order_width, cma->order_per_bit);
    }
}

void Cma::print_cma_statistics(const ColumnWidths& widths) {
    CMAStatistics stats = calculate_cma_statistics();
    PRINT("├────┴");
    for (int i = 0; i < widths.name_width; i++) PRINT("─");
    PRINT("┴");
    for (int i = 0; i < widths.addr_width; i++) PRINT("─");
    PRINT("┴");
    for (int i = 0; i < widths.range_width; i++) PRINT("─");
    PRINT("┴");
    for (int i = 0; i < widths.size_width; i++) PRINT("─");
    PRINT("┴");
    for (int i = 0; i < widths.used_width; i++) PRINT("─");
    PRINT("┴");
    for (int i = 0; i < widths.percent_width; i++) PRINT("─");
    PRINT("┴");
    for (int i = 0; i < widths.order_width; i++) PRINT("─");
    PRINT("┤\n");
    int total_content_width = widths.index_width + widths.name_width + widths.addr_width +
                             widths.range_width + widths.size_width +
                             widths.used_width + widths.percent_width +
                             widths.order_width + 7;
    char stats_line[256];
    snprintf(stats_line, sizeof(stats_line),
             "Areas: %zu | Total: %s | Used: %s (%.1f%%)",
             stats.total_areas,
             csize(stats.total_size).c_str(),
             csize(stats.total_used).c_str(),
             stats.overall_usage_percent);
    PRINT("│ %-*s │\n", total_content_width-2, stats_line);
    PRINT("└");
    for (int i = 0; i < total_content_width; i++) {
        PRINT("─");
    }
    PRINT("┘\n");
}

/**
 * @brief Display overview of all CMA areas in formatted table
 *
 * Generates a comprehensive table showing all CMA areas with their
 * allocation statistics, usage percentages, and summary information.
 * Uses dynamic column width calculation for optimal display formatting.
 */
void Cma::print_cma_areas(){
    // Check if we have any CMA areas to display
    if (mem_list.empty()) {
        LOGE("No CMA areas found in mem_list\n");
        return;
    }
    LOGD("Displaying %zu CMA areas\n", mem_list.size());
    // Calculate optimal column widths based on content
    ColumnWidths widths = calculate_optimal_column_widths();

    // Display formatted table
    print_cma_table_header(widths);
    print_cma_table_content(widths);
    print_cma_statistics(widths);
}

/**
 * @brief Calculate allocated size for a CMA area by analyzing its bitmap
 *
 * Reads the allocation bitmap for a CMA area and counts the number of
 * allocated bits to determine the total allocated memory size.
 *
 * @param cma Shared pointer to CMA area structure
 * @return Total allocated size in bytes
 */
size_t Cma::get_cma_used_size(const std::shared_ptr<cma_mem>& cma) {
    // Calculate bitmap size in bytes
    size_t nr_byte = (cma->count >> cma->order_per_bit) / 8;
    size_t per_bit_size = (1U << cma->order_per_bit) * page_size;

    size_t used_count = 0;
    ulong bitmap_addr = cma->bitmap;

    // Scan through bitmap to count allocated bits
    for (size_t i = 0; i < nr_byte; ++i) {
        unsigned char bitmap_data = read_byte(bitmap_addr, "cma bitmap");
        std::bitset<8> bits(bitmap_data);
        size_t nr_bit = bits.count();
        LOGD("Bitmap byte %zu: addr=0x%lx, data=0x%02x, set_bits=%zu\n",i, bitmap_addr, bitmap_data, nr_bit);

        used_count += nr_bit;
        bitmap_addr += 1;
        // Log progress for large bitmaps
        if (i > 0 && i % 100 == 0) {
            LOGD("Processed %zu bitmap bytes, current used_count=%zu\n", i, used_count);
        }
    }
    size_t total_used_size = used_count * per_bit_size;
    return total_used_size;
}

void Cma::print_cma_alloc_page_stack(const std::string& cma_name){
    // Check if page_owner is enabled in the kernel
    if (!is_enable_pageowner()) {
        PRINT("Page owner is not enabled in kernel. Cannot display stack traces.\n");
        return;
    }

    // Check if we have any CMA areas to analyze
    if (mem_list.empty()) {
        PRINT("No CMA areas found. Please run 'cma -a' first to parse CMA areas.\n");
        return;
    }

    // Find the specified CMA area
    std::shared_ptr<cma_mem> target_cma = nullptr;
    for (const auto& cma : mem_list) {
        if (cma->name == cma_name) {
            target_cma = cma;
            break;
        }
    }

    if (!target_cma) {
        PRINT("CMA area '%s' not found. Available CMA areas:\n", cma_name.c_str());
        for (const auto& cma : mem_list) {
            PRINT("  - %s\n", cma->name.c_str());
        }
        return;
    }

    LOGD("Starting CMA allocated pages stack trace analysis for area: %s", cma_name.c_str());

    // Statistics counters for the specific area
    size_t area_allocated_pages = 0;

    // Print header
    PRINT("================================================================================\n");
    PRINT("                CMA ALLOCATED PAGES WITH STACK TRACES - %s\n", cma_name.c_str());
    PRINT("================================================================================\n");

    // Calculate bitmap parameters
    size_t per_nr_pages = (1U << target_cma->order_per_bit);
    size_t bitmap_bits = target_cma->count >> target_cma->order_per_bit;

    // First pass: collect statistics for the specific area
    for (size_t bit_index = 0; bit_index < bitmap_bits; ++bit_index) {
        // Calculate PFN for the first page of this allocation block
        ulong pfn = target_cma->base_pfn + bit_index * per_nr_pages;

        // Use existing is_page_allocated function to check allocation status
        if (is_cma_page_allocated(target_cma, pfn)) {
            area_allocated_pages += per_nr_pages;
        }
    }

    // Print summary statistics
    PRINT("CMA Area: %s | Total Allocated Pages: %zu | Total Memory: %s\n",
          cma_name.c_str(), area_allocated_pages, csize(area_allocated_pages * page_size).c_str());
    PRINT("================================================================================\n");

    // Second pass: collect and display detailed information
    std::vector<std::shared_ptr<page_owner>> allocated_pages;

    for (size_t bit_index = 0; bit_index < bitmap_bits; ++bit_index) {
        // Calculate PFN for the first page of this allocation block
        ulong pfn = target_cma->base_pfn + bit_index * per_nr_pages;

        // Use existing is_page_allocated function to check allocation status
        if (is_cma_page_allocated(target_cma, pfn)) {
            // Get pageowner info for the first page of this allocation
            std::shared_ptr<page_owner> owner_ptr = parse_page_owner_by_pfn(pfn);
            if (owner_ptr) {
                allocated_pages.push_back(owner_ptr);
            }
        }
    }

    // Check if we have any allocations to display
    if (allocated_pages.empty()) {
        PRINT("No allocated pages found in CMA area '%s'.\n", cma_name.c_str());
        return;
    }
    // Display each allocated page with stack trace
    size_t entry_num = 0;
    for (const auto& owner_ptr : allocated_pages) {
        entry_num++;

        // Calculate page range for this allocation
        ulong page_count = 1UL << owner_ptr->order;
        ulong end_pfn = owner_ptr->pfn + page_count;

        // Get process information
        std::string comm = owner_ptr->comm;
        if (comm.empty()) {
            struct task_context *tc = pid_to_context(owner_ptr->pid);
            if (tc) {
                comm = std::string(tc->comm);
            } else {
                comm = "unknown";
            }
        }

        // Print page allocation info with timestamp and GFP mask in one line
        std::string time_str = "";
        if (owner_ptr->ts_nsec > 0) {
            time_str = formatTimestamp(owner_ptr->ts_nsec);
        }

        std::string gfp_str = "";
        if (owner_ptr->gfp_mask > 0) {
            char gfp_buf[32];
            snprintf(gfp_buf, sizeof(gfp_buf), "GFP:0x%x", owner_ptr->gfp_mask);
            gfp_str = gfp_buf;
        }

        PRINT("[%zu/%zu]PFN:0x%lx~0x%lx (%lu page%s, %s) - PID:%zu [%s] %s %s\n",
              entry_num, allocated_pages.size(),
              owner_ptr->pfn, end_pfn,
              page_count, (page_count > 1) ? "s" : "",
              csize(page_count * page_size).c_str(),
              owner_ptr->pid, comm.c_str(),
              gfp_str.c_str(),time_str.c_str());

        // Print allocation stack trace
        if (is_page_allocated(owner_ptr)) {
            std::shared_ptr<stack_record_t> record_ptr = get_stack_record(owner_ptr->handle);
            if (record_ptr != nullptr) {
                std::string stack = get_call_stack(record_ptr);
                // Indent each line of the stack trace
                std::istringstream iss(stack);
                std::string line;
                while (std::getline(iss, line)) {
                    if (!line.empty()) {
                        PRINT("    %s\n", line.c_str());
                    }
                }
            } else {
                PRINT("    [Stack trace not available - handle: 0x%x]\n", owner_ptr->handle);
            }
        } else {
            PRINT("    [No allocation stack trace available]\n");
        }

        PRINT("\n");
    }

    LOGD("CMA allocated pages stack trace analysis completed for area: %s", cma_name.c_str());
}

void Cma::print_cma_page_status(const std::string& addr_str){
    if (addr_str.empty()) {
        LOGE("Empty address provided\n");
        return;
    }
    ulonglong addr;
    try {
        addr = std::stoull(addr_str, nullptr, 16);
    } catch (const std::exception&) {
        LOGE("Invalid address format: %s\n", addr_str.c_str());
        return;
    }
    if (addr == 0) {
        LOGE("Invalid address: 0x0\n");
        return;
    }
    ulong pfn = phy_to_pfn(addr);
    if (pfn < min_low_pfn || pfn > max_pfn){
        LOGE("PFN 0x%lx out of valid range [0x%lx, 0x%lx]\n", pfn, min_low_pfn, max_pfn);
        return;
    }
    std::shared_ptr<cma_mem> cma_ptr;
    for (const auto& cma : mem_list) {
        if (pfn >= cma->base_pfn && pfn < (cma->base_pfn + cma->count)){
            cma_ptr = cma;
            break;
        }
    }
    if (cma_ptr == nullptr){
        LOGE("Address 0x%llx (PFN 0x%lx) not found in any CMA area\n", addr, pfn);
        return;
    }
    // Calculate which bit corresponds to this PFN
    ulong pfn_offset = pfn - cma_ptr->base_pfn;
    size_t per_nr_pages = (1U << cma_ptr->order_per_bit);
    size_t bit_index = pfn_offset / per_nr_pages;
    // Calculate start and end PFN for this bit
    bool allocated = is_cma_page_allocated(cma_ptr, pfn);
    PRINT("┌──────────────────────────────────────────────────────────────────────────────┐\n");
    PRINT("│  CMA Region:              %-50s │\n", cma_ptr->name.c_str());
    PRINT("│  Total Size:              %-50s │\n", csize(cma_ptr->count * page_size).c_str());
    PRINT("│  Address Range:           %-50s │\n", format_address_range(cma_ptr->base_pfn, cma_ptr->count).c_str());
    PRINT("└──────────────────────────────────────────────────────────────────────────────┘\n");

    PRINT("Page Information:\n");
    PRINT("┌──────────────────────────────────────────────────────────────────────────────┐\n");
    PRINT("│  Bit Index:               %-50zu │\n", bit_index);
    PRINT("│  Page Range:              %-50s │\n", format_address_range(cma_ptr->base_pfn + bit_index * per_nr_pages, per_nr_pages).c_str());
    PRINT("│  Page Size:               %-50s │\n", csize(per_nr_pages * page_size).c_str());
    std::string status_text = allocated ? "ALLOCATED" : "FREE";
    PRINT("│  Status:                  %-50s │\n", status_text.c_str());
    PRINT("└──────────────────────────────────────────────────────────────────────────────┘\n");
}

bool Cma::is_cma_page_allocated(const std::shared_ptr<cma_mem>& cma, ulong pfn){
    // Validate input
    if (!cma || pfn < cma->base_pfn || pfn >= (cma->base_pfn + cma->count)) {
        return false;
    }

    // Calculate pages per bit and bitmap parameters
    size_t per_nr_pages = (1U << cma->order_per_bit);
    size_t bitmap_bytes = (cma->count >> cma->order_per_bit) / 8;

    // Calculate which bit corresponds to this PFN
    ulong pfn_offset = pfn - cma->base_pfn;
    size_t bit_index = pfn_offset / per_nr_pages;
    size_t byte_index = bit_index / 8;
    size_t bit_offset = bit_index % 8;

    // Validate byte index
    if (byte_index >= bitmap_bytes) {
        return false;
    }

    // Read the specific byte containing our bit
    ulong bitmap_addr = cma->bitmap + byte_index;
    unsigned char bitmap_data = read_byte(bitmap_addr, "cma bitmap");

    // Extract and return bit value
    return ((bitmap_data >> bit_offset) & 0x1) != 0;
}
#pragma GCC diagnostic pop
