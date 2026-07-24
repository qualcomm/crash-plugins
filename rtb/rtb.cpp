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

#include "rtb.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(Rtb)
#endif

/**
 * cmd_main - Main entry point for the RTB command
 *
 * This function processes command-line arguments and dispatches to the appropriate
 * handler based on the options provided. It supports displaying RTB logs for all CPUs
 * or specific CPUs, and showing memory layout information.
 */
void Rtb::cmd_main(void) {
    // Check if sufficient arguments are provided
    if (argcnt < 2) {
        LOGD("Insufficient arguments: argcnt=%d", argcnt);
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }

    // Verify RTB is enabled in the kernel
    if (!is_enable_rtb()) {
        LOGE("RTB is not enabled in kernel");
        return;
    }

    // Parse RTB log if not already done (lazy initialization)
    if (rtb_state_ptr.get() == nullptr) {
        LOGD("RTB state not initialized, parsing RTB log...");
        parser_rtb_log();
        if (rtb_state_ptr.get() == nullptr) {
            LOGE("Failed to parse RTB log");
            return;
        }
    }
    int c;
    std::string cppString;
    int argerrs = 0;

    // Process command-line options
    while ((c = getopt(argcnt, args, "ac:i")) != EOF) {
        switch(c) {
            case 'a': // Display all RTB logs from all CPUs
                print_rtb_log();
                break;
            case 'c': // Display RTB logs for specific CPU
                cppString.assign(optarg);
                try {
                    int cpu = std::stoi(cppString);
                    if (cpu < 0 || cpu >= rtb_state_ptr->step_size) {
                        LOGE("Invalid CPU number: %d (valid range: 0-%d)", cpu, rtb_state_ptr->step_size - 1);
                        return;
                    }
                    print_percpu_rtb_log(cpu);
                } catch (const std::exception& e) {
                    LOGE("Failed to parse CPU argument: %s", cppString.c_str());
                }
                break;
            case 'i': // Display RTB memory layout information
                LOGD("Displaying RTB memory layout");
                print_rtb_log_memory();
                break;
            default:
                LOGE("Unknown option: %c", c);
                argerrs++;
                break;
        }
    }

    // Display usage if there were argument errors
    if (argerrs) {
        cmd_usage(pc->curcmd, SYNOPSIS);
    }
}

void Rtb::init_offset(void){
    field_init(msm_rtb_state,msm_rtb_idx);
    field_init(msm_rtb_state,rtb);
    field_init(msm_rtb_state,phys);
    field_init(msm_rtb_state,nentries);
    field_init(msm_rtb_state,size);
    field_init(msm_rtb_state,enabled);
    field_init(msm_rtb_state,initialized);
    field_init(msm_rtb_state,step_size);
    struct_init(msm_rtb_state);
    struct_init(msm_rtb_layout);
    struct_init(rtb_idx);
}

void Rtb::init_command(void) {
    cmd_name = "rtb";
    help_str_list={
        "rtb",                                     /* command name */
        "dump RTB (Register Trace Buffer) log",   /* short description */
        "[-a] [-c cpu] [-i]\n"
        "  This command dumps RTB (Register Trace Buffer) log information.\n"
        "\n"
        "    -a          display all RTB logs from all CPUs\n"
        "    -c cpu      display RTB logs for specified CPU\n"
        "    -i          display RTB memory layout information\n",
        "\n",
        "EXAMPLES",
        "  Display all RTB logs from all CPUs:",
        "    %s> rtb -a",
        "       [234.501829] [12532249254] <0>: LOGK_CTXID ctxid:1621 called from addr ffffffd4d628a684 __schedule Line 220 of include/trace/events/sched.h",
        "       [234.501836] [12532249398] <0>: LOGK_IRQ interrupt:1 handled from addr ffffffd4d627c7b4 ipi_handler.04f2cb5359f849bb5e8105832b6bf932.cfi_jt Line 888 of arch/arm64/kernel/entry.S",
        "       [234.501949] [12532251573] <0>: LOGK_CTXID ctxid:4284 called from addr ffffffd4d628a684 __schedule Line 220 of include/trace/events/sched.h",
        "       [234.502641] [12532264845] <0>: LOGK_CTXID ctxid:4285 called from addr ffffffd4d628a684 __schedule Line 220 of include/trace/events/sched.h",
        "\n",
        "  Display rtb log with specified cpu:",
        "    %s> rtb -c 0",
        "       [234.501829] [12532249254] <0>: LOGK_CTXID ctxid:1621 called from addr ffffffd4d628a684 __schedule Line 220 of include/trace/events/sched.h",
        "       [234.501836] [12532249398] <0>: LOGK_IRQ interrupt:1 handled from addr ffffffd4d627c7b4 ipi_handler.04f2cb5359f849bb5e8105832b6bf932.cfi_jt Line 888 of arch/arm64/kernel/entry.S",
        "       [234.501949] [12532251573] <0>: LOGK_CTXID ctxid:4284 called from addr ffffffd4d628a684 __schedule Line 220 of include/trace/events/sched.h",
        "       [234.502641] [12532264845] <0>: LOGK_CTXID ctxid:4285 called from addr ffffffd4d628a684 __schedule Line 220 of include/trace/events/sched.h",
        "\n",
        "  Display RTB memory layout information:",
        "    %s> rtb -i",
        "       RTB log size:1.00Mb",
        "",
        "       bc500000-->-----------------",
        "                  |    rtb_state  |",
        "       bc500828-->-----------------",
        "                  |    rtb_layout |",
        "                  |---------------|",
        "                  |    rtb_layout |",
        "                  |---------------|",
        "                  |    .....      |",
        "       bc600000-->-----------------",
        "\n",
    };
}

