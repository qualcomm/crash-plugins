/**
 * @file ftrace.h
 * @brief Ftrace plugin for analyzing Linux kernel ftrace data
 *
 * This file contains the Ftrace plugin class and related structures for parsing
 * and analyzing Linux kernel ftrace (function tracer) data from crash dumps.
 * The plugin supports multiple trace arrays, ring buffer analysis, event parsing,
 * and trace data export functionality.
 *
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

#ifndef FTRACE_DEFS_H_
#define FTRACE_DEFS_H_

#include "plugin.h"
#include "trace_event.h"
#include "events.h"

/**
 * @brief Trace command options for ftrace data export
 *
 * These options are used when exporting ftrace data to trace-cmd compatible format.
 */
enum {
    TRACECMD_OPTION_DONE,         /**< End of options marker */
    TRACECMD_OPTION_DATE,         /**< Date information option */
    TRACECMD_OPTION_CPUSTAT,      /**< CPU statistics option */
    TRACECMD_OPTION_BUFFER,       /**< Buffer information option */
    TRACECMD_OPTION_TRACECLOCK,   /**< Trace clock option */
    TRACECMD_OPTION_UNAME,        /**< System uname option */
    TRACECMD_OPTION_HOOK,         /**< Hook information option */
};

/**
 * @brief Per-CPU ring buffer structure
 *
 * Represents a per-CPU ring buffer used by the ftrace subsystem.
 * Each CPU has its own ring buffer to avoid locking overhead.
 */
struct ring_buffer_per_cpu {
    ulong addr;                           /**< Kernel address of the ring buffer structure */
    int cpu;                              /**< CPU number this buffer belongs to */
    std::vector<ulong> buffer_pages;      /**< List of all buffer page addresses */
    std::vector<ulong> data_pages;        /**< List of pages containing actual trace data */
};

/**
 * @brief Trace array structure representing a ftrace instance
 *
 * Each trace array represents a separate ftrace instance (e.g., global, per-subsystem).
 * Contains per-CPU ring buffers and metadata about the trace instance.
 */
struct trace_array {
    ulong addr;                                                      /**< Kernel address of trace_array structure */
    int cpus;                                                        /**< Number of CPUs in this trace array */
    std::string name;                                                /**< Name of the trace array (e.g., "global", "binder") */
    uint64_t time_start;                                             /**< Start timestamp for this trace array */
    size_t nr_pages;                                                 /**< Number of pages per CPU buffer */
    size_t offset;                                                   /**< File offset when exporting trace data */
    std::vector<std::shared_ptr<ring_buffer_per_cpu>> cpu_ring_buffers; /**< Per-CPU ring buffers */
};

/**
 * @brief Ring buffer event header structure
 *
 * Each event in the ring buffer starts with this header containing
 * type information and timestamp delta.
 */
struct ring_buffer_event {
    uint32_t type_len : 5;      /**< Event type and length encoding */
    uint32_t time_delta : 27;   /**< Time delta from previous event */
    uint32_t array[];           /**< Variable length data array */
};

/**
 * @brief Saved command lines buffer structure
 *
 * Kernel structure for mapping PIDs to command line names in ftrace.
 * Used to resolve process names from PIDs in trace events.
 */
struct saved_cmdlines_buffer {
    ulong map_pid_to_cmdline;   /**< Address of PID to cmdline index mapping */
    ulong map_cmdline_to_pid;   /**< Address of cmdline index to PID mapping */
    unsigned int cmdline_num;   /**< Number of saved command lines */
    int cmdline_idx;            /**< Current command line index */
    ulong saved_cmdlines;       /**< Address of saved command line strings */
};

/**
 * @brief Ring buffer event types
 *
 * Different types of events that can be stored in the ring buffer.
 */
enum ring_buffer_type {
    RINGBUF_TYPE_DATA_TYPE_LEN_MAX = 28,  /**< Maximum data type length */
    RINGBUF_TYPE_PADDING,                 /**< Padding event */
    RINGBUF_TYPE_TIME_EXTEND,             /**< Time extension event */
    RINGBUF_TYPE_TIME_STAMP,              /**< Absolute timestamp event */
};

/**
 * @brief Parsed trace log entry
 *
 * Represents a single parsed trace log entry with metadata.
 */
struct trace_log {
    int cpu;                    /**< CPU number where event occurred */
    ulong timestamp;            /**< Event timestamp */
    std::string array_name;     /**< Name of trace array this log belongs to */
    std::string log;            /**< Formatted log string */
};

/**
 * @brief Ftrace plugin class for analyzing Linux kernel ftrace data
 *
 * This plugin provides comprehensive analysis of Linux kernel ftrace data including:
 * - Multiple trace array parsing (global, per-subsystem instances)
 * - Ring buffer analysis and event extraction
 * - Trace event parsing and formatting
 * - Command line resolution for PIDs
 * - Export to trace-cmd compatible format
 * - Statistical analysis and filtering
 */
class Ftrace : public ParserPlugin {
private:
    // Trace entry flags for latency format
    const int TRACE_FLAG_IRQS_OFF           = 0x01;  /**< IRQs disabled flag */
    const int TRACE_FLAG_IRQS_NOSUPPORT     = 0x02;  /**< IRQ state not supported */
    const int TRACE_FLAG_NEED_RESCHED       = 0x04;  /**< Need reschedule flag */
    const int TRACE_FLAG_HARDIRQ            = 0x08;  /**< In hard IRQ context */
    const int TRACE_FLAG_SOFTIRQ            = 0x10;  /**< In soft IRQ context */
    const int TRACE_FLAG_PREEMPT_RESCHED    = 0x20;  /**< Preempt resched flag */
    const int TRACE_FLAG_NMI                = 0x40;  /**< In NMI context */
    const int TRACE_FLAG_BH_OFF             = 0x80;  /**< Bottom halves disabled */
    // System configuration
    int pid_max;                                                                /**< Maximum PID value in system */
    int nr_cpu_ids = 0;                                                         /**< Number of CPUs in system */

