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

#include "vmalloc.h"
#include "logger/logger_core.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(Vmalloc)
#endif

/**
 * @brief Main command handler for vmalloc plugin
 *
 * Processes command-line arguments and dispatches to appropriate display functions.
 * Supports multiple options for viewing vmalloc information in different formats.
 */
void Vmalloc::cmd_main(void) {
    int c;
    std::string cppString;

    // Validate minimum argument count
    if (argcnt < 2) {
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }

    // Parse vmap area list if not already cached
    if (area_list.size() == 0) {
        parser_vmap_area_list();
    }

    // Process command-line options
    while ((c = getopt(argcnt, args, "arvsf:t:")) != EOF) {
        switch(c) {
            case 'a':
                print_vmap_area_list();
                break;
            case 'r':
                print_vmap_area();
                break;
            case 'v':
                print_vm_struct();
                break;
            case 's':
                print_summary_info();
                break;
            case 'f':
                cppString.assign(optarg);
                print_vm_info_caller(cppString);
                break;
            case 't':
                cppString.assign(optarg);
                print_vm_info_type(cppString);
                break;
            default:
                argerrs++;
                break;
        }
    }

    if (argerrs) {
        cmd_usage(pc->curcmd, SYNOPSIS);
    }
}

/**
 * @brief Initialize kernel structure field offsets
 *
 * Initializes all structure field offsets and sizes needed for parsing
 * vmalloc-related kernel structures.
 */
void Vmalloc::init_offset(void) {
    // Initialize vmap_area structure fields
    field_init(vmap_area, va_start);
    field_init(vmap_area, va_end);
    field_init(vmap_area, vm);
    field_init(vmap_area, list);

    // Initialize vm_struct structure fields
    field_init(vm_struct, next);
    field_init(vm_struct, addr);
    field_init(vm_struct, size);
    field_init(vm_struct, flags);
    field_init(vm_struct, pages);
    field_init(vm_struct, nr_pages);
    field_init(vm_struct, phys_addr);
    field_init(vm_struct, caller);

    // Initialize structure sizes
    struct_init(vmap_area);
    struct_init(vm_struct);

    // Initialize vmap_node and vmap_pool structures (for newer kernels)
    field_init(vmap_node, pool);
    field_init(vmap_pool, head);
    field_init(vmap_pool, len);
    struct_init(vmap_node);
    struct_init(vmap_pool);
}

/**
 * @brief Initialize command metadata and help information
 *
 * Sets up the command name, description, and comprehensive usage examples.
 */