Rtb::Rtb(){
    do_init_offset = false;
}

/**
 * is_enable_rtb - Check if RTB is enabled and available
 *
 * This function verifies that the RTB kernel module is loaded and
 * the necessary data structures are available for parsing.
 *
 * Returns: true if RTB is enabled, false otherwise
 */
bool Rtb::is_enable_rtb(){
    init_offset();

    // Check if RTB data structures are available
    if(field_size(msm_rtb_state,rtb) == -1){
        LOGE("RTB data structures not found - module may not be loaded");
        return false;
    }
    LOGD("RTB is enabled and available");
    return true;
}

/**
 * validate_rtb_layout - Validate an RTB layout entry
 * @layout: RTB layout structure to validate
 *
 * Checks the sentinel values and basic validity of an RTB entry.
 *
 * Returns: true if valid, false otherwise
 */
bool Rtb::validate_rtb_layout(const rtb_layout& layout) {
    // Verify sentinel values for entry integrity
    if (layout.sentinel[0] != RTB_SENTINEL_0 ||
        layout.sentinel[1] != RTB_SENTINEL_1 ||
        layout.sentinel[2] != RTB_SENTINEL_2) {
        LOGD("Invalid sentinel values: 0x%02x 0x%02x 0x%02x",
             layout.sentinel[0], layout.sentinel[1], layout.sentinel[2]);
        return false;
    }

    // Check for empty/invalid entries
    if (layout.idx == 0 || layout.log_type == 0) {
        LOGD("Empty entry: idx=%u, log_type=%u", layout.idx, layout.log_type);
        return false;
    }

    return true;
}

/**
 * get_timestamp - Convert RTB timestamp to seconds
 * @layout: RTB layout structure containing timestamp
 *
 * Converts nanosecond timestamp to seconds with microsecond precision.
 *
 * Returns: Timestamp in seconds as a double
 */
double Rtb::get_timestamp(struct rtb_layout& layout){
    if (layout.timestamp == 0) {
        return 0.0;
    }

    // Convert nanoseconds to seconds with microsecond precision
    double ts_float = static_cast<double>(layout.timestamp) / 1e9;
    return std::round(ts_float * 1e6) / 1e6;
}

/**
 * get_caller - Get source code location for caller address
 * @layout: RTB layout structure containing caller address
 *
 * Uses GDB to retrieve the source file and line number information
 * for the caller address.
 *
 * Returns: Source location string, or empty string if not available
 */
