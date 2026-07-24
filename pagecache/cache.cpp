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

#include "cache.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(Cache)
#endif

/**
 * @brief Main command entry point for page cache analysis
 *
 * Parses command line arguments and dispatches to appropriate handler functions:
 * -l <path>: List files in directory with detailed information
 * -d <path>: Dump file or directory contents to disk
 * -f: Display file page cache statistics
 * -a: Display anonymous pages information
 */
void Cache::cmd_main(void) {
    int c;
    std::string cppString;
    if (argcnt < 2) {
        LOGD("Insufficient arguments provided, showing usage\n");
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }
    // Parse command line options
    while ((c = getopt(argcnt, args, "afl:d:")) != EOF) {
        switch(c) {
            case 'l':
                cppString.assign(optarg);
                LOGD("Executing list_files() for path: %s\n", cppString.c_str());
                list_files(cppString);
                break;
            case 'd':
                cppString.assign(optarg);
                LOGD("Executing dump_files() for path: %s\n", cppString.c_str());
                dump_files(cppString);
                break;
            case 'f':
                LOGD("Executing print_file_pages()\n");
                print_file_pages();
                break;
            case 'a':
                LOGD("Executing print_anon_pages()\n");
                print_anon_pages();
                break;
            default:
                LOGD("Unknown option: -%c\n", c);
                argerrs++;
                break;
        }
    }
    // Handle argument errors
    if (argerrs) {
        LOGE("Command line argument errors detected: %d\n", argerrs);
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }
}

/**
 * @brief Initialize kernel structure field offsets
 *
 * Sets up field offsets for inode structure fields used in page cache analysis.
 * These offsets are essential for reading inode data from kernel memory.
 */
void Cache::init_offset(void) {
    // Initialize inode structure field offsets
    field_init(inode,i_uid);        // User ID of file owner
    field_init(inode,i_gid);        // Group ID of file owner
    field_init(inode,i_size);       // File size in bytes
    field_init(inode,i_mtime);      // Last modification time
    field_init(inode,i_mapping);    // Address space for page cache
    field_init(inode,i_dentry);     // Dentry list head
    field_init(inode,i_sb);         // Superblock pointer
    field_init(inode,i_mode);       // File mode and permissions
    field_init(inode,i_nlink);      // Number of hard links
    field_init(inode,i_link);       // Symbolic link target
}

/**
 * @brief Initialize command help and usage information
 *
 * Sets up the command name, description, and detailed help text including
 * usage examples and expected output formats for the page cache plugin.
 */
