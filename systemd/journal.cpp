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

#include "journal.h"
#include <iomanip>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(Journal)
#endif

/**
 * @brief Main command entry point for systemd journal analysis
 *
 * Parses command line arguments and dispatches to appropriate handler functions:
 * -l: List journal files with statistics
 * -d: Dump journal files to disk
 * -s: Show journal log contents
 */
void Journal::cmd_main(void) {
    int c;
    std::string cppString;
    LOGD("Journal::cmd_main() - Starting systemd journal analysis\n");
    // Check minimum argument count
    if (argcnt < 2) {
        LOGD("Insufficient arguments provided, showing usage\n");
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }

    // Parse command line options
    while ((c = getopt(argcnt, args, "lsd")) != EOF) {
        switch(c) {
            case 'd':
                LOGD("Executing dump_journal_log()\n");
                dump_journal_log();
                break;
            case 'l':
                LOGD("Executing list_journal_log()\n");
                list_journal_log();
                break;
            case 's':
                LOGD("Executing show_journal_log()\n");
                show_journal_log();
                break;
            default:
                LOGD("Unknown option: -%c\n", c);
                argerrs++;
                break;
        }
    }
    // Handle argument errors
    if (argerrs){
        LOGE("Command line argument errors detected: %d\n", argerrs);
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }
}

/**
 * @brief Constructor with existing swap information
 * @param swap Shared pointer to existing Swapinfo instance
 */
Journal::Journal(std::shared_ptr<Swapinfo> swap) : swap_ptr(swap){

}

/**
 * @brief Default constructor - creates new swap information instance
 */
Journal::Journal(){
    swap_ptr = std::make_shared<Swapinfo>();
}

/**
 * @brief Initialize kernel structure field offsets
 *
 * Sets up field offsets for kernel structures used in journal analysis:
 * - inode structure fields for file system operations
 * - dentry structure fields for directory entry traversal
 */
void Journal::init_offset(void) {
    // Initialize inode structure field offsets
    field_init(inode,i_dentry);     // Dentry list head for this inode
    field_init(inode,i_sb);         // Superblock pointer
    field_init(inode,i_mapping);    // Address space for page cache
    field_init(inode,i_size);       // File size

    // Initialize dentry structure field offsets
    field_init(dentry,d_u);         // Union containing hash list node
}

/**
 * @brief Initialize command help and usage information
 *
 * Sets up the command name, description, and detailed help text including
 * usage examples and expected output formats for the systemd journal plugin.
 */
void Journal::init_command(void){
    cmd_name = "systemd";
    help_str_list={
        "systemd",                                /* command name */
        "display systemd journal information and extract log data",  /* short description */
        "[-l] [-d] [-s]\n"
        "  This command analyzes systemd journal files from both process memory (VMA)\n"
        "  and page cache (inodes), providing comprehensive journal log management.\n"
        "\n"
        "    -l              list all journal files with detailed statistics\n"
        "    -d              dump journal files to disk for offline analysis\n"
        "    -s              show journal log contents in chronological order\n",
        "\n",
        "EXAMPLES",
        "  List all journal files with detailed information:",
        "    %s> systemd -l",
        "    ═══════════════════════════════════════════════════════════════════════════════",
        "    Process Memory (VMA) - 2 files:",
        "     [ 1] system@bf00ed31f63c44bd8311c1e4185809e9-00000000000c340d-000000103eef2e0f.journal   8.00MB     SYSTEM",
        "     [ 2] system.journal                                       8.00MB     SYSTEM",
        "",
        "    Page Cache (Inodes) - 12 files:",
        "     [ 1] system@bf00ed31f63c44bd8311c1e4185809e9-00000000000a892c-0000000e05db5bbd.journal      8MB  1382p  67%   SYSTEM",
        "     [ 2] system.journal                                8MB  2048p 100%   SYSTEM",
        "",
        "    Summary Statistics:",
        "      Total Journal Files: 14",
        "        ├─ In Process Memory (VMA): 2          (Total: 16.01MB)",
        "        └─ In Page Cache (Inode):   12         (Total: 93.40MB)",
        "",
        "    Page Cache Statistics:",
        "       ├─ Total Cached Pages: 17351",
        "       └─ Cache Memory Usage: 67.78MB",
        "\n",
        "  Dump the journal log",
        "    %s> systemd -d",
        "     Save user-1000.journal to xx/systemd/user-1000.journal",
        "       - File size: 8388608 bytes",
        "       - Pages processed: 1382",
        "       - Pages written: 1382",
        "       - Pages excluded: 0",
        "       - Success rate: 100.0%",
        "\n",
        "  Show the journal log",
        "    %s> systemd -s system.journal",
        "     Journal Logs (274270 entries):",
        "     ================================================================================",
        "     [   11.343612] gen5 owfds0[1368]: [2348806272][LUTDMA_SetVqCmdDesc:1215] Workload Dirty Map for VQ0: 0x1",
        "     [   11.345555] gen5 owfds0[1368]: [2348806272][LUTDMA_SetVqCmdDesc:1215] Workload Dirty Map for VQ1: 0x0",
        "\n",
    };
}