std::string Rtb::get_caller(struct rtb_layout& layout){
    char cmd_buf[BUFSIZE];
    std::string result = "";

    // Validate caller address
    if (!is_kvaddr(layout.caller)){
        LOGD("Invalid caller address for source lookup: 0x%llx", (ulonglong)layout.caller);
        return result;
    }

    // Query GDB for source line information
    open_tmpfile();
    sprintf(cmd_buf, "info line *0x%llx",(ulonglong)layout.caller);

    if (!gdb_pass_through(cmd_buf, fp, GNU_RETURN_ON_ERROR)){
        LOGD("GDB query failed for address: 0x%llx", (ulonglong)layout.caller);
        close_tmpfile();
        return result;
    }

    // Parse GDB output to extract source location
    rewind(pc->tmpfile);
    while (fgets(cmd_buf, BUFSIZE, pc->tmpfile)){
        std::string content = cmd_buf;

        // Check if no line information is available
        size_t pos = content.find("No line number information available");
        if (pos != std::string::npos) {
            LOGD("No line information for address: 0x%llx", (ulonglong)layout.caller);
            break;
        }

        // Extract line information
        pos = content.find("starts at");
        if (pos != std::string::npos) {
            result = content.substr(0, pos);
            LOGD("Found source location: %s", result.c_str());
            break;
        }
    }
    close_tmpfile();

    return result;
}

void Rtb::print_none(int cpu, struct rtb_layout& layout) {
    PRINT("<%d> No data\n",cpu);
}

static std::vector<std::string> type_str = {
    "LOGK_NONE",
    "LOGK_READL",
    "LOGK_WRITEL",
    "LOGK_LOGBUF",
    "LOGK_HOTPLUG",
    "LOGK_CTXID",
    "LOGK_TIMESTAMP",
    "LOGK_L2CPREAD",
    "LOGK_L2CPWRITE",
    "LOGK_IRQ",
};

void Rtb::print_read_write(int cpu, struct rtb_layout& layout) {
    std::string name = to_symbol(layout.caller);
    std::string line = get_caller(layout);
    PRINT("[%f] [%lld] <%d>: %s from address:%llx(%llx) called from addr %llx %s %s\n",
        get_timestamp(layout),
        ((ulonglong)layout.cycle_count),
        cpu,
        type_str[layout.log_type].c_str(),
        ((ulonglong)layout.data),
        ((ulonglong)virt_to_phy(layout.data)),
        ((ulonglong)layout.caller),
        name.c_str(),
        line.c_str()
    );
}

void Rtb::print_logbuf(int cpu, struct rtb_layout& layout) {
    std::string name = to_symbol(layout.caller);
    std::string line = get_caller(layout);
    PRINT("[%f] [%lld] <%d>: %s log end:%llx called from addr %llx %s %s\n",
        get_timestamp(layout),
        ((ulonglong)layout.cycle_count),
        cpu,
        type_str[layout.log_type].c_str(),
        ((ulonglong)layout.data),
        ((ulonglong)layout.caller),
        name.c_str(),
        line.c_str()
    );
}

void Rtb::print_hotplug(int cpu, struct rtb_layout& layout) {
    std::string name = to_symbol(layout.caller);
    std::string line = get_caller(layout);
    PRINT("[%f] [%lld] <%d>: %s cpu data:%llx called from addr %llx %s %s\n",
        get_timestamp(layout),
        ((ulonglong)layout.cycle_count),
        cpu,
        type_str[layout.log_type].c_str(),
        ((ulonglong)layout.data),
        ((ulonglong)layout.caller),
        name.c_str(),
        line.c_str()
    );
}

void Rtb::print_ctxid(int cpu, struct rtb_layout& layout) {
    std::string name = to_symbol(layout.caller);
    std::string line = get_caller(layout);
    PRINT("[%f] [%lld] <%d>: %s ctxid:%lld called from addr %llx %s %s\n",
        get_timestamp(layout),
        ((ulonglong)layout.cycle_count),
        cpu,
        type_str[layout.log_type].c_str(),
        ((ulonglong)layout.data),
        ((ulonglong)layout.caller),
        name.c_str(),
        line.c_str()
    );
}

void Rtb::print_timestamp(int cpu, struct rtb_layout& layout) {
    std::string name = to_symbol(layout.caller);
    std::string line = get_caller(layout);
    PRINT("[%f] [%lld] <%d>: %s timestamp:%llx called from addr %llx %s %s\n",
        get_timestamp(layout),
        ((ulonglong)layout.cycle_count),
        cpu,
        type_str[layout.log_type].c_str(),
        ((ulonglong)layout.data),
        ((ulonglong)layout.caller),
        name.c_str(),
        line.c_str()
    );
}

