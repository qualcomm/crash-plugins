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

#include "logcatS.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

/**
 * Constructor - Initialize LogcatS parser for Android S and later
 * @param swap: Shared pointer to swap information
 */
LogcatS::LogcatS(std::shared_ptr<Swapinfo> swap) : Logcat(swap){
    LOGD("Initializing Logcat parser for Android S+\n");
}

/**
 * Initialize data type information and field offsets
 * Handles both cases: with and without debug symbols
 */
void LogcatS::init_datatype_info(){
    field_init(SerializedLogBuffer, logs_);
    // Case 1: No debug symbols available - use hardcoded offsets
    if (field_offset(SerializedLogBuffer, logs_) == -1){
        LOGD("Debug symbols not available, using hardcoded offsets\n");
        // Set offsets based on architecture (64-bit vs 32-bit)
        bool is_64bit = (BITS64() && !task_ptr->is_compat());
        g_offset.SerializedLogBuffer_logs_ = is_64bit ? 96 : 48;
        g_offset.SerializedLogBuffer_sequence_ = is_64bit ? 288 : 144;
        g_offset.SerializedLogChunk_contents_ = 0;
        g_offset.SerializedLogChunk_write_offset_ = is_64bit ? 16 : 8;
        g_offset.SerializedLogChunk_writer_active_ = is_64bit ? 24 : 16;
        g_offset.SerializedLogChunk_compressed_log_ = is_64bit ? 40 : 32;
        g_offset.SerializedData_size_ = is_64bit ? 8 : 4;
        g_offset.SerializedData_data_ = 0;
        g_size.SerializedData = is_64bit ? 16 : 8;
        g_size.SerializedLogChunk = is_64bit ? 80 : 56;
        g_size.SerializedLogBuffer_logs_ = is_64bit ? 192 : 96;
        g_size.stdlist_node_size = (g_offset.SerializedLogBuffer_sequence_ - g_offset.SerializedLogBuffer_logs_) / 8;
        LOGD("Using %s architecture offsets\n", is_64bit ? "64-bit" : "32-bit");
    }else{
        // Case 2: Debug symbols available - use actual field offsets
        LOGD("Debug symbols available, using actual field offsets\n");
        // Initialize all field offsets from debug symbols
        field_init(SerializedLogBuffer, sequence_);
        field_init(SerializedLogChunk, contents_);
        field_init(SerializedLogChunk, write_offset_);
        field_init(SerializedLogChunk, writer_active_);
        field_init(SerializedLogChunk, compressed_log_);
        field_init(SerializedData, size_);
        field_init(SerializedData, data_);
        struct_init(SerializedData);
        struct_init(SerializedLogChunk);

        // Store all offsets and sizes
        g_offset.SerializedLogBuffer_logs_ = field_offset(SerializedLogBuffer, logs_);
        g_offset.SerializedLogBuffer_sequence_ = field_offset(SerializedLogBuffer, sequence_);
        g_offset.SerializedLogChunk_contents_ = field_offset(SerializedLogChunk, contents_);
        g_offset.SerializedLogChunk_write_offset_ = field_offset(SerializedLogChunk, write_offset_);
        g_offset.SerializedLogChunk_writer_active_ = field_offset(SerializedLogChunk, writer_active_);
        g_offset.SerializedLogChunk_compressed_log_ = field_offset(SerializedLogChunk, compressed_log_);
        g_offset.SerializedData_size_ = field_offset(SerializedData, size_);
        g_offset.SerializedData_data_ = field_offset(SerializedData, data_);
        g_size.SerializedData = struct_size(SerializedData);
        g_size.SerializedLogChunk = struct_size(SerializedLogChunk);
        g_size.SerializedLogBuffer_logs_ = field_size(SerializedLogBuffer, logs_);
        g_size.stdlist_node_size = (g_offset.SerializedLogBuffer_sequence_ - g_offset.SerializedLogBuffer_logs_) / 8;
    }
    LOGD("SerializedLogChunk size: %d, stdlist_node_size: %d\n",
            g_size.SerializedLogChunk, g_size.stdlist_node_size);
}