/**
 * @brief Scan systemd-journal process VMA for journal files
 *
 * Searches through the virtual memory areas (VMAs) of the systemd-journal process
 * to find memory-mapped journal files. These are typically active journal files
 * that are currently being written to or recently accessed.
 */
void Journal::get_journal_vma_list(){
    LOGD("Starting VMA scan for journal files\n");
    // Find systemd-journal process
    tc_systemd_journal = find_proc("systemd-journal");
    if(!tc_systemd_journal){
        LOGE("Failed to find systemd-journal process\n");
        return;
    }
    LOGD("Found systemd-journal process at task: 0x%lx\n", tc_systemd_journal->task);
    // Initialize task utility if not already done
    if(!task_ptr){
        task_ptr = std::make_shared<UTask>(swap_ptr, tc_systemd_journal->task);
        LOGD("Created UTask instance for systemd-journal process\n");
    }
    // Clear and prepare VMA list
    log_vma_list.clear();
    log_vma_list.reserve(16); // Reserve space to avoid frequent reallocations

    size_t vma_scanned = 0;
    size_t journal_vmas_found = 0;

    // Scan all file-backed VMAs in the process
    for(const auto& vma_ptr : task_ptr->for_each_file_vma()){
        vma_scanned++;
        const std::string& vma_name = vma_ptr->name;
        // Check if this VMA contains a journal file
        if (vma_name.find(".journal") == std::string::npos){
            continue;
        }
        journal_vmas_found++;
        LOGD("Found journal VMA: %s (size: %lu bytes)\n", vma_name.c_str(), vma_ptr->vm_size);
        // Use VMA start address as key instead of filename to handle duplicate names
        log_vma_list[vma_ptr->addr] = vma_ptr;
    }
    LOGD("VMA scan completed: %zu VMAs scanned, %zu journal VMAs found\n",
         vma_scanned, journal_vmas_found);
}

/**
 * @brief Write VMA data to file
 *
 * Extracts data from a list of VMAs and writes it to a single output file.
 * This is used to reconstruct journal files from process memory mappings.
 * Each VMA is written to its correct file offset based on vm_pgoff.
 *
 * @param vma_list List of VMA structures containing journal data
 * @param filename Output filename
 * @param dst_dir Destination directory
 * @param show_log Whether to display progress information
 */
