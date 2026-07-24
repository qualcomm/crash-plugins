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

#include "workqueue.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(Workqueue)
#endif

/**
 * @brief Main command entry point for workqueue analysis
 *
 * Parses command line arguments and dispatches to appropriate handler functions:
 * -W: Display active workqueues with detailed information
 * -w: Display worker pools with comprehensive statistics
 */
void Workqueue::cmd_main(void) {
    if (argcnt < 2) {
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }

    /* Parse workqueue data if not already cached */
    if (workqueue_list.empty()) {
        LOGI("Workqueue list is empty, parsing workqueues");
        parse_workqueue();
    } else {
        LOGD("Using cached workqueue list (size=%zu)", workqueue_list.size());
    }

    int c;
    while ((c = getopt(argcnt, args, "Ww")) != EOF) {
        switch(c) {
            case 'W':
                print_workqueue_detailed();
                break;
            case 'w':
                print_worker_pool_detailed();
                break;
            default:
                LOGW("Unknown option: %c", c);
                argerrs++;
                break;
        }
    }

    if (argerrs) {
        cmd_usage(pc->curcmd, SYNOPSIS);
    }

    LOGD("Command execution completed");
}

/**
 * @brief Initialize kernel structure field offsets
 *
 * Sets up field offsets for workqueue-related kernel structures.
 * These offsets are essential for reading workqueue data from kernel memory.
 */
void Workqueue::init_offset(void) {
    /* Initialize workqueue_struct field offsets */
    field_init(workqueue_struct, list);
    field_init(workqueue_struct, name);
    field_init(workqueue_struct, flags);
    field_init(workqueue_struct, pwqs);

    /* Initialize pool_workqueue field offsets */
    field_init(pool_workqueue, pwqs_node);
    field_init(pool_workqueue, inactive_works);
    field_init(pool_workqueue, pool);
    field_init(pool_workqueue, refcnt);
    field_init(pool_workqueue, nr_active);
    field_init(pool_workqueue, max_active);

    /* Initialize worker_pool field offsets */
    field_init(worker_pool, cpu);
    field_init(worker_pool, node);
    field_init(worker_pool, id);
    field_init(worker_pool, flags);
    field_init(worker_pool, watchdog_ts);
    field_init(worker_pool, nr_running);
    field_init(worker_pool, worklist);
    field_init(worker_pool, nr_workers);
    field_init(worker_pool, nr_idle);
    field_init(worker_pool, idle_list);
    field_init(worker_pool, workers);
    field_init(worker_pool, busy_hash);
    field_init(worker_pool, manager);
    field_init(worker_pool, attrs);

    /* Initialize workqueue_attrs field offsets */
    struct_init(workqueue_attrs);
    field_init(workqueue_attrs, nice);

    /* Initialize worker field offsets */
    field_init(worker, node);
    field_init(worker, entry);
    field_init(worker, current_work);
    field_init(worker, current_func);
    field_init(worker, current_pwq);
    field_init(worker, task);
    field_init(worker, last_active);
    field_init(worker, id);
    field_init(worker, sleeping);
    field_init(worker, desc);
    field_init(worker, last_func);
    field_init(worker, flags);
    field_init(worker, hentry);

    /* Initialize work_struct field offsets */
    field_init(work_struct, entry);
    field_init(work_struct, data);
    field_init(work_struct, func);

    /* Initialize structure sizes */
    struct_init(workqueue_struct);
    struct_init(pool_workqueue);
    struct_init(worker_pool);
    struct_init(worker);
    struct_init(work_struct);

    /* Initialize hash list structures */
    struct_init(hlist_head);
    field_init(worker_pool, hash_node);
    field_init(hlist_head, first);
    field_init(hlist_node, next);
}

void Workqueue::init_command(void) {
    cmd_name = "wq";
    help_str_list={
        "wq",                                /* command name */
        "display workqueue information",     /* short description */
        "[-W] [-w]\n"
        "  This command displays Linux kernel workqueue information.\n"
        "\n"
        "    -W              display active workqueues with detailed information\n"
        "    -w              display worker pools with comprehensive statistics\n",
        "\n",
        "EXAMPLES",
        "  Display active workqueues with activity details:",
        "    %s> wq -W",
        "    [WORKQUEUE] events (WQ_MEM_RECLAIM)",
        "      ├─ pwq:0xffffff8003e40000 → worker_pool[0xffffff8003e40400] CPU:0  Normal (workers:4, idle:3, busy:1, pending:2)",
        "      │                               ├─ BUSY WORKERS:",
        "      │                               │  └─ kworker/0:1     [pid:123   ] → schedule_work_func",
        "      │                               └─ PENDING WORKS: (2 items)",
        "      │                                  ├─ delayed_work_timer_fn",
        "      │                                  └─ flush_to_ldisc",
        "\n",
        "  Display comprehensive worker pool information:",
        "    %s> wq -w",
        "    ===============================================================================",
        "                               WORKER POOL SUMMARY",
        "    ===============================================================================",
        "    System Overview:",
        "      CPUs: 8          Pools: 16       Workers: 32       Pending: 5",
        "    ",
        "    Pool Distribution:",
        "      Per-CPU Pools: 16 (8 Normal, 8 High Priority)",
        "      Unbound Pools: 0 (0 Normal, 0 High Priority)",
        "    ===============================================================================",
        "    ",
        "    CPU[0]: 2 pools, 4 workers, 2 pending works",
        "    ├─ worker_pool:0xffffff8003e40400 Normal (workers:2, idle:1, busy:1, pending:2)",
        "    │  ├─ Status: Normal | Flags: None",
        "    │  ├─ Idle Workers:",
        "    │  │  └─ kworker/0:0 (pid:8)",
        "    │  ├─ Busy Workers:",
        "    │  │  └─ kworker/0:1 (pid:10) → schedule_work_func",
        "    │  └─ Pending Works:",
        "    │     ├─ delayed_work_timer_fn",
        "    │     └─ flush_to_ldisc",
        "    └─ worker_pool:0xffffff8003e40800 High (workers:2, idle:2, busy:0, pending:0)",
        "       ├─ Status: Normal | Flags: None",
        "       └─ Idle Workers:",
        "          ├─ kworker/0:0H (pid:12)",
        "          └─ kworker/0:1H (pid:14)",
        "\n",
    };
}

