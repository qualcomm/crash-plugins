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

#include "timer.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(TimerParser)
#endif

TimerParser::TimerParser() {
    timer_42 = (THIS_KERNEL_VERSION >= LINUX(4, 2, 0));
    timer_69 = (THIS_KERNEL_VERSION >= LINUX(6, 9, 0));
    timer_has_data = (THIS_KERNEL_VERSION < LINUX(4, 15, 0));
    use_timer_base = (THIS_KERNEL_VERSION >= LINUX(4, 9, 0));

    std::string hz_cfg = get_config_val("CONFIG_HZ");
    if (!hz_cfg.empty() && hz_cfg != "n") {
        try {
            if (!std::all_of(hz_cfg.begin(), hz_cfg.end(), [](char ch) {
                    return ch >= '0' && ch <= '9';
                })) {
                throw std::invalid_argument("invalid CONFIG_HZ");
            }
            size_t pos = 0;
            unsigned long parsed_hz = std::stoul(hz_cfg, &pos);
            if (pos == hz_cfg.size() && parsed_hz > 0) {
                hz = static_cast<double>(parsed_hz);
            }
        } catch (const std::exception&) {
            // Keep the default HZ when the configuration cannot be parsed.
        }
    }

    std::string no_hz_cfg = get_config_val("CONFIG_NO_HZ_COMMON");
    no_hz_common = (no_hz_cfg == "y");
    setup_layout_by_kernel_version();
    setup_offsets();
}

void TimerParser::setup_layout_by_kernel_version() {
    base_types.clear();
    vectors.clear();

    // base_types is the same regardless of timer_base vs tvec_base
    base_types = {""};
    if (no_hz_common) {
        if (timer_69)       base_types = {"BASE_LOCAL", "BASE_GLOBAL", "BASE_DEF"};
        else if (timer_42)  base_types = {"BASE_STD",   "BASE_DEF"};
    }

    if (use_timer_base) {
        tvec_bases_symbol        = "timer_bases";
        global_deferrable_symbol = "timer_base_deferrable";
        int depth = (hz > 100.0) ? 9 : 8;
        vectors.push_back({"vectors", depth * 64, -1, true});
    } else {
        tvec_bases_symbol        = "tvec_bases";
        global_deferrable_symbol = "tvec_base_deferrable";
        bool use_hlist = timer_42;
        vectors.push_back({"tv1", 256, -1, use_hlist});
        vectors.push_back({"tv2",  64, -1, use_hlist});
        vectors.push_back({"tv3",  64, -1, use_hlist});
        vectors.push_back({"tv4",  64, -1, use_hlist});
        vectors.push_back({"tv5",  64, -1, use_hlist});
    }
}

void TimerParser::init_offset(void) {}

