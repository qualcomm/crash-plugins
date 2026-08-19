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

#ifndef TIMER_DEFS_H_
#define TIMER_DEFS_H_

#include "plugin.h"

struct TimerVectorConfig {
    std::string name;
    int size = 0;
    int offset = -1;
    bool use_hlist = true;
};

struct TimerEntryInfo {
    size_t index = 0;
    ulong addr = 0;
    ulong expires = 0;
    double expires_sec = 0.0;
    std::string function;
    std::string data_or_work;
    std::string wakeup_comm;
    std::string remarks;
    bool valid = true;
};

struct HrtimerEntryInfo {
    ulong addr = 0;
    ulong function = 0;
    std::string function_name;
    ulonglong softexpires = 0;
    ulonglong expires = 0;
    ulong task = 0;
    std::string comm;
};

class TimerParser : public ParserPlugin {
private:
    bool timer_42 = false;
    bool timer_69 = false;
    bool timer_has_data = true;
    bool use_timer_base = false;
    bool no_hz_common = false;

    double hz = 100.0;
    std::string tvec_bases_symbol;
    std::string global_deferrable_symbol;

    std::vector<std::string> base_types;
    std::vector<TimerVectorConfig> vectors;

    int base_struct_size = 0;
    int list_head_size = 0;
    int hlist_head_size = 0;

    int timer_jiffies_offset = -1;
    int next_timer_offset = -1;
    int active_timers_offset = -1;

    int timer_entry_offset = -1;
    int timer_function_offset = -1;
    int timer_expires_offset = -1;
    int timer_data_offset = -1;
    int timer_base_offset = -1;

    int delayed_work_timer_offset = -1;
    int work_struct_func_offset = -1;

    int hrtimer_cpu_base_clock_base_offset = -1;
    int hrtimer_clock_base_active_offset = -1;
    int hrtimer_clock_base_get_time_offset = -1;
    int hrtimer_clock_base_clockid_offset = -1;
    int hrtimer_clock_base_size = 0;
    int hrtimer_max_clock_bases = 0;

    int hrtimer_node_offset = -1;
    int hrtimer_function_offset = -1;
    int hrtimer_softexpires_offset = -1;
    int hrtimer_sleeper_timer_offset = -1;
    int hrtimer_sleeper_task_offset = -1;
    int task_struct_comm_offset = -1;
    int timerqueue_node_expires_offset = -1;
    int timerqueue_node_node_offset = 0;

    // timerfd_ctx resolution (for timerfd_tmrproc callbacks)
    int timerfd_ctx_t_offset = -1;
    int timerfd_ctx_wqh_offset = -1;
    int wait_queue_head_head_offset = -1;
    int wait_queue_entry_private_offset = -1;
    int wait_queue_entry_entry_offset = -1;

    void setup_layout_by_kernel_version();
    void setup_offsets();

    double jiffies_to_seconds(ulong jiffies_val) const;
    void print_jiffies_conversion(ulonglong jiffies_val) const;
    std::string base_type_name(size_t idx) const;
    bool is_delayed_work_timer_fn(const std::string& symbol) const;
    bool is_process_timeout_timer_fn(const std::string& symbol) const;
    std::string resolve_process_timeout_comm(ulong timer_addr);

    // When hrtimer callback is timerfd_tmrproc, walk timerfd_ctx.wqh
    // to collect "pid(comm)" strings for all waiting tasks.
    std::string resolve_timerfd_waiters(ulong hrtimer_addr);

    TimerEntryInfo parse_timer_entry(ulong timer_addr, size_t index, ulong expected_base);
    std::vector<TimerEntryInfo> collect_vector_timers(const TimerVectorConfig& vec, ulong base);
    void print_vector_timers(const TimerVectorConfig& vec, const std::vector<TimerEntryInfo>& entries);
    void dump_timer_base(ulong base, const std::string& title_prefix, int cpu = -1);

    void dump_global_deferrable_base();
    void dump_percpu_timer_bases();
    void dump_tick_do_timer_cpu();
    void dump_timer_list();

    std::vector<HrtimerEntryInfo> collect_hrtimers_for_clock_base(ulong clock_base_addr);
    void dump_hrtimer();

    int target_cpu_filter = -1;  // -1 = all CPUs; set by -c option

public:
    TimerParser();
    void cmd_main(void) override;
    void init_command(void) override;
    void init_offset(void) override;
    DEFINE_PLUGIN_INSTANCE(TimerParser)
};

#endif // TIMER_DEFS_H_
