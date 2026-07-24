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

#ifndef LOGCAT_DEFS_H_
#define LOGCAT_DEFS_H_

#include "plugin.h"
#include "memory/swapinfo.h"
#include "../utils/utask.h"
#include <array>
#include <chrono>

/**
 * Log buffer ID enumeration
 * Android maintains separate log buffers for different log types
 */
enum LOG_ID {
    MAIN = 0,      // Main application log buffer
    RADIO,         // Radio/telephony log buffer
    EVENTS,        // Binary event log buffer
    SYSTEM,        // System log buffer
    CRASH,         // Crash log buffer
    STATS,         // Statistics log buffer
    SECURITY,      // Security log buffer
    KERNEL,        // Kernel log buffer
    ALL,           // All log buffers (for filtering)
};

/**
 * Log priority/level enumeration
 * Defines the severity level of log messages
 */
enum LogLevel {
    LOG_UNKNOWN = 0,  // Unknown log level
    LOG_DEFAULT = 1,  // Default log level
    LOG_VERBOSE = 2,  // Verbose: detailed information
    LOG_DEBUG = 3,    // Debug: debugging information
    LOG_INFO = 4,     // Info: informational messages
    LOG_WARN = 5,     // Warning: warning messages
    LOG_ERROR = 6,    // Error: error messages
    LOG_FATAL = 7,    // Fatal: fatal error messages
    LOG_SILENT = 8    // Silent: no logging
};

/**
 * Event log data type enumeration
 * Event logs use binary format with typed data
 */
enum EventType {
    TYPE_INT = 0,     // 32-bit integer (int32_t)
    TYPE_LONG = 1,    // 64-bit integer (int64_t)
    TYPE_STRING = 2,  // Variable-length string
    TYPE_LIST = 3,    // List of multiple elements
    TYPE_FLOAT = 4    // Floating point number
};

/**
 * Log entry structure
 * Represents a single log message with all metadata
 */
struct LogEntry {
    LOG_ID logid;           // Log buffer ID (MAIN, SYSTEM, etc.)
    uint32_t uid;           // User ID of the logging process
    uint32_t pid;           // Process ID of the logging process
    uint32_t tid;           // Thread ID of the logging thread
    std::string timestamp;  // Formatted timestamp string
    std::string tag;        // Log tag (component name or event ID)
    LogLevel priority;      // Log priority/severity level
    std::string msg;        // Log message content
};

/**
 * Event log data structure
 * Represents a parsed event log data element
 */
struct LogEvent {
    int type;           // Event data type (TYPE_INT, TYPE_STRING, etc.)
    std::string val;    // String representation of the value
    int len;            // Length of the data in bytes
};

/**
 * Android event log header structure
 * Contains the event tag index
 */
typedef struct __attribute__((__packed__)){
    int32_t tag;  // Event tag index (maps to event name)
} android_event_header_t;

/**
 * Android event log long integer structure
 * Represents a 64-bit integer event value
 */
typedef struct __attribute__((__packed__)){
    int8_t type;   // Event type (TYPE_LONG)
    int64_t data;  // 64-bit integer value
} android_event_long_t;

/**
 * Android event log float structure
 * Represents a floating point event value
 */
typedef struct __attribute__((__packed__)){
    int8_t type;  // Event type (TYPE_FLOAT)
    float data;   // Float value
} android_event_float_t;

/**
 * Android event log integer structure
 * Represents a 32-bit integer event value
 */
typedef struct __attribute__((__packed__)){
    int8_t type;   // Event type (TYPE_INT)
    int32_t data;  // 32-bit integer value
} android_event_int_t;

/**
 * Android event log string structure
 * Represents a variable-length string event value
 */
typedef struct __attribute__((__packed__)){
    int8_t type;      // Event type (TYPE_STRING)
    int32_t length;   // Length of string data
    char data[];      // Variable-length string data
} android_event_string_t;

/**
 * Android event log list structure
 * Represents a list containing multiple event elements
 */
typedef struct __attribute__((__packed__)){
    int8_t type;            // Event type (TYPE_LIST)
    int8_t element_count;   // Number of elements in the list
} android_event_list_t;

/**
 * Log timestamp structure
 * Represents time with second and nanosecond precision
 */
struct log_time {
    uint32_t tv_sec;   // Seconds since epoch
    uint32_t tv_nsec;  // Nanoseconds component
};

/**
 * Logcat parser class - crash utility plugin for extracting Android logs
 * Provides functionality to parse and display logcat logs from kernel crash dumps
 * Supports multiple Android versions with version-specific implementations
 */
class Logcat : public ParserPlugin {
private:
    // Priority level mapping array for converting byte values to LogLevel enum
    const std::array<LogLevel, 9> priorityMap = {{
        LogLevel::LOG_UNKNOWN,   // 0
        LogLevel::LOG_DEFAULT,   // 1
        LogLevel::LOG_VERBOSE,   // 2
        LogLevel::LOG_DEBUG,     // 3
        LogLevel::LOG_INFO,      // 4
        LogLevel::LOG_WARN,      // 5
        LogLevel::LOG_ERROR,     // 6
        LogLevel::LOG_FATAL,     // 7
        LogLevel::LOG_SILENT     // 8
    }};

    // Private helper methods
    std::string remove_invalid_chars(const std::string &str);  // Filter non-printable characters
    LogEvent get_event(size_t pos, char *data, size_t len);    // Parse event log data
    std::string getLogLevelChar(LogLevel level);                // Convert log level to character

public:
    static bool is_LE;                                     // Little-endian flag
    std::string logd_symbol;                               // Symbol name for logd
    std::vector<std::shared_ptr<LogEntry>> log_list;      // Parsed log entries
    std::shared_ptr<UTask> task_ptr;                      // Task context for memory access
    struct task_context *tc_logd;                         // Task context for logd process
    std::shared_ptr<Swapinfo> swap_ptr;                   // Swap information pointer

    // Constructor and destructor
    Logcat(std::shared_ptr<Swapinfo> swap);  // Initialize with swap info
    ~Logcat();                                // Cleanup resources

    // Log parsing methods
    void parser_system_log(std::shared_ptr<LogEntry> log_ptr, char *logbuf, uint16_t msg_len);  // Parse system logs
    void parser_event_log(std::shared_ptr<LogEntry> log_ptr, char *logbuf, uint16_t msg_len);   // Parse event logs
    void parser_logcat_log();                                                                     // Main parsing entry point
    void print_logcat_log(LOG_ID id);                                                            // Print parsed logs

    // Utility methods
    std::string formatTime(uint32_t tv_sec, long tv_nsec);  // Format timestamp for display
    size_t get_stdlist(const std::function<bool(std::shared_ptr<vma_struct>)> &vma_callback,    // Search for std::list in memory
                       const std::function<bool(ulong)> &obj_callback);
    std::string find_symbol(std::string name);               // Find symbol by name

    // Plugin interface methods
    void cmd_main(void) override;         // Command entry point
    void init_offset(void) override;      // Initialize field offsets
    void init_command(void) override;     // Initialize command help

    // Version-specific virtual methods (implemented by derived classes)
    virtual ulong parser_logbuf_addr()=0;              // Parse log buffer address from memory
    virtual void parser_logbuf(ulong buf_addr)=0;      // Parse log buffer contents
    virtual size_t get_logbuf_addr_from_bss()=0;       // Get log buffer address from BSS section
};

#endif // LOGCAT_DEFS_H_
