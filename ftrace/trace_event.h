/**
 * @file trace_event.h
 * @brief Trace event parsing and formatting for ftrace
 *
 * This file contains structures and classes for parsing and formatting
 * Linux kernel trace events. It provides a flexible framework for handling
 * different event types and their associated data fields.
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

#ifndef TRACE_EVENT_DEFS_H_
#define TRACE_EVENT_DEFS_H_

#include "plugin.h"
#include <unordered_set>

/**
 * @brief Common trace entry header
 *
 * Every trace event starts with this common header containing
 * basic information about the event.
 */
struct trace_entry {
    unsigned short type;        /**< Event type ID */
    unsigned char flags;        /**< Trace flags (IRQ state, etc.) */
    unsigned char preempt_count; /**< Preemption count */
    int pid;                    /**< Process ID that generated the event */
};

/**
 * @brief Trace event field descriptor
 *
 * Describes a single field within a trace event structure,
 * including its type, location, and properties.
 */
struct trace_field {
    std::string name;           /**< Field name */
    std::string type;           /**< Field type (e.g., "int", "char[]") */
    int offset;                 /**< Offset within event structure */
    int size;                   /**< Size of field in bytes */
    int is_signed;              /**< Whether field is signed */
    int filter_type;            /**< Filter type for this field */
    std::vector<char> val;      /**< Field value buffer */
};

struct print_arg {
    std::string name;        /**< Argument/field name */
    std::string prefix;      // Prefix text before the format specifier, e.g., "skb=" or " hp="
    std::string format;      // Format specifier including %, e.g., "%d", "%lx", "%pK"
    std::string suffix;      // Suffix text after the last format specifier (only for the last token)
    char data[512];
};

/**
 * @brief Flag mapping structure for bitfield decoding
 *
 * Maps bit masks to their string representations for flag fields.
 */
struct flag_map {
    unsigned int mask;
    const char *name;
};

/**
 * @brief Base class for trace event parsing and formatting
 *
 * This class provides the framework for parsing trace events from
 * the kernel ring buffer and formatting them for display. Specific
 * event types can derive from this class to provide custom formatting.
 */
class TraceEvent{
public:
    /**
     * @brief Constructor for TraceEvent
     */
    TraceEvent();

    bool skiped = false;
    bool has_parserd = true;                                                           /**< Whether this event type is skipped */

    std::string format_arg(std::shared_ptr<print_arg> arg_ptr);

    /**
     * @brief Set the print format string for this event
     * @param format The kernel's print format string
     */
    void set_print_format(std::string format);

    /**
     * @brief Parse the output format string
     * @param format The format string to parse
     */
    void parser_output_format(std::string format);

    /**
     * @brief Extract format string from print_fmt
     * @param input The input string containing format
     * @return Extracted format string
     */
    std::string extractFormatString(const std::string& input);

    /**
     * @brief Check if format spec contains advanced features
     * @param spec The format specification to check
     * @return True if advanced features detected
     */
    bool contains_advanced_features(const std::string& spec);

    /**
     * @brief Extract field names from format string
     * @param input The format string
     * @return Vector of field names
     */
    std::vector<std::string> extract_field_names(const std::string& input);

    bool isIdentifierChar(char c);
    std::string extractIdentifier(const std::string &str, size_t &pos);
    void skipOpenParensAndSpaces(const std::string &str, size_t &pos);

    std::vector<std::shared_ptr<print_arg>> parseFormatString(const std::string& input);

    /**
     * @brief Trim whitespace from string
     * @param str The string to trim
     * @return Trimmed string
     */
    std::string trim(const std::string& str);

    /**
     * @brief Parse format specification
     * @return True if advanced features found
     */
    bool parse_format_spec();

    /**
     * @brief Check if format has advanced features
     * @return True if advanced features present
     */
    bool has_advanced_features();

    /**
     * @brief read a trace field value
     * @param addr Address of event data
     */
    void read_trace_field(ulong addr, std::shared_ptr<print_arg> arg_ptr);

    /**
     * @brief Print softirq event details
     * @param addr Address of event data
     * @param oss Output string stream
     */
    void print_softirq(ulong addr);