void TimerParser::setup_offsets() {
    base_struct_size = use_timer_base ? struct_size(timer_base) : struct_size(tvec_base);
    list_head_size = struct_size(list_head);
    hlist_head_size = struct_size(hlist_head);

    if (use_timer_base) {
        timer_jiffies_offset = field_offset(timer_base, clk);
        next_timer_offset = field_offset(timer_base, next_expiry);
        active_timers_offset = field_offset(timer_base, active_timers);
        for (auto &vec : vectors) {
            if (vec.name == "vectors") {
                vec.offset = field_offset(timer_base, vectors);
            }
        }
    } else {
        timer_jiffies_offset = field_offset(tvec_base, timer_jiffies);
        next_timer_offset = field_offset(tvec_base, next_timer);
        active_timers_offset = field_offset(tvec_base, active_timers);
        for (auto &vec : vectors) {
            if (vec.name == "tv1") vec.offset = field_offset(tvec_base, tv1);
            else if (vec.name == "tv2") vec.offset = field_offset(tvec_base, tv2);
            else if (vec.name == "tv3") vec.offset = field_offset(tvec_base, tv3);
            else if (vec.name == "tv4") vec.offset = field_offset(tvec_base, tv4);
            else if (vec.name == "tv5") vec.offset = field_offset(tvec_base, tv5);
        }
    }

    timer_entry_offset = field_offset(timer_list, entry);
    if (timer_entry_offset < 0) {
        timer_entry_offset = field_offset(timer_list, list);
    }
    if (timer_entry_offset < 0) {
        timer_entry_offset = 0;
    }

    timer_function_offset = field_offset(timer_list, function);
    timer_expires_offset = field_offset(timer_list, expires);
    timer_data_offset = field_offset(timer_list, data);
    timer_base_offset = field_offset(timer_list, base);

    delayed_work_timer_offset = field_offset(delayed_work, timer);
    work_struct_func_offset = field_offset(work_struct, func);

    hrtimer_cpu_base_clock_base_offset = field_offset(hrtimer_cpu_base, clock_base);
    hrtimer_clock_base_active_offset = field_offset(hrtimer_clock_base, active);
    if (hrtimer_clock_base_active_offset < 0) {
        hrtimer_clock_base_active_offset = field_offset(hrtimer_clock_base, first);
    }
    hrtimer_clock_base_get_time_offset = field_offset(hrtimer_clock_base, get_time);
    hrtimer_clock_base_clockid_offset  = field_offset(hrtimer_clock_base, clockid);

    hrtimer_clock_base_size = struct_size(hrtimer_clock_base);
    int clock_base_array_size = field_size(hrtimer_cpu_base, clock_base);
    if (hrtimer_clock_base_size > 0 && clock_base_array_size > 0) {
        hrtimer_max_clock_bases = clock_base_array_size / hrtimer_clock_base_size;
    }
    if (hrtimer_max_clock_bases <= 0) {
        hrtimer_max_clock_bases = 4;
    }

    hrtimer_node_offset = field_offset(hrtimer, node);
    hrtimer_function_offset = field_offset(hrtimer, function);
    hrtimer_softexpires_offset = field_offset(hrtimer, _softexpires);
    hrtimer_sleeper_timer_offset = field_offset(hrtimer_sleeper, timer);
    hrtimer_sleeper_task_offset = field_offset(hrtimer_sleeper, task);
    task_struct_comm_offset = field_offset(task_struct, comm);
    timerqueue_node_expires_offset = field_offset(timerqueue_node, expires);
    timerqueue_node_node_offset = field_offset(timerqueue_node, node);
    if (timerqueue_node_node_offset < 0) {
        timerqueue_node_node_offset = 0;
    }

    // timerfd_ctx offsets
    timerfd_ctx_t_offset   = field_offset(timerfd_ctx, t);
    timerfd_ctx_wqh_offset = field_offset(timerfd_ctx, wqh);
    wait_queue_head_head_offset      = field_offset(wait_queue_head, head);
    wait_queue_entry_private_offset  = field_offset(wait_queue_entry, private);
    wait_queue_entry_entry_offset    = field_offset(wait_queue_entry, entry);
}

void TimerParser::init_command(void) {
    cmd_name = "timerlist";
    help_str_list = {
        "timerlist",
        "display linux timer wheel and hrtimer information",
        "[-a] [-l] [-r] [-c cpu] [-t jiffies]\n"
        "  This command displays timer diagnostics.\n"
        "\n"
        "    -a        display both timer list and hrtimer information\n"
        "    -l        display timer list only\n"
        "    -r        display hrtimer list only\n"
        "    -c cpu    restrict output to the specified CPU index (both timer wheel and hrtimer)\n"
        "    -t val    convert absolute jiffies value to seconds since boot using CONFIG_HZ\n",
        "\n",
        "EXAMPLES",
        "  Display timer wheel and hrtimers for CPU 3 only:",
        "    %s> timerlist -c 3",
        "\n",
        "  Display timer wheel only:",
        "    %s> timerlist -l",
        "\n",
        "  Display hrtimer only:",
        "    %s> timerlist -r",
        "\n",
        "  Convert 12345 jiffies to seconds:",
        "    %s> timerlist -t 12345",
        "\n",
    };
}