/**
 * @brief Default constructor
 */
Workqueue::Workqueue() {}

/**
 * @brief Parse kernel flags into human-readable string format
 *
 * @param flags Raw flag value from kernel
 * @param flags_array Map of flag values to string names
 * @return String representation of flags
 */
template <typename T>
std::string Workqueue::parser_flags(uint flags,
                                    const std::unordered_map<T, std::string>& flags_array) {
    if (flags == 0) {
        return "None";
    }

    std::vector<std::string> flag_names;
    for (const auto& pair : flags_array) {
        if (flags & pair.first) {
            flag_names.push_back(pair.second);
        }
    }

    if (flag_names.empty()) {
        return "None";
    }

    std::ostringstream oss;
    for (size_t i = 0; i < flag_names.size(); ++i) {
        if (i > 0) oss << "|";
        oss << flag_names[i];
    }
    return oss.str();
}

std::shared_ptr<worker> Workqueue::parser_worker(ulong addr,
                                                  std::shared_ptr<worker_pool> wp_ptr) {
    auto worker_ptr = std::make_shared<worker>();
    worker_ptr->addr = addr;
    worker_ptr->wp_ptr = wp_ptr;
    static const std::unordered_map<worker_flags, std::string> flag_names = {
        {WORKER_DIE, "WORKER_DIE"},
        {WORKER_IDLE, "WORKER_IDLE"},
        {WORKER_PREP, "WORKER_PREP"},
        {WORKER_CPU_INTENSIVE, "WORKER_CPU_INTENSIVE"},
        {WORKER_UNBOUND, "WORKER_UNBOUND"},
        {WORKER_REBOUND, "WORKER_REBOUND"},
    };
    void *worker_buf = read_struct(addr, "worker");
    unsigned int flags = UINT(worker_buf + field_offset(worker, flags));
    worker_ptr->flags = parser_flags(flags, flag_names);
    worker_flags_len = std::max(worker_flags_len, worker_ptr->flags.size());
    worker_ptr->current_func = ULONG(worker_buf + field_offset(worker, current_func));
    worker_ptr->current_pwq = ULONG(worker_buf + field_offset(worker, current_pwq));
    worker_ptr->task_addr = ULONG(worker_buf + field_offset(worker, task));
    worker_ptr->last_active = ULONG(worker_buf + field_offset(worker, last_active));
    worker_ptr->id = INT(worker_buf + field_offset(worker, id));
    worker_ptr->sleeping = INT(worker_buf + field_offset(worker, sleeping));
    worker_ptr->last_func = ULONG(worker_buf + field_offset(worker, last_func));
    worker_ptr->desc = read_cstring(addr + field_offset(worker, desc), 24, "worker_desc");
    if (worker_ptr->desc.empty()) {
        worker_ptr->desc = "None";
    }
    workqueue_name_len = std::max(workqueue_name_len, worker_ptr->desc.size());
    struct task_context *tc = task_to_context(worker_ptr->task_addr);
    if (tc) {
        worker_ptr->comm = tc->comm;
        worker_ptr->pid = tc->pid;
    } else {
        worker_ptr->comm = "";
        worker_ptr->pid = -1;
    }
    FREEBUF(worker_buf);
    LOGI("    worker at 0x%lx (id=%d, flags=%s), task: %s (pid=%d)",
         addr, worker_ptr->id, worker_ptr->flags.c_str(),worker_ptr->comm, worker_ptr->pid);
    return worker_ptr;
}


std::vector<std::shared_ptr<worker>> Workqueue::parser_worker_list(ulong list_head_addr,int offset,std::shared_ptr<worker_pool> wp_ptr){
    std::vector<std::shared_ptr<worker>> worker_list;
    std::shared_ptr<worker> worker_ptr;
    for(const auto& worker_addr : for_each_list(list_head_addr, offset)){
        if (worker_list_map.find(worker_addr) == worker_list_map.end()) {
            worker_ptr = parser_worker(worker_addr,wp_ptr);
            worker_list_map[worker_addr] = worker_ptr;
        } else {
            worker_ptr = worker_list_map[worker_addr];
        }
        worker_list.push_back(worker_ptr);
    }
    return worker_list;
}

std::vector<std::shared_ptr<work_struct>> Workqueue::parser_work_list(ulong list_head_addr){
    std::vector<std::shared_ptr<work_struct>> work_list;
    for(const auto& work_addr : for_each_list(list_head_addr, field_offset(work_struct, entry))){
        LOGD("  Parsed work_struct at 0x%lx", work_addr);
        auto work_ptr = std::make_shared<work_struct>();
        work_ptr->addr = work_addr;
        work_ptr->data = read_ulong(work_addr + field_offset(work_struct, data), "work_struct_data");
        work_ptr->func = read_pointer(work_addr + field_offset(work_struct, func), "work_struct_func");
        work_list.push_back(work_ptr);
    }
    return work_list;
}