void Vmalloc::init_command(void) {
    cmd_name = "vmalloc";
    help_str_list = {
        "vmalloc",                                  /* command name */
        "display vmalloc memory information",       /* short description */
        "[-a] [-r] [-v] [-s] [-f func_name] [-t type_name]\n"
        "  This command displays vmalloc memory information.\n"
        "\n"
        "    -a              display vmalloc memory info\n"
        "    -r              display all vmap_area info\n"
        "    -v              display all vm_struct info\n"
        "    -s              display vmalloc statistical info\n"
        "    -f func_name    display allocated pages by function name\n"
        "    -t type_name    display allocated pages by type name\n",
        "\n",
        "EXAMPLES",
        "  Display vmalloc memory info:",
        "    %s> vmalloc -a",
        "    vmap_area:0xffffff8003015e00 range:[0xffffffc008000000~0xffffffc008005000] size:20.00Kb",
        "       vm_struct:0xffffff8003003a00 size:20.00Kb flags:vmalloc nr_pages:4 addr:0xffffffc008000000 phys_addr:0x0 start_kernel+496",
        "           Page:0xfffffffe000c2a00 PA:0x430a8000",
        "           Page:0xfffffffe000c2a40 PA:0x430a9000",
        "           Page:0xfffffffe000c2a80 PA:0x430aa000",
        "           Page:0xfffffffe000c2ac0 PA:0x430ab000",
        "\n",
        "  Display all vmap_area info:",
        "    %s> vmalloc -r",
        "    Total vm size:443.56Mb",
        "    ==============================================================================================================",
        "    [0]vmap_area:0xffffff8003015e00 range:[0xffffffc008000000~0xffffffc008005000] size:20.00Kb",
        "    [1]vmap_area:0xffffff8003015100 range:[0xffffffc008005000~0xffffffc008007000] size:8.00Kb",
        "    [2]vmap_area:0xffffff8003015180 range:[0xffffffc008008000~0xffffffc00800d000] size:20.00Kb",
        "    [3]vmap_area:0xffffff80030159c0 range:[0xffffffc00800d000~0xffffffc00800f000] size:8.00Kb",
        "\n",
        "  Display all vm_struct info:",
        "    %s> vmalloc -v",
        "    Total vm size:502.01Mb, physical size:109.80Mb",
        "    ==============================================================================================================",
        "    [0]vm_struct:0xffffff8003003a00 size:20.00Kb  flags:vmalloc  nr_pages:4      addr:0xffffffc008000000 phys_addr:0x0            start_kernel+496",
        "    [1]vm_struct:0xffffff8003003c40 size:8.00Kb   flags:vmalloc  nr_pages:1      addr:0xffffffc008005000 phys_addr:0x0            init_IRQ+344",
        "    [2]vm_struct:0xffffff8003003ac0 size:20.00Kb  flags:vmalloc  nr_pages:4      addr:0xffffffc008008000 phys_addr:0x0            start_kernel+496",
        "    [3]vm_struct:0xffffff8003003cc0 size:8.00Kb   flags:vmalloc  nr_pages:1      addr:0xffffffc00800d000 phys_addr:0x0            init_IRQ+344",
        "\n",
        "  Display vmalloc statistical info:",
        "    %s> vmalloc -s",
        "    Summary by caller:",
        "    ========================================================",
        "    [devm_ioremap_wc+112]                        virt_size:121.01Mb   phys_size:0b",
        "    [load_module+4704]                           virt_size:54.26Mb    phys_size:53.09Mb",
        "    [devm_ioremap+112]                           virt_size:46.35Mb    phys_size:0b",
        "",
        "    Summary by type:",
        "    ========================================================",
        "    [ioremap]           virt_size:203.71Mb   phys_size:0b",
        "    [vmap]              virt_size:154.91Mb   phys_size:0b",
        "    [vmalloc]           virt_size:132.02Mb   phys_size:109.80Mb",
        "    [vpages]            virt_size:11.38Mb    phys_size:0b",
        "\n",
        "  Display the allocated pages by function name:",
        "    %s> vmalloc -f load_module",
        "    [1]Page:0xfffffffe00555040 PA:0x55541000",
        "    [2]Page:0xfffffffe00560e00 PA:0x55838000",
        "    [3]Page:0xfffffffe0074d480 PA:0x5d352000",
        "    [4]Page:0xfffffffe0074d4c0 PA:0x5d353000",
        "\n",
        "  Display the allocated pages by type name:",
        "    %s> vmalloc -t vmalloc",
        "    [1]Page:0xfffffffe000c2a00 PA:0x430a8000",
        "    [2]Page:0xfffffffe000c2a40 PA:0x430a9000",
        "    [3]Page:0xfffffffe000c2a80 PA:0x430aa000",
        "    [4]Page:0xfffffffe000c2ac0 PA:0x430ab000",
        "\n",
    };
}

/**
 * @brief Constructor
 */
Vmalloc::Vmalloc() {

}

/**
 * @brief Parse vmap_nodes structure (newer kernel versions)
 *
 * Parses the vmap_nodes structure used in newer kernels (typically 5.10+)
 * for managing vmalloc address space. This structure uses per-node pools
 * for better NUMA performance.
 */