void Journal::write_vma_to_file(const std::vector<std::shared_ptr<vma_struct>>& vma_list, const std::string& filename, const std::string& dst_dir,bool show_log) {
    LOGD("Writing %zu VMAs to file: %s\n", vma_list.size(), filename.c_str());
    // Validate input parameters
    if (vma_list.empty()) {
        LOGE("Error: Empty VMA list for %s\n", filename.c_str());
        return;
    }
    if (dst_dir.empty() || filename.empty()) {
        LOGE("Error: Invalid destination directory or filename\n");
        return;
    }
    // Create output directory if it doesn't exist
    if (!create_directories_recursive(dst_dir)) {
        LOGE("Error: Failed to create directory %s: %s\n", dst_dir.c_str(), strerror(errno));
        return;
    }

    // Sort VMAs by file offset (vm_pgoff) to ensure correct ordering
    std::vector<std::shared_ptr<vma_struct>> sorted_vmas = vma_list;
    std::sort(sorted_vmas.begin(), sorted_vmas.end(),
              [](const std::shared_ptr<vma_struct>& a, const std::shared_ptr<vma_struct>& b) {
                  return a->vm_pgoff < b->vm_pgoff;
              });

    // Calculate total file size needed
    size_t max_file_size = 0;
    for (const auto& vma_ptr : sorted_vmas) {
        if (vma_ptr && vma_ptr->vm_size > 0) {
            size_t file_offset = vma_ptr->vm_pgoff * page_size;
            size_t end_offset = file_offset + vma_ptr->vm_size;
            max_file_size = std::max(max_file_size, end_offset);
        }
    }

    // Open output file
    const std::string log_path = dst_dir + "/" + filename;
    LOGD("Opening output file: %s (estimated size: %zu bytes)\n", log_path.c_str(), max_file_size);
    FILE* logfile = fopen(log_path.c_str(), "wb");
    if (!logfile) {
        LOGE("Can't open %s for writing\n", log_path.c_str());
        return;
    }

    size_t total_written = 0;
    size_t vma_count = 0;
    size_t vma_processed = 0;

    // Process each VMA in sorted order
    for (const auto& vma_ptr : sorted_vmas) {
        vma_processed++;
        if (!vma_ptr || vma_ptr->vm_size == 0) {
            LOGE("Skipping invalid VMA %zu (null or zero size)\n", vma_processed);
            continue;
        }

        // Calculate file offset based on vm_pgoff
        size_t file_offset = vma_ptr->vm_pgoff * page_size;
        LOGD("Processing VMA %zu: size=%lu bytes, vm_pgoff=%lu, file_offset=%zu\n",
             vma_processed, vma_ptr->vm_size, vma_ptr->vm_pgoff, file_offset);

        // Seek to the correct position in the file
        if (fseek(logfile, file_offset, SEEK_SET) != 0) {
            LOGE("Failed to seek to offset %zu in file %s: %s\n", file_offset, filename.c_str(), strerror(errno));
            continue;
        }

        // Read VMA data from process memory
        std::vector<char> vma_data = task_ptr->read_vma_data(vma_ptr);
        if (!vma_data.empty()) {
            const size_t write_size = std::min(vma_data.size(), static_cast<size_t>(vma_ptr->vm_size));
            // Write data to file at the correct offset
            if (fwrite(vma_data.data(), 1, write_size, logfile) == write_size) {
                total_written += write_size;
                vma_count++;
                LOGD("Successfully wrote %zu bytes from VMA %zu at offset %zu\n", write_size, vma_processed, file_offset);
            } else {
                LOGE("Warning: Failed to write VMA data for %s (VMA %zu) at offset %zu\n", filename.c_str(), vma_processed, file_offset);
            }
        } else {
            LOGE("Warning: Empty data read from VMA %zu\n", vma_processed);
        }
    }
    fclose(logfile);
    LOGD("VMA file write completed: %zu/%zu VMAs processed, %zu bytes written\n",
         vma_count, vma_processed, total_written);
    // Display progress information if requested
    if (show_log){
        PRINT("Save %s to %s\n", filename.c_str(), log_path.c_str());
        PRINT("  - VMAs processed: %zu\n", vma_count);
        PRINT("  - Total bytes written: %zu\n", total_written);
        PRINT("  - File size: %zu bytes\n\n", max_file_size);
    }
    return;
}

