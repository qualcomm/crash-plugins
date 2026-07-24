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

#include "pstore.h"
#include "logcat/logcat.h"
#include "logger/logger_core.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(Pstore)
#endif

/**
 * @brief Main command entry point for pstore plugin
 *
 * Parses command line arguments and dispatches to appropriate handler functions
 * for displaying different types of pstore logs (pmsg, console, ftrace, oops).
 */
void Pstore::cmd_main(void) {
    int c;
    std::string cppString;
    if (argcnt < 2) cmd_usage(pc->curcmd, SYNOPSIS);
    // Parse command line options
    while ((c = getopt(argcnt, args, "pcfo")) != EOF) {
        switch(c) {
            case 'p':
                print_pmsg();
                break;
            case 'c':
                print_console_log();
                break;
            case 'f':
                print_ftrace_log();
                break;
            case 'o':
                print_oops_log();
                break;
            default:
                argerrs++;
                break;
        }
    }

    if (argerrs)
        cmd_usage(pc->curcmd, SYNOPSIS);
}

/**
 * @brief Initialize kernel structure field offsets for pstore
 *
 * Initializes offsets for ramoops_context, persistent_ram_zone, and
 * persistent_ram_buffer structures to enable proper memory access.
 */
void Pstore::init_offset(void) {
    // Initialize ramoops_context structure fields
    field_init(ramoops_context,dprzs);
    field_init(ramoops_context,cprz);
    field_init(ramoops_context,fprzs);
    field_init(ramoops_context,mprz);
    field_init(ramoops_context,phys_addr);
    field_init(ramoops_context,size);
    field_init(ramoops_context,memtype);
    field_init(ramoops_context,record_size);
    field_init(ramoops_context,console_size);
    field_init(ramoops_context,ftrace_size);
    field_init(ramoops_context,pmsg_size);
    struct_init(ramoops_context);

    // Initialize persistent_ram_zone structure fields
    field_init(persistent_ram_zone,paddr);
    field_init(persistent_ram_zone,size);
    field_init(persistent_ram_zone,buffer);

    // Initialize persistent_ram_buffer structure fields
    field_init(persistent_ram_buffer,start);
    field_init(persistent_ram_buffer,size);
}

void Pstore::init_command(void) {
    cmd_name = "pstore";
    help_str_list={
        "pstore",                            /* command name */
        "dump pstore log information",        /* short description */
        "-p \n"
            "  pstore -c\n"
            "  pstore -f\n"
            "  pstore -o\n"
            "  This command dumps the pstore log info.",
        "\n",
        "EXAMPLES",
        "  Display pmsg log:",
        "    %s> pstore -p",
        "       25-04-20 19:03:41.000775 1719  2299  1000   I SDM ClstcAlgorithmAdapter::QueryLibraryRequest():711 Get library config 0, rc 0",
        "       25-04-20 19:03:41.000775 7290  7911  1000   I lcomm.qti.axiom Waiting for a blocking GC Alloc",
        "       25-04-20 19:03:41.000836 2683  8844  1000   W BestClock java.time.DateTimeException: No network time available",
        "\n",
        "  Display console log:",
        "    %s> pstore -c",
        "\n",
        "  Display ftrace log:",
        "    %s> pstore -f",
        "\n",
        "  Display Oops dump log:",
        "    %s> pstore -o",
        "\n",
    };
}

Pstore::Pstore(){}

/**
 * @brief Print ftrace log information from pstore
 *
 * Retrieves and displays the physical address and size of the ftrace log
 * stored in the pstore persistent RAM zone.
 */
