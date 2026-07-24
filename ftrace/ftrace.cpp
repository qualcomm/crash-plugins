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

#include "ftrace.h"
#include "logger/logger_core.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(Ftrace)
#endif

/**
 * @brief Main command handler for ftrace plugin
 *
 * Processes command line arguments and executes the appropriate ftrace analysis
 * functions. Supports various options for displaying trace data, events, and
 * exporting trace information.
 */
void Ftrace::cmd_main(void) {
    int c, cpu;
    std::string cppString;

    // Check minimum argument count
    if (argcnt < 2) {
        LOGD("Insufficient arguments provided");
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }

    // Initialize trace arrays if not already done
    if (trace_list.size() == 0) {
        parser_ftrace_trace_arrays();
    }

    // Initialize event maps if not already done
    if (event_maps.size() == 0) {
        parser_ftrace_events_list();
    }

    // Initialize common field maps if not already done
    if (common_field_maps.size() == 0) {
        parser_common_trace_fields();
    }

    // Process command line options
    while ((c = getopt(argcnt, args, "aA:lfec:")) != EOF) {
        switch(c) {
            case 'a':
                print_trace_log();
                break;
            case 'A':
                cppString.assign(optarg);
                print_trace_log(cppString);
                break;
            case 'l':
                print_trace_array();
                break;
            case 'c':
                cppString.assign(optarg);
                try {
                    cpu = std::stoi(cppString);
                    LOGD("Printing trace logs for CPU %d", cpu);
                    print_trace_log(cpu);
                } catch (...) {
                    LOGE("Invalid CPU argument: %s", cppString.c_str());
                }
                break;
            case 'f':
                print_event_format();
                break;
            case 'e':
                print_skip_events();
                break;
            case 'd':
                ftrace_dump();
                break;
            case 't':
                ftrace_show();
                break;
            default:
                LOGE("Unknown option: %c", c);
                argerrs++;
                break;
        }
    }
    if (argerrs) {
        LOGE("Command line argument errors detected");
        cmd_usage(pc->curcmd, SYNOPSIS);
    }
}

/**
 * @brief Initialize structure field offsets for ftrace-related kernel structures
 *
 * This function initializes all the field offsets for various kernel structures
 * used by the ftrace subsystem. These offsets are essential for correctly
 * parsing kernel memory structures.
 */
void Ftrace::init_offset(){
    // Trace array structure fields
    field_init(trace_array,current_trace);
    field_init(trace_array,array_buffer);
    field_init(trace_array,max_buffer);
    field_init(trace_array,buffer);
    field_init(trace_array, name);
    field_init(trace_array, list);

    // Array buffer structure fields
    field_init(array_buffer,buffer);
    field_init(array_buffer,time_start);
    field_init(array_buffer,cpu);

    // Tracer structure fields
    field_init(tracer,name);

    // Ring buffer structure fields
    field_init(ring_buffer, pages);
    field_init(ring_buffer, flags);
    field_init(ring_buffer, cpus);
    field_init(ring_buffer, buffers);

    // Trace buffer structure fields
    field_init(trace_buffer, pages);
    field_init(trace_buffer, flags);
    field_init(trace_buffer, cpus);
    field_init(trace_buffer, buffers);

    // Trace event call structure fields
    field_init(trace_event_call, print_fmt);
    field_init(trace_event_call, flags);
    field_init(trace_event_call, name);
    field_init(trace_event_call, class);
    field_init(trace_event_call, event);
    field_init(trace_event_call, list);
    field_init(trace_event_call, tp);

    // Trace event class structure fields
    field_init(trace_event_class, system);
    field_init(trace_event_class, fields);

    // Trace event structure fields
    field_init(trace_event, type);
    field_init(tracepoint, name);

    // Ring buffer per-CPU structure fields
    field_init(ring_buffer_per_cpu, nr_pages);
    field_init(ring_buffer_per_cpu, cpu);
    field_init(ring_buffer_per_cpu, pages);
    field_init(ring_buffer_per_cpu, head_page);
    field_init(ring_buffer_per_cpu, tail_page);
    field_init(ring_buffer_per_cpu, commit_page);
    field_init(ring_buffer_per_cpu, reader_page);
    field_init(ring_buffer_per_cpu, overrun);
    field_init(ring_buffer_per_cpu, entries);

    // Ring buffer event structure fields
    field_init(ring_buffer_event, array);
    struct_init(ring_buffer_event);
    struct_init(trace_entry);

    // Buffer page structure fields
    field_init(buffer_page, read);
    field_init(buffer_page, list);
    field_init(buffer_page, page);
    field_init(buffer_page, real_end);

    // List head structure fields
    field_init(list_head, next);

    // Ftrace event field structure fields
    field_init(ftrace_event_field, link);
    field_init(ftrace_event_field, name);
    field_init(ftrace_event_field, type);
    field_init(ftrace_event_field, offset);
    field_init(ftrace_event_field, size);
    field_init(ftrace_event_field, is_signed);
    field_init(ftrace_event_field, filter_type);

    // Buffer data page structure fields
    field_init(buffer_data_page, commit);
    field_init(buffer_data_page, time_stamp);
    field_init(buffer_data_page, data);

    // Saved command lines buffer structure fields
    field_init(saved_cmdlines_buffer, map_pid_to_cmdline);
    field_init(saved_cmdlines_buffer, map_cmdline_to_pid);
    field_init(saved_cmdlines_buffer, cmdline_num);
    field_init(saved_cmdlines_buffer, cmdline_idx);
    field_init(saved_cmdlines_buffer, saved_cmdlines);
}

/**
 * @brief Initialize command help and usage information
 *
 * Sets up the command name, description, and detailed help text with examples
 * for all supported ftrace plugin options.
 */
void Ftrace::init_command(void) {
    cmd_name = "ftrace";
    help_str_list={
        "ftrace",                                 /* command name */
        "display ftrace information",             /* short description */
        "[-a] [-A name] [-l] [-c cpu] [-f] [-e] [-d] [-t]\n"
        "  This command displays ftrace information.\n"
        "\n"
        "    -a              display all ftrace log\n"
        "    -A name         display ftrace log of specified trace array by name\n"
        "    -l              list all trace array\n"
        "    -c cpu          display ftrace log of specified cpu\n"
        "    -f              display all trace event info\n"
        "    -e              display skip events\n"
        "    -d              dump all trace to file\n"
        "    -t              display all trace via trace-cmd\n",
        "\n",
        "EXAMPLES",
        "  List all trace array:",
        "    %s> ftrace -l",
        "       usb_xhci",
        "       usb",
        "       memory",
        "       clock_reg",
        "       binder",
        "       suspend",
        "       rproc_qcom",
        "       global",
        "  Display all ftrace log:",
        "\n",
        "    %s> ftrace -a",
        "       usb        UsbFfs-worker-1908   [0] d..2. 165.323101: dwc3_ep_queue ep4in: req ffffff8068d7f000 length 0/24 zsI ==> -115",
        "       usb        UsbFfs-worker-1908   [0] d..2. 165.323143: dwc3_prepare_trb ep4in: trb ffffffc00953d000(E1:D0) buf 02946619584size 24ctrl 2065",
        "       usb        UsbFfs-worker-1908   [0] d..2. 165.323165: dwc3_gadget_ep_cmd ep4in: cmd 589831params 000--> status: 0",
        "       usb        UsbFfs-worker-1908   [0] d..2. 165.323253: dwc3_ep_queue ep4in: req ffffff801563fe00 length 0/14 zsI ==> -115",
        "       usb        UsbFfs-worker-1908   [0] d..2. 165.323272: dwc3_prepare_trb ep4in: trb ffffffc00953d010(E2:D0) buf 02941848960size 14ctrl 2065",
        "       usb        UsbFfs-worker-1908   [0] d..2. 165.323293: dwc3_gadget_ep_cmd ep4in: cmd 589831params 000--> status: 0",
        "\n",
        "  Display ftrace log of specified trace array by name:",
        "    %s> ftrace -A binder",
        "       binder     binder:943_4-1443    [0] ..... 227.397723: binder_transaction transaction=89703 dest_node=0 dest_proc=1067 dest_thread=3576 reply=1 flags=0x0 code=0x0",
        "       binder     binder:943_4-1443    [0] ...1. 227.397729: binder_update_page_range proc=1067 allocate=1 offset=0 size=0",
        "       binder     binder:943_4-1443    [0] ..... 227.397735: binder_transaction_alloc_buf transaction=89703 data_size=0 offsets_size=0 extra_buffers_size=0",
        "       binder     binder:943_4-1443    [0] ..... 227.397810: binder_set_priority proc=943 thread=1443 old=98 => new=120 desired=120",
        "       binder     binder:943_2-1018    [0] ..... 227.397908: binder_transaction_received transaction=89702",
        "       binder     binder:943_2-1018    [0] ..... 227.397913: binder_return cmd=2151707138",
        "\n",
        "  Display all trace event info:",
        "    %s> ftrace -f",
        "       [25] ipi_raise: 'target_mask=%s (%s)', __get_bitmask(target_cpus), REC->reason",
        "           format: 'target_mask=%s (%s)'",
        "           args  :",
        "               __get_bitmask(target_cpus)",
        "               reason",
        "           trace_event_raw_ipi_raise {",
        "               __data_loc unsigned long[] target_cpus,offset:8, size:4",
        "               const char * reason,     offset:16, size:8",
        "           }",
        "\n",
        "  Display ftrace log of specified trace array by cpu:",
        "    %s> ftrace -c 1",
        "    global     sh-24270             [1] ..s1. 3471.158992: softirq_exit vec=9 [action=RCU]",
        "    global     sh-24270             [1] ..s1. 3471.158986: softirq_exit vec=9 [action=RCU]",
        "\n",
        "  Dump all trace to file:",
        "    %s> ftrace -d",
        "    Save to /path/ftrace.data",
        "\n",
        "  Display all trace via trace-cmd:",
        "    %s> ftrace -t",
        "    memory:            <...>-67    [001]    43.835156: tlbi_end:             group=5800000.qcom,ipa:ipa_smmu_ap",
        "    memory:            <...>-67    [001]    43.835160: tlbi_start:           group=5800000.qcom,ipa:ipa_smmu_ap",
        "\n",
    };
}

/**
 * @brief Constructor for Ftrace plugin
 *
 * Initializes the Ftrace plugin by reading system configuration values
 * such as maximum PID and number of CPUs.
 */
Ftrace::Ftrace(){
    // Read maximum PID value from kernel
    pid_max = read_int(csymbol_value("pid_max"),"pid_max");

    // Read number of CPUs, default to 1 if not available
    if (!try_get_symbol_data(TO_CONST_STRING("nr_cpu_ids"), sizeof(int), &nr_cpu_ids)) {
        nr_cpu_ids = 1;
        LOGD("nr_cpu_ids not available, defaulting to 1");
    } else {
        LOGD("System nr_cpu_ids: %d", nr_cpu_ids);
    }
}

void Ftrace::print_skip_events(){
    for (const auto& pair : event_maps) {
        std::shared_ptr<TraceEvent> event_ptr = pair.second;
        if (event_ptr->skiped){
            event_ptr->dump2();
        }
    }
}

/**
 * @brief Print event format information for all registered events
 *
 * Iterates through all registered trace events and displays their
 * format information including fields, types, and print formats.
 */
void Ftrace::print_event_format(){
    PRINT("total trace event: %d\n", event_maps.size());
    int skip_cnt = 0;
    for (const auto& pair : event_maps) {
        std::shared_ptr<TraceEvent> event_ptr = pair.second;
        if (event_ptr->skiped){
            skip_cnt++;
        }
    }
    PRINT("skip trace event: %d\n", skip_cnt);
    for (const auto& pair : event_maps) {
        std::shared_ptr<TraceEvent> event_ptr = pair.second;
        event_ptr->dump();
    }
}

/**
 * @brief Print trace array information
 *
 * Displays information about all trace arrays including their names
 * and per-CPU ring buffer statistics.
 */
void Ftrace::print_trace_array(){
    std::ostringstream oss;
    for (const auto& ta : trace_list) {
        oss << std::left << std::setw(10)  << ta->name << "\n";
        for (const auto& cpu_rb_ptr : ta->cpu_ring_buffers) {
            oss << "    cpu[" << cpu_rb_ptr->cpu << "]"
                << " nr_pages:" << cpu_rb_ptr->buffer_pages.size()
                << " write_pages:" << cpu_rb_ptr->data_pages.size() << "\n";
        }
    }
    PRINT("%s", oss.str().c_str());
}

/**
 * @brief Print trace logs for a specific CPU
 *
 * Parses all trace data if not already done, then displays only
 * the trace logs that occurred on the specified CPU.
 *
 * @param cpu CPU number to filter logs for
 */
void Ftrace::print_trace_log(int cpu){
    LOGD("Printing trace logs for CPU %d", cpu);
    // Parse trace data if not already done
    if (trace_logs.size() == 0){
        LOGD("Parsing trace data from %zu trace arrays", trace_list.size());
        for (const auto& ta : trace_list) {
            for (const auto& cpu_rb_ptr : ta->cpu_ring_buffers) {
                for (const auto& rb_addr : cpu_rb_ptr->data_pages) {
                    parse_buffer_page(ta, cpu_rb_ptr, rb_addr);
                }
            }
        }
        // Sort logs by timestamp (newest first)
        std::sort(trace_logs.begin(), trace_logs.end(),[&](std::shared_ptr<trace_log> a, std::shared_ptr<trace_log> b){
            return a->timestamp < b->timestamp;
        });
        LOGD("Parsed %zu trace log entries", trace_logs.size());
    }

    // Print logs for specified CPU
    for (const auto& log_ptr : trace_logs) {
        if (log_ptr->cpu != cpu){
            continue;
        }
        auto end = log_ptr->log.find_last_not_of(" \n\r");
        if (end != std::string::npos) {
            log_ptr->log.erase(end + 1);
        } else {
            continue; // String contains only whitespace
        }
        PRINT("%s\n", log_ptr->log.c_str());
    }
}

/**
 * @brief Print all trace logs
 *
 * Parses all trace data if not already done, then displays all
 * trace logs sorted by timestamp.
 */
void Ftrace::print_trace_log(){
    LOGD("Printing all trace logs");
    // Parse trace data if not already done
    if (trace_logs.size() == 0){
        for (const auto& ta : trace_list) {
            for (const auto& cpu_rb_ptr : ta->cpu_ring_buffers) {
                for (const auto& rb_addr : cpu_rb_ptr->data_pages) {
                    parse_buffer_page(ta, cpu_rb_ptr, rb_addr);
                }
            }
        }
        // Sort logs by timestamp (newest first)
        std::sort(trace_logs.begin(), trace_logs.end(),[&](std::shared_ptr<trace_log> a, std::shared_ptr<trace_log> b){
            return a->timestamp < b->timestamp;
        });
        LOGD("Parsed %zu trace log entries", trace_logs.size());
    }
    // Print all logs
    for (const auto& log_ptr : trace_logs) {
        auto end = log_ptr->log.find_last_not_of(" \n\r");
        if (end != std::string::npos) {
            log_ptr->log.erase(end + 1);
        } else {
            continue; // String contains only whitespace
        }
        PRINT("%s\n", log_ptr->log.c_str());
    }
}