/**
 * Parse log buffer address using multiple strategies
 * Tries 4 different methods in order of efficiency:
 * 1. Static BSS section lookup
 * 2. CPU register inspection (ARM64 only)
 * 3. VMA scanning for SerializedLogBuffer structure
 * 4. std::list pattern matching
 * @return: Virtual address of log buffer, or 0 if not found
 */
ulong LogcatS::parser_logbuf_addr(){
    LOGD("Starting log buffer address parsing\n");
    size_t logbuf_addr;
    init_datatype_info();
    // Strategy 1: Find log buffer address from BSS section
    LOGD("Strategy 1: Searching in BSS section\n");
    field_init(SerializedLogBuffer, logs_);
    if (field_offset(SerializedLogBuffer, logs_) != -1 && !logd_symbol.empty()){
        LOGD("Looking for static logbuf in BSS\n");
        logbuf_addr = get_logbuf_addr_from_bss();
        if (logbuf_addr != 0){
            logbuf_addr += g_offset.SerializedLogBuffer_logs_;
            LOGD("Found potential logbuf at %#lx, validating...\n", logbuf_addr);
            if (check_SerializedLogChunk_list_array(logbuf_addr)){
                LOGD("Successfully found logbuf in BSS at %#lx\n", logbuf_addr);
                return logbuf_addr;
            }
        }
    }else{
        LOGD("Skipping due to No logd symbol found! \n");
    }

    // Timing variables for performance measurement
    std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::nanoseconds> start, end;
    std::chrono::duration<double> elapsed;

    // Strategy 2: Find log buffer address from CPU registers (ARM64 only)
    LOGD("Strategy 2: Searching in CPU registers\n");
    if (BITS64() && !task_ptr->is_compat()) {
        LOGD("Checking CPU registers (64-bit mode)\n");
        start = std::chrono::high_resolution_clock::now();
        logbuf_addr = get_logbuf_addr_from_register();
        end = std::chrono::high_resolution_clock::now();
        elapsed = end - start;
        LOGD("Register search completed in %.6f seconds\n", elapsed.count());
        if (logbuf_addr != 0){
            LOGD("Successfully found logbuf from registers at %#lx\n", logbuf_addr);
            return logbuf_addr;
        }
    } else {
        LOGD("Skipping register search (not 64-bit or compat mode)\n");
    }

    // Strategy 3: Find log buffer address by scanning VMA for SerializedLogBuffer structure
    LOGD("Strategy 3: Scanning VMA for SerializedLogBuffer\n");
    /*
     * VMA Scanning Strategy:
     *
     *      Scan VMA         SerializedLogBuffer         Core
     *   --------------    -->  -------------        --------------
     *   |            |    |    |   vtbl    |----    |            |
     *   |------------|test|    |-----------|   |in  |------------|
     *   |  VMA (RW)  |-----    |-----------|   ---> | logd .text |
     *   |------------|  |    --|reader_list|        |------------|
     *   |  VMA (RW)  |---  --| |   tags    |        |            |
     *   |------------|     | --|   stats   |        |------------|
     *   |            |     |   |-----------|   ---> |    stack   |
     *   --------------     |   -------------   |in  --------------
     *                      |--------------------
     *
     * Find logbuf based on the class layout characteristics of SerializedLogBuffer
     * Validates vtable pointer and member pointers point to expected memory regions
     */
    start = std::chrono::high_resolution_clock::now();
    logbuf_addr = get_SerializedLogBuffer_from_vma();
    end = std::chrono::high_resolution_clock::now();
    elapsed = end - start;
    LOGD("VMA scan completed in %.6f seconds\n", elapsed.count());
    if (logbuf_addr != 0){
        LOGD("Successfully found SerializedLogBuffer at %#lx\n", logbuf_addr);
        return logbuf_addr + g_offset.SerializedLogBuffer_logs_;
    }

    // Strategy 4: Find log buffer address by searching for std::list pattern
    LOGD("Strategy 4: Searching for std::list pattern\n");
    start = std::chrono::high_resolution_clock::now();

    // VMA filter: only check readable/writable allocator regions
    auto vma_callback = [&](std::shared_ptr<vma_struct> vma_ptr) -> bool {
        if (!(vma_ptr->vm_flags & VM_READ) || !(vma_ptr->vm_flags & VM_WRITE)) {
            return false;
        }
        // Only check allocator regions (jemalloc or scudo)
        if (vma_ptr->name.find("alloc") == std::string::npos &&
            vma_ptr->name.find("scudo:primary") == std::string::npos){
            return false;
        }
        return true;
    };

    // Object validator: check if this node is a valid SerializedLogChunk
    auto obj_callback = [&](ulong node_addr) -> bool {
        // Validate node address
        if (!is_uvaddr(node_addr,tc_logd)){
            return false;
        }

        // Read SerializedLogChunk data
        ulong data_addr = node_addr + 2 * task_ptr->get_pointer_size();
        std::vector<char> chunk_buf = task_ptr->read_data(data_addr,g_size.SerializedLogChunk);
        if (chunk_buf.size() == 0){
            return false;
        }

        // Parse chunk fields based on architecture
        ulong contents_data = 0;
        ulong contents_size = 0;
        ulong write_offset = 0;
        bool write_active = 0;
        ulong compressed_data = 0;
        ulong compressed_size = 0;
        // Extract fields based on architecture (64-bit vs 32-bit)
        if (BITS64() && !task_ptr->is_compat()) {
            contents_data = ULONG(chunk_buf.data() + g_offset.SerializedLogChunk_contents_ + g_offset.SerializedData_data_) & task_ptr->vaddr_mask;
            contents_size = ULONG(chunk_buf.data() + g_offset.SerializedLogChunk_contents_ + g_offset.SerializedData_size_) & task_ptr->vaddr_mask;
            write_offset = UINT(chunk_buf.data() + g_offset.SerializedLogChunk_write_offset_ ) & task_ptr->vaddr_mask;
            write_active = BOOL(chunk_buf.data() + g_offset.SerializedLogChunk_writer_active_ ) & task_ptr->vaddr_mask;
            compressed_data = ULONG(chunk_buf.data() + g_offset.SerializedLogChunk_compressed_log_ + g_offset.SerializedData_data_) & task_ptr->vaddr_mask;
            compressed_size = ULONG(chunk_buf.data() + g_offset.SerializedLogChunk_compressed_log_ + g_offset.SerializedData_size_) & task_ptr->vaddr_mask;
        }else{
            contents_data = UINT(chunk_buf.data() + g_offset.SerializedLogChunk_contents_ + g_offset.SerializedData_data_) & task_ptr->vaddr_mask;
            contents_size = UINT(chunk_buf.data() + g_offset.SerializedLogChunk_contents_ + g_offset.SerializedData_size_) & task_ptr->vaddr_mask;
            write_offset = UINT(chunk_buf.data() + g_offset.SerializedLogChunk_write_offset_ ) & task_ptr->vaddr_mask;
            write_active = BOOL(chunk_buf.data() + g_offset.SerializedLogChunk_writer_active_ ) & task_ptr->vaddr_mask;
            compressed_data = UINT(chunk_buf.data() + g_offset.SerializedLogChunk_compressed_log_ + g_offset.SerializedData_data_) & task_ptr->vaddr_mask;
            compressed_size = UINT(chunk_buf.data() + g_offset.SerializedLogChunk_compressed_log_ + g_offset.SerializedData_size_) & task_ptr->vaddr_mask;
        }

        // Validate chunk: check for uncompressed or compressed chunk
        if (write_active == true && contents_data != 0 && contents_size != 0 &&
            write_offset != 0 && write_offset < contents_size) {
            // Valid uncompressed chunk
            return true;
        } else if (write_active == false && compressed_data != 0 &&
            compressed_size != 0 && write_offset != 0 &&  compressed_size < write_offset) {
            // Valid compressed chunk
            return true;
        }
        return false;
    };

    logbuf_addr = get_stdlist(vma_callback, obj_callback);
    end = std::chrono::high_resolution_clock::now();
    elapsed = end - start;
    LOGD("std::list search completed in %.6f seconds\n", elapsed.count());
    if (logbuf_addr != 0){
        LOGD("Successfully found logbuf from std::list at %#lx\n", logbuf_addr);
        return logbuf_addr;
    }

    LOGD("Error: Failed to find log buffer address using all strategies\n");
    return 0;
}

