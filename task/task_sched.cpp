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

#include "task_sched.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(TaskSched)
#endif

void TaskSched::cmd_main(void) {
    int c;
    std::string cppString;
    if (argcnt < 2) cmd_usage(pc->curcmd, SYNOPSIS);
    while ((c = getopt(argcnt, args, "c:")) != EOF) {
        switch(c) {
            case 'c':
            {
                cppString.assign(optarg);
                int cpu = 0;
                try {
                    cpu = std::stoi(cppString);
                } catch (...) {
                    LOGE("invaild cpu arg %s\n",cppString.c_str());
                    break;
                }
                print_task_timestamps(cpu);
                break;
            }
            default:
                argerrs++;
                break;
        }
    }
    if (argerrs)
        cmd_usage(pc->curcmd, SYNOPSIS);
}

void TaskSched::init_offset(void) {
    field_init(task_struct,sched_info);
    field_init(task_struct,exit_state);
    field_init(task_struct,prio);
    field_init(task_struct,__state);
    field_init(task_struct,state);
    field_init(task_struct,last_enqueued_ts);
    field_init(task_struct,last_sleep_ts);
    field_init(task_struct,wts);
    field_init(task_struct,android_vendor_data1);
    field_init(sched_info,last_arrival);
    field_init(sched_info,last_queued);
    field_init(sched_info,pcount);
    field_init(sched_info,run_delay);
    struct_init(sched_info);
    struct_init(walt_task_struct);
    field_init(walt_task_struct,last_enqueued_ts);
    field_init(walt_task_struct,last_sleep_ts);
}

void TaskSched::init_command(void) {
    cmd_name = "sched";
    help_str_list={
        "sched",                            /* command name */
        "dump task sched information",        /* short description */
        "-c <cpu>\n"
            "  This command dumps the task sched info.",
        "\n",
        "EXAMPLES",
        "  Display task sched info:",
        "    %s> sched -c 1 ",
        "    Name                 pid     cpu   Exec_Started    Last_Queued     Total_wait_time times_exec Prio  State Last_enqueued_ts Last_sleep_ts   Last_runtime",
        "    swapper/3            0       3     0               0               0               0          120   RU    0               0               0",
        "    idle_inject/3        44      3     0.0894034       0               0               3          49    IN    0               0               0",
        "    oom_reaper           61      3     0.591827        0               1.6563e-05      2          120   IN    0               0               0",
        "\n",
    };
}

TaskSched::TaskSched(){}

