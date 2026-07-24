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

#include "ipc.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(IPCLog)
#endif

/**
 * @brief Initialize kernel structure field offsets for IPC logging
 *
 * Sets up field offsets for all IPC logging related kernel structures.
 * These offsets are essential for reading IPC log data from kernel memory
 * across different kernel versions and configurations.
 */
void IPCLog::init_offset(void) {
    // Initialize ipc_log_context structure offsets
    field_init(ipc_log_context,magic);          // Magic number for validation
    field_init(ipc_log_context,version);        // IPC logging version
    field_init(ipc_log_context,user_version);   // User-defined version
    field_init(ipc_log_context,name);           // Context name string
    field_init(ipc_log_context,list);           // List linkage for context chain
    field_init(ipc_log_context,first_page);     // First page in circular buffer
    field_init(ipc_log_context,last_page);      // Last page in circular buffer
    field_init(ipc_log_context,write_page);     // Current write page
    field_init(ipc_log_context,read_page);      // Current read page
    field_init(ipc_log_context,nd_read_page);   // Non-destructive read page
    field_init(ipc_log_context,write_avail);    // Available write space
    field_init(ipc_log_context,page_list);      // Page list head
    struct_init(ipc_log_context);               // Complete structure size

    // Initialize ipc_log_page structure offsets
    field_init(ipc_log_page,hdr);               // Page header
    field_init(ipc_log_page,data);              // Page data section

    // Initialize ipc_log_page_header structure offsets
    field_init(ipc_log_page_header,magic);      // Page magic number
    field_init(ipc_log_page_header,write_offset); // Current write offset
    field_init(ipc_log_page_header,start_time); // Page start timestamp
    field_init(ipc_log_page_header,end_time);   // Page end timestamp
    field_init(ipc_log_page_header,nd_read_offset); // Non-destructive read offset
    field_init(ipc_log_page_header,list);       // Page list linkage

    // Initialize structure sizes
    struct_init(ipc_log_page_header);           // Page header size
    struct_init(ipc_log_page);                  // Complete page size
    struct_init(tsv_header);                    // TSV header size
}

/**
 * @brief Main command entry point for IPC log analysis
 *
 * Parses command line arguments and dispatches to appropriate handler functions:
 * -a: Display all IPC log contexts with summary information
 * -l <name>: Display logs from specific IPC module by name
 * -s: Save all IPC logs to files for offline analysis
 */
