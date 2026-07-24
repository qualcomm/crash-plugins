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

#ifndef WORKQUEUE_DEFS_H_
#define WORKQUEUE_DEFS_H_

#include "plugin.h"
#include <map>

/**
 * @brief Worker thread flags from Linux kernel
 * These flags indicate the current state and properties of worker threads
 */
enum worker_flags {
    WORKER_DIE = 1 << 1,            /* Worker is dying */
    WORKER_IDLE = 1 << 2,           /* Worker is idle */
    WORKER_PREP = 1 << 3,           /* Worker is being prepared */
    WORKER_CPU_INTENSIVE = 1 << 6,  /* Worker is CPU intensive */
    WORKER_UNBOUND = 1 << 7,        /* Worker is unbound to specific CPU */
    WORKER_REBOUND = 1 << 8,        /* Worker is being rebound */
};

/**
 * @brief Worker pool flags from Linux kernel
 * These flags indicate the state of worker pools
 */
enum worker_pool_flags {
    POOL_MANAGER_ACTIVE = 1 << 0,   /* Pool manager is active */
    POOL_DISASSOCIATED = 1 << 2,    /* Pool is disassociated from CPU */
};

/**
 * @brief Workqueue structure flags from Linux kernel
 * These flags define workqueue behavior and properties
 */
enum workqueue_struct_flags {
    WQ_UNBOUND = 1 << 1,            /* Workqueue is not bound to specific CPU */
    WQ_FREEZABLE = 1 << 2,          /* Workqueue is freezable during suspend */
    WQ_MEM_RECLAIM = 1 << 3,        /* Workqueue may be used in memory reclaim path */
    WQ_HIGHPRI = 1 << 4,            /* Workqueue is high priority */
    WQ_CPU_INTENSIVE = 1 << 5,      /* Workqueue is CPU intensive */
    WQ_SYSFS = 1 << 6,              /* Workqueue is visible in sysfs */
};

/* Forward declarations */
struct worker;
struct worker_pool;
struct pool_workqueue;
struct workqueue_struct;

/**
 * @brief Represents a work item in the kernel workqueue system
 * Contains function pointer and data for deferred work execution
 */
struct work_struct {
    ulong addr;     /* Kernel address of work_struct */
    ulong data;     /* Work data field */
    ulong func;     /* Function pointer to execute */
};

/**
 * @brief Represents a worker thread in the kernel workqueue system
 * Worker threads execute work items from worker pools
 */
struct worker {
    ulong addr;                             /* Kernel address of worker struct */
    std::string comm;                       /* Process command name */
    int pid;                                /* Process ID */
    ulong current_pwq;                      /* Current pool workqueue */
    ulong current_func;                     /* Currently executing function */
    ulong task_addr;                        /* Associated task_struct address */
    ulong last_active;                      /* Last activity timestamp */
    std::string flags;                      /* Worker flags as string */
    int id;                                 /* Worker ID */
    int sleeping;                           /* Sleep state */
    std::string desc;                       /* Worker description */
    ulong last_func;                        /* Last executed function */
    std::shared_ptr<worker_pool> wp_ptr;    /* Pointer to parent worker pool */
};

/**
 * @brief Represents a worker pool in the kernel workqueue system
 * Worker pools manage groups of worker threads for work execution
 */
struct worker_pool {
    ulong addr;                                                 /* Kernel address of worker_pool struct */
    int cpu;                                                    /* Associated CPU (-1 for unbound) */
    int node;                                                   /* NUMA node */
    int id;                                                     /* Pool ID */
    int nice;                                                   /* Nice value (0=normal, -20=high priority) */
    std::string flags;                                          /* Pool flags as string */
    ulong watchdog_ts;                                          /* Watchdog timestamp */
    int nr_running;                                             /* Number of running workers */
    std::vector<std::shared_ptr<work_struct>> worklist;        /* Pending work items */
    int nr_workers;                                             /* Total number of workers */
    int nr_idle;                                                /* Number of idle workers */
    std::set<std::shared_ptr<pool_workqueue>> pwq_list;        /* Associated pool workqueues */
    std::vector<std::shared_ptr<worker>> idle_list;            /* List of idle workers */
    std::vector<std::shared_ptr<worker>> workers;              /* All workers in this pool */
    std::vector<std::shared_ptr<worker>> busy_workers;         /* Currently busy workers */
    std::shared_ptr<worker> manager;                            /* Manager worker */
};

