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

#include "logcatR.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

/**
 * Constructor - Initialize LogcatR parser for Android R
 * @param swap: Shared pointer to swap information
 */
LogcatR::LogcatR(std::shared_ptr<Swapinfo> swap) : Logcat(swap){
    LOGD("Initializing Logcat parser for Android R\n");
}

/**
 * Parse log buffer address using two strategies:
 * 1. Static BSS section lookup
 * 2. std::list pattern matching in allocator regions
 * @return: Virtual address of log buffer, or 0 if not found
 */
ulong LogcatR::parser_logbuf_addr(){
    LOGD("Starting log buffer address parsing\n");
    size_t logbuf_addr;
    // Initialize LogBufferElement structure
    struct_init(LogBufferElement);
    // Strategy 1: Find log buffer from BSS section
    if (struct_size(LogBufferElement) != -1 && !logd_symbol.empty()){
        LOGD("Strategy 1: Searching in BSS section\n");
        logbuf_addr = get_logbuf_addr_from_bss();
        if (logbuf_addr != 0){
            LOGE("Successfully found logbuf in BSS at %#lx\n", logbuf_addr);
            return logbuf_addr;
        }
        LOGD("BSS search failed, trying std::list search\n");
    }

    // Strategy 2: Find log buffer by searching for std::list pattern
    LOGD("Strategy 2: Searching for std::list pattern\n");
    auto start = std::chrono::high_resolution_clock::now();

    // VMA filter: only check readable/writable allocator regions
    auto vma_callback = [&](std::shared_ptr<vma_struct> vma_ptr) -> bool {
        if (!(vma_ptr->vm_flags & VM_READ) || !(vma_ptr->vm_flags & VM_WRITE)) {
            LOGE("vma_callback: skipping vma (missing READ or WRITE flags)\n");
            return false;
        }
        // Only check allocator regions (jemalloc)
        if (vma_ptr->name.find("alloc") == std::string::npos){
            LOGE("vma_callback: skipping vma (name doesn't contain 'alloc')\n");
            return false;
        }
        LOGD("vma_callback: vma passed validation\n");
        return true;
    };

    // Object validator: check if this node is a valid LogBufferElement
    auto obj_callback = [&](ulong node_addr) -> bool {
        // Validate node address
        if (!is_uvaddr(node_addr,tc_logd)){
            LOGE("obj_callback: invalid node address 0x%lx\n", node_addr);
            return false;
        }

        // Read pointer to LogBufferElement (std::list stores pointers)
        ulong data_addr = node_addr + 2 * task_ptr->get_pointer_size();
        data_addr = task_ptr->uread_pointer(data_addr);
        if (!is_uvaddr(data_addr,tc_logd)){
            LOGE("obj_callback: invalid data address 0x%lx\n", data_addr);
            return false;
        }

        // Read LogBufferElement structure
        std::vector<char> buf = task_ptr->uread_obj<LogBufferElement>(data_addr);
        if (buf.size() == 0){
            LOGE("obj_callback: failed to read LogBufferElement at 0x%lx\n", data_addr);
            return false;
        }
        LOGD("obj_callback: successfully read LogBufferElement (%zu bytes)\n", buf.size());
        // Validate LogBufferElement fields
        LogBufferElement* element = reinterpret_cast<LogBufferElement*>(buf.data());

        // Check dropped flag (should be 0 or 1)
        if (element->mDropped > 1){
            LOGE("obj_callback: invalid mDropped=%u (expected 0 or 1)\n", element->mDropped);
            return false;
        }

        // Check log ID (should be 0-8 for valid log types)
        if (element->mLogId > 8){
            LOGE("obj_callback: invalid mLogId=%u (expected 0-8)\n", element->mLogId);
            return false;
        }
        LOGD("obj_callback: valid LogBufferElement - mLogId=%u, mDropped=%u\n",
             element->mLogId, element->mDropped);
        return true;
    };
    logbuf_addr = get_stdlist(vma_callback, obj_callback);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    LOGD("std::list search completed in %.6f seconds\n", elapsed.count());

    if (logbuf_addr != 0){
        LOGD("Successfully found logbuf from std::list at %#lx\n", logbuf_addr);
        return logbuf_addr;
    }
    LOGD("Error: Failed to find log buffer address\n");
    return 0;
}