void TimerParser::cmd_main(void) {
    if (argcnt < 2) {
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }

    bool show_timer_list = false;
    bool show_hrtimer = false;
    bool convert_jiffies = false;
    ulonglong jiffies_val = 0;
    target_cpu_filter = -1;

    int c;
    argerrs = 0;
    optind = 0;
    opterr = 0;
    optarg = nullptr;

    while ((c = getopt(argcnt, args, "alrc:t:")) != EOF) {
        switch (c) {
            case 'a':
                show_timer_list = true;
                show_hrtimer = true;
                break;
            case 'l':
                show_timer_list = true;
                break;
            case 'r':
                show_hrtimer = true;
                break;
            case 'c': {
                int val = 0;
                bool valid = true;
                try {
                    size_t pos = 0;
                    val = std::stoi(optarg, &pos);
                    if (pos != std::string(optarg).size()) {
                        valid = false;
                    }
                } catch (const std::exception&) {
                    valid = false;
                }
                if (!valid || !is_valid_cpu(val)) {
                    PRINT("timerlist: invalid cpu: %s\n", optarg);
                    argerrs++;
                } else {
                    target_cpu_filter = val;
                    show_timer_list = true;
                    show_hrtimer = true;
                }
                break;
            }
            case 't':
                try {
                    size_t pos = 0;
                    if (optarg[0] == '-') {
                        throw std::invalid_argument("negative jiffies");
                    }
                    jiffies_val = std::stoull(optarg, &pos);
                    if (pos != std::string(optarg).size()) {
                        throw std::invalid_argument("trailing characters");
                    }
                    convert_jiffies = true;
                } catch (const std::exception&) {
                    PRINT("timerlist: invalid jiffies value: %s\n", optarg);
                    argerrs++;
                }
                break;
            default:
                argerrs++;
                break;
        }
    }

    if (argerrs) {
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }

    if (!show_timer_list && !show_hrtimer && !convert_jiffies) {
        show_timer_list = true;
        show_hrtimer = true;
    }

    if (convert_jiffies) {
        print_jiffies_conversion(jiffies_val);
    }

    if (show_timer_list) {
        dump_timer_list();
    }

    if (show_hrtimer) {
        dump_hrtimer();
    }
}

double TimerParser::jiffies_to_seconds(ulong jiffies_val) const {
    if (hz <= 0.0) {
        return 0.0;
    }
    double base = static_cast<double>(0x100000000ULL) - 300.0 * hz;
    return (static_cast<double>(jiffies_val) - base) / hz;
}

void TimerParser::print_jiffies_conversion(ulonglong jiffies_val) const {
    if (hz <= 0.0) {
        PRINT("%llu jiffies / HZ %.0f = 0.000000 seconds\n", jiffies_val, hz);
        return;
    }
    double initial_jiffies = static_cast<double>(0x100000000ULL) - 300.0 * hz;
    double seconds = (static_cast<double>(jiffies_val) - initial_jiffies) / hz;
    PRINT("%.6fs\n",seconds);
}

// Format a ktime_t nanosecond value as "RAW_NS (Xd Xh Xm Xs)" or
// "RAW_NS (X.XXXs)" for values under one minute.
// KTIME_MAX (INT64_MAX) is shown as "KTIME_MAX".
static std::string format_ktime_ns(ulonglong ns_val) {
    std::ostringstream oss;
    oss << ns_val << " (";

    // KTIME_MAX sentinel
    if (ns_val >= static_cast<ulonglong>(INT64_MAX)) {
        oss << "KTIME_MAX)";
        return oss.str();
    }

    double sec_total = static_cast<double>(ns_val) / 1e9;

    if (sec_total < 60.0) {
        oss << std::fixed << std::setprecision(6) << sec_total << "s)";
        return oss.str();
    }

    uint64_t total_sec = static_cast<uint64_t>(sec_total);
    uint64_t days    = total_sec / 86400;
    uint64_t hours   = (total_sec % 86400) / 3600;
    uint64_t minutes = (total_sec % 3600)  / 60;
    uint64_t seconds = total_sec % 60;

    if (days > 0)
        oss << days << "d ";
    if (days > 0 || hours > 0)
        oss << hours << "h ";
    if (days > 0 || hours > 0 || minutes > 0)
        oss << minutes << "m ";
    oss << seconds << "s)";
    return oss.str();
}