    /**
     * @brief Print DWC3 endpoint status
     * @param addr Address of event data
     * @param oss Output string stream
     */
    void print_dwc3_ep_status(ulong addr);

    /**
     * @brief Print DWC3 TRB event
     * @param addr Address of event data
     * @param oss Output string stream
     */
    void print_dwc3_trb_event(ulong addr);

    /**
     * @brief Print DMA unmap event
     * @param addr Address of event data
     * @param oss Output string stream
     */
    void print_dma_unmap_event(ulong addr);

    /**
     * @brief Print DMA map event
     * @param addr Address of event data
     * @param oss Output string stream
     */
    void print_dma_map_event(ulong addr);

    /**
     * @brief Dump event information for debugging
     */
    void dump();
    void ufshcd_runtime_event_handle(ulong addr);
    void ext4_event_fallocate_punch_zero_handle(ulong addr);
    void ext4_es_extent_event_handle(ulong addr);
    void lease_event_handle(ulong addr);
    void dwc3_request_event_handle(ulong addr);
    void xhci_urb_event_handle(ulong addr);
    void spi_transfer_event_handle(ulong addr);
    void dwc3_trb_event_handle(ulong addr);
    void usb_gadget_request_event_handle(ulong addr);
    void dwc3_gadget_ep_event_handle(ulong addr);
    void usb_ep_event_handle(ulong addr);
    void xhci_dbc_event_handle(ulong addr);
    void usb_gadget_event_handle(ulong addr);
    void f2fs_compress_pages_event_handle(ulong addr);
    void f2fs_prepare_bio_event_handle(ulong addr);
    void f2fs_writepage_event_handle(ulong addr);
    void f2fs_submit_page_event_handle(ulong addr);
    void f2fs_sync_dirty_event_handle(ulong addr);
    void v4l2_buf_event_handle(ulong addr);
    void v4l2_dqbuf_event_handle(ulong addr);
    void i2c_event_handle(ulong addr);
    void smbus_event_handle(ulong addr);
    void dump2();

    uint id;                                                                    /**< Event type ID */
    uint system_id;                                                             /**< System ID this event belongs to */
    std::string name;                                                           /**< Event name */
    std::string system;                                                         /**< System name (e.g., "sched", "irq") */
    std::string org_print_fmt;                                                  /**< Original print format string */
    std::vector<std::shared_ptr<print_arg>> arg_list;                                            /**< List of print arguments */
    std::string print_fmt;                                                      /**< Parsed print format */
    std::string struct_type;                                                    /**< Event structure type name */
    std::unordered_map<std::string, std::shared_ptr<trace_field>> field_maps;  /**< Map of field name to field descriptor */
    size_t pos_ = 0;                                                            /**< Current parsing position */
    static size_t skip_cnt;                                                     /**< Count of skipped events */
    void copy_str(std::shared_ptr<print_arg> arg_ptr, const std::string &data);