/**
 * @brief Print trace logs for a specific trace array
 *
 * Parses all trace data if not already done, then displays only
 * the trace logs from the specified trace array.
 *
 * @param name Name of the trace array to filter logs for
 */
void Ftrace::print_trace_log(std::string name){
    LOGD("Printing trace logs for array '%s'", name.c_str());
    // Parse trace data if not already done
    if (trace_logs.size() == 0){
        for (const auto& ta : trace_list) {
            for (const auto& cpu_rb_ptr : ta->cpu_ring_buffers) {
                for (const auto& rb_addr : cpu_rb_ptr->data_pages) {
                    parse_buffer_page(ta, cpu_rb_ptr, rb_addr);
                }
            }
        }
        // Sort logs by timestamp (newest first)
        std::sort(trace_logs.begin(), trace_logs.end(),[&](std::shared_ptr<trace_log> a, std::shared_ptr<trace_log> b){
            return a->timestamp < b->timestamp;
        });
        LOGD("Parsed %zu trace log entries", trace_logs.size());
    }
    // Print logs for specified array
    for (const auto& log_ptr : trace_logs) {
        if (log_ptr->array_name != name){
            continue;
        }
        auto end = log_ptr->log.find_last_not_of(" \n\r");
        if (end != std::string::npos) {
            log_ptr->log.erase(end + 1);
        } else {
            continue; // String contains only whitespace
        }
        PRINT("%s\n", log_ptr->log.c_str());
    }
}

/**
 * @brief Parse saved command lines buffer for PID to command name mapping
 *
 * Reads the kernel's saved command lines buffer which maps PIDs to process
 * command names. This is used to resolve process names in trace events.
 */
void Ftrace::parser_savedcmd(){
    ulong savedcmd_addr = read_pointer(csymbol_value("savedcmd"),"savedcmd");
    if (is_kvaddr(savedcmd_addr)) {
        LOGD("Found savedcmd at address 0x%lx", savedcmd_addr);
        savedcmd_ptr = std::make_shared<saved_cmdlines_buffer>();
        savedcmd_ptr->map_pid_to_cmdline = savedcmd_addr + field_offset(saved_cmdlines_buffer,map_pid_to_cmdline);
        savedcmd_ptr->map_cmdline_to_pid = read_pointer(savedcmd_addr + field_offset(saved_cmdlines_buffer,map_cmdline_to_pid),"map_cmdline_to_pid");
        savedcmd_ptr->cmdline_num = read_uint(savedcmd_addr + field_offset(saved_cmdlines_buffer,cmdline_num),"cmdline_num");
        savedcmd_ptr->cmdline_idx = read_int(savedcmd_addr + field_offset(saved_cmdlines_buffer,cmdline_idx),"cmdline_idx");
        savedcmd_ptr->saved_cmdlines = read_pointer(savedcmd_addr + field_offset(saved_cmdlines_buffer,saved_cmdlines),"saved_cmdlines");

        LOGD("Saved cmdlines buffer: num=%u, idx=%d", savedcmd_ptr->cmdline_num, savedcmd_ptr->cmdline_idx);
    } else {
        LOGD("No valid savedcmd buffer found");
    }
}

/**
 * @brief Parse all ftrace trace arrays from kernel memory
 *
 * Iterates through the kernel's ftrace_trace_arrays list and parses each
 * trace array structure. Also identifies the global trace array and
 * initializes the saved command lines buffer.
 */
void Ftrace::parser_ftrace_trace_arrays(){
    if (!csymbol_exists("ftrace_trace_arrays")){
        LOGE("ftrace_trace_arrays symbol doesn't exist in this kernel");
        return;
    }

    ulong trace_arrays_addr = csymbol_value("ftrace_trace_arrays");
    if (!is_kvaddr(trace_arrays_addr)) {
        LOGE("ftrace_trace_arrays address 0x%lx is invalid", trace_arrays_addr);
        return;
    }

    LOGD("ftrace_trace_arrays found at address 0x%lx", trace_arrays_addr);

    ulong gtrace_addr = csymbol_value("global_trace");
    LOGD("global_trace address: 0x%lx", gtrace_addr);

    int offset = field_offset(trace_array, list);
    size_t array_count = 0;
    // Parse each trace array in the list
    for (const auto& addr : for_each_list(trace_arrays_addr,offset)) {
        std::shared_ptr<trace_array> ta_ptr = parser_trace_array(addr);
        if (addr == gtrace_addr){
            global_trace = ta_ptr;
            ta_ptr->name = "global";
        }
        trace_list.push_back(ta_ptr);
        array_count++;
    }
    LOGD("Parsed %zu trace arrays successfully", array_count);

    // Parse saved command lines for PID resolution
    parser_savedcmd();
}

/**
 * @brief Parse a single trace array structure from kernel memory
 *
 * Reads and parses a trace_array structure, extracting its name, timing
 * information, and associated trace buffer. This function is critical for
 * understanding the ftrace subsystem's organization of trace data.
 *
 * @param addr Kernel address of the trace_array structure
 * @return Shared pointer to parsed trace_array structure, or nullptr on failure
 */
std::shared_ptr<trace_array> Ftrace::parser_trace_array(ulong addr){
    LOGD("Parsing trace_array at address 0x%lx", addr);

    if (!is_kvaddr(addr)) {
        LOGE("Invalid trace_array address 0x%lx", addr);
        return nullptr;
    }

    auto trace_ptr = std::make_shared<trace_array>();
    trace_ptr->addr = addr;

    // Read trace array name
    ulong name_addr = read_pointer(addr + field_offset(trace_array,name),"name addr");
    if (is_kvaddr(name_addr)) {
        trace_ptr->name = read_cstring(name_addr,64, "name");
    } else {
        trace_ptr->name = "";
    }
    LOGD("Trace array name: '%s'", trace_ptr->name.c_str());
    // Read array buffer information
    ulong array_addr = addr + field_offset(trace_array,array_buffer);
    trace_ptr->time_start = read_ulong(array_addr + field_offset(array_buffer,time_start),"time_start");

    // Parse the associated trace buffer
    ulong buffer = read_pointer(array_addr + field_offset(array_buffer,buffer),"buffer addr");
    if (is_kvaddr(buffer)) {
        parser_trace_buffer(trace_ptr,buffer);
    } else {
        LOGW("No valid trace buffer found for array '%s'", trace_ptr->name.c_str());
    }
    return trace_ptr;
}

/**
 * @brief Parse trace buffer and its per-CPU ring buffers
 *
 * Reads the trace buffer structure and parses all associated per-CPU
 * ring buffers for the given trace array.
 *
 * @param ta Shared pointer to trace array
 * @param addr Kernel address of the trace buffer structure
 */
void Ftrace::parser_trace_buffer(std::shared_ptr<trace_array> ta, ulong addr){
    LOGD("Parsing trace_buffer for array '%s' at address 0x%lx", ta->name.c_str(), addr);

    // Read number of CPUs
    ta->cpus = read_int(addr + field_offset(trace_buffer, cpus), "cpus");

    // Read buffers array pointer
    ulong buffers = read_pointer(addr + field_offset(trace_buffer, buffers), "buffers");
    if (!is_kvaddr(buffers)) {
        LOGE("Invalid buffers address 0x%lx for trace array '%s'", buffers, ta->name.c_str());
        return;
    }
    // Parse each per-CPU ring buffer
    for (int i = 0; i < ta->cpus; i++){
        ulong rb_addr = read_pointer(buffers + i * sizeof(void *), "ring_buffer addr");
        if (!is_kvaddr(rb_addr)) {
            LOGE("CPU %d has invalid ring buffer address 0x%lx", i, rb_addr);
            continue;
        }

        LOGD("Parsing ring_buffer_per_cpu for CPU#%d at address 0x%lx", i, rb_addr);
        parser_ring_buffer_per_cpu(ta, rb_addr);
    }
}

/**
 * @brief Parse ring buffer per-CPU structure and extract buffer pages
 *
 * This function parses the ring_buffer_per_cpu structure which contains
 * the actual trace data pages for a specific CPU. It identifies which
 * pages contain valid data and adds them to the data_pages list.
 * The ring buffer uses a circular buffer design with head, tail, commit,
 * and reader pages to manage trace data efficiently.
 *
 * @param ta Shared pointer to trace array
 * @param addr Kernel address of the ring_buffer_per_cpu structure
 */
void Ftrace::parser_ring_buffer_per_cpu(std::shared_ptr<trace_array> ta, ulong addr){
    // Read the number of pages allocated for this CPU's ring buffer
    ulong nr_pages = read_ulong(addr + field_offset(ring_buffer_per_cpu,nr_pages),"nr_pages");
    if(nr_pages == 0) {
        LOGE("  Ring buffer has 0 pages, skipping");
        return;
    }
    ta->nr_pages = nr_pages;

    // Create and initialize ring buffer structure
    auto ring_buf_ptr = std::make_shared<ring_buffer_per_cpu>();
    ring_buf_ptr->addr = addr;
    ring_buf_ptr->cpu = read_int(addr + field_offset(ring_buffer_per_cpu,cpu),"cpu");

    // Read important page pointers from the ring buffer structure
    ulong commit_page = read_pointer(addr + field_offset(ring_buffer_per_cpu,commit_page),"commit_page");
    ulong reader_page = read_pointer(addr + field_offset(ring_buffer_per_cpu,reader_page),"reader_page");
    ulong head_page = read_pointer(addr + field_offset(ring_buffer_per_cpu,head_page),"head_page");
    ulong pages = read_pointer(addr + field_offset(ring_buffer_per_cpu,pages),"pages");

    LOGD("  Ring buffer has %lu pages: head=0x%lx, commit=0x%lx, reader=0x%lx, pages=0x%lx", nr_pages,
         head_page, commit_page, reader_page, pages);

    // Initialize buffer pages array
    ring_buf_ptr->buffer_pages.resize(nr_pages);
    size_t page_index = 0;
    ulong real_head_page = head_page;
    ulong bp_addr = pages;

    // Walk through the circular list of buffer pages
    while (is_kvaddr(bp_addr) && page_index < nr_pages){
        ring_buf_ptr->buffer_pages[page_index] = bp_addr;
        bp_addr = read_pointer(bp_addr + field_offset(buffer_page,list) + field_offset(list_head,next),"buffer_page");
        // Handle page pointer encoding (lower 2 bits may contain flags)
        if (bp_addr & 3) {
            bp_addr &= ~3;  // Clear lower 2 bits
            real_head_page = bp_addr;
        }
        page_index++;
    }
    LOGD("  Successfully walked through %zu buffer pages, head page: 0x%lx", page_index,real_head_page);
    // Find the index of the real head page
    ulong head_page_index = 0;
    for (ulong i = 0; i < ring_buf_ptr->buffer_pages.size(); i++){
        if (real_head_page == ring_buf_ptr->buffer_pages[i]){
            head_page_index = i;
            LOGD("  Found head page at index %lu", head_page_index);
            break;
        }
    }

    // Check if reader page has data and add it to data pages
    if (buffer_page_has_data(reader_page)){
        ring_buf_ptr->data_pages.push_back(reader_page);
        LOGD("  Reader page has data, added to data pages");
    }

    // Add pages from head to commit that contain data
    ulong index = head_page_index;
    size_t data_pages_added = 0;

    if (reader_page != commit_page){
        LOGD("  Adding data pages from head (index %lu) to commit page", head_page_index);
        while (true){
            ring_buf_ptr->data_pages.push_back(ring_buf_ptr->buffer_pages[index]);
            data_pages_added++;

            // Stop when we reach the commit page
            if (ring_buf_ptr->buffer_pages[index] == commit_page){
                LOGD("  Reached commit page at index %lu", index);
                break;
            }

            // Move to next page (circular)
            index++;
            if (index == nr_pages){
                index = 0;  // Wrap around
            }

            // Prevent infinite loop
            if (index == head_page_index) {
                break;
            }
        }
    }
    // Add the completed ring buffer to the trace array
    ta->cpu_ring_buffers.push_back(ring_buf_ptr);
    LOGD("  Successfully parsed ring_buffer_per_cpu for CPU#%d: %zu total pages, %zu data pages",
         ring_buf_ptr->cpu, ring_buf_ptr->buffer_pages.size(), ring_buf_ptr->data_pages.size());
}

/**
 * @brief Check if a buffer page contains valid trace data
 *
 * Determines whether a buffer page has any trace data by checking
 * the real_end field. A non-zero value indicates the page contains data.
 *
 * @param addr Kernel address of the buffer_page structure
 * @return Non-zero value if page has data, 0 if empty
 */
ulong Ftrace::buffer_page_has_data(ulong addr){
    if (!is_kvaddr(addr)) {
        LOGD("Invalid buffer page address 0x%lx", addr);
        return 0;
    }
    ulong real_end = read_ulong(addr + field_offset(buffer_page,real_end),"real_end");
    return real_end;
}


/**
 * @brief Parse a buffer page containing trace events
 *
 * This function parses a buffer page that contains trace events. It reads
 * the buffer_data_page structure and iterates through all ring buffer events
 * within the page, handling different event types including data events,
 * padding events, time extend events, and timestamp events.
 *
 * @param ta_ptr Shared pointer to trace array
 * @param rb_ptr Shared pointer to ring buffer per-CPU structure
 * @param addr Kernel address of the buffer_page structure
 */
