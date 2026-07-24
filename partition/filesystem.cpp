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

#include "filesystem.h"
#include "logger/logger_core.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(FileSystem)
#endif

/**
 * @brief Main command handler for Mount base class (not used)
 */
void Mount::cmd_main(void) {}

/**
 * @brief Initialize structure offsets for Mount base class (not used)
 */
void Mount::init_offset(void) {}

/**
 * @brief Initialize command metadata for Mount base class (not used)
 */
void Mount::init_command(void) {}

/**
 * @brief Display filesystem statistics
 * @param width Column width for formatting the partition name
 *
 * Calculates and displays filesystem usage information including total size,
 * used space, available space, and usage percentage.
 */
void Mount::statfs(int width) {
    // Calculate filesystem statistics
    uint64_t total_size = static_cast<uint64_t>(f_bsize) * f_blocks;
    ulong used = f_bsize * (f_blocks - f_bfree);
    ulong avail = total_size - used;
    double ratio = (double)used / (double)total_size;

    // Format and print statistics
    std::ostringstream oss;
    oss << std::left << std::setw(width) << dir_name << " "
        << std::left << std::setw(5) << fs_type << " "
        << std::left << std::setw(10) << f_blocks << " "
        << std::left << std::setw(10) << csize(f_bsize) << " "
        << std::left << std::setw(10) << csize(total_size) << " "
        << std::left << std::setw(10) << csize(used) << " "
        << std::left << std::setw(10) << csize(avail) << " "
        << std::fixed << std::setprecision(2) << ratio * 100 << "%";
    PRINT("%s \n", oss.str().c_str());
}

/**
 * @brief Initialize F2FS structure field offsets
 *
 * Initializes all F2FS-specific kernel structure offsets needed for
 * parsing filesystem information.
 */
void F2fs::init_offset(void) {
    // Initialize f2fs_sb_info structure fields
    field_init(f2fs_sb_info, blocksize);
    field_init(f2fs_sb_info, user_block_count);
    field_init(f2fs_sb_info, total_valid_block_count);
    field_init(f2fs_sb_info, current_reserved_blocks);
    field_init(f2fs_sb_info, mount_opt);
    field_init(f2fs_sb_info, raw_super);

    // Initialize f2fs_mount_info structure fields
    field_init(f2fs_mount_info, root_reserved_blocks);

    // Initialize f2fs_super_block structure fields
    field_init(f2fs_super_block, block_count);
    field_init(f2fs_super_block, segment0_blkaddr);

    // Initialize structure sizes
    struct_init(f2fs_sb_info);
}

/**
 * @brief Initialize command metadata for F2FS (not used)
 */
void F2fs::init_command(void) {}

/**
 * @brief Constructor - initializes F2FS structure offsets
 */
F2fs::F2fs() {
    init_offset();
}

/**
 * @brief Display F2FS filesystem statistics
 * @param width Column width for formatting output
 *
 * Reads F2FS-specific structures from kernel memory and calculates
 * filesystem usage statistics.
 */
void F2fs::statfs(int width) {
    // Read f2fs_sb_info structure
    void *sbi = read_struct(fs_info_addr, "f2fs_sb_info");
    if (sbi == nullptr) {
        LOGE("Failed to read f2fs_sb_info at 0x%lx", fs_info_addr);
        return;
    }

    // Extract F2FS statistics
    f_bsize = UINT(sbi + field_offset(f2fs_sb_info, blocksize));
    uint32_t user_block_count = UINT(sbi + field_offset(f2fs_sb_info, user_block_count));
    uint32_t total_valid_block_count = UINT(sbi + field_offset(f2fs_sb_info, total_valid_block_count));
    uint32_t current_reserved_blocks = UINT(sbi + field_offset(f2fs_sb_info, current_reserved_blocks));

    // Calculate free and available blocks
    f_bfree = user_block_count - total_valid_block_count - current_reserved_blocks;
    f_bavail = f_bfree - UINT(sbi + field_offset(f2fs_sb_info, mount_opt) +
                               field_offset(f2fs_mount_info, root_reserved_blocks));

    // Read superblock information
    ulong raw_super_addr = ULONG(sbi + field_offset(f2fs_sb_info, raw_super));
    ulong total_count = read_ulong(raw_super_addr + field_offset(f2fs_super_block, block_count),
                                    "f2fs_super_block block_count");
    uint start_count = read_uint(raw_super_addr + field_offset(f2fs_super_block, segment0_blkaddr),
                                  "f2fs_super_block segment0_blkaddr");
    f_blocks = total_count - start_count;

    FREEBUF(sbi);
    // Display statistics using base class method
    Mount::statfs(width);
}