/**
 * Find log buffer address from BSS section
 * Looks for static logBuf variable in logd binary
 * @return: Address of log buffer, or 0 if not found
 */
size_t LogcatR::get_logbuf_addr_from_bss(){
    LOGD("Searching for logBuf in BSS section\n");
    // Get address of static logBuf variable
    ulong logbuf_addr = task_ptr->get_var_addr_by_bss(logd_symbol, "logBuf");
    if (logbuf_addr == 0){
        LOGE("logBuf variable not found in BSS\n");
        return 0;
    }
    // Dereference pointer: static LogBuffer* logBuf = nullptr
    logbuf_addr = task_ptr->uread_ulong(logbuf_addr);
    LOGD("Found logBuf variable at %#lx\n", logbuf_addr);
    return logbuf_addr;
}

/**
 * Parse log buffer and extract all log entries
 * Iterates through std::list of LogBufferElement pointers
 * @param buf_addr: Address of log buffer (std::list head)
 */
void LogcatR::parser_logbuf(ulong buf_addr){
    LOGD("Parsing log buffer at %#lx\n", buf_addr);
    int element_count = 0;
    ulong LogBufferElement_addr = 0;
    // Iterate through all elements in the std::list
    for(auto data_node: task_ptr->for_each_stdlist(buf_addr)){
        // Read pointer to LogBufferElement
        LogBufferElement_addr = task_ptr->uread_ulong(data_node);
        parser_LogBufferElement(LogBufferElement_addr);
        element_count++;
    }
    LOGD("Processed %d log buffer elements\n", element_count);
}

/**
 * Parse a single LogBufferElement and add to log list
 * @param vaddr: Virtual address of LogBufferElement structure
 */
void LogcatR::parser_LogBufferElement(ulong vaddr){
    LOGD("Parsing LogBufferElement at %#lx\n", vaddr);
    // Validate address
    if (!is_uvaddr(vaddr,tc_logd)){
        LOGE("Invalid LogBufferElement address: %#lx\n", vaddr);
        return;
    }
    // Read LogBufferElement structure
    std::vector<char> buf = task_ptr->uread_obj<LogBufferElement>(vaddr);
    if (buf.size() == 0){
        LOGE("Failed to read LogBufferElement at %#lx\n", vaddr);
        return;
    }
    LogBufferElement* element = reinterpret_cast<LogBufferElement*>(buf.data());
    // Skip dropped log entries
    if (element->mDropped == 1){
        LOGE("Skipping dropped log entry\n");
        return;
    }
    // Validate log ID
    if (element->mLogId < MAIN || element->mLogId > KERNEL) {
        LOGE("Invalid log ID: %d\n", element->mLogId);
        return;
    }
    // Create log entry
    std::shared_ptr<LogEntry> log_ptr = std::make_shared<LogEntry>();
    log_ptr->uid = element->mUid;
    log_ptr->pid = element->mPid;
    log_ptr->tid = element->mTid;
    log_ptr->logid = (LOG_ID)element->mLogId;
    log_ptr->timestamp = formatTime(element->mRealTime.tv_sec, element->mRealTime.tv_nsec);
    LOGD("Log entry: pid=%u tid=%u uid=%u logid=%d msglen=%u\n",
                element->mPid, element->mTid, element->mUid, element->mLogId, element->mMsgLen);
    // Parse message based on log type
    if (log_ptr->logid == SYSTEM || log_ptr->logid == MAIN || log_ptr->logid == KERNEL
        || log_ptr->logid == RADIO || log_ptr->logid == CRASH){
        // System logs: parse as text with priority and tag
        std::vector<char> log_msg = task_ptr->read_data(reinterpret_cast<ulong>(element->mMsg), element->mMsgLen);
        if(log_msg.size() != 0){
            parser_system_log(log_ptr, log_msg.data(), element->mMsgLen);
        } else {
            LOGE("Failed to read system log message\n");
        }
    }else{
        // Event logs: parse as binary event data
        std::vector<char> log_msg = task_ptr->read_data(reinterpret_cast<ulong>(element->mMsg), element->mMsgLen);
        if(log_msg.size() != 0){
            parser_event_log(log_ptr, log_msg.data(), element->mMsgLen);
        } else {
            LOGE("Failed to read event log message\n");
        }
    }
    // Add to log list
    log_list.push_back(log_ptr);
    return;
}

#pragma GCC diagnostic pop