void Pstore::print_ftrace_log(){
    // Check if oops_cxt symbol exists in kernel
    if (!csymbol_exists("oops_cxt")){
        LOGE("Pstore print_ftrace_log: oops_cxt doesn't exist in this kernel");
        return;
    }

    ulong cxt_addr = csymbol_value("oops_cxt");
    if (!is_kvaddr(cxt_addr)) {
        LOGE("Pstore print_ftrace_log: oops_cxt address 0x%lx is invalid", cxt_addr);
        return;
    }

    LOGD("Pstore print_ftrace_log: oops_cxt address: 0x%lx", cxt_addr);
    // Read ftrace buffer size from ramoops context
    size_t ftrace_size = read_ulong(cxt_addr + field_offset(ramoops_context,ftrace_size),"ftrace_size");
    if (ftrace_size == 0){
        LOGD("Pstore print_ftrace_log: ftrace_size is 0, no ftrace logs available");
        return;
    }

    LOGD("Pstore print_ftrace_log: ftrace_size: %zu bytes", ftrace_size);

    // Get ftrace persistent RAM zone address
    ulong fprzs_addr = read_pointer(cxt_addr + field_offset(ramoops_context,cprz),"fprzs");
    if (!is_kvaddr(fprzs_addr)) {
        LOGE("Pstore print_ftrace_log: fprzs address 0x%lx is invalid", fprzs_addr);
        return;
    }

    // Read physical address of ftrace zone
    ulong zone_paddr = read_ulong(fprzs_addr + field_offset(persistent_ram_zone,paddr),"paddr");
    ulong buffer_addr = read_pointer(fprzs_addr + field_offset(persistent_ram_zone,buffer),"buffer");
    if (!is_kvaddr(buffer_addr)) {
        LOGE("Pstore print_ftrace_log: buffer address 0x%lx is invalid", buffer_addr);
        return;
    }
    LOGD("Pstore print_ftrace_log: ftrace log physical address: 0x%lx, size: %zu", zone_paddr, ftrace_size);
}

/**
 * @brief Print oops/panic log information from pstore
 *
 * Retrieves and displays the physical address and size of the oops/panic dump
 * stored in the pstore persistent RAM zone.
 */
void Pstore::print_oops_log(){
    // Check if oops_cxt symbol exists in kernel
    if (!csymbol_exists("oops_cxt")){
        LOGE("Pstore print_oops_log: oops_cxt doesn't exist in this kernel");
        return;
    }

    ulong cxt_addr = csymbol_value("oops_cxt");
    if (!is_kvaddr(cxt_addr)) {
        LOGE("Pstore print_oops_log: oops_cxt address 0x%lx is invalid", cxt_addr);
        return;
    }

    LOGD("Pstore print_oops_log: oops_cxt address: 0x%lx", cxt_addr);

    // Read oops record size from ramoops context
    size_t record_size = read_ulong(cxt_addr + field_offset(ramoops_context,record_size),"record_size");
    if (record_size == 0){
        LOGD("Pstore print_oops_log: record_size is 0, no oops logs available");
        return;
    }

    LOGD("Pstore print_oops_log: record_size: %zu bytes", record_size);

    // Get oops persistent RAM zone address
    ulong dprzs_addr = read_pointer(cxt_addr + field_offset(ramoops_context,dprzs),"dprzs");
    if (!is_kvaddr(dprzs_addr)) {
        LOGE("Pstore print_oops_log: dprzs address 0x%lx is invalid", dprzs_addr);
        return;
    }

    // Read physical address of oops zone
    ulong zone_paddr = read_ulong(dprzs_addr + field_offset(persistent_ram_zone,paddr),"paddr");
    ulong buffer_addr = read_pointer(dprzs_addr + field_offset(persistent_ram_zone,buffer),"buffer");
    if (!is_kvaddr(buffer_addr)) {
        LOGE("Pstore print_oops_log: buffer address 0x%lx is invalid", buffer_addr);
        return;
    }

    LOGD("Pstore print_oops_log: oops log physical address: 0x%lx, size: %zu", zone_paddr, record_size);
}

/**
 * @brief Print console log information from pstore
 *
 * Retrieves and displays the physical address and size of the console log
 * stored in the pstore persistent RAM zone.
 */
