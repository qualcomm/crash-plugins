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

#ifndef LOGCAT_R_DEFS_H_
#define LOGCAT_R_DEFS_H_

#include "logcat.h"

/**
 * LogBufferElement structure - represents a single log entry in Android R
 * This is the main data structure used by Android R's logd to store log entries
 * Each log entry is stored as a LogBufferElement in a std::list
 *
 * The structure uses unions to save space:
 * - For system logs: mMsg points to message text, mMsgLen contains message length
 * - For event logs: mTag contains event tag ID, mDroppedCount may be used
 * - mDropped flag indicates if this entry represents dropped logs
 */
struct __attribute__((__packed__)) LogBufferElement {
    const uint32_t mUid;           // User ID of the logging process
    const uint32_t mPid;           // Process ID of the logging process
    const uint32_t mTid;           // Thread ID of the logging thread
    struct log_time mRealTime;     // Timestamp when log was created

    // Union: either message pointer or event tag
    union {
        char *mMsg;                // Pointer to log message text (system logs)
        int32_t mTag;              // Event tag ID (event logs)
    };

    // Union: either message length or dropped count
    union {
        const uint16_t mMsgLen;    // Length of log message
        uint16_t mDroppedCount;    // Number of dropped logs (when mDropped=1)
    };

    const uint8_t mLogId;          // Log buffer ID (MAIN, SYSTEM, EVENTS, etc.)
    uint8_t mDropped;              // Flag: 1 if this represents dropped logs, 0 otherwise
};

/**
 * LogcatR class - Parser for Android R logcat format
 * Handles LogBufferElement format stored in std::list
 * Inherits from base Logcat class
 */
class LogcatR : public Logcat {
private:
    // Private helper methods
    ulong parser_logbuf_addr() override;                  // Find log buffer address (2 strategies)
    void parser_LogBufferElement(ulong vaddr);            // Parse single LogBufferElement

public:
    // Constructor
    LogcatR(std::shared_ptr<Swapinfo> swap);             // Initialize with swap information

    // Override methods from base class
    void parser_logbuf(ulong buf_addr) override;         // Parse log buffer and extract entries
    size_t get_logbuf_addr_from_bss() override;          // Strategy 1: BSS section lookup
};

#endif // LOGCAT_R_DEFS_H_