void Journal::list_journal_log(){
    // Initialize lists if needed
    if(log_vma_list.empty()){
        get_journal_vma_list();
    }
    if(log_inode_list.empty()){
        get_journal_inode_list();
    }
    PRINT( "\n");
    PRINT( "═══════════════════════════════════════════════════════════════════════════════\n");
    size_t total_files = log_vma_list.size() + log_inode_list.size();
    if (total_files == 0) {
        PRINT( "\n");
        PRINT( "┌─ No Journal Files Found ─────────────────────────────────────────────────────┐\n");
        PRINT( "│                                                                              │\n");
        PRINT( "│  Possible reasons:                                                           │\n");
        PRINT( "│    • systemd-journal process not running                                     │\n");
        PRINT( "│    • Journal files not currently in memory                                   │\n");
        PRINT( "│    • Insufficient permissions to access journal data                         │\n");
        PRINT( "│    • System not using systemd journaling                                     │\n");
        PRINT( "│                                                                              │\n");
        PRINT( "└──────────────────────────────────────────────────────────────────────────────┘\n");
        return;
    }
    // Display VMA-based journal files
    if (!log_vma_list.empty()) {
        PRINT( "Process Memory (VMA) - %zu files:\n", log_vma_list.size());
        size_t vma_index = 1;
        for(const auto& pair : log_vma_list){
            const ulong vma_addr = pair.first;
            const auto& vma_ptr = pair.second;

            // Extract filename from VMA name
            const std::string& vma_name = vma_ptr->name;
            size_t pos = vma_name.find_last_of("/\\");
            std::string filename = (pos != std::string::npos) ?
                vma_name.substr(pos + 1) : vma_name;

            // Format file info
            std::string size_str = csize(vma_ptr->vm_size);
            std::string type_str = get_journal_file_type(filename);
            PRINT( " [%2zu] 0x%016lx %-40s %8s %10s \n",
                   vma_index++,
                   vma_addr,
                   filename.c_str(),
                   size_str.c_str(),
                   type_str.c_str());
        }
    }
    // Display page cache-based journal files
    if (!log_inode_list.empty()) {
        PRINT( "\n");
        PRINT( "Page Cache (Inodes) - %zu files:\n", log_inode_list.size());
        // Create sorted list for better display
        std::vector<std::pair<std::string, JournalFileInfo>> sorted_inodes;
        for(const auto& pair : log_inode_list){
            JournalFileInfo info = get_inode_file_info(pair.second);
            sorted_inodes.emplace_back(pair.first, info);
        }
        // Sort by file size (largest first)
        std::sort(sorted_inodes.begin(), sorted_inodes.end(),
                  [](const auto& a, const auto& b) {
                      return a.second.file_size > b.second.file_size;
                  });
        size_t inode_index = 1;
        for(const auto& item : sorted_inodes){
            const std::string& filename = item.first;
            const JournalFileInfo& info = item.second;
            std::string size_str = csize(info.file_size);
            std::string pages_str = std::to_string(info.cached_pages) + "p";
            std::string cache_pct = std::to_string(info.cache_percentage) + "%";
            std::string type_str = get_journal_file_type(filename);
            PRINT( " [%2zu] %-40s %8s %6s %4s %8s \n",
                   inode_index++,
                   filename.c_str(),
                   size_str.c_str(),
                   pages_str.c_str(),
                   cache_pct.c_str(),
                   type_str.c_str());
        }
    }
    // Display summary statistics
    display_summary_statistics();
}

JournalFileInfo Journal::get_inode_file_info(ulong inode_addr) {
    JournalFileInfo info = {0};
    info.inode_addr = inode_addr;
    void* inode_buf = read_struct(inode_addr, "inode");
    if (inode_buf) {
        info.mapping_addr = ULONG(inode_buf + field_offset(inode, i_mapping));
        info.file_size = ULONGLONG(inode_buf + field_offset(inode, i_size));
        if (is_kvaddr(info.mapping_addr)) {
            info.cached_pages = read_ulong(info.mapping_addr + field_offset(address_space, nrpages), "nrpages");
            // Calculate cache percentage
            if (info.file_size > 0) {
                ulonglong total_pages = (info.file_size + page_size - 1) / page_size;
                info.cache_percentage = (info.cached_pages * 100) / total_pages;
            }
        }
        FREEBUF(inode_buf);
    }
    return info;
}

std::string Journal::get_journal_file_type(const std::string& filename) {
    if (filename.find("system") != std::string::npos) {
        return "SYSTEM";
    } else if (filename.find("user-") != std::string::npos) {
        return "USER";
    } else if (filename.find(".journal~") != std::string::npos) {
        return "TEMP";
    } else if (filename.find("@") != std::string::npos) {
        return "ARCHIVE";
    } else {
        return "OTHER";
    }
}