void Cache::init_command(void) {
    cmd_name = "cache";
    help_str_list={
        "cache",                                   /* command name */
        "dump page cache information",            /* short description */
        "[-f] [-a] [-l path] [-d path]\n"
        "  This command dumps page cache information including file cache and anonymous pages.\n"
        "\n"
        "    -f          display file page cache statistics\n"
        "    -a          display anonymous pages information\n"
        "    -l path     list files in directory with detailed information\n"
        "    -d path     dump file or directory contents to disk\n",
        "\n",
        "EXAMPLES",
        "  Display file page cache statistics:",
        "    %s> cache -f",
        "    Total File cache size: 570.22MB",
        "    ===============================================",
        "    inode            address_space    nrpages  size       Path",
        "    ffffff805c413178 ffffff805c413340 16220    63.36MB    /app/webview/webview.apk",
        "    ffffff805c4e6128 ffffff805c4e62f0 8768     34.25MB    /priv-app/Settings/Settings.apk",
        "    ffffff8032903848 ffffff8032903a10 8590     33.55MB    /system/framework/framework.jar",
        "    ffffff803d3daaa8 ffffff803d3dac70 4209     16.44MB    /system/framework/services.jar",
        "\n",
        "  Display page cache of inode:",
        "    %s> files -p ffffff805c413178",
        "           INODE        NRPAGES",
        "    ffffff805c413178    16220",
        "    ",
        "          PAGE       PHYSICAL      MAPPING       INDEX CNT FLAGS",
        "    fffffffe00bd2000 6f480000 ffffff805c413340        0  3 10000000020014 uptodate,lru,mappedtodisk",
        "    fffffffe00630ec0 58c3b000 ffffff805c413340        2  3 10000000020014 uptodate,lru,mappedtodisk",
        "    fffffffe00bc6d80 6f1b6000 ffffff805c413340        3  3 10000000020014 uptodate,lru,mappedtodisk",
        "    fffffffe012b4cc0 8ad33000 ffffff805c413340        4  3 10000000020014 uptodate,lru,mappedtodisk",
        "\n",
        "  Display anon pages:",
        "    %s> cache -a",
        "    page:0xfffffffe0007f300  paddr:0x41fcc000",
        "    page:0xfffffffe0007f440  paddr:0x41fd1000",
        "    page:0xfffffffe0007f4c0  paddr:0x41fd3000",
        "\n",
        "  List files in directory with detailed information:",
        "    %s> cache -l /",
        "    DENTRY             INODE              PERMISSIONS      UID      GID     SIZE   NRPAGE TIME         PATH",
        "    0xffffff804614cb40 0xffffff8056480898 lrw-r--r--         0        0       21        0 Jan 01 08:00 sdcard -> /storage/self/primary",
        "    0xffffff804f5b9b40 0xffffff805647c5e8 drwxr-xr-x         0        0     4096        0 Jan 01 08:00 odm/",
        "    ------------------ ------------------ -----------        0        0        0        0 ------------ CalendarProvider.apk",
        "    0xffffff8009b64f00 0xffffff80564feec8 lrw-r--r--         0        0       11        0 Jan 01 08:00 etc -> /system/etc",
        "    0xffffff8030d1dd20 0xffffff802fa7e128 drwxr-xr-x         0        0     4096        0 Jan 01 08:00 data_mirror/",
        "    0xffffff802935d000 0xffffff802bbd4cb8 drwxrwx--x      1000     1000     4096        0 Jan 01 08:00 data/",
        "    0xffffff8029357f00 0xffffff80193a7598 drwxr-x--x         0     1028     4096        0 Jan 01 08:00 storage/",
        "    0xffffff8024ccd1e0 0xffffff80193a01c8 dr-xr-xr-x         0        0     4096        0 Jan 01 08:00 config/",
        "    0xffffff8013dcb2d0 0xffffff8013e023d8 drwxr-xr-x         0        0     4096        0 Jan 01 08:00 apex/",
        "\n",
        "  Display the file content of special file:",
        "    %s> cache -d /data/logcat.log",
        "    Dumping file: /data/logcat.log -> xx/data/logcat.log (size: 8285809 bytes, pages: 141)",
        "    Dump completed for /data/logcat.log:",
        "       - Pages processed: 141",
        "       - Pages written: 141",
        "       - Pages excluded: 0",
        "       - Success rate: 100.0%",
        "\n",
    };
}

/**
 * @brief Display information about anonymous pages
 *
 * Iterates through all anonymous pages in the system and displays their
 * virtual and physical addresses. Anonymous pages are not backed by files
 * and are typically used for process heap, stack, and other dynamic memory.
 */
void Cache::print_anon_pages(){
    LOGD("Starting anonymous pages analysis\n");
    size_t anon_page_count = 0;
    // Iterate through all anonymous pages in the system
    for (const auto& page : for_each_anon_page()) {
        physaddr_t paddr = page_to_phy(page);
        PRINT("page:%#lx  paddr:%#llx \n", page, (ulonglong)paddr);
        anon_page_count++;
        // Log progress for large numbers of pages
        if (anon_page_count % 1000 == 0) {
            LOGD("Processed %zu anonymous pages\n", anon_page_count);
        }
    }
    LOGD("Anonymous pages analysis completed: %zu pages processed\n", anon_page_count);
}

/**
 * @brief Parse and collect information about all cached files
 *
 * Scans through all inodes in the system to build a comprehensive list
 * of files that have pages cached in memory. For each inode, it extracts
 * the file path, mapping information, and page count.
 */