std::shared_ptr<worker_pool> Workqueue::parser_worker_pool(ulong addr){
    LOGD("Parsing worker_pool at address: 0x%lx", addr);
    auto wp_ptr = std::make_shared<worker_pool>();
    wp_ptr->addr = addr;
    void *wp_buf = read_struct(addr, "worker_pool");
    wp_ptr->cpu = INT(wp_buf + field_offset(worker_pool, cpu));
    wp_ptr->node = INT(wp_buf + field_offset(worker_pool, node));
    wp_ptr->id = INT(wp_buf + field_offset(worker_pool, id));

    wp_ptr->nice = 0;
    ulong attrs_addr = ULONG(wp_buf + field_offset(worker_pool, attrs));
    if (is_kvaddr(attrs_addr)) {
        void *attrs_buf = read_struct(attrs_addr, "workqueue_attrs");
        if (attrs_buf) {
            wp_ptr->nice = INT(attrs_buf + field_offset(workqueue_attrs, nice));
            FREEBUF(attrs_buf);
        }
    }

    static const std::unordered_map<worker_pool_flags, std::string> str_flag_array = {
        {POOL_MANAGER_ACTIVE, "POOL_MANAGER_ACTIVE"},
        {POOL_DISASSOCIATED, "POOL_DISASSOCIATED"}
    };
    unsigned int flags = UINT(wp_buf + field_offset(worker_pool, flags));
    wp_ptr->flags = parser_flags(flags, str_flag_array);
    worker_pool_flags_len = std::max(worker_pool_flags_len, wp_ptr->flags.size());
    wp_ptr->nr_workers = INT(wp_buf + field_offset(worker_pool, nr_workers));
    wp_ptr->nr_idle = INT(wp_buf + field_offset(worker_pool, nr_idle));
    wp_ptr->watchdog_ts = ULONG(wp_buf + field_offset(worker_pool, watchdog_ts));
    wp_ptr->nr_running = INT(wp_buf + field_offset(worker_pool, nr_running));
    wp_ptr->worklist = parser_work_list(addr + field_offset(worker_pool, worklist));
    int offset = field_offset(worker, node);
    wp_ptr->workers = parser_worker_list(addr + field_offset(worker_pool, workers),offset,wp_ptr);
    offset = field_offset(worker, entry);
    wp_ptr->idle_list = parser_worker_list(addr + field_offset(worker_pool, idle_list), offset,wp_ptr);
    idle_worker_list.insert(idle_worker_list.end(), wp_ptr->idle_list.begin(), wp_ptr->idle_list.end());

    // Parse manager field
    ulong manager_addr = ULONG(wp_buf + field_offset(worker_pool, manager));
    if (is_kvaddr(manager_addr)) {
        // Check if we already parsed this worker
        auto manager_it = worker_list_map.find(manager_addr);
        if (manager_it != worker_list_map.end()) {
            wp_ptr->manager = manager_it->second;
        } else {
            // Parse new manager worker
            wp_ptr->manager = parser_worker(manager_addr, wp_ptr);
            worker_list_map[manager_addr] = wp_ptr->manager;
        }
        LOGI("  Found manager worker at 0x%lx (pid=%d)",
             manager_addr, wp_ptr->manager->pid);
    }

    // Parse busy_hash
    parse_busy_hash(wp_ptr, addr);

    FREEBUF(wp_buf);
    LOGI("worker_pool at 0x%lx (cpu=%d, workers=%d, idle=%d, busy=%zu, works=%zu)",
         addr, wp_ptr->cpu, wp_ptr->nr_workers, wp_ptr->nr_idle,
         wp_ptr->busy_workers.size(), wp_ptr->worklist.size());
    return wp_ptr;
}

std::shared_ptr<pool_workqueue> Workqueue::parser_pool_workqueue(ulong addr){
    auto pwq_ptr = std::make_shared<pool_workqueue>();
    pwq_ptr->addr = addr;
    void *pwq_buf = read_struct(addr, "pool_workqueue");
    pwq_ptr->refcnt = INT(pwq_buf + field_offset(pool_workqueue, refcnt));
    pwq_ptr->nr_active = INT(pwq_buf + field_offset(pool_workqueue, nr_active));
    pwq_ptr->max_active = INT(pwq_buf + field_offset(pool_workqueue, max_active));
    ulong pool_addr = ULONG(pwq_buf + field_offset(pool_workqueue, pool));
    FREEBUF(pwq_buf);
    if (is_kvaddr(pool_addr)){
        std::shared_ptr<worker_pool> wp_ptr;
        if (worker_pool_map.find(pool_addr) == worker_pool_map.end()){ // Do not find the pool_addr
            wp_ptr = parser_worker_pool(pool_addr);
            wp_ptr->pwq_list.insert(pwq_ptr);
            worker_pool_map[pool_addr] = wp_ptr;
        }else{
            wp_ptr = worker_pool_map[pool_addr];
        }
        pwq_ptr->wp_ptr = wp_ptr;
    }
    if(field_offset(pool_workqueue, inactive_works) >= 0){
        pwq_ptr->inactive_works = parser_work_list(addr + field_offset(pool_workqueue, inactive_works));
    } else {
        field_init(pool_workqueue, delayed_works);
        pwq_ptr->inactive_works = parser_work_list(addr + field_offset(pool_workqueue, delayed_works));
    }
    LOGI("    pool_workqueue at 0x%lx (refcnt=%d, nr_active=%d, max_active=%d)",
         addr, pwq_ptr->refcnt, pwq_ptr->nr_active, pwq_ptr->max_active);
    return pwq_ptr;
}

