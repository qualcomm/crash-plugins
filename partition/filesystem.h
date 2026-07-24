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

#ifndef FILESYSTEM_DEFS_H_
#define FILESYSTEM_DEFS_H_

#include "plugin.h"

/**
 * @class Mount
 * @brief Base class for filesystem mount point information
 *
 * Represents a mounted filesystem with its properties including device name,
 * mount point, filesystem type, and space usage statistics.
 */
class Mount : public ParserPlugin{
public:
    ulong addr;                                      // Address of mount structure in kernel memory
    ulong f_bsize;                                   // Filesystem block size
    ulong f_blocks;                                  // Total number of blocks
    ulong f_bfree;                                   // Number of free blocks
    ulong f_bavail;                                  // Number of available blocks for non-root users
    std::string dev_name;                            // Device name (e.g., /dev/sda1)
    std::string dir_name;                            // Mount point directory path
    std::string fs_type;                             // Filesystem type (e.g., ext4, f2fs)
    ulong sb_addr;                                   // Address of super_block structure
    ulong fs_addr;                                   // Address of file_system_type structure
    ulong fs_info_addr;                              // Address of filesystem-specific info structure
    int mnt_flags;                                   // Mount flags
    std::vector<std::shared_ptr<Mount>> childs;     // Child mount points

    /**
     * @brief Main command handler (not used in base class)
     */
    void cmd_main(void) override;

    /**
     * @brief Initialize structure field offsets (not used in base class)
     */
    void init_offset(void) override;

    /**
     * @brief Initialize command metadata (not used in base class)
     */
    void init_command(void) override;

    /**
     * @brief Display filesystem statistics
     * @param width Column width for formatting output
     *
     * Prints filesystem usage information including total size, used space,
     * available space, and usage percentage.
     */
    virtual void statfs(int width);
};

/**
 * @class F2fs
 * @brief F2FS (Flash-Friendly File System) specific mount information
 *
 * Extends Mount class to handle F2FS-specific filesystem statistics
 * and structure parsing.
 */
class F2fs: public Mount {
public:
    /**
     * @brief Constructor - initializes F2FS structure offsets
     */
    F2fs();

    /**
     * @brief Display F2FS filesystem statistics
     * @param width Column width for formatting output
     */
    void statfs(int width) override;

    /**
     * @brief Initialize F2FS structure field offsets
     */
    void init_offset(void) override;

    /**
     * @brief Initialize command metadata (not used)
     */
    void init_command(void) override;
};

/**
 * @class Ext4
 * @brief EXT4 filesystem specific mount information
 *
 * Extends Mount class to handle EXT4-specific filesystem statistics
 * and structure parsing, including per-CPU counter aggregation.
 */
class Ext4 : public Mount {
public:
    /**
     * @brief Constructor - initializes EXT4 structure offsets
     */
    Ext4();

    /**
     * @brief Display EXT4 filesystem statistics
     * @param width Column width for formatting output
     */
    void statfs(int width) override;

    /**
     * @brief Initialize EXT4 structure field offsets
     */
    void init_offset(void) override;

    /**
     * @brief Initialize command metadata (not used)
     */
    void init_command(void) override;

private:
    /**
     * @brief Sum per-CPU counter values
     * @param addr Address of percpu_counter structure
     * @return Total sum of all per-CPU counter values
     *
     * Aggregates counter values across all CPUs for accurate statistics.
     */
    long long percpu_counter_sum(ulong addr);
};

/**
 * @class FileSystem
 * @brief Plugin for analyzing filesystem mount points and usage
 *
 * Provides commands to display mount hierarchy and partition size information
 * for various filesystem types including EXT4 and F2FS.
 */
class FileSystem : public ParserPlugin {
private:
    std::vector<std::shared_ptr<Mount>> mount_list;                    // Root-level mount points
    std::unordered_map<size_t, std::shared_ptr<Mount>> sb_list;        // Map of superblocks to mounts

    /**
     * @brief Parse a single mount point structure
     * @param mount_addr Kernel address of mount structure
     * @return Shared pointer to Mount object, or nullptr on failure
     *
     * Reads mount structure from kernel memory and creates appropriate
     * Mount-derived object based on filesystem type.
     */
    std::shared_ptr<Mount> parser_mount(ulong mount_addr);

    /**
     * @brief Print mount tree hierarchy recursively
     * @param mnt_list List of mount points to print
     * @param level Current indentation level
     *
     * Displays mount points in tree format with proper indentation.
     */
    void print_mount_tree(std::vector<std::shared_ptr<Mount>>& mnt_list, int level);

    /**
     * @brief Print partition size information for all filesystems
     *
     * Displays a table of filesystem usage statistics including size,
     * used space, available space, and usage percentage.
     */
    void print_partition_size(void);

    /**
     * @brief Parse the entire mount tree starting from init namespace
     *
     * Entry point for parsing all mount points in the system.
     */
    void parser_mount_tree(void);

    /**
     * @brief Parse mount tree recursively from a given mount point
     * @param mount_addr Kernel address of mount structure
     * @param mnt_list List to store parsed mount points
     *
     * Recursively parses mount points and their children.
     */
    void parser_mount_tree(ulong mount_addr, std::vector<std::shared_ptr<Mount>>& mnt_list);

public:
    /**
     * @brief Constructor
     */
    FileSystem();

    /**
     * @brief Main command handler
     *
     * Processes command-line arguments and dispatches to appropriate functions.
     */
    void cmd_main(void) override;

    /**
     * @brief Initialize structure field offsets
     *
     * Initializes all kernel structure offsets needed for parsing mount information.
     */
    void init_offset(void) override;

    /**
     * @brief Initialize command metadata
     *
     * Sets up command name, help text, and usage examples.
     */
    void init_command(void) override;

    DEFINE_PLUGIN_INSTANCE(FileSystem)
};

#endif // FILESYSTEM_DEFS_H_