void Journal::display_summary_statistics() {
    size_t total_vma_files = log_vma_list.size();
    size_t total_inode_files = log_inode_list.size();
    size_t total_files = total_vma_files + total_inode_files;
    // Calculate total sizes
    size_t total_vma_size = 0;
    for (const auto& pair : log_vma_list) {
        total_vma_size += pair.second->vm_size;
    }
    ulonglong total_inode_size = 0;
    ulong total_cached_pages = 0;
    for (const auto& pair : log_inode_list) {
        JournalFileInfo info = get_inode_file_info(pair.second);
        total_inode_size += info.file_size;
        total_cached_pages += info.cached_pages;
    }
    PRINT( "\n");
    PRINT( "Summary Statistics:\n");
    PRINT( "   Total Journal Files: %-10zu \n", total_files);
    PRINT( "     ├─ In Process Memory (VMA): %-10zu (Total: %s)%*s \n",
           total_vma_files, csize(total_vma_size).c_str(),
           20 - (int)csize(total_vma_size).length(), "");
    PRINT( "     └─ In Page Cache (Inode):   %-10zu (Total: %s)%*s \n",
           total_inode_files, csize(total_inode_size).c_str(),
           20 - (int)csize(total_inode_size).length(), "");
    PRINT( "\n");
    PRINT( "   Page Cache Statistics: \n");
    PRINT( "    ├─ Total Cached Pages: %-10lu \n", total_cached_pages);
    PRINT( "    └─ Cache Memory Usage: %s%*s \n",
           csize(total_cached_pages * page_size).c_str(),
           45 - (int)csize(total_cached_pages * page_size).length(), "");
    PRINT( "\nUse 'systemd -d' to dump journal log\n");
    PRINT( "Use 'systemd -s' to show journal log contents\n");
}

void Journal::parser_journal_log(const std::string &filepath) {
    if (filepath.empty()) {
        LOGE( "Error: Invalid filepath\n");
        return;
    }
    LOGD( "Starting to parse journal log file: %s\n", filepath.c_str());
    sd_journal *journal = nullptr;
    const char *paths[] = {filepath.c_str(), nullptr};
    int ret = sd_journal_open_files(&journal, paths, 0);
    if (ret) {
        LOGE( "Open file failed: %s\n", strerror(-ret));
        return;
    }
    ret = sd_journal_seek_head(journal);
    if (ret < 0) {
        LOGE( "Failed to seek to journal head: %s\n", strerror(-ret));
        sd_journal_close(journal);
        return;
    }
    size_t entry_count = 0;
    size_t error_count = 0;
    SD_JOURNAL_FOREACH(journal) {
        auto log_ptr = std::make_shared<journal_log>();
        entry_count++;
        ret = sd_journal_get_realtime_usec(journal, &log_ptr->timestamp);
        if (ret < 0) {
            LOGE( "Failed to get timestamp for entry %zu: %s\n", entry_count, strerror(-ret));
            error_count++;
            continue;
        }
        sd_id128_t boot_id = SD_ID128_NULL;
        // Get monotonic timestamp for accurate boot-relative time
        ret = sd_journal_get_monotonic_usec(journal, &log_ptr->monotonic_timestamp, &boot_id);
        if (ret < 0) {
            LOGE( "Failed to get monotonic_timestamp for entry %zu: %s\n", entry_count, strerror(-ret));
            error_count++;
            continue;
        }

        const char *message = nullptr;
        const char *pid = nullptr;
        const char *comm = nullptr;
        const char *hostname = nullptr;
        size_t length;
        // <timestamp> <hostname> <COMM>[<pid>]: <MESSAGE>
        ret = sd_journal_get_data(journal, "_HOSTNAME", (const void**)&hostname, &length);
        if (ret >= 0 && hostname) {
            const char *host_content = strchr(hostname, '=');
            if (host_content) {
                host_content++;
                log_ptr->hostname = std::string(host_content, (int)(length - (host_content - hostname)));
            }
        }
        ret = sd_journal_get_data(journal, "_PID", (const void**)&pid, &length);
        if (ret >= 0 && pid) {
            const char *pid_content = strchr(pid, '=');
            if (pid_content) {
                pid_content++;
                log_ptr->pid = std::string(pid_content, (int)(length - (pid_content - pid)));
            }
        }
        ret = sd_journal_get_data(journal, "_COMM", (const void**)&comm, &length);
        if (ret >= 0 && comm) {
            const char *comm_content = strchr(comm, '=');
            if (comm_content) {
                comm_content++;
                log_ptr->com = std::string(comm_content, (int)(length - (comm_content - comm)));
            }
        }
        ret = sd_journal_get_data(journal, "MESSAGE", (const void**)&message, &length);
        if (ret >= 0 && message) {
            const char *msg_content = strchr(message, '=');
            if (msg_content) {
                msg_content++;
                log_ptr->message = std::string(msg_content, (int)(length - (msg_content - message)));
                LOGD( "%s \n", log_ptr->message.c_str());
            }
        }
        log_list.emplace_back(std::move(log_ptr));
    }
    sd_journal_close(journal);
    LOGD( "Total entries processed: %zu\n", entry_count);
    LOGD( "Errors encountered: %zu\n", error_count);
    LOGD( "Success rate: %.1f%%\n", entry_count > 0 ? ((entry_count - error_count) * 100.0 / entry_count) : 0.0);
    LOGD( "Journal file: %s\n", filepath.c_str());
}