void Pstore::print_console_log(){
    // Check if oops_cxt symbol exists in kernel
    if (!csymbol_exists("oops_cxt")){
        LOGE("Pstore print_console_log: oops_cxt doesn't exist in this kernel");
        return;
    }

    ulong cxt_addr = csymbol_value("oops_cxt");
    if (!is_kvaddr(cxt_addr)) {
        LOGE("Pstore print_console_log: oops_cxt address 0x%lx is invalid", cxt_addr);
        return;
    }

    LOGD("Pstore print_console_log: oops_cxt address: 0x%lx", cxt_addr);

    // Read console buffer size from ramoops context
    size_t console_size = read_ulong(cxt_addr + field_offset(ramoops_context,console_size),"console_size");
    if (console_size == 0){
        LOGE("Pstore print_console_log: console_size is 0, no console logs available");
        return;
    }

    LOGD("Pstore print_console_log: console_size: %zu bytes", console_size);

    // Get console persistent RAM zone address
    ulong cprz_addr = read_pointer(cxt_addr + field_offset(ramoops_context,cprz),"cprz");
    if (!is_kvaddr(cprz_addr)) {
        LOGE("Pstore print_console_log: cprz address 0x%lx is invalid", cprz_addr);
        return;
    }

    // Read physical address of console zone
    ulong zone_paddr = read_ulong(cprz_addr + field_offset(persistent_ram_zone,paddr),"paddr");
    ulong buffer_addr = read_pointer(cprz_addr + field_offset(persistent_ram_zone,buffer),"buffer");
    if (!is_kvaddr(buffer_addr)) {
        LOGE("Pstore print_console_log: buffer address 0x%lx is invalid", buffer_addr);
        return;
    }

    LOGD("Pstore print_console_log: console log physical address: 0x%lx, size: %zu", zone_paddr, console_size);
}

/**
 * @brief Print Android pmsg (persistent message) logs from pstore
 *
 * Reads the pmsg buffer from pstore persistent RAM, handles circular buffer
 * wrapping, and extracts Android log messages for display.
 */
void Pstore::print_pmsg(){
    // Check if oops_cxt symbol exists in kernel
    if (!csymbol_exists("oops_cxt")){
        LOGE("Pstore print_pmsg: oops_cxt doesn't exist in this kernel");
        return;
    }

    ulong cxt_addr = csymbol_value("oops_cxt");
    if (!is_kvaddr(cxt_addr)) {
        LOGE("Pstore print_pmsg: oops_cxt address 0x%lx is invalid", cxt_addr);
        return;
    }

    LOGD("Pstore print_pmsg: oops_cxt address: 0x%lx", cxt_addr);

    // Read pmsg buffer size from ramoops context
    size_t pmsg_size = read_ulong(cxt_addr + field_offset(ramoops_context,pmsg_size),"pmsg_size");
    if (pmsg_size == 0){
        LOGE("Pstore print_pmsg: pmsg_size is 0, no pmsg logs available");
        return;
    }

    LOGD("Pstore print_pmsg: pmsg_size: %zu bytes", pmsg_size);

    // Get pmsg persistent RAM zone address
    ulong mprz_addr = read_pointer(cxt_addr + field_offset(ramoops_context,mprz),"mprz");
    if (!is_kvaddr(mprz_addr)) {
        LOGE("Pstore print_pmsg: mprz address 0x%lx is invalid", mprz_addr);
        return;
    }

    // Read physical address and buffer info
    ulong zone_paddr = read_ulong(mprz_addr + field_offset(persistent_ram_zone,paddr),"paddr");
    ulong buffer_addr = read_pointer(mprz_addr + field_offset(persistent_ram_zone,buffer),"buffer");
    if (!is_kvaddr(buffer_addr)) {
        LOGE("Pstore print_pmsg: buffer address 0x%lx is invalid", buffer_addr);
        return;
    }

    // Read circular buffer start and size
    int buf_start = read_int(buffer_addr + field_offset(persistent_ram_buffer,start),"start");
    int buf_size = read_int(buffer_addr + field_offset(persistent_ram_buffer,size),"size");

    LOGD("Pstore print_pmsg: buffer start: %d, size: %d, physical addr: 0x%lx", buf_start, buf_size, zone_paddr);
    std::vector<char> logbuf;
    ulong addr = 0;
    size_t len = 0;
    void *data_buf;

    // Handle circular buffer wrapping
    if (buf_start < buf_size){
        // Read from start position to end of buffer
        addr = zone_paddr + buf_start;
        len = buf_size - buf_start;
        LOGD("Pstore print_pmsg: Reading first segment from 0x%lx, length: %zu", addr, len);
        data_buf = read_memory(addr, len,"data1",false);
        appendBuffer(logbuf,data_buf , len);
        FREEBUF(data_buf);

        // Read from beginning of buffer to start position
        addr = zone_paddr;
        len = buf_start;
        LOGD("Pstore print_pmsg: Reading second segment from 0x%lx, length: %zu", addr, len);
        data_buf = read_memory(addr, len,"data2",false);
        appendBuffer(logbuf, data_buf, len);
        FREEBUF(data_buf);
    }else{
        // Buffer not wrapped, read entire buffer
        addr = zone_paddr;
        len = buf_size;
        LOGD("Pstore print_pmsg: Reading entire buffer from 0x%lx, length: %zu", addr, len);
        data_buf = read_memory(addr, len,"data",false);
        appendBuffer(logbuf, data_buf, len);
        FREEBUF(data_buf);
    }
    extract_pmsg_logs(logbuf);
}