void Vmalloc::parser_vmap_nodes() {
    // Check if vmap_nodes symbol exists
    if (!csymbol_exists("vmap_nodes")) {
        LOGE("vmap_nodes doesn't exist in this kernel!");
        return;
    }

    // Read vmap_nodes address
    ulong nodes_addr = read_pointer(csymbol_value("vmap_nodes"), "vmap_nodes pages");
    if (!is_kvaddr(nodes_addr)) {
        LOGE("Invalid vmap_nodes address: 0x%lx", nodes_addr);
        return;
    }

    // Get number of NUMA nodes
    size_t nr_node = read_int(csymbol_value("nr_vmap_nodes"), "nr_vmap_nodes");
    size_t pool_cnt = field_size(vmap_node, pool) / struct_size(vmap_pool);
    size_t offset = field_offset(vmap_area, list);
    // Iterate through all nodes and pools
    for (size_t i = 0; i < nr_node; i++) {
        ulong pools_addr = nodes_addr + i * struct_size(vmap_node) + field_offset(vmap_node, pool);
        for (size_t p = 0; p < pool_cnt; p++) {
            ulong pool_addr = pools_addr + p * struct_size(vmap_pool);
            if (!is_kvaddr(pool_addr)) {
                continue;
            }
            ulong len = read_ulong(pool_addr + field_offset(vmap_pool, len), "vmap_pool len");
            if (len == 0) {
                continue;
            }
            // Parse each vmap_area in this pool
            for (const auto& area_addr : for_each_list(pool_addr, offset)) {
                parser_vmap_area(area_addr);
            }
        }
    }
}

/**
 * @brief Parse a single vmap_area structure
 * @param addr Kernel address of the vmap_area structure
 *
 * Reads and parses a vmap_area structure and all associated vm_struct entries.
 * This function handles the linked list of vm_struct allocations within the area.
 */