std::string TimerParser::base_type_name(size_t idx) const {
    if (idx < base_types.size()) {
        return base_types[idx];
    }
    return "BASE_" + std::to_string(idx);
}

static std::string symbol_base_name(const std::string& symbol) {
    std::string base = symbol.substr(0, symbol.find('['));
    base = base.substr(0, base.find('+'));
    return base;
}

bool TimerParser::is_delayed_work_timer_fn(const std::string& symbol) const {
    return symbol_base_name(symbol) == "delayed_work_timer_fn";
}

bool TimerParser::is_process_timeout_timer_fn(const std::string& symbol) const {
    return symbol_base_name(symbol) == "process_timeout";
}

std::string TimerParser::resolve_process_timeout_comm(ulong timer_addr) {
    int process_timer_timer_offset = field_offset(process_timer, timer);
    int process_timer_task_offset = field_offset(process_timer, task);

    if (process_timer_timer_offset < 0 || process_timer_task_offset < 0 ||
        task_struct_comm_offset < 0) {
        return "";
    }

    ulong timeout_addr = timer_addr - static_cast<ulong>(process_timer_timer_offset);
    if (!is_kvaddr(timeout_addr)) {
        return "";
    }

    ulong task_addr = read_pointer(timeout_addr + process_timer_task_offset,
                                   "process_timer.task");
    if (!is_kvaddr(task_addr)) {
        return "";
    }

    return read_cstring(task_addr + task_struct_comm_offset, 16, "task_struct.comm");
}

TimerEntryInfo TimerParser::parse_timer_entry(ulong timer_addr, size_t index, ulong expected_base) {
    TimerEntryInfo entry;
    entry.addr = timer_addr;
    entry.index = index;

    const ulonglong dead_mask = 0xFFFF000000000000ULL;
    const ulonglong dead_value = 0xDEAD000000000000ULL;
    if ((static_cast<ulonglong>(timer_addr) & dead_mask) == dead_value) {
        entry.valid = false;
        entry.function = "NA";
        entry.data_or_work = "NA";
        entry.remarks = "LIST_POISON(DELETED)";
        return entry;
    }

    if (!is_kvaddr(timer_addr)) {
        entry.valid = false;
        entry.function = "NA";
        entry.data_or_work = "NA";
        entry.remarks = "INVALID_TIMER_ADDRESS";
        return entry;
    }

    ulong function_addr = 0;
    if (timer_function_offset >= 0) {
        function_addr = read_pointer(timer_addr + timer_function_offset, "timer_list.function");
    }

    if (timer_expires_offset >= 0) {
        entry.expires = read_ulong(timer_addr + timer_expires_offset, "timer_list.expires");
        entry.expires_sec = jiffies_to_seconds(entry.expires);
    }

    entry.function = to_symbol(function_addr);

    if (timer_has_data && timer_data_offset >= 0) {
        ulong data = read_ulong(timer_addr + timer_data_offset, "timer_list.data");
        std::ostringstream data_ss;
        data_ss << "0x" << std::hex << data;
        entry.data_or_work = data_ss.str();
    }

    if (is_delayed_work_timer_fn(entry.function) &&
        delayed_work_timer_offset >= 0 && work_struct_func_offset >= 0 && timer_addr >= static_cast<ulong>(delayed_work_timer_offset)) {
        ulong work_addr = timer_addr - delayed_work_timer_offset;
        ulong work_func = read_pointer(work_addr + work_struct_func_offset, "work_struct.func");
        std::string work_symbol = to_symbol(work_func);
        if (timer_has_data) {
            if (entry.data_or_work.empty()) {
                entry.data_or_work = work_symbol;
            } else {
                entry.data_or_work += " / " + work_symbol;
            }
        } else {
            entry.data_or_work = work_symbol;
        }
    }

    if (is_process_timeout_timer_fn(entry.function)) {
        entry.wakeup_comm = resolve_process_timeout_comm(timer_addr);
    }

    if (!timer_42 && timer_base_offset >= 0) {
        ulong timer_base = read_pointer(timer_addr + timer_base_offset, "timer_list.base") & ~0x3UL;
        if (is_kvaddr(expected_base) && timer_base != expected_base) {
            entry.remarks = "Timer Base Mismatch detected";
        }
    }

    if (!timer_has_data && entry.data_or_work.empty()) {
        entry.data_or_work = "-";
    }

    return entry;
}

