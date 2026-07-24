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

#include "slub.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(Slub)
#endif

/* ============================================================================
 * Command Line Interface Implementation
 * ============================================================================ */

/**
 * @brief Main command entry point with enhanced argument parsing
 *
 * This function implements a comprehensive command-line interface supporting
 * multiple operation modes with optional cache filtering. The interface is
 * designed to be intuitive and follows common Unix command patterns.
 */
void Slub::cmd_main(void) {
    int c;
    std::string optarg_str;
    std::string cache_name; // Cache name or address for filtering operations

    // Operation flags - track which operations to perform
    bool show_summary = false;          // -s: Display cache summary
    bool show_objects = false;          // -o: Display object information
    bool show_poison = false;           // -d: Check memory corruption
    bool show_traces = false;           // -t: Display allocation traces
    bool show_alloc_traces = false;     // -t -a: Display allocation call stacks
    bool show_free_traces = false;      // -t -f: Display free call stacks
    bool show_pid_stats = false;        // -p: Display PID statistics

    // Detailed analysis parameters
    std::string pid_details_arg;        // -P: Specific PID analysis
    std::string frame_info_arg;         // -b: Frame stack information
    std::string object_trace_arg;       // -O: Object trace analysis

    // Validate minimum argument count
    if (argcnt < 2) {
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }

    // First pass: Parse and validate all command-line arguments
    while ((c = getopt(argcnt, args, "sc:od:tafpP:b:O:")) != EOF) {
        if (optarg) {
            optarg_str.assign(optarg);
        }

        switch(c) {
            case 's':
                show_summary = true;
                LOGD("Operation: Display cache summary");
                break;

            case 'c': // Cache name for other operations
                cache_name = optarg_str;
                LOGD("Cache name set: '%s'", cache_name.c_str());
                break;

            case 'o': // Object information (supports -c filtering)
                show_objects = true;
                LOGD("Operation: Display object information");
                break;

            case 'd': // Memory corruption check (supports -c filtering)
                show_poison = true;
                LOGD("Operation: Check memory corruption");
                break;

            case 't': // Allocation traces (supports -c filtering)
                show_traces = true;
                LOGD("Operation: Display allocation traces");
                break;

            case 'a': // Allocation call stacks (used with -t)
                show_alloc_traces = true;
                LOGD("Sub-option: Display allocation call stacks");
                break;

            case 'f': // Free call stacks (used with -t)
                show_free_traces = true;
                LOGD("Sub-option: Display free call stacks");
                break;

            case 'p': // PID statistics
                show_pid_stats = true;
                LOGD("Operation: Display PID statistics");
                break;

            case 'P': // Detailed PID analysis
                pid_details_arg = optarg_str;
                LOGD("Operation: Analyze PID %s", pid_details_arg.c_str());
                break;

            case 'b': // Frame stack information
                frame_info_arg = optarg_str;
                LOGD("Operation: Display frame %s information", frame_info_arg.c_str());
                break;

            case 'O': // Object trace analysis
                object_trace_arg = optarg_str;
                LOGD("Operation: Analyze object at %s", object_trace_arg.c_str());
                break;

            default:
                LOGW("Unknown option: -%c", c);
                argerrs++;
                break;
        }
    }

    // Check for argument errors
    if (argerrs) {
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }

    // Determine if we need to parse all caches or just a specific one
    bool need_all_caches = cache_name.empty() || show_summary || show_pid_stats ||
                          !pid_details_arg.empty() || !frame_info_arg.empty() || !object_trace_arg.empty();

    if (need_all_caches) {
        // Operations that require all caches or no specific cache name
        if (cache_list.empty()) {
            parser_kmem_caches();
        }
    } else {
        // Operations with specific cache name (-c) - try to parse only that cache
        if (cache_list.empty() || cache_list.find(cache_name) == cache_list.end()) {
            LOGI("Attempting to parse specific cache: %s", cache_name.c_str());
            parse_kmem_cache(cache_name);
        }
    }

    // Second pass: Execute operations in logical order
    // Note: Operations are executed in a specific order to provide
    // the most logical flow of information for the user
    if (show_summary) {
        LOGI("Executing: Display cache summary");
        print_slab_summary_info();
    }

    if (show_objects) {
        LOGI("Executing: Display all objects information%s",
             cache_name.empty() ? " (all caches)" : (" for cache: " + cache_name).c_str());
        print_all_objects_info(cache_name);
    }

    if (show_poison) {
        LOGI("Executing: Memory corruption check%s",
             cache_name.empty() ? " (all caches)" : (" for cache: " + cache_name).c_str());
        print_slub_poison(cache_name);
    }

    if (show_traces) {
        if (show_alloc_traces) {
            LOGI("Executing: Display allocation traces%s",
                 cache_name.empty() ? " (all caches)" : (" for cache: " + cache_name).c_str());
            print_top_alloc_trace(cache_name);
        } else if (show_free_traces) {
            LOGI("Executing: Display free traces%s",
                 cache_name.empty() ? " (all caches)" : (" for cache: " + cache_name).c_str());
            print_top_free_trace(cache_name);
        } else {
            // Default behavior when -t is used without -a or -f
            LOGI("Executing: Display allocation traces%s",
                 cache_name.empty() ? " (all caches)" : (" for cache: " + cache_name).c_str());
            print_top_alloc_trace(cache_name);
        }
    }

    if (show_pid_stats) {
        LOGI("Executing: Display PID allocation statistics");
        print_alloc_mem_by_pid();
    }

    if (!pid_details_arg.empty()) {
        LOGI("Executing: Detailed PID analysis for %s", pid_details_arg.c_str());
        func_call(pid_details_arg, "pid", [this](const std::string& s) {
            print_pid_details(std::stoi(s));
        });
    }

    if (!frame_info_arg.empty()) {
        LOGI("Executing: Frame stack information for %s", frame_info_arg.c_str());
        func_call(frame_info_arg, "frame_id", [this](const std::string& s) {
            print_frame_stack_info(std::stoull(s));
        });
    }

    if (!object_trace_arg.empty()) {
        LOGI("Executing: Object trace analysis for %s", object_trace_arg.c_str());
        print_object_trace(object_trace_arg);
    }
}

/* ============================================================================
 * Initialization Functions
 * ============================================================================ */

/**
 * @brief Initialize kernel structure field offsets and sizes
 *
 * This function initializes all the field offsets and structure sizes needed
 * to parse kernel data structures. It handles version differences between
 * kernel releases and provides fallback values for older kernels.
 */
void Slub::init_offset(void) {
    LOGD("Initializing kernel structure offsets...");

    // Initialize kmem_cache structure fields
    field_init(kmem_cache, cpu_slab);
    field_init(kmem_cache, flags);
    field_init(kmem_cache, min_partial);
    field_init(kmem_cache, size);
    field_init(kmem_cache, reciprocal_size);
    field_init(kmem_cache, object_size);
    field_init(kmem_cache, offset);
    field_init(kmem_cache, cpu_partial);
    field_init(kmem_cache, oo);
    field_init(kmem_cache, max);
    field_init(kmem_cache, min);
    field_init(kmem_cache, allocflags);
    field_init(kmem_cache, refcount);
    field_init(kmem_cache, inuse);
    field_init(kmem_cache, align);
    field_init(kmem_cache, red_left_pad);
    field_init(kmem_cache, name);
    field_init(kmem_cache, random);
    field_init(kmem_cache, list);
    field_init(kmem_cache, useroffset);
    field_init(kmem_cache, usersize);
    field_init(kmem_cache, node);

    // Initialize kmem_cache_node structure fields
    field_init(kmem_cache_node, nr_partial);
    field_init(kmem_cache_node, partial);
    field_init(kmem_cache_node, nr_slabs);
    field_init(kmem_cache_node, total_objects);
    field_init(kmem_cache_node, full);

    // Initialize kmem_cache_cpu structure fields
    field_init(kmem_cache_cpu, freelist);
    field_init(kmem_cache_cpu, tid);
    field_init(kmem_cache_cpu, page);
    field_init(kmem_cache_cpu, partial);
    field_init(kmem_cache_cpu, slab);

    // Initialize track structure fields for allocation tracing
    field_init(track, addrs);
    field_init(track, addr);
    field_init(track, cpu);
    field_init(track, pid);
    field_init(track, when);
    field_init(track, handle);

    // Initialize stack record structure fields for stack depot
    field_init(stack_record, next);
    field_init(stack_record, size);
    field_init(stack_record, handle);
    field_init(stack_record, entries);

    // Initialize slab/page structure fields (handle both old and new kernels)
    field_init(slab, slab_list);
    field_init(slab, counters);
    field_init(slab, freelist);
    field_init(slab, next);
    field_init(page, slab_list);
    field_init(page, _mapcount);
    field_init(page, freelist);
    field_init(page, next);
    field_init(page, counters);
    field_init(page, flags);

    // Initialize structure sizes
    struct_init(slab);
    struct_init(page);
    struct_init(kmem_cache);
    struct_init(kmem_cache_node);
    struct_init(kmem_cache_cpu);
    struct_init(atomic_t);
    struct_init(track);
    struct_init(stack_record);

    // Initialize SLAB debug flags from kernel enumerations
    // Try to read from kernel's _slab_flag_bits enumeration first
    if (get_enumerator_list("_slab_flag_bits").size() > 0) {
        LOGD("Reading SLAB flags from kernel enumerations");
        SLAB_RED_ZONE = 1U << read_enum_val("_SLAB_RED_ZONE");
        SLAB_POISON = 1U << read_enum_val("_SLAB_POISON");
        SLAB_STORE_USER = 1U << read_enum_val("_SLAB_STORE_USER");
        OBJECT_POISON = 1U << read_enum_val("_SLAB_OBJECT_POISON");
        SLAB_KMALLOC = 1U << read_enum_val("_SLAB_KMALLOC");
    } else {
        // Fallback to hardcoded values for older kernels
        LOGD("Using fallback SLAB flag values");
        SLAB_RED_ZONE = 0x400;
        SLAB_POISON = 0x800;
        SLAB_STORE_USER = 0x10000;
        OBJECT_POISON = 0x80000000;
        SLAB_KMALLOC = 0x00001000;
    }

    LOGD("SLAB flags initialized: RED_ZONE=%#x, POISON=%#x, STORE_USER=%#x, OBJECT_POISON=%#x, KMALLOC=%#x",
         SLAB_RED_ZONE, SLAB_POISON, SLAB_STORE_USER, OBJECT_POISON, SLAB_KMALLOC);
}

/**
 * @brief Initialize command help information and usage examples
 *
 * This function sets up comprehensive help documentation for the SLUB plugin,
 * including detailed usage examples, parameter descriptions, and practical
 * use cases for memory analysis and debugging.
 */
void Slub::init_command(void) {
    cmd_name = "slub";
    help_str_list = {
        "slub",                                    /* command name */
        "comprehensive SLUB allocator analysis",  /* short description */

        /* Usage synopsis */
        "-s                                display all cache summary information\n"
        "  slub -o [-c <cache_name>]              display detailed object information with traces\n"
        "  slub -d [-c <cache_name>]              check memory corruption (poison detection)\n"
        "  slub -t [-a|-f] [-c <cache_name>]      display allocation/free traces (default: allocation)\n"
        "  slub -p                                display memory allocation by PID\n"
        "  slub -P <pid>                          display specific PID details\n"
        "  slub -b <frame_id>                     display frame stack information\n"
        "  slub -O <address>                      display object information and track\n"
        "  \n"
        "  Trace options:\n"
        "    -a                                   show allocation call stacks (used with -t)\n"
        "    -f                                   show free call stacks (used with -t)\n"
        "  \n"
        "  This command provides comprehensive SLUB allocator analysis including\n"
        "  cache information, memory corruption detection, allocation tracing,\n"
        "  frame-based memory allocation statistics, and global PID memory analysis.\n"
        "  \n"
        "  Cache filtering (-c) can be used with -o, -d, and -t options to limit\n"
        "  analysis to a specific cache. The cache can be specified by name or address.",
        "\n",

        /* Examples section */
        "EXAMPLES",
        "  Display comprehensive cache summary with memory usage statistics:",
        "    %s> slub -s",
        "    kmem_cache        name                      slabs slab_size    per_obj total_objs obj_size   pad_size align_size total_size",
        "    ffffff80030c6000  inode_cache               6310  16K          17      107270     920        16       928        94.93MB",
        "    ffffff80030c4500  vm_area_struct            7350  4K           17      124950     232        8        240        28.60MB",
        "\n",

        "  Display detailed object information for all caches:",
        "    %s> slub -o",
        "    ALLOC [0xffffff8bb12d0000~0xffffff8bb12d8000](4 KB) slab:0xfffffffee34bba00 kmem_cache:0xffffff884800bd80 maple_node pid:1234 cpu:0 timestamp:(8394673983) 00:00:08.394673983 (uptime)",
        "    [<ffffffed3bdb039c>] post_alloc_hook+20c",
        "    [<ffffffed3bdb3064>] prep_new_page+28",
        "    [<ffffffed3bdb46a4>] get_page_from_freelist+12ac",
        "    FREE [0xffffff8bb12d8000~0xffffff8bb12e0000](4 KB) slab:0xfffffffee34bba00 kmem_cache:0xffffff884800bd80 maple_node pid:5678 cpu:1 timestamp:(8394680123) 00:00:08.394680123 (uptime)",
        "    [<ffffffed3bdb1234>] kfree+15c",
        "    [<ffffffed3bdb5678>] cleanup_function+44",
        "\n",

        "  Display object information for specific cache:",
        "    %s> slub -o -c inode_cache",
        "    Shows detailed object information with allocation/free traces for the specified cache only",
        "\n",

        "  Perform comprehensive memory corruption check:",
        "    %s> slub -d",
        "    SLUB Memory Corruption Check",
        "    ===============================================================",
        "    CACHE: inode_cache (0xffffff80030c6000)",
        "      Objects: 107270 total, 107270 checked",
        "      Result: CLEAN - No corruption detected",
        "      Status: PASS",
        "    CORRUPTION CHECK SUMMARY",
        "    Overall Status: SYSTEM CLEAN",
        "    Statistics: 45 caches total (45 clean, 0 corrupted)",
        "\n",

        "  Check corruption for specific cache with detailed analysis:",
        "    %s> slub -d -c kmalloc-64",
        "    Shows detailed corruption analysis with memory layout for each corrupted object",
        "\n",

        "  Display top allocation traces sorted by memory usage:",
        "    %s> slub -t",
        "    ================================================================================",
        "                            SLUB ALLOCATION STATISTICS",
        "    ================================================================================",
        "    Call Stacks: 1247     | Total Allocations: 2847291    | Total Memory: 1.2GB",
        "    [1]Frame ID:12856162743170019396 - Allocations: 164, Memory: 41.00KB",
        "       [<ffffffff811d4c5a>] __kmalloc+0x12a/0x1b0",
        "       [<ffffffff812a8f3c>] seq_buf_alloc+0x2c/0x60",
        "      Per-Process Breakdown:",
        "         PID        Count      Memory",
        "         1234       100        25.00KB",
        "         5678       64         16.00KB",
        "\n",

        "  Display allocation traces for specific cache:",
        "    %s> slub -t -c kmalloc-64",
        "    Shows allocation traces only for the specified cache",
        "\n",

        "  Display global PID memory statistics:",
        "    %s> slub -p",
        "    ================================================================================",
        "                        SLUB ALLOCATION STATISTICS (Top 20 by memory usage)",
        "    ================================================================================",
        "    Process: 1247     | Total Allocations: 2847291    | Total Memory: 1.2GB",
        "    PID        Allocations     Total Memory    Frames",
        "    1234       50110           17.20MB         25",
        "    5678       25055           8.60MB          12",
        "\n",

        "  Display detailed statistics for specific PID:",
        "    %s> slub -P 1234",
        "    ================================================================================",
        "                               Detailed PID Analysis",
        "    PID: 1234 | Total Allocations: 50110 | Total Memory: 17.20MB | Frame count: 25",
        "    Frame ID                       Memory              Percentage",
        "    12856162743170019396          5.20MB              30.2%",
        "    98765432109876543210          3.80MB              22.1%",
        "    Use 'slub -b <frame_id>' to view detailed stack information.",
        "\n",

        "  Display detailed stack information for specific frame:",
        "    %s> slub -b 12856162743170019396",
        "    ================================================================================",
        "    Frame Stack Information - ID: 12856162743170019396",
        "    Total Allocations: 164 | Total Memory: 41.00KB",
        "      [<ffffffff811d4c5a>] __kmalloc+0x12a",
        "      [<ffffffff812a8f3c>] seq_buf_alloc+0x2c",
        "    PID        Allocations     Memory",
        "    1234       100             25.00KB",
        "    5678       64              16.00KB",
        "\n",

        "  Find object by virtual address and show detailed trace:",
        "    %s> slub -O 0xffffff881fc10010",
        "    ================================================================================",
        "    SLUB Object Analysis for Address: 0xffffff881fc10010",
        "    ================================================================================",
        "    KMEM_CACHE:",
        "       Address     : 0xffffff80030c6000",
        "       Name        : inode_cache",
        "       Flags       : 0x50020 (STORE_USER enabled)",
        "    OBJECT:",
        "       Index       : 1",
        "       Status      : FREED",
        "       Offset      : +0x10 (16 bytes from object start)",
        "    STACK TRACE:",
        "       Type        : FREE",
        "       PID         : 123",
        "       Call Stack  :",
        "          [<ffffffff811d4c5a>] kfree+0x12a/0x1b0",
        "\n",

        /* Advanced usage section */
        "ADVANCED USAGE",
        "  Combine cache filtering with different operations:",
        "    %s> slub -o -c kmalloc-64        # Objects in kmalloc-64 cache",
        "    %s> slub -d -c kmalloc-64        # Corruption check for specific cache",
        "    %s> slub -t -c kmalloc-64        # Traces for inode_cache only",
        "\n",

        "  Memory leak investigation workflow:",
        "    1. %s> slub -p                   # Find PIDs with high memory usage",
        "    2. %s> slub -P <pid>             # Analyze specific PID's allocations",
        "    3. %s> slub -b <frame_id>        # Examine call stack details",
        "    4. %s> slub -t -c <cache>        # Focus on problematic cache",
        "\n",

        "  Memory corruption investigation workflow:",
        "    1. %s> slub -d                   # System-wide corruption check",
        "    2. %s> slub -d -c <cache>        # Detailed analysis of corrupted cache",
        "    3. %s> slub -O <address>         # Analyze specific corrupted object",
        "\n",
    };
}