void Ftrace::parse_buffer_page(std::shared_ptr<trace_array> ta_ptr, std::shared_ptr<ring_buffer_per_cpu> rb_ptr, ulong addr){
    LOGD("Parsing ring_buffer_per_cpu 0x%lx for array '%s' CPU#%d", addr, ta_ptr->name.c_str(), rb_ptr->cpu);

    // Get the buffer data page address
    ulong bdp_addr = read_pointer(addr + field_offset(buffer_page,page),"page");
    if (!is_kvaddr(bdp_addr)) {
        LOGE("Invalid buffer data page address 0x%lx", bdp_addr);
        return;
    }

    // Read the commit size (how much data is in this page)
    ulong commit = read_ulong(bdp_addr + field_offset(buffer_data_page,commit),"commit");
    if (commit <= 0) {
        return;
    }

    LOGD("Buffer page has committed %lu bytes", commit);

    // Read the base timestamp for this page
    uint64_t time_stamp = read_ulonglong(bdp_addr + field_offset(buffer_data_page,time_stamp),"time_stamp");
    ulong event_timestamp = 0;
    ulong event_end = bdp_addr + commit;
    ulong event_addr = bdp_addr + field_offset(buffer_data_page,data);
    ulong total_read = 0;
    // Parse all events in this buffer page
    while (total_read < commit){
        // Read the ring buffer event header
        struct ring_buffer_event rb_event;
        if(!read_struct(event_addr,&rb_event,sizeof(rb_event),"ring_buffer_event")){
            LOGE("Failed to read ring buffer event at offset %lu", total_read);
            break;
        }
        int trace_len = 0;
        event_timestamp += rb_event.time_delta;
        // so type_len is set to 0 and 32 bit array filed holds length
        // while payload starts afterwards at array[1]
        //                               event_addr                           array[]  trace_entry_addr
        //                                   |                                   |           |
        //                                   v                                   v           v
        //   ---------------------------------------------------------------------------------------------------------------------------
        //   |    struct buffer_data_page    |      struct ring_buffer_event     |    u32    | struct trace_entry  |  struct trace_xxx  |
        //   ----------------------------------------------------------------------------------------------------------------------------
        // Handle different types of ring buffer events
        if (rb_event.type_len == 0){
            // Extended length event: length is stored in array[0], data starts at array[1]
            ulong trace_entry_addr = event_addr + field_offset(ring_buffer_event,array) + sizeof(uint32_t);
            if (is_kvaddr(trace_entry_addr)) {
                parse_trace_entry(ta_ptr, rb_ptr, trace_entry_addr, time_stamp + event_timestamp);
            }
            trace_len = struct_size(ring_buffer_event) + rb_event.array[0];
        // Data Events
        //                               event_addr                      trace_entry_addr
        //                                   |                                   |
        //                                   v                                   v
        //   -----------------------------------------------------------------------------------------------
        //   |    struct buffer_data_page    |      struct ring_buffer_event     |   struct trace_entry    |
        //   -----------------------------------------------------------------------------------------------
        }else if (rb_event.type_len <= RINGBUF_TYPE_DATA_TYPE_LEN_MAX){
            // Regular data event: length encoded in type_len field
            ulong trace_entry_addr = event_addr + field_offset(ring_buffer_event,array);
            if (is_kvaddr(trace_entry_addr)) {
                parse_trace_entry(ta_ptr, rb_ptr, trace_entry_addr, time_stamp + event_timestamp);
            }
            trace_len = struct_size(ring_buffer_event) + (rb_event.type_len << 2);
        // Padding event or discarded event
        }else if (rb_event.type_len == RINGBUF_TYPE_PADDING){
            // Padding event: used to fill unused space at end of page
            if (event_addr > event_end){
                break;
            }
            if (rb_event.time_delta == 1){
                trace_len = struct_size(ring_buffer_event) + rb_event.array[0];
            }else{
                trace_len = struct_size(ring_buffer_event) + (event_end - event_addr);
            }
        // This is a time extend event
        //                               event_addr                           array[]
        //                                   |                                   |
        //                                   v                                   v
        //   ---------------------------------------------------------------------------------
        //   |    struct buffer_data_page    |      struct ring_buffer_event     |    u32    |
        //   ---------------------------------------------------------------------------------
        }else if (rb_event.type_len == RINGBUF_TYPE_TIME_EXTEND){
            // Time extend event: extends the timestamp range
            ulong time_extend = read_uint(event_addr + field_offset(ring_buffer_event,array),"array") << 27;
            event_timestamp += time_extend;
            trace_len = struct_size(ring_buffer_event) + sizeof(uint32_t);
        // Accounts for an absolute timestamp
        }else if (rb_event.type_len == RINGBUF_TYPE_TIME_STAMP){
            // Absolute timestamp event: resets the timestamp
            event_timestamp = 0;
            trace_len = struct_size(ring_buffer_event) + sizeof(uint32_t);
        }else{
            LOGE("Unknown ring buffer event type: %u", rb_event.type_len);
            break;
        }

        // Move to next event
        event_addr += trace_len;
        total_read += trace_len;
    }
}

/**
 * @brief Parse a single trace entry and create a formatted log entry
 *
 * This function parses a trace_entry structure from a ring buffer event,
 * validates the event type and PID, resolves the process command name,
 * formats the latency information, and creates a complete trace log entry.
 *
 * @param ta_ptr Shared pointer to trace array
 * @param rb_ptr Shared pointer to ring buffer per-CPU structure
 * @param addr Kernel address of the trace_entry structure
 * @param timestamp Absolute timestamp for this trace entry (in nanoseconds)
 */
void Ftrace::parse_trace_entry(std::shared_ptr<trace_array> ta_ptr, std::shared_ptr<ring_buffer_per_cpu> rb_ptr, ulong addr, ulong timestamp){
    // Read the trace entry header
    struct trace_entry entry;
    if(!read_struct(addr, &entry,sizeof(entry),"trace_entry")){
        LOGE("Failed to read trace_entry structure at 0x%lx", addr);
        return;
    }

    LOGD("trace_entry:0x%lx type=%u, flags=0x%x, preempt_count=%u, pid=%d", addr,
         entry.type, entry.flags, entry.preempt_count, entry.pid);

    // Validate event type exists in our event map
    if (event_maps.find(entry.type) == event_maps.end()) {
        LOGD("Event type %u not found in event maps, skipping", entry.type);
        return;
    }

    // Validate PID is within valid range
    if (entry.pid > pid_max){
        LOGE("PID %d exceeds pid_max %d, skipping", entry.pid, pid_max);
        return;
    }

    // Get the event handler for this event type
    std::shared_ptr<TraceEvent> event_ptr = event_maps[entry.type];
    if (event_ptr->name.empty()){
        LOGE("Event type %u has empty name, skipping", entry.type);
        return;
    }
    // Resolve process command name from PID
    std::string comm;
    if (comm_pid_dict.find(entry.pid) != comm_pid_dict.end()) {
        // Command name already cached
        comm = comm_pid_dict[entry.pid];
    }else{
        // Look up command name and cache it
        comm = find_cmdline(entry.pid);
    }

    // Format latency information (IRQ state, preemption, etc.)
    std::string lat_fmt = get_lat_fmt(entry.flags,entry.preempt_count);

    // Build the formatted trace log entry
    std::ostringstream oss;
    // Format: <comm-pid> [cpu] lat_fmt timestamp: event_name event_data
    oss << std::right << std::setw(25) << comm << " "
        << "[" << std::setfill('0') << std::setw(3) << rb_ptr->cpu << std::setfill(' ') << "] "
        << lat_fmt << " "
        << std::setw(12) << std::fixed << std::setprecision(6) << (double)timestamp/1000000000 << ": "
        << std::left << std::setw(30) << (event_ptr->name + ":") << " ";

    // Let the event handler format event-specific data
    event_ptr->handle(addr);
    event_ptr->print_log(oss);
    // Create and store the trace log entry
    std::shared_ptr<trace_log> log_ptr = std::make_shared<trace_log>();
    log_ptr->array_name = ta_ptr->name;
    log_ptr->cpu = rb_ptr->cpu;
    log_ptr->timestamp = timestamp;
    log_ptr->log = oss.str();
    trace_logs.push_back(log_ptr);
}

/**
 * @brief Find command name for a given PID
 *
 * This function resolves a PID to its corresponding process command name.
 * It first checks the kernel's saved command lines buffer, and falls back
 * to the task context if not found. The result is cached for future lookups.
 *
 * @param pid Process ID to look up
 * @return Formatted string containing command name and PID (e.g., "bash-1234")
 */
std::string Ftrace::find_cmdline(int pid){
    std::string comm = "<TBD>";
    // Special case: PID 0 is the idle task
    if (pid == 0){
        comm = "<idle>";
    }else if (savedcmd_ptr != nullptr){
        // Use saved command lines buffer to resolve PID
        // Hash PID to get index in the map
        int tpid = pid & (pid_max - 1);
        int index = read_int(savedcmd_ptr->map_pid_to_cmdline + tpid * sizeof(unsigned int),"map_pid_to_cmdline");
        ulong pid_addr = savedcmd_ptr->map_cmdline_to_pid + index * sizeof(unsigned int);
        if (index >= 0 && is_kvaddr(pid_addr)){
            // Verify the PID matches (handle hash collisions)
            int cmdline_tpid = read_int(pid_addr,"map_cmdline_to_pid");
            ulong comm_addr = savedcmd_ptr->saved_cmdlines + index * 16;

            if (cmdline_tpid == pid && is_kvaddr(comm_addr)){
                // Found matching PID, read command name
                comm = read_cstring(comm_addr, 16, "comm");
            }else{
                struct task_context * tc = pid_to_context(pid);
                if (tc){
                    comm = tc->comm;
                }
            }
        }
    }
    // Format as "command-pid" and cache the result
    std::string name = comm + "-" + std::to_string(pid);
    comm_pid_dict[pid] = name;
    LOGD("Resolved PID %d to: %s", pid, name.c_str());
    return name;
}

/**
 * @brief Parse the kernel's ftrace events list
 *
 * This function iterates through the kernel's ftrace_events list and parses
 * each trace_event_call structure. It builds a map of event IDs to event
 * handlers that will be used to format trace entries.
 */
void Ftrace::parser_ftrace_events_list(){
    LOGD("Starting to parse ftrace events list");

    if (!csymbol_exists("ftrace_events")){
        LOGE("ftrace_events symbol doesn't exist in this kernel");
        return;
    }

    ulong ftrace_events = csymbol_value("ftrace_events");
    if (!is_kvaddr(ftrace_events)) {
        LOGE("ftrace_events address 0x%lx is invalid", ftrace_events);
        return;
    }

    LOGD("ftrace_events found at address 0x%lx", ftrace_events);
    // Iterate through all trace event calls in the list
    for (const auto& addr : for_each_list(ftrace_events,field_offset(trace_event_call, list))) {
        std::shared_ptr<TraceEvent> event_ptr = parser_trace_event_call(addr);
        if (event_ptr != nullptr){
            event_maps[event_ptr->id] = event_ptr;
            LOGD("Added event '%s' (ID %u) to event map \n\n", event_ptr->name.c_str(), event_ptr->id);
        }else{
            LOGD("Skipped trace_event_call at 0x%lx (duplicate or invalid) \n\n", addr);
        }
    }
}

#define CREATE_EVENT_IF_MATCH(event_name)               \
else if (name == #event_name){                          \
    event_ptr = std::make_shared<event_name##_event>(); \
    event_ptr->name = name;                             \
    event_ptr->struct_type = "trace_event_raw_" + name; \
}


/**
 * @brief Parse a trace_event_call structure and create appropriate event handler
 *
 * This function parses a trace_event_call structure from kernel memory and creates
 * the appropriate TraceEvent handler based on the event type. It handles both
 * built-in ftrace events (like bprint, print, kernel_stack) and dynamically
 * registered trace events. The function also parses all event-specific fields.
 *
 * @param addr Kernel address of the trace_event_call structure
 * @return Shared pointer to created TraceEvent handler, or nullptr if duplicate/invalid
 */