std::vector<TimerEntryInfo> TimerParser::collect_vector_timers(const TimerVectorConfig& vec, ulong base) {
    std::vector<TimerEntryInfo> entries;
    if (vec.offset < 0 || vec.size <= 0) {
        return entries;
    }

    int bucket_size = vec.use_hlist ? hlist_head_size : list_head_size;
    if (bucket_size <= 0) {
        return entries;
    }

    for (int i = 0; i < vec.size; ++i) {
        ulong bucket = base + vec.offset + static_cast<ulong>(i) * bucket_size;
        std::vector<ulong> timer_nodes = vec.use_hlist
            ? for_each_hlist(bucket, timer_entry_offset)
            : for_each_list(bucket, timer_entry_offset);

        for (const auto& timer_addr : timer_nodes) {
            entries.push_back(parse_timer_entry(timer_addr, i, base));
        }
    }

    return entries;
}

void TimerParser::print_vector_timers(const TimerVectorConfig& vec,
                                       const std::vector<TimerEntryInfo>& entries) {
    if (entries.empty()) {
        PRINT("  + %s  (no timers)\n", vec.name.c_str());
        return;
    }

    PRINT("  + %s  (%zu timers)\n", vec.name.c_str(), entries.size());

    const std::vector<std::string> hdrs = {
        "INDEX", "timer_list", "EXPIRES",
        "FUNCTION",
        "WAKEUP_COMM",
        timer_has_data ? "DATA/WORK" : "WORK",
    };
    const std::vector<bool> ra = {true, false, true, false, false, false};

    std::vector<std::vector<std::string>> tbl;
    tbl.reserve(entries.size());
    for (const auto& e : entries) {
        std::string exp_str, func_str, wakeup_comm, work_str;

        if (e.valid) {
            std::ostringstream oss;
            oss << e.expires << "(" << std::fixed << std::setprecision(3)
                << e.expires_sec << "s)";
            exp_str  = oss.str();
            // Append any diagnostic remarks to the function name
            func_str = e.remarks.empty()
                       ? e.function
                       : e.function + "  [" + e.remarks + "]";
            wakeup_comm = e.wakeup_comm.empty() ? "-" : e.wakeup_comm;
            work_str = e.data_or_work.empty() ? "-" : e.data_or_work;
        } else {
            exp_str  = "NA";
            func_str = e.remarks.empty() ? "NA" : e.remarks;
            wakeup_comm = "NA";
            work_str = "NA";
        }

        char addr_buf[20];
        snprintf(addr_buf, sizeof(addr_buf), "%#lx", e.addr);
        tbl.push_back({std::to_string(e.index), addr_buf,
                       exp_str, func_str, wakeup_comm, work_str});
    }

    std::string table = build_box_table("", hdrs, tbl, ra);
    std::istringstream iss(table);
    std::string line;
    while (std::getline(iss, line))
        PRINT("    %s\n", line.c_str());
}

void TimerParser::dump_timer_base(ulong base, const std::string& title_prefix, int cpu) {
    if (!is_kvaddr(base)) {
        return;
    }

    ulong timer_jiffies = 0, next_timer = 0, active_timers = 0;
    bool  has_active = (active_timers_offset >= 0);

    if (timer_jiffies_offset >= 0)
        timer_jiffies = read_ulong(base + timer_jiffies_offset, "timer_jiffies");
    if (next_timer_offset >= 0)
        next_timer    = read_ulong(base + next_timer_offset, "next_timer");
    if (has_active)
        active_timers = read_ulong(base + active_timers_offset, "active_timers");

    // ── single-line header, matching the hrtimer CLOCK: line style ──────────
    if (cpu >= 0) PRINT("CPU %d  ", cpu);
    PRINT("timer_base: %#lx", base);
    if (!title_prefix.empty()) PRINT("  [%s]", title_prefix.c_str());
    PRINT("  jiffies: %lu(%.3fs)  next: %lu(%.3fs)  active: ",
          timer_jiffies, jiffies_to_seconds(timer_jiffies),
          next_timer,    jiffies_to_seconds(next_timer));
    if (has_active) PRINT("%lu\n", active_timers);
    else            PRINT("NA\n");

    for (const auto& vec : vectors) {
        std::vector<TimerEntryInfo> entries = collect_vector_timers(vec, base);
        print_vector_timers(vec, entries);
    }
}

