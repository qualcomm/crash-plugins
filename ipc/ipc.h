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

#ifndef IPC_LOG_DEFS_H_
#define IPC_LOG_DEFS_H_

#include "plugin.h"

/**
 * @brief TSV (Type-Size-Value) header structure for IPC log entries
 *
 * This structure defines the header format used in IPC logging system.
 * Each log entry starts with this header that specifies the type and size
 * of the following data payload.
 */
struct tsv_header {
    unsigned char type;     // Type of the data (timestamp, pointer, etc.)
    unsigned char size;     // Size of the data payload in bytes
};

/**
 * @brief IPC log context information structure
 *
 * This structure contains metadata and state information for an IPC logging
 * context. Each IPC module (like glink, rpm, mmc, etc.) has its own context
 * that manages a circular buffer of log pages.
 */
struct ipc_log {
    ulong addr;                         // Address of ipc_log_context structure
    std::string name;                   // Name of the IPC module (e.g., "glink_pkt")
    uint32_t version;                   // Version of the IPC logging format
    ulong first_page;                   // Address of the first log page
    ulong last_page;                    // Address of the last log page
    ulong write_page;                   // Address of current write page
    ulong read_page;                    // Address of current read page
    ulong nd_read_page;                 // Address of non-destructive read page
    std::vector<std::string> logs;      // Parsed log entries (cached)
};

// TSV type definitions for different data types in IPC logs
#define TSV_TYPE_INVALID        0       // Invalid/unknown type
#define TSV_TYPE_TIMESTAMP      1       // Timestamp data (32 or 64-bit)
#define TSV_TYPE_POINTER        2       // Pointer value
#define TSV_TYPE_INT32          3       // 32-bit integer
#define TSV_TYPE_BYTE_ARRAY     4       // Byte array (log message text)
#define TSV_TYPE_QTIMER         5       // QTimer value (hardware timer)

// Magic numbers for validation
#define IPC_LOG_CONTEXT_MAGIC_NUM   0x25874452  // Magic for ipc_log_context
#define IPC_LOGGING_MAGIC_NUM       0x52784425  // Magic for ipc_log_page

/**
 * @brief IPC Log parser and analyzer class
 *
 * This class provides functionality to parse and analyze IPC (Inter-Process
 * Communication) logs from Qualcomm systems. IPC logs contain debugging
 * information from various subsystems like:
 * - GLINK (Generic Link protocol)
 * - RPM (Resource Power Manager)
 * - MMC (MultiMediaCard controller)
 * - QRTR (Qualcomm IPC Router)
 *
 * The class supports:
 * - Listing all available IPC log contexts
 * - Displaying logs from specific IPC modules
 * - Saving logs to files for offline analysis
 * - Parsing circular buffer structures with proper timestamp handling
 */
class IPCLog : public ParserPlugin {
private:
    // List of all discovered IPC log contexts
    std::vector<std::shared_ptr<ipc_log>> ipc_list;

    /**
     * @brief Parse all IPC log contexts from kernel
     *
     * Traverses the kernel's ipc_log_context_list to discover all
     * available IPC logging contexts and their metadata.
     */
    void parser_ipc_log();

    /**
     * @brief Display summary information for all IPC contexts
     *
     * Shows a table with context addresses, versions, page pointers,
     * and module names for all discovered IPC log contexts.
     */
    void print_ipc_info();

    /**
     * @brief Save all IPC logs to files
     *
     * Creates individual files for each IPC context containing
     * their parsed log entries. Files are saved in an "ipc_log"
     * subdirectory with names matching the IPC module names.
     */
    void save_ipc_log();

    /**
     * @brief Parse log entries from IPC log pages
     * @param log_ptr Pointer to IPC log context to parse
     *
     * Traverses the circular buffer of log pages for the specified
     * context, parsing TSV-formatted log entries and extracting
     * timestamps and message text.
     */
    void parser_ipc_log_page(std::shared_ptr<ipc_log> log_ptr);

    /**
     * @brief Append data to a buffer
     * @param destBuf Destination buffer to append to
     * @param sourceBuf Source data to append
     * @param length Number of bytes to append
     *
     * Helper function to safely append data from source buffer
     * to destination buffer, handling buffer resizing as needed.
     */
    void appendBuffer(std::vector<char> &destBuf, void *sourceBuf, size_t length);

public:
    /**
     * @brief Default constructor
     *
     * Initializes the IPC log parser with default settings.
     */
    IPCLog();

    /**
     * @brief Display logs from a specific IPC module
     * @param name Name of the IPC module to display logs for
     *
     * Searches for the specified IPC module by name and displays
     * all parsed log entries with timestamps and formatted output.
     */
    void print_ipc_log(const std::string &name);

    /**
     * @brief Main command entry point
     *
     * Processes command-line arguments and dispatches to appropriate
     * handlers. Supports options for:
     * -a         : Display all IPC contexts
     * -l <name>  : Display logs from specific module
     * -s         : Save all logs to files
     */
    void cmd_main(void) override;

    /**
     * @brief Initialize kernel structure field offsets
     *
     * Sets up field offsets for IPC logging structures including
     * ipc_log_context, ipc_log_page, and related headers.
     */
    void init_offset(void) override;

    /**
     * @brief Initialize command metadata
     *
     * Sets up command name, description, usage information, and examples
     * for the IPC log analysis command.
     */
    void init_command(void) override;

    // Plugin instance definition macro
    DEFINE_PLUGIN_INSTANCE(IPCLog)
};

#endif // IPC_LOG_DEFS_H_