void Rtb::print_l2cpread_write(int cpu, struct rtb_layout& layout) {
    std::string name = to_symbol(layout.caller);
    std::string line = get_caller(layout);
    PRINT("[%f] [%lld] <%d>: %s from offset:%llx called from addr %llx %s %s\n",
        get_timestamp(layout),
        ((ulonglong)layout.cycle_count),
        cpu,
        type_str[layout.log_type].c_str(),
        ((ulonglong)layout.data),
        ((ulonglong)layout.caller),
        name.c_str(),
        line.c_str()
    );
}

void Rtb::print_irq(int cpu, struct rtb_layout& layout) {
    std::string name = to_symbol(layout.caller);
    std::string line = get_caller(layout);
    PRINT("[%f] [%lld] <%d>: %s interrupt:%lld handled from addr %llx %s %s\n",
        get_timestamp(layout),
        ((ulonglong)layout.cycle_count),
        cpu,
        type_str[layout.log_type].c_str(),
        ((ulonglong)layout.data),
        ((ulonglong)layout.caller),
        name.c_str(),
        line.c_str()
    );
}

/**
 * print_rtb_layout - Print a single RTB layout entry
 * @cpu: CPU number for this entry
 * @index: Index into the RTB ring buffer
 *
 * Reads and displays a single RTB entry from the specified index.
 * Validates the entry before printing.
 *
 * Returns: The entry's index value, or 0 if invalid
 */
int Rtb::print_rtb_layout(int cpu, int index){
    // Calculate address of the RTB entry
    ulong addr = rtb_state_ptr->rtb_layout + index * struct_size(msm_rtb_layout);
    struct rtb_layout layout;

    // Read the RTB layout structure from memory
    if(!read_struct(addr, &layout, sizeof(layout), "msm_rtb_layout")){
        LOGD("Failed to read RTB layout at index %d, addr 0x%lx", index, addr);
        return 0;
    }

    // Validate the entry using sentinel values and basic checks
    if (!validate_rtb_layout(layout)) {
        return 0;
    }

    LOGD("Processing RTB entry: CPU=%d, index=%d, idx=%u, type=%u",
         cpu, index, layout.idx, layout.log_type);

    // Extract log type (mask off flags)
    int type = layout.log_type & RTB_LOG_TYPE_MASK;
    // Dispatch to appropriate print function based on log type
    switch (type) {
    case LOGK_NONE:
        print_none(cpu, layout);
        break;
    case LOGK_READL:
    case LOGK_WRITEL:
        print_read_write(cpu, layout);
        break;
    case LOGK_LOGBUF:
        print_logbuf(cpu, layout);
        break;
    case LOGK_HOTPLUG:
        print_hotplug(cpu, layout);
        break;
    case LOGK_CTXID:
        print_ctxid(cpu, layout);
        break;
    case LOGK_TIMESTAMP:
        print_timestamp(cpu, layout);
        break;
    case LOGK_L2CPREAD:
    case LOGK_L2CPWRITE:
        print_l2cpread_write(cpu, layout);
        break;
    case LOGK_IRQ:
        print_irq(cpu, layout);
        break;
    default:
        LOGD("Unknown log type: %d", type);
        print_none(cpu, layout);
        break;
    }

    return layout.idx;
}

/**
 * next_rtb_entry - Calculate the next entry index in the ring buffer
 * @index: Current index
 *
 * RTB uses a ring buffer with per-CPU entries. This function calculates
 * the next valid entry index, accounting for CPU stride and wraparound.
 *
 * Returns: Next entry index
 */
int Rtb::next_rtb_entry(int index){
    int step_size = rtb_state_ptr->step_size;
    int mask = rtb_state_ptr->nentries - 1;
    int unused_size = (mask + 1) % step_size;

    // Handle wraparound in the ring buffer
    if (((index + step_size + unused_size) & mask) < (index & mask)){
        LOGD("Ring buffer wraparound: index=%d, next=%d",
             index, (index + step_size + unused_size) & mask);
        return (index + step_size + unused_size) & mask;
    }

    return (index + step_size) & mask;
}

/**
 * print_percpu_rtb_log - Print RTB logs for a specific CPU
 * @cpu: CPU number to display logs for
 *
 * Iterates through the RTB ring buffer for the specified CPU and
 * displays all valid log entries in chronological order.
 */