/**
 * Find log buffer based on virtual class structure
 * Scans VMA regions for SerializedLogBuffer by validating class layout
 * @return: Address of SerializedLogBuffer, or 0 if not found
 */
size_t LogcatS::get_SerializedLogBuffer_from_vma() {
    // VMA filter: only check writable regions
    auto vma_callback = [&](std::shared_ptr<vma_struct> vma_ptr) -> bool {
        if (!(vma_ptr->vm_flags & VM_WRITE)) {
            return false;
        }
        return true;
    };

    // Get executable name for vtable validation
    std::string libname = task_ptr->uread_cstring(task_ptr->get_auxv(AT_EXECFN), 64);
    LOGD("Executable name: %s\n", libname.c_str());

    if (BITS64() && !task_ptr->is_compat()) {
        LOGD("Searching for 64-bit SerializedLogBuffer\n");

        // Object validator for 64-bit: check if pointers point to stack
        auto obj_callback = [&](SerializedLogBuffer64_t* obj) -> bool {
            bool match = true;
            std::shared_ptr<vma_struct> stack_vma_ptr = task_ptr->get_vma(task_ptr->get_auxv(AT_PLATFORM));
            if(stack_vma_ptr){
                // Validate that reader_list, tags, and stats pointers are in stack region
                match &= task_ptr->is_contains(stack_vma_ptr, obj->reader_list_);
                match &= task_ptr->is_contains(stack_vma_ptr, obj->tags_);
                match &= task_ptr->is_contains(stack_vma_ptr, obj->stats_);
            }
            return match;
        };
        return task_ptr->search_obj<SerializedLogBuffer64_t,uint64_t>(libname, false, vma_callback, obj_callback,10);
    } else {
        LOGD("Searching for 32-bit SerializedLogBuffer\n");

        // Object validator for 32-bit: check if pointers point to stack
        auto obj_callback = [&](SerializedLogBuffer32_t* obj) -> bool {
            bool match = true;
            std::shared_ptr<vma_struct> stack_vma_ptr = task_ptr->get_vma(task_ptr->get_auxv(AT_PLATFORM));
            if(stack_vma_ptr){
                // Validate that reader_list, tags, and stats pointers are in stack region
                match &= task_ptr->is_contains(stack_vma_ptr, obj->reader_list_);
                match &= task_ptr->is_contains(stack_vma_ptr, obj->tags_);
                match &= task_ptr->is_contains(stack_vma_ptr, obj->stats_);
            }
            return match;
        };
        return task_ptr->search_obj<SerializedLogBuffer32_t,uint32_t>(libname, false, vma_callback, obj_callback,10);
    }
}

