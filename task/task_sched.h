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

#ifndef TASK_SCHED_DEFS_H_
#define TASK_SCHED_DEFS_H_

#include "plugin.h"

struct schedinfo {
    struct task_context *tc;
    uint32_t task_prio = 0;
    uint64_t last_arrival = 0;
    uint64_t last_queued = 0;
    uint32_t pcount = 0;
    uint64_t run_delay = 0;
    uint64_t last_enqueued = 0;
    uint64_t last_sleep = 0;
    uint64_t runtime = 0;
};

class TaskSched : public ParserPlugin {
private:
    void print_task_timestamps(int cpu);

public:
    TaskSched();
    void init_offset(void) override;
    void init_command(void) override;
    void cmd_main(void) override;
    DEFINE_PLUGIN_INSTANCE(TaskSched)
};

#endif // TASK_SCHED_DEFS_H_