void Vmalloc::parser_vmap_area(ulong addr) {
    LOGD("Parsing vmap_area at 0x%lx", addr);
    // Read vmap_area structure
    void *vmap_buf = read_struct(addr, "vmap_area");
    if (!vmap_buf) {
        LOGE("Failed to read vmap_area at 0x%lx", addr);
        return;
    }

    // Create and populate vmap_area object
    std::shared_ptr<vmap_area> area_ptr = std::make_shared<vmap_area>();
    area_ptr->addr = addr;
    area_ptr->va_start = ULONG(vmap_buf + field_offset(vmap_area, va_start));
    area_ptr->va_end = ULONG(vmap_buf + field_offset(vmap_area, va_end));
    area_list.push_back(area_ptr);

    ulong vm_addr = ULONG(vmap_buf + field_offset(vmap_area, vm));
    FREEBUF(vmap_buf);

    LOGD("vmap_area range: [0x%lx~0x%lx], size: %lu bytes",
         area_ptr->va_start, area_ptr->va_end,
         area_ptr->va_end - area_ptr->va_start);

    // Parse linked list of vm_struct entries
    int vm_count = 0;
    std::set<ulong> vm_map;
    while (is_kvaddr(vm_addr)) {
        void *vm_buf = read_struct(vm_addr, "vm_struct");
        if (!vm_buf) {
            LOGE("Failed to read vm_struct structure at address %lx", vm_addr);
            break;
        }

        // Read vm_struct fields
        size_t vm_size = ULONG(vm_buf + field_offset(vm_struct, size));
        size_t nr_pages = UINT(vm_buf + field_offset(vm_struct, nr_pages));

        // Check for loops
        if (vm_map.find(vm_addr) != vm_map.end()) {
            LOGE("Detected loop in vm_struct at 0x%lx", vm_addr);
            break;
        }
        vm_map.insert(vm_addr);

        // Validate vm_struct data
        // if (vm_size % page_size != 0 || (vm_size / page_size) != (nr_pages + 1)) {
        //     LOGE("  Invalid vm_struct at 0x%lx: size=%zu, nr_pages=%zu",
        //          vm_addr, vm_size, nr_pages);
        //     FREEBUF(vm_buf);
        //     break;
        // }

        // Create and populate vm_struct object
        std::shared_ptr<vm_struct> vm_ptr = std::make_shared<vm_struct>();
        vm_ptr->addr = vm_addr;
        vm_ptr->kaddr = ULONG(vm_buf + field_offset(vm_struct, addr));
        vm_ptr->size = vm_size;
        vm_ptr->nr_pages = nr_pages;
        vm_ptr->phys_addr = ULONG(vm_buf + field_offset(vm_struct, phys_addr));

        ulong caller = ULONG(vm_buf + field_offset(vm_struct, caller));
        ulong next = ULONG(vm_buf + field_offset(vm_struct, next));
        ulong pages = ULONG(vm_buf + field_offset(vm_struct, pages));
        ulong flags = ULONG(vm_buf + field_offset(vm_struct, flags));
        FREEBUF(vm_buf);

        // Decode allocation flags
        if (flags & Vmalloc::VM_IOREMAP) {
            vm_ptr->flags.assign("ioremap");
        } else if (flags & Vmalloc::VM_ALLOC) {
            vm_ptr->flags.assign("vmalloc");
        } else if (flags & Vmalloc::VM_MAP) {
            vm_ptr->flags.assign("vmap");
        } else if (flags & Vmalloc::VM_USERMAP) {
            vm_ptr->flags.assign("user");
        } else if (flags & Vmalloc::VM_VPAGES) {
            vm_ptr->flags.assign("vpages");
        } else if (flags & Vmalloc::VM_UNLIST) {
            vm_ptr->flags.assign("unlist");
        } else {
            vm_ptr->flags.assign("unknow");
        }
        vm_ptr->caller = to_symbol(caller);

        // Parse page list
        if (is_kvaddr(pages)) {
            for (int j = 0; j < vm_ptr->nr_pages; ++j) {
                ulong addr = pages + j * sizeof(void *);
                if (!is_kvaddr(addr)) break;

                ulong page_addr = read_pointer(addr, "vm_struct pages");
                if (!is_kvaddr(page_addr)) continue;

                physaddr_t paddr = page_to_phy(page_addr);
                if (paddr <= 0) continue;

                vm_ptr->page_list.push_back(page_addr);
            }
        }
        area_ptr->vm_list.push_back(vm_ptr);
        LOGD("  Parsed vm_struct#%d: type=%s, size=%zu, pages=%d, caller=%s",
             vm_count, vm_ptr->flags.c_str(), vm_ptr->size,
             vm_ptr->nr_pages, vm_ptr->caller.c_str());
        vm_count++;
        vm_addr = next;
    }
    vm_map.clear();
}

/**
 * @brief Parse all vmap areas in the system
 *
 * Entry point for parsing vmalloc information. Automatically detects
 * whether to use vmap_nodes (newer kernels) or vmap_area_list (older kernels).
 */
void Vmalloc::parser_vmap_area_list() {
    // Try newer vmap_nodes structure first
    if (!csymbol_exists("vmap_area_list")) {
        LOGD("Using vmap_nodes structure (newer kernel)");
        parser_vmap_nodes();
    } else {
        // Use older vmap_area_list structure
        LOGD("Using vmap_area_list structure (older kernel)");

        ulong area_list_addr = csymbol_value("vmap_area_list");
        if (!is_kvaddr(area_list_addr)) {
            LOGE("vmap_area_list address is invalid!");
            return;
        }
        int offset = field_offset(vmap_area, list);
        for (const auto& area_addr : for_each_list(area_list_addr, offset)) {
            parser_vmap_area(area_addr);
        }
    }
}

/**
 * @brief Print detailed information for all vmap areas
 *
 * Displays comprehensive information including vmap_area addresses, ranges,
 * vm_struct details, and individual page information.
 */