std::shared_ptr<TraceEvent> Ftrace::parser_trace_event_call(ulong addr){
    LOGD("Parsing trace_event_call at address 0x%lx", addr);

    // Get event type ID
    uint id = get_event_type_id(addr);

    // Check if this event ID already exists (avoid duplicates)
    if (event_maps.find(id) != event_maps.end()) {
        LOGE("Event ID %u already exists in event map, skipping", id);
        return nullptr;
    }

    // Get event name
    std::string name = get_event_type_name(addr);
    std::shared_ptr<TraceEvent> event_ptr;
    // Create specialized event handlers for known event types
    if (false) { // TRACE_BPRINT
        //TODO
    }
    // Create specialized handlers for known trace events
    CREATE_EVENT_IF_MATCH(bprint)                        /**< Binary print event handler */
    CREATE_EVENT_IF_MATCH(print)                         /**< Print event handler */
    CREATE_EVENT_IF_MATCH(kernel_stack)                  /**< Kernel stack trace event handler */
    CREATE_EVENT_IF_MATCH(user_stack)                    /**< User stack trace event handler */
    CREATE_EVENT_IF_MATCH(bputs)                         /**< Binary puts event handler */

    // Scheduler events
    CREATE_EVENT_IF_MATCH(sched_switch)                  /**< Scheduler context switch event handler */

    // Interrupt events
    CREATE_EVENT_IF_MATCH(softirq_raise)                 /**< Soft IRQ raise event handler */
    CREATE_EVENT_IF_MATCH(softirq_entry)                 /**< Soft IRQ entry event handler */
    CREATE_EVENT_IF_MATCH(softirq_exit)                  /**< Soft IRQ exit event handler */
    CREATE_EVENT_IF_MATCH(irq_handler_exit)              /**< IRQ handler exit event handler */

    // Binder IPC events
    CREATE_EVENT_IF_MATCH(binder_return)                 /**< Binder return event handler */
    CREATE_EVENT_IF_MATCH(binder_command)                /**< Binder command event handler */

    // DWC3 USB controller events
    CREATE_EVENT_IF_MATCH(dwc3_ep_queue)                 /**< DWC3 endpoint queue event handler */
    CREATE_EVENT_IF_MATCH(dwc3_ep_dequeue)               /**< DWC3 endpoint dequeue event handler */
    CREATE_EVENT_IF_MATCH(dwc3_prepare_trb)              /**< DWC3 prepare TRB event handler */
    CREATE_EVENT_IF_MATCH(dwc3_gadget_giveback)          /**< DWC3 gadget giveback event handler */
    CREATE_EVENT_IF_MATCH(dwc3_gadget_ep_cmd)            /**< DWC3 gadget endpoint command event handler */
    CREATE_EVENT_IF_MATCH(dwc3_event)                    /**< DWC3 general event handler */
    CREATE_EVENT_IF_MATCH(dwc3_complete_trb)             /**< DWC3 complete TRB event handler */

    // DWC3 request events
    CREATE_EVENT_IF_MATCH(dwc3_alloc_request)            /**< DWC3 alloc request event handler */
    CREATE_EVENT_IF_MATCH(dwc3_free_request)             /**< DWC3 free request event handler */

    // DWC3 endpoint events
    CREATE_EVENT_IF_MATCH(dwc3_gadget_ep_enable)         /**< DWC3 gadget EP enable event handler */
    CREATE_EVENT_IF_MATCH(dwc3_gadget_ep_disable)        /**< DWC3 gadget EP disable event handler */

    // Memory-mapped I/O events
    CREATE_EVENT_IF_MATCH(rwmmio_read)                   /**< Read MMIO event handler */
    CREATE_EVENT_IF_MATCH(rwmmio_write)                  /**< Write MMIO event handler */
    CREATE_EVENT_IF_MATCH(rwmmio_post_read)              /**< Post-read MMIO event handler */
    CREATE_EVENT_IF_MATCH(rwmmio_post_write)             /**< Post-write MMIO event handler */

    // GPIO events
    CREATE_EVENT_IF_MATCH(gpio_value)                    /**< GPIO value change event handler */
    CREATE_EVENT_IF_MATCH(gpio_direction)                /**< GPIO direction change event handler */

    // DMA events
    CREATE_EVENT_IF_MATCH(dma_map_page)                  /**< DMA page mapping event handler */
    CREATE_EVENT_IF_MATCH(dma_unmap_page)                /**< DMA page unmapping event handler */
    CREATE_EVENT_IF_MATCH(dma_map_resource)              /**< DMA resource mapping event handler */
    CREATE_EVENT_IF_MATCH(dma_unmap_resource)            /**< DMA resource unmapping event handler */
    CREATE_EVENT_IF_MATCH(dma_alloc)                     /**< DMA allocation event handler */
    CREATE_EVENT_IF_MATCH(dma_free)                      /**< DMA free event handler */
    CREATE_EVENT_IF_MATCH(dma_map_sg)                    /**< DMA scatter-gather mapping event handler */
    CREATE_EVENT_IF_MATCH(dma_unmap_sg)                  /**< DMA scatter-gather unmapping event handler */
    CREATE_EVENT_IF_MATCH(dma_sync_single_for_cpu)       /**< DMA sync single for CPU event handler */
    CREATE_EVENT_IF_MATCH(dma_sync_single_for_device)    /**< DMA sync single for device event handler */
    CREATE_EVENT_IF_MATCH(dma_sync_sg_for_cpu)           /**< DMA sync scatter-gather for CPU event handler */
    CREATE_EVENT_IF_MATCH(dma_sync_sg_for_device)        /**< DMA sync scatter-gather for device event handler */
    CREATE_EVENT_IF_MATCH(swiotlb_bounced)               /**< SWIOTLB bounce event handler */

    CREATE_EVENT_IF_MATCH(sys_enter)
    CREATE_EVENT_IF_MATCH(hrtimer_init)
    CREATE_EVENT_IF_MATCH(hrtimer_start)
    CREATE_EVENT_IF_MATCH(timer_start)
    CREATE_EVENT_IF_MATCH(tick_stop)

    // Alarmtimer events
    CREATE_EVENT_IF_MATCH(alarmtimer_suspend)            /**< Alarmtimer suspend event handler */
    CREATE_EVENT_IF_MATCH(alarmtimer_fired)              /**< Alarmtimer fired event handler */
    CREATE_EVENT_IF_MATCH(alarmtimer_start)              /**< Alarmtimer start event handler */
    CREATE_EVENT_IF_MATCH(alarmtimer_cancel)             /**< Alarmtimer cancel event handler */

    // CPU idle and power management events
    CREATE_EVENT_IF_MATCH(cpu_idle_miss)                 /**< CPU idle miss event handler */
    CREATE_EVENT_IF_MATCH(suspend_resume)                /**< Suspend/resume event handler */

    // Memory management events
    CREATE_EVENT_IF_MATCH(mm_lru_insertion)              /**< Memory LRU insertion event handler */
    CREATE_EVENT_IF_MATCH(mm_vmscan_wakeup_kswapd)       /**< VM scan wakeup kswapd event handler */
    CREATE_EVENT_IF_MATCH(mm_vmscan_direct_reclaim_begin) /**< VM scan direct reclaim begin event handler */
    CREATE_EVENT_IF_MATCH(mm_vmscan_memcg_reclaim_begin) /**< VM scan memcg reclaim begin event handler */
    CREATE_EVENT_IF_MATCH(mm_vmscan_memcg_softlimit_reclaim_begin) /**< VM scan memcg softlimit reclaim begin event handler */
    CREATE_EVENT_IF_MATCH(mm_vmscan_node_reclaim_begin)  /**< VM scan node reclaim begin event handler */
    CREATE_EVENT_IF_MATCH(mm_shrink_slab_start)          /**< Shrink slab start event handler */
    CREATE_EVENT_IF_MATCH(kmalloc)                       /**< Kmalloc event handler */
    CREATE_EVENT_IF_MATCH(kmem_cache_alloc)              /**< Kmem cache alloc event handler */
    CREATE_EVENT_IF_MATCH(kmalloc_node)                  /**< Kmalloc node event handler */
    CREATE_EVENT_IF_MATCH(kmem_cache_alloc_node)         /**< Kmem cache alloc node event handler */
    CREATE_EVENT_IF_MATCH(mm_page_free)                  /**< Page free event handler */
    CREATE_EVENT_IF_MATCH(mm_page_free_batched)          /**< Page free batched event handler */
    CREATE_EVENT_IF_MATCH(mm_page_alloc)                 /**< Page alloc event handler */
    CREATE_EVENT_IF_MATCH(mm_page_alloc_zone_locked)     /**< Page alloc zone locked event handler */
    CREATE_EVENT_IF_MATCH(mm_page_pcpu_drain)            /**< Page per-CPU drain event handler */
    CREATE_EVENT_IF_MATCH(mm_page_alloc_extfrag)         /**< Page alloc external fragmentation event handler */
    CREATE_EVENT_IF_MATCH(rss_stat)                      /**< RSS stat event handler */

    // Writeback events
    CREATE_EVENT_IF_MATCH(writeback_mark_inode_dirty)    /**< Writeback mark inode dirty event handler */
    CREATE_EVENT_IF_MATCH(writeback_dirty_inode_start)   /**< Writeback dirty inode start event handler */
    CREATE_EVENT_IF_MATCH(writeback_dirty_inode)         /**< Writeback dirty inode event handler */
    CREATE_EVENT_IF_MATCH(writeback_queue)               /**< Writeback queue event handler */
    CREATE_EVENT_IF_MATCH(writeback_exec)                /**< Writeback exec event handler */
    CREATE_EVENT_IF_MATCH(writeback_start)               /**< Writeback start event handler */
    CREATE_EVENT_IF_MATCH(writeback_written)             /**< Writeback written event handler */
    CREATE_EVENT_IF_MATCH(writeback_wait)                /**< Writeback wait event handler */
    CREATE_EVENT_IF_MATCH(writeback_queue_io)            /**< Writeback queue IO event handler */
    CREATE_EVENT_IF_MATCH(writeback_sb_inodes_requeue)   /**< Writeback sb inodes requeue event handler */
    CREATE_EVENT_IF_MATCH(writeback_single_inode_start)  /**< Writeback single inode start event handler */
    CREATE_EVENT_IF_MATCH(writeback_single_inode)        /**< Writeback single inode event handler */
    CREATE_EVENT_IF_MATCH(writeback_lazytime)            /**< Writeback lazytime event handler */
    CREATE_EVENT_IF_MATCH(writeback_lazytime_iput)       /**< Writeback lazytime iput event handler */
    CREATE_EVENT_IF_MATCH(writeback_dirty_inode_enqueue) /**< Writeback dirty inode enqueue event handler */
    CREATE_EVENT_IF_MATCH(sb_mark_inode_writeback)       /**< SB mark inode writeback event handler */
    CREATE_EVENT_IF_MATCH(sb_clear_inode_writeback)      /**< SB clear inode writeback event handler */

    // SCSI events
    CREATE_EVENT_IF_MATCH(scsi_dispatch_cmd_start)       /**< SCSI dispatch command start event handler */
    CREATE_EVENT_IF_MATCH(scsi_dispatch_cmd_done)        /**< SCSI dispatch command done event handler */
    CREATE_EVENT_IF_MATCH(scsi_dispatch_cmd_error)       /**< SCSI dispatch command error event handler */
    CREATE_EVENT_IF_MATCH(scsi_dispatch_cmd_timeout)     /**< SCSI dispatch command timeout event handler */

    // Memory events
    CREATE_EVENT_IF_MATCH(mem_connect)                   /**< Memory connect event handler */
    CREATE_EVENT_IF_MATCH(mem_disconnect)                /**< Memory disconnect event handler */
    CREATE_EVENT_IF_MATCH(mem_return_failed)             /**< Memory return failed event handler */
    CREATE_EVENT_IF_MATCH(mm_filemap_add_to_page_cache)  /**< Filemap add to page cache event handler */
    CREATE_EVENT_IF_MATCH(mm_filemap_delete_from_page_cache) /**< Filemap delete from page cache event handler */
    CREATE_EVENT_IF_MATCH(reclaim_retry_zone)            /**< Reclaim retry zone event handler */
    CREATE_EVENT_IF_MATCH(compact_retry)                 /**< Compact retry event handler */

    // File lock events
    CREATE_EVENT_IF_MATCH(posix_lock_inode)              /**< POSIX lock inode event handler */
    CREATE_EVENT_IF_MATCH(fcntl_setlk)                   /**< Fcntl setlk event handler */
    CREATE_EVENT_IF_MATCH(locks_remove_posix)            /**< Locks remove POSIX event handler */
    CREATE_EVENT_IF_MATCH(flock_lock_inode)              /**< Flock lock inode event handler */
    CREATE_EVENT_IF_MATCH(break_lease_noblock)           /**< Break lease noblock event handler */
    CREATE_EVENT_IF_MATCH(break_lease_block)             /**< Break lease block event handler */
    CREATE_EVENT_IF_MATCH(break_lease_unblock)           /**< Break lease unblock event handler */
    CREATE_EVENT_IF_MATCH(generic_delete_lease)          /**< Generic delete lease event handler */
    CREATE_EVENT_IF_MATCH(time_out_leases)               /**< Time out leases event handler */
    CREATE_EVENT_IF_MATCH(generic_add_lease)             /**< Generic add lease event handler */
    CREATE_EVENT_IF_MATCH(leases_conflict)               /**< Leases conflict event handler */

    // Filesystem events (key events only)
    CREATE_EVENT_IF_MATCH(iomap_iter)                    /**< Iomap iter event handler */
    CREATE_EVENT_IF_MATCH(ext4_allocate_blocks)          /**< Ext4 allocate blocks event handler */
    CREATE_EVENT_IF_MATCH(ext4_free_blocks)              /**< Ext4 free blocks event handler */
    CREATE_EVENT_IF_MATCH(ext4_ext_map_blocks_enter)     /**< Ext4 ext map blocks enter event handler */
    CREATE_EVENT_IF_MATCH(ext4_ext_map_blocks_exit)      /**< Ext4 ext map blocks exit event handler */

    // Scheduler advanced events
    CREATE_EVENT_IF_MATCH(sched_switch_with_ctrs)        /**< Sched switch with counters event handler */
    CREATE_EVENT_IF_MATCH(sched_enq_deq_task)            /**< Sched enqueue/dequeue task event handler */

    // Devfreq events
    CREATE_EVENT_IF_MATCH(devfreq_monitor)               /**< Devfreq monitor event handler */
    CREATE_EVENT_IF_MATCH(devfreq_frequency)             /**< Devfreq frequency event handler */

    // UFS events
    CREATE_EVENT_IF_MATCH(ufshcd_command)                /**< UFS command event handler */
    CREATE_EVENT_IF_MATCH(ufshcd_clk_gating)             /**< UFS clock gating event handler */
    CREATE_EVENT_IF_MATCH(ufshcd_upiu)                   /**< UFS UPIU event handler */
    CREATE_EVENT_IF_MATCH(ufshcd_uic_command)            /**< UFS UIC command event handler */
    CREATE_EVENT_IF_MATCH(ufshcd_wl_runtime_resume)      /**< UFS WL runtime resume event handler */
    CREATE_EVENT_IF_MATCH(ufshcd_wl_runtime_suspend)     /**< UFS WL runtime suspend event handler */
    CREATE_EVENT_IF_MATCH(ufshcd_wl_resume)              /**< UFS WL resume event handler */
    CREATE_EVENT_IF_MATCH(ufshcd_wl_suspend)             /**< UFS WL suspend event handler */
    CREATE_EVENT_IF_MATCH(ufshcd_init)                   /**< UFS init event handler */
    CREATE_EVENT_IF_MATCH(ufshcd_runtime_resume)         /**< UFS runtime resume event handler */
    CREATE_EVENT_IF_MATCH(ufshcd_runtime_suspend)        /**< UFS runtime suspend event handler */
    CREATE_EVENT_IF_MATCH(ufshcd_system_resume)          /**< UFS system resume event handler */
    CREATE_EVENT_IF_MATCH(ufshcd_system_suspend)         /**< UFS system suspend event handler */

    // Scheduler WALT events
    CREATE_EVENT_IF_MATCH(sched_update_task_ravg)        /**< Sched update task RAVG event handler */
    CREATE_EVENT_IF_MATCH(sched_update_history)          /**< Sched update history event handler */
    CREATE_EVENT_IF_MATCH(sched_update_pred_demand)      /**< Sched update pred demand event handler */

    // Filesystem additional events
    CREATE_EVENT_IF_MATCH(locks_get_lock_context)        /**< Locks get lock context event handler */
    CREATE_EVENT_IF_MATCH(iomap_iter_dstmap)             /**< Iomap iter dstmap event handler */
    CREATE_EVENT_IF_MATCH(iomap_iter_srcmap)             /**< Iomap iter srcmap event handler */
    CREATE_EVENT_IF_MATCH(ext4_da_write_pages_extent)    /**< Ext4 da write pages extent event handler */
    CREATE_EVENT_IF_MATCH(ext4_request_blocks)           /**< Ext4 request blocks event handler */
    CREATE_EVENT_IF_MATCH(ext4_mballoc_alloc)            /**< Ext4 mballoc alloc event handler */
    CREATE_EVENT_IF_MATCH(ext4_fallocate_enter)          /**< Ext4 fallocate enter event handler */
    CREATE_EVENT_IF_MATCH(ext4_punch_hole)               /**< Ext4 punch hole event handler */
    CREATE_EVENT_IF_MATCH(ext4_zero_range)               /**< Ext4 zero range event handler */
    CREATE_EVENT_IF_MATCH(ext4_ind_map_blocks_enter)     /**< Ext4 ind map blocks enter event handler */
    CREATE_EVENT_IF_MATCH(ext4_ind_map_blocks_exit)      /**< Ext4 ind map blocks exit event handler */
    CREATE_EVENT_IF_MATCH(ext4_ext_handle_unwritten_extents) /**< Ext4 ext handle unwritten extents event handler */
    CREATE_EVENT_IF_MATCH(ext4_get_implied_cluster_alloc_exit) /**< Ext4 get implied cluster alloc exit event handler */
    CREATE_EVENT_IF_MATCH(ext4_es_insert_extent)         /**< Ext4 es insert extent event handler */
    CREATE_EVENT_IF_MATCH(ext4_es_cache_extent)          /**< Ext4 es cache extent event handler */
    CREATE_EVENT_IF_MATCH(ext4_es_find_extent_range_exit) /**< Ext4 es find extent range exit event handler */
    CREATE_EVENT_IF_MATCH(ext4_es_lookup_extent_exit)    /**< Ext4 es lookup extent exit event handler */
    CREATE_EVENT_IF_MATCH(ext4_es_insert_delayed_block)  /**< Ext4 es insert delayed block event handler */

    // Memory management additional events
    CREATE_EVENT_IF_MATCH(mm_vmscan_lru_isolate)         /**< VM scan LRU isolate event handler */
    CREATE_EVENT_IF_MATCH(mm_vmscan_writepage)           /**< VM scan writepage event handler */
    CREATE_EVENT_IF_MATCH(mm_vmscan_lru_shrink_inactive) /**< VM scan LRU shrink inactive event handler */
    CREATE_EVENT_IF_MATCH(mm_vmscan_lru_shrink_active)   /**< VM scan LRU shrink active event handler */
    CREATE_EVENT_IF_MATCH(mm_compaction_begin)           /**< Memory compaction begin event handler */
    CREATE_EVENT_IF_MATCH(mm_compaction_end)             /**< Memory compaction end event handler */
    CREATE_EVENT_IF_MATCH(mm_compaction_try_to_compact_pages) /**< Memory compaction try event handler */
    CREATE_EVENT_IF_MATCH(mm_compaction_finished)        /**< Memory compaction finished event handler */
    CREATE_EVENT_IF_MATCH(mm_compaction_suitable)        /**< Memory compaction suitable event handler */
    CREATE_EVENT_IF_MATCH(mm_compaction_deferred)        /**< Memory compaction deferred event handler */
    CREATE_EVENT_IF_MATCH(mm_compaction_defer_compaction) /**< Memory compaction defer event handler */
    CREATE_EVENT_IF_MATCH(mm_compaction_defer_reset)     /**< Memory compaction defer reset event handler */
    CREATE_EVENT_IF_MATCH(mm_compaction_wakeup_kcompactd) /**< Memory compaction wakeup kcompactd event handler */
    CREATE_EVENT_IF_MATCH(mm_compaction_kcompactd_wake)  /**< Memory compaction kcompactd wake event handler */
    CREATE_EVENT_IF_MATCH(mmap_lock_start_locking)       /**< Mmap lock start locking event handler */
    CREATE_EVENT_IF_MATCH(mmap_lock_acquire_returned)    /**< Mmap lock acquire returned event handler */
    CREATE_EVENT_IF_MATCH(mmap_lock_released)            /**< Mmap lock released event handler */
    CREATE_EVENT_IF_MATCH(vm_unmapped_area)              /**< VM unmapped area event handler */
    CREATE_EVENT_IF_MATCH(mm_migrate_pages)              /**< Page migration event handler */
    CREATE_EVENT_IF_MATCH(mm_migrate_pages_start)        /**< Page migration start event handler */
    CREATE_EVENT_IF_MATCH(mm_khugepaged_scan_pmd)        /**< Khugepaged scan PMD event handler */
    CREATE_EVENT_IF_MATCH(mm_collapse_huge_page)         /**< Collapse huge page event handler */
    CREATE_EVENT_IF_MATCH(mm_collapse_huge_page_isolate) /**< Collapse huge page isolate event handler */

    // SPMI events
    CREATE_EVENT_IF_MATCH(spmi_read_end)                 /**< SPMI read end event handler */
    CREATE_EVENT_IF_MATCH(spmi_write_begin)              /**< SPMI write begin event handler */

    // SPI events
    CREATE_EVENT_IF_MATCH(spi_transfer_start)            /**< SPI transfer start event handler */
    CREATE_EVENT_IF_MATCH(spi_transfer_stop)             /**< SPI transfer stop event handler */
    CREATE_EVENT_IF_MATCH(spi_set_cs)                    /**< SPI set CS event handler */
    CREATE_EVENT_IF_MATCH(spi_setup)                     /**< SPI setup event handler */

    // USB Gadget request events
    CREATE_EVENT_IF_MATCH(usb_gadget_giveback_request)   /**< USB gadget giveback request event handler */
    CREATE_EVENT_IF_MATCH(usb_ep_dequeue)                /**< USB EP dequeue event handler */
    CREATE_EVENT_IF_MATCH(usb_ep_queue)                  /**< USB EP queue event handler */
    CREATE_EVENT_IF_MATCH(usb_ep_free_request)           /**< USB EP free request event handler */
    CREATE_EVENT_IF_MATCH(usb_ep_alloc_request)          /**< USB EP alloc request event handler */

    // USB Gadget endpoint events
    CREATE_EVENT_IF_MATCH(usb_ep_enable)                 /**< USB EP enable event handler */
    CREATE_EVENT_IF_MATCH(usb_ep_disable)                /**< USB EP disable event handler */
    CREATE_EVENT_IF_MATCH(usb_ep_set_halt)               /**< USB EP set halt event handler */
    CREATE_EVENT_IF_MATCH(usb_ep_clear_halt)             /**< USB EP clear halt event handler */
    CREATE_EVENT_IF_MATCH(usb_ep_set_wedge)              /**< USB EP set wedge event handler */
    CREATE_EVENT_IF_MATCH(usb_ep_fifo_status)            /**< USB EP fifo status event handler */
    CREATE_EVENT_IF_MATCH(usb_ep_fifo_flush)             /**< USB EP fifo flush event handler */
    CREATE_EVENT_IF_MATCH(usb_ep_set_maxpacket_limit)    /**< USB EP set maxpacket limit event handler */

    // USB Gadget device events
    CREATE_EVENT_IF_MATCH(usb_gadget_frame_number)       /**< USB gadget frame number event handler */
    CREATE_EVENT_IF_MATCH(usb_gadget_wakeup)             /**< USB gadget wakeup event handler */
    CREATE_EVENT_IF_MATCH(usb_gadget_set_remote_wakeup)  /**< USB gadget set remote wakeup event handler */
    CREATE_EVENT_IF_MATCH(usb_gadget_set_selfpowered)    /**< USB gadget set selfpowered event handler */
    CREATE_EVENT_IF_MATCH(usb_gadget_clear_selfpowered)  /**< USB gadget clear selfpowered event handler */
    CREATE_EVENT_IF_MATCH(usb_gadget_vbus_connect)       /**< USB gadget vbus connect event handler */
    CREATE_EVENT_IF_MATCH(usb_gadget_vbus_draw)          /**< USB gadget vbus draw event handler */
    CREATE_EVENT_IF_MATCH(usb_gadget_vbus_disconnect)    /**< USB gadget vbus disconnect event handler */
    CREATE_EVENT_IF_MATCH(usb_gadget_connect)            /**< USB gadget connect event handler */
    CREATE_EVENT_IF_MATCH(usb_gadget_disconnect)         /**< USB gadget disconnect event handler */
    CREATE_EVENT_IF_MATCH(usb_gadget_deactivate)         /**< USB gadget deactivate event handler */
    CREATE_EVENT_IF_MATCH(usb_gadget_activate)           /**< USB gadget activate event handler */

    // xHCI DBC events
    CREATE_EVENT_IF_MATCH(xhci_dbc_alloc_request)        /**< xHCI DBC alloc request event handler */
    CREATE_EVENT_IF_MATCH(xhci_dbc_free_request)         /**< xHCI DBC free request event handler */
    CREATE_EVENT_IF_MATCH(xhci_dbc_queue_request)        /**< xHCI DBC queue request event handler */
    CREATE_EVENT_IF_MATCH(xhci_dbc_giveback_request)     /**< xHCI DBC giveback request event handler */

    // xHCI URB events
    CREATE_EVENT_IF_MATCH(xhci_urb_enqueue)              /**< xHCI URB enqueue event handler */
    CREATE_EVENT_IF_MATCH(xhci_urb_giveback)             /**< xHCI URB giveback event handler */
    CREATE_EVENT_IF_MATCH(xhci_urb_dequeue)              /**< xHCI URB dequeue event handler */

    // F2FS compression events
    CREATE_EVENT_IF_MATCH(f2fs_compress_pages_start)     /**< F2FS compress pages start event handler */
    CREATE_EVENT_IF_MATCH(f2fs_decompress_pages_start)   /**< F2FS decompress pages start event handler */

    // F2FS page operation events
    CREATE_EVENT_IF_MATCH(f2fs_writepage)                /**< F2FS writepage event handler */
    CREATE_EVENT_IF_MATCH(f2fs_do_write_data_page)       /**< F2FS do write data page event handler */
    CREATE_EVENT_IF_MATCH(f2fs_readpage)                 /**< F2FS readpage event handler */
    CREATE_EVENT_IF_MATCH(f2fs_set_page_dirty)           /**< F2FS set page dirty event handler */
    CREATE_EVENT_IF_MATCH(f2fs_vm_page_mkwrite)          /**< F2FS vm page mkwrite event handler */

    // F2FS BIO events
    CREATE_EVENT_IF_MATCH(f2fs_submit_page_bio)          /**< F2FS submit page bio event handler */
    CREATE_EVENT_IF_MATCH(f2fs_submit_page_write)        /**< F2FS submit page write event handler */
    CREATE_EVENT_IF_MATCH(f2fs_prepare_write_bio)        /**< F2FS prepare write bio event handler */
    CREATE_EVENT_IF_MATCH(f2fs_prepare_read_bio)         /**< F2FS prepare read bio event handler */
    CREATE_EVENT_IF_MATCH(f2fs_submit_read_bio)          /**< F2FS submit read bio event handler */
    CREATE_EVENT_IF_MATCH(f2fs_submit_write_bio)         /**< F2FS submit write bio event handler */

    // F2FS GC events
    CREATE_EVENT_IF_MATCH(f2fs_gc_begin)                 /**< F2FS GC begin event handler */
    CREATE_EVENT_IF_MATCH(f2fs_get_victim)               /**< F2FS get victim event handler */

    // F2FS sync events
    CREATE_EVENT_IF_MATCH(f2fs_sync_file_exit)           /**< F2FS sync file exit event handler */
    CREATE_EVENT_IF_MATCH(f2fs_sync_fs)                  /**< F2FS sync fs event handler */
    CREATE_EVENT_IF_MATCH(f2fs_write_checkpoint)         /**< F2FS write checkpoint event handler */
    CREATE_EVENT_IF_MATCH(f2fs_issue_flush)              /**< F2FS issue flush event handler */
    CREATE_EVENT_IF_MATCH(f2fs_sync_dirty_inodes_enter)  /**< F2FS sync dirty inodes enter event handler */
    CREATE_EVENT_IF_MATCH(f2fs_sync_dirty_inodes_exit)   /**< F2FS sync dirty inodes exit event handler */

    // F2FS extent tree events
    CREATE_EVENT_IF_MATCH(f2fs_lookup_extent_tree_start) /**< F2FS lookup extent tree start event handler */
    CREATE_EVENT_IF_MATCH(f2fs_shrink_extent_tree)       /**< F2FS shrink extent tree event handler */
    CREATE_EVENT_IF_MATCH(f2fs_destroy_extent_tree)      /**< F2FS destroy extent tree event handler */

    // F2FS other events
    CREATE_EVENT_IF_MATCH(f2fs_truncate_partial_nodes)   /**< F2FS truncate partial nodes event handler */
    CREATE_EVENT_IF_MATCH(f2fs_writepages)               /**< F2FS writepages event handler */
    CREATE_EVENT_IF_MATCH(f2fs_shutdown)                 /**< F2FS shutdown event handler */

    // V4L2 videobuf2 events
    CREATE_EVENT_IF_MATCH(vb2_v4l2_buf_done)             /**< VB2 V4L2 buf done event handler */
    CREATE_EVENT_IF_MATCH(vb2_v4l2_buf_queue)            /**< VB2 V4L2 buf queue event handler */
    CREATE_EVENT_IF_MATCH(vb2_v4l2_dqbuf)                /**< VB2 V4L2 dqbuf event handler */
    CREATE_EVENT_IF_MATCH(vb2_v4l2_qbuf)                 /**< VB2 V4L2 qbuf event handler */

    // V4L2 events
    CREATE_EVENT_IF_MATCH(v4l2_dqbuf)                    /**< V4L2 dqbuf event handler */
    CREATE_EVENT_IF_MATCH(v4l2_qbuf)                     /**< V4L2 qbuf event handler */

    // I2C events
    CREATE_EVENT_IF_MATCH(i2c_write)                     /**< I2C write event handler */
    CREATE_EVENT_IF_MATCH(i2c_reply)                     /**< I2C reply event handler */
    CREATE_EVENT_IF_MATCH(i2c_read)                      /**< I2C read event handler */
    CREATE_EVENT_IF_MATCH(i2c_result)                    /**< I2C result event handler */

    // SMBus events
    CREATE_EVENT_IF_MATCH(smbus_write)                   /**< SMBus write event handler */
    CREATE_EVENT_IF_MATCH(smbus_read)                    /**< SMBus read event handler */
    CREATE_EVENT_IF_MATCH(smbus_reply)                   /**< SMBus reply event handler */
    CREATE_EVENT_IF_MATCH(smbus_result)                  /**< SMBus result event handler */

    // Thermal events
    CREATE_EVENT_IF_MATCH(thermal_zone_trip)             /**< Thermal zone trip event handler */
    CREATE_EVENT_IF_MATCH(thermal_power_cpu_get_power)   /**< Thermal power CPU get power event handler */
    CREATE_EVENT_IF_MATCH(thermal_power_devfreq_get_power) /**< Thermal power devfreq get power event handler */
    CREATE_EVENT_IF_MATCH(thermal_power_allocator)       /**< Thermal power allocator event handler */

    // RCU events
    CREATE_EVENT_IF_MATCH(rcu_batch_end)                 /**< RCU batch end event handler */
    CREATE_EVENT_IF_MATCH(rcu_segcb_stats)               /**< RCU segcb stats event handler */

    // SMC Invoke events
    CREATE_EVENT_IF_MATCH(smcinvoke_ioctl)               /**< SMC invoke ioctl event handler */

    CREATE_EVENT_IF_MATCH(module_load)                   /**< module event handler */
    else{
        // Generic event handler for unknown event types
        event_ptr = std::make_shared<TraceEvent>();
        event_ptr->name = name;
        event_ptr->struct_type = "trace_event_raw_" + name;
    }
    // Set event properties
    event_ptr->set_print_format(get_event_type_print_fmt(addr));
    event_ptr->system = get_event_type_system(addr);
    event_ptr->id = id;
    event_ptr->plugin_ptr = this;
    LOGD("=================================");
    LOGD("TraceEvent name: %s", name.c_str());
    LOGD("TraceEvent id: %u", id);
    LOGD("TraceEvent system: %s", event_ptr->system.c_str());
    LOGD("TraceEvent format: %s", event_ptr->org_print_fmt.c_str());
    LOGD("TraceEvent struct: %s", event_ptr->struct_type.c_str());
    LOGD("==================================");
    // Validate event has required properties
    if (event_ptr->name.empty() || event_ptr->system.empty()){
        LOGD("Event has empty name or system, rejecting");
        return nullptr;
    }
    // Parse event-specific fields from the event class
    ulong event_class = read_pointer(addr + field_offset(trace_event_call,class),"event_class");
    if (is_kvaddr(event_class)) {
        for (const auto& field_addr : for_each_list(event_class + field_offset(trace_event_class, fields),field_offset(ftrace_event_field, link))) {
            auto field_ptr = parser_event_field(field_addr);
            event_ptr->field_maps[field_ptr->name] = field_ptr;
        }
    }
    return event_ptr;
}

