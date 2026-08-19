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
#include "lock_decoder.h"

/**
 * Detailed information structure for held locks
 * Corresponds to struct held_lock in the kernel
 */
struct HeldLockInfo {
    ulong prev_chain_key;      // Lock chain key for deadlock detection
    ulong acquire_ip;          // Instruction pointer where lock was acquired
    ulong instance;            // Memory address of lock instance
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
public:
    /* LockType is declared first so private method signatures below can use it */
    enum class LockType { NONE, MUTEX, RT_MUTEX, RWSEM, PERCPU_RWSEM, SEMAPHORE, COMPLETION, SPINLOCK };
    ulong find_any_blocking_lock(ulong task_addr, LockType &lock_type, ulong &owner_raw_out);

private:
    // Core parsing functions
    bool ensure_lockdep_globals();
    void parse_task_held_locks(ulong task_addr, TaskLockdepInfo& task_info);
    bool test_bit(uint32_t nr, ulong addr);
    void parse_class_idx_bits(uint32_t class_idx_full, HeldLockInfo& lock_info);
    std::string get_lock_name(uint32_t class_idx);

    // Unified lock classification and blocked-on detection.

    /* Per-task info for comprehensive analysis (-t without target) */
    struct TaskScanInfo {
        ulong task_addr;
        std::vector<ulong>       held_addrs;
        std::vector<std::string> held_names;
        std::vector<ulong>       held_ips;
        ulong    blocked_lock  = 0;
        LockType blocked_type  = LockType::NONE;
        ulong    blocked_owner = 0;
        bool     blocked_is_io = false;
    };
    /* Per-lock info for comprehensive analysis (-t without target) */
    struct LockScanInfo {
        LockType    type = LockType::NONE;
        std::string name;
        ulong       holder = 0;
        std::vector<ulong> waiters;
        int         stack_refs = 0;
        bool        is_external = false;
    };

    std::vector<ulong> collect_lock_waiters(ulong lock_addr, LockType type,
                                            ulong skip_task);
    LockType classify_lock_type(const std::string &class_name, ulong instance);
    std::string format_task_line(ulong task_addr);

    // Lock object analysis (used by -t <lock_addr>)
    void show_lock_object(ulong lock_addr);
    int  task_prio(ulong task_addr);
    std::string format_prio_str(ulong task_addr);  // prio with PI boost annotation

    // Chain rendering helpers for -t task snapshots
    std::string format_task_entry(ulong task_addr, bool detailed = false);
    std::string chain_lock_label(ulong lock_addr, LockType ltype);
    ulong       chain_follow_task(ulong lock_addr, LockType ltype, ulong owner_raw);
    bool        try_print_rwlock_stuck(ulong rt_mutex_addr, const std::string& prefix);
    void        print_lock_chain(ulong cur_task, const std::string& prefix,
                                 bool is_last, int depth, bool print_self,
                                 std::unordered_set<ulong>& visited);

    // Held lock discovery without LOCKDEP
    std::vector<ulong> find_held_locks_pi_waiters(ulong task_addr);
    std::string        try_name_percpu_lock(ulong lock_addr);
    std::vector<ulong> find_held_locks_all(ulong task_addr,
                                           std::vector<std::string> &names,
                                           std::vector<std::string> &sources);

    // Comprehensive analysis (used by -t without target)
    void show_lock_analysis();
    void collect_lock_analysis(std::vector<TaskScanInfo> &tasks,
                               std::unordered_map<ulong, LockScanInfo> &lmap);
    void scan_stack_refs(ulong task_addr,
                         std::unordered_map<ulong, int> &refs);
    void find_deadlock_cycles(const std::unordered_map<ulong, std::vector<ulong>> &graph,
                    const std::vector<ulong> &nodes,
                    std::vector<std::vector<ulong>> &sccs);
    void print_lock_analysis(std::vector<TaskScanInfo> &tasks,
                             std::unordered_map<ulong, LockScanInfo> &lmap);
    ulong find_blocking_lock_on_stack(ulong task_addr, LockType &type_out, ulong &raw_out);
    std::vector<std::pair<ulong, LockType>> find_held_locks_on_stack(ulong task_addr);

    // Enhanced -d: lock nesting detection
    void detect_lock_nesting();
    void print_task_nesting_detail(ulong task_addr);

    // Enhanced -t: lock chain investigation
    void show_task_lock_snapshot(ulong task_addr);
    int  describe_spinlock(ulong lock_addr);  // returns 0=unlocked, 1=locked, -1=err
    std::vector<ulong> collect_semaphore_waiters(ulong sem_addr, ulong skip_task);
    void show_lock_smart(ulong addr);

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
    void init_command(void) override;
    void init_offset(void) override;
    void cmd_main(void) override;
    DEFINE_PLUGIN_INSTANCE(Lockdep)
};

#endif // LOCKDEP_DEFS_H_