void Vmalloc::print_vmap_area_list() {
    size_t index = 0;
    std::ostringstream oss;
    for (auto area : area_list) {
        // Print vmap_area header
        oss << "[" << std::setw(4) << std::setfill('0') << std::dec << std::right << index << "]"
            << "vmap_area:" << std::hex << area->addr << " "
            << "range:[" << std::hex << area->va_start << "~" << std::hex << area->va_end << "]" << " "
            << "size:" << csize((area->va_end - area->va_start)) << "\n";

        // Print each vm_struct in this area
        for (auto vm : area->vm_list) {
            oss << "   vm_struct:" << std::hex << vm->addr << " "
                << "size:" << csize(vm->size) << " "
                << "flags:" << std::dec << vm->flags.c_str() << " "
                << "nr_pages:" << std::dec << vm->nr_pages << " "
                << "addr:" << std::hex << vm->kaddr << " "
                << "phys_addr:" << std::hex << vm->phys_addr << " "
                << vm->caller << "\n";

            // Print each page in this vm_struct
            size_t cnt = 1;
            for (auto page_addr : vm->page_list) {
                physaddr_t paddr = page_to_phy(page_addr);
                oss << "       [" << std::setw(4) << std::setfill('0') << std::dec << std::right << cnt << "]"
                    << "Page:" << std::hex << page_addr << " "
                    << "PA:" << paddr << "\n";
                cnt++;
            }
        }
        index++;
        oss << "\n";
    }
    PRINT( "%s", oss.str().c_str());
}

/**
 * @brief Print summary of all vmap areas
 *
 * Displays a concise list of all vmap areas with their address ranges and sizes.
 */
void Vmalloc::print_vmap_area() {
    // Calculate total virtual memory size
    ulong total_size = 0;
    for (auto area : area_list) {
        total_size += (area->va_end - area->va_start);
    }
    PRINT( "Total vm size:%s\n", csize(total_size).c_str());
    PRINT( "==============================================================================================================\n");
    std::ostringstream oss;
    for (size_t i = 0; i < area_list.size(); i++) {
        oss << "[" << std::setw(4) << std::setfill('0') << std::dec << std::right << i << "]"
            << "vmap_area:" << std::hex << area_list[i]->addr << " "
            << "range:[" << std::hex << area_list[i]->va_start << "~" << std::hex << area_list[i]->va_end << "]" << " "
            << "size:" << csize((area_list[i]->va_end - area_list[i]->va_start))
            << "\n";
    }
    PRINT( "%s \n", oss.str().c_str());
}

/**
 * @brief Print all vm_struct information
 *
 * Displays detailed information about all vm_struct allocations,
 * including total virtual and physical memory usage.
 */
void Vmalloc::print_vm_struct() {
    // Calculate totals
    ulong total_size = 0;
    ulong total_pages = 0;
    for (auto area : area_list) {
        for (auto vm : area->vm_list) {
            total_size += vm->size;
            total_pages += vm->nr_pages;
        }
    }

    PRINT( "Total vm size:%s, ", csize(total_size).c_str());
    PRINT( "physical size:%s\n", csize(total_pages * page_size).c_str());
    PRINT( "==============================================================================================================\n");
    int index = 0;
    std::ostringstream oss;
    for (auto area : area_list) {
        for (auto vm : area->vm_list) {
            oss << "[" << std::setw(4) << std::setfill('0') << std::dec << std::right << index << "]"
                << "vm_struct:" << std::hex << vm->addr << " "
                << "size:" << std::left << std::setw(8) << std::setfill(' ') << csize(vm->size) << " "
                << "flags:" << std::setw(8) << vm->flags << " "
                << "nr_pages:" << std::dec << std::setw(4) << vm->nr_pages << " "
                << "kaddr:" << std::hex << vm->kaddr << " "
                << "phys_addr:" << std::hex << vm->phys_addr
                << "\n";
            index += 1;
        }
    }
    PRINT( "%s \n", oss.str().c_str());
}

