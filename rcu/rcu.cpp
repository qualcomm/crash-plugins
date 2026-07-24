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

#include "rcu.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(Rcu)
#endif

/**
 * @brief Main command entry point for RCU analysis
 *
 * Parses command line arguments and dispatches to appropriate handler functions:
 * -a: Display all RCU data with callback statistics
 * -c <cpu>: Show detailed information for specific CPU
 * -s: Show summary statistics
 */
void Rcu::cmd_main(void) {
    // Check minimum argument count
    if (argcnt < 2) {
        LOGD("Insufficient arguments provided, showing usage\n");
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }

    int argerrs = 0;
    int c;
    bool show_all = false;
    int target_cpu = -1;

    // Parse command line options
    while ((c = getopt(argcnt, args, "ac:")) != EOF) {
        switch(c) {
            case 'a':
                LOGD("Executing analyze_rcu_data() - display all RCU data\n");
                show_all = true;
                break;
            case 'c':
                target_cpu = std::atoi(optarg);
                LOGD("Executing print_cpu_rcu_details() for CPU %d\n", target_cpu);
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

    // Parse RCU data if not already done
    if (rcu_data_list.empty()) {
        LOGD("RCU data list is empty, analyzing RCU data from kernel\n");
        analyze_rcu_data();
    } else {
        LOGD("Using cached RCU data list with %zu entries\n", rcu_data_list.size());
    }

    // Execute requested operations
    if (show_all) {
        print_rcu_data_details();
    }

    if (target_cpu >= 0) {
        print_cpu_rcu_details(target_cpu);
    }
}

/**
 * @brief Initialize kernel structure field offsets
 *
 * Sets up field offsets for RCU structure fields used in RCU analysis.
 * These offsets are essential for reading RCU data from kernel memory.
 */
void Rcu::init_offset(void) {
    // Initialize RCU data structure field offsets
    struct_init(rcu_data);
    field_init(rcu_data, cblist);
    field_init(rcu_data, gp_seq);
    field_init(rcu_data, gp_seq_needed);
    field_init(rcu_data, core_needs_qs);
    field_init(rcu_data, beenonline);

    // Initialize RCU segmented callback list structure field offsets
    struct_init(rcu_segcblist);
    field_init(rcu_segcblist, head);
    field_init(rcu_segcblist, tails);
    field_init(rcu_segcblist, gp_seq);
    field_init(rcu_segcblist, len);
    field_init(rcu_segcblist, seglen);

    // Initialize callback head structure field offsets
    struct_init(callback_head);
    field_init(callback_head, next);
    field_init(callback_head, func);
}

/**
 * @brief Initialize command help and usage information
 *
 * Sets up the command name, description, and detailed help text including
 * usage examples and expected output formats for the RCU plugin.
 */
void Rcu::init_command(void) {
    cmd_name = "rcu";
    help_str_list = {
        "rcu",                                /* command name */
        "display RCU (Read-Copy-Update) information",  /* short description */
        "[-a] [-c <cpu_id>]\n"
        "  This command displays RCU subsystem information.\n"
        "\n"
        "    -a              display all RCU data with detailed callback information\n"
        "    -c <cpu_id>     display detailed information for specific CPU\n",
        "\n",
        "EXAMPLES",
        "  Display all RCU data with callback details:",
        "    %s> rcu -a",
        "    CPU[0] rcu_data at: 0xffffffc010b2c000",
        "      gp_seq: 12345, gp_seq_needed: 12346",
        "      core_needs_qs: false, beenonline: true",
        "      cblist address: 0xffffffc010b2c080",
        "        rcu_segcblist analysis:",
        "          head: 0xffffffc012345678",
        "          len: 25",
        "            [1] callback_head at 0xffffffc012345678: func:kfree_rcu_work",
        "            [2] callback_head at 0xffffffc012345690: func:call_rcu_tasks",
        "\n",
        "  Display detailed information for specific CPU:",
        "    %s> rcu -c 0",
        "    CPU[0] RCU Data Details:",
        "    ================================================================================",
        "    RCU Data Address: 0xffffffc010b2c000",
        "    Grace Period Seq: 12345 (needed: 12346)",
        "    Core Needs QS: false",
        "    Been Online: true",
        "    Callback List: 25 callbacks",
        "\n",
    };
}

/**
 * @brief Default constructor
 *
 * Initializes the RCU plugin with default settings.
 */
Rcu::Rcu() {
    // Initialize data structures
    rcu_data_list.clear();
    all_callbacks.clear();
}

/**
 * @brief Analyze RCU data structures from kernel memory
 *
 * This is the main analysis function that replicates the functionality
 * from slub.cpp's test() function. It reads the kernel's per-CPU RCU data
 * and extracts detailed information about RCU callbacks and state.
 */
void Rcu::analyze_rcu_data() {
    PRINT("================================================================================\n");
    PRINT("                        RCU ANALYSIS\n");
    PRINT("================================================================================\n");

    // Get the base address of the per-CPU rcu_data variable
    ulong rcu_data = csymbol_value("rcu_data");
    if (!rcu_data) {
        PRINT("Error: Cannot find rcu_data symbol\n");
        return;
    }

    PRINT("Base rcu_data address: %#lx\n", rcu_data);
    PRINT("================================================================================\n");

    size_t active_cpus = 0;
    size_t total_callbacks = 0;

    // Iterate through all possible CPUs in the system
    std::vector<ulong> percpu_list = for_each_percpu(rcu_data);
    for (size_t i = 0; i < percpu_list.size(); i++){
        ulong per_rcu_data = percpu_list[i];
        if (!is_kvaddr(per_rcu_data)) {
            continue;
        }

        active_cpus++;
        PRINT("CPU[%zu] rcu_data at: %#lx", i, per_rcu_data);

        // Create RCU data info structure
        auto rcu_data_ptr = std::make_shared<rcu_data_info>();
        rcu_data_ptr->rcu_data_addr = per_rcu_data;
        rcu_data_ptr->cpu_id = i;

        // Parse struct rcu_data
        void* rcu_data_buf = read_struct(per_rcu_data, "rcu_data");
        if (!rcu_data_buf) {
            PRINT("  Error: Failed to read rcu_data structure\n");
            continue;
        }

        // Extract basic rcu_data fields
        rcu_data_ptr->gp_seq = ULONG(rcu_data_buf + field_offset(rcu_data, gp_seq));
        rcu_data_ptr->gp_seq_needed = ULONG(rcu_data_buf + field_offset(rcu_data, gp_seq_needed));
        rcu_data_ptr->core_needs_qs = read_bool(per_rcu_data + field_offset(rcu_data, core_needs_qs), "core_needs_qs");
        rcu_data_ptr->beenonline = read_bool(per_rcu_data + field_offset(rcu_data, beenonline), "beenonline");

        PRINT("  gp_seq:%lu, gp_seq_needed:%lu", rcu_data_ptr->gp_seq, rcu_data_ptr->gp_seq_needed);
        PRINT("  core_needs_qs:%s, beenonline:%s\n",
              rcu_data_ptr->core_needs_qs ? "true" : "false",
              rcu_data_ptr->beenonline ? "true" : "false");

        // Get the address of the cblist (struct rcu_segcblist)
        ulong cblist_addr = per_rcu_data + field_offset(rcu_data, cblist);

        PRINT("  cblist address: %#lx\n", cblist_addr);

        // Parse struct rcu_segcblist
        parse_rcu_segcblist(cblist_addr, i, total_callbacks);

        FREEBUF(rcu_data_buf);

        // Add to our data list
        rcu_data_list.push_back(rcu_data_ptr);

        PRINT("\n");
    }

    PRINT("================================================================================\n");
    PRINT("SUMMARY: Processed %zu active CPU(s), found %zu total callbacks\n",
          active_cpus, total_callbacks);
    PRINT("================================================================================\n");
}

/**
 * @brief Parse struct rcu_segcblist and extract callback information
 * @param cblist_addr Address of the rcu_segcblist structure
 * @param cpu_id CPU ID for display purposes
 * @param total_callbacks Reference to total callback counter
 */
void Rcu::parse_rcu_segcblist(ulong cblist_addr, size_t cpu_id, size_t& total_callbacks) {
    if (!is_kvaddr(cblist_addr)) {
        PRINT("    Invalid cblist address: %#lx\n", cblist_addr);
        return;
    }

    // Create segcblist info structure
    auto cblist_info = std::make_shared<rcu_segcblist_info>();
    cblist_info->cblist_addr = cblist_addr;
    cblist_info->cpu_id = cpu_id;

    // Read the rcu_segcblist structure
    void* cblist_buf = read_struct(cblist_addr, "rcu_segcblist");
    if (!cblist_buf) {
        PRINT("    Error: Failed to read rcu_segcblist structure\n");
        return;
    }

    // Extract rcu_segcblist fields
    cblist_info->head_addr = ULONG(cblist_buf + field_offset(rcu_segcblist, head));
    ulong tails_addr = cblist_addr + field_offset(rcu_segcblist, tails);
    ulong gp_seq_addr = cblist_addr + field_offset(rcu_segcblist, gp_seq);
    cblist_info->len = read_long(cblist_addr + field_offset(rcu_segcblist, len), "rcu_segcblist.len");
    ulong seglen_addr = cblist_addr + field_offset(rcu_segcblist, seglen);

    PRINT("    rcu_segcblist:%#lx\n", cblist_info->head_addr);
    PRINT("      len: %ld\n", cblist_info->len);

    // Parse tails array (4 elements)
    PRINT("      tails[4]: ");
    for (int i = 0; i < 4; i++) {
        cblist_info->tails[i] = read_pointer(tails_addr + i * sizeof(void*), "tails");
        PRINT("[%d]=%#lx ", i, cblist_info->tails[i]);
    }
    PRINT("\n");

    // Parse gp_seq array (4 elements)
    PRINT("      gp_seq[4]: ");
    for (int i = 0; i < 4; i++) {
        cblist_info->gp_seq[i] = read_ulong(gp_seq_addr + i * sizeof(unsigned long), "gp_seq");
        PRINT("[%d]=%lu ", i, cblist_info->gp_seq[i]);
    }
    PRINT("\n");

    // Parse seglen array (4 elements)
    PRINT("      seglen[4]: ");
    for (int i = 0; i < 4; i++) {
        cblist_info->seglen[i] = read_long(seglen_addr + i * sizeof(long), "seglen");
        PRINT("[%d]=%ld ", i, cblist_info->seglen[i]);
    }
    PRINT("\n");

    FREEBUF(cblist_buf);

    // Traverse the callback list starting from head
    if (is_kvaddr(cblist_info->head_addr) && cblist_info->len > 0) {
        parse_callback_chain(cblist_info->head_addr, cpu_id, total_callbacks);
    } else {
        PRINT("      No callbacks in chain (head=%#lx, len=%ld)\n", cblist_info->head_addr, cblist_info->len);
    }
}

/**
 * @brief Parse callback chain and extract all func pointers
 * @param head_addr Address of the first callback_head in the chain
 * @param cpu_id CPU ID for display purposes
 * @param total_callbacks Reference to total callback counter
 */
void Rcu::parse_callback_chain(ulong head_addr, size_t cpu_id, size_t& total_callbacks) {
    std::unordered_set<ulong> visited_callbacks; // Prevent infinite loops
    ulong current = head_addr;
    size_t callback_count = 0;
    const size_t MAX_CALLBACKS = 10000; // Safety limit
    while (is_kvaddr(current) && callback_count < MAX_CALLBACKS) {
        // Check for circular references
        if (visited_callbacks.find(current) != visited_callbacks.end()) {
            PRINT("        Warning: Circular reference detected at %#lx\n", current);
            break;
        }
        visited_callbacks.insert(current);

        // Read the callback_head structure
        void* callback_buf = read_struct(current, "callback_head");
        if (!callback_buf) {
            PRINT("        Error: Failed to read callback_head at %#lx\n", current);
            break;
        }

        // Extract callback_head fields
        ulong next = ULONG(callback_buf + field_offset(callback_head, next));
        ulong func = ULONG(callback_buf + field_offset(callback_head, func));

        callback_count++;
        total_callbacks++;

        // Create callback info structure
        auto callback_info = std::make_shared<rcu_callback_info>();
        callback_info->callback_addr = current;
        callback_info->next_addr = next;
        callback_info->func_addr = func;
        callback_info->cpu_id = cpu_id;
        callback_info->func_name = to_symbol(func);

        // Add to global callback list
        all_callbacks.push_back(callback_info);

        // Print callback information with symbol resolution
        PRINT("        [%zu] callback_head at %#lx: func:%s\n",
              callback_count, current, callback_info->func_name.c_str());

        FREEBUF(callback_buf);

        // Move to next callback
        current = next;

        // Break if we've reached the end of the list
        if (current == 0 || current == head_addr) {
            break;
        }
    }
}

/**
 * @brief Print detailed RCU data for all CPUs
 */
void Rcu::print_rcu_data_details() {
    if (rcu_data_list.empty()) {
        analyze_rcu_data();
        return;
    }

    PRINT("================================================================================\n");
    PRINT("                        DETAILED RCU DATA ANALYSIS\n");
    PRINT("================================================================================\n");

    for (const auto& rcu_data : rcu_data_list) {
        PRINT("CPU[%zu] RCU Data at %#lx:\n", rcu_data->cpu_id, rcu_data->rcu_data_addr);
        PRINT("  Grace Period Seq: %lu (needed: %lu)\n", rcu_data->gp_seq, rcu_data->gp_seq_needed);
        PRINT("  Core Needs QS: %s\n", rcu_data->core_needs_qs ? "true" : "false");
        PRINT("  Been Online: %s\n", rcu_data->beenonline ? "true" : "false");

        // Count callbacks for this CPU
        size_t cpu_callbacks = 0;
        for (const auto& callback : all_callbacks) {
            if (callback->cpu_id == rcu_data->cpu_id) {
                cpu_callbacks++;
            }
        }
        PRINT("  Callbacks: %zu\n", cpu_callbacks);
        PRINT("\n");
    }
}

/**
 * @brief Print callback statistics
 */
void Rcu::print_callback_statistics() {
    if (all_callbacks.empty()) {
        PRINT("No callback data available\n");
        return;
    }

    PRINT("================================================================================\n");
    PRINT("                        RCU CALLBACK STATISTICS\n");
    PRINT("================================================================================\n");

    // Group callbacks by CPU
    std::map<size_t, std::vector<std::shared_ptr<rcu_callback_info>>> cpu_callbacks;
    for (const auto& callback : all_callbacks) {
        cpu_callbacks[callback->cpu_id].push_back(callback);
    }

    for (const auto& cpu_pair : cpu_callbacks) {
        size_t cpu_id = cpu_pair.first;
        const auto& callbacks = cpu_pair.second;

        PRINT("CPU[%zu] - %zu callbacks:\n", cpu_id, callbacks.size());

        for (size_t i = 0; i < callbacks.size() && i < 10; i++) {
            const auto& callback = callbacks[i];
            PRINT("  [%zu] %#lx -> %s\n", i + 1, callback->callback_addr, callback->func_name.c_str());
        }

        if (callbacks.size() > 10) {
            PRINT("  ... and %zu more callbacks\n", callbacks.size() - 10);
        }
        PRINT("\n");
    }
}

/**
 * @brief Print detailed information for specific CPU
 * @param cpu_id CPU ID to analyze
 */
void Rcu::print_cpu_rcu_details(size_t cpu_id) {
    if (rcu_data_list.empty()) {
        analyze_rcu_data();
    }

    // Find the specific CPU data
    std::shared_ptr<rcu_data_info> target_cpu = nullptr;
    for (const auto& rcu_data : rcu_data_list) {
        if (rcu_data->cpu_id == cpu_id) {
            target_cpu = rcu_data;
            break;
        }
    }

    if (!target_cpu) {
        PRINT("CPU[%zu] not found in RCU data\n", cpu_id);
        return;
    }

    PRINT("================================================================================\n");
    PRINT("CPU[%zu] RCU Data Details:\n", cpu_id);
    PRINT("================================================================================\n");
    PRINT("RCU Data Address: %#lx\n", target_cpu->rcu_data_addr);
    PRINT("Grace Period Seq: %lu (needed: %lu)\n", target_cpu->gp_seq, target_cpu->gp_seq_needed);
    PRINT("Core Needs QS: %s\n", target_cpu->core_needs_qs ? "true" : "false");
    PRINT("Been Online: %s\n", target_cpu->beenonline ? "true" : "false");

    // Show callbacks for this CPU
    std::vector<std::shared_ptr<rcu_callback_info>> cpu_callbacks;
    for (const auto& callback : all_callbacks) {
        if (callback->cpu_id == cpu_id) {
            cpu_callbacks.push_back(callback);
        }
    }

    PRINT("Callback List: %zu callbacks\n", cpu_callbacks.size());

    if (!cpu_callbacks.empty()) {
        PRINT("\nCallback Details:\n");
        PRINT("%-4s %-18s %-18s %s\n", "#", "Address", "Next", "Function");
        PRINT("%-4s %-18s %-18s %s\n", "---", "-------", "----", "--------");

        for (size_t i = 0; i < cpu_callbacks.size(); i++) {
            const auto& callback = cpu_callbacks[i];
            PRINT("%-4zu %#-16lx %#-16lx %s\n",
                  i + 1, callback->callback_addr, callback->next_addr, callback->func_name.c_str());
        }
    }

    PRINT("================================================================================\n");
}

#pragma GCC diagnostic pop
