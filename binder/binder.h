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

#ifndef BINDER_DEFS_H_
#define BINDER_DEFS_H_

#include "plugin.h"

/**
 * Binder command argument structure
 * Used to pass filtering and display options to binder functions
 */
struct binder_argument_t {
    struct task_context *tc;  // Task context for address translation
    int pid;                   // Process ID to filter (0 = no filter)
    int dump_all;              // Flag: dump all processes if set
    int flags;                 // Bitmask of information types to display
};

/**
 * Binder priority structure - scheduler policy and priority
 * @sched_policy: Scheduler policy (SCHED_NORMAL, SCHED_FIFO, SCHED_RR, etc.)
 * @prio: Priority value [100..139] for SCHED_NORMAL, [0..99] for FIFO/RT
 *
 * The binder driver supports inheriting the following scheduler policies:
 * SCHED_NORMAL, SCHED_BATCH, SCHED_FIFO, SCHED_RR
 */
struct binder_priority {
    unsigned int sched_policy;  // Scheduling policy
    int prio;                   // Priority level
};

/**
 * Binder work type enumeration
 * Defines different types of work items that can be queued
 * There are separate work lists for proc, thread, and node (async)
 */
enum binder_work_type {
    BINDER_WORK_TRANSACTION = 1,                    // Transaction work item
    BINDER_WORK_TRANSACTION_COMPLETE,               // Transaction completion notification
    BINDER_WORK_TRANSACTION_PENDING,                // Pending transaction
    BINDER_WORK_TRANSACTION_ONEWAY_SPAM_SUSPECT,    // Suspected oneway spam transaction
    BINDER_WORK_RETURN_ERROR,                       // Error return work
    BINDER_WORK_NODE,                               // Node work item
    BINDER_WORK_DEAD_BINDER,                        // Dead binder notification
    BINDER_WORK_DEAD_BINDER_AND_CLEAR,              // Dead binder with clear
    BINDER_WORK_CLEAR_DEATH_NOTIFICATION,           // Clear death notification
};

/**
 * Binder work structure - work item enqueued on a worklist
 * @entry: List node for enqueueing
 * @type: Type of work to be performed
 */
struct binder_work {
    struct kernel_list_head entry;  // List entry for work queue
    enum binder_work_type type;     // Type of this work item
};

/**
 * Binder error structure - represents an error to be returned
 */
struct binder_error {
    struct binder_work work;  // Base work structure
    unsigned int cmd;         // Error command code
};

#define BINDERFS_MAX_NAME 255  // Maximum length of binder context name

/**
 * Binder transaction log entry structure
 * Records information about a single binder transaction for debugging
 */
struct binder_transaction_log_entry {
    int debug_id;                                  // Transaction debug ID
    int debug_id_done;                             // Completion debug ID
    int call_type;                                 // Call type (0=call, 1=async, 2=reply)
    int from_proc;                                 // Source process ID
    int from_thread;                               // Source thread ID
    int target_handle;                             // Target handle number
    int to_proc;                                   // Destination process ID
    int to_thread;                                 // Destination thread ID
    int to_node;                                   // Destination node ID
    int data_size;                                 // Size of transaction data
    int offsets_size;                              // Size of offsets array
    int return_error_line;                         // Line number where error occurred
    uint32_t return_error;                         // Error code returned
    uint32_t return_error_param;                   // Error parameter
    char context_name[BINDERFS_MAX_NAME + 1];     // Binder context name
};

/**
 * Binder transaction log structure
 * Circular buffer that records recent transactions for debugging
 */
struct binder_transaction_log {
    int cur;                                       // Current index in circular buffer
    bool full;                                     // Flag: buffer has wrapped around
    struct binder_transaction_log_entry entry[32]; // Array of log entries (32 max)
};

/**
 * Binder LRU page structure - tracks a page in the buffer pool
 * Pages are managed in an LRU (Least Recently Used) list
 */
struct binder_lru_page {
    struct kernel_list_head lru;  // LRU list entry
    void *page_ptr;               // Pointer to page structure
    void *alloc;                  // Pointer to owning allocation structure
};

/**
 * Binder plugin class - crash utility plugin for analyzing binder IPC
 * Provides commands to dump binder state from kernel crash dumps
 */
class Binder : public ParserPlugin {
private:
    // Display flags for controlling output
    static const int BINDER_THREAD = 0x0001;   // Display thread information
    static const int BINDER_NODE = 0x0002;     // Display node information
    static const int BINDER_REF = 0x0004;      // Display reference information
    static const int BINDER_ALLOC = 0x0008;    // Display allocation information

    // Scheduler policy names for display
    char* sched_name[7] = {
        TO_CONST_STRING("SCHED_NORMAL"),    // Normal scheduling
        TO_CONST_STRING("SCHED_FIFO"),      // First-in, first-out real-time
        TO_CONST_STRING("SCHED_RR"),        // Round-robin real-time
        TO_CONST_STRING("SCHED_BATCH"),     // Batch processing
        TO_CONST_STRING("SCHED_ISO"),       // Isochronous
        TO_CONST_STRING("SCHED_IDLE"),      // Idle priority
        TO_CONST_STRING("SCHED_DEADLINE"),  // Deadline scheduling
    };

    // Private helper functions for printing binder information
    void print_binder_transaction_log_entry(bool fail_log);  // Print transaction log entries
    void binder_proc_show(struct binder_argument_t* binder_arg);  // Show binder processes
    void print_binder_alloc(struct task_context *tc, ulong alloc_addr);  // Print allocation info
    void print_binder_proc(ulong proc_addr, int flags);  // Print process info
    void print_binder_node_nilocked(ulong node_addr);  // Print node info
    void print_binder_ref_olocked(ulong ref_addr);  // Print reference info
    void print_binder_thread_ilocked(ulong thread);  // Print thread info
    void print_binder_transaction_ilocked(ulong proc_addr, const char* prefix, ulong transaction);  // Print transaction info
    void print_binder_work_ilocked(ulong proc_addr, const char* prefix, const char* transaction_prefix, ulong work);  // Print work item info

public:
    Binder();  // Constructor
    void cmd_main(void) override;  // Main command entry point
    void init_offset(void) override;  // Initialize structure field offsets
    void init_command(void) override;  // Initialize command help information
    DEFINE_PLUGIN_INSTANCE(Binder)  // Define plugin instance
};

#endif // BINDER_DEFS_H_