/* ============================================================================
 * Constructor and Utility Functions
 * ============================================================================ */

/**
 * @brief Default constructor
 */
Slub::Slub() {}

/**
 * @brief Safe function call wrapper with exception handling
 *
 * This utility function provides a safe way to execute functions that might
 * throw exceptions, particularly when parsing user input like addresses or
 * numeric values. It catches exceptions and reports them as argument errors.
 *
 * @param input_str Input string to process
 * @param param_name Parameter name for error reporting
 * @param func Function to execute with the input string
 */
void Slub::func_call(const std::string& input_str,
                    const std::string& param_name,
                    std::function<void(const std::string&)> func) {
    try {
        func(input_str);
    } catch (const std::exception& e) {
        LOGE("Invalid %s: %s (exception: %s)", param_name.c_str(), input_str.c_str(), e.what());
        argerrs++;
    }
}

/* ============================================================================
 * Core SLUB Structure Parsing Functions
 * ============================================================================ */

/**
 * Parse a single SLUB slab structure and extract all object information.
 *
 * This function reads a slab/page structure from kernel memory and constructs
 * a complete representation including:
 * - Slab metadata (object counts, page order, addresses)
 * - Free object tracking via freelist traversal
 * - Individual object states (allocated vs freed)
 *
 * The function handles both old (page-based) and new (slab-based) kernel structures
 * and performs freelist validation to detect memory corruption.
 *
 * @param cache_ptr Pointer to the parent kmem_cache structure
 * @param slab_page_addr Kernel virtual address of the slab/page structure
 * @return Shared pointer to parsed slab structure, or nullptr on failure
 */
std::shared_ptr<slab> Slub::parser_slab(std::shared_ptr<kmem_cache> cache_ptr, ulong slab_page_addr){
    // Validate input address
    if (!is_kvaddr(slab_page_addr)){
        LOGD("      Invalid slab address %#lx, skipping", slab_page_addr);
        return nullptr;
    }
    // Read slab/page structure from kernel memory
    // Newer kernels use 'struct slab', older kernels use 'struct page'
    void *page_buf = nullptr;
    if (struct_size(slab) != -1){
        page_buf = read_struct(slab_page_addr, "slab");
    } else {
        page_buf = read_struct(slab_page_addr, "page");
    }
    if(page_buf == nullptr) {
        LOGE("      Failed to read page at %#lx for cache '%s'", slab_page_addr, cache_ptr->name.c_str());
        return nullptr;
    }
    // Extract slab metadata from the structure
    // The 'counters' field encodes both object count and in-use count
    ulong count = 0;
    ulong freelist = 0;
    if (struct_size(slab) != -1){
        count = ULONG(page_buf + field_offset(slab, counters));
        freelist = ULONG(page_buf + field_offset(slab, freelist));
    } else {
        count = ULONG(page_buf + field_offset(page, counters));
        freelist = ULONG(page_buf + field_offset(page, freelist));
    }

    // Free the temporary buffer immediately after extracting needed fields
    FREEBUF(page_buf);

    // Create and initialize the slab object
    std::shared_ptr<slab> slab_ptr = std::make_shared<slab>();

    // Decode the counters field:
    // - Upper 15 bits (bits 16-30): total object count
    // - Lower 16 bits (bits 0-15): in-use object count
    slab_ptr->totalobj = (count >> 16) & 0x00007FFF;
    slab_ptr->inuse = count & 0x0000FFFF;
    slab_ptr->freeobj = slab_ptr->totalobj - slab_ptr->inuse;
    slab_ptr->order = cache_ptr->page_order;
    slab_ptr->first_page = slab_page_addr;

    LOGD("      Parsing slab at %#lx, first_page=%#lx, totalobj=%u, inuse=%u, freeobj=%u, order=%u",
         slab_page_addr, slab_ptr->first_page, slab_ptr->totalobj, slab_ptr->inuse, slab_ptr->freeobj, slab_ptr->order);

    // Validate object counts for sanity
    if (slab_ptr->totalobj == 0 || slab_ptr->totalobj > 1024) {
        LOGE("      Suspicious object count %u in slab %#lx, possible corruption",
             slab_ptr->totalobj, slab_page_addr);
        return nullptr;
    }

    // Build free object bitmap by traversing the freelist
    // The freelist is a linked list of free objects within the slab
    std::vector<int> obj_free(slab_ptr->totalobj, 0);

    // Convert page address to virtual address for object calculations
    physaddr_t slab_paddr = page_to_phy(slab_page_addr);
    ulong slab_vaddr = phy_to_virt(slab_paddr);

    // Traverse the freelist to mark all free objects
    ulong fobj = freelist;
    unsigned int free_count = 0;
    while (is_kvaddr(fobj)){
        // Calculate object index from its address
        unsigned int obj_index = (fobj - slab_vaddr) / cache_ptr->size;

        // Validate object index to detect freelist corruption
        if (obj_index >= slab_ptr->totalobj) {
            LOGE("        obj_index=%u exceeds totalobj=%u (freelist=%#lx)", obj_index, slab_ptr->totalobj, fobj);
            break;
        }

        // Detect circular freelist (object already marked as free)
        if (obj_free[obj_index] == 1){
            LOGE("        circular reference detected at obj_index=%u (freelist=%#lx)",obj_index, fobj);
            break;
        }

        // Mark this object as free
        obj_free[obj_index] = 1;
        LOGD("        obj[%u] at %#lx is freed", obj_index, fobj);
        free_count++;

        // Get the next free object in the list
        // The free pointer may be obfuscated for security (CONFIG_SLAB_FREELIST_HARDENED)
        fobj = get_free_pointer(cache_ptr, fobj);
    }

    // Verify freelist count matches metadata
    if (free_count != slab_ptr->freeobj) {
        LOGW("      Freelist count mismatch: traversed=%u, expected=%u",free_count, slab_ptr->freeobj);
    }
    for (unsigned int i = 0; i < slab_ptr->totalobj; i++) {
        ulong obj_addr = slab_vaddr + (i * cache_ptr->size);
        // Create object descriptor
        std::shared_ptr<obj> obj_ptr = std::make_shared<obj>();
        obj_ptr->slab_ptr = slab_ptr;
        obj_ptr->cache_ptr = cache_ptr;
        obj_ptr->index = i + 1;  // 1-based indexing for display
        obj_ptr->start = obj_addr;
        obj_ptr->end = obj_addr + cache_ptr->size;
        obj_ptr->is_free = (obj_free[i] == 1);
        slab_ptr->obj_list.push_back(obj_ptr);

        // Validate that the cache has trace support
        if (cache_ptr->flags & SLAB_STORE_USER) {
            // Create a track structure to hold the parsed trace information
            auto track_ptr = std::make_shared<track>();
            obj_ptr->track_ptr = track_ptr;
            track_ptr->kmem_cache_ptr = cache_ptr;
            track_ptr->obj_ptr = obj_ptr;
            track_ptr->obj_addr = obj_addr;
            // Determine which trace to extract based on object state
            // - Free objects: Get the stack trace of who freed it (TRACK_FREE)
            // - Allocated objects: Get the stack trace of who allocated it (TRACK_ALLOC)
            uint8_t track_type = obj_ptr->is_free ? TRACK_FREE : TRACK_ALLOC;
            track_ptr->track_addr = get_obj_track_addr(cache_ptr, obj_ptr->start, track_type);
            LOGD("        obj[%d] at %#lx: %s, track_addr=%#lx",
                obj_ptr->index, obj_ptr->start, obj_ptr->is_free ? "freed" : "allocated",
                track_ptr->track_addr);
            // Parse the actual trace data from kernel memory
            parser_obj_trace(track_ptr->track_addr, track_ptr);
            if (obj_ptr->is_free) {
                cache_ptr->free_list.push_back(track_ptr);
            } else {
                cache_ptr->alloc_list.push_back(track_ptr);
            }

        }
    }
    LOGD("      Slab parsing complete: %zu objects created (%u allocated, %u free) \n",
         slab_ptr->obj_list.size(), slab_ptr->inuse, slab_ptr->freeobj);

    return slab_ptr;
}

/**
 * Parse all slabs from a linked list structure.
 *
 * This function traverses a kernel linked list of slab/page structures and parses
 * each one into a slab object. It handles both old (page-based) and new (slab-based)
 * kernel structures and performs duplicate detection to avoid processing the same
 * slab multiple times.
 *
 * The function is used to parse:
 * - Partial slab lists (slabs with some free objects)
 * - Full slab lists (slabs with all objects allocated)
 * - Per-CPU partial slab lists
 *
 * @param cache_ptr Pointer to the parent kmem_cache structure
 * @param head_addr Address of the list head structure
 * @return Vector of parsed slab structures
 */
std::vector<std::shared_ptr<slab>> Slub::parser_slab_from_list(std::shared_ptr<kmem_cache> cache_ptr, ulong head_addr){
    std::vector<std::shared_ptr<slab>> slab_list;

    // Duplicate detection set to handle circular or corrupted lists
    // Using unordered_set for O(1) lookup performance
    std::unordered_set<ulong> visited_addrs;

    // Determine the list field offset based on kernel version
    // Newer kernels use 'struct slab', older kernels use 'struct page'
    int list_offset = 0;
    if (struct_size(slab) == -1){
        // Older kernel: use page structure
        list_offset = field_offset(page, slab_list);
    } else {
        // Newer kernel: use slab structure
        list_offset = field_offset(slab, slab_list);
    }
    // Traverse the linked list and collect all slab addresses
    // for_each_list() walks the kernel's doubly-linked list structure
    std::vector<ulong> page_list = for_each_list(head_addr, list_offset);
    LOGD("    Found %zu slab", page_list.size());
    // Validate and warn if list seems suspiciously large
    if (page_list.size() > 10000) {
        LOGW("    Unusually large slab list (%zu entries) - possible corruption or memory leak",
             page_list.size());
    }
    // Parse each slab in the list
    for (const auto& slab_page_addr : page_list) {
        // Skip duplicate addresses (can occur in corrupted lists)
        if (visited_addrs.find(slab_page_addr) != visited_addrs.end()) {
            LOGD("    Skipping duplicate slab address %#lx", slab_page_addr);
            continue;
        }
        // Mark this address as visited
        visited_addrs.insert(slab_page_addr);
        // Parse the individual slab structure
        std::shared_ptr<slab> slab_ptr = parser_slab(cache_ptr, slab_page_addr);
        if (slab_ptr) {
            // Successfully parsed - add to result list
            slab_list.push_back(slab_ptr);
        } else {
            // Failed to parse this slab
            LOGW("    Failed to parse slab at %#lx", slab_page_addr);
        }
    }
    return slab_list;
}

/**
 * Parse NUMA node information for a specific SLUB cache.
 *
 * In NUMA (Non-Uniform Memory Access) systems, each memory node maintains its own
 * set of slabs to optimize memory locality. This function parses all kmem_cache_node
 * structures associated with a cache, extracting:
 * - Node statistics (partial/full slab counts, object counts)
 * - Partial slab list (slabs with some free objects)
 * - Full slab list (slabs with all objects allocated)
 *
 * @param cache_ptr Pointer to the parent kmem_cache structure
 * @param node_addr Base address of the node array in the kmem_cache structure
 * @return Vector of parsed kmem_cache_node structures
 */
std::vector<std::shared_ptr<kmem_cache_node>> Slub::parser_kmem_cache_node(std::shared_ptr<kmem_cache> cache_ptr, ulong node_addr){
    std::vector<std::shared_ptr<kmem_cache_node>> node_list;

    // Calculate the number of NUMA nodes by dividing the node array size by pointer size
    // The 'node' field in kmem_cache is an array of pointers to kmem_cache_node structures
    int node_cnt = field_size(kmem_cache, node) / sizeof(void *);
    LOGD(" Parsing kmem_cache_node for cache '%s': %d node", cache_ptr->name.c_str(), node_cnt);

    // Iterate through each potential NUMA node
    for (int i = 0; i < node_cnt; i++){
        // Read the pointer to the kmem_cache_node structure for this node
        ulong addr = read_pointer((node_addr + i * sizeof(void *)), "kmem_cache_node addr");

        // Skip if the pointer is invalid (node may not be present on this system)
        if (!is_kvaddr(addr)) {
            LOGD("  Node[%d]: skipped (invalid address)", i);
            continue;
        }

        LOGD("  Node[%d]: reading kmem_cache_node at %#lx", i, addr);

        // Read the kmem_cache_node structure from kernel memory
        void *node_buf = read_struct(addr, "kmem_cache_node");
        if(node_buf == nullptr) {
            LOGE("  Node[%d]: failed to read kmem_cache_node at %#lx", i, addr);
            continue;
        }

        // Create a new node object and populate its fields
        std::shared_ptr<kmem_cache_node> node_ptr = std::make_shared<kmem_cache_node>();
        node_ptr->addr = addr;

        // Parse node statistics from the structure
        // nr_partial: Number of partially-filled slabs (have both free and allocated objects)
        node_ptr->nr_partial = ULONG(node_buf + field_offset(kmem_cache_node, nr_partial));

        // nr_slabs: Total number of slabs managed by this node
        node_ptr->nr_slabs = ULONG(node_buf + field_offset(kmem_cache_node, nr_slabs));

        // total_objects: Total number of objects across all slabs in this node
        node_ptr->total_objects = ULONG(node_buf + field_offset(kmem_cache_node, total_objects));

        LOGD("  Node[%d]: nr_partial=%lu, nr_slabs=%lu, total_objects=%lu",
             i, node_ptr->nr_partial, node_ptr->nr_slabs, node_ptr->total_objects);

        // Free the temporary buffer after extracting basic fields
        FREEBUF(node_buf);

        // Parse the partial slab list
        // Partial slabs are kept in a linked list for quick allocation
        ulong partial_addr = addr + field_offset(kmem_cache_node, partial);
        LOGD("  Node[%d]: parsing partial slab list at %#lx", i, partial_addr);
        node_ptr->partial = parser_slab_from_list(cache_ptr, partial_addr);

        size_t partial_obj_count = 0;
        for (const auto& slab : node_ptr->partial) {
            partial_obj_count += slab->totalobj;
        }
        LOGD("  Node[%d]: partial list contains %zu slab(s) with %zu total objects",
             i, node_ptr->partial.size(), partial_obj_count);

        // Parse the full slab list
        // Full slabs are kept separately and may be freed when all objects are deallocated
        ulong full_addr = addr + field_offset(kmem_cache_node, full);
        LOGD("  Node[%d]: parsing full slab list at %#lx", i, full_addr);
        node_ptr->full = parser_slab_from_list(cache_ptr, full_addr);

        size_t full_obj_count = 0;
        for (const auto& slab : node_ptr->full) {
            full_obj_count += slab->totalobj;
        }
        LOGD("  Node[%d]: full list contains %zu slab(s) with %zu total objects",
             i, node_ptr->full.size(), full_obj_count);

        // Add the successfully parsed node to the list
        node_list.push_back(node_ptr);
    }
    return node_list;
}