/**
 * Find log buffer address from CPU registers (ARM64 only)
 * Checks X21, X22, X23 registers which may contain SerializedLogBuffer pointer
 * @return: Address of log buffer, or 0 if not found
 */
size_t LogcatS::get_logbuf_addr_from_register(){
#if defined(ARM64)
    LOGD("Reading ARM64 CPU registers\n");

    // Get register context from stack
    ulong pt_regs_addr = GET_STACKTOP(tc_logd->task) - machdep->machspec->user_eframe_offset;
    uint64_t* regs = (uint64_t*)read_memory(pt_regs_addr, 31 * sizeof(uint64_t), "user_pt_regs");

    LOGD("user_pt_regs address: %#lx\n", pt_regs_addr);
    for (int i = 0; i < 31; i++){
        LOGD("regs[%d]: %#lx\n", i, regs[i] & task_ptr->vaddr_mask);
    }

    // Check X21 register
    uint64_t x21 = regs[21] & task_ptr->vaddr_mask;
    if (x21 > 0 && is_uvaddr(x21, tc_logd)){
        LOGD("Checking X21 register: %#lx\n", x21);
        x21 += g_offset.SerializedLogBuffer_logs_;
        if (check_SerializedLogChunk_list_array(x21)){
            LOGD("Found valid logbuf in X21\n");
            FREEBUF(regs);
            return x21;
        }
    }

    // Check X22 register (commonly used for 'this' pointer)
    uint64_t x22 = regs[22] & task_ptr->vaddr_mask;
    if (x22 > 0 && is_uvaddr(x22, tc_logd)){
        LOGD("Checking X22 register: %#lx\n", x22);
        x22 += g_offset.SerializedLogBuffer_logs_;
        if (check_SerializedLogChunk_list_array(x22)){
            LOGD("Found valid logbuf in X22\n");
            FREEBUF(regs);
            return x22;
        }
    }

    // Check X23 register
    uint64_t x23 = regs[23] & task_ptr->vaddr_mask;
    if (x23 > 0 && is_uvaddr(x23, tc_logd)){
        LOGD("Checking X23 register: %#lx\n", x23);
        x23 += g_offset.SerializedLogBuffer_logs_;
        if (check_SerializedLogChunk_list_array(x23)){
            LOGD("Found valid logbuf in X23\n");
            FREEBUF(regs);
            return x23;
        }
    }

    FREEBUF(regs);
    LOGD("No valid logbuf found in registers\n");
#else
    LOGD("Register search not supported on this architecture\n");
#endif
    return 0;
}