/**
 * @brief Parse common trace fields shared by all trace events
 *
 * This function parses the ftrace_common_fields list which contains field
 * definitions that are common to all trace events (like type, flags, pid, etc.).
 * These fields are present in every trace_entry structure.
 */
void Ftrace::parser_common_trace_fields(){
    LOGD("Parsing common trace fields");
    if (!csymbol_exists("ftrace_common_fields")){
        LOGD("ftrace_common_fields symbol doesn't exist, skipping");
        return;
    }

    ulong ftrace_common_addr = csymbol_value("ftrace_common_fields");
    LOGD("ftrace_common_fields found at address 0x%lx", ftrace_common_addr);

    // Parse each common field
    for (const auto& field_addr : for_each_list(ftrace_common_addr,field_offset(ftrace_event_field, link))) {
        auto field_ptr = parser_event_field(field_addr);
        common_field_maps[field_ptr->name] = field_ptr;
    }
}

/**
 * @brief Get event type ID from trace_event_call structure
 *
 * Reads the event type ID from a trace_event_call structure. The ID uniquely
 * identifies each trace event type in the system.
 *
 * @param addr Kernel address of the trace_event_call structure
 * @return Event type ID
 */
uint Ftrace::get_event_type_id(ulong addr){
    uint id = read_uint(addr + field_offset(trace_event_call,event) + field_offset(trace_event,type),"type");
    return id;
}