void Rtb::print_percpu_rtb_log(int cpu){
    // Verify RTB was initialized
    if (rtb_state_ptr->initialized != 1){
        LOGE("RTB not initialized (initialized=%d)", rtb_state_ptr->initialized);
        return;
    }
    LOGD("Printing RTB logs for CPU %d", cpu);
    int index = 0;
    int last_idx = 0;
    int next = 0;
    int next_idx = 0;

    // Determine starting index based on kernel version and configuration
    if (THIS_KERNEL_VERSION >= LINUX(5,10,0)){
        if(get_config_val("CONFIG_QCOM_RTB_SEPARATE_CPUS") == "y"){
            index = rtb_state_ptr->rtb_idx[cpu];
            LOGD("Using separate CPU index: %d", index);
        }else{
            index = rtb_state_ptr->rtb_idx[0];
            LOGD("Using shared index: %d", index);
        }
    }else{
        if(get_config_val("CONFIG_QCOM_RTB_SEPARATE_CPUS") == "y"){
            index = read_int(csymbol_value("msm_rtb_idx_cpu") + kt->__per_cpu_offset[cpu],"msm_rtb_idx_cpu");
            LOGD("Read per-CPU index from msm_rtb_idx_cpu: %d", index);
        }else{
            index = read_int(csymbol_value("msm_rtb_idx"),"msm_rtb_idx");
            LOGD("Read shared index from msm_rtb_idx: %d", index);
        }
    }

    // Mask index to valid range
    index = index & (rtb_state_ptr->nentries - 1);
    LOGD("Starting RTB iteration at index %d for CPU %d", index, cpu);
    struct rtb_layout layout;
    int entries_processed = 0;

    // Iterate through ring buffer entries
    while (true){
        last_idx = print_rtb_layout(cpu, index);
        entries_processed++;

        // Calculate next entry position
        next = next_rtb_entry(index);
        ulong addr = rtb_state_ptr->rtb_layout + next * struct_size(msm_rtb_layout);

        // Read next entry to check if we should continue
        BZERO(&layout, sizeof(layout));
        if(!read_struct(addr, &layout, sizeof(layout), "msm_rtb_layout")){
            LOGD("Failed to read next entry at index %d, stopping iteration", next);
            break;
        }

        next_idx = layout.idx;

        // Continue if next entry has a higher index (chronological order)
        if (last_idx < next_idx){
            index = next;
        }

        // Stop if we've wrapped around to the same position
        if (next != index){
            LOGD("Reached end of valid entries at index %d", index);
            break;
        }

        // Safety check to prevent infinite loops
        if (entries_processed > rtb_state_ptr->nentries) {
            LOGE("Processed too many entries (%d), breaking loop", entries_processed);
            break;
        }
    }

    LOGD("Completed RTB log display for CPU %d, processed %d entries", cpu, entries_processed);
}

/**
 * print_rtb_log_memory - Display RTB memory layout information
 *
 * Shows the physical memory layout of the RTB buffer, including
 * the state structure and log entries.
 */
void Rtb::print_rtb_log_memory(){
    physaddr_t start = rtb_state_ptr->phys - struct_size(msm_rtb_state);
    size_t size = rtb_state_ptr->size;

    LOGD("RTB memory layout: start=0x%llx, size=%zu, phys=0x%llx",
         (ulonglong)start, size, (ulonglong)rtb_state_ptr->phys);

    PRINT("RTB log size:%s\n\n",csize(size).c_str());
    PRINT("%llx-->-----------------\n",(ulonglong)start);
    PRINT("           |    rtb_state  |\n");
    PRINT("%llx-->-----------------\n",(ulonglong)rtb_state_ptr->phys);
    PRINT("           |    rtb_layout |\n");
    PRINT("           |---------------|\n");
    PRINT("           |    rtb_layout |\n");
    PRINT("           |---------------|\n");
    PRINT("           |    .....      |\n");
    PRINT("%llx-->-----------------\n",(ulonglong)(start + size));
    PRINT("\n");
}

/**
 * print_rtb_log - Print RTB logs for all CPUs
 *
 * Iterates through all CPUs and displays their RTB logs.
 */