/**
 * Parse per-CPU slab information for a specific SLUB cache.
 *
 * In SMP (Symmetric Multi-Processing) systems, each CPU maintains its own set of slabs
 * to minimize lock contention and improve allocation performance. This function parses
 * all kmem_cache_cpu structures, extracting:
 * - Current active slab (the slab currently being used for allocations)
 * - Partial slab list (per-CPU cache of slabs with free objects)
 * - Transaction ID (for lockless operations)
 *
 * The per-CPU data uses the kernel's per-CPU variable mechanism, where each CPU's
 * data is accessed via an offset from a base address.
 *
 * @param cache_ptr Pointer to the parent kmem_cache structure
 * @param cpu_addr Base address of the per-CPU kmem_cache_cpu structure
 * @return Vector of parsed kmem_cache_cpu structures (one per active CPU)
 */
std::vector<std::shared_ptr<kmem_cache_cpu>> Slub::parser_kmem_cache_cpu(std::shared_ptr<kmem_cache> cache_ptr, ulong cpu_addr){
    std::vector<std::shared_ptr<kmem_cache_cpu>> cpu_list;
    std::vector<ulong> percpu_list = for_each_percpu(cpu_addr);
    size_t active_cpus = 0;
    for (size_t i = 0; i < percpu_list.size(); i++){
        ulong addr = percpu_list[i];
        // Validate the calculated address
        if (!is_kvaddr(addr)) {
            LOGD("  CPU[%zu]: skipped (invalid address %#lx)", i, addr);
            continue;
        }

        active_cpus++;
        LOGD("  CPU[%zu]: reading kmem_cache_cpu at %#lx", i, addr);

        // Read the kmem_cache_cpu structure from kernel memory
        void *cpu_buf = read_struct(addr, "kmem_cache_cpu");
        if(cpu_buf == nullptr) {
            LOGE("  CPU[%zu]: failed to read kmem_cache_cpu at %#lx", i, addr);
            continue;
        }

        // Create a new CPU slab object
        std::shared_ptr<kmem_cache_cpu> cpu_ptr = std::make_shared<kmem_cache_cpu>();
        cpu_ptr->addr = addr;

        // Parse the transaction ID (used for lockless operations)
        // The tid is incremented on each allocation/free to detect concurrent modifications
        cpu_ptr->tid = ULONG(cpu_buf + field_offset(kmem_cache_cpu, tid));
        // Parse the current active slab address
        // The field name changed from 'page' to 'slab' in newer kernels
        // We check which structure is available and use the appropriate field
        ulong cur_slab_addr = 0;
        if (struct_size(slab) != -1){
            // Newer kernel: use 'slab' field
            cur_slab_addr = ULONG(cpu_buf + field_offset(kmem_cache_cpu, slab));
        } else {
            // Older kernel: use 'page' field
            cur_slab_addr = ULONG(cpu_buf + field_offset(kmem_cache_cpu, page));
        }
        LOGD("  CPU[%zu]: current slab at %#lx", i, cur_slab_addr);
        // Parse the current active slab
        // This is the slab from which new allocations are served
        cpu_ptr->cur_slab = parser_slab(cache_ptr, cur_slab_addr);

        // Parse the per-CPU partial slab list
        // This is a linked list of slabs cached on this CPU for fast allocation
        // When the current slab is exhausted, the next slab comes from this list
        ulong partial_addr = ULONG(cpu_buf + field_offset(kmem_cache_cpu, partial));
        LOGD("  CPU[%zu]: partial slab list at %#lx", i, partial_addr);

        ulong slab_page_addr = partial_addr;
        size_t partial_count = 0;
        size_t partial_obj_count = 0;

        // Traverse the linked list of partial slabs
        while (is_kvaddr(slab_page_addr)){
            std::shared_ptr<slab> slab_ptr = parser_slab(cache_ptr, slab_page_addr);
            if (slab_ptr) {
                cpu_ptr->partial.push_back(slab_ptr);
                partial_count++;
                partial_obj_count += slab_ptr->totalobj;

                // Get the next slab in the list
                // Again, handle both old ('page') and new ('slab') structure names
                if (struct_size(slab) != -1){
                    slab_page_addr = read_structure_field(slab_page_addr, "slab", "next");
                } else {
                    slab_page_addr = read_structure_field(slab_page_addr, "page", "next");
                }
            } else {
                // Failed to parse slab, break to avoid infinite loop
                LOGE("  CPU[%zu]: failed to parse slab at %#lx, stopping list traversal", i, slab_page_addr);
                break;
            }
        }

        LOGD("  CPU[%zu]: partial list contains %zu slab(s) with %zu total objects",
                i, partial_count, partial_obj_count);

        // Free the temporary buffer
        FREEBUF(cpu_buf);

        // Add the successfully parsed CPU slab to the list
        cpu_list.push_back(cpu_ptr);
    }
    LOGD(" Scanned %zu active CPU(s), successfully parsed %zu per-CPU slab structure(s)",
             active_cpus, cpu_list.size());
    // Log summary statistics
    if (cpu_list.size() > 0) {
        size_t total_partial_slabs = 0;
        size_t total_current_slabs = 0;
        for (const auto& cpu : cpu_list) {
            total_partial_slabs += cpu->partial.size();
            if (cpu->cur_slab) {
                total_current_slabs++;
            }
        }
        LOGD(" Per-CPU summary: %zu CPU(s), %zu current slab(s), %zu partial slab(s)",
             cpu_list.size(), total_current_slabs, total_partial_slabs);
    }

    return cpu_list;
}

/**
 * Parse all SLUB kmem_cache structures from the kernel's slab_caches list.
 * This function builds a comprehensive database of all SLUB caches including:
 * - Cache metadata (size, flags, alignment, etc.)
 * - NUMA node information and associated slabs
 * - Per-CPU slab information
 * - Object statistics and memory usage
 *
 * The parsed data is stored in cache_list for subsequent analysis operations.
 */
void Slub::parser_kmem_caches(){
    LOGI("Starting SLAB cache parsing");

    // Clear any existing cache data to ensure fresh parsing
    cache_list.clear();

    // Validate that the slab_caches symbol exists in the kernel
    if (!csymbol_exists("slab_caches")){
        LOGE("slab_caches symbol doesn't exist in this kernel!");
        return;
    }

    // Get the address of the global slab_caches list head
    ulong slab_caches_addr = csymbol_value("slab_caches");
    if (!is_kvaddr(slab_caches_addr)){
        LOGE("Invalid slab_caches address: %#lx!", slab_caches_addr);
        return;
    }

    // Calculate the offset of the 'list' field within kmem_cache structure
    // This is used to traverse the linked list of caches
    int list_offset = field_offset(kmem_cache, list);
    LOGD("slab_caches list head at %#lx", slab_caches_addr);

    size_t cache_count = 0;
    size_t successful_parses = 0;

    // Iterate through all kmem_cache structures in the global list
    for (const auto& cache_addr : for_each_list(slab_caches_addr, list_offset)) {
        cache_count++;
        LOGD("Processing cache #%zu at address %#lx", cache_count, cache_addr);

        // Use the unified parse_kmem_cache function to parse each cache
        std::shared_ptr<kmem_cache> cache_ptr = parse_kmem_cache(cache_addr);
        if (cache_ptr) {
            successful_parses++;
        } else {
            LOGW("Failed to parse cache at %#lx", cache_addr);
        }
    }

    // Calculate aggregate statistics across all successfully parsed caches
    size_t total_slabs = 0;
    size_t total_objs = 0;

    for (const auto& cache_pair : cache_list) {
        const auto& cache_ptr = cache_pair.second;
        total_slabs += cache_ptr->total_nr_slabs;
        total_objs += cache_ptr->total_nr_objs;
    }

    LOGI("Parsing complete: %zu/%zu caches successfully parsed, %zu slabs, %zu objects",
         successful_parses, cache_count, total_slabs, total_objs);
}

std::shared_ptr<kmem_cache> Slub::parse_kmem_cache(std::string& name) {
    // Get the address of the global slab_caches list head
    ulong slab_caches_addr = csymbol_value("slab_caches");
    if (!is_kvaddr(slab_caches_addr)){
        LOGE("Invalid slab_caches address: %#lx!", slab_caches_addr);
        return nullptr;
    }
    // Iterate through all kmem_cache structures in the global list
    ulong target_cache_addr = 0;
    for (const auto& cache_addr : for_each_list(slab_caches_addr, field_offset(kmem_cache, list))) {
        ulong name_addr = read_pointer(cache_addr + field_offset(kmem_cache, name), "name");
        std::string cache_name = read_cstring(name_addr, 64, "kmem_cache_name");
        if (name == cache_name){
            target_cache_addr = cache_addr;
            break;
        }
    }
    if (!is_kvaddr(target_cache_addr)){
        return nullptr;
    }
    return parse_kmem_cache(target_cache_addr);
}

/**
 * Parse a single SLUB kmem_cache structure by its address.
 * This function is used for on-demand parsing when only a specific cache is needed.
 *
 * @param cache_addr Kernel virtual address of the kmem_cache structure
 * @return Shared pointer to parsed kmem_cache structure, or nullptr on failure
 */
std::shared_ptr<kmem_cache> Slub::parse_kmem_cache(ulong cache_addr) {
    // Validate the cache address
    if (!is_kvaddr(cache_addr)) {
        LOGE("Invalid cache address: %#lx", cache_addr);
        return nullptr;
    }

    // Read the kmem_cache structure from kernel memory
    void *cache_buf = read_struct(cache_addr, "kmem_cache");
    if (cache_buf == nullptr) {
        LOGE("Failed to read kmem_cache structure at %#lx", cache_addr);
        return nullptr;
    }

    // Create a new cache object to store parsed data
    std::shared_ptr<kmem_cache> cache_ptr = std::make_shared<kmem_cache>();
    cache_ptr->addr = cache_addr;

    // Parse basic cache properties from the structure
    cache_ptr->flags = UINT(cache_buf + field_offset(kmem_cache, flags));
    cache_ptr->min_partial = ULONG(cache_buf + field_offset(kmem_cache, min_partial));
    cache_ptr->size = UINT(cache_buf + field_offset(kmem_cache, size));
    cache_ptr->object_size = UINT(cache_buf + field_offset(kmem_cache, object_size));
    cache_ptr->offset = UINT(cache_buf + field_offset(kmem_cache, offset));
    cache_ptr->cpu_partial = UINT(cache_buf + field_offset(kmem_cache, cpu_partial));

    // Parse the 'oo' field which encodes both object count and page order
    cache_ptr->oo = UINT(cache_buf + field_offset(kmem_cache, oo));
    cache_ptr->per_slab_obj = cache_ptr->oo & 0x0000FFFF;
    cache_ptr->page_order = cache_ptr->oo >> 16;

    // Parse additional cache configuration
    cache_ptr->max = UINT(cache_buf + field_offset(kmem_cache, max));
    cache_ptr->min = UINT(cache_buf + field_offset(kmem_cache, min));
    cache_ptr->allocflags = UINT(cache_buf + field_offset(kmem_cache, allocflags));
    cache_ptr->refcount = INT(cache_buf + field_offset(kmem_cache, refcount));
    cache_ptr->inuse = UINT(cache_buf + field_offset(kmem_cache, inuse));
    cache_ptr->align = UINT(cache_buf + field_offset(kmem_cache, align));
    cache_ptr->red_left_pad = UINT(cache_buf + field_offset(kmem_cache, red_left_pad));
    cache_ptr->useroffset = UINT(cache_buf + field_offset(kmem_cache, useroffset));
    cache_ptr->usersize = UINT(cache_buf + field_offset(kmem_cache, usersize));

    // Read the cache name string
    cache_ptr->name = read_cstring(ULONG(cache_buf + field_offset(kmem_cache, name)), 64, "kmem_cache_name");

    // Parse the random value used for freelist pointer obfuscation
    cache_ptr->random = ULONG(cache_buf + field_offset(kmem_cache, random));
    LOGD("%s at %#lx obj_size=%u flags=%#x size=%u inuse=%u",
         cache_ptr->name.c_str(), cache_addr,
         cache_ptr->object_size, cache_ptr->flags, cache_ptr->size, cache_ptr->inuse);

    // Parse NUMA node information and associated slabs
    ulong node_addr = cache_addr + field_offset(kmem_cache, node);
    cache_ptr->node_list = parser_kmem_cache_node(cache_ptr, node_addr);

    // Parse per-CPU slab information
    ulong cpu_slab_addr = ULONG(cache_buf + field_offset(kmem_cache, cpu_slab));
    cache_ptr->cpu_slabs = parser_kmem_cache_cpu(cache_ptr, cpu_slab_addr);

    // Calculate aggregate statistics
    for (const auto& node_ptr : cache_ptr->node_list) {
        cache_ptr->total_nr_slabs += node_ptr->nr_slabs;
        cache_ptr->total_nr_objs += node_ptr->total_objects;
    }

    for (const auto& cpu_ptr : cache_ptr->cpu_slabs) {
        for (const auto& slab_ptr : cpu_ptr->partial) {
            cache_ptr->total_nr_slabs += 1;
            cache_ptr->total_nr_objs += slab_ptr->totalobj;
        }
        if (cpu_ptr->cur_slab != nullptr) {
            cache_ptr->total_nr_slabs += 1;
            cache_ptr->total_nr_objs += cpu_ptr->cur_slab->totalobj;
        }
    }

    cache_ptr->total_size = cache_ptr->total_nr_objs * cache_ptr->size;

    // Free the temporary buffer and add cache to the list
    FREEBUF(cache_buf);
    cache_list[cache_ptr->name] = cache_ptr;
    PRINT("Parsing kmem_cache %s at address: %#lx success\n", cache_ptr->name.c_str(), cache_addr);
    return cache_ptr;
}

/* ============================================================================
 * Memory Layout and Poison Helper Functions
 * ============================================================================ */

/**
 * @brief Calculate object size excluding red zone padding
 * @param cache_ptr Cache structure
 * @return Object size without red zone
 */
unsigned int Slub::size_from_object(std::shared_ptr<kmem_cache> cache_ptr){
    if (cache_ptr->flags & SLAB_RED_ZONE) {
        return cache_ptr->size - cache_ptr->red_left_pad;
    }
    return cache_ptr->size;
}

/**
 * @brief Validate that a pointer points to a valid object within a slab
 * @param cache_ptr Cache structure
 * @param page_addr Page address
 * @param object_start Object address to validate
 * @return True if pointer is valid
 */