/**
 * @brief Print summary statistics grouped by caller function
 *
 * Aggregates and displays vmalloc statistics organized by the function
 * that allocated the memory, sorted by virtual memory size.
 */
void Vmalloc::print_summary_caller() {
    // Group vm_struct by caller function
    std::unordered_map<std::string, std::vector<std::shared_ptr<vm_struct>>> caller_map;
    for (auto area : area_list) {
        for (auto vm : area->vm_list) {
            auto it = caller_map.find(vm->caller);
            if (it != caller_map.end()) {
                it->second.push_back(vm);
            } else {
                caller_map[vm->caller] = std::vector<std::shared_ptr<vm_struct>>{vm};
            }
        }
    }

    // Calculate statistics for each caller
    std::vector<vmalloc_info> callers;
    for (const auto& pair : caller_map) {
        vmalloc_info info;
        info.func = pair.first;
        ulong total_size = 0;
        ulong total_cnt = 0;

        for (auto vm : pair.second) {
            total_size += vm->size;
            total_cnt += vm->nr_pages;
        }

        info.virt_size = total_size;
        info.page_cnt = total_cnt;
        callers.push_back(info);
    }

    // Sort by virtual size (descending)
    std::sort(callers.begin(), callers.end(), [&](vmalloc_info a, vmalloc_info b) {
        return a.virt_size > b.virt_size;
    });

    // Find maximum function name length for formatting
    size_t max_len = 0;
    for (const auto& info : callers) {
        max_len = std::max(max_len, info.func.size());
    }

    // Print table
    std::ostringstream oss;
    oss << std::left << std::setw(max_len + 2) << "Func Name" << " "
        << std::left << std::setw(15) << "virt" << " "
        << std::left << std::setw(15) << "phys"
        << "\n";

    for (const auto& info : callers) {
        oss << std::left << std::setw(max_len + 2) << info.func << " "
            << std::left << std::setw(15) << csize(info.virt_size) << " "
            << std::left << std::setw(15) << csize(info.page_cnt * page_size)
            << "\n";
    }
    PRINT( "%s \n", oss.str().c_str());
}

/**
 * @brief Print summary statistics grouped by allocation type
 *
 * Aggregates and displays vmalloc statistics organized by allocation type
 * (ioremap, vmalloc, vmap, etc.), sorted by virtual memory size.
 */
void Vmalloc::print_summary_type() {
    // Group vm_struct by allocation type
    std::unordered_map<std::string, std::vector<std::shared_ptr<vm_struct>>> type_maps;
    for (auto area : area_list) {
        for (auto vm : area->vm_list) {
            auto it = type_maps.find(vm->flags);
            if (it != type_maps.end()) {
                it->second.push_back(vm);
            } else {
                type_maps[vm->flags] = std::vector<std::shared_ptr<vm_struct>>{vm};
            }
        }
    }

    // Calculate statistics for each type
    std::vector<vmalloc_info> types;
    for (const auto& pair : type_maps) {
        vmalloc_info info;
        info.func = pair.first;
        ulong total_size = 0;
        ulong total_cnt = 0;

        for (auto vm : pair.second) {
            total_size += vm->size;
            total_cnt += vm->nr_pages;
        }

        info.virt_size = total_size;
        info.page_cnt = total_cnt;
        types.push_back(info);
    }

    // Sort by virtual size (descending)
    std::sort(types.begin(), types.end(), [&](vmalloc_info a, vmalloc_info b) {
        return a.virt_size > b.virt_size;
    });

    // Find maximum type name length for formatting
    size_t max_len = 0;
    for (const auto& info : types) {
        max_len = std::max(max_len, info.func.size());
    }

    // Print table
    std::ostringstream oss;
    oss << std::left << std::setw(max_len + 2) << "Type" << " "
        << std::left << std::setw(15) << "virt" << " "
        << std::left << std::setw(15) << "phys"
        << "\n";

    for (const auto& info : types) {
        oss << std::left << std::setw(max_len + 2) << info.func << " "
            << std::left << std::setw(15) << csize(info.virt_size) << " "
            << std::left << std::setw(15) << csize(info.page_cnt * page_size)
            << "\n";
    }
    PRINT( "%s \n", oss.str().c_str());
}