void TaskSched::print_task_timestamps(int cpu){
    std::vector<std::shared_ptr<schedinfo>> task_list;
    for(ulong task_addr: for_each_threads()){
        struct task_context *tc = task_to_context(task_addr);
        if(!tc || tc->processor != cpu){
            continue;
        }
        std::shared_ptr<schedinfo> sched_ptr = std::make_shared<schedinfo>();
        sched_ptr->tc = tc;
        sched_ptr->task_prio = read_uint(task_addr + field_offset(task_struct,prio),"prio");
        if(struct_size(sched_info) != -1){
            sched_ptr->last_arrival = read_ulonglong(task_addr + field_offset(task_struct, sched_info) + field_offset(sched_info, last_arrival), "last_arrival");
            sched_ptr->last_queued = read_ulonglong(task_addr + field_offset(task_struct, sched_info) + field_offset(sched_info, last_queued), "last_queued");
            sched_ptr->pcount = read_uint(task_addr + field_offset(task_struct, sched_info) + field_offset(sched_info, pcount), "pcount");
            sched_ptr->run_delay = read_ulonglong(task_addr + field_offset(task_struct, sched_info) + field_offset(sched_info, run_delay), "run_delay");
        }
        if(field_offset(task_struct, last_enqueued_ts) != -1){
            sched_ptr->last_enqueued = read_ulonglong(task_addr + field_offset(task_struct, last_enqueued_ts), "last_enqueued_ts");
        } else {
            if (get_config_val("CONFIG_SCHED_WALT") == "y" && struct_size(walt_task_struct) != -1) {
                if(field_offset(task_struct, wts) != -1){
                    sched_ptr->last_enqueued = read_ulonglong(task_addr + field_offset(task_struct, wts) + field_offset(walt_task_struct, last_enqueued_ts), "last_enqueued_ts");
                }else{
                    sched_ptr->last_enqueued = read_ulonglong(task_addr + field_offset(task_struct, android_vendor_data1) + field_offset(walt_task_struct, last_enqueued_ts), "last_enqueued_ts");
                }
            }
        }
        if(field_offset(task_struct, last_sleep_ts) != -1){
            sched_ptr->last_sleep = read_ulonglong(task_addr + field_offset(task_struct, last_sleep_ts), "last_sleep_ts");
        } else {
            if (get_config_val("CONFIG_SCHED_WALT") == "y" && struct_size(walt_task_struct) != -1) {
                if(field_offset(task_struct, wts) != -1){
                    sched_ptr->last_sleep = read_ulonglong(task_addr + field_offset(task_struct, wts) + field_offset(walt_task_struct, last_sleep_ts), "last_sleep_ts");
                }else{
                    sched_ptr->last_sleep = read_ulonglong(task_addr + field_offset(task_struct, android_vendor_data1) + field_offset(walt_task_struct, last_sleep_ts), "last_sleep_ts");
                }
            }
        }
        if (sched_ptr->last_enqueued < sched_ptr->last_sleep){
            sched_ptr->runtime = sched_ptr->last_sleep - sched_ptr->last_enqueued;
        }
        task_list.push_back(sched_ptr);
    }
    std::ostringstream oss;
    oss << std::left << std::setw(20)   << "Name" << " "
        << std::left << std::setw(7)    << "pid" << " "
        << std::left << std::setw(3)    << "cpu" << " "
        << std::left << std::setw(15)   << "Exec_Started" << " "
        << std::left << std::setw(15)   << "Last_Queued" << " "
        << std::left << std::setw(15)   << "Total_wait_time" << " "
        << std::left << std::setw(10)   << "times_exec" << " "
        << std::left << std::setw(5)    << "Prio" << " "
        << std::left << std::setw(5)    << "State" << " "
        << std::left << std::setw(15)   << "Last_enqueued_ts" << " "
        << std::left << std::setw(15)   << "Last_sleep_ts" << " "
        << std::left << std::setw(15)   << "Last_runtime" << "\n";
    if (task_list.size() == 0){
        return;
    }
    std::sort(task_list.begin(), task_list.end(),[&](const std::shared_ptr<schedinfo>& a, const std::shared_ptr<schedinfo>& b){
        return a->last_arrival < b->last_arrival;
    });
    for (const auto& sched_ptr : task_list) {
        char buf1[BUFSIZE];
        char buf2[BUFSIZE];
        oss << std::left << std::setw(20)   << sched_ptr->tc->comm << " "
            << std::left << std::setw(7)    << sched_ptr->tc->pid << " "
            << std::left << std::setw(3)    << task_cpu(sched_ptr->tc->processor, buf2, !VERBOSE) << " "
            << std::left << std::setw(15)   << (double)sched_ptr->last_arrival/1000000000.0 << " "
            << std::left << std::setw(15)   << (double)sched_ptr->last_queued/1000000000.0 << " "
            << std::left << std::setw(15)   << (double)sched_ptr->run_delay/1000000000.0 << " "
            << std::left << std::setw(10)   << sched_ptr->pcount << " "
            << std::left << std::setw(5)    << sched_ptr->task_prio << " "
            << std::left << std::setw(5)    << task_state_string(sched_ptr->tc->task, buf1, !VERBOSE) << " "
            << std::left << std::setw(15)   << (double)sched_ptr->last_enqueued/1000000000.0 << " "
            << std::left << std::setw(15)   << (double)sched_ptr->last_sleep/1000000000.0 << " "
            << std::left << std::setw(15)   << (double)sched_ptr->runtime/1000000.0 << "\n";
    }
    PRINT("%s \n", oss.str().c_str());
}

#pragma GCC diagnostic pop