bool Slub::check_valid_pointer(std::shared_ptr<kmem_cache> cache_ptr, ulong page_addr, ulong object_start){
    if(!is_kvaddr(object_start)) {
        return true;
    }
    ulong slab_vaddr = phy_to_virt(page_to_phy(page_addr));
    ulong object = restore_red_left(cache_ptr, object_start);
    ulong count;
    if (struct_size(slab) == -1){
        count = read_ulong(page_addr + field_offset(page, counters), "page counters");
    } else {
        count = read_ulong(page_addr + field_offset(slab, counters), "slab counters");
    }
    ulong objects = (count >> 16) & 0x00007FFF;
    if((object < slab_vaddr) || (object >= slab_vaddr + objects * cache_ptr->size) || ((object - slab_vaddr) % cache_ptr->size)) {
        return false;
    }
    return true;
}

/**
 * @brief Restore object address by removing left red zone offset
 * @param cache_ptr Cache structure
 * @param object_start Object start address
 * @return Address without red zone offset
 */
ulong Slub::restore_red_left(std::shared_ptr<kmem_cache> cache_ptr, ulong object_start){
    if (cache_ptr->flags & SLAB_RED_ZONE) {
        object_start -= cache_ptr->red_left_pad;
    }
    return object_start;
}

/**
 * @brief Get original size for kmalloc caches with size debugging
 * @param cache_ptr Cache structure
 * @return Original requested size
 */
unsigned int Slub::get_orig_size(std::shared_ptr<kmem_cache> cache_ptr){
    if(!slub_debug_orig_size(cache_ptr)) {
        return cache_ptr->object_size;
    }
    unsigned int p = get_info_end(cache_ptr);
    p += struct_size(track) * 2;
    return p;
}

/**
 * @brief Check if cache supports original size debugging
 * @param cache_ptr Cache structure
 * @return True if original size debugging is enabled
 */
bool Slub::slub_debug_orig_size(std::shared_ptr<kmem_cache> cache_ptr){
    return (cache_ptr->flags & SLAB_STORE_USER) && (cache_ptr->flags & SLAB_KMALLOC);
}

/**
 * @brief Check bytes using 8-byte aligned access for performance
 * @param start Start address
 * @param value Expected byte value
 * @param bytes Number of bytes to check
 * @return Address of first non-matching byte, or 0 if all match
 */
ulong Slub::check_bytes8(ulong start, uint8_t value, size_t bytes){
    while (bytes) {
        if (read_byte(start,"check_bytes8") != value) {
            return start;
        }
        start++;
        bytes--;
    }
    return 0;
}

/**
 * @brief Find first byte not matching the specified value (optimized version)
 *
 * This function efficiently searches for the first byte that doesn't match
 * the expected value, using 64-bit aligned access when possible for better
 * performance on large memory regions.
 *
 * @param start_addr Starting memory address
 * @param c Byte value to compare against
 * @param bytes Number of bytes to check
 * @return Address of first non-matching byte, or 0 if all bytes match
 */
ulong Slub::memchr_inv(ulong start_addr, uint8_t c, size_t bytes) {
    if (bytes == 0) return 0;
    uint8_t value = c;
    if (bytes <= 16) {
        return check_bytes8(start_addr, value, bytes);
    }
    uint64_t value64 = value;
    value64 |= value64 << 8;
    value64 |= value64 << 16;
    value64 |= value64 << 32;
    ulong prefix = start_addr & 7;
    if (prefix) {
        prefix = 8 - prefix;
        if (prefix > bytes) prefix = bytes;

        ulong r = check_bytes8(start_addr, value, prefix);
        if (r) return r;
        start_addr += prefix;
        bytes -= prefix;
    }
    while (bytes >= 8) {
        if (read_ulonglong(start_addr, "memchr_inv") != value64) {
            return check_bytes8(start_addr, value, 8);
        }
        start_addr += 8;
        bytes -= 8;
    }
    return check_bytes8(start_addr, value, bytes);
}

/**
 * @brief Adjust object address to account for left red zone
 * @param cache_ptr Cache structure
 * @param object_start_addr Object start address
 * @return Adjusted address pointing to actual object data
 */
ulong Slub::fixup_red_left(std::shared_ptr<kmem_cache> cache_ptr, ulong object_start_addr){
    if(cache_ptr->flags & SLAB_RED_ZONE){
        object_start_addr += cache_ptr->red_left_pad;
    }
    return object_start_addr;
}

/**
 * @brief Check if free pointer is stored outside object data
 * @param cache_ptr Cache structure
 * @return True if free pointer is outside object
 */
bool Slub::freeptr_outside_object(std::shared_ptr<kmem_cache> cache_ptr){
    return cache_ptr->offset >= cache_ptr->inuse;
}

/**
 * @brief Get the end offset of object info area
 * @param cache_ptr Cache structure
 * @return End offset of info area
 */
unsigned int Slub::get_info_end(std::shared_ptr<kmem_cache> cache_ptr){
    if(freeptr_outside_object(cache_ptr)){
        return cache_ptr->inuse + sizeof(void *);
    } else {
        return cache_ptr->inuse;
    }
}

/**
 * @brief Extract free pointer from object (handling obfuscation)
 *
 * This function extracts the free pointer from a SLUB object, handling
 * the security obfuscation that may be applied when CONFIG_SLAB_FREELIST_HARDENED
 * is enabled. The obfuscation uses XOR with a random value and the pointer address.
 *
 * @param cache_ptr Cache structure
 * @param object_start_addr Object start address
 * @return Free pointer value (deobfuscated if necessary)
 */
ulong Slub::get_free_pointer(std::shared_ptr<kmem_cache> cache_ptr, ulong object_start_addr){
    ulong ptr_addr = object_start_addr + cache_ptr->offset;
    ulong ptr = read_pointer(ptr_addr, "obj freeptr");
    if (BITS64()){
        ptr_addr = swap64(ptr_addr, 1);
    }else{
        ptr_addr = swap32(ptr_addr, 1);
    }
    if (get_config_val("CONFIG_SLAB_FREELIST_HARDENED") == "y") {
        return ptr ^ cache_ptr->random ^ ptr_addr;
    } else {
        return ptr;
    }
}

/* ============================================================================
 * Stack Trace Analysis Functions
 * ============================================================================ */

/**
 * @brief Extract and format call stack from kernel address
 *
 * This function resolves a kernel virtual address to its corresponding symbol
 * and formats it as a human-readable call stack entry. It performs symbol
 * lookup using the kernel's symbol table to identify the function name and
 * offset within that function.
 *
 * @param frames_addr Kernel virtual address of instruction pointer
 * @return Formatted string containing symbol information
 */
std::string Slub::extract_callstack(ulong frames_addr){
    std::ostringstream oss;
    oss << "   [<" << std::hex << frames_addr << ">] " << to_symbol(frames_addr) << "\n";
    return oss.str();
}

/**
 * @brief Parse stack trace from track structure
 *
 * This function extracts allocation or deallocation stack trace information
 * stored in a SLUB object's track structure, handling both stack depot and
 * inline storage mechanisms.
 *
 * @param track_addr Address of track structure
 * @param track_ptr Track structure to populate
 */
void Slub::parser_obj_trace(ulong track_addr, std::shared_ptr<track>& track_ptr){
    // Validate input parameters
    if (!track_ptr) {
        LOGE("          parser_obj_trace called with null track_ptr");
        return;
    }
    if (!is_kvaddr(track_addr)) {
        LOGE("          Invalid track address %#lx", track_addr);
        return;
    }

    // Read the track structure from kernel memory
    void* track_buf = read_struct(track_addr, "track");
    if (!track_buf) {
        LOGE("          Failed to read track at %#lx", track_addr);
        return;
    }

    // Extract basic tracking information
    track_ptr->cpu = UINT(track_buf + field_offset(track, cpu));
    track_ptr->pid = std::max(0U, UINT(track_buf + field_offset(track, pid)));
    track_ptr->when = ULONG(track_buf + field_offset(track, when));

    LOGD("          track:%#lx, CPU=%u, PID=%u, timestamp=%lu",track_addr,
         track_ptr->cpu, track_ptr->pid, track_ptr->when);

    // Free the temporary buffer after extracting basic fields
    FREEBUF(track_buf);

    // Determine which stack storage mechanism is in use
    bool use_stack_depot = (field_offset(track, handle) != -1) &&
                           (get_config_val("CONFIG_STACKDEPOT") == "y") &&
                           !read_bool(csymbol_value("stack_depot_disabled"), "stack_depot_disabled");

    if (use_stack_depot) {
        // Method 1: Stack Depot - centralized stack storage
        LOGD("          Using stack depot for trace extraction");
        ulong handle_parts_addr = track_addr + field_offset(track, handle);
        uint handle = read_uint(handle_parts_addr, "track handle");
        LOGD("          Stack depot handle: %#x", handle);

        if (handle == 0) {
            LOGW("          Stack depot handle is zero - no trace available");
            return;
        }

        // Retrieve the stack record from the depot using the handle
        std::shared_ptr<stack_record_t> record_ptr = get_stack_record(handle);
        if (record_ptr != nullptr){
            LOGD("          Retrieved stack record from depot, extracting call stack");
            track_ptr->frame += get_call_stack(record_ptr);
            LOGD("          Stack depot extraction complete: %zu bytes", track_ptr->frame.size());
        } else {
            LOGW("          Failed to retrieve stack record for handle %#x", handle);
        }
    } else {
        // Method 2: Inline Storage - stack frames stored directly
        ulong track_addrs_addr = track_addr + field_offset(track, addrs);
        uint frame_size = field_size(track, addrs) / sizeof(unsigned long);
        LOGD("          stack array at %#lx, capacity: %u frames", track_addrs_addr, frame_size);

        // Validate the frame array address and size
        if (!is_kvaddr(track_addrs_addr)) {
            LOGW("          Invalid stack array address %#lx", track_addrs_addr);
            return;
        }
        if (frame_size > 16) {
            LOGW("          Suspicious stack size %u (max 16), limiting to 16", frame_size);
            frame_size = 16;
        }

        for(uint i = 0; i < frame_size; i++){
            // Read the return address for this frame
            ulong frame_addr = read_pointer(track_addrs_addr + sizeof(unsigned long) * i, "frame_addr");

            // Only process valid kernel addresses
            if(is_kvaddr(frame_addr)){
                // Resolve the address to symbol+offset and add to the call stack
                std::string frame_str = extract_callstack(frame_addr);
                if (!frame_str.empty()) {
                    track_ptr->frame += frame_str;
                }
            } else {
                // Invalid frame address but not zero - may be corrupted
                std::ostringstream oss;
                oss << "   [<" << std::hex << frame_addr << ">] UNKNOWN_SYMBOL\n";
                track_ptr->frame += oss.str();
            }
        }
    }

    // Log final result
    if (track_ptr->frame.empty()) {
        LOGW("          No stack trace extracted for track at %#lx", track_addr);
    } else {
        LOGD("          Successfully extracted %zu bytes of stack trace", track_ptr->frame.size());
    }
}

/**
 * @brief Get address of track structure for an object
 * @param cache_ptr Cache structure
 * @param object_start_addr Object start address
 * @param track_type Track type (TRACK_ALLOC or TRACK_FREE)
 * @return Address of track structure
 */
ulong Slub::get_obj_track_addr(std::shared_ptr<kmem_cache> cache_ptr, ulong object_start_addr, uint8_t track_type){
    unsigned int track_size = struct_size(track);
    return object_start_addr + cache_ptr->red_left_pad + get_info_end(cache_ptr) + track_type * track_size;
}

/* ============================================================================
 * Statistics Collection and Analysis Functions
 * ============================================================================ */


/**
 * @brief Print frame statistics information in a formatted way
 * @param sorted_frames Vector of sorted frame statistics
 * @param total_allocations Total number of allocations across all frames
 * @param total_memory Total memory usage across all frames
 */
void Slub::print_frame_statistics_info(const std::vector<std::pair<std::string, FrameStatistics>>& sorted_frames,
                                      size_t total_allocations, size_t total_memory) {
    for (size_t i = 0; i < sorted_frames.size(); i++) {
        const auto& frame_pair = sorted_frames[i];
        const auto& frame = frame_pair.first;
        const auto& stats = frame_pair.second;
        size_t frame_hash = std::hash<std::string>{}(frame);

        PRINT("[%zu]Frame ID:%zu - Allocations: %zu, Memory: %s\n",
                i + 1, frame_hash, stats.total_allocations, csize(stats.total_memory).c_str());

        // Show call stack (first few lines)
        std::istringstream iss(frame);
        std::string line;
        while (std::getline(iss, line)) {
            if (!line.empty()) {
                size_t start = line.find_first_not_of(" \t");
                if (start != std::string::npos) {
                    line = line.substr(start);
                    PRINT("  %s\n", line.c_str());
                }
            }
        }

        // Sort PIDs by memory usage within this frame
        std::vector<std::pair<int, PidStatistics>> sorted_pids;
        sorted_pids.reserve(stats.pid_stats.size());
        for (const auto& pid_pair : stats.pid_stats) {
            sorted_pids.emplace_back(pid_pair.first, pid_pair.second);
        }

        std::sort(sorted_pids.begin(), sorted_pids.end(),
                  [](const auto& a, const auto& b) {
                      return a.second.total_memory > b.second.total_memory;
                  });

        PRINT("  Per-Process Breakdown:\n");
        PRINT("     %-10s %-10s %-15s\n", "PID", "Count", "Memory");
        PRINT("     %-10s %-10s %-15s\n", "---", "-----", "------");

        // Show top DEFAULT_TOP_COUNT PIDs for this frame (or all if less than DEFAULT_TOP_COUNT)
        size_t pid_display_count = std::min(sorted_pids.size(), static_cast<size_t>(DEFAULT_TOP_COUNT));
        for (size_t j = 0; j < pid_display_count; j++) {
            const auto& pid_pair = sorted_pids[j];
            const auto& pid = pid_pair.first;
            const auto& pid_stats = pid_pair.second;
            PRINT("     %-10d %-10zu %-15s\n",
                  pid, pid_stats.allocation_count, csize(pid_stats.total_memory).c_str());
        }

        if (sorted_pids.size() > DEFAULT_TOP_COUNT) {
            PRINT("     ... and %zu more processes\n", sorted_pids.size() - DEFAULT_TOP_COUNT);
        }
        PRINT("\n");
    }
}

/**
 * @brief Display frame allocation statistics sorted by total memory usage
 */
void Slub::print_alloc_mem_by_stack() {
    // Collect frame statistics from alloc_list across all caches
    std::unordered_map<std::string, FrameStatistics> frame_stats;

    for (const auto& cache_pair : cache_list) {
        const auto& cache_ptr = cache_pair.second;

        // Check if the cache has trace support
        if ((cache_ptr->flags & SLAB_STORE_USER) == 0) {
            continue;
        }

        // Process allocation tracks
        for (const auto& track_ptr : cache_ptr->alloc_list) {
            if (!track_ptr || track_ptr->frame.empty()) {
                continue;
            }

            size_t alloc_size = track_ptr->kmem_cache_ptr->size;

            // Update frame-level statistics
            auto& frame_stat = frame_stats[track_ptr->frame];
            frame_stat.total_allocations++;
            frame_stat.total_memory += alloc_size;

            // Update PID-level statistics within this frame
            auto& pid_stat = frame_stat.pid_stats[track_ptr->pid];
            pid_stat.allocation_count++;
            pid_stat.total_memory += alloc_size;
        }
    }

    if (frame_stats.empty()) {
        PRINT("No frame statistics available. Ensure SLAB_STORE_USER is enabled.\n");
        return;
    }

    // Convert to vector for sorting
    std::vector<std::pair<std::string, FrameStatistics>> sorted_frames;
    sorted_frames.reserve(frame_stats.size());

    for (const auto& frame_pair : frame_stats) {
        sorted_frames.emplace_back(frame_pair.first, frame_pair.second);
    }

    // Sort by total memory usage (descending)
    std::sort(sorted_frames.begin(), sorted_frames.end(),
              [](const auto& a, const auto& b) {
                  return a.second.total_memory > b.second.total_memory;
              });

    // Limit output if requested
    if (DEFAULT_TOP_COUNT > 0 && sorted_frames.size() > DEFAULT_TOP_COUNT) {
        sorted_frames.resize(DEFAULT_TOP_COUNT);
    }

    // Calculate totals
    size_t total_allocations = 0;
    size_t total_memory = 0;
    for (const auto& frame_pair : frame_stats) {
        const auto& stats = frame_pair.second;
        total_allocations += stats.total_allocations;
        total_memory += stats.total_memory;
    }

    // Print enhanced header
    PRINT("================================================================================\n");
    PRINT("                        SLUB ALLOCATION STATISTICS\n");
    PRINT("================================================================================\n");
    PRINT("Call Stacks: %-8zu | Total Allocations: %-10zu | Total Memory: %s\n",
          frame_stats.size(), total_allocations, csize(total_memory).c_str());
    PRINT("================================================================================\n");

    // Use the new encapsulated function to print frame statistics
    print_frame_statistics_info(sorted_frames, total_allocations, total_memory);
}