std::shared_ptr<workqueue_struct> Workqueue::parser_workqueue_struct(ulong addr){
    auto wq_ptr = std::make_shared<workqueue_struct>();
    wq_ptr->addr = addr;
    wq_ptr->name = read_cstring(addr + field_offset(workqueue_struct, name), 24, "name");
    unsigned int flags = read_uint(addr + field_offset(workqueue_struct, flags), "flags");
    static const std::unordered_map<workqueue_struct_flags, std::string> str_flag_array = {
        {WQ_UNBOUND, "WQ_UNBOUND"},
        {WQ_FREEZABLE, "WQ_FREEZABLE"},
        {WQ_MEM_RECLAIM, "WQ_MEM_RECLAIM"},
        {WQ_HIGHPRI, "WQ_HIGHPRI"},
        {WQ_CPU_INTENSIVE, "WQ_CPU_INTENSIVE"},
        {WQ_SYSFS, "WQ_SYSFS"}
    };
    wq_ptr->flags = parser_flags(flags, str_flag_array);
    ulong pwqs_head_addr = addr + field_offset(workqueue_struct, pwqs);
    std::vector<ulong> pwq_list = for_each_list(pwqs_head_addr, field_offset(pool_workqueue, pwqs_node));
    for (const auto& pwq_addr : pwq_list){
        std::shared_ptr<pool_workqueue> pwq_ptr = parser_pool_workqueue(pwq_addr);
        pwq_ptr->wq_ptr = wq_ptr;
        wq_ptr->pwqs.push_back(pwq_ptr);
    }
    LOGI("workqueue_struct '%s' at 0x%lx (flags=%s, pwqs=%zu)",
         wq_ptr->name.c_str(), addr, wq_ptr->flags.c_str(), wq_ptr->pwqs.size());
    return wq_ptr;
}

void Workqueue::parse_cpu_worker_pools() {
    if (!csymbol_exists("cpu_worker_pools")) {
        LOGW("cpu_worker_pools symbol not found");
        return;
    }
    ulong cpu_worker_pools_addr = csymbol_value("cpu_worker_pools");
    ulong n_pools = read_enum_val("NR_STD_WORKER_POOLS");
    int worker_pool_size = struct_size(worker_pool);
    std::vector<ulong> percpu_list = for_each_percpu(cpu_worker_pools_addr);
    for (size_t i = 0; i < percpu_list.size(); i++){
        ulong cwp_addr = percpu_list[i];
        // Parse normal pool + high pool
        for (size_t j = 0; j < n_pools; j++) {
            ulong wq_addr = cwp_addr + j * worker_pool_size;

            // Check if already parsed
            if (worker_pool_map.find(wq_addr) != worker_pool_map.end()) {
                LOGD("CPU[%zu] pool#%zu already parsed, skipping", i, j);
                continue;
            }

            // Use existing parser_worker_pool to parse and fill data
            LOGD("Parsing CPU[%zu] pool#%zu at 0x%lx", i, j, wq_addr);
            auto wp_ptr = parser_worker_pool(wq_addr);
            worker_pool_map[wq_addr] = wp_ptr;
        }
    }
}

void Workqueue::parse_unbound_pool_hash() {
    if (!csymbol_exists("unbound_pool_hash")) {
        LOGW("unbound_pool_hash symbol not found");
        return;
    }

    ulong unbound_pool_hash = csymbol_value("unbound_pool_hash");
    int hash_entry_size = struct_size(hlist_head);
    size_t total_unbound_pools = 0;
    for (size_t hash_index = 0; hash_index < 64; hash_index++){
        ulong hlist_head = unbound_pool_hash + hash_index * hash_entry_size;
        for (auto& wp_addr : for_each_hlist(hlist_head,field_offset(worker_pool, hash_node))) {
            // Check if already parsed
            if (worker_pool_map.find(wp_addr) != worker_pool_map.end()) {
                LOGD("  Unbound pool 0x%lx already parsed, skipping", wp_addr);
                continue;
            }

            total_unbound_pools++;

            // Use existing parser_worker_pool to parse and fill data
            auto wp_ptr = parser_worker_pool(wp_addr);
            worker_pool_map[wp_addr] = wp_ptr;
        }
    }
}

void Workqueue::print_workqueue_detailed() {
    LOGD("Starting print_workqueue_detailed");
    // Traverse all workqueues and show details for active ones
    for (const auto& wq : workqueue_list) {
        if (has_activity(wq)) {
            show_workqueue_details(wq);
        }
    }
}

void Workqueue::print_worker_pool_detailed() {
    LOGD("Starting print_worker_pool_detailed");

    // Separate per-CPU and unbound pools
    std::map<int, std::vector<std::shared_ptr<worker_pool>>> cpu_pools;
    std::vector<std::shared_ptr<worker_pool>> unbound_pools;

    for (const auto& pair : worker_pool_map) {
        auto pool = pair.second;
        if (pool->cpu >= 0) {
            cpu_pools[pool->cpu].push_back(pool);
        } else {
            unbound_pools.push_back(pool);
        }
    }

    // Print summary first
    print_worker_pool_summary(cpu_pools, unbound_pools);

    // Display per-CPU pools
    for (const auto& cpu_pair : cpu_pools) {
        int cpu = cpu_pair.first;
        const auto& pools = cpu_pair.second;

        // Calculate CPU statistics
        size_t total_workers = 0, total_pending = 0;
        for (const auto& pool : pools) {
            total_workers += pool->nr_workers;
            total_pending += pool->worklist.size();
        }

        // Determine CPU status
        std::string cpu_status = "";
        if (total_pending > 100) {
            cpu_status = " [HIGH LOAD]";
        }

        PRINT("\nCPU[%d]: %zu pools, %zu workers, %zu pending works%s\n",
              cpu, pools.size(), total_workers, total_pending, cpu_status.c_str());

        // Display pools with tree structure
        for (size_t i = 0; i < pools.size(); ++i) {
            bool is_last = (i == pools.size() - 1);
            show_worker_pool_info(pools[i], is_last);
        }
    }

    // Display unbound pools
    if (!unbound_pools.empty()) {
        size_t total_workers = 0, total_pending = 0;
        for (const auto& pool : unbound_pools) {
            total_workers += pool->nr_workers;
            total_pending += pool->worklist.size();
        }

        PRINT("\nUnbound Pools: %zu pools, %zu workers, %zu pending works\n",
              unbound_pools.size(), total_workers, total_pending);

        for (size_t i = 0; i < unbound_pools.size(); ++i) {
            bool is_last = (i == unbound_pools.size() - 1);
            show_worker_pool_info(unbound_pools[i], is_last);
        }
    }
}