    // File export related
    std::string trace_path;                                                     /**< Path for exported trace file */
    FILE* trace_file;                                                           /**< File handle for trace export */

    // Trace data structures
    std::shared_ptr<trace_array> global_trace;                                  /**< Global trace array instance */
    std::vector<std::shared_ptr<trace_array>> trace_list;                       /**< List of all trace arrays */
    std::vector<std::shared_ptr<trace_log>> trace_logs;                         /**< Parsed trace log entries */
    std::shared_ptr<saved_cmdlines_buffer> savedcmd_ptr;                        /**< Command line resolution buffer */

    // Event parsing structures
    std::unordered_map<unsigned int, std::shared_ptr<TraceEvent>> event_maps;   /**< Map of event ID to event handler */
    std::unordered_map<std::string, std::shared_ptr<trace_field>> common_field_maps; /**< Common trace fields */
    std::unordered_map<int, std::string> comm_pid_dict;                         /**< PID to command name cache */
    // Trace array parsing methods
    std::shared_ptr<trace_array> parser_trace_array(ulong addr);                /**< Parse trace array structure */
    ulong buffer_page_has_data(ulong addr);                                     /**< Check if buffer page has data */
    void parser_trace_buffer(std::shared_ptr<trace_array> ta, ulong addr);      /**< Parse trace buffer structure */
    void parser_ring_buffer_per_cpu(std::shared_ptr<trace_array> ta, ulong addr); /**< Parse per-CPU ring buffer */

    // Trace data export methods
    bool write_trace_data();                                                     /**< Write complete trace data to file */
    void write_buffer_data();                                                    /**< Write buffer data section */
    void write_trace_array_buffers(std::shared_ptr<trace_array> ta_ptr);        /**< Write trace array buffers */
    void write_res_data();                                                       /**< Write resource data section */
    void write_cmdlines();                                                       /**< Write command line data */
    void write_printk_data();                                                    /**< Write printk format data */
    void write_kallsyms_data();                                                  /**< Write kernel symbol data */
    void save_proc_kallsyms_mod_v6_4(std::vector<char> &buf, size_t &pos);      /**< Save module symbols (v6.4+) */
    void save_proc_kallsyms_mod_legacy(std::vector<char> &buf, size_t &pos);    /**< Save module symbols (legacy) */
    void write_to_buf(std::vector<char> &buffer, size_t &pos, const char *fmt, ...); /**< Write formatted data to buffer */
    void write_init_data();                                                      /**< Write trace file header */
    void dump_file();                                                            /**< Dump trace file contents */
    void write_header_data();                                                    /**< Write header page data */
    void write_events_data();                                                    /**< Write events data section */
    void write_events(uint system_id);                                           /**< Write events for specific system */
    void write_event(std::shared_ptr<TraceEvent> event_ptr);                     /**< Write single event definition */

    // Display and utility methods
    void ftrace_show();                                                          /**< Show trace using trace-cmd */
    void ftrace_dump();                                                          /**< Dump trace to file */
    void print_trace_log(std::string name);                                     /**< Print trace logs by array name */
    void print_trace_log();                                                      /**< Print all trace logs */
    void print_trace_log(int cpu);                                              /**< Print trace logs by CPU */
    void print_event_format();                                                   /**< Print event format information */
    void print_trace_array();                                                    /**< Print trace array information */

    // Parsing and analysis methods
    void parser_savedcmd();                                                      /**< Parse saved command lines */
    void parser_ftrace_trace_arrays();                                          /**< Parse all trace arrays */
    void parse_buffer_page(std::shared_ptr<trace_array> ta_ptr, std::shared_ptr<ring_buffer_per_cpu> rb_ptr, ulong addr); /**< Parse buffer page */
    int get_trace_length(ulong event_addr, ring_buffer_event *event, ulong event_end); /**< Get trace event length */
    void parse_trace_entry(std::shared_ptr<trace_array> ta_ptr, std::shared_ptr<ring_buffer_per_cpu> rb_ptr, ulong addr, ulong timestamp); /**< Parse trace entry */
    std::vector<std::string> extractPrefixes(const std::string &input);         /**< Extract format prefixes */
    std::string find_cmdline(int pid);                                           /**< Find command line for PID */

    // Event parsing methods
    std::shared_ptr<TraceEvent> parser_trace_event_call(ulong addr);            /**< Parse trace event call */
    void parser_common_trace_fields();                                          /**< Parse common trace fields */
    uint get_event_type_id(ulong addr);                                         /**< Get event type ID */
    std::string get_event_type_name(ulong addr);                                /**< Get event type name */
    std::string get_event_type_system(ulong addr);                              /**< Get event system name */
    std::string get_event_type_print_fmt(ulong addr);                           /**< Get event print format */
    std::shared_ptr<trace_field> parser_event_field(ulong addr);                /**< Parse event field */
    std::string get_lat_fmt(unsigned char flags, unsigned char preempt_count);  /**< Get latency format string */
    void parser_ftrace_events_list();                                           /**< Parse ftrace events list */

public:
    /**
     * @brief Constructor for Ftrace plugin
     */
    Ftrace();

    void print_skip_events();

    /**
     * @brief Main command handler for ftrace plugin
     */
    void cmd_main(void) override;

    /**
     * @brief Initialize structure field offsets
     */
    void init_offset(void) override;

    /**
     * @brief Initialize command help and usage information
     */
    void init_command(void) override;

    DEFINE_PLUGIN_INSTANCE(Ftrace)
};

#endif // FTRACE_DEFS_H_