/**
 * @brief Display global PID memory allocation statistics
 */
void Slub::print_alloc_mem_by_pid() {
    // Collect global PID statistics from alloc_list across all caches
    std::unordered_map<int, PidStatistics> pid_stats;

    for (const auto& cache_pair : cache_list) {
        const auto& cache_ptr = cache_pair.second;

        // Check if the cache has trace support
        if ((cache_ptr->flags & SLAB_STORE_USER) == 0) {
            continue;
        }

        // Process allocation tracks
        for (const auto& track_ptr : cache_ptr->alloc_list) {
            if (!track_ptr || track_ptr->frame.empty()) {
                continue;
            }

            size_t alloc_size = track_ptr->kmem_cache_ptr->size;
            size_t frame_hash = std::hash<std::string>{}(track_ptr->frame);

            // Update global PID statistics
            auto& global_pid_stat = pid_stats[track_ptr->pid];
            global_pid_stat.allocation_count++;
            global_pid_stat.total_memory += alloc_size;

            // Track which frames this PID uses
            global_pid_stat.frame_memory[frame_hash] += alloc_size;
        }
    }

    if (pid_stats.empty()) {
        PRINT("No global PID statistics available. Ensure SLAB_STORE_USER is enabled.\n");
        return;
    }

    // Convert to vector for sorting
    std::vector<std::pair<int, PidStatistics>> sorted_pids;
    sorted_pids.reserve(pid_stats.size());

    for (const auto& pid_pair : pid_stats) {
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
    for (const auto& pid_pair : pid_stats) {
        const auto& stats = pid_pair.second;
        total_allocations += stats.allocation_count;
        total_memory += stats.total_memory;
    }

    // Print enhanced header
    PRINT("================================================================================\n");
    PRINT("                        SLUB ALLOCATION STATISTICS (Top 20 by memory usage)\n");
    PRINT("================================================================================\n");
    PRINT("Process: %-8zu | Total Allocations: %-10zu | Total Memory: %s\n",
          pid_stats.size(), total_allocations, csize(total_memory).c_str());
    PRINT("================================================================================\n");

    // Print header
    PRINT("%-10s %-15s %-15s %-10s\n",
          "PID", "Allocations", "Total Memory", "Frames");
    PRINT("%-10s %-15s %-15s %-10s\n",
          "---", "-----------", "------------", "------");

    // Print data rows
    for (const auto& pid_pair : sorted_pids) {
        int pid = pid_pair.first;
        const auto& stats = pid_pair.second;

        PRINT("%-10d %-15zu %-15s %-10zu\n",
              pid,
              stats.allocation_count,
              csize(stats.total_memory).c_str(),
              stats.frame_memory.size());
    }
}

/**
 * @brief Display detailed memory allocation for a specific PID
 * @param pid Process ID to analyze
 */
void Slub::print_pid_details(int pid) {
    // Collect PID statistics from alloc_list across all caches
    PidStatistics pid_stats;
    std::unordered_map<size_t, size_t> frame_memory;

    for (const auto& cache_pair : cache_list) {
        const auto& cache_ptr = cache_pair.second;

        // Check if the cache has trace support
        if ((cache_ptr->flags & SLAB_STORE_USER) == 0) {
            continue;
        }

        // Process allocation tracks for this specific PID
        for (const auto& track_ptr : cache_ptr->alloc_list) {
            if (!track_ptr || track_ptr->frame.empty() || track_ptr->pid != pid) {
                continue;
            }

            size_t alloc_size = track_ptr->kmem_cache_ptr->size;
            size_t frame_hash = std::hash<std::string>{}(track_ptr->frame);

            // Update PID statistics
            pid_stats.allocation_count++;
            pid_stats.total_memory += alloc_size;

            // Track which frames this PID uses
            frame_memory[frame_hash] += alloc_size;
        }
    }

    if (pid_stats.allocation_count == 0) {
        PRINT("PID %d not found in global statistics\n", pid);
        return;
    }

    PRINT("================================================================================\n");
    PRINT("                           Detailed PID Analysis\n");
    PRINT("PID: %d | Total Allocations: %zu | Total Memory: %s | Frame count: %zu\n",
          pid, pid_stats.allocation_count, csize(pid_stats.total_memory).c_str(), frame_memory.size());
    PRINT("================================================================================\n");

    // Display frame breakdown sorted by memory usage
    if (!frame_memory.empty()) {
        PRINT("%-30s    %-18s    %-12s\n", "Frame ID", "Memory", "Percentage");
        PRINT("%-30s    %-18s    %-12s\n", "--------", "------", "----------");

        // Sort frames by memory usage
        std::vector<std::pair<size_t, size_t>> sorted_frames;
        sorted_frames.reserve(frame_memory.size());

        for (const auto& frame_pair : frame_memory) {
            sorted_frames.emplace_back(frame_pair.first, frame_pair.second);
        }

        std::sort(sorted_frames.begin(), sorted_frames.end(),
                  [](const auto& a, const auto& b) {
                      return a.second > b.second;
                  });

        // Show top DEFAULT_TOP_COUNT frames
        size_t display_count = std::min(sorted_frames.size(), static_cast<size_t>(DEFAULT_TOP_COUNT));
        for (size_t i = 0; i < display_count; i++) {
            const auto& frame_pair = sorted_frames[i];
            size_t frame_id = frame_pair.first;
            size_t frame_memory = frame_pair.second;

            double percentage = pid_stats.total_memory > 0 ?
                               (static_cast<double>(frame_memory) / pid_stats.total_memory) * 100.0 : 0.0;

            PRINT("%-30zu    %-18s    %.1f%%\n",
                  frame_id,
                  csize(frame_memory).c_str(),
                  percentage);
        }

        if (sorted_frames.size() > DEFAULT_TOP_COUNT) {
            PRINT("... and %zu more frames\n", sorted_frames.size() - 10);
        }

        PRINT("\nUse 'slub -f <frame_id>' to view detailed stack information.\n");
    }

    PRINT("================================================================================\n");
}

/**
 * @brief Display detailed stack information for a specific frame ID
 * @param frame_id Hash ID of the frame to display
 */
void Slub::print_frame_stack_info(size_t frame_id) {
    // Find the frame by hash from alloc_list across all caches
    std::string target_frame;
    FrameStatistics target_stats;
    bool found = false;

    for (const auto& cache_pair : cache_list) {
        const auto& cache_ptr = cache_pair.second;

        // Check if the cache has trace support
        if ((cache_ptr->flags & SLAB_STORE_USER) == 0) {
            continue;
        }

        // Process allocation tracks to find the matching frame
        for (const auto& track_ptr : cache_ptr->alloc_list) {
            if (!track_ptr || track_ptr->frame.empty()) {
                continue;
            }

            size_t current_frame_hash = std::hash<std::string>{}(track_ptr->frame);
            if (current_frame_hash == frame_id) {
                target_frame = track_ptr->frame;
                found = true;
                break;
            }
        }
        if (found) break;
    }

    if (!found) {
        PRINT("Frame ID %zu not found in statistics\n", frame_id);
        return;
    }

    // Now collect statistics for this specific frame
    std::unordered_map<int, PidStatistics> pid_stats;
    size_t total_allocations = 0;
    size_t total_memory = 0;

    for (const auto& cache_pair : cache_list) {
        const auto& cache_ptr = cache_pair.second;

        // Check if the cache has trace support
        if ((cache_ptr->flags & SLAB_STORE_USER) == 0) {
            continue;
        }

        // Process allocation tracks for this specific frame
        for (const auto& track_ptr : cache_ptr->alloc_list) {
            if (!track_ptr || track_ptr->frame.empty()) {
                continue;
            }

            size_t current_frame_hash = std::hash<std::string>{}(track_ptr->frame);
            if (current_frame_hash != frame_id) {
                continue;
            }

            size_t alloc_size = track_ptr->kmem_cache_ptr->size;
            total_allocations++;
            total_memory += alloc_size;

            // Update PID-level statistics within this frame
            auto& pid_stat = pid_stats[track_ptr->pid];
            pid_stat.allocation_count++;
            pid_stat.total_memory += alloc_size;
        }
    }

    PRINT("================================================================================\n");
    PRINT("Frame Stack Information - ID: %zu\n", frame_id);
    PRINT("Total Allocations: %zu | Total Memory: %s \n", total_allocations, csize(total_memory).c_str());
    PRINT("================================================================================\n");
    std::istringstream iss(target_frame);
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty()) {
            size_t start = line.find_first_not_of(" \t");
            if (start != std::string::npos) {
                line = line.substr(start);
                PRINT("  %s\n", line.c_str());
            }
        }
    }

    // Sort PIDs by memory usage
    std::vector<std::pair<int, PidStatistics>> sorted_pids;
    sorted_pids.reserve(pid_stats.size());

    for (const auto& pid_pair : pid_stats) {
        sorted_pids.emplace_back(pid_pair.first, pid_pair.second);
    }

    std::sort(sorted_pids.begin(), sorted_pids.end(),
              [](const auto& a, const auto& b) {
                  return a.second.total_memory > b.second.total_memory;
              });

    PRINT("%-10s %-15s %-15s\n", "PID", "Allocations", "Memory");
    PRINT("%-10s %-15s %-15s\n", "---", "-----------", "------");

    for (const auto& pid_pair : sorted_pids) {
        int pid = pid_pair.first;
        const auto& pid_stats = pid_pair.second;
        PRINT("%-10d %-15zu %-15s\n",
              pid,
              pid_stats.allocation_count,
              csize(pid_stats.total_memory).c_str());
    }

    PRINT("================================================================================\n");
}

/**
 * @brief Print object information for all slabs or filtered by cache
 * @param cache_name Cache name or address to name (empty = all caches)
 */
void Slub::print_slab_obj_info(const std::string& cache_name) {
    LOGI("Printing slab object information%s",
         cache_name.empty() ? " (all caches)" : (" for cache: " + cache_name).c_str());
    if (cache_name.empty()) {
        for (const auto& cache_pair : cache_list) {
            const auto& cache_ptr = cache_pair.second;
            print_kmem_cache(cache_ptr);
        }
    }else{
        for (const auto& cache_pair : cache_list) {
            const auto& cache_ptr = cache_pair.second;
            if (cache_ptr->name == cache_name) {
                print_kmem_cache(cache_ptr);
            }
        }
    }
}

/**
 * @brief Print SLUB poison information with cache filtering support
 * @param cache_name Cache name
 */
void Slub::print_slub_poison(const std::string& cache_name) {
    LOGI("Checking memory corruption%s",
         cache_name.empty() ? " (all caches)" : (" for cache: " + cache_name).c_str());
    if (cache_name.empty()) {
        for (const auto& cache_pair : cache_list) {
            const auto& cache_ptr = cache_pair.second;
            print_slub_poison(cache_ptr->addr);
        }
    }else{
        for (const auto& cache_pair : cache_list) {
            const auto& cache_ptr = cache_pair.second;
            if (cache_ptr->name == cache_name) {
                print_slub_poison(cache_ptr->addr);
            }
        }
    }
}

/**
 * @brief Print top allocation traces with cache filtering support
 * @param cache_name Cache name or address to name (empty = all caches)
 */
void Slub::print_top_alloc_trace(const std::string& cache_name) {
    LOGI("Printing top %zu allocation traces%s",
         DEFAULT_TOP_COUNT,
         cache_name.empty() ? " (all caches)" : (" for cache: " + cache_name).c_str());

    if (cache_name.empty()) {
        // Print top entries for all caches
        print_alloc_mem_by_stack();
    } else {
        // Print top entries for specific cache
        print_cache_alloc_mem_by_stack(cache_name);
    }
}

/**
 * @brief Display frame allocation statistics for a specific cache
 * @param cache_name Cache name
 */
void Slub::print_cache_alloc_mem_by_stack(const std::string& cache_name) {
    // Find the target cache
    auto it = cache_list.find(cache_name);
    if (it == cache_list.end()) {
        PRINT("Cache not found: %s", cache_name.c_str());
        return;
    }
    auto target_cache = it->second;
    LOGD("Located cache '%s' at %#lx", target_cache->name.c_str(), target_cache->addr);

    // Check if the cache has trace support
    if ((target_cache->flags & SLAB_STORE_USER) == 0) {
        PRINT("Cache %s SLAB_STORE_USER not enabled\n", target_cache->name.c_str());
        return;
    }
    // Collect frame statistics specific to this cache
    std::unordered_map<std::string, FrameStatistics> cache_frame_stats;

    // Process allocation tracks
    for (const auto& track_ptr : target_cache->alloc_list) {
        if (!track_ptr || track_ptr->frame.empty()) {
            continue;
        }

        size_t alloc_size = track_ptr->kmem_cache_ptr->size;

        // Update frame-level statistics
        auto& frame_stat = cache_frame_stats[track_ptr->frame];
        frame_stat.total_allocations++;
        frame_stat.total_memory += alloc_size;

        // Update PID-level statistics within this frame
        auto& pid_stat = frame_stat.pid_stats[track_ptr->pid];
        pid_stat.allocation_count++;
        pid_stat.total_memory += alloc_size;
    }
    if (cache_frame_stats.empty()) {
        PRINT("No frame statistics available for cache '%s'\n", target_cache->name.c_str());
        return;
    }

    // Convert to vector for sorting
    std::vector<std::pair<std::string, FrameStatistics>> sorted_frames;
    sorted_frames.reserve(cache_frame_stats.size());
    for (const auto& frame_pair : cache_frame_stats) {
        sorted_frames.emplace_back(frame_pair.first, frame_pair.second);
    }
    // Sort by total memory usage (descending)
    std::sort(sorted_frames.begin(), sorted_frames.end(),
              [](const auto& a, const auto& b) {
                  return a.second.total_memory > b.second.total_memory;
              });

    // Limit to top
    if (sorted_frames.size() > DEFAULT_TOP_COUNT) {
        sorted_frames.resize(DEFAULT_TOP_COUNT);
    }
    // Calculate totals for this cache
    size_t total_allocations = 0;
    size_t total_memory = 0;
    for (const auto& frame_pair : cache_frame_stats) {
        const auto& stats = frame_pair.second;
        total_allocations += stats.total_allocations;
        total_memory += stats.total_memory;
    }
    // Print enhanced header
    PRINT("================================================================================\n");
    PRINT("                        SLUB ALLOCATION STATISTICS\n");
    PRINT("================================================================================\n");
    PRINT("Cache Address: %#lx | Object Size: %u bytes | Flags: %#x\n",
          target_cache->addr, target_cache->object_size, target_cache->flags);
    PRINT("Call Stacks: %-8zu | Total Allocations: %-10zu | Total Memory: %s\n",
          cache_frame_stats.size(), total_allocations, csize(total_memory).c_str());
    PRINT("================================================================================\n");

    // Use the new encapsulated function to print frame statistics
    print_frame_statistics_info(sorted_frames, total_allocations, total_memory);
}