void TimerParser::dump_global_deferrable_base() {
    if (!csymbol_exists(global_deferrable_symbol)) {
        return;
    }

    ulong base = csymbol_value(global_deferrable_symbol);
    if (!is_kvaddr(base)) {
        return;
    }

    dump_timer_base(base, "DEFERRABLE_BASE");
}

void TimerParser::dump_percpu_timer_bases() {
    if (!csymbol_exists(tvec_bases_symbol)) {
        PRINT("%s doesn't exist in this kernel\n", tvec_bases_symbol.c_str());
        return;
    }

    ulong tvec_bases_addr = csymbol_value(tvec_bases_symbol);
    if (!is_kvaddr(tvec_bases_addr) || base_struct_size <= 0) {
        PRINT("%s address invalid: %#lx\n", tvec_bases_symbol.c_str(), tvec_bases_addr);
        return;
    }

    size_t base_count = 1;
    if (no_hz_common && timer_69) {
        base_count = 3;
    } else if (no_hz_common && timer_42) {
        base_count = 2;
    }

    // Collect per-CPU slot addresses for each base index up front.
    std::vector<std::vector<ulong>> slots_per_base(base_count);
    size_t cpu_count = 0;
    for (size_t i = 0; i < base_count; ++i) {
        ulong per_cpu_slot_base = tvec_bases_addr + static_cast<ulong>(i) * base_struct_size;
        slots_per_base[i] = for_each_percpu(per_cpu_slot_base);
        cpu_count = std::max(cpu_count, slots_per_base[i].size());
    }

    // Iterate CPU-first so all bases of one CPU appear together.
    for (size_t cpu = 0; cpu < cpu_count; ++cpu) {
        if (target_cpu_filter >= 0 && static_cast<int>(cpu) != target_cpu_filter)
            continue;
        for (size_t i = 0; i < base_count; ++i) {
            const auto& slots = slots_per_base[i];
            if (cpu >= slots.size()) continue;
            ulong per_cpu_slot = slots[cpu];
            if (!is_kvaddr(per_cpu_slot)) continue;

            ulong base = timer_42 ? per_cpu_slot : read_pointer(per_cpu_slot, "tvec_base_ptr");
            if (!is_kvaddr(base)) continue;

            dump_timer_base(base, base_type_name(i), static_cast<int>(cpu));
        }
    }
}

void TimerParser::dump_tick_do_timer_cpu() {
    if (!csymbol_exists("tick_do_timer_cpu")) {
        return;
    }

    int tick_cpu = read_int(csymbol_value("tick_do_timer_cpu"), "tick_do_timer_cpu");
    PRINT("tick_do_timer_cpu: %d\n", tick_cpu);
}

void TimerParser::dump_timer_list() {
    dump_global_deferrable_base();
    dump_percpu_timer_bases();
    dump_tick_do_timer_cpu();
}

// POSIX clockid → name table (shared by dump_hrtimer and resolve_timerfd_waiters)
static const std::pair<int, const char*> kClockIdNames[] = {
    {0,  "CLOCK_REALTIME"},     {1,  "CLOCK_MONOTONIC"},
    {4,  "CLOCK_MONOTONIC_RAW"},{7,  "CLOCK_BOOTTIME"},
    {11, "CLOCK_TAI"},
};

static const char* clockid_to_name(int clockid) {
    for (const auto& p : kClockIdNames)
        if (p.first == clockid) return p.second;
    return nullptr;
}