/**
 * @brief Extract and parse Android pmsg logs from buffer
 *
 * Parses the pmsg buffer to extract individual Android log entries.
 * Each entry consists of android_pmsg_log_header_t + android_log_header_t + message data.
 * Validates magic numbers and dispatches to appropriate parser based on log type.
 *
 * @param logbuf Buffer containing raw pmsg log data
 */
void Pstore::extract_pmsg_logs(std::vector<char>& logbuf){
    char* logptr = logbuf.data();
    long len = logbuf.size();
    long head_len = sizeof(android_pmsg_log_header_t) + sizeof(android_log_header_t) ;
    while (len > head_len){
        //   ----------------------------------------------------------------------------------------
        //   |    android_pmsg_log_header_t    |   android_log_header_t   |    tag     |   msg      |
        //   ----------------------------------------------------------------------------------------
        android_pmsg_log_header_t pmsgHeader = *reinterpret_cast<android_pmsg_log_header_t*>(logptr);
        android_log_header_t header = *reinterpret_cast<android_log_header_t*>(logptr + sizeof(android_pmsg_log_header_t));
        if (pmsgHeader.magic == 'l' && (header.id < 7 && header.id >=0) && pmsgHeader.len > head_len){
            if (len > pmsgHeader.len ){
                android_pmsg_log_header_t pmsgHeader2 = *reinterpret_cast<android_pmsg_log_header_t*>(logptr + pmsgHeader.len);
                if (pmsgHeader2.magic != 'l'){
                    len -= head_len;
                    logptr += head_len;
                    continue;
                }
            }
            std::string timestamp = formatTime(header.realtime.tv_sec,header.realtime.tv_nsec);
            char* msg_ptr = logptr + head_len;
            int msg_len = pmsgHeader.len - head_len;
            if (header.id == MAIN || header.id == SYSTEM || header.id == RADIO || header.id == CRASH || header.id == KERNEL){
                //   --------------------------------------------------------
                //   |    priority    |          tag         |   log         |
                //   --------------------------------------------------------
                parser_system_log(timestamp, pmsgHeader.uid, header.tid, pmsgHeader.pid, msg_ptr, msg_len);
            }else if (header.id == EVENTS){
                parser_event_log(timestamp, pmsgHeader.uid, header.tid, pmsgHeader.pid, msg_ptr, msg_len);
            }
            len -= pmsgHeader.len;
            logptr += pmsgHeader.len;
        }else{
            len -= 1;
            logptr += 1;
        }
    }
}

/**
 * @brief Parse and format Android system log entry
 *
 * Parses system log messages (MAIN, SYSTEM, RADIO, CRASH, KERNEL buffers).
 * Format: priority byte + tag string + null + message string
 * Sanitizes null bytes and newlines in the message before output.
 *
 * @param timestamp Formatted timestamp string
 * @param uid User ID of the logging process
 * @param tid Thread ID
 * @param pid Process ID
 * @param logbuf Pointer to log message buffer
 * @param msg_len Length of the message buffer
 */