void IPCLog::cmd_main(void) {
    // Check minimum argument count
    if (argcnt < 2) {
        LOGD("Insufficient arguments provided, showing usage\n");
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }
    // Initialize and parse IPC contexts if not already done
    if (ipc_list.empty()) {
        init_offset();
        parser_ipc_log();
    } else {
        LOGD("Using cached IPC context list with %zu contexts\n", ipc_list.size());
    }
    int argerrs = 0;
    int c;
    std::string module_name;

    // Parse command line options
    while ((c = getopt(argcnt, args, "al:s")) != EOF) {
        switch(c) {
            case 'a':
                print_ipc_info();
                break;
            case 'l':
                module_name.assign(optarg);
                print_ipc_log(module_name);
                break;
            case 's':
                save_ipc_log();
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

void IPCLog::init_command(void) {
    cmd_name = "ipc";
    help_str_list={
        "ipc",                                     /* command name */
        "dump IPC (Inter-Process Communication) log",  /* short description */
        "[-a] [-l module_name] [-s]\n"
        "  This command dumps IPC (Inter-Process Communication) log information.\n"
        "\n"
        "    -a              display all IPC log contexts with summary information\n"
        "    -l module_name  display logs from specific IPC module by name\n"
        "    -s              save all IPC logs to files for offline analysis\n",
        "\n",
        "EXAMPLES",
        "  Display all IPC log contexts:",
        "    %s> ipc -a",
        "    ipc_log_context  Magic      Version first_page       last_page        write_page       read_page        Name",
        "    ffffff80084bfd00 25874452   3       ffffff800a3b7000 ffffff8008628000 ffffff800a3b1000 ffffff800a3b1000 rpm-glink",
        "    ffffff8008654500 25874452   3       ffffff800863d000 ffffff800863b000 ffffff800863a000 ffffff800863a000 qrtr_ns",
        "    ffffff800a721600 25874452   3       ffffff8008705000 ffffff8013304000 ffffff8008705000 ffffff8008705000 mmc0",
        "    ffffff80180c5c00 25874452   3       ffffff801496b000 ffffff8015d7b000 ffffff801496e000 ffffff801496e000 glink_pkt",
        "\n",
        "  Display IPC logs from specific module:",
        "    %s> ipc -l glink_pkt",
        "    [ 161.448574138 0x90dee57d91b]   [glink_pkt_poll]: Exit channel:location_ctrl",
        "    [ 161.448616117 0x90dee57dc3f]   [glink_pkt_poll]: Wait for pkt on channel:location_ctrl",
        "    [ 161.448627731 0x90dee57dd1d]   [glink_pkt_poll]: Exit channel:location_ctrl",
        "    [ 161.475913096 0x90dee5fdb85]   [glink_pkt_rpdev_copy_cb]: Data received on:ss_bt_ctrl len:60",
        "    [ 161.475946481 0x90dee5fde06]   [glink_pkt_rpdev_copy_cb]: Data queued on:ss_bt_ctrl len:60",
        "    [ 161.475976325 0x90dee5fe043]   [glink_pkt_poll]: Wait for pkt on channel:ss_bt_ctrl",
        "\n",
        "  Save all IPC logs to files:",
        "    %s> ipc -s",
        "    Save rpm-glink to /xxx/ipc_log/rpm-glink",
        "    Save qrtr_ns to /xxx/ipc_log/qrtr_ns",
        "    Save mmc0 to /xxx/ipc_log/mmc0",
        "    Save glink_pkt to /xxx/ipc_log/glink_pkt",
        "\n",
    };
}

/**
 * @brief Default constructor
 *
 * Initializes the IPC log parser with default settings.
 * Sets do_init_offset to false to prevent automatic offset initialization.
 */
IPCLog::IPCLog(){
    do_init_offset = false;
}

/**
 * @brief Display logs from a specific IPC module by name
 * @param name Name of the IPC module to display logs for
 *
 * Searches through all discovered IPC contexts for the specified module name,
 * validates the log page magic number, parses the log entries if not already
 * cached, and displays all log entries with proper formatting.
 */
void IPCLog::print_ipc_log(const std::string& name){
    LOGD("Searching for IPC module: %s in %zu contexts\n", name.c_str(), ipc_list.size());
    bool found_module = false;
    for (const auto& log_ptr : ipc_list) {
        // Skip contexts with empty names or non-matching names
        if (log_ptr->name.empty() || name != log_ptr->name){
            continue;
        }
        found_module = true;
        LOGD("Found matching IPC context: %s at address 0x%lx\n", log_ptr->name.c_str(), log_ptr->addr);

        // Validate log page magic number
        uint32_t magic = read_ulong(log_ptr->nd_read_page + field_offset(ipc_log_page,hdr) + field_offset(ipc_log_page_header,magic),"magic");
        if (magic != IPC_LOGGING_MAGIC_NUM){
            LOGE("Invalid magic number 0x%x for IPC context %s (expected 0x%x)\n",
                 magic, log_ptr->name.c_str(), IPC_LOGGING_MAGIC_NUM);
            continue;
        }
        // Parse log pages if not already cached
        if (log_ptr->logs.empty()){
            parser_ipc_log_page(log_ptr);
            LOGD("Parsed %zu log entries for context %s\n", log_ptr->logs.size(), log_ptr->name.c_str());
        } else {
            LOGD("Using cached log entries (%zu entries) for context %s\n",
                 log_ptr->logs.size(), log_ptr->name.c_str());
        }

        // Display all log entries with formatting cleanup
        size_t displayed_count = 0;
        for (auto& log : log_ptr->logs){
            // Clean up trailing whitespace and newlines
            while (!log.empty() && (log.back() == ' ' || log.back() == '\n' || log.back() == '\r')){
                log.pop_back();
            }
            if (!log.empty()) {
                PRINT( "%s\n", log.c_str());
                displayed_count++;
            }
        }
        LOGD("Displayed %zu log entries for IPC module %s\n", displayed_count, name.c_str());
    }
    if (!found_module) {
        LOGE("IPC module '%s' not found in %zu available contexts\n", name.c_str(), ipc_list.size());
    }
}

/**
 * @brief Parse log entries from IPC log pages in circular buffer
 * @param log_ptr Pointer to IPC log context to parse
 *
 * This function traverses the circular buffer of log pages for the specified
 * IPC context, parsing TSV-formatted log entries and extracting timestamps
 * and message text. The circular buffer may wrap around, requiring special
 * handling to read data in the correct order.
 *
 * The TSV format consists of:
 * 1. TSV_TYPE_TIMESTAMP: System timestamp (32 or 64-bit)
 * 2. TSV_TYPE_QTIMER: Hardware QTimer value (32 or 64-bit)
 * 3. TSV_TYPE_BYTE_ARRAY: Log message text
 */
void IPCLog::parser_ipc_log_page(std::shared_ptr<ipc_log> log_ptr){
    // Validate input parameters
    if (!log_ptr) {
        LOGE("Invalid log_ptr parameter (null pointer)\n");
        return;
    }

    if (!is_kvaddr(log_ptr->nd_read_page)) {
        LOGE("Invalid nd_read_page address 0x%lx for context %s\n",
             log_ptr->nd_read_page, log_ptr->name.c_str());
        return;
    }

    LOGD("Starting to parse IPC log pages for context %s\n", log_ptr->name.c_str());
    LOGD("Context details: addr=0x%lx, nd_read_page=0x%lx\n",
         log_ptr->addr, log_ptr->nd_read_page);

    // Initialize circular buffer traversal variables
    ulong curr_read_page = log_ptr->nd_read_page;
    ulong page_list = log_ptr->addr + field_offset(ipc_log_context,page_list);

    // Read initial page header information
    uint16_t write_offset = read_ushort(curr_read_page + field_offset(ipc_log_page,hdr)
            + field_offset(ipc_log_page_header,write_offset),"write_offset");
    uint16_t nd_read_offset = read_ushort(curr_read_page + field_offset(ipc_log_page,hdr)
            + field_offset(ipc_log_page_header,nd_read_offset),"nd_read_offset");

    // Determine if circular buffer has wrapped around
    bool wrapped_around = (nd_read_offset > write_offset);
    LOGD("ipc_log_page_header offsets: write_offset=%u, nd_read_offset=%u, wrapped_around=%s\n",
         write_offset, nd_read_offset, wrapped_around ? "true" : "false");

    // Initialize buffer for collecting log data
    std::vector<char> ipcLogBuf;
    ipcLogBuf.reserve(65536); // Pre-allocate reasonable size

    // Calculate field offsets for efficient access
    ulong hdr_offset = field_offset(ipc_log_page,hdr);
    ulong write_offset_field = hdr_offset + field_offset(ipc_log_page_header,write_offset);
    ulong nd_read_offset_field = hdr_offset + field_offset(ipc_log_page_header,nd_read_offset);
    ulong list_offset = hdr_offset + field_offset(ipc_log_page_header,list);
    ulong data_offset = field_offset(ipc_log_page,data);
    size_t data_size = field_size(ipc_log_page,data);

    LOGD("ipc_log_page offsets: hdr=%lu, data=%lu, data_size=%zu\n",
         hdr_offset, data_offset, data_size);

    // Traverse circular buffer pages
    size_t pages_processed = 1;
    size_t total_bytes_copied = 0;

    while (is_kvaddr(curr_read_page)){
        LOGD("Processing page %zu at address 0x%lx\n", pages_processed, curr_read_page);

        // Read page structure from memory
        void *log_page_buf = read_struct(curr_read_page,"ipc_log_page");
        if (!log_page_buf) {
            LOGE("Failed to read ipc_log_page structure at 0x%lx\n", curr_read_page);
            return;
        }

        // Extract current page offsets
        write_offset = USHORT(log_page_buf + write_offset_field);
        nd_read_offset = USHORT(log_page_buf + nd_read_offset_field);
        FREEBUF(log_page_buf);

        LOGD("Page %zu: write_offset=%u, nd_read_offset=%u\n",
             pages_processed, write_offset, nd_read_offset);

        // Calculate bytes to copy from this page
        size_t bytes_to_copy = (nd_read_offset <= write_offset) ?
                               (write_offset - nd_read_offset) :
                               (data_size - nd_read_offset);

        if (bytes_to_copy <= 0){
            LOGD("No data to copy from page %zu, breaking\n", pages_processed);
            break;
        }

        LOGD("Copying %zu bytes from page %zu\n", bytes_to_copy, pages_processed);

        // Read and append page data to buffer
        ulong start_addr = curr_read_page + data_offset + nd_read_offset;
        void* data_buf = read_memory(start_addr, bytes_to_copy, "data");
        if (data_buf) {
            appendBuffer(ipcLogBuf, data_buf, bytes_to_copy);
            total_bytes_copied += bytes_to_copy;
            FREEBUF(data_buf);
            LOGD("Successfully copied %zu bytes, total so far: %zu\n",
                 bytes_to_copy, total_bytes_copied);
        } else {
            LOGE("Failed to read %zu bytes from address 0x%lx\n", bytes_to_copy, start_addr);
        }

        // Check if we should continue to next page
        if (!wrapped_around && write_offset < data_size){
            LOGD("Reached end of data in non-wrapped buffer\n");
            break;
        }

        // Navigate to next page in circular buffer
        ulong list_addr = curr_read_page + list_offset;
        ulong list = read_pointer(list_addr,"list");
        if (list == page_list){
            list = read_pointer(list,"list");
        }
        curr_read_page = list - list_offset;

        LOGD("Moving to next page: 0x%lx\n", curr_read_page);

        // Prevent infinite loop
        if (curr_read_page == log_ptr->nd_read_page){
            LOGD("Completed circular buffer traversal (returned to start page)\n");
            break;
        }
        pages_processed++;
    }

    // Handle wrapped buffer case - read remaining data from start of first page
    if (wrapped_around){
        LOGD("Handling wrapped buffer case\n");
        write_offset = read_ushort(log_ptr->nd_read_page + write_offset_field,"write_offset");
        if (write_offset > 0) {
            LOGD("Reading %u bytes from start of first page\n", write_offset);
            ulong start_addr = log_ptr->nd_read_page + data_offset;
            void* data_buf = read_memory(start_addr, write_offset, "data");
            if (data_buf) {
                appendBuffer(ipcLogBuf, data_buf, write_offset);
                total_bytes_copied += write_offset;
                FREEBUF(data_buf);
                LOGD("Added %u bytes from wrapped section, total: %zu\n",
                     write_offset, total_bytes_copied);
            }
        }
    }

    LOGD("Completed page traversal: %zu pages processed, %zu total bytes collected\n",
         pages_processed, total_bytes_copied);

    // Parse TSV-formatted log entries from collected buffer
    size_t len = 0;
    char* dataPtr = ipcLogBuf.data();
    size_t bufSize = ipcLogBuf.size();
    std::ostringstream oss;
    size_t entries_parsed = 0;
    LOGD("Starting log parsing: buffer size=%zu bytes\n", bufSize);
    while (len + sizeof(tsv_header) * 3 <= bufSize) {
        // Skip first TSV header (entry separator)
        len += sizeof(tsv_header);
        dataPtr += sizeof(tsv_header);

        // Parse timestamp TSV entry
        if (len + sizeof(tsv_header) > bufSize) break;
        tsv_header msg = *reinterpret_cast<tsv_header*>(dataPtr);
        len += sizeof(tsv_header);
        dataPtr += sizeof(tsv_header);

        uint64_t TimeStamp = 0;
        if (msg.type == TSV_TYPE_TIMESTAMP && len + msg.size <= bufSize){
            if (msg.size == 4){
                TimeStamp = *reinterpret_cast<uint32_t*>(dataPtr);
            }else if (msg.size == 8){
                TimeStamp = *reinterpret_cast<uint64_t*>(dataPtr);
            }
            len += msg.size;
            dataPtr += msg.size;
            // LOGD("Parsed timestamp: %lu (size=%u)\n", TimeStamp, msg.size);
        }

        // Parse QTimer TSV entry
        if (len + sizeof(tsv_header) > bufSize) break;
        msg = *reinterpret_cast<tsv_header*>(dataPtr);
        len += sizeof(tsv_header);
        dataPtr += sizeof(tsv_header);

        uint64_t TimeQtimer = 0;
        if (msg.type == TSV_TYPE_QTIMER && len + msg.size <= bufSize){
            if (msg.size == 4){
                TimeQtimer = *reinterpret_cast<uint32_t*>(dataPtr);
            }else if (msg.size == 8){
                TimeQtimer = *reinterpret_cast<uint64_t*>(dataPtr);
            }
            len += msg.size;
            dataPtr += msg.size;
            // LOGD("Parsed QTimer: 0x%lx (size=%u)\n", TimeQtimer, msg.size);
        }

        // Parse message text TSV entry
        if (len + sizeof(tsv_header) > bufSize) break;
        msg = *reinterpret_cast<tsv_header*>(dataPtr);
        len += sizeof(tsv_header);
        dataPtr += sizeof(tsv_header);

        if (msg.type == TSV_TYPE_BYTE_ARRAY && len + msg.size <= bufSize){
            // Format log entry with timestamp and message
            oss << "[ " << std::fixed << std::setprecision(9)
                << TimeStamp / 1000000000.0 << " 0x" << std::hex << TimeQtimer << "]   ";
            oss.write(dataPtr, msg.size);
            if (msg.size == 0 || dataPtr[msg.size - 1] != '\n') {
                oss << "\n";
            }
            log_ptr->logs.push_back(oss.str());
            entries_parsed++;

            if (entries_parsed <= 5) { // Log first few entries for debugging
                LOGD("Parsed log entry %zu: timestamp=%.9f, qtimer=0x%lx, size=%u\n",
                     entries_parsed, TimeStamp / 1000000000.0, TimeQtimer, msg.size);
            }

            oss.str("");
            oss.clear();
            len += msg.size;
            dataPtr += msg.size;
        }
    }

    LOGD("log parsing completed: %zu log entries parsed from %zu bytes\n",
         entries_parsed, bufSize);
}

void IPCLog::appendBuffer(std::vector<char>& destBuf, void* sourceBuf, size_t length) {
    size_t currentSize = destBuf.size();
    destBuf.resize(currentSize + length);
    memcpy(destBuf.data() + currentSize, sourceBuf, length);
}


/**
 * @brief Save all IPC logs to individual files for offline analysis
 *
 * Creates an "ipc_log" directory in the current working directory and saves
 * each IPC context's log entries to a separate file named after the context.
 * This allows for offline analysis and archival of IPC debugging information.
 *
 * The function validates each context's magic number before processing and
 * parses log pages if they haven't been cached yet. Each log file contains
 * the formatted log entries with timestamps and message text.
 */
void IPCLog::save_ipc_log(){
    LOGD("Starting to save IPC logs for %zu contexts\n", ipc_list.size());

    // Create base directory path for IPC log files
    std::stringstream base_path = get_curpath();
    base_path << "/ipc_log/";
    std::string base_path_str = base_path.str();

    LOGD("Creating IPC log directory: %s\n", base_path_str.c_str());

    // Create directory with read/write/execute permissions for owner and group
    int mkdir_result = mkdir(base_path_str.c_str(), 0777);
    if (mkdir_result != 0 && errno != EEXIST) {
        LOGE("Failed to create directory %s: %s\n", base_path_str.c_str(), strerror(errno));
        return;
    } else if (mkdir_result == 0) {
        LOGD("Successfully created directory: %s\n", base_path_str.c_str());
    } else {
        LOGD("Directory already exists: %s\n", base_path_str.c_str());
    }

    size_t contexts_processed = 0;
    size_t contexts_saved = 0;
    size_t total_entries_saved = 0;

    // Process each IPC context
    for (const auto& log_ptr : ipc_list) {
        contexts_processed++;

        LOGD("Processing context %zu/%zu: %s (addr=0x%lx)\n",
             contexts_processed, ipc_list.size(), log_ptr->name.c_str(), log_ptr->addr);

        // Validate log page magic number before processing
        uint32_t magic = read_ulong(log_ptr->nd_read_page + field_offset(ipc_log_page,hdr) + field_offset(ipc_log_page_header,magic),"magic");
        if (magic != IPC_LOGGING_MAGIC_NUM){
            LOGE("Invalid magic number 0x%x for context %s (expected 0x%x), skipping\n",
                 magic, log_ptr->name.c_str(), IPC_LOGGING_MAGIC_NUM);
            continue;
        }
        // Parse log pages if not already cached
        if (log_ptr->logs.empty()){
            LOGD("Log entries not cached for context %s, parsing now\n", log_ptr->name.c_str());
            parser_ipc_log_page(log_ptr);
        } else {
            LOGD("Using cached log entries (%zu entries) for context %s\n",
                 log_ptr->logs.size(), log_ptr->name.c_str());
        }

        // Skip contexts with no log entries
        if (log_ptr->logs.empty()) {
            LOGD("No log entries found for context %s, skipping file creation\n", log_ptr->name.c_str());
            continue;
        }

        // Create output file path
        std::string ipc_file_path = base_path_str + log_ptr->name;
        LOGD("Creating log file: %s\n", ipc_file_path.c_str());

        // Open file for writing (binary mode to preserve exact formatting)
        FILE* ipc_file = fopen(ipc_file_path.c_str(), "wb");
        if (!ipc_file) {
            LOGE("Failed to open file %s for writing: %s\n", ipc_file_path.c_str(), strerror(errno));
            continue;
        }

        LOGD("Successfully opened file %s for writing\n", ipc_file_path.c_str());

        // Write all log entries to file
        size_t entries_written = 0;
        size_t bytes_written = 0;

        for (const auto& log : log_ptr->logs){
            size_t log_size = log.size();
            size_t written = fwrite(log.c_str(), 1, log_size, ipc_file);

            if (written != log_size) {
                LOGE("Failed to write complete log entry %zu (wrote %zu/%zu bytes)\n",
                     entries_written, written, log_size);
            } else {
                entries_written++;
                bytes_written += written;
            }
        }

        // Close file and report results
        fclose(ipc_file);

        LOGD("File write completed for context %s: %zu entries, %zu bytes\n",
             log_ptr->name.c_str(), entries_written, bytes_written);

        // Report success to user
        PRINT( "Save %s to %s\n", log_ptr->name.c_str(), ipc_file_path.c_str());

        contexts_saved++;
        total_entries_saved += entries_written;
    }

    // Log final statistics
    LOGD("IPC log save operation completed:\n");
    LOGD("  - Total contexts processed: %zu\n", contexts_processed);
    LOGD("  - Contexts successfully saved: %zu\n", contexts_saved);
    LOGD("  - Total log entries saved: %zu\n", total_entries_saved);
    LOGD("  - Output directory: %s\n", base_path_str.c_str());

    if (contexts_saved == 0) {
        LOGE("No IPC contexts were saved - check for valid contexts and permissions\n");
    } else {
        LOGD("Successfully saved %zu IPC contexts to %s\n", contexts_saved, base_path_str.c_str());
    }
}

/**
 * @brief Parse all IPC log contexts from kernel's ipc_log_context_list
 *
 * Traverses the kernel's global list of IPC log contexts to discover all
 * available IPC logging contexts and their metadata. Each context represents
 * a different IPC module (like GLINK, RPM, MMC, etc.) that uses the IPC
 * logging framework for debugging.
 *
 * The function validates each context's magic number and extracts essential
 * information including page pointers, version, and module name. Invalid
 * contexts are skipped to ensure only valid IPC logs are processed.
 */
void IPCLog::parser_ipc_log(){
    LOGD("Starting to parse IPC log contexts from kernel\n");

    // Check if ipc_log_context_list symbol exists in kernel
    if (!csymbol_exists("ipc_log_context_list")){
        LOGE("ipc_log_context_list symbol doesn't exist in this kernel!\n");
        return;
    }

    // Get the list head address
    size_t list_head = csymbol_value("ipc_log_context_list");
    if (!is_kvaddr(list_head)) {
        LOGE("ipc_log_context_list address 0x%zx is invalid!\n", list_head);
        return;
    }

    LOGD("Found ipc_log_context_list at address 0x%zx\n", list_head);

    // Calculate field offsets for efficient access
    int offset = field_offset(ipc_log_context, list);
    ulong magic_offset = field_offset(ipc_log_context, magic);
    ulong name_offset = field_offset(ipc_log_context, name);
    ulong version_offset = field_offset(ipc_log_context, version);
    ulong first_page_offset = field_offset(ipc_log_context, first_page);
    ulong last_page_offset = field_offset(ipc_log_context, last_page);
    ulong write_page_offset = field_offset(ipc_log_context, write_page);
    ulong read_page_offset = field_offset(ipc_log_context, read_page);
    ulong nd_read_page_offset = field_offset(ipc_log_context, nd_read_page);

    LOGD("ipc_log_context offsets: list=%d, magic=%lu, name=%lu, version=%lu\n",
         offset, magic_offset, name_offset, version_offset);

    // Pre-allocate space for expected number of contexts
    ipc_list.reserve(32);

    size_t contexts_found = 0;
    size_t contexts_valid = 0;

    // Traverse the linked list of IPC log contexts
    for (const auto& ctx_addr : for_each_list(list_head, offset)) {
        contexts_found++;
        LOGD("Processing IPC context %zu at address 0x%lx\n", contexts_found, ctx_addr);

        // Read the context structure from memory
        void *ctx_buf = read_struct(ctx_addr, "ipc_log_context");
        if (!ctx_buf) {
            LOGE("Failed to read ipc_log_context structure at address 0x%lx\n", ctx_addr);
            continue;
        }

        // Validate context magic number
        uint32_t magic = UINT(ctx_buf + magic_offset);
        if (magic != IPC_LOG_CONTEXT_MAGIC_NUM){
            LOGE("Invalid magic number 0x%x at context 0x%lx (expected 0x%x)\n",
                 magic, ctx_addr, IPC_LOG_CONTEXT_MAGIC_NUM);
            FREEBUF(ctx_buf);
            continue;
        }
        // Create new IPC log context object
        std::shared_ptr<ipc_log> log_ptr = std::make_shared<ipc_log>();
        log_ptr->addr = ctx_addr;

        // Extract context name
        log_ptr->name = read_cstring(ctx_addr + name_offset, 32, "name");
        if (log_ptr->name.empty()) {
            LOGD("Context at 0x%lx has empty name, using address as identifier\n", ctx_addr);
            log_ptr->name = "unnamed_" + std::to_string(ctx_addr);
        }

        // Extract context fields from structure
        log_ptr->version = UINT(ctx_buf + version_offset);
        log_ptr->first_page = ULONG(ctx_buf + first_page_offset);
        log_ptr->last_page = ULONG(ctx_buf + last_page_offset);
        log_ptr->write_page = ULONG(ctx_buf + write_page_offset);
        log_ptr->read_page = ULONG(ctx_buf + read_page_offset);
        log_ptr->nd_read_page = ULONG(ctx_buf + nd_read_page_offset);

        FREEBUF(ctx_buf);

        // Log extracted context information
        LOGD("Extracted context info for '%s':\n", log_ptr->name.c_str());
        LOGD("  - Version: %u\n", log_ptr->version);
        LOGD("  - First page: 0x%lx\n", log_ptr->first_page);
        LOGD("  - Last page: 0x%lx\n", log_ptr->last_page);
        LOGD("  - Write page: 0x%lx\n", log_ptr->write_page);
        LOGD("  - Read page: 0x%lx\n", log_ptr->read_page);
        LOGD("  - ND read page: 0x%lx\n", log_ptr->nd_read_page);

        // Validate page addresses
        bool valid_pages = true;
        if (!is_kvaddr(log_ptr->first_page)) {
            LOGE("Invalid first_page address 0x%lx for context %s\n",
                 log_ptr->first_page, log_ptr->name.c_str());
            valid_pages = false;
        }
        if (!is_kvaddr(log_ptr->nd_read_page)) {
            LOGE("Invalid nd_read_page address 0x%lx for context %s\n",
                 log_ptr->nd_read_page, log_ptr->name.c_str());
            valid_pages = false;
        }

        if (!valid_pages) {
            LOGE("Context %s has invalid page addresses, skipping\n", log_ptr->name.c_str());
            continue;
        }

        // Add valid context to list
        ipc_list.push_back(std::move(log_ptr));
        contexts_valid++;

        LOGD("Successfully added context %s to IPC list (valid context %zu)\n",
             ipc_list.back()->name.c_str(), contexts_valid);
    }

    // Log final statistics
    LOGD("IPC context parsing completed:\n");
    LOGD("  - Total contexts found: %zu\n", contexts_found);
    LOGD("  - Valid contexts added: %zu\n", contexts_valid);
    LOGD("  - Final ipc_list size: %zu\n", ipc_list.size());

    if (contexts_valid == 0) {
        LOGE("No valid IPC contexts found - IPC logging may not be available\n");
    } else {
        LOGD("Successfully discovered %zu IPC logging contexts\n", contexts_valid);
    }
}

void IPCLog::print_ipc_info(){
    std::ostringstream oss;
    oss  << std::left << std::setw(VADDR_PRLEN)  << "ipc_log_context" << " "
            << std::left << std::setw(7)            << "Version"            << " "
            << std::left << std::setw(VADDR_PRLEN)  << "first_page"         << " "
            << std::left << std::setw(VADDR_PRLEN)  << "last_page"          << " "
            << std::left << std::setw(VADDR_PRLEN)  << "write_page"         << " "
            << std::left << std::setw(VADDR_PRLEN)  << "read_page"          << " "
            << std::left << "Name"
            << "\n";
    for (const auto& log_ptr : ipc_list) {
        oss << std::left << std::setw(VADDR_PRLEN)  << std::hex << log_ptr->addr         << " "
            << std::left << std::setw(7)            << std::dec << log_ptr->version      << " "
            << std::left << std::setw(VADDR_PRLEN)  << std::hex << log_ptr->first_page   << " "
            << std::left << std::setw(VADDR_PRLEN)  << std::hex << log_ptr->last_page    << " "
            << std::left << std::setw(VADDR_PRLEN)  << std::hex << log_ptr->write_page   << " "
            << std::left << std::setw(VADDR_PRLEN)  << std::hex << log_ptr->read_page    << " "
            << std::left << log_ptr->name
            << "\n";
    }
    PRINT( "%s \n",oss.str().c_str());
}

#pragma GCC diagnostic pop
