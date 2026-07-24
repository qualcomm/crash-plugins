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

#ifndef LOGCAT_S_DEFS_H_
#define LOGCAT_S_DEFS_H_

#include "logcat.h"
#include "zstd.h"  // For ZSTD decompression support

/**
 * Logcat offset table structure
 * Stores field offsets for SerializedLogBuffer and related structures
 * Used to access fields when debug symbols are not available
 */
struct logcat_offset_table {
    int SerializedLogBuffer_sequence_;          // Offset to sequence number field
    int SerializedLogBuffer_logs_;              // Offset to logs array field
    int SerializedLogChunk_contents_;           // Offset to contents field in chunk
    int SerializedLogChunk_write_offset_;       // Offset to write offset field
    int SerializedLogChunk_writer_active_;      // Offset to writer active flag
    int SerializedLogChunk_compressed_log_;     // Offset to compressed log field
    int SerializedLogChunk;                     // Size of SerializedLogChunk structure
    int SerializedData_size_;                   // Offset to size field in SerializedData
    int SerializedData_data_;                   // Offset to data pointer in SerializedData
};

/**
 * Logcat size table structure
 * Stores structure sizes for memory layout calculations
 */
struct logcat_size_table {
    int SerializedData;                // Size of SerializedData structure
    int SerializedLogChunk;            // Size of SerializedLogChunk structure
    int SerializedLogBuffer_logs_;     // Size of logs array (96 bytes for 32-bit, 192 for 64-bit)
    int stdlist_node_size;             // Size of std::list node (24 bytes typically)
};

/**
 * Serialized log entry header structure
 * Represents the header of each log entry in the serialized format
 * Followed by variable-length message data
 */
struct SerializedLogEntry {
    uint32_t uid;                  // User ID of logging process
    uint32_t pid;                  // Process ID of logging process
    uint32_t tid;                  // Thread ID of logging thread
    uint64_t sequence;             // Sequence number for ordering
    struct log_time realtime;      // Timestamp of log entry
    uint16_t msg_len;              // Length of message data following this header
}__attribute__((packed));

/**
 * SerializedLogBuffer structure for 32-bit architecture
 * Represents the main log buffer container with virtual function table
 */
typedef struct{
    uint32_t vtpr;                 // Virtual table pointer
    uint32_t reader_list_;         // Pointer to reader list
    uint32_t tags_;                // Pointer to tags
    uint32_t stats_;               // Pointer to statistics
    uint32_t max_size_[8];         // Maximum size for each log type
    list_node32_t logs_[8];        // Array of std::list nodes for each log type
} SerializedLogBuffer32_t;

/**
 * SerializedLogBuffer structure for 64-bit architecture
 * Represents the main log buffer container with virtual function table
 */
typedef struct{
    uint64_t vtpr;                 // Virtual table pointer
    uint64_t reader_list_;         // Pointer to reader list
    uint64_t tags_;                // Pointer to tags
    uint64_t stats_;               // Pointer to statistics
    uint64_t max_size_[8];         // Maximum size for each log type
    list_node64_t logs_[8];        // Array of std::list nodes for each log type
} SerializedLogBuffer64_t;

/**
 * LogcatS class - Parser for Android S and later logcat format
 * Handles SerializedLogBuffer format with compression support
 * Inherits from base Logcat class
 */
class LogcatS : public Logcat {
private:
    // SerializedLogBuffer has 10 virtual functions in its vtable
    const size_t vtbl_size = 10;
    // Structure offset and size tables
    struct logcat_offset_table g_offset;  // Field offsets for structure access
    struct logcat_size_table g_size;      // Structure sizes for memory calculations

    // Private helper methods
    void init_datatype_info();                                                    // Initialize offsets and sizes
    ulong parser_logbuf_addr() override;                                          // Find log buffer address (4 strategies)
    size_t get_SerializedLogBuffer_from_vma();                                    // Strategy 3: VMA scanning
    size_t get_logbuf_addr_from_bss() override;                                   // Strategy 1: BSS section lookup
    size_t get_logbuf_addr_from_register();                                       // Strategy 2: CPU register inspection
    bool check_SerializedLogChunk_list_array(ulong addr);                         // Validate log buffer structure
    void parser_SerializedLogChunk(LOG_ID log_id, ulong vaddr);                   // Parse single log chunk
    void parser_SerializedLogEntry(LOG_ID log_id, char *log_data, uint32_t data_len);  // Parse log entries

public:
    // Constructor
    LogcatS(std::shared_ptr<Swapinfo> swap);  // Initialize with swap information

    // Override methods from base class
    void parser_logbuf(ulong buf_addr) override;  // Parse log buffer and extract entries
};

#endif // LOGCAT_S_DEFS_H_