/**
 * @brief Represents a pool workqueue in the kernel workqueue system
 * Pool workqueues connect workqueues to worker pools
 */
struct pool_workqueue {
    ulong addr;                                             /* Kernel address of pool_workqueue struct */
    std::shared_ptr<workqueue_struct> wq_ptr;              /* Pointer to parent workqueue */
    std::shared_ptr<worker_pool> wp_ptr;                    /* Pointer to associated worker pool */
    int refcnt;                                             /* Reference count */
    int nr_active;                                          /* Number of active work items */
    int max_active;                                         /* Maximum active work items */
    std::vector<std::shared_ptr<work_struct>> inactive_works; /* Delayed/inactive work items */
};

/**
 * @brief Represents a workqueue in the kernel workqueue system
 * Workqueues are the main interface for submitting work items
 */
struct workqueue_struct {
    ulong addr;                                         /* Kernel address of workqueue_struct */
    std::string name;                                   /* Workqueue name */
    std::string flags;                                  /* Workqueue flags as string */
    int nr_drainers;                                    /* Number of drainers */
    std::vector<std::shared_ptr<pool_workqueue>> pwqs; /* Associated pool workqueues */
    std::shared_ptr<worker> rescuer;                    /* Rescuer worker */
};

/**
 * @brief Main workqueue analysis plugin class
 * Provides comprehensive analysis of Linux kernel workqueue subsystem
 */
class Workqueue : public ParserPlugin {
private:
    /* Data structures for parsed workqueue information */
    std::vector<std::shared_ptr<workqueue_struct>> workqueue_list;
    std::unordered_map<ulong, std::shared_ptr<worker>> worker_list_map;
    std::unordered_map<ulong, std::shared_ptr<worker_pool>> worker_pool_map;
    std::vector<std::shared_ptr<worker>> idle_worker_list;

    /* Display formatting variables */
    size_t workqueue_name_len = 0;
    size_t worker_flags_len = 0;
    size_t worker_pool_flags_len = 0;

    /* Core parsing functions */
    void parse_workqueue();
    void parse_cpu_worker_pools();
    void parse_unbound_pool_hash();
    void parse_busy_hash(std::shared_ptr<worker_pool> wp_ptr, ulong pool_addr);

    /* Individual structure parsers */
    std::shared_ptr<workqueue_struct> parser_workqueue_struct(ulong addr);
    std::shared_ptr<pool_workqueue> parser_pool_workqueue(ulong addr);
    std::shared_ptr<worker_pool> parser_worker_pool(ulong addr);
    std::shared_ptr<worker> parser_worker(ulong addr, std::shared_ptr<worker_pool> wp_ptr);
    std::vector<std::shared_ptr<worker>> parser_worker_list(ulong list_head_addr, int offset, std::shared_ptr<worker_pool> wp_ptr);
    std::vector<std::shared_ptr<work_struct>> parser_work_list(ulong list_head);

    /* Display functions */
    void print_workqueue_detailed();
    void print_worker_pool_detailed();
    void print_worker_pool_summary(const std::map<int, std::vector<std::shared_ptr<worker_pool>>>& cpu_pools,
                                   const std::vector<std::shared_ptr<worker_pool>>& unbound_pools);

    /* Detail display functions */
    void show_workqueue_details(std::shared_ptr<workqueue_struct> wq);
    void show_worker_pool_details(std::shared_ptr<worker_pool> pool, bool is_last_group);
    void show_worker_pool_info(std::shared_ptr<worker_pool> pool, bool is_last);
    void show_pwq_details(std::shared_ptr<pool_workqueue> pwq, const char* prefix, bool is_last_pwq);

    /* Utility functions */
    bool has_activity(std::shared_ptr<workqueue_struct> wq);
    template <typename T>
    std::string parser_flags(uint flags, const std::unordered_map<T, std::string>& flags_array);

public:
    Workqueue();
    void cmd_main(void) override;
    void init_offset(void) override;
    void init_command(void) override;
    DEFINE_PLUGIN_INSTANCE(Workqueue)
};

#endif // WORKQUEUE_DEFS_H_