void Cache::parser_file_pages(){
    LOGD("Starting file pages parsing\n");
    // Clear existing cache list and prepare for new data
    cache_list.clear();
    size_t inodes_processed = 0;
    size_t files_with_cache = 0;
    // Scan all inodes in the system
    for (const auto& addr : for_each_inode()) {
        inodes_processed++;
        // Create new FileCache entry
        std::shared_ptr<FileCache> file_ptr = std::make_shared<FileCache>();
        file_ptr->inode = addr;
        // Get file path from dentry list
        ulong hlist_head = addr + field_offset(inode,i_dentry);
        int offset = field_offset(dentry,d_u);
        for (const auto& dentry : for_each_hlist(hlist_head,offset)) {
            file_ptr->name = get_dentry_path(dentry);
            if (!file_ptr->name.empty()){
                break;
            }
        }
        if (!file_ptr->name.empty()){
            LOGD("Found file path: %s for inode 0x%lx\n", file_ptr->name.c_str(), addr);
        }else{
            file_ptr->name = "unknown";
        }
        // Get mapping and page count information
        file_ptr->i_mapping = read_pointer(addr + field_offset(inode,i_mapping),"i_mapping");
        if (is_kvaddr(file_ptr->i_mapping)) {
            file_ptr->nrpages = read_ulong(file_ptr->i_mapping + field_offset(address_space, nrpages), "nrpages");
            // Only add files that have cached pages
            if (file_ptr->nrpages > 0) {
                cache_list.push_back(file_ptr);
                files_with_cache++;
                LOGD("Added cached file: %s (%lu pages)\n", file_ptr->name.c_str(), file_ptr->nrpages);
            }
        }
        // Log progress for large numbers of inodes
        if (inodes_processed % 1000 == 0) {
            LOGD("Processed %zu inodes, found %zu files with cache\n", inodes_processed, files_with_cache);
        }
    }
    LOGD("File pages parsing completed: %zu inodes processed, %zu cached files found\n", inodes_processed, files_with_cache);
}

/**
 * @brief Dump file or directory contents to disk
 *
 * Analyzes the specified path and determines whether it's a file or directory,
 * then calls the appropriate dump function. For directories, it recursively
 * dumps all contained files. For regular files, it extracts the cached content.
 *
 * @param path File or directory path to dump
 */
void Cache::dump_files(std::string& path){
    LOGD("Starting dump operation for path: %s\n", path.c_str());
    // Validate input path
    if (path.empty()){
        LOGE("Empty path provided for dump operation\n");
        return;
    }
    // Normalize and resolve path
    normalize_path(path);
    LOGD("Normalized path: %s\n", path.c_str());
    ulong dentry = path_to_dentry(path);
    if (!is_kvaddr(dentry)){
        LOGE("Failed to find dentry for path: %s (dentry: 0x%lx)\n", path.c_str(), dentry);
        return;
    }
    LOGD("Found dentry: 0x%lx for path: %s\n", dentry, path.c_str());
    ulong inode_addr = get_inode(dentry);
    if (!is_kvaddr(inode_addr)){
        LOGE("Failed to get inode for dentry: 0x%lx\n", dentry);
        return;
    }
    LOGD("Found inode: 0x%lx for dentry: 0x%lx\n", inode_addr, dentry);
    // Get file name and mode
    std::string name = get_dentry_name(dentry);
    uint i_mode = 0;
    // Read file mode (handle different kernel versions)
    if(field_size(inode,i_mode) == sizeof(int)){
        i_mode = read_ushort(inode_addr + field_offset(inode,i_mode), "i_mode");
    }else{
        i_mode = read_uint(inode_addr + field_offset(inode,i_mode), "i_mode");
    }
    LOGD("File mode: 0x%x, name: %s\n", i_mode, name.c_str());
    // Dispatch based on file type
    if (S_ISDIR(i_mode)){
        LOGD("Path is directory, starting recursive dump\n");
        dump_directory(path);
    } else if (S_ISREG(i_mode)) {
        LOGD("Path is regular file, starting file dump\n");
        dump_regular_file(path, name);
    } else {
        LOGD("Path is neither directory nor regular file (mode: 0x%x)\n", i_mode);
    }
    LOGD("Dump operation completed for path: %s\n", path.c_str());
}

/**
 * @brief Recursively dump all files in a directory
 *
 * Traverses a directory structure and dumps all regular files found within.
 * Handles subdirectories recursively to ensure complete directory dumping.
 *
 * @param path Directory path to dump
 */