/**
 * @brief Get event type name from trace_event_call structure
 *
 * Reads the event name from a trace_event_call structure. The method of
 * retrieving the name depends on the kernel version and whether the event
 * is a tracepoint-based event.
 *
 * @param addr Kernel address of the trace_event_call structure
 * @return Event name string
 */
std::string Ftrace::get_event_type_name(ulong addr){
    int flags = read_int(addr + field_offset(trace_event_call,flags),"flags");

    // Determine TRACE_EVENT_FL_TRACEPOINT flag value based on kernel version
    int TRACE_EVENT_FL_TRACEPOINT = 0;
    if (THIS_KERNEL_VERSION >= LINUX(4, 14, 0)){
        TRACE_EVENT_FL_TRACEPOINT = 0x10;
    }else if (THIS_KERNEL_VERSION >= LINUX(4, 9, 0)){
        TRACE_EVENT_FL_TRACEPOINT = 0x20;
    }else{
        TRACE_EVENT_FL_TRACEPOINT = 0x40;
    }
    std::string name="";
    // For newer kernels with tracepoint flag, get name from tracepoint structure
    if (THIS_KERNEL_VERSION >= LINUX(3, 18, 0) && (flags & TRACE_EVENT_FL_TRACEPOINT)){
        ulong tracepoint_addr = read_pointer(addr + field_offset(trace_event_call,tp),"tracepoint addr");
        if (is_kvaddr(tracepoint_addr)) {
            ulong name_addr = read_pointer(tracepoint_addr + field_offset(tracepoint,name),"name addr");
            if (is_kvaddr(name_addr)) {
                name = read_cstring(name_addr,64, "name");
            }
        }
    }else{
        // For older kernels or non-tracepoint events, get name directly
        ulong name_addr = read_pointer(addr + field_offset(trace_event_call,name),"name addr");
        if (is_kvaddr(name_addr)) {
            name = read_cstring(name_addr,64, "name");
        }
    }
    return name;
}

/**
 * @brief Get event system name from trace_event_call structure
 *
 * Reads the system/subsystem name that this event belongs to (e.g., "sched",
 * "irq", "binder"). Events are organized into systems for categorization.
 *
 * @param addr Kernel address of the trace_event_call structure
 * @return System name string
 */
std::string Ftrace::get_event_type_system(ulong addr){
    std::string name;
    // Read the event class pointer
    ulong class_addr = read_pointer(addr + field_offset(trace_event_call,class),"class addr");
    if (is_kvaddr(class_addr)) {
        // Read the system name from the event class
        ulong name_addr = read_pointer(class_addr + field_offset(trace_event_class,system),"system addr");
        if (is_kvaddr(name_addr)) {
            name = read_cstring(name_addr,64, "name");
        }
    }
    return name;
}

/**
 * @brief Get event print format string from trace_event_call structure
 *
 * Reads the printf-style format string used to display this event's data.
 * This format string contains placeholders for event-specific fields.
 *
 * @param addr Kernel address of the trace_event_call structure
 * @return Print format string
 */
std::string Ftrace::get_event_type_print_fmt(ulong addr){
    std::string print_fmt;
    ulong fmt_addr = read_pointer(addr + field_offset(trace_event_call,print_fmt),"print_fmt addr");
    if (is_kvaddr(fmt_addr)) {
        print_fmt = read_long_string(fmt_addr,"print_fmt");
    }
    return print_fmt;
}

/**
 * @brief Parse an event field structure
 *
 * Reads a ftrace_event_field structure which describes a single field within
 * a trace event. This includes the field's name, type, offset, size, and
 * whether it's signed.
 *
 * @param addr Kernel address of the ftrace_event_field structure
 * @return Shared pointer to parsed trace_field structure
 */
std::shared_ptr<trace_field> Ftrace:: parser_event_field(ulong addr){
    std::shared_ptr<trace_field> field_ptr = std::make_shared<trace_field>();

    // Read field properties
    field_ptr->offset = read_int(addr + field_offset(ftrace_event_field,offset),"offset");
    field_ptr->size = read_int(addr + field_offset(ftrace_event_field,size),"size");
    field_ptr->is_signed = read_int(addr + field_offset(ftrace_event_field,is_signed),"is_signed");
    field_ptr->filter_type = read_int(addr + field_offset(ftrace_event_field,filter_type),"filter_type");

    // Read field name
    ulong name_addr = read_pointer(addr + field_offset(ftrace_event_field,name),"name addr");
    if (is_kvaddr(name_addr)) {
        field_ptr->name = read_cstring(name_addr,512, "name");
    }else{
        field_ptr->name = "";
    }

    // Read field type
    ulong type_addr = read_pointer(addr + field_offset(ftrace_event_field,type),"type addr");
    if (is_kvaddr(type_addr)) {
        field_ptr->type = read_cstring(type_addr,512, "type");
    }else{
        field_ptr->type = "";
    }

    LOGD("Parsed ftrace_event_field at address 0x%lx: %s %s (offset=%d, size=%d, signed=%d)", addr,
         field_ptr->type.c_str(), field_ptr->name.c_str(),
         field_ptr->offset, field_ptr->size, field_ptr->is_signed);

    return field_ptr;
}

/**
 * @brief Format latency information into a human-readable string
 *
 * Converts trace entry flags and preempt count into a 5-character latency
 * format string that indicates IRQ state, scheduling state, and context.
 * Format: [irqs_off][need_resched][hardsoft_irq][preempt_depth][preempt_count]
 *
 * Examples:
 *   "d..2." - IRQs disabled, preempt depth 2
 *   "..s1." - In softirq context, preempt depth 1
 *   "Dn.3." - IRQs disabled + BH off, need resched, preempt depth 3
 *
 * @param flags Trace entry flags indicating IRQ/context state
 * @param preempt_count Preemption count and depth
 * @return 5-character latency format string
 */
std::string Ftrace::get_lat_fmt(unsigned char flags, unsigned char preempt_count) {
    std::string lat_fmt;

    // Extract context flags
    bool nmi = flags & TRACE_FLAG_NMI;
    bool hardirq = flags & TRACE_FLAG_HARDIRQ;
    bool softirq = flags & TRACE_FLAG_SOFTIRQ;
    bool bh_off = flags & TRACE_FLAG_BH_OFF;

    // Character 1: IRQ state
    char irqs_off;
    if ((flags & TRACE_FLAG_IRQS_OFF) && bh_off) {
        irqs_off = 'D';  // IRQs disabled + BH off
    } else if (flags & TRACE_FLAG_IRQS_OFF) {
        irqs_off = 'd';  // IRQs disabled
    } else if (bh_off) {
        irqs_off = 'b';  // BH off
    } else if (flags & TRACE_FLAG_IRQS_NOSUPPORT) {
        irqs_off = 'X';  // IRQ state unknown
    } else {
        irqs_off = '.';  // IRQs enabled
    }

    // Character 2: Scheduling state
    unsigned char resched = flags & (TRACE_FLAG_NEED_RESCHED | TRACE_FLAG_PREEMPT_RESCHED);
    char need_resched;
    if (resched == (TRACE_FLAG_NEED_RESCHED | TRACE_FLAG_PREEMPT_RESCHED)) {
        need_resched = 'N';  // Both need resched flags set
    } else if (resched == TRACE_FLAG_NEED_RESCHED) {
        need_resched = 'n';  // Need resched
    } else if (resched == TRACE_FLAG_PREEMPT_RESCHED) {
        need_resched = 'p';  // Preempt resched
    } else {
        need_resched = '.';  // No resched needed
    }

    // Character 3: IRQ/NMI context
    char hardsoft_irq;
    if (nmi && hardirq) {
        hardsoft_irq = 'Z';  // NMI + hardirq
    } else if (nmi) {
        hardsoft_irq = 'z';  // NMI context
    } else if (hardirq && softirq) {
        hardsoft_irq = 'H';  // Hardirq + softirq
    } else if (hardirq) {
        hardsoft_irq = 'h';  // Hardirq context
    } else if (softirq) {
        hardsoft_irq = 's';  // Softirq context
    } else {
        hardsoft_irq = '.';  // Normal context
    }

    // Build the latency format string
    lat_fmt += irqs_off;
    lat_fmt += need_resched;
    lat_fmt += hardsoft_irq;

    // Character 4: Preempt depth (lower 4 bits)
    if (preempt_count & 0xf) {
        lat_fmt += std::to_string(preempt_count & 0xf)[0];
    } else {
        lat_fmt += '.';
    }

    // Character 5: Preempt count (upper 4 bits)
    if (preempt_count & 0xf0) {
        lat_fmt += std::to_string((preempt_count >> 4))[0];
    } else {
        lat_fmt += '.';
    }

    return lat_fmt;
}

#define TRACE_CMD_FILE_VERSION_STRING "6"

/**
 * @brief Write complete trace data to file in trace-cmd format
 *
 * This is the main function that orchestrates writing all trace data to a file
 * in the trace-cmd format. It writes data in the following order:
 * 1. Initialization data (magic, version, endianness, page size)
 * 2. Header data (page and event format)
 * 3. Events data (all trace event definitions)
 * 4. Kallsyms data (kernel symbol table)
 * 5. Printk data (bprintk format strings)
 * 6. Command lines (PID to command name mapping)
 * 7. Resource data (trace array options)
 * 8. Buffer data (actual trace data from ring buffers)
 *
 * @return true on success, false on failure
 */
bool Ftrace::write_trace_data(){
    // Write file header and metadata
    write_init_data();
    write_header_data();
    write_events_data();
    write_kallsyms_data();
    write_printk_data();
    write_cmdlines();
    // Save position for resource data (will be updated later with offsets)
    ulonglong res_data_offset = ftell(trace_file);
    // Write resource data (first pass - placeholder offsets)
    write_res_data();
    // Write actual trace buffer data
    write_buffer_data();
    // Go back and update resource data with actual offsets
    fseek(trace_file, res_data_offset, SEEK_SET);
    write_res_data();
    // Flush all data to disk
    fflush(trace_file);
    return true;
}

/**
 * @brief Write buffer data for all trace arrays
 *
 * Writes the actual trace data from ring buffers to the file. The global
 * trace array is written first, followed by all other trace arrays (instances).
 * Each non-global array is prefixed with "flyrecord" marker.
 */
void Ftrace::write_buffer_data(){
    // Write global trace array first
    for (const auto& ta_ptr : trace_list) {
        if (ta_ptr->name != "global") continue;
        ta_ptr->offset = ftell(trace_file);
        write_trace_array_buffers(ta_ptr);
    }

    // Write other trace arrays (instances)
    for (const auto& ta_ptr : trace_list) {
        if (ta_ptr->name == "global") continue;
        ta_ptr->offset = ftell(trace_file);

        // Write flyrecord marker for non-global arrays
        fwrite("flyrecord", 10, 1, trace_file);
        write_trace_array_buffers(ta_ptr);
    }
}

// +--------------------------------+
// | CPU Buffer Headers             |
// |--------------------------------|<---- offset
// | [CPU 0] Offset (8 bytes)       |
// | [CPU 0] Size   (8 bytes)       |
// |--------------------------------|
// | [CPU 1] Offset (8 bytes)       |
// | [CPU 1] Size   (8 bytes)       |
// |--------------------------------|
// | ...                            |
// |--------------------------------|
// | Padding to page_size           |
// +------------------------------- +<---- buffer_offset
// | CPU 0 data                     |
// +--------------------------------+
// | CPU 1 data                     |
// +--------------------------------+
// | ...                            |
// +--------------------------------+
/**
 * @brief Write trace array buffers in trace-cmd format
 *
 * Writes the ring buffer data for a trace array. The format consists of:
 * 1. CPU buffer headers (offset and size for each CPU)
 * 2. Padding to page boundary
 * 3. Actual buffer data pages for each CPU
 *
 * @param ta_ptr Shared pointer to trace array to write
 */
void Ftrace::write_trace_array_buffers(std::shared_ptr<trace_array> ta_ptr) {
    LOGD("Writing buffers for trace array %s",ta_ptr->name.c_str());

    // Calculate header layout
    size_t header_start_offset = ftell(trace_file);
    size_t header_total_size = ta_ptr->cpus * 16;  // 16 bytes per CPU (offset + size)
    size_t data_start_offset = roundup(header_start_offset + header_total_size, page_size);

    // Write CPU buffer headers (offset and size for each CPU)
    size_t buffer_offset = data_start_offset;
    size_t total_buffer_size = 0;

    for (int cpu = 0; cpu < ta_ptr->cpus; cpu++) {
        auto& ring_buffer = ta_ptr->cpu_ring_buffers[cpu];
        size_t buffer_size = page_size * ring_buffer->data_pages.size();

        fwrite(&buffer_offset, 8, 1, trace_file);
        fwrite(&buffer_size, 8, 1, trace_file);

        buffer_offset += buffer_size;
        total_buffer_size += buffer_size;
    }

    // Seek to data start (after padding)
    fseek(trace_file, data_start_offset, SEEK_SET);
    LOGD("Writing %zu bytes of buffer data", total_buffer_size);

    // Write actual buffer data for each CPU
    size_t pages_written = 0;
    for (int cpu = 0; cpu < ta_ptr->cpus; cpu++) {
        auto& ring_buffer = ta_ptr->cpu_ring_buffers[cpu];

        LOGD("Writing %zu data pages for CPU %d", ring_buffer->data_pages.size(), cpu);

        for (const auto& buffer_page : ring_buffer->data_pages) {
            ulong page = read_pointer(buffer_page + field_offset(buffer_page, page), "buffer_page page");

            // Read page from kernel memory
            char* buf = (char*)read_memory(page, page_size, "get page context");
            if (buf) {
                fwrite(buf, page_size, 1, trace_file);
                FREEBUF(buf);
                pages_written++;
            } else {
                LOGE("Failed to read page at address 0x%lx", page);
            }
        }
    }

    LOGD("Successfully wrote %zu pages for trace array '%s'",
         pages_written, ta_ptr->name.c_str());
}

