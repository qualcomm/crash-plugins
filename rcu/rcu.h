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

#ifndef __RCU_H__
#define __RCU_H__

#include "../plugin.h"
#include <vector>
#include <memory>
#include <unordered_set>
#include <string>

/**
 * @brief RCU callback information structure
 */
struct rcu_callback_info {
    ulong callback_addr;        // Address of callback_head structure
    ulong next_addr;           // Next callback in chain
    ulong func_addr;           // Function pointer
    std::string func_name;     // Resolved function name
    size_t cpu_id;             // CPU ID this callback belongs to
};

/**
 * @brief RCU segmented callback list information
 */
struct rcu_segcblist_info {
    ulong cblist_addr;         // Address of rcu_segcblist structure
    ulong head_addr;           // Head of callback chain
    long len;                  // Total length of callback list
    ulong tails[4];            // Tail pointers for each segment
    ulong gp_seq[4];           // Grace period sequence numbers
    long seglen[4];            // Length of each segment
    std::vector<std::shared_ptr<rcu_callback_info>> callbacks;
    size_t cpu_id;             // CPU ID this list belongs to
};

/**
 * @brief Per-CPU RCU data information
 */
struct rcu_data_info {
    ulong rcu_data_addr;       // Address of rcu_data structure
    ulong gp_seq;              // Current grace period sequence
    ulong gp_seq_needed;       // Needed grace period sequence
    bool core_needs_qs;        // Core needs quiescent state
    bool beenonline;           // Has been online
    std::shared_ptr<rcu_segcblist_info> cblist_info;
    size_t cpu_id;             // CPU ID
};

/**
 * @brief RCU statistics summary
 */
struct rcu_statistics {
    size_t total_cpus;         // Total active CPUs
    size_t total_callbacks;    // Total callbacks across all CPUs
    size_t total_cblists;      // Total callback lists
    std::unordered_map<std::string, size_t> func_counts; // Function call counts
};

/**
 * @brief RCU analysis plugin class
 */
class Rcu : public ParserPlugin {
private:
    // Data storage
    std::vector<std::shared_ptr<rcu_data_info>> rcu_data_list;
    std::vector<std::shared_ptr<rcu_callback_info>> all_callbacks;

    // Core analysis functions
    void analyze_rcu_data();
    void parse_rcu_segcblist(ulong cblist_addr, size_t cpu_id, size_t& total_callbacks);
    void parse_callback_chain(ulong head_addr, size_t cpu_id, size_t& total_callbacks);

    // Display functions
    void print_rcu_data_details();
    void print_callback_statistics();
    void print_cpu_rcu_details(size_t cpu_id);

public:
    /**
     * @brief Default constructor
     */
    Rcu();

    /**
     * @brief Main command entry point - handles command line arguments
     */
    void cmd_main(void) override;

    /**
     * @brief Initialize kernel structure field offsets
     */
    void init_offset(void) override;

    /**
     * @brief Initialize command help and usage information
     */
    void init_command(void) override;

    DEFINE_PLUGIN_INSTANCE(Rcu)
};

#endif // __RCU_H__