void Cache::dump_directory(const std::string& path) {
    LOGD("Cache::dump_directory() - Starting directory dump for: %s\n", path.c_str());

    // Convert path to dentry
    ulong dentry = path_to_dentry(path);
    if (!is_kvaddr(dentry)){
        LOGE("Invalid dentry address: 0x%lx for path: %s\n", dentry, path.c_str());
        return;
    }

    LOGD("Found valid dentry: 0x%lx for directory: %s\n", dentry, path.c_str());

    // Get list of subdirectories/files
    std::vector<ulong> subdirs_list = for_each_subdirs(dentry);
    LOGD("Found %zu entries in directory: %s\n", subdirs_list.size(), path.c_str());

    if (subdirs_list.empty()) {
        LOGD("Directory is empty: %s\n", path.c_str());
        return;
    }

    size_t processed_entries = 0;
    size_t skipped_entries = 0;
    size_t directories_found = 0;
    size_t files_found = 0;

    // Process each entry in the directory
    for (const auto& sub_dentry : subdirs_list) {
        processed_entries++;

        // Get entry name
        std::string name = get_dentry_name(sub_dentry);
        LOGD("Processing entry %zu: dentry=0x%lx, name='%s'\n",
             processed_entries, sub_dentry, name.c_str());

        // Skip special entries
        if (name.empty() || name == "." || name == "..") {
            LOGD("Skipping special entry: '%s'\n", name.c_str());
            skipped_entries++;
            continue;
        }

        // Get inode for this entry
        ulong sub_inode = get_inode(sub_dentry);
        if (!is_kvaddr(sub_inode)) {
            LOGD("Invalid inode address: 0x%lx for entry: %s\n", sub_inode, name.c_str());
            skipped_entries++;
            continue;
        }

        LOGD("Found valid inode: 0x%lx for entry: %s\n", sub_inode, name.c_str());

        // Read file mode to determine type
        uint sub_mode = 0;
        if(field_size(inode,i_mode) == sizeof(int)){
            sub_mode = read_ushort(sub_inode + field_offset(inode,i_mode), "i_mode");
        }else{
            sub_mode = read_uint(sub_inode + field_offset(inode,i_mode), "i_mode");
        }

        LOGD("Entry mode: 0x%x for %s\n", sub_mode, name.c_str());

        // Process based on file type
        if (S_ISDIR(sub_mode)) {
            directories_found++;
            LOGD("Entry is directory: %s, recursing into subdirectory\n", name.c_str());
            dump_directory(path + "/" + name);
        } else if (S_ISREG(sub_mode)) {
            files_found++;
            LOGD("Entry is regular file: %s, dumping file\n", name.c_str());
            dump_regular_file(path, name);
        } else {
            LOGD("Entry is special file type (mode: 0x%x): %s, skipping\n", sub_mode, name.c_str());
            skipped_entries++;
        }

        // Log progress for large directories
        if (processed_entries % 50 == 0) {
            LOGD("Directory processing progress: %zu/%zu entries processed\n",
                 processed_entries, subdirs_list.size());
        }
    }

    LOGD("Cache::dump_directory() - Completed directory dump for: %s\n", path.c_str());
    LOGD("  - Total entries: %zu\n", subdirs_list.size());
    LOGD("  - Processed entries: %zu\n", processed_entries);
    LOGD("  - Skipped entries: %zu\n", skipped_entries);
    LOGD("  - Directories found: %zu\n", directories_found);
    LOGD("  - Files found: %zu\n", files_found);
}

void Cache::dump_regular_file(const std::string& full_path, const std::string& name) {
    LOGD("dump_regular_file: %s\n", full_path.c_str());
    if (full_path.empty()) {
        LOGE("Error: Empty file path provided\n");
        return;
    }
    if (full_path[0] != '/') {
        LOGE("Error: Path must be absolute (start with /): %s\n", full_path.c_str());
        return;
    }
    ulong dentry = path_to_dentry(full_path);
    if (!is_kvaddr(dentry)){
        LOGE("Error: Failed to find dentry for path '%s' (dentry: %#lx)\n",
                full_path.c_str(), dentry);
        return;
    }
    LOGD("Found dentry: %#lx for file: %s\n", dentry, full_path.c_str());
    size_t pos = full_path.find_last_of("/\\");
    std::string dirPath = (pos == std::string::npos) ? "" : full_path.substr(0, pos);
    dirPath = get_curpath().str() + "/cache/" + dirPath;
    ulong inode_addr = get_inode(dentry);
    write_pagecache_to_file(inode_addr, name, dirPath, true);
}