/**
 * @brief Constructor - initializes EXT4 structure offsets
 */
Ext4::Ext4() {
    init_offset();
}

/**
 * @brief Initialize EXT4 structure field offsets
 *
 * Initializes all EXT4-specific kernel structure offsets needed for
 * parsing filesystem information.
 */
void Ext4::init_offset(void) {
    // Initialize super_block structure fields
    field_init(super_block, s_blocksize);

    // Initialize ext4_sb_info structure fields
    field_init(ext4_sb_info, s_es);
    field_init(ext4_sb_info, s_overhead);
    field_init(ext4_sb_info, s_cluster_bits);
    field_init(ext4_sb_info, s_resv_clusters);
    field_init(ext4_sb_info, s_freeclusters_counter);
    field_init(ext4_sb_info, s_dirtyclusters_counter);

    // Initialize ext4_super_block structure fields
    field_init(ext4_super_block, s_blocks_count_hi);
    field_init(ext4_super_block, s_blocks_count_lo);
    field_init(ext4_super_block, s_r_blocks_count_hi);
    field_init(ext4_super_block, s_r_blocks_count_lo);

    // Initialize percpu_counter structure fields
    field_init(percpu_counter, count);
    field_init(percpu_counter, counters);

    // Initialize structure sizes
    struct_init(ext4_sb_info);
    struct_init(ext4_super_block);
}

/**
 * @brief Initialize command metadata for EXT4 (not used)
 */
void Ext4::init_command(void) {}

/**
 * @brief Display EXT4 filesystem statistics
 * @param width Column width for formatting output
 *
 * Reads EXT4-specific structures from kernel memory and calculates
 * filesystem usage statistics including per-CPU counter aggregation.
 */