// For a timerfd_tmrproc hrtimer, walk timerfd_ctx.wqh to collect all
// tasks that are blocked waiting for the timer to fire.
// Returns a comma-separated "pid(comm)" string, or "" if unavailable.
std::string TimerParser::resolve_timerfd_waiters(ulong hrtimer_addr) {
    if (timerfd_ctx_t_offset < 0 || timerfd_ctx_wqh_offset < 0 ||
        wait_queue_entry_private_offset < 0) {
        return "";
    }

    // container_of(hrtimer, timerfd_ctx, t.tmr)
    // t is a union; tmr is its first member, so offset(t.tmr) == offset(t)
    ulong ctx_addr  = hrtimer_addr - static_cast<ulong>(timerfd_ctx_t_offset);
    ulong wqh_addr  = ctx_addr + timerfd_ctx_wqh_offset;

    std::string waiters;
    for (const auto& entry_addr : for_each_waiter(wqh_addr)) {
        ulong task_addr = read_pointer(entry_addr + wait_queue_entry_private_offset,"private");
        if (!is_kvaddr(task_addr)) continue;
        struct task_context *tc = task_to_context(task_addr);
        if (!tc) continue;

        if (!waiters.empty()) waiters += ",";
        waiters += std::to_string(tc->pid) + "(" + std::string(tc->comm) + ")";
    }

    if (waiters.empty()) waiters = "no waiters";
    return waiters;
}

std::vector<HrtimerEntryInfo>
TimerParser::collect_hrtimers_for_clock_base(ulong clock_base_addr) {
    std::vector<HrtimerEntryInfo> entries;

    if (hrtimer_clock_base_active_offset < 0 || hrtimer_node_offset < 0 ||
        hrtimer_function_offset < 0 || hrtimer_softexpires_offset < 0 ||
        timerqueue_node_expires_offset < 0) {
        return entries;
    }

    int rb_node_offset = hrtimer_node_offset + timerqueue_node_node_offset;
    ulong active_addr  = clock_base_addr + hrtimer_clock_base_active_offset;
    ulong rb_node = read_pointer(active_addr, "active/first");
    if (!is_kvaddr(rb_node)) {
        return entries;
    }

    for (const auto& hrtimer_addr : for_each_rbtree(rb_node, rb_node_offset)) {
        HrtimerEntryInfo e;
        e.addr          = hrtimer_addr;
        e.function      = read_pointer(hrtimer_addr + hrtimer_function_offset, "function");
        e.function_name = to_symbol(e.function);
        e.softexpires   = read_ulonglong(hrtimer_addr + hrtimer_softexpires_offset,"_softexpires");
        e.expires       = read_ulonglong(hrtimer_addr + hrtimer_node_offset +
                                          timerqueue_node_expires_offset,"expires");

        if (hrtimer_sleeper_task_offset >= 0 && task_struct_comm_offset >= 0) {
            ulong task_addr = read_pointer(hrtimer_addr + hrtimer_sleeper_task_offset,"task");
            if (!is_kvaddr(task_addr) && hrtimer_sleeper_timer_offset >= 0 &&
                hrtimer_addr > static_cast<ulong>(hrtimer_sleeper_timer_offset)) {
                ulong sleeper_addr = hrtimer_addr - static_cast<ulong>(hrtimer_sleeper_timer_offset);
                task_addr = read_pointer(sleeper_addr + hrtimer_sleeper_task_offset,"task");
            }
            if (is_kvaddr(task_addr)) {
                e.task = task_addr;
                e.comm = read_cstring(task_addr + task_struct_comm_offset, 16, "comm");
            }
        }

        // For timerfd_tmrproc, resolve the blocking task(s) via timerfd_ctx.wqh
        if (e.comm.empty() &&
            e.function_name.find("timerfd_tmrproc") != std::string::npos) {
            std::string waiters = resolve_timerfd_waiters(hrtimer_addr);
            if (!waiters.empty()) e.comm = waiters;
        }
        if (e.comm.empty()) e.comm = "-";
        entries.push_back(e);
    }

    std::sort(entries.begin(), entries.end(),
              [](const HrtimerEntryInfo& a, const HrtimerEntryInfo& b) {
                  return a.expires < b.expires;
              });
    return entries;
}