void Pstore::parser_system_log(std::string timestamp, uint16_t uid, uint16_t tid, uint16_t pid, char* logbuf, uint16_t msg_len){
    // Validate priority byte
    if (logbuf[0] < 0 || logbuf[0] >= 9){
        LOGE("Invalid priority byte: %d", logbuf[0]);
        return;
    }

    // Extract priority level from first byte
    LogLevel priority = priorityMap[logbuf[0]];

    // Extract message (skip priority byte)
    std::string msg = std::string(logbuf +1, msg_len - 1);

    // Sanitize message: replace null bytes with spaces
    size_t pos = 0;
    while ((pos = msg.find('\0')) != std::string::npos) {
        msg.replace(pos, 1, " ");
    }

    // Sanitize message: replace newlines with spaces
    while ((pos = msg.find('\n')) != std::string::npos) {
        msg.replace(pos, 1, " ");
    }

    // Format log entry: timestamp pid tid uid level message
    std::ostringstream log;
    log << std::setw(18) << std::left << timestamp << " "
        << std::setw(5) << pid << " "
        << std::setw(5) << tid << " "
        << std::setw(6) << uid << " "
        << getLogLevelChar(priority) << " "
        << msg;
    PRINT( "%s \n",log.str().c_str());
}

/**
 * @brief Parse and format Android event log entry
 *
 * Parses binary event log messages. Event logs contain structured data
 * with type information (int, long, float, string, list).
 * Extracts tag and decodes typed values into human-readable format.
 *
 * @param timestamp Formatted timestamp string
 * @param uid User ID of the logging process
 * @param tid Thread ID
 * @param pid Process ID
 * @param logbuf Pointer to log message buffer
 * @param msg_len Length of the message buffer
 */
void Pstore::parser_event_log(std::string timestamp, uint16_t uid, uint16_t tid, uint16_t pid, char* logbuf, uint16_t msg_len){
    const size_t header_size = sizeof(android_event_header_t);
    size_t pos = 0;
    char* msg_ptr = logbuf;
    LogLevel priority = priorityMap[LOG_INFO];
    std::ostringstream log;
    log << std::setw(18) << std::left << timestamp << " "
        << std::setw(5) << pid << " "
        << std::setw(5) << tid << " "
        << std::setw(6) << uid << " "
        << getLogLevelChar(priority) << " ";
    while (pos < msg_len){
        if (pos + header_size > msg_len){
            break;
        }
        android_event_header_t head = *reinterpret_cast<android_event_header_t*>(msg_ptr);
        msg_ptr += header_size;
        pos += header_size;
        // read the tag
        log << head.tag << " :[";
        LogEvent event = get_event(pos, msg_ptr, msg_len);
        if (event.type == -1) {
            break;
        }
        msg_ptr += event.len;
        pos += event.len;
        if (event.type == TYPE_LIST) {
            std::string list_msg;
            int cnt = std::stoi(event.val);
            for (int i = 0; i < cnt && pos < msg_len; ++i) {
                event = get_event(pos, msg_ptr, msg_len);
                if (!list_msg.empty()) {
                    list_msg += ",";
                }
                list_msg += event.val;
                msg_ptr += event.len;
                pos += event.len;
            }
            log << list_msg;
        } else {
            log << event.val;
        }
        log << "]";
    }
    PRINT( "%s \n",log.str().c_str());
}

/**
 * @brief Decode a single event log value from binary data
 *
 * Parses typed event log data and extracts the value based on type.
 * Supports INT, LONG, FLOAT, STRING, and LIST types.
 * Performs bounds checking to prevent buffer overruns.
 *
 * @param pos Current position in the buffer
 * @param data Pointer to event data
 * @param len Total length of the buffer
 * @return LogEvent structure containing type, value, and length
 */