/**
 * @brief Print top free traces with cache filtering support
 * @param cache_name Cache name or address to name (empty = all caches)
 */
void Slub::print_top_free_trace(const std::string& cache_name) {
    LOGI("Printing top %zu free traces%s",
         DEFAULT_TOP_COUNT,
         cache_name.empty() ? " (all caches)" : (" for cache: " + cache_name).c_str());

    if (cache_name.empty()) {
        // Print top entries for all caches
        print_free_mem_by_stack();
    } else {
        // Print top entries for specific cache
        print_cache_free_mem_by_stack(cache_name);
    }
}

/**
 * @brief Display frame free statistics sorted by total memory usage
 */
void Slub::print_free_mem_by_stack() {
    // Collect frame statistics from free_list across all caches
    std::unordered_map<std::string, FrameStatistics> free_frame_stats;

    for (const auto& cache_pair : cache_list) {
        const auto& cache_ptr = cache_pair.second;

        // Check if the cache has trace support
        if ((cache_ptr->flags & SLAB_STORE_USER) == 0) {
            continue;
        }

        // Process free tracks
        for (const auto& track_ptr : cache_ptr->free_list) {
            if (!track_ptr || track_ptr->frame.empty()) {
                continue;
            }

            size_t alloc_size = track_ptr->kmem_cache_ptr->size;

            // Update frame-level statistics
            auto& frame_stat = free_frame_stats[track_ptr->frame];
            frame_stat.total_allocations++;
            frame_stat.total_memory += alloc_size;

            // Update PID-level statistics within this frame
            auto& pid_stat = frame_stat.pid_stats[track_ptr->pid];
            pid_stat.allocation_count++;
            pid_stat.total_memory += alloc_size;
        }
    }

    if (free_frame_stats.empty()) {
        PRINT("No free frame statistics available. Ensure SLAB_STORE_USER is enabled.\n");
        return;
    }

    // Convert to vector for sorting
    std::vector<std::pair<std::string, FrameStatistics>> sorted_frames;
    sorted_frames.reserve(free_frame_stats.size());

    for (const auto& frame_pair : free_frame_stats) {
        sorted_frames.emplace_back(frame_pair.first, frame_pair.second);
    }

    // Sort by total memory usage (descending)
    std::sort(sorted_frames.begin(), sorted_frames.end(),
              [](const auto& a, const auto& b) {
                  return a.second.total_memory > b.second.total_memory;
              });

    // Limit output if requested
    if (DEFAULT_TOP_COUNT > 0 && sorted_frames.size() > DEFAULT_TOP_COUNT) {
        sorted_frames.resize(DEFAULT_TOP_COUNT);
    }

    // Calculate totals
    size_t total_allocations = 0;
    size_t total_memory = 0;
    for (const auto& frame_pair : free_frame_stats) {
        const auto& stats = frame_pair.second;
        total_allocations += stats.total_allocations;
        total_memory += stats.total_memory;
    }

    // Print enhanced header
    PRINT("================================================================================\n");
    PRINT("                        SLUB FREE STATISTICS\n");
    PRINT("================================================================================\n");
    PRINT("Call Stacks: %-8zu | Total Frees: %-10zu | Total Memory: %s\n",
          free_frame_stats.size(), total_allocations, csize(total_memory).c_str());
    PRINT("================================================================================\n");

    // Use the existing function to print frame statistics
    print_frame_statistics_info(sorted_frames, total_allocations, total_memory);
}

/**
 * @brief Display frame free statistics for a specific cache
 * @param cache_name Cache name
 */
void Slub::print_cache_free_mem_by_stack(const std::string& cache_name) {
    // Find the target cache
    auto it = cache_list.find(cache_name);
    if (it == cache_list.end()) {
        PRINT("Cache not found: %s", cache_name.c_str());
        return;
    }
    auto target_cache = it->second;
    LOGD("Located cache '%s' at %#lx", target_cache->name.c_str(), target_cache->addr);

    // Check if the cache has trace support
    if ((target_cache->flags & SLAB_STORE_USER) == 0) {
        PRINT("Cache %s SLAB_STORE_USER not enabled\n", target_cache->name.c_str());
        return;
    }

    // Collect frame statistics specific to this cache
    std::unordered_map<std::string, FrameStatistics> cache_frame_stats;

    // Process free tracks
    for (const auto& track_ptr : target_cache->free_list) {
        if (!track_ptr || track_ptr->frame.empty()) {
            continue;
        }

        size_t alloc_size = track_ptr->kmem_cache_ptr->size;

        // Update frame-level statistics
        auto& frame_stat = cache_frame_stats[track_ptr->frame];
        frame_stat.total_allocations++;
        frame_stat.total_memory += alloc_size;

        // Update PID-level statistics within this frame
        auto& pid_stat = frame_stat.pid_stats[track_ptr->pid];
        pid_stat.allocation_count++;
        pid_stat.total_memory += alloc_size;
    }

    if (cache_frame_stats.empty()) {
        PRINT("No free frame statistics available for cache '%s'\n", target_cache->name.c_str());
        return;
    }

    // Convert to vector for sorting
    std::vector<std::pair<std::string, FrameStatistics>> sorted_frames;
    sorted_frames.reserve(cache_frame_stats.size());
    for (const auto& frame_pair : cache_frame_stats) {
        sorted_frames.emplace_back(frame_pair.first, frame_pair.second);
    }

    // Sort by total memory usage (descending)
    std::sort(sorted_frames.begin(), sorted_frames.end(),
              [](const auto& a, const auto& b) {
                  return a.second.total_memory > b.second.total_memory;
              });

    // Limit to top
    if (sorted_frames.size() > DEFAULT_TOP_COUNT) {
        sorted_frames.resize(DEFAULT_TOP_COUNT);
    }

    // Calculate totals for this cache
    size_t total_allocations = 0;
    size_t total_memory = 0;
    for (const auto& frame_pair : cache_frame_stats) {
        const auto& stats = frame_pair.second;
        total_allocations += stats.total_allocations;
        total_memory += stats.total_memory;
    }

    // Print enhanced header
    PRINT("================================================================================\n");
    PRINT("                        SLUB FREE STATISTICS\n");
    PRINT("================================================================================\n");
    PRINT("Cache Address: %#lx | Object Size: %u bytes | Flags: %#x\n",
          target_cache->addr, target_cache->object_size, target_cache->flags);
    PRINT("Call Stacks: %-8zu | Total Frees: %-10zu | Total Memory: %s\n",
          cache_frame_stats.size(), total_allocations, csize(total_memory).c_str());
    PRINT("================================================================================\n");

    // Use the existing function to print frame statistics
    print_frame_statistics_info(sorted_frames, total_allocations, total_memory);
}

/* ============================================================================
 * Memory Corruption Detection Functions
 * ============================================================================ */

/**
 * @brief Check memory corruption for all or specific cache
 * @param kmem_cache_addr Cache address (0 = all caches)
 */
void Slub::print_slub_poison(ulong kmem_cache_addr){
    LOGI("Starting memory corruption check %s", kmem_cache_addr ? " for specific cache" : " (all caches)");
    std::vector<SlubCheckResult> results;
    bool is_single_cache = (kmem_cache_addr != 0);

    PRINT("SLUB Memory Corruption Check\n");
    PRINT("===============================================================\n");

    size_t checked_caches = 0;
    for (const auto& cache_pair : cache_list) {
        const auto& cache_ptr = cache_pair.second;
        if (kmem_cache_addr != 0 && cache_ptr->addr != kmem_cache_addr) {
            continue;
        }
        checked_caches++;
        SlubCheckResult result;
        result.cache_name = cache_ptr->name;
        result.cache_addr = cache_ptr->addr;

        check_cache_corruption(cache_ptr, result);
        results.push_back(result);

        // For single cache, show all errors; for all caches, limit errors
        print_cache_check_result(result, is_single_cache);
    }

    // Only display summary for all caches option, not for single cache
    if (!is_single_cache) {
        print_corruption_summary(results);
    }
    LOGI("Corruption check complete: %zu cache(s) examined", checked_caches);
}

/**
 * @brief Check corruption for a specific cache
 * @param cache_ptr Cache to check
 * @param result Result structure to populate
 */
void Slub::check_cache_corruption(std::shared_ptr<kmem_cache> cache_ptr, SlubCheckResult& result) {
    // Check if corruption detection is supported
    if ((cache_ptr->flags & (SLAB_RED_ZONE | SLAB_POISON | SLAB_STORE_USER)) == 0) {
        LOGD(" Cache '%s': no debug flags, skipping", cache_ptr->name.c_str());
        result.errors.push_back("No corruption detection enabled (missing debug flags)");
        return;
    }

    // Traverse all slabs for checking
    for (const auto& node_ptr : cache_ptr->node_list) {
        check_slab_list_corruption(cache_ptr, node_ptr->partial, result);
        check_slab_list_corruption(cache_ptr, node_ptr->full, result);
    }
    for (const auto& cpu_ptr : cache_ptr->cpu_slabs) {
        check_slab_list_corruption(cache_ptr, cpu_ptr->partial, result);
        if (cpu_ptr->cur_slab) {
            check_single_slab_corruption(cache_ptr, cpu_ptr->cur_slab, result);
        }
    }

    result.overall_result = (result.corrupted_objects == 0);
    if (result.corrupted_objects > 0) {
        LOGE(" Cache '%s': %d corrupted object(s) - redzone:%d poison:%d freeptr:%d padding:%d",
             cache_ptr->name.c_str(), result.corrupted_objects,
             result.redzone_errors, result.poison_errors, result.freeptr_errors, result.padding_errors);
    } else {
        LOGD(" Cache '%s' is clean (%d objects checked)", cache_ptr->name.c_str(), result.checked_objects);
    }
}

/**
 * @brief Check corruption in a list of slabs
 * @param cache_ptr Parent cache
 * @param slab_list List of slabs to check
 * @param result Result structure to update
 */
void Slub::check_slab_list_corruption(std::shared_ptr<kmem_cache> cache_ptr,
                                     const std::vector<std::shared_ptr<slab>>& slab_list,
                                     SlubCheckResult& result) {
    for (const auto& slab_ptr : slab_list) {
        check_single_slab_corruption(cache_ptr, slab_ptr, result);
    }
}

/**
 * @brief Check corruption in a single slab
 * @param cache_ptr Parent cache
 * @param slab_ptr Slab to check
 * @param result Result structure to update
 */
void Slub::check_single_slab_corruption(std::shared_ptr<kmem_cache> cache_ptr,
                                       std::shared_ptr<slab> slab_ptr,
                                       SlubCheckResult& result) {
    if (!slab_ptr) return;

    for (const auto& obj : slab_ptr->obj_list) {
        result.total_objects++;
        result.checked_objects++;

        ObjectCheckResult obj_result;
        bool obj_ok = check_object_detailed(cache_ptr, slab_ptr->first_page,
                                           obj->start, obj->is_free ? SLUB_RED_INACTIVE : SLUB_RED_ACTIVE,
                                           obj_result);

        if (!obj_ok) {
            result.corrupted_objects++;
            result.overall_result = false;
            result.redzone_errors += obj_result.redzone_errors;
            result.poison_errors += obj_result.poison_errors;
            result.freeptr_errors += obj_result.freeptr_errors;
            result.padding_errors += obj_result.padding_errors;

            std::ostringstream oss;
            ulong obj_with_redzone = obj->start;
            ulong obj_data_start = fixup_red_left(cache_ptr, obj->start);
            oss << "Object corruption detected: ";
            oss << "Index: " << std::dec << obj->index;
            oss << " | Full Object: " << std::hex << obj_with_redzone;
            oss << " | Data Start: " << std::hex << obj_data_start;
            oss << " | Size: " << std::dec << cache_ptr->size << " bytes";

            result.errors.push_back(oss.str());
            // Add memory layout
            result.errors.push_back(print_object_layout(cache_ptr, obj, obj_result));
            // Add aligned error details
            for (const auto& detail : obj_result.details) {
                result.errors.push_back("        " + detail);
            }
        }
    }
}

/**
 * @brief Perform detailed corruption check on a single object
 * @param cache_ptr Parent cache
 * @param page_addr Page address
 * @param object_start_addr Object start address
 * @param val Expected poison value
 * @param obj_result Result structure to populate
 * @return True if object is clean, false if corrupted
 */
bool Slub::check_object_detailed(std::shared_ptr<kmem_cache> cache_ptr, ulong page_addr,
                                ulong object_start_addr, uint8_t val, ObjectCheckResult& obj_result) {
    ulong object = fixup_red_left(cache_ptr, object_start_addr);
    ulong p = object;
    ulong endobject = object + cache_ptr->object_size;
    bool ret = true;

    // Red Zone check
    if (cache_ptr->flags & SLAB_RED_ZONE) {
        // Left Red Zone check
        if (!check_bytes_and_report_detailed(cache_ptr, page_addr, object, "Left Redzone",
                                            object - cache_ptr->red_left_pad, val, cache_ptr->red_left_pad, obj_result)) {
            ret = false;
            obj_result.left_redzone_errors++;
            obj_result.redzone_errors++;
        }
        // Right Red Zone check
        if (!check_bytes_and_report_detailed(cache_ptr, page_addr, object, "Right Redzone",
                                            endobject, val, cache_ptr->inuse - cache_ptr->object_size, obj_result)) {
            ret = false;
            obj_result.right_redzone_errors++;
            obj_result.redzone_errors++;
        }
        // kmalloc Redzone check
        if(slub_debug_orig_size(cache_ptr) && val == SLUB_RED_ACTIVE){
            unsigned int orig_size_offset = get_orig_size(cache_ptr);
            unsigned int orig_size = read_int(object + orig_size_offset, "orig_size");
            if(cache_ptr->object_size > orig_size &&
                !check_bytes_and_report_detailed(cache_ptr, page_addr, object, "kmalloc Redzone",
                                                p + orig_size, val, cache_ptr->object_size - orig_size, obj_result)){
                ret = false;
                obj_result.redzone_errors++;
            }
        }
    } else {
        // Alignment padding check when no red zone
        if((cache_ptr->flags & SLAB_POISON) && (cache_ptr->object_size < cache_ptr->inuse)){
            if (!check_bytes_and_report_detailed(cache_ptr, page_addr, object, "Alignment padding",
                                                endobject, POISON_INUSE, cache_ptr->inuse - cache_ptr->object_size, obj_result)) {
                ret = false;
                obj_result.padding_errors++;
            }
        }
    }

    // Poison check
    if (cache_ptr->flags & SLAB_POISON) {
        if ((val != SLUB_RED_ACTIVE) && (cache_ptr->flags & OBJECT_POISON)) {
            // Main object poison check
            if (!check_bytes_and_report_detailed(cache_ptr, page_addr, object, "Object Poison",
                                                p, POISON_FREE, cache_ptr->object_size - 1, obj_result)) {
                ret = false;
                obj_result.poison_errors++;
            }
            // End poison byte check
            if (!check_bytes_and_report_detailed(cache_ptr, page_addr, object, "End Poison",
                                                p + cache_ptr->object_size - 1, POISON_END, 1, obj_result)) {
                ret = false;
                obj_result.poison_errors++;
            }
        }
        // Padding check
        if (!check_pad_bytes_detailed(cache_ptr, page_addr, p, obj_result)) {
            ret = false;
            obj_result.padding_errors++;
        }
    }

    // Free Pointer check
    if ((freeptr_outside_object(cache_ptr) || (val != SLUB_RED_ACTIVE)) &&
        !check_valid_pointer(cache_ptr, page_addr, get_free_pointer(cache_ptr, p))) {
        ret = false;
        obj_result.freeptr_errors++;

        // Get detailed free pointer information
        ulong freeptr = get_free_pointer(cache_ptr, p);
        ulong freeptr_addr = p + cache_ptr->offset;
        std::ostringstream oss;
        oss << "Free pointer corruption: addr=" << std::hex << freeptr_addr
            << ", value=" << std::hex << freeptr << " (invalid)";
        obj_result.details.push_back(oss.str());
    }
    return ret;
}