void Workqueue::parse_workqueue() {
    LOGD("Starting workqueue parsing");

    if (!workqueue_list.empty()) {
        LOGD("Workqueue list already populated (size=%zu)", workqueue_list.size());
        return;
    }
    // 1. Parse per-CPU worker pools
    parse_cpu_worker_pools();

    // 2. Parse unbound worker pools
    parse_unbound_pool_hash();

    // 3. Parse workqueues list
    if (csymbol_exists("workqueues")) {
        ulong workqueues_addr = csymbol_value("workqueues");
        LOGD("workqueues symbol address: 0x%lx", workqueues_addr);
        if (is_kvaddr(workqueues_addr)) {
            for (const auto& addr : for_each_list(workqueues_addr, field_offset(workqueue_struct, list))) {
                auto workqueue_struct = parser_workqueue_struct(addr);
                workqueue_list.push_back(workqueue_struct);
            }
        }
    }
    LOGI("Successfully parsed all workqueues (total=%zu, worker_pools=%zu, workers=%zu)",
         workqueue_list.size(), worker_pool_map.size(), worker_list_map.size());
}

void Workqueue::parse_busy_hash(std::shared_ptr<worker_pool> wp_ptr, ulong pool_addr) {
    LOGD("Parsing busy_hash for worker_pool at 0x%lx", pool_addr);

    // busy_hash is a hashtable with 64 buckets (BUSY_WORKER_HASH_ORDER = 6)
    const int BUSY_WORKER_HASH_SIZE = 64;

    ulong busy_hash_addr = pool_addr + field_offset(worker_pool, busy_hash);
    int hlist_head_size = struct_size(hlist_head);

    for (int bucket = 0; bucket < BUSY_WORKER_HASH_SIZE; bucket++) {
        ulong hlist_head_addr = busy_hash_addr + bucket * hlist_head_size;
        for (ulong worker_addr : for_each_hlist(hlist_head_addr, field_offset(worker, hentry))) {
            LOGD("  Found busy worker at 0x%lx in bucket %d", worker_addr, bucket);

            // Find the corresponding worker object in the already parsed workers
            auto worker_it = std::find_if(wp_ptr->workers.begin(), wp_ptr->workers.end(),
                [worker_addr](const std::shared_ptr<worker>& w) {
                    return w->addr == worker_addr;
                });

            if (worker_it != wp_ptr->workers.end()) {
                wp_ptr->busy_workers.push_back(*worker_it);
                LOGD("    Added busy worker: %s (pid=%d, current_func=0x%lx)",
                     (*worker_it)->comm.c_str(), (*worker_it)->pid, (*worker_it)->current_func);
            } else {
                LOGW("    Worker 0x%lx found in busy_hash but not in workers list", worker_addr);
            }
        }
    }

    LOGI("Parsed busy_hash: found %zu busy workers out of %d total workers",
         wp_ptr->busy_workers.size(), wp_ptr->nr_workers);
}

bool Workqueue::has_activity(std::shared_ptr<workqueue_struct> wq) {
    // Check if workqueue has any activity (busy workers, pending works, inactive works)
    for (const auto& pwq : wq->pwqs) {
        if (pwq->nr_active > 0 || !pwq->inactive_works.empty()) {
            return true;
        }
        if (pwq->wp_ptr) {
            if (!pwq->wp_ptr->busy_workers.empty() || !pwq->wp_ptr->worklist.empty()) {
                return true;
            }
        }
    }
    return false;
}

void Workqueue::show_workqueue_details(std::shared_ptr<workqueue_struct> wq) {
    PRINT("[WORKQUEUE] %s (%s)\n", wq->name.c_str(), wq->flags.c_str());

    // Group PWQs by worker_pool address to detect relationship type
    std::map<ulong, std::vector<std::shared_ptr<pool_workqueue>>> pool_groups;

    // Collect PWQs grouped by worker_pool
    for (const auto& pwq : wq->pwqs) {
        if (pwq->wp_ptr) {
            pool_groups[pwq->wp_ptr->addr].push_back(pwq);
        }
    }

    // Track displayed worker_pools and PWQs to avoid duplicates
    std::set<ulong> displayed_pools;
    std::set<ulong> displayed_pwqs;

    // Count items to display for proper tree formatting
    std::vector<std::pair<std::shared_ptr<pool_workqueue>, bool>> display_items; // pwq, is_multi_to_one

    for (const auto& pwq : wq->pwqs) {
        if (!pwq->wp_ptr) continue;

        // Skip if this PWQ has already been processed
        if (displayed_pwqs.find(pwq->addr) != displayed_pwqs.end()) {
            continue;
        }

        auto pool = pwq->wp_ptr;

        // Check if this is a multi-to-one relationship
        if (pool_groups[pool->addr].size() > 1) {
            // Multi-to-one: Only add the first PWQ of this group
            if (displayed_pools.find(pool->addr) == displayed_pools.end()) {
                display_items.push_back({pwq, true});
                displayed_pools.insert(pool->addr);
                displayed_pwqs.insert(pwq->addr);
            }
        } else {
            // One-to-one: Add this PWQ
            display_items.push_back({pwq, false});
            displayed_pwqs.insert(pwq->addr);
        }
    }

    // Reset displayed_pools for actual display
    displayed_pools.clear();

    // Display information based on relationship type
    for (size_t item_idx = 0; item_idx < display_items.size(); ++item_idx) {
        auto& item = display_items[item_idx];
        auto pwq = item.first;
        bool is_multi_to_one = item.second;

        bool is_last_item = (item_idx == display_items.size() - 1);
        const char* pwq_prefix = is_last_item ? "  └─" : "  ├─";

        auto pool = pwq->wp_ptr;
        const char* pool_type = (pool->nice == 0) ? "Normal" : "High";

        if (is_multi_to_one) {
            // Multi-to-one: Use grouped display
            PRINT("%s PWQs:\n", pwq_prefix);
            for (size_t i = 0; i < pool_groups[pool->addr].size(); ++i) {
                PRINT("  │     pwq:0x%lx\n", pool_groups[pool->addr][i]->addr);
            }

            // Display worker_pool details
            PRINT("  └─ worker_pool[0x%lx] CPU:%-2d %-6s (workers:%-2d, idle:%-2d, busy:%-2zu, pending:%zu)\n",
                  pool->addr, pool->cpu, pool_type,
                  pool->nr_workers, pool->nr_idle, pool->busy_workers.size(), pool->worklist.size());

            // Show worker_pool details with appropriate indentation
            show_worker_pool_details(pool, true);
        } else {
            // One-to-one: Use original format
            PRINT("%s pwq:0x%-16lx → worker_pool[0x%-16lx] CPU:%-2d %-6s (workers:%-2d, idle:%-2d, busy:%-2zu, pending:%zu)\n",
                  pwq_prefix, pwq->addr, pool->addr, pool->cpu, pool_type,
                  pool->nr_workers, pool->nr_idle, pool->busy_workers.size(), pool->worklist.size());

            // Check if this worker_pool has any activity
            bool has_activity = !pool->busy_workers.empty() || !pool->worklist.empty();

            // Check for inactive works
            size_t total_inactive = 0;
            for (const auto& pwq_check : pool->pwq_list) {
                total_inactive += pwq_check->inactive_works.size();
            }
            if (total_inactive > 0) has_activity = true;

            if (has_activity) {
                // Show details with proper indentation (aligned to worker_pool address)
                show_pwq_details(pwq, pwq_prefix, is_last_item);
            }
        }
    }
    PRINT("\n");
}

