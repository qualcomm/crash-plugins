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

#ifndef PSTORE_DEFS_H_
#define PSTORE_DEFS_H_

#include "plugin.h"
#include "logcat/logcat.h"
#include <ctime>

/**
 * @struct android_pmsg_log_header_t
 * @brief Android PMSG log header structure
 *
 * Header structure for Android persistent message (pmsg) logs stored in pstore.
 */
typedef struct __attribute__((__packed__)) {
    uint8_t magic;      // Magic byte ('l' for log)
    uint16_t len;       // Total length of log entry
    uint16_t uid;       // User ID
    uint16_t pid;       // Process ID
} android_pmsg_log_header_t;

/**
 * @struct android_log_header_t
 * @brief Android log header structure
 *
 * Header structure containing log buffer ID, thread ID, and timestamp.
 */
typedef struct __attribute__((__packed__)) {
    uint8_t id;         // Log buffer ID (main, system, events, etc.)
    uint16_t tid;       // Thread ID
    log_time realtime;  // Timestamp
} android_log_header_t;

/**
 * @class Pstore
 * @brief Plugin for analyzing pstore (persistent storage) logs
 *
 * Provides commands to extract and display various types of logs stored in
 * pstore including pmsg, console, ftrace, and oops/panic logs.
 */
class Pstore : public ParserPlugin {
private:
    // Priority level mapping array for log level conversion
    const std::array<LogLevel, 9> priorityMap = {{
        LogLevel::LOG_DEFAULT,
        LogLevel::LOG_DEFAULT,
        LogLevel::LOG_VERBOSE,
        LogLevel::LOG_DEBUG,
        LogLevel::LOG_INFO,
        LogLevel::LOG_WARN,
        LogLevel::LOG_ERROR,
        LogLevel::LOG_FATAL,
        LogLevel::LOG_SILENT
    }};

    /**
     * @brief Print ftrace log from pstore
     *
     * Extracts and displays ftrace logs stored in pstore ramoops.
     */
    void print_ftrace_log();

    /**
     * @brief Print oops/panic log from pstore
     *
     * Extracts and displays kernel oops/panic logs stored in pstore.
     */
    void print_oops_log();

    /**
     * @brief Print console log from pstore
     *
     * Extracts and displays console logs stored in pstore.
     */
    void print_console_log();

    /**
     * @brief Print Android pmsg logs from pstore
     *
     * Extracts and displays Android persistent message logs.
     */
    void print_pmsg();

    /**
     * @brief Extract and parse pmsg logs from buffer
     * @param logbuf Buffer containing raw pmsg log data
     *
     * Parses the pmsg log buffer and extracts individual log entries.
     */
    void extract_pmsg_logs(std::vector<char> &logbuf);

    /**
     * @brief Parse Android system log entry
     * @param timestamp Log timestamp string
     * @param uid User ID
     * @param tid Thread ID
     * @param pid Process ID
     * @param logbuf Log message buffer
     * @param msg_len Message length
     *
     * Parses and formats system/main/radio log entries.
     */
    void parser_system_log(std::string timestamp, uint16_t uid, uint16_t tid, uint16_t pid, char *logbuf, uint16_t msg_len);

    /**
     * @brief Parse Android event log entry
     * @param timestamp Log timestamp string
     * @param uid User ID
     * @param tid Thread ID
     * @param pid Process ID
     * @param logbuf Log message buffer
     * @param msg_len Message length
     *
     * Parses and formats binary event log entries.
     */
    void parser_event_log(std::string timestamp, uint16_t uid, uint16_t tid, uint16_t pid, char* logbuf, uint16_t msg_len);

    /**
     * @brief Get event log entry from buffer
     * @param pos Current position in buffer
     * @param data Data buffer pointer
     * @param len Total buffer length
     * @return LogEvent structure containing parsed event
     *
     * Parses a single event log entry (int, long, float, string, or list).
     */
    LogEvent get_event(size_t pos, char* data, size_t len);

    /**
     * @brief Append source buffer to destination buffer
     * @param destBuf Destination vector buffer
     * @param sourceBuf Source buffer pointer
     * @param length Length to append
     *
     * Helper function to append data to a vector buffer.
     */
    void appendBuffer(std::vector<char> &destBuf, void *sourceBuf, size_t length);

    /**
     * @brief Format timestamp to readable string
     * @param tv_sec Seconds since epoch
     * @param tv_nsec Nanoseconds
     * @return Formatted timestamp string (YY-MM-DD HH:MM:SS.microseconds)
     */
    std::string formatTime(uint32_t tv_sec, uint32_t tv_nsec);

    /**
     * @brief Get log level character representation
     * @param level Log level enum
     * @return Single character representing log level (V/D/I/W/E/F/S)
     */
    std::string getLogLevelChar(LogLevel level);

public:
    /**
     * @brief Constructor
     */
    Pstore();

    /**
     * @brief Main command handler
     *
     * Processes command-line arguments and dispatches to appropriate functions.
     */
    void cmd_main(void) override;

    /**
     * @brief Initialize structure field offsets
     *
     * Initializes kernel structure offsets for ramoops/pstore parsing.
     */
    void init_offset(void) override;

    /**
     * @brief Initialize command metadata
     *
     * Sets up command name, help text, and usage examples.
     */
    void init_command(void) override;

    DEFINE_PLUGIN_INSTANCE(Pstore)
};

#endif // PSTORE_DEFS_H_