// +------------------------------+
// | nr_cpu_buffers (4 bytes)     |
// +------------------------------+
// | "options  " (10 bytes)       |
// +------------------------------+
// | option (2 bytes)             |
// +------------------------------+
// | option_size (4 bytes)        |
// +------------------------------+
// | offset (8 bytes)             |
// +------------------------------+
// | name (N+1 bytes)             |
// +------------------------------+
// | ...                          |
// +------------------------------+
// | TRACECMD_OPTION_DONE (2B)    |
// +------------------------------+
// | "flyrecord" (10 bytes)       |
// +------------------------------+
/**
 * @brief Write resource data section
 *
 * Writes the resource data section which includes:
 * - Number of CPUs
 * - Options section with trace array instance information
 * - Flyrecord marker
 *
 * This function is called twice: once to write placeholder data, and again
 * to update with actual buffer offsets after buffer data is written.
 */
void Ftrace::write_res_data(){
    // Write number of CPUs
    fwrite(&nr_cpu_ids, 4, 1, trace_file);
    // Write options section
    fwrite("options  ", 10, 1, trace_file);

    // Write buffer options for each non-global trace array
    unsigned short option = TRACECMD_OPTION_BUFFER;
    size_t instance_count = 0;

    for (const auto& ta_ptr : trace_list) {
        if(ta_ptr->name == "global"){
            continue;
        }

        // Write option type
        fwrite(&option, 2, 1, trace_file);

        // Write option size (name length + null terminator + offset size)
        ulonglong option_size = ta_ptr->name.size() + 1 + 8;
        fwrite(&option_size, 4, 1, trace_file);

        // Write buffer offset
        fwrite(&ta_ptr->offset, 8, 1, trace_file);

        // Write instance name
        fwrite(ta_ptr->name.c_str(), ta_ptr->name.size() + 1, 1, trace_file);
        instance_count++;
    }

    // Write option done marker
    option = TRACECMD_OPTION_DONE;
    fwrite(&option, 2, 1, trace_file);

    // Write flyrecord marker
    fwrite("flyrecord", 10, 1, trace_file);
}

/**
 * @brief Write command lines section
 *
 * Writes the mapping of PIDs to command names for all running tasks.
 * This allows trace-cmd to display process names in trace output.
 * Format: <size><pid> <command>\n<pid> <command>\n...
 */
void Ftrace::write_cmdlines(){
    std::vector<char> data_buf;
    size_t pos = 0;
    struct task_context *tc = FIRST_CONTEXT();
    ulong task_count = RUNNING_TASKS();
    // Build command lines buffer
    for (ulong i = 0; i < task_count; i++){
        write_to_buf(data_buf, pos, "%d %s\n",  (int)tc[i].pid, tc[i].comm);
    }

    // Write size followed by data
    fwrite(&pos, 8, 1, trace_file);
    fwrite(data_buf.data(), data_buf.size() - 1, 1, trace_file);
    LOGD("cmdlines data:\n%s \n\n", hexdump(0x1000,(char*)data_buf.data(),data_buf.size()-1).c_str());
}

/**
 * @brief Write printk format strings section
 *
 * Writes the bprintk format strings used by trace_bprintk() in the kernel.
 * These format strings are stored at specific addresses and referenced by
 * trace events. This section includes both built-in and module format strings.
 * Format: <size><address> : "format string"\n...
 */
void Ftrace::write_printk_data(){
    // Check if bprintk format symbols exist
    if (!csymbol_exists("__start___trace_bprintk_fmt") || !csymbol_exists("__stop___trace_bprintk_fmt")){
        LOGD("bprintk format symbols not found, skipping printk data");
        return;
    }

    std::vector<char> data_buf;
    size_t pos = 0;

    // Get bprintk format string array bounds
    ulong bprintk_fmt_s = csymbol_value("__start___trace_bprintk_fmt");
    ulong bprintk_fmt_e = csymbol_value("__stop___trace_bprintk_fmt");
    size_t count = (bprintk_fmt_e - bprintk_fmt_s) / sizeof(long);

    // Process built-in format strings
    if (count != 0) {
        ulong bprintks[count];
        if(read_struct(bprintk_fmt_s,bprintks, count * sizeof(ulong), "__trace_bprintk_fmt")){
            size_t valid_count = 0;

            for (size_t i = 0; i < count; i++) {
                if (!is_kvaddr(bprintks[i])) {
                    continue;
                }

                char tmpbuf[4096];
                size_t len = read_string(bprintks[i], tmpbuf, sizeof(tmpbuf));
                if (!len){
                    continue;
                }

                // Write format string address and escaped content
                write_to_buf(data_buf, pos, "0x%lx : \"",  bprintks[i]);

                // Escape special characters in format string
                for (size_t j = 0; tmpbuf[j]; j++) {
                    switch (tmpbuf[j]) {
                        case '\n':
                            write_to_buf(data_buf, pos, "\\n");
                            break;
                        case '\t':
                            write_to_buf(data_buf, pos, "\\t");
                            break;
                        case '\\':
                            write_to_buf(data_buf, pos, "\\\\");
                            break;
                        case '"':
                            write_to_buf(data_buf, pos, "\\\"");
                            break;
                        default:
                            write_to_buf(data_buf, pos, "%c", tmpbuf[j]);
                            break;
                    }
                }
                write_to_buf(data_buf, pos, "\"\n");
                valid_count++;
            }
        }
    }

    // Add module format strings
    if (csymbol_exists("trace_bprintk_fmt_list")){
        // Determine if format address is stored as array or pointer
        int addr_is_array = 0;
        switch (MEMBER_TYPE(TO_CONST_STRING("trace_bprintk_fmt"), TO_CONST_STRING("fmt"))) {
            case TYPE_CODE_ARRAY:
                addr_is_array = 1;
                break;
            case TYPE_CODE_PTR:
            default:
                break;
        }

        ulong fmt_list = csymbol_value("trace_bprintk_fmt_list");
        struct kernel_list_head mod_fmt;

        if(read_struct(fmt_list, &mod_fmt, sizeof(mod_fmt), "list_head")){
            // Iterate through module format string list
            while ((ulong)mod_fmt.next != fmt_list){
                ulong node_addr = (ulong)mod_fmt.next + sizeof(mod_fmt);
                if (!addr_is_array) {
                    node_addr = read_ulong(node_addr,"node_addr");
                }

                if(!read_struct((ulong)mod_fmt.next, &mod_fmt, sizeof(mod_fmt), "list_head")){
                    LOGD("Failed to read next list entry");
                    break;
                }

                char tmpbuf[4096];
                size_t len = read_string(node_addr, tmpbuf, sizeof(tmpbuf));
                if (!len){
                    continue;
                }

                // Write module format string
                write_to_buf(data_buf, pos, "0x%lx : \"",  node_addr);

                // Escape special characters
                for (size_t i = 0; tmpbuf[i]; i++) {
                    switch (tmpbuf[i]) {
                    case '\n':
                        write_to_buf(data_buf, pos, "\\n");
                        break;
                    case '\t':
                        write_to_buf(data_buf, pos, "\\t");
                        break;
                    case '\\':
                        write_to_buf(data_buf, pos, "\\\\");
                        break;
                    case '"':
                        write_to_buf(data_buf, pos, "\\\"");
                        break;
                    default:
                        write_to_buf(data_buf, pos, "%c", tmpbuf[i]);
                        break;
                    }
                }
                write_to_buf(data_buf, pos, "\"\n");
                count++;
            }
        }
    }

    // Write data to file
    if (count == 0){
        LOGD("No format strings found, writing empty section");
        unsigned int size = 0;
        fwrite(&size, 4, 1, trace_file);
    } else {
        fwrite(&pos, 4, 1, trace_file);
        fwrite(data_buf.data(), data_buf.size() - 1, 1, trace_file);
        LOGD("printk format strings:\n%s \n\n", hexdump(0x1000,(char*)data_buf.data(),data_buf.size()-1).c_str());
    }
}

/**
 * @brief Write kernel symbol table (kallsyms) section
 *
 * Writes the kernel symbol table which maps addresses to symbol names.
 * This includes both built-in kernel symbols and module symbols.
 * Format: <size><address> <type> <name>\n...
 */
void Ftrace::write_kallsyms_data(){
    struct syment *sp;
    std::vector<char> sym_buf;
    size_t pos = 0;

    // Write built-in kernel symbols
    for (sp = st->symtable; sp < st->symend; sp++){
        if (!sp) continue;
        write_to_buf(sym_buf, pos, "%lx %c %s\n",  sp->value, sp->type, sp->name);
    }
    // Write module symbols (kernel version dependent)
    if (MODULE_MEMORY()){
        save_proc_kallsyms_mod_v6_4(sym_buf, pos);
    }else{
        save_proc_kallsyms_mod_legacy(sym_buf, pos);
    }
    // Write size and data
    fwrite(&pos, 4, 1, trace_file);
    fwrite(sym_buf.data(), sym_buf.size() - 1, 1, trace_file);
    LOGD("kallsyms data:\n%s \n\n", hexdump(0x1000,(char*)sym_buf.data(),sym_buf.size()-1).c_str());
}

/**
 * @brief Save module symbols for kernel v6.4+
 *
 * Writes module symbols using the v6.4+ memory type structure.
 * Each module may have multiple memory types (text, data, etc.).
 *
 * @param buf Buffer to write symbols to
 * @param pos Current position in buffer (updated)
 */
void Ftrace::save_proc_kallsyms_mod_v6_4(std::vector<char>& buf, size_t& pos){
    int i = 0, t = 0;
    struct syment *sp;
    size_t module_symbol_count = 0;

    // Iterate through all installed modules
    for (i = 0; i < st->mods_installed; i++) {
        struct load_module *lm = &st->load_modules[i];

        // Iterate through each memory type for this module
        for_each_mod_mem_type(t) {
            if (!lm->symtable[t]){
                continue;
            }

            // Write all symbols for this memory type
            for (sp = lm->symtable[t]; sp <= lm->symend[t]; sp++) {
                // Skip internal module symbols
                if (!strncmp(sp->name, "_MODULE_", strlen("_MODULE_"))){
                    continue;
                }

                // Write symbol (type is always 'm' for module symbols)
                write_to_buf(buf, pos, "%lx %c %s\t[%s]\n",  sp->value, 'm', sp->name, lm->mod_name);
                module_symbol_count++;
            }
        }
    }
}

/**
 * @brief Save module symbols for legacy kernels (< v6.4)
 *
 * Writes module symbols using the legacy symbol table structure.
 * Each module has a single symbol table.
 *
 * @param buf Buffer to write symbols to
 * @param pos Current position in buffer (updated)
 */
void Ftrace::save_proc_kallsyms_mod_legacy(std::vector<char>& buf, size_t& pos){
    int i = 0;
    struct syment *sp;
    // Iterate through all installed modules
    for (i = 0; i < st->mods_installed; i++) {
        struct load_module *lm = &st->load_modules[i];

        // Write all symbols from module symbol table
        for (sp = lm->mod_symtable; sp <= lm->mod_symend; sp++) {
            // Skip internal module symbols
            if (!strncmp(sp->name, "_MODULE_", strlen("_MODULE_")))
                continue;

            // Write symbol (type is always 'm' for module symbols)
            write_to_buf(buf, pos, "%lx %c %s\t[%s]\n",  sp->value, 'm', sp->name, lm->mod_name);
        }
    }
}

/**
 * @brief Write events data section with system categorization
 *
 * Writes all trace event definitions organized by system/subsystem.
 * Events are grouped into systems (e.g., "ftrace", "sched", "irq", "binder")
 * and each system is assigned a unique ID. The ftrace system (ID 1) is
 * written first, followed by all other systems.
 *
 * Format:
 * - ftrace events (system ID 1)
 * - Number of other systems (4 bytes)
 * - For each system:
 *   - System name (null-terminated string)
 *   - Events for that system
 */
void Ftrace::write_events_data(){
    uint system_id = 1;
    uint index = 0;
    std::unordered_map<std::string, uint> system_id_map;

    // Assign system ID 1 to ftrace (built-in events)
    system_id_map["ftrace"] = system_id++;
    LOGD("Assigned system ID %u to 'ftrace'", system_id_map["ftrace"]);

    // Assign system IDs to all other systems
    for (auto &pair : event_maps) {
        std::shared_ptr<TraceEvent> event_ptr = pair.second;
        const std::string &sys = event_ptr->system;

        if (system_id_map.find(sys) == system_id_map.end()) {
            system_id_map[sys] = system_id++;
            LOGD("Assigned system ID %u to '%s'", system_id_map[sys], sys.c_str());
        }

        event_ptr->system_id = system_id_map[sys];
        index++;
    }

    LOGD("Categorized %u events into %zu systems", index, system_id_map.size());

    // Write ftrace events (system ID 1)
    write_events(1);

    // Write other systems
    uint nr_systems = system_id - 2;  // Exclude ftrace system
    fwrite(&nr_systems, 4, 1, trace_file);

    // Write each system's events
    for (uint id = 2; id < nr_systems + 2; id++){
        // Find a representative event for this system to get the system name
        std::shared_ptr<TraceEvent> event_ptr;
        for (auto &pair : event_maps) {
            event_ptr = pair.second;
            if (event_ptr->system_id == id){
                break;
            }
        }

        LOGD("Writing system '%s' (ID %u)", event_ptr->system.c_str(), id);

        // Write system name
        fwrite(event_ptr->system.data(), event_ptr->system.size() + 1, 1, trace_file);

        // Write all events for this system
        write_events(event_ptr->system_id);
    }
}

/**
 * @brief Write all events for a specific system
 *
 * Writes the event definitions for all events belonging to a specific
 * system ID. First writes the count of events, then writes each event's
 * complete format definition.
 *
 * @param system_id System ID to write events for
 */
void Ftrace::write_events(uint system_id){
    // Count events in this system
    int total = 0;
    for (auto &pair : event_maps) {
        std::shared_ptr<TraceEvent> event_ptr = pair.second;
        if (event_ptr->system_id == system_id){
            total++;
        }
    }
    LOGD("System ID %u has %d events", system_id, total);
    // Write event count
    fwrite(&total, 4, 1, trace_file);

    // Write each event definition
    for (auto &pair : event_maps) {
        std::shared_ptr<TraceEvent> event_ptr = pair.second;
        if (event_ptr->system_id != system_id){
            continue;
        }
        write_event(event_ptr);
    }
}