void Ext4::statfs(int width) {
    // Read block size from super_block
    f_bsize = read_ulong(sb_addr + field_offset(super_block, s_blocksize), "super_block s_blocksize");

    // Read ext4_sb_info structure
    void *sbi = read_struct(fs_info_addr, "ext4_sb_info");
    if (sbi == nullptr) {
        LOGE("Failed to read ext4_sb_info at 0x%lx", fs_info_addr);
        return;
    }

    // Read ext4_super_block structure
    ulong esb_addr = ULONG(sbi + field_offset(ext4_sb_info, s_es));
    void *esb = read_struct(esb_addr, "ext4_super_block");
    if (esb == nullptr) {
        LOGE("Failed to read ext4_super_block at 0x%lx", esb_addr);
        FREEBUF(sbi);
        return;
    }

    // Extract EXT4 configuration
    ulong s_resv_clusters = ULONG(sbi + field_offset(ext4_sb_info, s_resv_clusters));
    uint s_cluster_bits = UINT(sbi + field_offset(ext4_sb_info, s_cluster_bits));
    ulong s_overhead = ULONG(sbi + field_offset(ext4_sb_info, s_overhead));

    // Calculate total block count (64-bit value)
    uint32_t s_blocks_count_hi = UINT(esb + field_offset(ext4_super_block, s_blocks_count_hi));
    uint32_t s_blocks_count_lo = UINT(esb + field_offset(ext4_super_block, s_blocks_count_lo));
    uint64_t ext4_blocks_count = (static_cast<uint64_t>(s_blocks_count_hi) << 32) | s_blocks_count_lo;

    // Calculate reserved block count (64-bit value)
    uint32_t s_r_blocks_count_hi = UINT(esb + field_offset(ext4_super_block, s_r_blocks_count_hi));
    uint32_t s_r_blocks_count_lo = UINT(esb + field_offset(ext4_super_block, s_r_blocks_count_lo));
    uint64_t ext4_r_blocks_count = (static_cast<uint64_t>(s_r_blocks_count_hi) << 32) | s_r_blocks_count_lo;

    // Calculate total blocks excluding overhead
    f_blocks = ext4_blocks_count - (s_overhead << s_cluster_bits);

    // Aggregate per-CPU counters for free and dirty clusters
    long long s_freeclusters_counter = percpu_counter_sum(fs_info_addr +
                                                           field_offset(ext4_sb_info, s_freeclusters_counter));
    long long s_dirtyclusters_counter = percpu_counter_sum(fs_info_addr +
                                                            field_offset(ext4_sb_info, s_dirtyclusters_counter));

    // Calculate free blocks
    f_bfree = (s_freeclusters_counter - s_dirtyclusters_counter) << s_cluster_bits;

    // Calculate available blocks for non-root users
    long long resv_blocks = s_resv_clusters << s_cluster_bits;
    if (f_bfree < (ext4_r_blocks_count + resv_blocks)) {
        f_bavail = 0;
    } else {
        f_bavail = f_bfree - (ext4_r_blocks_count + resv_blocks);
    }

    FREEBUF(sbi);
    FREEBUF(esb);

    // Display statistics using base class method
    Mount::statfs(width);
}

/**
 * @brief Sum per-CPU counter values
 * @param addr Address of percpu_counter structure
 * @return Total sum of all per-CPU counter values
 *
 * Aggregates counter values across all CPUs to get accurate total count.
 * This is necessary because EXT4 uses per-CPU counters for performance.
 */
long long Ext4::percpu_counter_sum(ulong addr) {
    // Read base count
    long long count = read_ulonglong(addr + field_offset(percpu_counter, count), "percpu_counter count");
    ulong counters = read_ulong(addr + field_offset(percpu_counter, counters), "percpu_counter counters");

    // Aggregate per-CPU values
    for (auto& per_cpu_counter_addr : for_each_percpu(counters)) {
        if (!is_kvaddr(per_cpu_counter_addr)) continue;
        int cpu_val = read_int(per_cpu_counter_addr, "per_cpu_counter");
        count += cpu_val;
    }
    return count;
}

/**
 * @brief Main command handler for filesystem plugin
 *
 * Processes command-line arguments and dispatches to appropriate display functions.
 * Supports options for displaying mount tree hierarchy and partition size information.
 */
void FileSystem::cmd_main(void) {
    int c;
    std::string cppString;

    // Validate minimum argument count
    if (argcnt < 2) {
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }

    // Parse mount tree if not already cached
    if (mount_list.size() == 0) {
        parser_mount_tree();
    }

    // Process command-line options
    while ((c = getopt(argcnt, args, "ts")) != EOF) {
        switch(c) {
            case 't':
                print_mount_tree(mount_list, 0);
                break;
            case 's':
                print_partition_size();
                break;
            default:
                argerrs++;
                break;
        }
    }

    if (argerrs) {
        cmd_usage(pc->curcmd, SYNOPSIS);
    }
}

/**
 * @brief Initialize kernel structure field offsets
 *
 * Initializes all kernel structure offsets needed for parsing mount information.
 */