LogEvent Pstore::get_event(size_t pos, char* data, size_t len) {
    LogEvent event = {-1, "", -1};

    // Check if we have enough data for type byte
    if ((pos + sizeof(int8_t)) >= len) {
        return event;
    }

    // Read event type
    int8_t event_type = *reinterpret_cast<int8_t*>(data);

    switch (event_type) {
        case TYPE_INT:{
            // Parse 32-bit integer event
            if (pos + sizeof(android_event_int_t) > len) {
                return event;
            }
            android_event_int_t event_int = *reinterpret_cast<android_event_int_t*>(data);
            event.len = sizeof(android_event_int_t);
            event.type = event_int.type;
            event.val = std::to_string(event_int.data);
        }
        break;
        case TYPE_LONG:{
            // Parse 64-bit long event
            if (pos + sizeof(android_event_long_t) > len) {
                return event;
            }
            android_event_long_t event_long = *reinterpret_cast<android_event_long_t*>(data);
            event.len = sizeof(android_event_long_t);
            event.type = event_long.type;
            event.val = std::to_string(event_long.data);
        }
        break;
        case TYPE_FLOAT:{
            // Parse floating point event
            if (pos + sizeof(android_event_float_t) > len) {
                return event;
            }
            android_event_float_t event_float = *reinterpret_cast<android_event_float_t*>(data);
            event.len = sizeof(android_event_float_t);
            event.type = event_float.type;
            event.val = std::to_string(event_float.data);
        }
        break;
        case TYPE_LIST:{
            // Parse list header (contains element count)
            if (pos + sizeof(android_event_list_t) > len) {
                return event;
            }
            android_event_list_t event_list = *reinterpret_cast<android_event_list_t*>(data);
            event.len = sizeof(android_event_list_t);
            event.type = event_list.type;
            event.val = std::to_string(event_list.element_count);
        }
        break;
        case TYPE_STRING:{
            // Parse variable-length string event
            if (pos + sizeof(android_event_string_t) > len) {
                return event;
            }
            android_event_string_t event_str = *reinterpret_cast<android_event_string_t*>(data);
            if (pos + sizeof(android_event_string_t) + event_str.length > len) {
                return event;
            }
            event.len = sizeof(android_event_string_t) + event_str.length;
            event.type = event_str.type;
            event.val.assign(data + sizeof(android_event_string_t), event_str.length);
        }
        break;
        default:
            LOGD("Pstore get_event: Unknown event type: %d", event_type);
            break;
    }
    return event;
}

/**
 * @brief Convert log level enum to single character representation
 *
 * Maps Android log level to standard single-character format
 * (V=Verbose, D=Debug, I=Info, W=Warn, E=Error, F=Fatal, S=Silent).
 *
 * @param level Log level enum value
 * @return Single character string representing the log level
 */
std::string Pstore::getLogLevelChar(LogLevel level) {
    switch (level) {
        case LOG_DEFAULT: return "D";
        case LOG_VERBOSE: return "V";
        case LOG_DEBUG: return "D";
        case LOG_INFO: return "I";
        case LOG_WARN: return "W";
        case LOG_ERROR: return "E";
        case LOG_FATAL: return "F";
        case LOG_SILENT: return "S";
        default: return "";
    }
}

/**
 * @brief Append data from source buffer to destination vector
 *
 * Utility function to concatenate binary data to a vector buffer.
 * Resizes the destination buffer and copies data using memcpy.
 *
 * @param destBuf Destination vector to append to
 * @param sourceBuf Source buffer containing data to append
 * @param length Number of bytes to append
 */
void Pstore::appendBuffer(std::vector<char>& destBuf, void* sourceBuf, size_t length) {
    size_t currentSize = destBuf.size();
    destBuf.resize(currentSize + length);
    memcpy(destBuf.data() + currentSize, sourceBuf, length);
}

/**
 * @brief Format Unix timestamp to human-readable string
 *
 * Converts seconds and nanoseconds since epoch to formatted timestamp string.
 * Format: YY-MM-DD HH:MM:SS.microseconds
 *
 * @param tv_sec Seconds since Unix epoch
 * @param tv_nsec Nanoseconds component
 * @return Formatted timestamp string
 */
std::string Pstore::formatTime(uint32_t tv_sec, uint32_t tv_nsec) {
    // Convert to chrono time point
    std::chrono::seconds sec(tv_sec);
    std::chrono::nanoseconds nsec(tv_nsec);
    auto tp = std::chrono::time_point<std::chrono::system_clock>(sec) + nsec;

    // Convert to time_t for formatting
    std::time_t rtc_time = std::chrono::system_clock::to_time_t(tp);

    // Extract milliseconds for sub-second precision
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()) % 1000;

    // Format date and time
    std::tm* tm = std::localtime(&rtc_time);
    char buffer[30];
    strftime(buffer, sizeof(buffer), "%y-%m-%d %H:%M:%S", tm);

    // Append microseconds
    std::ostringstream oss;
    oss << buffer << "." << std::setw(6) << std::setfill('0') << ms.count();
    return oss.str();
}
#pragma GCC diagnostic pop