/**
 * Validate SerializedLogChunk list array
 * Checks if the address points to a valid array of std::list structures
 * @param addr: Address to validate
 * @return: true if valid, false otherwise
 */
bool LogcatS::check_SerializedLogChunk_list_array(ulong addr){
    LOGD("Validating SerializedLogChunk list array at %#lx\n", addr);
    bool match = true;

    // Check all log buffer types (MAIN through KERNEL)
    for (size_t i = 0; i < ALL; i++){
        ulong log_list_addr = addr + g_size.stdlist_node_size * i;
        ulong res_addr = 0;
        ulong list_size = 0;  // Not used but required by API

        // Validate std::list structure based on architecture
        if (BITS64() && !task_ptr->is_compat()) {
            res_addr = task_ptr->check_stdlist<list_node64_t, uint64_t>(log_list_addr, nullptr, list_size);
        } else {
            res_addr = task_ptr->check_stdlist<list_node32_t, uint32_t>(log_list_addr, nullptr, list_size);
        }
        LOGD("Checking Log[%zu] at %#lx, result: %#lx\n", i, log_list_addr, res_addr);

        // Valid if check returns the same address
        if (res_addr > 0 && res_addr == log_list_addr){
            match &= true;
        }else{
            match &= false;
        }
    }
    LOGD("Validation result: %s \n", match ? "PASS" : "FAIL");
    return match;
}

/**
 * Find log buffer address from BSS section
 * Looks for static log_buffer variable in logd binary
 * @return: Address of log buffer, or 0 if not found
 */
size_t LogcatS::get_logbuf_addr_from_bss(){
    LOGD("Searching for log_buffer in BSS section\n");

    // Get address of static log_buffer variable
    size_t logbuf_addr = task_ptr->get_var_addr_by_bss(logd_symbol, "log_buffer");
    if (!is_uvaddr(logbuf_addr,tc_logd)){
        LOGE("Invalid log_buffer address: %#lx\n", logbuf_addr);
        return 0;
    }
    // Dereference pointer: static LogBuffer* log_buffer = nullptr
    logbuf_addr = task_ptr->uread_ulong(logbuf_addr);
    LOGD("Found log_buffer variable at %#zx\n", logbuf_addr);
    return logbuf_addr;
}

/**
 * Parse log buffer and extract all log entries
 * Iterates through all log types (MAIN, RADIO, EVENTS, etc.)
 * @param buf_addr: Address of log buffer array
 */
void LogcatS::parser_logbuf(ulong buf_addr){
    LOGD("Parsing log buffer at %#lx\n", buf_addr);
    // Calculate size of each log list (8 lists total for different log types)
    size_t log_size = g_size.SerializedLogBuffer_logs_ / 8;
    // Iterate through all log types
    for (size_t i = 0; i <= KERNEL; i++){
        if (i >= MAIN && i <= KERNEL) {
            ulong log_list_addr = buf_addr + i * log_size;
            LOGD("Processing log type %zu at %#lx\n", i, log_list_addr);

            int chunk_count = 0;
            // Iterate through all chunks in this log list
            for(auto data_node: task_ptr->for_each_stdlist(log_list_addr)){
                parser_SerializedLogChunk(static_cast<LOG_ID>(i), data_node);
                chunk_count++;
            }
            LOGD("Processed %d LogChunk for log type %zu\n", chunk_count, i);
        }
    }
}