void FileSystem::init_offset(void) {
    // Initialize nsproxy structure fields
    field_init(nsproxy, mnt_ns);

    // Initialize mnt_namespace structure fields
    field_init(mnt_namespace, root);
    field_init(mnt_namespace, list);

    // Initialize mount structure fields
    field_init(mount, mnt_devname);
    field_init(mount, mnt_child);
    field_init(mount, mnt_mounts);
    field_init(mount, mnt_mountpoint);
    field_init(mount, mnt);
    field_init(mount, mnt_list);

    // Initialize vfsmount structure fields
    field_init(vfsmount, mnt_sb);
    field_init(vfsmount, mnt_flags);

    // Initialize super_block structure fields
    field_init(super_block, s_type);
    field_init(super_block, s_fs_info);

    // Initialize file_system_type structure fields
    field_init(file_system_type, name);

    // Initialize structure sizes
    struct_init(mount);
}

/**
 * @brief Initialize command metadata and help information
 *
 * Sets up the command name, description, and comprehensive usage examples.
 */
void FileSystem::init_command(void) {
    cmd_name = "df";
    help_str_list = {
        "df",                                    /* command name */
        "display filesystem information",        /* short description */
        "[-t] [-s]\n"
        "  This command displays filesystem mount information.\n"
        "\n"
        "    -t    display mount info by tree hierarchy\n"
        "    -s    display partition size info\n",
        "\n",
        "EXAMPLES",
        "  Display mount info by tree hierarchy:",
        "    %s> df -t",
        "    [rootfs]rootfs --> /",
        "        [ext4]/dev/block/dm-4 --> /",
        "            [tmpfs]tmpfs --> /dev",
        "                [devpts]devpts --> /dev/pts",
        "\n",
        "  Display partition size info:",
        "    %s> df -s",
        "    Partition                 Type  Blocks     Block SZ   Size       Used       Avail      Use%",
        "    /                         ext4  124545     4.00Kb     486.50Mb   485.01Mb   0b         99.69%",
        "    /metadata                 ext4  2940       4.00Kb     11.48Mb    160.00Kb   10.86Mb    1.36%",
        "    /product                  ext4  32258      4.00Kb     126.01Mb   125.61Mb   0b         99.68%",
        "    /system_ext               ext4  31968      4.00Kb     124.88Mb   124.48Mb   0b         99.68%",
        "    data                      f2fs  1834496    4.00Kb     7.00Gb     468.12Mb   2.53Gb     6.53%",
        "    /vendor                   ext4  83917      4.00Kb     327.80Mb   326.80Mb   0b         99.70%",
        "\n",
    };
}

/**
 * @brief Constructor
 */
FileSystem::FileSystem() {

}

/**
 * @brief Print partition size information for all filesystems
 *
 * Displays a formatted table of filesystem usage statistics for all mounted
 * filesystems that have superblocks (unique filesystems).
 */
void FileSystem::print_partition_size() {
    // Find maximum partition name length for formatting
    size_t max_len = 0;
    for (const auto& pair : sb_list) {
        std::shared_ptr<Mount> mnt_ptr = pair.second;
        max_len = std::max(max_len, mnt_ptr->dir_name.size());
    }

    // Print table header
    std::ostringstream oss;
    oss << std::left << std::setw(max_len) << "Partition" << " "
        << std::left << std::setw(5) << "Type" << " "
        << std::left << std::setw(10) << "Blocks" << " "
        << std::left << std::setw(10) << "Block SZ" << " "
        << std::left << std::setw(10) << "Size" << " "
        << std::left << std::setw(10) << "Used" << " "
        << std::left << std::setw(10) << "Avail" << " "
        << std::left << "Use%";
    PRINT("%s \n", oss.str().c_str());

    // Print statistics for each filesystem
    for (const auto& pair : sb_list) {
        std::shared_ptr<Mount> mnt_ptr = pair.second;
        if (mnt_ptr->fs_type == "f2fs") {
            std::shared_ptr<F2fs> f2fs_ptr = std::dynamic_pointer_cast<F2fs>(mnt_ptr);
            f2fs_ptr->statfs(max_len);
        } else if (mnt_ptr->fs_type == "ext4") {
            std::shared_ptr<Ext4> ext4_ptr = std::dynamic_pointer_cast<Ext4>(mnt_ptr);
            ext4_ptr->statfs(max_len);
        }
    }
}