/**
 * @brief Check and report byte pattern violations
 * @param cache_ptr Parent cache
 * @param page_addr Page address
 * @param obj_start Object start address
 * @param what Description of the region being checked
 * @param start Start address of region to check
 * @param value Expected byte value
 * @param bytes Number of bytes to check
 * @param obj_result Result structure to update
 * @return True if region is clean, false if corrupted
 */
bool Slub::check_bytes_and_report_detailed(std::shared_ptr<kmem_cache> cache_ptr, ulong page_addr,
                                          ulong obj_start, std::string what, ulong start,
                                          uint8_t value, size_t bytes, ObjectCheckResult& obj_result) {
    ulong fault = memchr_inv(start, value, bytes);
    if (!is_kvaddr(fault)) {
        return true;
    }

    // Get the actual corrupted byte value
    uint8_t actual_byte = read_byte(fault, "check_bytes_and_report_detailed");
    // Calculate offset from slab start for better debugging
    ulong slab_vaddr = phy_to_virt(page_to_phy(page_addr));
    ulong offset_in_slab = fault - slab_vaddr;

    // Find the end of corruption
    ulong end = start + bytes;
    while (end > fault && read_byte(end - 1, "check_bytes_and_report_detailed") == value) {
        end -= 1;
    }

    std::ostringstream oss;
    oss << what << " corruption: " << std::hex << fault << "-" << (end - 1)
        << " @offset=" << offset_in_slab
        << ". Expected: 0x" << std::hex << (int)value
        << ", Found: 0x" << std::hex << (int)actual_byte;
    obj_result.details.push_back(oss.str());
    return false;
}

/**
 * @brief Check padding bytes for corruption
 * @param cache_ptr Parent cache
 * @param page_addr Page address
 * @param obj_start Object start address
 * @param obj_result Result structure to update
 * @return True if padding is clean, false if corrupted
 */
bool Slub::check_pad_bytes_detailed(std::shared_ptr<kmem_cache> cache_ptr, ulong page_addr,
                                   ulong obj_start, ObjectCheckResult& obj_result) {
    unsigned int off = get_info_end(cache_ptr);
    if (cache_ptr->flags & SLAB_STORE_USER) {
        off += 2 * struct_size(track);
        if(cache_ptr->flags & SLAB_KMALLOC){
            off += sizeof(unsigned int);
        }
    }
    if(size_from_object(cache_ptr) == off){
        return true;
    }
    return check_bytes_and_report_detailed(cache_ptr, page_addr, obj_start, "Object Padding",
                                          obj_start + off, POISON_INUSE,
                                          size_from_object(cache_ptr) - off, obj_result);
}

/**
 * @brief Generate memory layout visualization for a corrupted object
 * @param cache_ptr Parent cache
 * @param obj_ptr Object structure
 * @param obj_result Corruption check results
 * @return Formatted memory layout string
 */
std::string Slub::print_object_layout(std::shared_ptr<kmem_cache> cache_ptr,
                                      std::shared_ptr<obj> obj_ptr,
                                      const ObjectCheckResult& obj_result) {
    LOGD("     Generating memory layout for object #%d", obj_ptr->index);
    std::ostringstream oss;
    oss << "    Object Memory Layout:" << std::hex << "[" << obj_ptr->start << " - " << obj_ptr->start + cache_ptr->size << "]";

    if (cache_ptr->flags & SLAB_RED_ZONE) {
        // Left Red Zone
        oss << "\n            [" << std::hex << obj_ptr->start << "] Left Red Zone  ("
            << std::dec << cache_ptr->red_left_pad << " bytes)";
        if (obj_result.left_redzone_errors > 0) {
            oss << " <-- CORRUPTED";
        }

        // Object Data
        ulong data_start = fixup_red_left(cache_ptr, obj_ptr->start);
        oss << "\n            [" << std::hex << data_start << "] Object Data    ("
            << std::dec << cache_ptr->object_size << " bytes)";
        if (obj_result.poison_errors > 0) {
            oss << " <-- CORRUPTED";
        }

        // Free Pointer (if inside object)
        if (!freeptr_outside_object(cache_ptr)) {
            ulong freeptr_addr = data_start + cache_ptr->offset;
            oss << "\n            [" << std::hex << freeptr_addr << "] Free Pointer   ("
                << std::dec << sizeof(void*) << " bytes)";
            if (obj_result.freeptr_errors > 0) {
                oss << " <-- CORRUPTED";
            }
        }

        // Right Red Zone
        ulong right_redzone = data_start + cache_ptr->object_size;
        oss << "\n            [" << std::hex << right_redzone << "] Right Red Zone ("
            << std::dec << (cache_ptr->inuse - cache_ptr->object_size) << " bytes)";
        if (obj_result.right_redzone_errors > 0) {
            oss << " <-- CORRUPTED";
        }

        // Free Pointer (if outside object)
        if (freeptr_outside_object(cache_ptr)) {
            ulong freeptr_addr = data_start + cache_ptr->offset;
            oss << "\n            [" << std::hex << freeptr_addr << "] Free Pointer   ("
                << std::dec << sizeof(void*) << " bytes)";
            if (obj_result.freeptr_errors > 0) {
                oss << " <-- CORRUPTED";
            }
        }
    } else {
        // No Red Zone case
        oss << "\n            [" << std::hex << obj_ptr->start << "] Object Data    ("
            << std::dec << cache_ptr->object_size << " bytes)";
        if (obj_result.poison_errors > 0) {
            oss << " <-- CORRUPTED";
        }

        // Free Pointer
        ulong freeptr_addr = obj_ptr->start + cache_ptr->offset;
        oss << "\n            [" << std::hex << freeptr_addr << "] Free Pointer   ("
            << std::dec << sizeof(void*) << " bytes)";
        if (obj_result.freeptr_errors > 0) {
            oss << " <-- CORRUPTED";
        }
    }

    // Track Info
    if (cache_ptr->flags & SLAB_STORE_USER) {
        ulong track_start = obj_ptr->start + cache_ptr->red_left_pad + get_info_end(cache_ptr);
        oss << "\n            [" << std::hex << track_start << "] Track Info     ("
            << std::dec << (2 * struct_size(track)) << " bytes)";

        // Original size info for kmalloc caches
        if (slub_debug_orig_size(cache_ptr)) {
            ulong orig_size_addr = track_start + 2 * struct_size(track);
            oss << "\n            [" << std::hex << orig_size_addr << "] Original Size  ("
                << std::dec << sizeof(unsigned int) << " bytes)";
        }
    }

    // Padding Area
    if (obj_result.padding_errors > 0) {
        unsigned int off = get_info_end(cache_ptr);
        if (cache_ptr->flags & SLAB_STORE_USER) {
            off += 2 * struct_size(track);
            if (cache_ptr->flags & SLAB_KMALLOC) {
                off += sizeof(unsigned int);
            }
        }
        if (size_from_object(cache_ptr) > off) {
            ulong padding_start = obj_ptr->start + cache_ptr->red_left_pad + off;
            ulong padding_size = size_from_object(cache_ptr) - off;
            oss << "\n            [" << std::hex << padding_start << "] Padding Area   ("
                << std::dec << padding_size << " bytes) <-- CORRUPTED";
        }
    }
    return oss.str();
}

/**
 * @brief Print corruption check results for a cache
 * @param result Check results
 * @param show_all_errors Whether to show all errors or limit output
 */
void Slub::print_cache_check_result(const SlubCheckResult& result, bool show_all_errors) {
    LOGD(" Formatting results for cache '%s'", result.cache_name.c_str());
    PRINT("CACHE: %s (%#lx)\n", result.cache_name.c_str(), result.cache_addr);

    if (result.total_objects == 0) {
        PRINT("  No objects to check\n");
        PRINT("  Status: SKIPPED\n\n");
        return;
    }

    PRINT("  Objects: %d total, %d checked\n",
            result.total_objects, result.checked_objects);

    if (result.overall_result) {
        PRINT("  Result: CLEAN - No corruption detected\n");
    } else {
        PRINT("  Result: CORRUPTED - %d objects affected\n", result.corrupted_objects);
        if (result.redzone_errors > 0) {
            PRINT("    Red Zone violations: %d\n", result.redzone_errors);
        }
        if (result.poison_errors > 0) {
            PRINT("    Poison pattern errors: %d\n", result.poison_errors);
        }
        if (result.freeptr_errors > 0) {
            PRINT("    Free pointer corruptions: %d\n", result.freeptr_errors);
        }
        if (result.padding_errors > 0) {
            PRINT("    Padding violations: %d\n", result.padding_errors);
        }
    }

    if (!result.errors.empty()) {
        PRINT("  Error Details:\n");
        if (show_all_errors) {
            // Show all errors when using detailed analysis
            for (const auto& error : result.errors) {
                PRINT("    %s\n", error.c_str());
            }
        } else {
            // Show limited errors for summary view
            int shown = 0;
            for (const auto& error : result.errors) {
                if (shown >= 9) {
                    PRINT("    ... and %zu more errors\n", result.errors.size() - 9);
                    break;
                }
                PRINT("    %s\n", error.c_str());
                shown++;
            }
        }
    }
    PRINT("  Status: %s\n\n", result.overall_result ? "PASS" : "FAIL");
}

/**
 * @brief Print summary of corruption check results
 * @param results Vector of check results
 */
void Slub::print_corruption_summary(const std::vector<SlubCheckResult>& results) {
    LOGI("Generating corruption summary for %zu cache(s)", results.size());
    PRINT("CORRUPTION CHECK SUMMARY\n");
    PRINT("===============================================================\n");

    int total_caches = results.size();
    int clean_caches = 0;
    int corrupted_caches = 0;
    int total_corrupted_objects = 0;

    // Collect corrupted cache information
    std::vector<std::pair<std::string, ulong>> corrupted_cache_info;
    for (const auto& result : results) {
        if (result.overall_result) {
            clean_caches++;
        } else {
            corrupted_caches++;
            total_corrupted_objects += result.corrupted_objects;
            corrupted_cache_info.push_back({result.cache_name, result.cache_addr});
        }
    }

    if (corrupted_caches > 0) {
        LOGE("Corruption detected in %d caches with %d corrupted objects", corrupted_caches, total_corrupted_objects);
    } else {
        LOGI("All %d caches are clean", total_caches);
    }

    PRINT("Overall Status: %s\n",
            corrupted_caches == 0 ? "SYSTEM CLEAN" : "CORRUPTION DETECTED");
    PRINT("Statistics:\n");
    PRINT("  Caches: %d total (%d clean, %d corrupted)\n",
            total_caches, clean_caches, corrupted_caches);

    // Display corrupted cache details
    if (!corrupted_cache_info.empty()) {
        PRINT("  Corrupted Caches:\n");
        for (const auto& cache_info : corrupted_cache_info) {
            PRINT("    - %s (%#lx)\n", cache_info.first.c_str(), cache_info.second);
        }
    }
    PRINT("\n");

    if (corrupted_caches > 0) {
        PRINT("RECOMMENDATION: Investigate corrupted caches immediately!\n");
        PRINT("Use the following commands for detailed analysis:\n");
        for (const auto& cache_info : corrupted_cache_info) {
            PRINT("  slub -d -c %s  # %s\n", cache_info.first.c_str(), cache_info.first.c_str());
        }
    } else {
        PRINT("All SLUB caches are healthy - no memory corruption detected.\n");
    }
    PRINT("===============================================================\n");
    LOGD("Summary generation complete");
}

/* ============================================================================
 * Display and Analysis Functions
 * ============================================================================ */

/**
 * @brief Print detailed information for a single slab
 * @param slab_ptr Slab structure to display
 */
void Slub::print_slab_info(std::shared_ptr<slab> slab_ptr){
    LOGD("  Formatting slab %#lx: %u objects (%u used)",
         slab_ptr->first_page, slab_ptr->totalobj, slab_ptr->inuse);

    physaddr_t paddr = page_to_phy(slab_ptr->first_page);
    ulong slab_vaddr = phy_to_virt(paddr);

    PRINT("       slab:%#lx order:%d VA:[%#lx~%#lx] totalobj:%d inuse:%d freeobj:%d\n",
            slab_ptr->first_page,
            slab_ptr->order,
            slab_vaddr, (slab_vaddr + (power(2, slab_ptr->order) * page_size)),
            slab_ptr->totalobj,
            slab_ptr->inuse,
            slab_ptr->freeobj);

    std::ostringstream oss;
    for (const auto& obj_ptr : slab_ptr->obj_list) {
        oss << "           obj[" << std::setw(5) << std::setfill('0') << std::dec << obj_ptr->index << "]"
            << "VA:[0x" << std::hex << obj_ptr->start
            << "~0x" << std::hex << obj_ptr->end << "]"
            << " status:" << (obj_ptr->is_free ? "freed":"alloc")
            << "\n";
    }
    PRINT("%s \n", oss.str().c_str());
}

/**
 * @brief Print information for all caches
 */
void Slub::print_kmem_caches(){
    for (const auto& cache_pair : cache_list) {
        print_kmem_cache(cache_pair.second);
    }
}

/**
 * @brief Print information for a specific cache
 * @param cache_ptr Cache structure to display
 */
void Slub::print_kmem_cache(std::shared_ptr<kmem_cache> cache_ptr){
    PRINT("kmem_cache:%#lx %s\n",cache_ptr->addr,cache_ptr->name.c_str());

    for (const auto& node_ptr : cache_ptr->node_list) {
        PRINT("   kmem_cache_node:%#lx nr_partial:%ld nr_slabs:%ld total_objects:%ld\n",
                node_ptr->addr,node_ptr->nr_partial,node_ptr->nr_slabs,node_ptr->total_objects);
        for (const auto& slab_ptr : node_ptr->partial) {
            print_slab_info(slab_ptr);
        }
        for (const auto& slab_ptr : node_ptr->full) {
            print_slab_info(slab_ptr);
        }
    }

    for (size_t i = 0; i < cache_ptr->cpu_slabs.size(); i++){
        std::shared_ptr<kmem_cache_cpu> cpu_ptr = cache_ptr->cpu_slabs[i];
        PRINT("   kmem_cache_cpu[%zu]:%#lx\n",i,cpu_ptr->addr);
        for (const auto& slab_ptr : cpu_ptr->partial) {
            print_slab_info(slab_ptr);
        }
        if (cpu_ptr->cur_slab != nullptr){
            print_slab_info(cpu_ptr->cur_slab);
        }
    }
    PRINT("\n");
}

/**
 * @brief Print information for a specific cache by name
 * @param Cache name
 */
void Slub::print_kmem_cache(std::string& name){
    LOGI("Looking up cache: %s", name.c_str());
    // First try to find in existing cache_list
    auto it = cache_list.find(name);
    std::shared_ptr<kmem_cache> target_cache;
    if (it == cache_list.end()){
        target_cache = parse_kmem_cache(name);
    }else{
        target_cache = it->second;
        LOGD(" Found cache '%s' in existing list", target_cache->name.c_str());
    }
    if (target_cache) {
        print_kmem_cache(target_cache);
    } else {
        LOGE("Cache not found: %s", name.c_str());
    }
}

/**
 * @brief Print summary information for all caches
 */