void Workqueue::show_worker_pool_details(std::shared_ptr<worker_pool> pool, bool is_last_group) {
    // Prepare indentation for worker_pool sub-items
    const char* pool_indent = is_last_group ? "     " : "  │  ";

    // Show busy workers
    if (!pool->busy_workers.empty()) {
        bool has_pending = !pool->worklist.empty();
        bool has_inactive = false;

        // Check if any PWQ has inactive works
        for (const auto& pwq : pool->pwq_list) {
            if (!pwq->inactive_works.empty()) {
                has_inactive = true;
                break;
            }
        }

        const char* busy_prefix = (has_pending || has_inactive) ? "├─" : "└─";
        PRINT("%s%s BUSY WORKERS:\n", pool_indent, busy_prefix);

        for (size_t i = 0; i < pool->busy_workers.size(); ++i) {
            const auto& worker = pool->busy_workers[i];
            bool is_last_worker = (i == pool->busy_workers.size() - 1);
            const char* worker_prefix = is_last_worker ? "└─" : "├─";

            std::string func_name = "None";
            if (is_kvaddr(worker->current_func)) {
                func_name = to_symbol(worker->current_func);
            }

            PRINT("%s│  %s %-16s [pid:%-6d] → %s\n",
                  pool_indent, worker_prefix, worker->comm.c_str(), worker->pid, func_name.c_str());
        }
    }

    // Show pending works
    if (!pool->worklist.empty()) {
        // Count work functions
        std::map<std::string, int> func_count;
        for (const auto& work : pool->worklist) {
            if (is_kvaddr(work->func)) {
                std::string func_name = to_symbol(work->func);
                func_count[func_name]++;
            }
        }

        // Check if any PWQ has inactive works
        bool has_inactive = false;
        for (const auto& pwq : pool->pwq_list) {
            if (!pwq->inactive_works.empty()) {
                has_inactive = true;
                break;
            }
        }

        const char* pending_prefix = has_inactive ? "├─" : "└─";
        PRINT("%s%s PENDING WORKS: (%zu items)\n", pool_indent, pending_prefix, pool->worklist.size());

        size_t func_idx = 0;
        for (const auto& pair : func_count) {
            bool is_last_func = (func_idx == func_count.size() - 1);
            const char* func_prefix = is_last_func ? "└─" : "├─";

            if (pair.second > 1) {
                PRINT("%s│  %s %s (×%d)\n", pool_indent, func_prefix, pair.first.c_str(), pair.second);
            } else {
                PRINT("%s│  %s %s\n", pool_indent, func_prefix, pair.first.c_str());
            }
            func_idx++;
        }
    }

    // Show inactive works (collect from all PWQs pointing to this pool)
    std::map<std::string, int> inactive_func_count;
    size_t total_inactive = 0;

    for (const auto& pwq : pool->pwq_list) {
        for (const auto& work : pwq->inactive_works) {
            if (is_kvaddr(work->func)) {
                std::string func_name = to_symbol(work->func);
                inactive_func_count[func_name]++;
                total_inactive++;
            }
        }
    }

    if (total_inactive > 0) {
        PRINT("%s└─ INACTIVE WORKS: (%zu items)\n", pool_indent, total_inactive);

        size_t func_idx = 0;
        for (const auto& pair : inactive_func_count) {
            bool is_last_func = (func_idx == inactive_func_count.size() - 1);
            const char* func_prefix = is_last_func ? "└─" : "├─";

            if (pair.second > 1) {
                PRINT("%s   %s %s (×%d)\n", pool_indent, func_prefix, pair.first.c_str(), pair.second);
            } else {
                PRINT("%s   %s %s\n", pool_indent, func_prefix, pair.first.c_str());
            }
            func_idx++;
        }
    }
}