/**
 * @brief Print summary statistics
 *
 * Displays comprehensive statistics including summaries by both
 * caller function and allocation type.
 */
void Vmalloc::print_summary_info() {
    PRINT( "Summary by caller:\n");
    PRINT( "========================================================\n");
    print_summary_caller();

    PRINT( "\n\nSummary by type:\n");
    PRINT( "========================================================\n");
    print_summary_type();
}

/**
 * @brief Print page information for a specific caller function
 * @param func Function name to filter by (partial match supported)
 *
 * Displays all physical pages allocated by functions matching the given name.
 */
void Vmalloc::print_vm_info_caller(std::string func) {
    // Group vm_struct by caller
    std::unordered_map<std::string, std::vector<std::shared_ptr<vm_struct>>> caller_map;
    for (auto area : area_list) {
        for (auto vm : area->vm_list) {
            auto it = caller_map.find(vm->caller);
            if (it != caller_map.end()) {
                it->second.push_back(vm);
            } else {
                caller_map[vm->caller] = std::vector<std::shared_ptr<vm_struct>>{vm};
            }
        }
    }

    std::ostringstream oss;
    // Find matching callers and print their pages
    for (const auto& item : caller_map) {
        std::string func_name = item.first;
        std::vector<std::shared_ptr<vm_struct>> vm_list = item.second;

        if (func_name.find(func) != std::string::npos) {
            int index = 1;
            for (auto vm : vm_list) {
                for (auto page_addr : vm->page_list) {
                    physaddr_t paddr = page_to_phy(page_addr);
                    oss << "[" << std::setw(4) << std::setfill('0') << std::dec << std::right << index << "]"
                        << "Page:" << std::left << std::hex << page_addr << " "
                        << "PA:" << paddr
                        << "\n";
                    index += 1;
                }
            }
        }
    }
    PRINT( "%s \n", oss.str().c_str());
}

/**
 * @brief Print page information for a specific allocation type
 * @param type Allocation type to filter by (partial match supported)
 *
 * Displays all physical pages allocated with the specified type
 * (e.g., vmalloc, ioremap, vmap).
 */
void Vmalloc::print_vm_info_type(std::string type) {
    // Group vm_struct by type
    std::unordered_map<std::string, std::vector<std::shared_ptr<vm_struct>>> type_maps;
    for (auto area : area_list) {
        for (auto vm : area->vm_list) {
            auto it = type_maps.find(vm->flags);
            if (it != type_maps.end()) {
                it->second.push_back(vm);
            } else {
                type_maps[vm->flags] = std::vector<std::shared_ptr<vm_struct>>{vm};
            }
        }
    }
    std::ostringstream oss;
    // Find matching types and print their pages
    for (const auto& item : type_maps) {
        std::string type_name = item.first;
        std::vector<std::shared_ptr<vm_struct>> vm_list = item.second;
        if (type_name.find(type) != std::string::npos) {
            int index = 1;
            for (auto vm : vm_list) {
                for (auto page_addr : vm->page_list) {
                    physaddr_t paddr = page_to_phy(page_addr);
                    oss << "[" << std::setw(4) << std::setfill('0') << std::dec << std::right << index << "]"
                        << "Page:" << std::left << std::hex << page_addr << " "
                        << "PA:" << paddr
                        << "\n";
                    index += 1;
                }
            }
        }
    }
    PRINT( "%s \n", oss.str().c_str());
}

#pragma GCC diagnostic pop