/**
 * @brief Print mount tree hierarchy recursively
 * @param mnt_list List of mount points to print
 * @param level Current indentation level
 *
 * Displays mount points in a tree format with proper indentation to show
 * the parent-child relationships between mount points.
 */
void FileSystem::print_mount_tree(std::vector<std::shared_ptr<Mount>>& mnt_list, int level) {
    for (const auto& mnt_ptr : mnt_list) {
        // Print indentation
        for (int i = 0; i < level; i++) {
            PRINT("\t");
        }

        // Print mount information
        PRINT("[%s]%s --> %s\n", mnt_ptr->fs_type.c_str(),
              mnt_ptr->dev_name.c_str(), mnt_ptr->dir_name.c_str());

        // Recursively print child mounts
        if (mnt_ptr->childs.size() > 0) {
            print_mount_tree(mnt_ptr->childs, (level + 1));
        }
    }
}

/**
 * @brief Parse the entire mount tree starting from init namespace
 *
 * Entry point for parsing all mount points in the system. Starts from the
 * init process's mount namespace and recursively parses all mount points.
 */
void FileSystem::parser_mount_tree() {
    // Check if init_nsproxy symbol exists
    if (!csymbol_exists("init_nsproxy")) {
        LOGE("init_nsproxy doesn't exist in this kernel!");
        return;
    }

    // Read init_nsproxy address
    ulong init_nsproxy_addr = csymbol_value("init_nsproxy");
    if (!is_kvaddr(init_nsproxy_addr)) {
        LOGE("Invalid init_nsproxy address: 0x%lx", init_nsproxy_addr);
        return;
    }

    // Read mount namespace address
    ulong mnt_namespace_addr = read_pointer(init_nsproxy_addr + field_offset(nsproxy, mnt_ns),
                                            "nsproxy mnt_ns");
    if (!is_kvaddr(mnt_namespace_addr)) {
        LOGE("Invalid mnt_namespace address: 0x%lx", mnt_namespace_addr);
        return;
    }

    // Read root mount address
    ulong root_mount_addr = read_pointer(mnt_namespace_addr + field_offset(mnt_namespace, root),
                                         "mnt_namespace root");
    if (!is_kvaddr(root_mount_addr)) {
        LOGE("Invalid root mount address: 0x%lx", root_mount_addr);
        return;
    }

    // Parse mount tree starting from root
    parser_mount_tree(root_mount_addr, mount_list);
}

/**
 * @brief Parse mount tree recursively from a given mount point
 * @param mount_addr Kernel address of mount structure
 * @param mnt_list List to store parsed mount points
 *
 * Recursively parses a mount point and all its children, building a tree
 * structure of mount points.
 */
void FileSystem::parser_mount_tree(ulong mount_addr, std::vector<std::shared_ptr<Mount>>& mnt_list) {
    // Parse the mount structure
    std::shared_ptr<Mount> mnt_ptr = parser_mount(mount_addr);
    if (mnt_ptr == nullptr) {
        LOGE("Failed to parse mount at 0x%lx", mount_addr);
        return;
    }

    // Set default directory name if empty
    if (mnt_ptr->dir_name.empty()) {
        mnt_ptr->dir_name = "None";
    }

    // Add to mount list
    mnt_list.push_back(mnt_ptr);

    // Parse child mounts
    int offset = field_offset(mount, mnt_child);
    ulong list_head = mount_addr + field_offset(mount, mnt_mounts);

    for (auto& addr : for_each_list(list_head, offset)) {
        parser_mount_tree(addr, mnt_ptr->childs);
    }
}