/**
 * @brief List files in a directory with detailed information
 *
 * Displays comprehensive information about files in the specified directory,
 * including dentry address, inode address, permissions, ownership, size,
 * cached pages, modification time, and file path.
 *
 * @param path Directory path to list
 */
void Cache::list_files(std::string& path){
    LOGD("Starting file listing for path: %s\n", path.c_str());
    // Validate input path
    if (path.empty()){
        PRINT("Please input the path\n");
        return;
    }
    // Normalize path
    normalize_path(path);
    LOGD("Normalized path: %s\n", path.c_str());
    // Convert path to dentry
    ulong dentry = path_to_dentry(path);
    if (!is_kvaddr(dentry)){
        LOGE("Failed to find dentry for path: %s (dentry: 0x%lx)\n", path.c_str(), dentry);
        return;
    }
    LOGD("Found valid dentry: 0x%lx for path: %s\n", dentry, path.c_str());
    // Get subdirectories/files list
    std::vector<ulong> subdirs_list = for_each_subdirs(dentry);
    LOGD("Found %zu entries in directory: %s\n", subdirs_list.size(), path.c_str());
    if(subdirs_list.size() > 0){
        LOGD("Directory contains %zu entries, displaying detailed listing\n", subdirs_list.size());
        // Print header
        PRINT("%-18s %-18s %-11s %8s %8s %8s %8s %-12s %s\n",
              "DENTRY", "INODE", "PERMISSIONS", "UID", "GID", "SIZE", "NRPAGE", "TIME", "PATH");
        size_t processed_entries = 0;
        size_t valid_entries = 0;
        size_t invalid_entries = 0;
        // Process each entry
        for (const auto& sub_dentry : subdirs_list) {
            processed_entries++;
            LOGD("Processing directory entry %zu: dentry=0x%lx\n", processed_entries, sub_dentry);
            // Get entry name for validation
            std::string entry_name = get_dentry_name(sub_dentry);
            if (entry_name.empty()) {
                LOGE("Entry %zu has empty name, skipping\n", processed_entries);
                invalid_entries++;
                continue;
            }
            LOGD("Entry[%zu] name: '%s'\n", processed_entries, entry_name.c_str());
            // Check if entry has valid inode
            ulong entry_inode = get_inode(sub_dentry);
            if (is_kvaddr(entry_inode)) {
                LOGD("%s has valid inode: 0x%lx\n", entry_name.c_str(), entry_inode);
                valid_entries++;
            } else {
                LOGE("%s has invalid inode: 0x%lx\n", entry_name.c_str(), entry_inode);
                invalid_entries++;
            }
            // Print file information
            print_file_info(sub_dentry);
            // Log progress for large directories
            if (processed_entries % 25 == 0) {
                LOGD("Directory listing progress: %zu/%zu entries processed\n", processed_entries, subdirs_list.size());
            }
        }
        LOGD("Directory listing completed for: %s\n", path.c_str());
        LOGD("  - Total entries: %zu\n", subdirs_list.size());
        LOGD("  - Processed entries: %zu\n", processed_entries);
        LOGD("  - Valid entries: %zu\n", valid_entries);
        LOGD("  - Invalid entries: %zu\n", invalid_entries);
    } else {
        LOGD("Path appears to be a single file, displaying file info\n");
        print_file_info(dentry);
    }
}

/**
 * @brief Print detailed information for a single file
 *
 * Extracts and displays comprehensive file information including dentry address,
 * inode address, permissions, ownership, size, cached pages, and modification time.
 * Handles cases where inode information is not available.
 *
 * @param dentry_addr Kernel address of the dentry structure
 */
