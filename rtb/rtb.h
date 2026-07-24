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

#ifndef RTB_DEFS_H_
#define RTB_DEFS_H_

#include "plugin.h"
#include <cmath>
#include <memory>

// RTB (Register Trace Buffer) log event types
// These represent different types of kernel events that can be logged
enum logk_event_type {
    LOGK_NONE = 0,       // No event
    LOGK_READL = 1,      // Memory read operation
    LOGK_WRITEL = 2,     // Memory write operation
    LOGK_LOGBUF = 3,     // Log buffer event
    LOGK_HOTPLUG = 4,    // CPU hotplug event
    LOGK_CTXID = 5,      // Context ID change
    LOGK_TIMESTAMP = 6,  // Timestamp marker
    LOGK_L2CPREAD = 7,   // L2 cache read
    LOGK_L2CPWRITE = 8,  // L2 cache write
    LOGK_IRQ = 9,        // Interrupt event
};

// RTB sentinel values for validating log entries
#define RTB_SENTINEL_0 0xFF
#define RTB_SENTINEL_1 0xAA
#define RTB_SENTINEL_2 0xFF

// Log type mask to extract actual type from flags
#define RTB_LOG_TYPE_MASK 0x7F

// RTB layout structure - represents a single log entry in the trace buffer
// This structure is packed to match kernel memory layout exactly
struct rtb_layout {
    unsigned char sentinel[3];  // Magic bytes to validate entry integrity
    unsigned char log_type;     // Type of log event (see logk_event_type)
    uint32_t idx;               // Sequential index of this entry
    uint64_t caller;            // Address of the function that logged this event
    uint64_t data;              // Event-specific data payload
    uint64_t timestamp;         // Timestamp in nanoseconds
    uint64_t cycle_count;       // CPU cycle count at time of logging
} __attribute__ ((__packed__));

// RTB state structure - holds the overall state of the trace buffer
struct rtb_state {
    std::vector<ulong> rtb_idx; // Per-CPU indices into the ring buffer
    ulong rtb_layout;           // Virtual address of the layout array
    physaddr_t phys;            // Physical address of the buffer
    int nentries;               // Total number of entries in the buffer
    int size;                   // Total size of the buffer in bytes
    int enabled;                // Whether RTB logging is enabled
    int initialized;            // Whether RTB has been initialized
    uint32_t filter;            // Event filter mask
    int step_size;              // Number of CPUs (step between per-CPU entries)
};

// Rtb class - Main plugin class for parsing and displaying RTB logs
class Rtb : public ParserPlugin {
private:
    std::shared_ptr<rtb_state> rtb_state_ptr;  // Shared pointer to parsed RTB state

    // Core parsing and validation functions
    void parser_rtb_log();                      // Parse RTB log from kernel memory
    bool is_enable_rtb();                       // Check if RTB is enabled and available
    bool validate_rtb_layout(const rtb_layout& layout); // Validate RTB layout entry

    // Display functions
    void print_rtb_log();                       // Print all RTB logs from all CPUs
    void print_percpu_rtb_log(int cpu);         // Print RTB logs for a specific CPU
    void print_rtb_log_memory();                // Display RTB memory layout information
    int print_rtb_layout(int cpu, int index);   // Print a single RTB entry

    // Navigation and utility functions
    int next_rtb_entry(int index);              // Calculate next entry index in ring buffer

    // Event-specific print functions
    void print_none(int cpu, struct rtb_layout& layout);
    void print_read_write(int cpu, struct rtb_layout& layout);
    void print_logbuf(int cpu, struct rtb_layout& layout);
    void print_hotplug(int cpu, struct rtb_layout& layout);
    void print_ctxid(int cpu, struct rtb_layout& layout);
    void print_timestamp(int cpu, struct rtb_layout& layout);
    void print_l2cpread_write(int cpu, struct rtb_layout& layout);
    void print_irq(int cpu, struct rtb_layout& layout);

    // Helper functions for extracting information
    std::string get_caller(struct rtb_layout& layout);    // Get source code location
    double get_timestamp(struct rtb_layout& layout);      // Convert timestamp to seconds

public:
    Rtb();
    void cmd_main(void) override;
    void init_offset(void) override;
    void init_command(void) override;
    DEFINE_PLUGIN_INSTANCE(Rtb)
};

#endif // RTB_DEFS_H_