void Workqueue::show_pwq_details(std::shared_ptr<pool_workqueue> pwq, const char* prefix, bool is_last_pwq) {
    auto pool = pwq->wp_ptr;
    if (!pool) {
        return;
    }
    // Prepare indentation for worker_pool sub-items (deeper level)
    const char* pool_indent = is_last_pwq ? "                                " : "  │                             ";

    // Show busy workers
    if (!pool->busy_workers.empty()) {
        PRINT("%s├─ BUSY WORKERS:\n", pool_indent);
        for (size_t i = 0; i < pool->busy_workers.size(); ++i) {
            const auto& worker = pool->busy_workers[i];
            bool is_last_worker = (i == pool->busy_workers.size() - 1);
            const char* worker_prefix = is_last_worker ? "└─" : "├─";

            std::string func_name = "None";
            if (is_kvaddr(worker->current_func)) {
                func_name = to_symbol(worker->current_func);
            }

            PRINT("%s│  %s %-16s [pid:%-6d] → %s\n",
                  pool_indent, worker_prefix, worker->comm.c_str(), worker->pid, func_name.c_str());
        }
    }

    // Show pending works
    if (!pool->worklist.empty()) {
        // Count work functions
        std::map<std::string, int> func_count;
        for (const auto& work : pool->worklist) {
            if (is_kvaddr(work->func)) {
                std::string func_name = to_symbol(work->func);
                func_count[func_name]++;
            }
        }
        bool has_inactive = !pwq->inactive_works.empty();
        const char* pending_prefix = (has_inactive) ? "├─" : "└─";

        PRINT("%s%s PENDING WORKS: (%zu items)\n", pool_indent, pending_prefix, pool->worklist.size());

        size_t func_idx = 0;
        for (const auto& pair : func_count) {
            bool is_last_func = (func_idx == func_count.size() - 1);
            const char* func_prefix = is_last_func ? "└─" : "├─";

            if (pair.second > 1) {
                PRINT("%s│  %s %s (×%d)\n", pool_indent, func_prefix, pair.first.c_str(), pair.second);
            } else {
                PRINT("%s│  %s %s\n", pool_indent, func_prefix, pair.first.c_str());
            }
            func_idx++;
        }
    }

    // Show inactive works
    if (!pwq->inactive_works.empty()) {
        // Count work functions
        std::map<std::string, int> func_count;
        for (const auto& work : pwq->inactive_works) {
            if (is_kvaddr(work->func)) {
                std::string func_name = to_symbol(work->func);
                func_count[func_name]++;
            }
        }

        PRINT("%s└─ INACTIVE WORKS: (%zu items)\n", pool_indent, pwq->inactive_works.size());

        size_t func_idx = 0;
        for (const auto& pair : func_count) {
            bool is_last_func = (func_idx == func_count.size() - 1);
            const char* func_prefix = is_last_func ? "└─" : "├─";

            if (pair.second > 1) {
                PRINT("%s   %s %s (×%d)\n", pool_indent, func_prefix, pair.first.c_str(), pair.second);
            } else {
                PRINT("%s   %s %s\n", pool_indent, func_prefix, pair.first.c_str());
            }
            func_idx++;
        }
    }
}

void Workqueue::print_worker_pool_summary(const std::map<int, std::vector<std::shared_ptr<worker_pool>>>& cpu_pools,
                                          const std::vector<std::shared_ptr<worker_pool>>& unbound_pools) {
    // Calculate statistics
    size_t total_pools = 0;
    size_t total_workers = 0;
    size_t total_pending = 0;
    size_t cpu_normal_pools = 0;
    size_t cpu_high_pools = 0;
    size_t unbound_normal_pools = 0;
    size_t unbound_high_pools = 0;

    std::vector<std::string> critical_warnings;
    // Process CPU pools
    for (const auto& cpu_pair : cpu_pools) {
        int cpu = cpu_pair.first;
        const auto& pools = cpu_pair.second;
        size_t cpu_pending = 0;
        size_t hung_pools = 0;

        for (const auto& pool : pools) {
            total_pools++;
            total_workers += pool->nr_workers;
            total_pending += pool->worklist.size();
            cpu_pending += pool->worklist.size();

            // Count pool types
            if (pool->nice == 0) {
                cpu_normal_pools++;
            } else {
                cpu_high_pools++;
            }

            // Check for hung pools
            if (!pool->worklist.empty()) {
                ulong current_jiffies = read_ulong(csymbol_value("jiffies"), "jiffies");
                unsigned long hung = jiffies_to_msecs(current_jiffies - pool->watchdog_ts) / 1000;
                if (hung > 10) {
                    hung_pools++;
                }
            }
        }

        // Check CPU load
        if (cpu_pending > 100) {
            critical_warnings.push_back("CPU[" + std::to_string(cpu) + "]: High load detected (" +
                                       std::to_string(cpu_pending) + " pending works)");
        }

        if (hung_pools > 0) {
            critical_warnings.push_back(std::to_string(hung_pools) + " pools hung (>10s) on CPU[" + std::to_string(cpu) + "]");
        }
    }

    // Process unbound pools
    for (const auto& pool : unbound_pools) {
        total_pools++;
        total_workers += pool->nr_workers;
        total_pending += pool->worklist.size();

        // Count unbound pool types (based on flags or other criteria)
        if (pool->flags.find("WQ_HIGHPRI") != std::string::npos) {
            unbound_high_pools++;
        } else {
            unbound_normal_pools++;
        }
    }

    // Print summary
    PRINT("===============================================================================\n");
    PRINT("                           WORKER POOL SUMMARY\n");
    PRINT("===============================================================================\n");

    PRINT("System Overview:\n");
    PRINT("  CPUs: %-10zu Pools: %-8zu Workers: %-8zu Pending: %zu\n\n",
          cpu_pools.size(), total_pools, total_workers, total_pending);

    PRINT("Pool Distribution:\n");
    PRINT("  Per-CPU Pools: %zu (%zu Normal, %zu High Priority)\n",
          cpu_normal_pools + cpu_high_pools, cpu_normal_pools, cpu_high_pools);
    PRINT("  Unbound Pools: %zu (%zu Normal, %zu High Priority)\n\n",
          unbound_normal_pools + unbound_high_pools, unbound_normal_pools, unbound_high_pools);

    // Print warnings
    bool has_warnings = !critical_warnings.empty();
    if (has_warnings) {
        PRINT("Warnings:\n");

        for (const auto& warning : critical_warnings) {
            PRINT("  [CRITICAL] %s\n", warning.c_str());
        }
    }
    PRINT("===============================================================================");
}