/**
 * Parse a single SerializedLogChunk
 * Handles both compressed and uncompressed chunks
 * @param log_id: Log buffer ID (MAIN, SYSTEM, etc.)
 * @param vaddr: Virtual address of SerializedLogChunk
 */
void LogcatS::parser_SerializedLogChunk(LOG_ID log_id, ulong vaddr){
    // Validate address
    if (!is_uvaddr(vaddr,tc_logd)){
        LOGE("Invalid chunk address: %#lx\n", vaddr);
        return;
    }
    LOGD("Parsing LogChunk at %#lx for log_id %d\n", vaddr, log_id);
    // Read chunk structure
    ulong contents_data = 0;
    int write_offset = 0;
    bool writer_active = false;
    ulong compressed_data = 0;
    ulong compressed_size = 0;

    std::vector<char> chunk_buf = task_ptr->read_data(vaddr,g_size.SerializedLogChunk);
    if (chunk_buf.size() == 0){
        LOGD("Failed to read chunk data\n");
        return;
    }
    // Extract chunk fields based on architecture
    if (BITS64() && !task_ptr->is_compat()) {
        contents_data = ULONG(chunk_buf.data() + g_offset.SerializedLogChunk_contents_ + g_offset.SerializedData_data_) & task_ptr->vaddr_mask;
        write_offset = UINT(chunk_buf.data() + g_offset.SerializedLogChunk_write_offset_ ) & task_ptr->vaddr_mask;
        writer_active = BOOL(chunk_buf.data() + g_offset.SerializedLogChunk_writer_active_ );
        compressed_data = ULONG(chunk_buf.data() + g_offset.SerializedLogChunk_compressed_log_ + g_offset.SerializedData_data_) & task_ptr->vaddr_mask;
        compressed_size = ULONG(chunk_buf.data() + g_offset.SerializedLogChunk_compressed_log_ + g_offset.SerializedData_size_) & task_ptr->vaddr_mask;
    }else{
        contents_data = UINT(chunk_buf.data() + g_offset.SerializedLogChunk_contents_ + g_offset.SerializedData_data_) & task_ptr->vaddr_mask;
        write_offset = UINT(chunk_buf.data() + g_offset.SerializedLogChunk_write_offset_ ) & task_ptr->vaddr_mask;
        writer_active = BOOL(chunk_buf.data() + g_offset.SerializedLogChunk_writer_active_ );
        compressed_data = UINT(chunk_buf.data() + g_offset.SerializedLogChunk_compressed_log_ + g_offset.SerializedData_data_) & task_ptr->vaddr_mask;
        compressed_size = UINT(chunk_buf.data() + g_offset.SerializedLogChunk_compressed_log_ + g_offset.SerializedData_size_) & task_ptr->vaddr_mask;
    }

    // Handle compressed chunk
    if (writer_active == false){
        LOGD("Processing compressed chunk (size: %lu)\n", compressed_size);
        // Validate compressed data
        if (!is_uvaddr(compressed_data,tc_logd) || compressed_size == 0){
            LOGE("Invalid compressed data: addr=%#lx, size=%lu\n", compressed_data, compressed_size);
            return;
        }

        // Read compressed data
        std::vector<char> compressed_log = task_ptr->read_data(compressed_data,compressed_size);
        if (compressed_log.size() == 0){
            LOGE("Failed to read compressed log data\n");
            return;
        }

        // Get decompressed size
        size_t const rBuffSize = ZSTD_getFrameContentSize(compressed_log.data(), compressed_size);
        if (rBuffSize == ZSTD_CONTENTSIZE_ERROR || rBuffSize == ZSTD_CONTENTSIZE_UNKNOWN) {
            LOGE("Error determining decompressed size for %#lx bytes\n", compressed_size);
            return;
        }
        LOGD("Decompressing %lu bytes to %zu bytes\n", compressed_size, rBuffSize);
        // Decompress data
        std::vector<char> buffer(rBuffSize);
        size_t const dSize = ZSTD_decompress(buffer.data(), buffer.size(), compressed_log.data(), compressed_size);
        if (ZSTD_isError(dSize)) {
            LOGE("Decompression failed: %s\n", ZSTD_getErrorName(dSize));
            return;
        }
        LOGD("Successfully decompressed %zu bytes\n", dSize);
        parser_SerializedLogEntry(log_id, buffer.data(),buffer.size());
    }else{
        // Handle uncompressed chunk
        LOGD("Processing uncompressed chunk (offset: %d)\n", write_offset);
        // Validate uncompressed data
        if (!is_uvaddr(contents_data,tc_logd) || write_offset == 0){
            LOGE("Invalid uncompressed data: addr=%#lx, offset=%d\n", contents_data, write_offset);
            return;
        }

        // Read uncompressed data
        uint32_t log_len = write_offset;
        std::vector<char> log_data = task_ptr->read_data(contents_data,log_len);
        if (log_data.size() == 0){
            LOGE("Failed to read uncompressed log data\n");
            return;
        }
        LOGD("Read %u bytes of uncompressed data\n", log_len);
        parser_SerializedLogEntry(log_id, log_data.data(), log_len);
    }
}