void TimerParser::dump_hrtimer() {
    if (!csymbol_exists("hrtimer_bases")) {
        PRINT("hrtimer_bases doesn't exist in this kernel\n");
        return;
    }

    ulong hrtimer_bases_addr = csymbol_value("hrtimer_bases");
    if (!is_kvaddr(hrtimer_bases_addr)) {
        PRINT("hrtimer_bases address invalid: %#lx\n", hrtimer_bases_addr);
        return;
    }
    if (hrtimer_cpu_base_clock_base_offset < 0 || hrtimer_clock_base_size <= 0) {
        PRINT("hrtimer_clock_base offsets unavailable\n");
        return;
    }

    std::vector<ulong> per_cpu_bases = for_each_percpu(hrtimer_bases_addr);
    for (size_t cpu = 0; cpu < per_cpu_bases.size(); ++cpu) {
        if (target_cpu_filter >= 0 && static_cast<int>(cpu) != target_cpu_filter)
            continue;
        ulong hrtimer_cpu_base_addr = per_cpu_bases[cpu];
        if (!is_kvaddr(hrtimer_cpu_base_addr)) continue;
        PRINT("CPU %zu  hrtimer_cpu_base: %#lx\n", cpu, hrtimer_cpu_base_addr);
        ulong clock_base_array = hrtimer_cpu_base_addr + hrtimer_cpu_base_clock_base_offset;
        for (int j = 0; j < hrtimer_max_clock_bases; ++j) {
            ulong cb_addr = clock_base_array + static_cast<ulong>(j) * hrtimer_clock_base_size;
            std::string get_time_name;
            if (hrtimer_clock_base_get_time_offset >= 0) {
                ulong fn = read_pointer(cb_addr + hrtimer_clock_base_get_time_offset,"get_time");
                if (is_kvaddr(fn))
                    get_time_name = to_symbol(fn);
            }
            if (get_time_name.empty()) get_time_name = "?";
            // HRTIMER_MAX_CLOCK_BASES is typically 8; indices 0-3 are hardirq,
            // indices 4-7 are softirq bases added in Linux 5.4.
            const char* irq_mode = (hrtimer_max_clock_bases > 4 && j >= hrtimer_max_clock_bases / 2)
                                   ? "SOFT" : "HARD";
            std::string clockid_str;
            if (hrtimer_clock_base_clockid_offset >= 0) {
                int clockid = read_int(cb_addr + hrtimer_clock_base_clockid_offset,"clockid");
                const char* name = clockid_to_name(clockid);
                clockid_str = name ? name : ("CLOCKID(" + std::to_string(clockid) + ")");
            }
            PRINT("  CLOCK: %d  hrtimer_clock_base: %#lx  [%s | %s | %s]\n",
                  j, cb_addr, get_time_name.c_str(), irq_mode,
                  clockid_str.empty() ? "?" : clockid_str.c_str());

            std::vector<HrtimerEntryInfo> entries = collect_hrtimers_for_clock_base(cb_addr);
            if (entries.empty()) {
                PRINT("    (no hrtimers)\n");
                continue;
            }
            // Build box table rows
            const std::vector<std::string> hdrs = {"HRTIMER", "FUNCTION_NAME", "COMM", "SOFTEXPIRES(ns)", "EXPIRES(ns)"};
            const std::vector<bool> tbl_ra = {false, false, false, true, true};
            std::vector<std::vector<std::string>> tbl;
            tbl.reserve(entries.size());
            for (const auto& e : entries) {
                char addr_buf[20];
                snprintf(addr_buf, sizeof(addr_buf), "%#lx", e.addr);
                tbl.push_back({addr_buf, e.function_name, e.comm,
                               format_ktime_ns(e.softexpires),
                               format_ktime_ns(e.expires)});
            }

            std::string table = build_box_table("", hdrs, tbl, tbl_ra);
            // Indent the table by 4 spaces to reflect the CPU → clock-base hierarchy
            std::istringstream iss(table);
            std::string line;
            while (std::getline(iss, line))
                PRINT("    %s\n", line.c_str());
        }
        PRINT("\n");
    }
}

#pragma GCC diagnostic pop