void Workqueue::show_worker_pool_info(std::shared_ptr<worker_pool> pool, bool is_last) {
    // Determine pool type
    const char* pool_type;
    if (pool->cpu >= 0) {
        pool_type = (pool->nice == 0) ? "Normal" : "High";
    } else {
        pool_type = "Unbound";
    }

    // Calculate hung time
    unsigned long hung = 0;
    if (!pool->worklist.empty()) {
        ulong current_jiffies = read_ulong(csymbol_value("jiffies"), "jiffies");
        hung = jiffies_to_msecs(current_jiffies - pool->watchdog_ts) / 1000;
    }

    // Tree structure prefix
    const char* tree_prefix = is_last ? "└─" : "├─";
    const char* indent = is_last ? "   " : "│  ";

    // Print main pool line
    PRINT("%s worker_pool:0x%lx %s (workers:%d, idle:%d, busy:%zu, pending:%zu)",
          tree_prefix, pool->addr, pool_type, pool->nr_workers, pool->nr_idle,
          pool->busy_workers.size(), pool->worklist.size());

    if (hung > 0) {
        PRINT(" hung=%lus", hung);
    }
    PRINT("\n");

    // Print status and flags
    PRINT("%s├─ Status: ", indent);
    if (hung > 60) {
        PRINT("Critical");
    } else if (hung > 10) {
        PRINT("Blocked");
    } else if (pool->busy_workers.size() > pool->nr_workers * 0.7) {
        PRINT("High Load");
    } else {
        PRINT("Normal");
    }
    PRINT(" | Flags: %s\n", pool->flags.c_str());

    // Count sections to display
    bool has_manager = (pool->manager != nullptr);
    bool has_idle = !pool->idle_list.empty();
    bool has_busy = !pool->busy_workers.empty();
    bool has_pending = !pool->worklist.empty();

    int sections = (has_manager ? 1 : 0) + (has_idle ? 1 : 0) + (has_busy ? 1 : 0) + (has_pending ? 1 : 0);
    int current_section = 0;

    // Display manager worker
    if (has_manager) {
        current_section++;
        bool is_last_section = (current_section == sections);
        const char* section_prefix = is_last_section ? "└─" : "├─";

        PRINT("%s%s Manager Worker:\n", indent, section_prefix);
        const auto& manager = pool->manager;

        if (manager->pid > 0 && !manager->comm.empty()) {
            PRINT("%s│  └─ %s (pid:%d)\n", indent, manager->comm.c_str(), manager->pid);
        } else {
            PRINT("%s│  └─ worker:0x%lx\n", indent, manager->addr);
        }
    }

    // Display idle workers
    if (has_idle) {
        current_section++;
        bool is_last_section = (current_section == sections);
        const char* section_prefix = is_last_section ? "└─" : "├─";

        PRINT("%s%s Idle Workers:\n", indent, section_prefix);
        for (size_t i = 0; i < pool->idle_list.size(); ++i) {
            const auto& worker = pool->idle_list[i];
            bool is_last_worker = (i == pool->idle_list.size() - 1);
            const char* worker_prefix = is_last_worker ? "└─" : "├─";

            if (worker->pid > 0 && !worker->comm.empty()) {
                PRINT("%s│  %s %s (pid:%d)\n", indent, worker_prefix, worker->comm.c_str(), worker->pid);
            } else {
                PRINT("%s│  %s worker:0x%lx\n", indent, worker_prefix, worker->addr);
            }
        }
    }

    // Display busy workers
    if (has_busy) {
        current_section++;
        bool is_last_section = (current_section == sections);
        const char* section_prefix = is_last_section ? "└─" : "├─";

        PRINT("%s%s Busy Workers:\n", indent, section_prefix);
        for (size_t i = 0; i < pool->busy_workers.size(); ++i) {
            const auto& worker = pool->busy_workers[i];
            bool is_last_worker = (i == pool->busy_workers.size() - 1);
            const char* worker_prefix = is_last_worker ? "└─" : "├─";

            std::string func_name = "unknown";
            if (is_kvaddr(worker->current_func)) {
                func_name = to_symbol(worker->current_func);
            }

            if (worker->pid > 0 && !worker->comm.empty()) {
                PRINT("%s│  %s %s (pid:%d) → %s\n", indent, worker_prefix,
                      worker->comm.c_str(), worker->pid, func_name.c_str());
            } else {
                PRINT("%s│  %s worker:0x%lx → %s\n", indent, worker_prefix,
                      worker->addr, func_name.c_str());
            }
        }
    }

    // Display pending works
    if (has_pending) {
        current_section++;
        bool is_last_section = (current_section == sections);
        const char* section_prefix = is_last_section ? "└─" : "├─";

        PRINT("%s%s Pending Works:\n", indent, section_prefix);

        // Count work functions
        std::map<std::string, int> func_count;
        for (const auto& work : pool->worklist) {
            if (is_kvaddr(work->func)) {
                std::string func_name = to_symbol(work->func);
                func_count[func_name]++;
            }
        }

        size_t func_idx = 0;
        for (const auto& pair : func_count) {
            bool is_last_func = (func_idx == func_count.size() - 1);
            const char* func_prefix = is_last_func ? "└─" : "├─";

            if (pair.second > 1) {
                PRINT("%s│  %s %s (×%d)\n", indent, func_prefix, pair.first.c_str(), pair.second);
            } else {
                PRINT("%s│  %s %s\n", indent, func_prefix, pair.first.c_str());
            }
            func_idx++;
        }
    }
}

#pragma GCC diagnostic pop