void Journal::print_syslog(std::shared_ptr<journal_log> log_ptr) {
    std::ostringstream oss;
    // Convert monotonic timestamp from microseconds to seconds
    double monotonic_seconds = (double)log_ptr->monotonic_timestamp / 1000000.0;

    // Format timestamp to match journalctl -o short-monotonic: [   11.343612]
    // Right-aligned within brackets, 6 decimal places, total width ~12 characters
    oss << "[" << std::setw(11) << std::right << std::fixed << std::setprecision(6)
        << monotonic_seconds << "] ";

    time_t time_sec = log_ptr->timestamp / 1000000;
    struct tm *tm_info = localtime(&time_sec);
    if (tm_info) {
        char time_str[32];
        strftime(time_str, sizeof(time_str), "%b %d %H:%M:%S", tm_info);
        oss << time_str << " ";
    } else {
        oss << "Unknown_Time ";
    }

    // Format hostname
    if (!log_ptr->hostname.empty()) {
        oss << log_ptr->hostname;
    } else {
        oss << "localhost";
    }

    oss << " ";

    // Format command name
    if (!log_ptr->com.empty()) {
        oss << log_ptr->com;
    } else {
        oss << "kernel";
    }

    // Format PID if available
    if (!log_ptr->pid.empty()) {
        oss << "[" << log_ptr->pid << "]";
    }

    oss << ": ";

    // Format message
    if (!log_ptr->message.empty()) {
        oss << log_ptr->message;
    } else {
        oss << "(no message)";
    }

    PRINT("%s\n", oss.str().c_str());
}

bool Journal::find_journal_log(ulong inode,std::string& file){
    ulong hlist_head = inode + field_offset(inode,i_dentry);
    int offset = field_offset(dentry,d_u);
    for (const auto& dentry : for_each_hlist(hlist_head,offset)) {
        file = get_dentry_path(dentry);
        if (!file.empty() && file.find(".journal") != std::string::npos){
            return true;
        }
    }
    return false;
}

/**
 * @brief Scan kernel inodes for journal files in page cache
 *
 * Searches through all kernel inodes to find journal files that are cached
 * in the page cache. These files may not be currently mapped in any process
 * but are still accessible through the kernel's page cache mechanism.
 */
void Journal::get_journal_inode_list(){
    LOGD("Starting inode scan for journal files\n");
    log_inode_list.reserve(64); // Reserve space to avoid frequent reallocations

    size_t inodes_scanned = 0;
    size_t journal_inodes_found = 0;
    size_t inodes_with_pages = 0;

    // Scan all inodes in the system
    for (const auto& addr : for_each_inode()) {
        inodes_scanned++;

        // Check if this inode represents a journal file
        std::string log_file;
        if (!find_journal_log(addr, log_file)){
            continue;
        }

        journal_inodes_found++;
        LOGD("Found journal inode: %s (addr: 0x%lx)\n", log_file.c_str(), addr);

        // Extract filename more efficiently
        size_t pos = log_file.find_last_of("/\\");
        std::string fileName = (pos != std::string::npos) ?
            log_file.substr(pos + 1) : log_file;

        // Check if inode has valid address space mapping
        ulong i_mapping = read_pointer(addr + field_offset(inode, i_mapping), "i_mapping");
        if (!is_kvaddr(i_mapping)) {
            LOGE("Skipping inode %s - invalid mapping address: 0x%lx\n", fileName.c_str(), i_mapping);
            continue;
        }

        // Check if there are cached pages
        ulong nrpages = read_ulong(i_mapping + field_offset(address_space, nrpages), "nrpages");
        if (!nrpages) {
            LOGE("Skipping inode %s - no cached pages\n", fileName.c_str());
            continue;
        }

        // Verify that pages are actually accessible
        std::vector<ulong> pagelist = for_each_address_space(i_mapping);
        if (pagelist.empty()) {
            LOGE("Skipping inode %s - page list empty despite nrpages=%lu\n", fileName.c_str(), nrpages);
            continue;
        }

        inodes_with_pages++;
        LOGD("Added journal inode: %s (%lu pages cached)\n", fileName.c_str(), nrpages);
        log_inode_list.emplace(std::move(fileName), addr);
    }

    LOGD("Inode scan completed: %zu inodes scanned, %zu journal inodes found, %zu with cached pages\n",
         inodes_scanned, journal_inodes_found, inodes_with_pages);
    LOGD("Unique journal files in page cache: %zu\n", log_inode_list.size());
}