    virtual void handle(ulong addr);
    virtual void print_log(std::ostringstream &oss);
    void print_rwmmio_event(std::ostringstream& oss);
    // Common helper functions for decoding event fields
    std::string decode_dma_direction(int dir);
    std::string decode_dma_attrs(ulong attrs);
    std::string decode_softirq_vec(unsigned int vec);
    std::string decode_dwc3_trb_type(uint trb_type);
    std::string decode_alarm_type(uint alarm_type);
    std::string decode_lru_flags(unsigned int flags);
    std::string decode_gfp_flags(unsigned long gfp_flags);
    std::string decode_rss_stat_member(int member);
    std::string decode_lru_type(int lru);
    std::string decode_reclaim_flags(unsigned int flags);
    std::string decode_compaction_status(int status);
    std::string decode_zone_type(int idx);
    std::string decode_migrate_mode(int mode);
    std::string decode_migrate_reason(int reason);
    std::string decode_khugepaged_status(int status);
    std::string decode_inode_state(unsigned long state);
    std::string decode_writeback_reason(int reason);
    std::string decode_scsi_prot_op(int prot_op);
    std::string decode_scsi_opcode(unsigned char opcode);
    std::string decode_scsi_host_status(unsigned char host_byte);
    std::string decode_scsi_sam_status(unsigned char status_byte);
    std::string scsi_trace_parse_cdb(const unsigned char *cdb, int len);
    std::string format_hex_string(const unsigned char *data, int len);
    std::string decode_mem_type(int mem_type);
    std::string decode_compact_priority(int priority);
    std::string decode_compact_result(int result);
    std::string decode_file_lock_flags(unsigned int flags);
    std::string decode_file_lock_type(unsigned char type);
    std::string decode_iomap_type(int type);
    std::string decode_iomap_flags(unsigned int flags);
    std::string decode_iomap_iter_flags(unsigned int flags);
    std::string decode_ext4_alloc_flags(unsigned int flags);
    std::string decode_ext4_free_flags(unsigned int flags);
    std::string decode_ext4_fallocate_mode(unsigned int mode);
    std::string decode_ext4_map_flags(unsigned int flags);
    std::string decode_ext4_mflags(unsigned int mflags);
    std::string decode_ext4_es_status(unsigned int status);
    std::string decode_task_state(long state);
    std::string get_task_event_name(int evt);
    std::string decode_ufs_trace_str_t(int str_t);
    std::string decode_ufs_trace_tsf_t(int tsf_t);
    std::string decode_ufs_pwr_mode(int mode);
    std::string decode_ufs_link_state(int state);
    std::string decode_ufs_clk_gating_state(int state);
    std::string decode_spi_mode_flags(unsigned long mode);
    std::string format_buffer_hex(const unsigned char *buf, int len);
    std::string decode_usb_request_flags(int zero, int short_not_ok, int no_interrupt);
    std::string decode_usb_gadget_flags(bool sg_supported, bool is_otg, bool is_a_peripheral,
                                        bool b_hnp_enable, bool a_hnp_support, bool hnp_polling_support,
                                        bool host_request_flag, bool quirk_ep_out_aligned_size,
                                        bool quirk_altset_not_supp, bool quirk_stall_not_supp,
                                        bool quirk_zlp_not_supp, bool is_selfpowered,
                                        bool deactivated, bool connected);
    std::string decode_usb_pipe_type(int type);
    std::string decode_dwc3_ep_flags(unsigned long flags, int direction);
    std::string decode_dwc3_trb_ctrl_flags(unsigned int ctrl);
    std::string get_dwc3_pcm_string(int type, unsigned int size);
    std::string decode_f2fs_compress_algorithm(int algtype);
    std::string decode_f2fs_shutdown_mode(int mode);
    std::string decode_f2fs_inode_type(int type);
    std::string decode_f2fs_extent_type(int type);
    std::string decode_f2fs_page_type(int type);
    std::string decode_f2fs_dir_type(int dir);
    std::string decode_f2fs_temp_type(int temp);
    std::string decode_f2fs_gc_type(int gc_type);
    std::string decode_f2fs_alloc_mode(int mode);
    std::string decode_f2fs_gc_mode(int mode);
    std::string decode_f2fs_victim_type(int type);
    std::string decode_f2fs_cp_reason(int reason);
    std::string decode_v4l2_buffer_flags(unsigned int flags);
    std::string decode_v4l2_field(int field);
    std::string decode_v4l2_timecode_type(int type);
    std::string decode_v4l2_timecode_flags(unsigned int flags);
    std::string decode_v4l2_buf_type(int type);
    std::string decode_thermal_trip_type(int trip_type);
    std::string decode_smbus_protocol(int protocol);
    std::string decode_swiotlb_force(int swiotlb_force);
    std::string decode_smcinvoke_cmd(unsigned int cmd);
    std::string decode_tick_dependency(int dependency);
    std::string decode_clockid(uint clockid);
    std::string decode_hrtimer_mode(uint mode);
    std::string decode_task_state(long state, uint16_t mask);
    std::string build_flag_string(unsigned int flags, const flag_map *maps, int count);
    ParserPlugin* plugin_ptr;                                                   /**< Pointer to parent plugin */
};

#endif // TRACE_EVENT_DEFS_H_
