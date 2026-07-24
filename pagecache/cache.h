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

#ifndef CACHE_DEFS_H_
#define CACHE_DEFS_H_

#include "plugin.h"

/**
 * @brief Structure representing cached file information
 *
 * Contains information about a file that is cached in the kernel's page cache,
 * including inode details, file path, and caching statistics.
 */
struct FileCache {
    ulong inode;            ///< Kernel address of the inode structure
    std::string name;       ///< Full path of the cached file
    ulong i_mapping;        ///< Address of the address_space structure for page cache management
    ulong nrpages;          ///< Number of pages currently cached for this file
};

/**
 * @brief Page cache analyzer plugin for crash utility
 *
 * This plugin provides functionality to analyze the kernel's page cache from
 * crash dumps. It can list cached files, display file information, dump file
 * contents, and analyze anonymous pages.
 *
 * Main features:
 * - List files in directories with detailed information
 * - Display page cache statistics for all cached files
 * - Dump cached file contents to disk for analysis
 * - Show anonymous pages information
 * - Analyze file permissions and metadata
 */
class Cache : public ParserPlugin {
private:
    std::vector<std::shared_ptr<FileCache>> cache_list;         ///< List of cached files discovered in the system

    // Core functionality methods
    void parser_file_pages();                                   ///< Parse and collect information about all cached files
    void list_files(std::string& path);                        ///< List files in a directory with detailed information
    void dump_files(std::string& path);                        ///< Dump file or directory contents to disk
    void print_file_pages();                                   ///< Display statistics of all cached files
    void print_anon_pages();                                   ///< Display information about anonymous pages

    // Utility and formatting methods
    std::string format_file_info(ulong dentry_addr, ulong inode_addr, const std::string& filename);  ///< Format file information for display
    std::string format_permissions(mode_t mode);               ///< Convert mode bits to permission string (e.g., "rwxr-xr-x")
    void dump_regular_file(const std::string& full_path, const std::string& name);  ///< Dump a single regular file
    void dump_directory(const std::string& path);              ///< Recursively dump all files in a directory
    void print_file_info(ulong dentry_addr);                   ///< Print detailed information for a single file

public:
    /**
     * @brief Default constructor
     */
    Cache();

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

    DEFINE_PLUGIN_INSTANCE(Cache)
};

#endif // CACHE_DEFS_H_