/**
 * @brief Display journal log entries in syslog format
 *
 * Extracts journal files from both VMA and page cache, parses them using
 * systemd library, and displays the log entries in chronological order.
 * Temporary files are created in /tmp for parsing purposes.
 */
void Journal::show_journal_log(){
    LOGD("Starting journal log display\n");
    if (log_list.empty()) {
        LOGD("Log list is empty, initializing journal file discovery\n");
        // Initialize lists if needed
        if(log_vma_list.empty()){
            get_journal_vma_list();
        }
        if(log_inode_list.empty()){
            get_journal_inode_list();
        }

        size_t vma_files_processed = 0;
        size_t inode_files_processed = 0;

        // Process VMA logs - group VMAs by filename
        LOGD("Processing %zu VMA journal files\n", log_vma_list.size());

        // Group VMAs by filename
        std::unordered_map<std::string, std::vector<std::shared_ptr<vma_struct>>> grouped_vmas;
        for(const auto& pair : log_vma_list){
            const auto& vma_ptr = pair.second;

            // Extract filename from VMA name
            const std::string& vma_name = vma_ptr->name;
            size_t pos = vma_name.find_last_of("/\\");
            std::string filename = (pos != std::string::npos) ?
                vma_name.substr(pos + 1) : vma_name;

            grouped_vmas[filename].push_back(vma_ptr);
        }

        // Process each file (which may have multiple VMAs)
        for(const auto& file_pair : grouped_vmas){
            const std::string& log_name = file_pair.first;
            const auto& vma_vector = file_pair.second;

            LOGD("Processing VMA file: %s (%zu VMAs)\n", log_name.c_str(), vma_vector.size());

            // Generate random filename for /tmp directory
            srand(time(nullptr) + vma_files_processed); // Add counter to ensure uniqueness
            char random_suffix[16];
            snprintf(random_suffix, sizeof(random_suffix), "%08x", rand());
            std::string random_filename = std::string("journal_vma_") + random_suffix + ".journal";
            write_vma_to_file(vma_vector, random_filename, "/tmp", false);
            parser_journal_log(std::string("/tmp/") + random_filename);
            vma_files_processed++;
        }

        // Process inode logs
        LOGD("Processing %zu inode journal files\n", log_inode_list.size());
        for(const auto& pair : log_inode_list){
            const std::string& log_name = pair.first;
            const ulong inode_addr = pair.second;
            LOGD("Processing inode file: %s (addr: 0x%lx)\n", log_name.c_str(), inode_addr);

            // Generate random filename for /tmp directory
            srand(time(nullptr) + inode_files_processed); // Add counter to ensure uniqueness
            char random_suffix[16];
            snprintf(random_suffix, sizeof(random_suffix), "%08x", rand());
            std::string random_filename = std::string("journal_cache_") + random_suffix + ".journal";
            write_pagecache_to_file(inode_addr, random_filename, "/tmp", false);
            parser_journal_log(std::string("/tmp/") + random_filename);
            inode_files_processed++;
        }

        LOGD("Processed %zu VMA files and %zu inode files\n", vma_files_processed, inode_files_processed);
        LOGD("Total log entries collected: %zu\n", log_list.size());

        // Sort logs by timestamp (ascending order for chronological display)
        LOGD("Sorting log entries by timestamp\n");
        std::sort(log_list.begin(), log_list.end(),
            [](const std::shared_ptr<journal_log>& a, const std::shared_ptr<journal_log>& b) {
                // Handle null pointers
                if (!a && !b) return false;
                if (!a) return false; // null comes last
                if (!b) return true;
                // Sort by timestamp (ascending order for chronological display)
                return a->timestamp < b->timestamp;
        });
    }

    // Display results
    if (log_list.empty()) {
        PRINT("No journal logs to display\n");
        return;
    }
    PRINT("Journal Logs (%zu entries):\n", log_list.size());
    PRINT("================================================================================\n");

    // Use a set to track unique logs for deduplication
    std::set<std::tuple<uint64_t, std::string, std::string, std::string, std::string>> seen_logs;
    size_t displayed_entries = 0;
    size_t duplicate_entries = 0;

    for (const auto& log : log_list) {
        if (!log) continue;

        // Create a unique key for this log entry
        auto log_key = std::make_tuple(
            log->timestamp,
            log->hostname,
            log->com,
            log->pid,
            log->message
        );

        // Check if we've already seen this log entry
        if (seen_logs.find(log_key) != seen_logs.end()) {
            duplicate_entries++;
            continue;  // Skip duplicate log
        }

        // Mark this log as seen and display it
        seen_logs.insert(log_key);
        print_syslog(log);
        displayed_entries++;
    }

    PRINT("================================================================================\n");
    LOGD("Displayed %zu unique entries, filtered %zu duplicates\n", displayed_entries, duplicate_entries);
}