/**
 * @brief Parse a single mount point structure
 * @param mount_addr Kernel address of mount structure
 * @return Shared pointer to Mount object, or nullptr on failure
 *
 * Reads mount structure from kernel memory and creates appropriate
 * Mount-derived object (F2fs, Ext4, or base Mount) based on filesystem type.
 */
std::shared_ptr<Mount> FileSystem::parser_mount(ulong mount_addr) {
    char buf[BUFSIZE];
    LOGD("Parsing mount structure at 0x%lx", mount_addr);

    // Read device name
    ulong name_addr = read_pointer(mount_addr + field_offset(mount, mnt_devname), "mount mnt_devname");
    if (!is_kvaddr(name_addr)) {
        LOGE("Invalid device name address at mount 0x%lx", mount_addr);
        return nullptr;
    }
    std::string dev_name = read_cstring(name_addr, 64, "mnt_devname");

    // Read mount point path
    ulong mountpoint_addr = read_pointer(mount_addr + field_offset(mount, mnt_mountpoint),
                                         "mount mnt_mountpoint");
    if (!is_kvaddr(mountpoint_addr)) {
        LOGE("Invalid mountpoint address at mount 0x%lx", mount_addr);
        return nullptr;
    }

    ulong vfsmount_addr = mount_addr + field_offset(mount, mnt);
    if (!is_kvaddr(vfsmount_addr)) {
        LOGE("Invalid vfsmount address at mount 0x%lx", mount_addr);
        return nullptr;
    }

    get_pathname(mountpoint_addr, buf, BUFSIZE, 1, vfsmount_addr);
    std::string dir_name = buf;

    // Read superblock address
    ulong sb_addr = read_pointer(vfsmount_addr + field_offset(vfsmount, mnt_sb), "vfsmount mnt_sb");
    if (!is_kvaddr(sb_addr)) {
        LOGE("Invalid superblock address at mount 0x%lx", mount_addr);
        return nullptr;
    }

    // Read filesystem type
    ulong fs_addr = read_pointer(sb_addr + field_offset(super_block, s_type), "super_block s_type");
    if (!is_kvaddr(fs_addr)) {
        LOGE("Invalid filesystem type address at mount 0x%lx", mount_addr);
        return nullptr;
    }

    name_addr = read_pointer(fs_addr + field_offset(file_system_type, name), "file_system_type name");
    if (!is_kvaddr(name_addr)) {
        LOGE("Invalid filesystem name address at mount 0x%lx", mount_addr);
        return nullptr;
    }
    std::string fs_type = read_cstring(name_addr, 64, "file_system_type name");
    // Create appropriate Mount object based on filesystem type
    std::shared_ptr<Mount> mnt_ptr;
    if (fs_type == "f2fs") {
        mnt_ptr = std::make_shared<F2fs>();
    } else if (fs_type == "ext4") {
        mnt_ptr = std::make_shared<Ext4>();
    } else {
        mnt_ptr = std::make_shared<Mount>();
    }

    // Populate mount object
    mnt_ptr->addr = mount_addr;
    mnt_ptr->dir_name = dir_name;
    mnt_ptr->dev_name = dev_name;
    mnt_ptr->sb_addr = sb_addr;
    mnt_ptr->fs_addr = fs_addr;
    mnt_ptr->fs_type = fs_type;
    mnt_ptr->mnt_flags = read_int(vfsmount_addr + field_offset(vfsmount, mnt_flags), "vfsmount mnt_flags");
    mnt_ptr->fs_info_addr = read_pointer(sb_addr + field_offset(super_block, s_fs_info), "super_block s_fs_info");

    // Add to superblock list if not already present
    if (sb_list.find(mnt_ptr->sb_addr) == sb_list.end()) {
        sb_list[mnt_ptr->sb_addr] = mnt_ptr;
    }
    return mnt_ptr;
}

#pragma GCC diagnostic pop