void Slub::print_slab_summary_info(){
    // Convert unordered_map to vector for sorting
    std::vector<std::shared_ptr<kmem_cache>> cache_vector;
    cache_vector.reserve(cache_list.size());
    for (const auto& cache_pair : cache_list) {
        cache_vector.push_back(cache_pair.second);
    }

    std::sort(cache_vector.begin(), cache_vector.end(),[&](const std::shared_ptr<kmem_cache>& a, const std::shared_ptr<kmem_cache>& b){
        return a->total_size > b->total_size;
    });

    // Calculate dynamic column widths for better alignment
    size_t max_cache_name_len = 15; // minimum width for cache name
    for (const auto& cache_ptr : cache_vector) {
        max_cache_name_len = std::max(max_cache_name_len, cache_ptr->name.length());
    }

    // Define column widths for better alignment
    const int addr_width = 16;           // kmem_cache address
    const int name_width = std::min(max_cache_name_len, static_cast<size_t>(25)); // cache name (max 25)
    const int slabs_width = 6;           // slabs count
    const int slab_size_width = 10;      // slab size
    const int per_obj_width = 8;         // per_slab_obj
    const int total_objs_width = 10;     // total_objs
    const int obj_size_width = 10;       // obj_size
    const int pad_size_width = 8;        // pad_size
    const int align_size_width = 10;     // align_size
    const int total_size_width = 12;     // total_size

    std::ostringstream oss;

    // Print header with proper alignment and spacing
    oss << std::left << std::setw(addr_width) << "kmem_cache" << " "
        << std::left << std::setw(name_width) << "name" << " "
        << std::right << std::setw(slabs_width) << "slabs" << " "
        << std::right << std::setw(slab_size_width) << "slab_size" << " "
        << std::right << std::setw(per_obj_width) << "per_obj" << " "
        << std::right << std::setw(total_objs_width) << "total_objs" << " "
        << std::right << std::setw(obj_size_width) << "obj_size" << " "
        << std::right << std::setw(pad_size_width) << "pad_size" << " "
        << std::right << std::setw(align_size_width) << "align_size" << " "
        << std::right << std::setw(total_size_width) << "total_size"
        << "\n";

    // Print separator line (account for spaces between columns)
    oss << std::string(addr_width + name_width + slabs_width + slab_size_width +
                      per_obj_width + total_objs_width + obj_size_width +
                      pad_size_width + align_size_width + total_size_width + 9, '-') << "\n";

    // Print data rows with proper alignment
    for (const auto& cache_ptr : cache_vector) {
        int page_cnt = 1U << cache_ptr->page_order;

        // Truncate long cache names if necessary
        std::string display_name = cache_ptr->name;
        if (display_name.length() > static_cast<size_t>(name_width - 1)) {
            display_name = display_name.substr(0, name_width - 4) + "...";
        }

        oss << std::left << std::setw(addr_width) << std::hex << cache_ptr->addr << " "
            << std::left << std::setw(name_width) << display_name << " "
            << std::right << std::setw(slabs_width) << std::dec << cache_ptr->total_nr_slabs << " "
            << std::right << std::setw(slab_size_width) << csize(page_cnt * page_size) << " "
            << std::right << std::setw(per_obj_width) << std::dec << cache_ptr->per_slab_obj << " "
            << std::right << std::setw(total_objs_width) << std::dec << cache_ptr->total_nr_objs << " "
            << std::right << std::setw(obj_size_width) << csize(cache_ptr->object_size) << " "
            << std::right << std::setw(pad_size_width) << std::dec << cache_ptr->red_left_pad << " "
            << std::right << std::setw(align_size_width) << csize(cache_ptr->size) << " "
            << std::right << std::setw(total_size_width) << csize(cache_ptr->total_size)
            << "\n";
    }

    PRINT("%s", oss.str().c_str());
}

/* ============================================================================
 * Object Detail Printing Functions
 * ============================================================================ */

/**
 * @brief Print single object detail with trace information
 *
 * This function prints detailed information for a single object including:
 * - Object status (ALLOC/FREE)
 * - Address range and size
 * - Slab and cache information
 * - Process and timing information
 * - Call stack trace (if available)
 *
 * @param obj_ptr Object to print details for
 */
void Slub::print_single_object_detail(std::shared_ptr<obj> obj_ptr) {
    if (!obj_ptr || !obj_ptr->cache_ptr || !obj_ptr->slab_ptr) {
        LOGW("Invalid object pointer provided to print_single_object_detail");
        return;
    }

    // Determine object status and get appropriate trace information
    std::string status = obj_ptr->is_free ? "FREE" : "ALLOC";

    // Calculate object size for display
    size_t obj_size = obj_ptr->end - obj_ptr->start;

    // Print basic object information line
    PRINT("%s [%#lx~%#lx](%s) slab:%#lx kmem_cache:%#lx %s",
          status.c_str(),
          obj_ptr->start,
          obj_ptr->end,
          csize(obj_size).c_str(),
          obj_ptr->slab_ptr->first_page,
          obj_ptr->cache_ptr->addr,
          obj_ptr->cache_ptr->name.c_str());

    // Check if trace information is available
    if ((obj_ptr->cache_ptr->flags & SLAB_STORE_USER) == 0) {
        PRINT(" (no trace available)\n");
        return;
    }

    // Create and populate track structure for trace extraction
    auto track_ptr = obj_ptr->track_ptr;
    if (!track_ptr) {
        LOGW("Invalid track pointer(no trace available)");
        return;
    }
    // Add process and timing information to the basic line
    PRINT(" pid:%d cpu:%d timestamp:(%lu) %s\n",
          track_ptr->pid,
          track_ptr->cpu,
          track_ptr->when,
          formatTimestamp(track_ptr->when).c_str());

    // Print call stack if available
    if (!track_ptr->frame.empty()) {
        // Parse and print each line of the call stack
        std::istringstream iss(track_ptr->frame);
        std::string line;
        while (std::getline(iss, line)) {
            if (!line.empty()) {
                // Remove leading whitespace and print the stack frame
                size_t start = line.find_first_not_of(" \t");
                if (start != std::string::npos) {
                    line = line.substr(start);
                    // Format as [<address>] function+offset
                    PRINT("%s\n", line.c_str());
                }
            }
        }
    }
    PRINT("\n");
}

/**
 * @brief Print all objects information with detailed trace
 *
 * This function iterates through all caches (or a specific cache if filtered)
 * and prints detailed information for every object, including trace information.
 *
 * @param cache_name Cache name or address filter (empty = all caches)
 */
void Slub::print_all_objects_info(const std::string& cache_name) {
    LOGI("Printing all objects information%s",
         cache_name.empty() ? " (all caches)" : (" for cache: " + cache_name).c_str());

    size_t total_objects_printed = 0;
    size_t caches_processed = 0;

    // Iterate through all caches or find specific cache
    for (const auto& cache_pair : cache_list) {
        const auto& cache_ptr = cache_pair.second;

        // Apply cache filter if specified
        if (!cache_name.empty() && cache_ptr->name != cache_name) {
            continue;
        }

        caches_processed++;
        LOGD("Processing cache '%s' with %zu total objects",
             cache_ptr->name.c_str(), cache_ptr->total_nr_objs);

        // Process all slabs in NUMA nodes
        for (const auto& node_ptr : cache_ptr->node_list) {
            // Process partial slabs
            for (const auto& slab_ptr : node_ptr->partial) {
                for (const auto& obj_ptr : slab_ptr->obj_list) {
                    print_single_object_detail(obj_ptr);
                    total_objects_printed++;
                }
            }

            // Process full slabs
            for (const auto& slab_ptr : node_ptr->full) {
                for (const auto& obj_ptr : slab_ptr->obj_list) {
                    print_single_object_detail(obj_ptr);
                    total_objects_printed++;
                }
            }
        }

        // Process per-CPU slabs
        for (const auto& cpu_ptr : cache_ptr->cpu_slabs) {
            // Process per-CPU partial slabs
            for (const auto& slab_ptr : cpu_ptr->partial) {
                for (const auto& obj_ptr : slab_ptr->obj_list) {
                    print_single_object_detail(obj_ptr);
                    total_objects_printed++;
                }
            }

            // Process current slab
            if (cpu_ptr->cur_slab) {
                for (const auto& obj_ptr : cpu_ptr->cur_slab->obj_list) {
                    print_single_object_detail(obj_ptr);
                    total_objects_printed++;
                }
            }
        }
    }

    LOGI("Completed printing %zu objects from %zu cache(s)",
         total_objects_printed, caches_processed);
}

/* ============================================================================
 * Object Search and Analysis Functions
 * ============================================================================ */

/**
 * @brief Find object by virtual address and print detailed trace
 * @param addr_str Address string to analyze
 */
void Slub::print_object_trace(std::string addr_str) {
    LOGI("Locating object containing address: %s", addr_str.c_str());
    ulong target_addr;
    try {
        target_addr = std::stoul(addr_str, nullptr, 16);
        LOGD(" Target address: %#lx", target_addr);
    } catch (const std::exception& e) {
        LOGE("Invalid address format: %s (exception: %s)", addr_str.c_str(), e.what());
        return;
    }

    if (!is_kvaddr(target_addr)) {
        LOGE("Invalid kernel virtual address: %#lx", target_addr);
        return;
    }

    LOGD(" Scanning all caches for address match");
    std::shared_ptr<obj> found_obj = find_object(target_addr);
    if (!found_obj) {
        PRINT("Address %#lx not found in any SLUB object\n", target_addr);
        return;
    }

    LOGI(" Object found in cache '%s' - index:%d status:%s",
         found_obj->cache_ptr->name.c_str(), found_obj->index, found_obj->is_free ? "freed" : "allocated");

    ulong offset_in_obj = target_addr - found_obj->start;

    PRINT("================================================================================\n");
    PRINT("SLUB Object Analysis for Address: %#lx\n", target_addr);
    PRINT("================================================================================\n");

    PRINT("KMEM_CACHE:\n");
    PRINT("   Address     : %#lx\n", found_obj->cache_ptr->addr);
    PRINT("   Name        : %s\n", found_obj->cache_ptr->name.c_str());
    PRINT("   Object Size : %u bytes\n", found_obj->cache_ptr->object_size);
    PRINT("   Total Size  : %u bytes\n", found_obj->cache_ptr->size);
    PRINT("   Flags       : %#x%s\n", found_obj->cache_ptr->flags,
            (found_obj->cache_ptr->flags & SLAB_STORE_USER) ? " (STORE_USER enabled)" : " (no trace)");

    PRINT("\n");

    ulong slab_start = phy_to_virt(page_to_phy(found_obj->slab_ptr->first_page));
    ulong slab_end = slab_start + (power(2, found_obj->slab_ptr->order) * page_size);
    PRINT("SLAB:\n");
    PRINT("   Page Address: %#lx\n", found_obj->slab_ptr->first_page);
    PRINT("   Order       : %u (%s)\n", found_obj->slab_ptr->order, csize(power(2, found_obj->slab_ptr->order) * page_size).c_str());
    PRINT("   VA Range    : [%#lx ~ %#lx]\n", slab_start, slab_end);
    PRINT("   Total Objs  : %u\n", found_obj->slab_ptr->totalobj);
    PRINT("   In Use      : %u\n", found_obj->slab_ptr->inuse);
    PRINT("   Free        : %u\n", found_obj->slab_ptr->freeobj);

    PRINT("\n");

    PRINT("OBJECT:\n");
    PRINT("   Index       : %d\n", found_obj->index);
    PRINT("   VA Range    : [%#lx ~ %#lx] (%s)\n",
            found_obj->start, found_obj->end, csize(found_obj->end - found_obj->start).c_str());
    PRINT("   Status      : %s\n", found_obj->is_free ? "FREED" : "ALLOCATED");
    PRINT("   Target Addr : %#lx\n", target_addr);
    PRINT("   Offset      : +%#lx (%lu bytes from object start)\n", offset_in_obj, offset_in_obj);

    PRINT("\n");

    print_object_stack_trace(found_obj);

    PRINT("================================================================================\n");
    LOGD("Object analysis complete");
}

/**
 * @brief Print stack trace for a specific object
 * @param obj_ptr Object to analyze
 */
void Slub::print_object_stack_trace(std::shared_ptr<obj> obj_ptr) {
    LOGD(" Extracting stack trace for object #%d", obj_ptr->index);
    if ((obj_ptr->cache_ptr->flags & SLAB_STORE_USER) == 0) {
        PRINT("STACK TRACE:\n");
        PRINT("   Not available (SLAB_STORE_USER not enabled)\n");
        return;
    }

    auto track_ptr = std::make_shared<track>();
    track_ptr->kmem_cache_ptr = obj_ptr->cache_ptr;
    track_ptr->obj_addr = obj_ptr->start;

    uint8_t track_type = obj_ptr->is_free ? TRACK_FREE : TRACK_ALLOC;
    track_ptr->track_addr = get_obj_track_addr(obj_ptr->cache_ptr, obj_ptr->start, track_type);
    parser_obj_trace(track_ptr->track_addr, track_ptr);

    PRINT("STACK TRACE:\n");
    if (!track_ptr->frame.empty()) {
        PRINT("   Type        : %s\n", obj_ptr->is_free ? "FREE" : "ALLOC");
        PRINT("   PID         : %d\n", track_ptr->pid);
        PRINT("   CPU         : %d\n", track_ptr->cpu);
        PRINT("   Timestamp   : %lu %s\n", track_ptr->when, formatTimestamp(track_ptr->when).c_str());
        PRINT("   Call Stack  :\n");
        std::istringstream iss(track_ptr->frame);
        std::string line;
        while (std::getline(iss, line)) {
            if (!line.empty()) {
                size_t start = line.find_first_not_of(" \t");
                if (start != std::string::npos) {
                    line = line.substr(start);
                }
                PRINT("      %s\n", line.c_str());
            }
        }
    } else {
        PRINT("   No stack trace available\n");
    }
}

/**
 * @brief Find object containing a specific virtual address
 * @param target_addr Virtual address to search for
 * @return Object structure or nullptr if not found
 */
std::shared_ptr<obj> Slub::find_object(ulong target_addr) {
    LOGD(" Searching for object at %#lx", target_addr);
    for (const auto& cache_pair : cache_list) {
        const auto& cache_ptr = cache_pair.second;
        for (const auto& node_ptr : cache_ptr->node_list) {
            for (const auto& slab_ptr : node_ptr->partial) {
                auto obj = find_object_in_slab(slab_ptr, target_addr);
                if (obj) {
                    return obj;
                }
            }
            for (const auto& slab_ptr : node_ptr->full) {
                auto obj = find_object_in_slab(slab_ptr, target_addr);
                if (obj) {
                    return obj;
                }
            }
        }
        for (const auto& cpu_ptr : cache_ptr->cpu_slabs) {
            for (const auto& slab_ptr : cpu_ptr->partial) {
                auto obj = find_object_in_slab(slab_ptr, target_addr);
                if (obj) {
                    return obj;
                }
            }
            if (cpu_ptr->cur_slab) {
                auto obj = find_object_in_slab(cpu_ptr->cur_slab, target_addr);
                if (obj) {
                    return obj;
                }
            }
        }
    }
    LOGD(" Object search exhausted - not found");
    return nullptr;
}

/**
 * @brief Find object within a specific slab
 * @param slab_ptr Slab to search
 * @param target_addr Virtual address to search for
 * @return Object structure or nullptr if not found
 */
std::shared_ptr<obj> Slub::find_object_in_slab(std::shared_ptr<slab> slab_ptr, ulong target_addr) {
    LOGD("  Checking slab for address %#lx", target_addr);
    if (!slab_ptr) {
        LOGD("   Null slab, skipping");
        return nullptr;
    }

    ulong slab_start = slab_ptr->obj_list.empty() ? 0 : slab_ptr->obj_list.front()->start;
    ulong slab_end = slab_ptr->obj_list.empty() ? 0 : slab_ptr->obj_list.back()->end;

    // Quick range check before iterating
    if (target_addr < slab_start || target_addr >= slab_end) {
        LOGD("   Address outside slab range [%#lx-%#lx]", slab_start, slab_end);
        return nullptr;
    }

    for (const auto& obj_ptr : slab_ptr->obj_list) {
        if (target_addr >= obj_ptr->start && target_addr < obj_ptr->end) {
            LOGD("   Match: object #%d [%#lx-%#lx]", obj_ptr->index, obj_ptr->start, obj_ptr->end);
            return obj_ptr;
        }
    }
    LOGD("   No match in this slab");
    return nullptr;
}

#pragma GCC diagnostic pop