/**
 * Parse serialized log entries from chunk data
 * Extracts individual log entries and adds them to log_list
 * @param log_id: Log buffer ID
 * @param log_data: Pointer to log data buffer
 * @param data_len: Length of log data
 */
void LogcatS::parser_SerializedLogEntry(LOG_ID log_id, char* log_data, uint32_t data_len){
    LOGD("Parsing %u bytes of LogEntry for log_id %d\n", data_len, log_id);

    size_t pos = 0;
    char* logbuf = log_data;
    int entry_size = sizeof(SerializedLogEntry);
    int entry_count = 0;

    // Parse all log entries in the buffer
    while (pos + entry_size <= data_len){
        // Parse log entry header
        SerializedLogEntry* entry = (SerializedLogEntry*)logbuf;
        std::shared_ptr<LogEntry> log_ptr = std::make_shared<LogEntry>();
        log_ptr->logid = log_id;
        log_ptr->pid = entry->pid;
        log_ptr->tid = entry->tid;
        log_ptr->uid = entry->uid;
        log_ptr->timestamp = formatTime(entry->realtime.tv_sec,entry->realtime.tv_nsec);

        pos += entry_size;
        logbuf += entry_size;

        // Skip entries with no message
        if(entry->msg_len <= 0){
            continue;
        }

        // Validate entry to detect memory corruption
        const size_t remaining = data_len - pos;
        if(log_ptr->pid == 0 || log_ptr->pid > 100000 || log_ptr->uid > 100000 ||
           log_ptr->tid > 100000 || (entry->msg_len > remaining)){
            LOGD("Warning: Abnormal entry detected - pid:%d uid:%d tid:%d msg_len:%#x remaining:%zu\n",
                    log_ptr->pid, log_ptr->uid, log_ptr->tid, entry->msg_len, remaining);
            LOGD("Saving corrupted data to file for analysis\n");

            // Save corrupted data for debugging
            char filename[256];
            snprintf(filename, sizeof(filename), "logcat_corrupt_%u.bin", data_len);
            FILE *file = fopen(filename, "wb");
            if (file) {
                fwrite(log_data, 1, data_len, file);
                fclose(file);
                LOGD("Saved to %s\n", filename);
            }
            break;
        }

        // Parse message content based on log type
        if (log_id == MAIN || log_id == SYSTEM || log_id == RADIO || log_id == CRASH || log_id == KERNEL){
            parser_system_log(log_ptr, (char *)logbuf, entry->msg_len);
        }else{
            parser_event_log(log_ptr, (char *)logbuf, entry->msg_len);
        }

        pos += entry->msg_len;
        logbuf += entry->msg_len;
        log_list.push_back(log_ptr);
        entry_count++;
    }

    LOGD("Parsed %d log entries from %u bytes\n", entry_count, data_len);
}

#pragma GCC diagnostic pop
