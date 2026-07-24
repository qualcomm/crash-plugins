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

#ifndef JOURNAL_DEFS_H_
#define JOURNAL_DEFS_H_

#include "plugin.h"
#include "utils/utask.h"
#include <systemd/sd-journal.h>

/**
 * @brief Structure representing a single systemd journal log entry
 *
 * Contains parsed information from a systemd journal entry including
 * timestamp, process information, and message content
 */
struct journal_log {
    uint64_t timestamp;          ///< Log timestamp in microseconds since epoch (1970-01-01) - CLOCK_REALTIME
    uint64_t monotonic_timestamp; ///< Monotonic timestamp in microseconds since boot - CLOCK_MONOTONIC
    std::string pid;             ///< Process ID as string
    std::string message;         ///< Log message content
    std::string com;             ///< Process command name
    std::string hostname;        ///< Hostname where the log was generated
};

/**
 * @brief Structure containing journal file information and statistics
 *
 * Stores kernel-level information about journal files including inode address,
 * file size, cached pages count and other statistical data
 */
struct JournalFileInfo {
    ulong inode_addr;       ///< Kernel address of the inode structure
    ulong mapping_addr;     ///< Address of address_space structure for page cache management
    ulonglong file_size;    ///< Total file size in bytes
    ulong cached_pages;     ///< Number of pages currently in page cache
    int cache_percentage;   ///< Cache percentage (0-100)
};

/**
 * @brief SystemD Journal analyzer plugin for crash utility
 *
 * This plugin provides functionality to analyze systemd journal files from
 * kernel crash dumps. It can discover journal files in both process memory (VMA)
 * and page cache (inodes), extract their contents, and parse the journal format.
 *
 * Main features:
 * - List journal files found in memory
 * - Dump journal files to disk for analysis
 * - Parse and display journal log entries
 * - Provide statistics about journal file usage
 */
class Journal : public ParserPlugin {
private:
    // Configuration and state
    bool debug = false;                                                          ///< Debug mode flag
    std::shared_ptr<UTask> task_ptr = nullptr;                                  ///< Task utility for process analysis
    std::shared_ptr<Swapinfo> swap_ptr;                                         ///< Swap information utility
    struct task_context *tc_systemd_journal = nullptr;                          ///< systemd-journal process context

    // Data containers
    std::unordered_map<ulong, std::shared_ptr<vma_struct>> log_vma_list;        ///< Journal files in process VMA (key: VMA start address)
    std::unordered_map<std::string, ulong> log_inode_list;                      ///< Journal files in page cache
    std::vector<std::shared_ptr<journal_log>> log_list;                         ///< Parsed journal log entries

    // Core functionality methods
    void get_journal_vma_list();                                                ///< Scan process VMA for journal files
    void get_journal_inode_list();                                              ///< Scan page cache for journal files
    void write_vma_to_file(const std::vector<std::shared_ptr<vma_struct>>& vma_list,
                          const std::string& filename, const std::string& dst_dir, bool show_log=false);  ///< Write VMA data to file

    // Display and output methods
    void show_journal_log();                                                    ///< Display parsed journal entries
    void dump_journal_log();                                                    ///< Dump all journal files to disk
    void list_journal_log();                                                    ///< List available journal files with statistics

    // Parsing and utility methods
    void parser_journal_log(const std::string &filepath);                       ///< Parse journal file using systemd library
    void print_syslog(std::shared_ptr<journal_log> log_ptr);                   ///< Format and print single log entry
    bool find_journal_log(ulong inode, std::string &file);                     ///< Check if inode contains journal file
    void display_summary_statistics();                                          ///< Display summary statistics
    JournalFileInfo get_inode_file_info(ulong inode_addr);                     ///< Get detailed inode file information
    std::string get_journal_file_type(const std::string& filename);            ///< Determine journal file type from filename

public:
    /**
     * @brief Constructor with swap information
     * @param swap Shared pointer to swap information utility
     */
    Journal(std::shared_ptr<Swapinfo> swap);

    /**
     * @brief Default constructor
     */
    Journal();

    /**
     * @brief Initialize kernel structure field offsets
     */
    void init_offset(void) override;

    /**
     * @brief Initialize command help and usage information
     */
    void init_command(void) override;

    /**
     * @brief Main command entry point - handles command line arguments
     */
    void cmd_main(void) override;

    DEFINE_PLUGIN_INSTANCE(Journal)
};

#endif // JOURNAL_DEFS_H_