void Rtb::print_rtb_log(){
    // Verify RTB was initialized
    if (rtb_state_ptr->initialized != 1){
        LOGE("RTB not initialized (initialized=%d)", rtb_state_ptr->initialized);
        return;
    }
    LOGD("Printing RTB logs for all %d CPUs", rtb_state_ptr->step_size);
    // Display logs for each CPU
    for (int cpu = 0; cpu < rtb_state_ptr->step_size; cpu++){
        print_percpu_rtb_log(cpu);
    }
    LOGD("Completed printing RTB logs for all CPUs");
}

/**
 * parser_rtb_log - Parse RTB log from kernel memory
 *
 * This function locates and parses the RTB state structure from kernel memory,
 * extracting all necessary information for displaying RTB logs.
 * It handles different kernel versions and configurations.
 */
void Rtb::parser_rtb_log(){
    ulong msm_rtb_addr = 0;

    // Locate RTB state structure based on kernel version
    if (THIS_KERNEL_VERSION >= LINUX(5,10,0)){
        msm_rtb_addr = csymbol_value("msm_rtb_ptr");
        LOGD("Kernel >= 5.10.0, using msm_rtb_ptr");
    }else{
        msm_rtb_addr = csymbol_value("msm_rtb");
        LOGD("Kernel < 5.10.0, using msm_rtb");
    }

    // Check if RTB is available
    if(!msm_rtb_addr){
        LOGE("RTB symbol not found - RTB is disabled");
        return;
    }
    LOGD("Found RTB symbol at address: 0x%lx", msm_rtb_addr);
    // Read pointer to actual RTB state
    msm_rtb_addr = read_pointer(msm_rtb_addr, "msm_rtb");
    if (!msm_rtb_addr) {
        LOGE("Failed to read RTB pointer");
        return;
    }
    LOGD("msm_rtb_state at: 0x%lx", msm_rtb_addr);
    // Read RTB state structure
    void *rtb_state_buf = read_struct(msm_rtb_addr, "msm_rtb_state");
    if(rtb_state_buf == nullptr) {
        LOGE("Failed to read msm_rtb_state structure");
        return;
    }

    // Allocate and populate RTB state
    rtb_state_ptr = std::make_shared<rtb_state>();

    // Parse per-CPU indices
    size_t cnt = field_size(msm_rtb_state, msm_rtb_idx) / struct_size(rtb_idx);
    LOGD("Parsing %zu RTB indices", cnt);

    for (size_t i = 0; i < cnt; i++){
        ulong rtb_idx_addr = msm_rtb_addr + i * struct_size(rtb_idx);
        int idx = read_int(rtb_idx_addr, "rtb_idx");
        rtb_state_ptr->rtb_idx.push_back(idx);
        LOGD("rtb_idx[%zu] = %d (addr: 0x%lx)", i, idx, rtb_idx_addr);
    }

    // Extract RTB state fields
    rtb_state_ptr->rtb_layout = ULONG(rtb_state_buf + field_offset(msm_rtb_state, rtb));
    rtb_state_ptr->phys = ULONG(rtb_state_buf + field_offset(msm_rtb_state, phys));
    rtb_state_ptr->nentries = INT(rtb_state_buf + field_offset(msm_rtb_state, nentries));
    rtb_state_ptr->size = INT(rtb_state_buf + field_offset(msm_rtb_state, size));
    rtb_state_ptr->enabled = INT(rtb_state_buf + field_offset(msm_rtb_state, enabled));
    rtb_state_ptr->initialized = INT(rtb_state_buf + field_offset(msm_rtb_state, initialized));
    rtb_state_ptr->step_size = INT(rtb_state_buf + field_offset(msm_rtb_state, step_size));

    LOGD("RTB state parsed successfully:");
    LOGD("  rtb_layout: 0x%lx", rtb_state_ptr->rtb_layout);
    LOGD("  phys: 0x%llx", (ulonglong)rtb_state_ptr->phys);
    LOGD("  nentries: %d", rtb_state_ptr->nentries);
    LOGD("  size: %d bytes (%s)", rtb_state_ptr->size, csize(rtb_state_ptr->size).c_str());
    LOGD("  enabled: %d", rtb_state_ptr->enabled);
    LOGD("  initialized: %d", rtb_state_ptr->initialized);
    LOGD("  step_size (CPUs): %d", rtb_state_ptr->step_size);

    FREEBUF(rtb_state_buf);
}
#pragma GCC diagnostic pop