/**
 * @brief Write a single event format definition
 *
 * Writes the complete format definition for a trace event, including:
 * - Event name and ID
 * - Common fields (shared by all events)
 * - Event-specific fields
 * - Print format string
 *
 * The format matches the kernel's /sys/kernel/debug/tracing/events/format
 * files, which allows trace-cmd to properly parse and display events.
 *
 * @param event_ptr Shared pointer to the event to write
 */
void Ftrace::write_event(std::shared_ptr<TraceEvent> event_ptr){
    LOGD("Writing event format for '%s' (ID %u)", event_ptr->name.c_str(), event_ptr->id);

    std::vector<char> event_buf;
    size_t pos = 0;

    // Write event name and ID
    write_to_buf(event_buf, pos, "name: %s\n", event_ptr->name.c_str());
    write_to_buf(event_buf, pos, "ID: %d\n", event_ptr->id);
    write_to_buf(event_buf, pos, "format:\n");

    // Write common fields (present in all trace events)
    for (auto &pair : common_field_maps) {
        std::shared_ptr<trace_field> field_ptr = pair.second;

        // Handle array types specially (e.g., "char[16]" -> "char name[16]")
        size_t position = field_ptr->type.find("[");
        if (position == std::string::npos || field_ptr->type == "__data_loc"){
            // Simple type (no array)
            write_to_buf(event_buf, pos, "\tfield:%s %s;\toffset:%u;\tsize:%u;\tsigned:%d;\n",
                        field_ptr->type.c_str(), field_ptr->name.c_str(),
                        field_ptr->offset, field_ptr->size, !!field_ptr->is_signed);
        }else{
            // Array type - split type and array dimensions
            write_to_buf(event_buf, pos, "\tfield:%.*s %s%s;\toffset:%u;\tsize:%u;\tsigned:%d;\n",
                        (int)(position), field_ptr->type.c_str(), field_ptr->name.c_str(),
                        field_ptr->type.c_str() + position, field_ptr->offset,
                        field_ptr->size, !!field_ptr->is_signed);
        }
    }
    // Separator between common and event-specific fields
    write_to_buf(event_buf, pos, "\n");

    // Write event-specific fields
    for (auto &pair : event_ptr->field_maps) {
        std::shared_ptr<trace_field> field_ptr = pair.second;

        // Handle array types specially
        size_t position = field_ptr->type.find("[");
        if (position == std::string::npos || field_ptr->type == "__data_loc"){
            // Simple type (no array)
            write_to_buf(event_buf, pos, "\tfield:%s %s;\toffset:%u;\tsize:%u;\tsigned:%d;\n",
                        field_ptr->type.c_str(), field_ptr->name.c_str(),
                        field_ptr->offset, field_ptr->size, !!field_ptr->is_signed);
        }else{
            // Array type - split type and array dimensions
            write_to_buf(event_buf, pos, "\tfield:%.*s %s%s;\toffset:%u;\tsize:%u;\tsigned:%d;\n",
                        (int)(position), field_ptr->type.c_str(), field_ptr->name.c_str(),
                        field_ptr->type.c_str() + position, field_ptr->offset,
                        field_ptr->size, !!field_ptr->is_signed);
        }
    }
    // Write print format string
    write_to_buf(event_buf, pos, "\nprint fmt: %s\n", event_ptr->org_print_fmt.c_str());

    // Write size followed by event format data
    fwrite(&pos, 8, 1, trace_file);
    fwrite(event_buf.data(), event_buf.size() - 1, 1, trace_file);
    LOGD("\n%s \n\n", hexdump(0x1000,(char*)event_buf.data(),event_buf.size()-1).c_str());
}

/**
 * @brief Write header data section
 *
 * Writes the header data section which describes the format of:
 * 1. header_page: The buffer_data_page structure format
 * 2. header_event: The ring_buffer_event structure format
 *
 * These headers are essential for trace-cmd to understand how to parse
 * the raw ring buffer data.
 */
void Ftrace::write_header_data(){
    // Write header_page format
    fwrite("header_page", 12, 1, trace_file);

    std::vector<char> head_buf;
    size_t pos = 0;

    // Describe buffer_data_page structure fields
    write_to_buf(head_buf, pos, "\tfield: u64 timestamp;\toffset:0;\tsize:8;\tsigned:0;\n");
    write_to_buf(head_buf, pos, "\tfield: local_t commit;\toffset:8;\tsize:%zu;\tsigned:1;\n", sizeof(long));
    write_to_buf(head_buf, pos, "\tfield: int overwrite;\toffset:8;\tsize:%zu;\tsigned:1;\n", sizeof(long));
    write_to_buf(head_buf, pos, "\tfield: char data;\toffset:%zu;\tsize:%zu;\tsigned:1;\n",
                8 + sizeof(long), PAGESIZE() - 8 - sizeof(long));

    fwrite(&pos, 8, 1, trace_file);
    fwrite(head_buf.data(), head_buf.size()-1, 1, trace_file);
    LOGD("buffer_data_page:\n%s \n\n", hexdump(0x1000,(char*)head_buf.data(),head_buf.size()-1).c_str());

    // Write header_event format
    fwrite("header_event", 13, 1, trace_file);

    std::vector<char> event_buf;
    pos = 0;
    // Describe ring_buffer_event structure format
    write_to_buf(event_buf, pos, "# compressed entry header\n"
            "\ttype_len    :    5 bits\n"
            "\ttime_delta  :   27 bits\n"
            "\tarray       :   32 bits\n"
            "\n"
            "\tpadding     : type == 29\n"
            "\ttime_extend : type == 30\n"
            "\tdata max type_len  == 28\n");

    fwrite(&pos, 8, 1, trace_file);
    fwrite(event_buf.data(), event_buf.size()-1, 1, trace_file);
    LOGD("header_event:\n%s \n\n", hexdump(0x1000,(char*)event_buf.data(),event_buf.size()-1).c_str());
}

/**
 * @brief Write initialization data section
 *
 * Writes the file header with magic number, version, and system information.
 * This is the first data written to the trace file and identifies it as a
 * trace-cmd format file.
 *
 * Format:
 * - Magic number: "\027\010\104tracing" (10 bytes)
 * - Version string: "6\0" (2 bytes)
 * - Endianness: 0=little endian, 1=big endian (1 byte)
 * - Size of long: sizeof(long) (1 byte)
 * - Page size: page_size (4 bytes)
 */
void Ftrace::write_init_data(){
    // Write magic number
    fwrite("\027\010\104tracing", 10, 1, trace_file);

    // Write version string
    fwrite(TRACE_CMD_FILE_VERSION_STRING, strlen(TRACE_CMD_FILE_VERSION_STRING) + 1, 1, trace_file);

    // Write endianness
    // Note: Crash utility ensures core file endian matches host endian
    int value = 1;
    if (is_bigendian()){
        value = 1;
    }else{
        value = 0;
    }
    fwrite(&value, 1, 1, trace_file);

    // Write size of long (architecture dependent)
    value = sizeof(long);
    fwrite(&value, 1, 1, trace_file);

    // Write page size
    value = page_size;
    fwrite(&value, 4, 1, trace_file);
    LOGD("Page size: %d bytes", value);
}

/**
 * @brief Dump trace file contents as hexadecimal
 *
 * Reads the entire trace file and displays its contents as a hexadecimal
 * dump. This is useful for debugging the trace file format.
 *
 * Note: This function is primarily for debugging purposes and may consume
 * significant memory for large trace files.
 */
void Ftrace::dump_file(){
    LOGD("Dumping trace file: %s", trace_path.c_str());

    // Open trace file for reading
    FILE* tmp_file = fopen(trace_path.c_str(), "rb");
    if (!tmp_file) {
        LOGE("Failed to open trace file: %s", trace_path.c_str());
        return;
    }

    // Get file size
    fseek(tmp_file, 0, SEEK_END);
    size_t fileSize = ftell(tmp_file);
    rewind(tmp_file);

    LOGD("Trace file size: %zu bytes", fileSize);

    // Allocate buffer for entire file
    char* buffer = static_cast<char*>(malloc(fileSize));
    if (buffer == nullptr) {
        LOGE("Failed to allocate %zu bytes for file buffer", fileSize);
        fclose(tmp_file);
        return;
    }

    // Read entire file
    size_t bytesRead = fread(buffer, 1, fileSize, tmp_file);
    fclose(tmp_file);

    if (bytesRead != fileSize) {
        LOGE("File read error: expected %zu bytes, read %zu bytes", fileSize, bytesRead);
        free(buffer);
        return;
    }

    LOGD("Successfully read %zu bytes, generating hexdump", bytesRead);

    // Display hexdump
    PRINT("%s \n\n", hexdump(0x1000,(char*)buffer,fileSize).c_str());

    free(buffer);
    LOGD("Hexdump complete");
}

/**
 * @brief Write formatted string to a dynamically resizing buffer
 *
 * This is a helper function that writes a printf-style formatted string
 * to a vector buffer. If the buffer is too small, it automatically resizes
 * to accommodate the formatted string. This is used throughout the trace
 * file writing process to build up sections of data before writing to disk.
 *
 * @param buffer Vector buffer to write to (automatically resized if needed)
 * @param pos Current position in buffer (updated after write)
 * @param fmt Printf-style format string
 * @param ... Variable arguments for format string
 */
void Ftrace::write_to_buf(std::vector<char>& buffer, size_t& pos, const char* fmt, ...) {
    if (fmt == nullptr) {
        LOGD("write_to_buf called with null format string");
        return;
    }

    va_list args;
    va_start(args, fmt);

    // Try to print into the current buffer
    int n = vsnprintf(buffer.data() + pos, buffer.size() - pos, fmt, args);
    va_end(args);

    if (n > -1 && static_cast<size_t>(n) < buffer.size() - pos) {
        // Success: formatted string fits in current buffer
        pos += n;
    } else {
        // Buffer too small, need to resize
        if (n > -1) {
            // We know exactly how many bytes we need
            size_t newSize = pos + n + 1;
            buffer.resize(newSize);
        } else {
            // vsnprintf error (e.g., invalid format string)
            LOGE("vsnprintf error with format string: %s", fmt);
            return;
        }

        // Retry printing into the resized buffer
        va_start(args, fmt);
        n = vsnprintf(buffer.data() + pos, buffer.size() - pos, fmt, args);
        va_end(args);

        if (n > -1) {
            pos += n;
        } else {
            LOGE("vsnprintf error after resizing buffer");
        }
    }
}

/**
 * @brief Display trace data using trace-cmd tool
 *
 * This function uses the external trace-cmd tool to display the trace data
 * in a human-readable format. It first checks if trace-cmd is available,
 * then ensures the trace file exists (creating it if necessary), and finally
 * runs "trace-cmd report" to display the formatted trace output.
 *
 * The trace-cmd tool can be specified via the TRACE_CMD environment variable
 * if it's installed in a non-standard location.
 */
void Ftrace::ftrace_show() {
    LOGD("Displaying trace data via trace-cmd");

    char buf[4096] = {0};
    std::string traceCmd = "trace-cmd";

    // Check for custom trace-cmd path in environment
    const char* envTraceCmd = std::getenv("TRACE_CMD");
    if (envTraceCmd) {
        traceCmd = envTraceCmd;
        LOGD("Using trace-cmd from environment: %s", envTraceCmd);
    } else {
        LOGD("Using default trace-cmd command");
    }

    // Verify trace-cmd is available
    LOGD("Checking trace-cmd availability");
    FILE* file = popen(traceCmd.c_str(), "r");
    if (!file) {
        LOGE("Failed to execute trace-cmd");
        return;
    }

    size_t ret = fread(buf, 1, sizeof(buf) - 1, file);
    buf[ret] = '\0';
    pclose(file);

    // Verify trace-cmd output
    if (!strstr(buf, "trace-cmd version")) {
        if (envTraceCmd) {
            LOGE("Invalid TRACE_CMD environment variable: %s", envTraceCmd);
        } else {
            LOGE("\"Ftrace -S\" requires trace-cmd.\n Please set the environment TRACE_CMD if you installed it in a special path.\n");
        }
        return;
    }

    LOGD("trace-cmd is available");

    // Get current working directory
    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd))) {
        LOGE("Failed to get current working directory");
        return;
    }

    std::string tracePath = std::string(cwd) + "/ftrace.data";
    LOGD("Trace file path: %s", tracePath.c_str());

    // Create trace file if it doesn't exist
    if (access(tracePath.c_str(), F_OK) != 0) {
        LOGD("Trace file doesn't exist, creating it");
        ftrace_dump();
    } else {
        LOGD("Trace file already exists");
    }

    // Run trace-cmd report to display trace data
    LOGD("Running trace-cmd report");
    std::ostringstream cmd;
    cmd << traceCmd << " report " << tracePath;

    file = popen(cmd.str().c_str(), "r");
    if (!file) {
        LOGE("Failed to run trace-cmd report");
        return;
    }

    // Stream trace-cmd output to user
    size_t total_bytes = 0;
    while ((ret = fread(buf, 1, sizeof(buf) - 1, file)) > 0) {
        buf[ret] = '\0';
        PRINT("%s", buf);
        total_bytes += ret;
    }
    pclose(file);

    LOGD("trace-cmd report complete, displayed %zu bytes", total_bytes);
}

/**
 * @brief Dump trace data to file in trace-cmd format
 *
 * This function exports all trace data from the kernel's ftrace ring buffers
 * to a file in trace-cmd format. The file can be analyzed with the trace-cmd
 * tool or transferred to another system for analysis.
 *
 * The output file is created in the current working directory as "ftrace.data".
 * If the file already exists, it is deleted and recreated.
 */
void Ftrace::ftrace_dump() {
    LOGD("Starting ftrace dump to file");

    // Get current working directory
    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd))) {
        LOGE("Failed to get current working directory");
        return;
    }

    // Build trace file path
    trace_path = std::string(cwd) + "/ftrace.data";
    LOGD("Trace file path: %s", trace_path.c_str());

    // Remove existing file if present
    if (access(trace_path.c_str(), F_OK) == 0) {
        LOGD("Removing existing trace file");
        if (unlink(trace_path.c_str()) != 0) {
            LOGE("Failed to remove existing trace file: %s", trace_path.c_str());
            return;
        }
    }

    // Open trace file for writing
    trace_file = fopen(trace_path.c_str(), "wb");
    if (!trace_file) {
        LOGE("Failed to open trace file for writing: %s", trace_path.c_str());
        return;
    }

    LOGD("Trace file opened successfully, writing trace data");

    // Write all trace data to file
    write_trace_data();

    // Close file
    fclose(trace_file);
    trace_file = nullptr;

    PRINT("Saved to %s\n", trace_path.c_str());
}

#pragma GCC diagnostic pop
