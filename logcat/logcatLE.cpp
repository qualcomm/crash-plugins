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

#include "logcatLE.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

/**
 * Constructor - Initialize LogcatLE parser for Android LE (Legacy Edition)
 * @param swap: Shared pointer to swap information
 */
LogcatLE::LogcatLE(std::shared_ptr<Swapinfo> swap) : Logcat(swap){
    LOGD("Initializing LogcatLE parser for Android LE\n");
}

/**
 * Parse log buffer address using two strategies:
 * 1. Static BSS section lookup
 * 2. std::list pattern matching with vtable validation
 * @return: Virtual address of log buffer, or 0 if not found
 */
ulong LogcatLE::parser_logbuf_addr(){
    LOGD("Starting log buffer address parsing\n");
    size_t logbuf_addr;
    // Initialize LogBufferElement structure
    struct_init(LogBufferElement);
    // Strategy 1: Find log buffer from BSS section
    if (struct_size(LogBufferElement) != -1 && !logd_symbol.empty()){
        LOGD("Strategy 1: Searching in BSS section\n");
        logbuf_addr = get_logbuf_addr_from_bss();
        if (logbuf_addr != 0){
            LOGD("Successfully found logbuf in BSS at %#lx\n", logbuf_addr);
            return logbuf_addr;
        }
        LOGD("BSS search failed, trying std::list search\n");
    }

    // Strategy 2: Find log buffer by searching for std::list pattern
    LOGD("Strategy 2: Searching for std::list pattern\n");
    auto start = std::chrono::high_resolution_clock::now();

    // VMA filter: only check readable/writable regions
    auto vma_callback = [&](std::shared_ptr<vma_struct> vma_ptr) -> bool {
        if (!(vma_ptr->vm_flags & VM_READ) || !(vma_ptr->vm_flags & VM_WRITE)) {
            LOGE("vma_callback: skipping vma (missing READ or WRITE flags)\n");
            return false;
        }
        LOGD("vma_callback: vma passed validation\n");
        return true;
    };

    // Object validator: check if this node is a valid LogBufferElement_LE
    auto obj_callback = [&](ulong node_addr) -> bool {
        LOGD("obj_callback: validating node at 0x%lx\n", node_addr);
        // Validate node address
        if (!is_uvaddr(node_addr,tc_logd)){
            LOGE("obj_callback: invalid node address 0x%lx\n", node_addr);
            return false;
        }

        // Read pointer to LogBufferElement_LE (std::list stores pointers)
        ulong data_addr = node_addr + 2 * task_ptr->get_pointer_size();
        data_addr = task_ptr->uread_pointer(data_addr);
        if (!is_uvaddr(data_addr, tc_logd)){
            LOGE("obj_callback: invalid data address 0x%lx\n", data_addr);
            return false;
        }

        // Read LogBufferElement_LE structure
        std::vector<char> buf = task_ptr->uread_obj<LogBufferElement_LE>(data_addr);
        if (buf.size() == 0){
            LOGE("obj_callback: failed to read LogBufferElement_LE at 0x%lx\n", data_addr);
            return false;
        }
        LOGD("obj_callback: successfully read LogBufferElement_LE (%zu bytes)\n", buf.size());
        // Validate LogBufferElement_LE fields
        LogBufferElement_LE* element = reinterpret_cast<LogBufferElement_LE*>(buf.data());

        // Validate vtable pointer (should NOT point to executable region)
        // LogBufferElement has virtual destructor, so first field is vtable pointer
        std::shared_ptr<vma_struct> exec_vma_ptr = task_ptr->get_vma(task_ptr->get_auxv(AT_ENTRY));
        if (exec_vma_ptr) {
            LOGD("obj_callback: exec_vma range [0x%lx - 0x%lx], mVptr=0x%lx\n",
                 exec_vma_ptr->vm_start, exec_vma_ptr->vm_end, element->mVptr);
            if (exec_vma_ptr->vm_start <= element->mVptr && element->mVptr < exec_vma_ptr->vm_end){
                LOGE("obj_callback: invalid mVptr=0x%lx (points to executable region)\n", element->mVptr);
                return false;
            }
        } else {
            LOGE("obj_callback: warning - could not get exec_vma_ptr\n");
        }

        // Validate log ID (0-6 for LE: MAIN, RADIO, EVENTS, SYSTEM, CRASH, KERNEL)
        // enum log_id: LOG_ID_MIN, LOG_ID_MAIN=0, LOG_ID_RADIO, LOG_ID_EVENTS,
        //              LOG_ID_SYSTEM, LOG_ID_CRASH, LOG_ID_KERNEL, LOG_ID_MAX
        if (element->mLogId > 6){
            LOGE("obj_callback: invalid mLogId=%u (expected 0-6)\n", element->mLogId);
            return false;
        }
        LOGD("obj_callback: valid LogBufferElement_LE - mLogId=%u, mVptr=0x%lx\n",
             element->mLogId, element->mVptr);
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
size_t LogcatLE::get_logbuf_addr_from_bss(){
    LOGD("Searching for logBuf in BSS section\n");

    // Get address of static logBuf variable
    ulong logbuf_addr = task_ptr->get_var_addr_by_bss(logd_symbol, "logBuf");
    if (logbuf_addr == 0){
        LOGE("logBuf variable not found in BSS\n");
        return 0;
    }
    // Dereference pointer: static LogBuffer* logBuf = nullptr
    logbuf_addr = task_ptr->uread_ulong(logbuf_addr);
    LOGD("LogBuffer points to: %#lx\n", logbuf_addr);
    return logbuf_addr;
}

/**
 * Parse log buffer and extract all log entries
 * Iterates through std::list of LogBufferElement_LE pointers
 * @param buf_addr: Address of log buffer (std::list head)
 */
void LogcatLE::parser_logbuf(ulong buf_addr){
    LOGD("Parsing log buffer at %#lx\n", buf_addr);
    ulong LogElements_list_addr = buf_addr;
    int element_count = 0;
    ulong LogBufferElement_addr = 0;

    // Iterate through all elements in the std::list
    for(auto data_node: task_ptr->for_each_stdlist(LogElements_list_addr)){
        // Read pointer to LogBufferElement_LE
        LogBufferElement_addr = task_ptr->uread_ulong(data_node);
        parser_LogBufferElement(LogBufferElement_addr);
        element_count++;
    }
    LOGD("Processed %d log buffer elements\n", element_count);
}

/**
 * Parse a single LogBufferElement_LE and add to log list
 * @param vaddr: Virtual address of LogBufferElement_LE structure
 */
void LogcatLE::parser_LogBufferElement(ulong vaddr){
    LOGD("Parsing LogBufferElement_LE at %#lx\n", vaddr);
    // Validate address
    if (!is_uvaddr(vaddr,tc_logd)){
        LOGE("Invalid LogBufferElement_LE address: %#lx\n", vaddr);
        return;
    }
    // Read LogBufferElement_LE structure
    std::vector<char> buf = task_ptr->uread_obj<LogBufferElement_LE>(vaddr);
    if (buf.size() == 0){
        LOGE("Failed to read LogBufferElement_LE at %#lx\n", vaddr);
        return;
    }
    LogBufferElement_LE* element = reinterpret_cast<LogBufferElement_LE*>(buf.data());
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
    LOGD("Log entry: pid=%u tid=%u uid=%u logid=%d msglen=%u seq=%lu\n",
            element->mPid, element->mTid, element->mUid, element->mLogId,
            element->mMsgLen, element->mSequence);
    // Parse message based on log type
    if (log_ptr->logid == SYSTEM || log_ptr->logid == MAIN || log_ptr->logid == KERNEL
        || log_ptr->logid == RADIO || log_ptr->logid == CRASH){
        // System logs: parse as text with priority and tag
        std::vector<char> log_msg = task_ptr->read_data(reinterpret_cast<ulong>(element->mMsg), element->mMsgLen);
        if(log_msg.size() != 0){
            parser_system_log(log_ptr, log_msg.data(), element->mMsgLen);
        } else {
            LOGD("Failed to read system log message\n");
        }
    }else{
        // Event logs: parse as binary event data
        std::vector<char> log_msg = task_ptr->read_data(reinterpret_cast<ulong>(element->mMsg), element->mMsgLen);
        if(log_msg.size() != 0){
            parser_event_log(log_ptr, log_msg.data(), element->mMsgLen);
        } else {
            LOGD("Failed to read event log message\n");
        }
    }

    // Add to log list
    log_list.push_back(log_ptr);
    return;
}

#pragma GCC diagnostic pop