void Cache::print_file_info(ulong dentry_addr) {
    LOGD("Processing dentry: 0x%lx\n", dentry_addr);
    // Get dentry name
    std::string dentry_name = get_dentry_name(dentry_addr);
    LOGD("Dentry name: '%s' for dentry: 0x%lx\n", dentry_name.c_str(), dentry_addr);
    if(!dentry_name.empty()){
        // Get inode address
        ulong inode_addr = get_inode(dentry_addr);
        LOGD("Retrieved inode address: 0x%lx for dentry: 0x%lx\n", inode_addr, dentry_addr);
        if (is_kvaddr(inode_addr)){
            // Format and display complete file information
            std::string file_info = format_file_info(dentry_addr, inode_addr, dentry_name);
            PRINT("%s\n", file_info.c_str());
            LOGD("Successfully displayed file info for: %s\n", dentry_name.c_str());
        } else {
            LOGD("Invalid inode address: 0x%lx \n", inode_addr);
            // Display placeholder information for invalid inode
            std::ostringstream oss;
            oss << std::left << "--" << std::setw(16) << std::setfill('-') << ""
                << " " << "--" << std::setw(16) << std::setfill('-') << ""
                << " " << std::left << std::setw(11) << "" << std::setfill('-')
                << " " << std::right << std::setw(8) << std::setfill(' ') << 0
                << " " << std::setw(8) << 0
                << " " << std::setw(8) << 0
                << " " << std::setw(8) << 0
                << " " << std::left << std::setw(12) << std::setfill('-') << ""
                << " " << dentry_name;

            PRINT("%s \n", oss.str().c_str());
        }
    } else {
        LOGD("Empty dentry name for dentry: 0x%lx, skipping\n", dentry_addr);
    }
}

/**
 * @brief Format comprehensive file information for display
 *
 * Reads inode data and formats it into a human-readable string containing
 * dentry address, inode address, permissions, ownership, size, cached pages,
 * modification time, and file path with appropriate type indicators.
 *
 * @param dentry_addr Kernel address of the dentry structure
 * @param inode_addr Kernel address of the inode structure
 * @param filename Name of the file
 * @return Formatted string containing file information
 */
std::string Cache::format_file_info(ulong dentry_addr, ulong inode_addr, const std::string& filename) {
    // Read inode structure from kernel memory
    void *inode_buf = read_struct(inode_addr,"inode");
    if (!inode_buf) {
        LOGE("Failed to read inode structure at address: 0x%lx\n", inode_addr);
        return filename + " (inode read failed)";
    }
    // Extract file mode
    uint mode = 0;
    if(field_size(inode,i_mode) == sizeof(int)){
        mode = USHORT(inode_buf + field_offset(inode,i_mode));
    }else{
        mode = UINT(inode_buf + field_offset(inode,i_mode));
    }
    // Format permissions string
    std::string permissions = format_permissions(mode);
    LOGD("Formatted permissions: %s\n", permissions.c_str());
    // Extract file attributes
    ulong i_link = ULONG(inode_buf + field_offset(inode,i_link));
    uint32_t i_uid = UINT(inode_buf + field_offset(inode,i_uid));
    uint32_t i_gid = UINT(inode_buf + field_offset(inode,i_gid));
    long long i_size = ULONGLONG(inode_buf + field_offset(inode,i_size));
    ulong i_mapping = ULONG(inode_buf + field_offset(inode,i_mapping));
    // Format timestamp
    time_t timestamp = ULONGLONG(inode_buf + field_offset(inode,i_mtime));
    LOGD("File attributes extracted:\n");
    LOGD("  - UID: %u, GID: %u\n", i_uid, i_gid);
    LOGD("  - Size: %lld bytes\n", i_size);
    LOGD("  - Mapping address: 0x%lx\n", i_mapping);
    LOGD("  - Link address: 0x%lx\n", i_link);
    LOGD("  - Timestamp: 0x%lx\n", timestamp);
    // Get cached pages count
    ulong nrpages = 0;
    if (is_kvaddr(i_mapping)){
        nrpages = read_ulong(i_mapping + field_offset(address_space, nrpages), "nrpages");
        LOGD("Cached pages: %lu\n", nrpages);
    } else {
        LOGD("Invalid mapping address, no cached pages info\n");
    }
    FREEBUF(inode_buf);

    struct tm* tm_info = localtime(&timestamp);
    std::ostringstream oss;
    char time_str[32];
    if (tm_info) {
        strftime(time_str, sizeof(time_str), "%b %d %H:%M", tm_info);
    } else {
        strcpy(time_str, "Invalid Time");
        LOGD("Invalid timestamp, using placeholder\n");
    }
    // Build formatted output string
    oss << std::left << std::hex << "0x" << std::setfill('0') << std::setw(16) << dentry_addr
        << " " << "0x" << std::setw(16) << inode_addr << std::dec << std::setfill(' ')
        << " " << std::left << std::setw(11) << permissions
        << " " << std::right << std::setw(8) << i_uid
        << " " << std::setw(8) << i_gid
        << " " << std::setw(8) << i_size
        << " " << std::setw(8) << nrpages
        << " " << std::left << std::setw(12) << time_str
        << " " << filename;
    // Add file type indicators
    if (S_ISREG(mode)) {
        if (mode & (S_IXUSR|S_IXGRP|S_IXOTH)){
            oss << "*";
        }
    } else if (S_ISDIR(mode)){
        oss << "/";
        LOGD("File is directory\n");
    } else if(S_ISLNK(mode) && is_kvaddr(i_link)) {
        LOGD("File is symbolic link, reading target\n");
        std::string link_target = read_long_string(i_link,"link path");
        if(!link_target.empty()) {
            oss << " -> " << link_target;
            LOGD("Link target: %s\n", link_target.c_str());
        } else {
            LOGE("Failed to read link target\n");
        }
    } else {
        LOGD("File is special type (mode: 0x%x)\n", mode);
    }
    std::string result = oss.str();
    return result;
}

