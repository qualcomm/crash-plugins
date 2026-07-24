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

#ifndef LOGCAT_LE_DEFS_H_
#define LOGCAT_LE_DEFS_H_

#include "logcat.h"

/**
 * LogBufferElement_LE structure - represents a single log entry in Android LE (Legacy Edition)
 * This is the main data structure used by Android LE's logd to store log entries
 * Each log entry is stored as a LogBufferElement_LE in a std::list
 *
 * Key differences from Android R:
 * - Has virtual destructor (mVptr is vtable pointer)
 * - Includes mSequence field for ordering
 * - Different field ordering
 *
 * The structure uses a union to save space:
 * - mMsgLen: Length of log message (normal case)
 * - mDropped: Number of dropped logs (when this represents dropped logs)
 */
struct LogBufferElement_LE {
    // Virtual destructor: virtual ~LogBufferElement();
    uint64_t mVptr;                // Virtual table pointer (for virtual destructor)
    const uint32_t mLogId;         // Log buffer ID (MAIN, SYSTEM, EVENTS, etc.)
    const uint32_t mUid;           // User ID of the logging process
    const uint32_t mPid;           // Process ID of the logging process
    const uint32_t mTid;           // Thread ID of the logging thread
    char *mMsg;                    // Pointer to log message text

    // Union: either message length or dropped count
    union {
        const unsigned short mMsgLen;   // Length of log message
        unsigned short mDropped;        // Flag: 1 if this represents dropped logs
    };

    const uint64_t mSequence;      // Sequence number for ordering log entries
    struct log_time mRealTime;     // Timestamp when log was created
};

/**
 * LogcatLE class - Parser for Android LE (Legacy Edition) logcat format
 * Handles LogBufferElement_LE format stored in std::list
 * Inherits from base Logcat class
 *
 * Key features:
 * - Supports virtual destructor validation via vtable pointer
 * - Handles sequence numbers for log ordering
 * - Compatible with older Android versions
 */
class LogcatLE : public Logcat {
private:
    // Private helper methods
    ulong parser_logbuf_addr() override;                  // Find log buffer address (2 strategies)
    void parser_LogBufferElement(ulong vaddr);            // Parse single LogBufferElement_LE

public:
    // Constructor
    LogcatLE(std::shared_ptr<Swapinfo> swap);            // Initialize with swap information

    // Override methods from base class
    void parser_logbuf(ulong buf_addr) override;         // Parse log buffer and extract entries
    size_t get_logbuf_addr_from_bss() override;          // Strategy 1: BSS section lookup
};

#endif // LOGCAT_LE_DEFS_H_