/**
 * @brief Dump journal files to disk for offline analysis
 *
 * Extracts journal files from both process memory (VMA) and page cache (inodes)
 * and saves them to separate directories for further analysis. Files from page
 * cache are saved to systemd/cache/ and VMA files to systemd/vma/.
 */
void Journal::dump_journal_log(){
    LOGD("Starting journal file dump\n");
    // Initialize lists if needed
    if(log_inode_list.empty()){
        get_journal_inode_list();
    }
    if(log_vma_list.empty()){
        get_journal_vma_list();
    }

    size_t inode_files_dumped = 0;
    size_t vma_files_dumped = 0;

    // Dump files from page cache (inodes)
    std::string log_path = get_curpath().str() + "/systemd/cache";
    LOGD("Dumping %zu inode journal files to: %s\n", log_inode_list.size(), log_path.c_str());

    for (const auto& inode_it : log_inode_list) {
        LOGD("Dumping inode file: %s (addr: 0x%lx)\n", inode_it.first.c_str(), inode_it.second);
        write_pagecache_to_file(inode_it.second, inode_it.first, log_path, true);
        inode_files_dumped++;
    }

    // Dump files from process memory (VMA) - group VMAs by filename
    log_path = get_curpath().str() + "/systemd/vma";
    LOGD("Dumping %zu VMA journal files to: %s\n", log_vma_list.size(), log_path.c_str());

    // Group VMAs by filename
    std::unordered_map<std::string, std::vector<std::shared_ptr<vma_struct>>> grouped_vmas;
    for(const auto& vma_it : log_vma_list) {
        const auto& vma_ptr = vma_it.second;

        // Extract filename from VMA name
        const std::string& vma_name = vma_ptr->name;
        size_t pos = vma_name.find_last_of("/\\");
        std::string filename = (pos != std::string::npos) ?
            vma_name.substr(pos + 1) : vma_name;

        grouped_vmas[filename].push_back(vma_ptr);
    }

    // Dump each file (which may have multiple VMAs)
    for(const auto& file_pair : grouped_vmas){
        const std::string& filename = file_pair.first;
        const auto& vma_vector = file_pair.second;

        LOGD("Dumping VMA file: %s (%zu VMAs)\n", filename.c_str(), vma_vector.size());
        write_vma_to_file(vma_vector, filename, log_path, true);
        vma_files_dumped++;
    }

    LOGD("Journal dump completed: %zu inode files, %zu VMA files\n",
         inode_files_dumped, vma_files_dumped);

    // Display summary
    PRINT("\nJournal dump summary:\n");
    PRINT("  - Page cache files dumped: %zu\n", inode_files_dumped);
    PRINT("  - VMA files dumped: %zu\n", vma_files_dumped);
    PRINT("  - Total files dumped: %zu\n", inode_files_dumped + vma_files_dumped);
}
#pragma GCC diagnostic pop