std::string Cache::format_permissions(mode_t mode) {
    std::string perm(10, '-');
    if(S_ISDIR(mode)) perm[0] = 'd';
    else if(S_ISLNK(mode)) perm[0] = 'l';
    else if(S_ISBLK(mode)) perm[0] = 'b';
    else if(S_ISCHR(mode)) perm[0] = 'c';
    else if(S_ISFIFO(mode)) perm[0] = 'p';
    else if(S_ISSOCK(mode)) perm[0] = 's';
    if(mode & S_IRUSR) perm[1] = 'r';
    if(mode & S_IWUSR) perm[2] = 'w';
    if(mode & S_IXUSR) perm[3] = 'x';
    if(mode & S_IRGRP) perm[4] = 'r';
    if(mode & S_IWGRP) perm[5] = 'w';
    if(mode & S_IXGRP) perm[6] = 'x';
    if(mode & S_IROTH) perm[7] = 'r';
    if(mode & S_IWOTH) perm[8] = 'w';
    if(mode & S_IXOTH) perm[9] = 'x';
    return perm;
}

/**
 * @brief Display file page cache statistics
 *
 * Shows a comprehensive overview of all files that have pages cached in memory,
 * sorted by the number of cached pages (largest first). Displays total cache
 * usage and detailed information for each cached file including inode address,
 * address space, page count, size, and file path.
 */
void Cache::print_file_pages(){
    // Parse file pages if not already done
    if (cache_list.size() == 0){
        LOGD("Cache list empty, parsing file pages first\n");
        parser_file_pages();
    }
    // Calculate total cache size
    uint64_t total_size = 0;
    for (const auto& file_ptr : cache_list) {
        total_size += file_ptr->nrpages;
    }
    PRINT("Total cached files: %zu, total size: %s\n", cache_list.size(), csize(total_size * page_size).c_str());
    // Sort files by number of cached pages (descending order)
    std::sort(cache_list.begin(), cache_list.end(),[&](std::shared_ptr<FileCache> a, std::shared_ptr<FileCache> b){
        return a->nrpages > b->nrpages;
    });
    // Display header
    PRINT("===============================================\n");
    std::ostringstream oss;
    oss << std::left << std::setw(VADDR_PRLEN)  << "inode" << " "
        << std::left << std::setw(VADDR_PRLEN)  << "address_space" << " "
        << std::left << std::setw(8)            << "nrpages" << " "
        << std::left << std::setw(10)           << "size" << " "
        << std::left << "Path"
        << "\n";
    for (const auto& file_ptr : cache_list) {
        // Skip files with no cached pages
        if (file_ptr->nrpages == 0) {
            continue;
        }
        oss << std::left << std::hex  << std::setw(VADDR_PRLEN) << file_ptr->inode << " "
            << std::left << std::hex  << std::setw(VADDR_PRLEN) << file_ptr->i_mapping << " "
            << std::left << std::dec  << std::setw(8)           << file_ptr->nrpages << " "
            << std::left << std::dec  << std::setw(10)          << csize(file_ptr->nrpages * page_size) << " "
            << std::left << file_ptr->name
            << "\n";
    }
    PRINT("%s \n", oss.str().c_str());
}

/**
 * @brief Default constructor
 *
 * Initializes the Cache plugin with default settings.
 */
Cache::Cache(){

}

#pragma GCC diagnostic pop
