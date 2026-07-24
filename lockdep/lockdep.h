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

#ifndef LOCKDEP_DEFS_H_
#define LOCKDEP_DEFS_H_

#include "plugin.h"

/**
 * Detailed information structure for held locks
 * Corresponds to struct held_lock in the kernel
 */
struct HeldLockInfo {
    ulong prev_chain_key;      // Lock chain key for deadlock detection
    ulong acquire_ip;          // Instruction pointer where lock was acquired
    ulong acquire_ip_caller;   // Caller's address
    ulong instance;            // Memory address of lock instance
    ulong nest_lock;           // Nested lock
    uint32_t class_idx_full;   // Complete lock class index (contains various flags)
    uint32_t pin_count;        // Lock reference count
    uint64_t waittime_stamp;   // Timestamp when waiting for lock
    uint64_t holdtime_stamp;   // Timestamp when holding lock

    // Bit field information parsed from class_idx
    uint32_t class_idx;        // Lock class index (bit 0-12)
    uint32_t irq_context;      // Interrupt context flag (bit 13-14)
    uint32_t trylock;          // Whether it's a trylock (bit 15)
    uint32_t read_mode;        // Read lock mode (bit 16-17)
    uint32_t check;            // Lock check flag (bit 18)
    uint32_t hardirqs_off;     // Hard interrupt disabled flag (bit 19)
    uint32_t references;       // Reference count (bit 20-31)

    // Parsed readable information
    std::string lock_name;     // Name of the lock
    std::string acquire_function; // Function name that acquired the lock
};

/**
 * Task's lock dependency information
 * Contains task context and information about all held locks
 */
struct TaskLockdepInfo {
    struct task_context *tc;   // Task context
    uint32_t lockdep_depth;    // Number of currently held locks
    std::vector<HeldLockInfo> held_locks; // List of held locks
};

/**
 * Lockdep parser class
 * Used to analyze Linux kernel lock dependency information and detect potential deadlock issues
 */
class Lockdep : public ParserPlugin {
private:
    // Core parsing functions
    void parse_all_tasks_lockdep();
    void parse_tasks_summary();
    void parse_single_task_detail(ulong task_addr);
    void parse_task_held_locks(ulong task_addr, TaskLockdepInfo& task_info);
    bool test_bit(uint32_t nr, ulong addr);
    void parse_class_idx_bits(uint32_t class_idx_full, HeldLockInfo& lock_info);
    std::string get_lock_name(uint32_t class_idx);

    // Formatting output functions - convert lock states to readable descriptions
    std::string get_lock_mode_description(uint32_t hl_read);
    std::string get_acquire_method_description(uint32_t hl_trylock);
    std::string get_irq_state_description(uint32_t hl_hardirqs_off, uint32_t hl_irq_context);
    std::string get_lock_validation_description(uint32_t hl_check);
    std::string get_lock_status_description(uint64_t waittime, uint64_t holdtime);
    std::string get_timing_description(uint64_t waittime, uint64_t holdtime);
    std::string format_lock_info_tree(int lock_index, ulong instance, const HeldLockInfo& lock_info);

    // Global lockdep data addresses
    ulong lock_classes_in_use_bitmap; // Lock class usage bitmap address
    ulong lock_classes;               // Lock class array address
    uint32_t sizeof_held_lock;        // Size of held_lock structure
    uint32_t sizeof_lock_class;       // Size of lock_class structure

public:
    Lockdep();
    void init_offset(void) override;
    void init_command(void) override;
    void cmd_main(void) override;
    DEFINE_PLUGIN_INSTANCE(Lockdep)
};

#endif // LOCKDEP_DEFS_H_
