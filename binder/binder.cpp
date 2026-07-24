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

#include "binder.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(Binder)
#endif

/**
 * Main command entry point for binder plugin
 * Parses command line arguments and dispatches to appropriate handler
 */
void Binder::cmd_main(void) {
    // Validate minimum argument count
    if (argcnt < 2) {
        LOGE("Insufficient arguments\n");
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }
    // Initialize binder argument structure
    struct binder_argument_t binder_arg;
    BZERO(&binder_arg, sizeof(binder_arg));
    // Parse command line options
    int c;
    while ((c = getopt(argcnt, args, "tnrbflap:")) != EOF) {
        switch(c) {
            case 'a':
                // Dump all binder processes with full information
                LOGD("Dumping all binder processes\n");
                binder_arg.dump_all = 1;
                binder_arg.flags = BINDER_THREAD | BINDER_NODE | BINDER_REF | BINDER_ALLOC;
                binder_proc_show(&binder_arg);
                return;
            case 'p':
                // Dump specific process by PID
                binder_arg.pid = atoi(optarg);
                LOGD("Dumping binder info for PID: %d\n", binder_arg.pid);
                binder_arg.flags = BINDER_THREAD | BINDER_NODE | BINDER_REF | BINDER_ALLOC;
                binder_proc_show(&binder_arg);
                return;
            case 'f':
                // Print failed transaction log
                LOGD("Printing failed transaction log\n");
                print_binder_transaction_log_entry(true);
                return;
            case 'l':
                // Print normal transaction log
                LOGD("Printing transaction log\n");
                print_binder_transaction_log_entry(false);
                return;
            case 't':
                // Dump thread information only
                LOGD("Dumping thread information\n");
                binder_arg.dump_all = 1;
                binder_arg.flags = BINDER_THREAD;
                binder_proc_show(&binder_arg);
                return;
            case 'n':
                // Dump node information only
                LOGD("Dumping node information\n");
                binder_arg.dump_all = 1;
                binder_arg.flags = BINDER_NODE;
                binder_proc_show(&binder_arg);
                return;
            case 'r':
                // Dump reference information only
                LOGD("Dumping reference information\n");
                binder_arg.dump_all = 1;
                binder_arg.flags = BINDER_REF;
                binder_proc_show(&binder_arg);
                return;
            case 'b':
                // Dump buffer allocation information only
                LOGD("Dumping buffer allocation information\n");
                binder_arg.dump_all = 1;
                binder_arg.flags = BINDER_ALLOC;
                binder_proc_show(&binder_arg);
                return;
            default:
                LOGE("Invalid option '%c'\n", c);
                cmd_usage(pc->curcmd, SYNOPSIS);
                return;
        }
    }
}

/**
 * Initialize field offsets for binder structures
 * This must be called before accessing any binder kernel structures
 */
void Binder::init_offset(void) {
    // Initialize binder_proc structure offsets
    struct_init(binder_proc);
    field_init(binder_proc, proc_node);
    field_init(binder_proc, context);
    field_init(binder_proc, max_threads);
    field_init(binder_proc, threads);
    field_init(binder_proc, nodes);
    field_init(binder_proc, refs_by_desc);
    field_init(binder_proc, alloc);
    field_init(binder_proc, todo);
    field_init(binder_proc, pid);
    field_init(binder_proc, tsk);
    field_init(binder_proc, waiting_threads);
    field_init(binder_proc, is_dead);
    field_init(binder_proc, is_frozen);
    field_init(binder_proc, sync_recv);
    field_init(binder_proc, async_recv);
    field_init(binder_proc, requested_threads);
    field_init(binder_proc, requested_threads_started);

    // Initialize binder_thread structure offsets
    struct_init(binder_thread);
    field_init(binder_thread, proc);
    field_init(binder_thread, pid);
    field_init(binder_thread, looper);
    field_init(binder_thread, looper_need_return);
    field_init(binder_thread, transaction_stack);
    field_init(binder_thread, is_dead);
    field_init(binder_thread, rb_node);
    field_init(binder_thread, tmp_ref);
    field_init(binder_thread, waiting_thread_node);

    // Initialize binder_transaction structure offsets
    struct_init(binder_transaction);
    field_init(binder_transaction, work);
    field_init(binder_transaction, from);
    field_init(binder_transaction, from_parent);
    field_init(binder_transaction, to_parent);
    field_init(binder_transaction, to_thread);
    field_init(binder_transaction, to_proc);
    field_init(binder_transaction, buffer);
    field_init(binder_transaction, debug_id);
    field_init(binder_transaction, code);
    field_init(binder_transaction, flags);
    field_init(binder_transaction, need_reply);
    field_init(binder_transaction, priority);

    // Initialize binder_node structure offsets
    struct_init(binder_node);
    field_init(binder_node, debug_id);
    field_init(binder_node, work);
    field_init(binder_node, rb_node);
    field_init(binder_node, async_todo);
    field_init(binder_node, refs);
    field_init(binder_node, proc);
    field_init(binder_node, ptr);
    field_init(binder_node, cookie);
    field_init(binder_node, internal_strong_refs);
    field_init(binder_node, has_strong_ref);
    field_init(binder_node, sched_policy);
    field_init(binder_node, local_weak_refs);
    field_init(binder_node, local_strong_refs);
    field_init(binder_node, tmp_refs);
    field_init(binder_node, min_priority);

    // Initialize binder_ref structure offsets
    struct_init(binder_ref);
    field_init(binder_ref, data);
    field_init(binder_ref, node);
    field_init(binder_ref, death);
    field_init(binder_ref, rb_node_desc);
    field_init(binder_ref, proc);
    field_init(binder_ref, node_entry);

    // Initialize binder_ref_data structure offsets
    field_init(binder_ref_data, debug_id);
    field_init(binder_ref_data, desc);
    field_init(binder_ref_data, strong);
    field_init(binder_ref_data, weak);

    // Initialize binder_buffer structure offsets
    struct_init(binder_buffer);
    field_init(binder_buffer, rb_node);
    field_init(binder_buffer, free);
    field_init(binder_buffer, user_data);
    field_init(binder_buffer, data_size);
    field_init(binder_buffer, offsets_size);
    field_init(binder_buffer, extra_buffers_size);
    field_init(binder_buffer, pid);
    field_init(binder_buffer, transaction);

    // Initialize binder_alloc structure offsets
    struct_init(binder_alloc);
    field_init(binder_alloc, vma);
    field_init(binder_alloc, free_buffers);
    field_init(binder_alloc, vma_vm_mm);
    field_init(binder_alloc, buffer);
    field_init(binder_alloc, buffer_size);
    field_init(binder_alloc, free_async_space);
    field_init(binder_alloc, pages);
    field_init(binder_alloc, allocated_buffers);
    // Initialize binder_work structure offsets
    struct_init(binder_work);
    field_init(binder_work, entry);

    // Initialize binder_priority structure offsets
    field_init(binder_priority, prio);
    field_init(binder_priority, sched_policy);

    // Initialize common kernel structure offsets
    field_init(rb_root, rb_node);
    field_init(list_head, next);
    field_init(binder_context, name);
    field_init(task_struct, comm);

    // Initialize remaining structure sizes
    struct_init(binder_lru_page);
}

void Binder::init_command(void) {
    cmd_name = "binder";
    help_str_list={
        "binder",                            /* command name */
        "dump binder log information",        /* short description */
        "[-a] [-p pid] [-l] [-f] [-n] [-b] [-t] [-r]\n"
            "\n"
            "  -a         display all binder proc states\n"
            "  -p pid     display specific process binder states\n"
            "  -l         display binder transaction log\n"
            "  -f         display binder fail log\n"
            "  -n         display binder node info\n"
            "  -b         display binder buffer info\n"
            "  -t         display binder thread info\n"
            "  -r         display binder ref info\n",
        "\n",
        "EXAMPLES",
        "  Display all binder proc states:",
        "    %s> binder -a",
        "       proc 7312",
        "       context hwbinder",
        "           thread 7335: l 00 need_return 0 tr 0",
        "\n",
        "  Display specific process binder states:",
        "    %s> binder -p 7312",
        "       proc 7312",
        "       context hwbinder",
        "           thread 7335: l 00 need_return 0 tr 0",
        "\n",
        "  Display binder transaction log:",
        "    %s> binder -l",
        "       409173: reply from 1239:5444 to 5301:5301 context binder node 0 handle -1 size 8:0 ret 0/0 l=0",
        "       409174: call  from 666:666 to 614:0 context hwbinder node 1171 handle 4 size 220:32 ret 0/0 l=0",
        "\n",
        "  Display binder fail log:",
        "    %s> binder -f",
        "       840   : call  from 616:616 to 0:0 context hwbinder node 738 handle 1 size 32:0 ret 29201/-1 l=2793",
        "       210296: reply from 1239:2118 to 0:0 context binder node 0 handle -1 size 8:0 ret 29189/0 l=2717" ,
        "\n",
        "  Display binder thread info:",
        "    %s> binder -t",
        "       binder_proc:0xea194000 viders.calendar [7340] binder dead:0 frozen:0 sr:0 ar:0 max:15 total:6 requested:0 started:4 ready:5",
        "         binder_thread:0xef071e00 pid:7358 loop:12 need_return:0",
        "         binder_thread:0xea194600 pid:7360 loop:11 need_return:0",
        "\n",
        "  Display binder node info:",
        "    %s> binder -n",
        "       binder_proc:0xea194000 viders.calendar [7340] binder dead:0 frozen:0 sr:0 ar:0 max:15 total:6 requested:0 started:4 ready:5",
        "         binder_node:0xebb46680 id:407676 ptr:0xa7fe1460 cookie:0xac7c9278 pri:0[139] hs:1 hw:1 ls:0 lw:0 is:1 iw:0 tr:0",
        "         binder_node:0xebeed500 id:407473 ptr:0xa7fe13e0 cookie:0xac7c91b0 pri:0[139] hs:1 hw:1 ls:0 lw:0 is:2 iw:0 tr:0",
        "\n",
        "  Display binder ref info:",
        "    %s> binder -r",
        "       binder_proc:0xea194000 viders.calendar [7340] binder dead:0 frozen:0 sr:0 ar:0 max:15 total:6 requested:0 started:4 ready:5",
        "         binder_ref:0xe8407e80 id:406535 desc:15 s:1 w:1 death:0x0 -> node_id:9291 binder_proc:0xebf3ba00 Binder:1239_3[1239]",
        "         binder_ref:0xdec2e640 id:406527 desc:7 s:1 w:1 death:0x0 -> node_id:9540 binder_proc:0xebf3ba00 Binder:1239_3[1239]",
        "\n",
        "  Display binder buffer info:",
        "    %s> binder -b",
        "       binder_proc:0xea194000 viders.calendar [7340] binder dead:0 frozen:0 sr:0 ar:0 max:15 total:6 requested:0 started:4 ready:5",
        "         binder_alloc:0xea194154 mm_struct:0xe681df80 vma:0xefa9d880 buffer:0x81a39000 size:1040384 free:520192",
        "           Page :0xf815f28c PA:0xa664b000",
        "           Page :0xf7d338f8 PA:0x88bce000",
        "           Free binder_buffer :0xdb035300 id:409018 data:0x81a390e0 PA:0xa664b0e0 size:296 offset:24 extra:0 pid:666 delivered",
        "           Alloc binder_buffer:0xdd8f62c0 id:408034 data:0x81a390d8 PA:0xa664b0d8 size:8 offset:0 extra:0 pid:1239 delivered",
        "\n",
    };
}

Binder::Binder(){}

/**
 * Print binder transaction log entries
 * @param fail_log: true for failed transactions, false for all transactions
 */
void Binder::print_binder_transaction_log_entry(bool fail_log){
    const char* symbol_name = fail_log ? "binder_transaction_log_failed" : "binder_transaction_log";
    // Check if the symbol exists in kernel
    if (!csymbol_exists(symbol_name)) {
        LOGE("%s doesn't exist in this kernel!\n", symbol_name);
        return;
    }

    // Get symbol address
    ulong btl_addr = csymbol_value(symbol_name);
    if (!is_kvaddr(btl_addr)) {
        LOGE("Invalid kernel address for %s\n", symbol_name);
        return;
    }

    // Read transaction log structure
    struct binder_transaction_log btl;
    if (!read_struct(btl_addr, &btl, sizeof(btl), "binder_transaction_log")) {
        LOGE("Failed to read transaction log structure\n");
        return;
    }

    // Determine number of valid entries
    int count = btl.full ? 32 : btl.cur;
    LOGD("Processing %d transaction log entries (full=%d, cur=%d)\n",
            count, btl.full, btl.cur);

    for (int i = 0; i < count; i++) {
        const struct binder_transaction_log_entry& log_entry = btl.entry[i];
        // Skip invalid entries
        if (log_entry.debug_id <= 0) continue;

        // Decode call type
        const char* call_type_str = (log_entry.call_type == 2) ? "reply" :
                                   (log_entry.call_type == 1) ? "async" : "call ";

        // Print transaction log entry with full details
        PRINT("%-6d: %s from %d:%d to %d:%d context %s node %d handle %d size %d:%d ret %d/%d l=%d\n",
            log_entry.debug_id, call_type_str, log_entry.from_proc,
            log_entry.from_thread, log_entry.to_proc, log_entry.to_thread, log_entry.context_name,
            log_entry.to_node, log_entry.target_handle, log_entry.data_size, log_entry.offsets_size,
            log_entry.return_error, log_entry.return_error_param,
            log_entry.return_error_line);
    }
}

/**
 * Show binder process information based on filter criteria
 * @param binder_arg: Filter and display options
 */
void Binder::binder_proc_show(struct binder_argument_t* binder_arg) {
    // Verify binder_procs symbol exists
    if (!csymbol_exists("binder_procs")){
        LOGE("binder_procs doesn't exist in this kernel!\n");
        return;
    }

    // Get binder process list head
    ulong binder_procs = csymbol_value("binder_procs");
    if (!is_kvaddr(binder_procs)) {
        LOGE("Invalid address for binder_procs\n");
        return;
    }

    // Traverse the hash list of binder processes
    const int offset = field_offset(binder_proc, proc_node);
    const std::vector<ulong> proc_list = for_each_hlist(binder_procs, offset);

    LOGD("Found %zu binder processes\n", proc_list.size());

    int processed_count = 0;
    for (const auto& proc_addr : proc_list) {
        void *binder_proc_buf = read_struct(proc_addr, "binder_proc");
        if (!binder_proc_buf) {
            LOGE("Failed to read binder_proc structure at address %lx\n", proc_addr);
            return;
        }
        int pid = INT(binder_proc_buf + field_offset(binder_proc, pid));
        FREEBUF(binder_proc_buf);

        // Filter by PID if specified
        if (binder_arg->dump_all || binder_arg->pid == pid) {
            LOGD("Processing binder_proc PID=%d at %#lx\n",pid, proc_addr);
            print_binder_proc(proc_addr, binder_arg->flags);
            processed_count++;
        }
    }
}

/**
 * Print binder buffer allocation information
 * @param tc: Task context for virtual to physical address translation
 * @param alloc_addr: Address of binder_alloc structure
 */
void Binder::print_binder_alloc(struct task_context *tc, ulong alloc_addr) {
    if (!is_kvaddr(alloc_addr)) {
        LOGE("Invalid alloc address %#lx\n", alloc_addr);
        return;
    }

    void *binder_alloc_buf = read_struct(alloc_addr, "binder_alloc");
    if (!binder_alloc_buf) {
        LOGE("Failed to read binder_alloc structure at address %lx\n", alloc_addr);
        return;
    }

    // Read all fields at once for better cache locality
    ulong vm_mm = ULONG(binder_alloc_buf + field_offset(binder_alloc, vma_vm_mm));
    ulong vma = ULONG(binder_alloc_buf + field_offset(binder_alloc, vma));
    ulong buffer = ULONG(binder_alloc_buf + field_offset(binder_alloc, buffer));
    ulong pages = ULONG(binder_alloc_buf + field_offset(binder_alloc, pages));
    ulong buffer_size = ULONG(binder_alloc_buf + field_offset(binder_alloc, buffer_size));
    ulong free_async_space = ULONG(binder_alloc_buf + field_offset(binder_alloc, free_async_space));
    ulong free_buffers_rb_node = ULONG(binder_alloc_buf + field_offset(binder_alloc, free_buffers) + field_offset(rb_root, rb_node));
    ulong allocated_buffers_rb_node = ULONG(binder_alloc_buf + field_offset(binder_alloc, allocated_buffers) + field_offset(rb_root, rb_node));

    LOGD("binder_alloc at %lx: vma_vm_mm=%lx, vma=%lx, buffer=%lx, buffer_size=%lu, free_async_space=%lu\n",
        alloc_addr, vm_mm, vma, buffer, buffer_size, free_async_space);
    FREEBUF(binder_alloc_buf);

    PRINT("  binder_alloc:%#lx mm_struct:%p vma:%p buffer:%p size:%d free:%d\n",
            alloc_addr, vm_mm, vma, buffer, buffer_size, free_async_space);

    // Process pages only if valid
    if (pages && buffer_size > 0) {
        const int nr_pages = buffer_size / PAGESIZE();
        const size_t lru_page_size = struct_size(binder_lru_page);
        LOGD("Processing %d pages (page_size=%lu)\n", nr_pages, PAGESIZE());

        for (int i = 0; i < nr_pages; ++i) {
            const ulong lru_page_addr = pages + i * lru_page_size;
            struct binder_lru_page lru_page;
            if (!read_struct(lru_page_addr, &lru_page, sizeof(lru_page), "binder_lru_page")) {
                LOGE("Failed to read lru_page at index %d\n", i);
                continue;
            }
            const ulong page_addr = (ulong)lru_page.page_ptr;
            if (is_kvaddr(page_addr)) {
                const physaddr_t paddr = page_to_phy(page_addr);
                PRINT("    Page :%#lx PA:%#llx\n", page_addr, (ulonglong)paddr);
            }
        }
    }

    // Helper lambda for printing buffer information (free or allocated)
    auto print_buffer = [&](const char* type, ulong buffer_addr) {
        void *binder_buffer_buf = read_struct(buffer_addr, "binder_buffer");
        if (!binder_buffer_buf) {
            LOGE("Failed to read binder_buffer structure at address %lx\n", buffer_addr);
            return;
        }

        // Read all buffer fields at once
        unsigned int flags = UINT(binder_buffer_buf + field_offset(binder_buffer, free));
        unsigned int debug_id = (flags >> 5) & 0x7FFFFFF;
        ulong user_data = ULONG(binder_buffer_buf + field_offset(binder_buffer, user_data));
        ulong transaction = ULONG(binder_buffer_buf + field_offset(binder_buffer, transaction));
        ulong data_size = ULONG(binder_buffer_buf + field_offset(binder_buffer, data_size));
        ulong offsets_size = ULONG(binder_buffer_buf + field_offset(binder_buffer, offsets_size));
        ulong extra_buffers_size = ULONG(binder_buffer_buf + field_offset(binder_buffer, extra_buffers_size));
        int pid = INT(binder_buffer_buf + field_offset(binder_buffer, pid));

        LOGD("binder_buffer at %lx: debug_id=%u, user_data=%lx, transaction=%lx, data_size=%lu, offsets_size=%lu, extra_buffers_size=%lu, pid=%d\n",
            buffer_addr, debug_id, user_data, transaction, data_size, offsets_size, extra_buffers_size, pid);
        FREEBUF(binder_buffer_buf);

        // Translate user virtual address to physical address if task context available
        physaddr_t paddr = 0;
        if (tc != nullptr) {
            uvtop(tc, user_data, &paddr, 0);
        }

        PRINT("    %s binder_buffer:%#lx id:%d data:%#lx PA:%#llx size:%zd offset:%zd extra:%zd pid:%d %s\n",
                type, buffer_addr, debug_id, user_data, (ulonglong)paddr,
                data_size, offsets_size, extra_buffers_size, pid, transaction ? "active" : "delivered");
    };

    // Process free and allocated buffers
    const int buffer_offset = field_offset(binder_buffer, rb_node);

    // List all free buffers from red-black tree
    const std::vector<ulong> free_list = for_each_rbtree(free_buffers_rb_node, buffer_offset);
    LOGD("Found %zu free buffers\n", free_list.size());
    for (const auto& buffer_addr : free_list) {
        print_buffer("Free", buffer_addr);
    }

    // List all allocated buffers from red-black tree
    const std::vector<ulong> alloc_list = for_each_rbtree(allocated_buffers_rb_node, buffer_offset);
    LOGD("Found %zu allocated buffers\n", alloc_list.size());
    for (const auto& buffer_addr : alloc_list) {
        print_buffer("Alloc", buffer_addr);
    }
}

/**
 * Print detailed binder process information
 * @param proc_addr: Address of binder_proc structure
 * @param flags: Bitmask indicating which information to display
 */
void Binder::print_binder_proc(ulong proc_addr, int flags) {
    if (!is_kvaddr(proc_addr)) {
        LOGE("Invalid proc address %#lx\n", proc_addr);
        return;
    }

    void *binder_proc_buf = read_struct(proc_addr, "binder_proc");
    if (!binder_proc_buf) {
        LOGE("Failed to read binder_proc structure at address %lx\n", proc_addr);
        return;
    }

    // Read all fields at once for better cache locality
    int pid = INT(binder_proc_buf + field_offset(binder_proc, pid));
    ulong tsk = ULONG(binder_proc_buf + field_offset(binder_proc, tsk));
    ulong threads_rb_node = ULONG(binder_proc_buf + field_offset(binder_proc, threads) + field_offset(rb_root, rb_node));
    ulong nodes_rb_node = ULONG(binder_proc_buf + field_offset(binder_proc, nodes) + field_offset(rb_root, rb_node));
    ulong refs_by_desc_rb_node = ULONG(binder_proc_buf + field_offset(binder_proc, refs_by_desc) + field_offset(rb_root, rb_node));
    ulong waiting_threads_next = ULONG(binder_proc_buf + field_offset(binder_proc, waiting_threads) + field_offset(list_head, next));
    ulong context_addr = ULONG(binder_proc_buf + field_offset(binder_proc, context));
    ulong list_head_todo = ULONG(binder_proc_buf + field_offset(binder_proc, todo));
    bool is_dead = BOOL(binder_proc_buf + field_offset(binder_proc, is_dead));
    bool is_frozen = BOOL(binder_proc_buf + field_offset(binder_proc, is_frozen));
    bool sync_recv = BOOL(binder_proc_buf + field_offset(binder_proc, sync_recv));
    bool async_recv = BOOL(binder_proc_buf + field_offset(binder_proc, async_recv));
    int max_threads = INT(binder_proc_buf + field_offset(binder_proc, max_threads));
    int requested_threads = INT(binder_proc_buf + field_offset(binder_proc, requested_threads));
    int requested_threads_started = INT(binder_proc_buf + field_offset(binder_proc, requested_threads_started));

    FREEBUF(binder_proc_buf);

    LOGD("binder_proc at %lx: pid=%d, tsk=%lx, is_dead=%d, is_frozen=%d, sync_recv=%d, async_recv=%d, max_threads=%d, requested_threads=%d, requested_threads_started=%d\n",
          proc_addr, pid, tsk, is_dead, is_frozen, sync_recv, async_recv, max_threads, requested_threads, requested_threads_started);

    // Get task context for this process
    struct task_context *tc = pid_to_context(pid);

    // Read binder context information (e.g., "binder", "hwbinder", "vndbinder")
    ulong name_addr = read_structure_field(context_addr, "binder_context", "name");
    if (!is_kvaddr(name_addr)) {
        LOGE("Invalid context name address\n");
        return;
    }

    // Read context and task names
    std::string context_name = read_cstring(name_addr, 16, "binder_context_name");
    std::string task_name = read_cstring(tsk + field_offset(task_struct, comm), 16, "task_struct_comm");

    // Collect thread information from red-black tree
    std::vector<ulong> thread_list = for_each_rbtree(threads_rb_node, field_offset(binder_thread, rb_node));

    // Collect waiting threads from linked list
    std::vector<ulong> wait_thread_list = for_each_list(waiting_threads_next, field_offset(binder_thread, waiting_thread_node));
    LOGD("Found %zu threads, %zu waiting threads\n", thread_list.size(), wait_thread_list.size());

    // Print proc header
    PRINT("binder_proc:%#lx %s [%d] %s dead:%d frozen:%d sr:%d ar:%d max:%d total:%zu requested:%d started:%d ready:%zu\n",
            proc_addr, task_name.c_str(), pid, context_name.c_str(),
            is_dead, is_frozen, sync_recv, async_recv,
            max_threads, thread_list.size(), requested_threads,
            requested_threads_started, wait_thread_list.size());

    // Process different flag types based on user request
    if (flags & BINDER_THREAD) {
        LOGD("Printing %zu thread(s)\n", thread_list.size());
        for (const auto& thread_addr : thread_list) {
            print_binder_thread_ilocked(thread_addr);
        }
    }

    if (flags & BINDER_NODE) {
        const int node_offset = field_offset(binder_node, rb_node);
        std::vector<ulong> node_list = for_each_rbtree(nodes_rb_node, node_offset);
        LOGD("Printing %zu node(s)\n", node_list.size());
        for (const auto& node_addr : node_list) {
            print_binder_node_nilocked(node_addr);
        }
    }

    if (flags & BINDER_REF) {
        const int ref_offset = field_offset(binder_ref, rb_node_desc);
        std::vector<ulong> ref_list = for_each_rbtree(refs_by_desc_rb_node, ref_offset);
        LOGD("Printing %zu reference(s)\n", ref_list.size());
        for (const auto& ref_addr : ref_list) {
            print_binder_ref_olocked(ref_addr);
        }
    }

    if (flags & BINDER_ALLOC) {
        ulong alloc_addr = proc_addr + field_offset(binder_proc, alloc);
        if (is_kvaddr(alloc_addr)) {
            print_binder_alloc(tc, alloc_addr);
        }
    }

    // Process pending work items in the todo list
    const int work_offset = field_offset(binder_work, entry);
    std::vector<ulong> work_list = for_each_list(list_head_todo, work_offset);
    LOGD("Processing %zu pending work item(s)\n", work_list.size());
    for (const auto& work_addr : work_list) {
        print_binder_work_ilocked(proc_addr, "    ", "    pending transaction", work_addr);
    }

    PRINT("\n");
}

/**
 * Print binder node information (represents a binder object)
 * @param node_addr: Address of binder_node structure
 */
void Binder::print_binder_node_nilocked(ulong node_addr) {
    if (!is_kvaddr(node_addr)) {
        LOGE("Invalid node address %#lx\n", node_addr);
        return;
    }

    void *binder_node_buf = read_struct(node_addr, "binder_node");
    if (!binder_node_buf) {
        LOGE("Failed to read binder_node structure at address %lx\n", node_addr);
        return;
    }

    // Read all fields at once for better cache locality
    int debug_id = INT(binder_node_buf + field_offset(binder_node, debug_id));
    unsigned long long ptr = ULONGLONG(binder_node_buf + field_offset(binder_node, ptr));
    unsigned long long cookie = ULONGLONG(binder_node_buf + field_offset(binder_node, cookie));
    ulong node_proc_addr = ULONG(binder_node_buf + field_offset(binder_node, proc));
    int internal_strong_refs = INT(binder_node_buf + field_offset(binder_node, internal_strong_refs));
    int local_weak_refs = INT(binder_node_buf + field_offset(binder_node, local_weak_refs));
    int local_strong_refs = INT(binder_node_buf + field_offset(binder_node, local_strong_refs));
    int tmp_refs = INT(binder_node_buf + field_offset(binder_node, tmp_refs));
    unsigned char flags1 = UCHAR(binder_node_buf + field_offset(binder_node, has_strong_ref));
    unsigned char flags2 = UCHAR(binder_node_buf + field_offset(binder_node, sched_policy));
    unsigned char min_priority = UCHAR(binder_node_buf + field_offset(binder_node, min_priority));
    FREEBUF(binder_node_buf);

    // Extract flags
    int has_strong_ref = (flags1 >> 0) & 0x01;
    int has_weak_ref = (flags1 >> 2) & 0x01;
    unsigned char sched_policy = flags2 & 0x03;

    LOGD("binder_node at %lx: debug_id=%d, ptr=%llx, cookie=%llx, internal_strong_refs=%d, local_weak_refs=%d, local_strong_refs=%d, tmp_refs=%d, has_strong_ref=%d, has_weak_ref=%d, sched_policy=%u, min_priority=%u\n",
          node_addr, debug_id, ptr, cookie, internal_strong_refs, local_weak_refs, local_strong_refs, tmp_refs, has_strong_ref, has_weak_ref, sched_policy, min_priority);

    // Get reference list (all references pointing to this node)
    const std::vector<ulong> ref_list = for_each_hlist(node_addr + field_offset(binder_node, refs), field_offset(binder_ref, node_entry));

    PRINT("  binder_node:%#lx id:%d ptr:%#lx cookie:%#lx pri:%s[%d] hs:%d hw:%d ls:%d lw:%d is:%d iw:%zu tr:%d\n",
           node_addr, debug_id, ptr, cookie,
           sched_name[sched_policy], min_priority, has_strong_ref, has_weak_ref,
           local_strong_refs, local_weak_refs, internal_strong_refs, ref_list.size(), tmp_refs);

    // Print all references to this node
    LOGD("Node has %zu reference(s)\n", ref_list.size());
    for (const auto& ref_addr : ref_list) {
        void *binder_ref_buf = read_struct(ref_addr, "binder_ref");
        if (!binder_ref_buf) {
            LOGE("Failed to read binder_ref structure at address %lx\n", ref_addr);
            continue;
        }

        ulong proc_addr = ULONG(binder_ref_buf + field_offset(binder_ref, proc));
        int ref_debug_id = INT(binder_ref_buf + field_offset(binder_ref, data) + field_offset(binder_ref_data, debug_id));
        FREEBUF(binder_ref_buf);

        LOGD("binder_ref at %lx: debug_id=%d, proc=%lx\n", ref_addr, ref_debug_id, proc_addr);

        if (!is_kvaddr(proc_addr)) {
            continue;
        }

        void *binder_proc_buf = read_struct(proc_addr, "binder_proc");
        if (!binder_proc_buf) {
            LOGE("Failed to read binder_proc structure at address %lx\n", proc_addr);
            continue;
        }

        int pid = INT(binder_proc_buf + field_offset(binder_proc, pid));
        ulong tsk_addr = ULONG(binder_proc_buf + field_offset(binder_proc, tsk));
        FREEBUF(binder_proc_buf);

        LOGD("binder_proc at %lx: pid=%d, tsk=%lx\n", proc_addr, pid, tsk_addr);

        const std::string task_name = read_cstring(tsk_addr + field_offset(task_struct, comm), 16, "task_struct_comm");
        PRINT("     binder_ref:%#lx id:%d binder_proc:%#lx %s[%d]\n", ref_addr, ref_debug_id, proc_addr, task_name.c_str(), pid);
    }

    // Print pending async transactions for this node
    if (is_kvaddr(node_proc_addr)) {
        const std::vector<ulong> work_list = for_each_list(node_addr + field_offset(binder_node, async_todo), field_offset(binder_work, entry));
        LOGD("Node has %zu pending async transaction(s)\n", work_list.size());
        for (const auto& work_addr : work_list) {
            print_binder_work_ilocked(node_proc_addr, "    ", "    pending async transaction", work_addr);
        }
    }
}

/**
 * Print binder reference information (handle to a binder object)
 * @param ref_addr: Address of binder_ref structure
 */
void Binder::print_binder_ref_olocked(ulong ref_addr) {
    if (!is_kvaddr(ref_addr)) {
        LOGE("Invalid ref address %#lx\n", ref_addr);
        return;
    }

    void *binder_ref_buf = read_struct(ref_addr, "binder_ref");
    if (!binder_ref_buf) {
        LOGE("Failed to read binder_ref structure at address %#lx\n", ref_addr);
        return;
    }

    // Extract all ref data in one pass
    ulong node_addr = ULONG(binder_ref_buf + field_offset(binder_ref, node));
    ulong death_addr = ULONG(binder_ref_buf + field_offset(binder_ref, death));
    ulong data_offset = field_offset(binder_ref, data);
    int ref_debug_id = INT(binder_ref_buf + data_offset + field_offset(binder_ref_data, debug_id));
    uint32_t ref_desc = UINT(binder_ref_buf + data_offset + field_offset(binder_ref_data, desc));
    int ref_strong = INT(binder_ref_buf + data_offset + field_offset(binder_ref_data, strong));
    int ref_weak = INT(binder_ref_buf + data_offset + field_offset(binder_ref_data, weak));

    LOGD("binder_ref at %#lx: node=%#lx, death=%#lx, debug_id=%d, desc=%u, strong=%d, weak=%d\n",
          ref_addr, node_addr, death_addr, ref_debug_id, ref_desc, ref_strong, ref_weak);
    FREEBUF(binder_ref_buf);

    if (!is_kvaddr(node_addr)) {
        LOGE("Invalid node address in ref\n");
        return;
    }

    // Read node and proc data in batch
    void *binder_node_buf = read_struct(node_addr, "binder_node");
    if (!binder_node_buf) {
        LOGE("Failed to read binder_node structure at address %#lx\n", node_addr);
        return;
    }

    int node_debug_id = INT(binder_node_buf + field_offset(binder_node, debug_id));
    ulong node_proc = ULONG(binder_node_buf + field_offset(binder_node, proc));
    LOGD("binder_node at %#lx: debug_id=%d, proc=%#lx\n", node_addr, node_debug_id, node_proc);
    FREEBUF(binder_node_buf);

    if (!is_kvaddr(node_proc)) {
        PRINT("  binder_ref :0x%-16lx id:%-6d desc:%-4d s:%-1d w:%-1d death:%-lx -> binder_node:0x%-16lx id:%-6d %s\n",
                ref_addr, ref_debug_id, ref_desc, ref_strong,
                ref_weak, death_addr, node_addr, node_debug_id, "[dead]");
        return;
    }

    void *binder_proc_buf = read_struct(node_proc, "binder_proc");
    if (!binder_proc_buf) {
        LOGE("Failed to read binder_proc structure at address %#lx\n", node_proc);
        return;
    }

    int proc_pid = INT(binder_proc_buf + field_offset(binder_proc, pid));
    ulong tsk_addr = ULONG(binder_proc_buf + field_offset(binder_proc, tsk));
    LOGD("binder_proc at %#lx: pid=%d, tsk=%#lx\n", node_proc, proc_pid, tsk_addr);
    FREEBUF(binder_proc_buf);

    std::string task_name = read_cstring(tsk_addr + field_offset(task_struct, comm), 16, "task_struct_comm");

    PRINT("  binder_ref :0x%-16lx id:%-6d desc:%-4d s:%-1d w:%-1d death:%-lx -> binder_node:0x%-16lx id:%-6d binder_proc:0x%-16lx %s[%d]\n",
          ref_addr,ref_debug_id,ref_desc,ref_strong,ref_weak,death_addr,node_addr,node_debug_id,node_proc,task_name.c_str(),proc_pid);
}

/**
 * Print binder thread information and its transaction stack
 * @param thread: Address of binder_thread structure
 */
void Binder::print_binder_thread_ilocked(ulong thread) {
    if (!is_kvaddr(thread)) {
        LOGE("Invalid thread address %#lx\n", thread);
        return;
    }

    void *binder_thread_buf = read_struct(thread, "binder_thread");
    if (!binder_thread_buf) {
        LOGE("Failed to read binder_thread structure at address %lx\n", thread);
        return;
    }

    // Read all fields at once for better cache locality
    ulong proc = ULONG(binder_thread_buf + field_offset(binder_thread, proc));
    ulong transaction_stack = ULONG(binder_thread_buf + field_offset(binder_thread, transaction_stack));
    int pid = INT(binder_thread_buf + field_offset(binder_thread, pid));
    int looper = INT(binder_thread_buf + field_offset(binder_thread, looper));
    bool looper_need_return = BOOL(binder_thread_buf + field_offset(binder_thread, looper_need_return));
    bool is_dead = BOOL(binder_thread_buf + field_offset(binder_thread, is_dead));

    LOGD("binder_thread at %lx: pid=%d, looper=%d, is_dead=%d\n", thread, pid, looper, is_dead);
    FREEBUF(binder_thread_buf);

    if (pid <= 0) {
        LOGE("Invalid thread PID %d\n", pid);
        return;
    }
    PRINT("  binder_thread:0x%-16lx pid:%-6d loop:%-2d need_return:%d\n",
          thread,        // binder_thread
          pid,                // pid
          looper,             // loop (looper)
          looper_need_return ? 1 : 0);  // need_return
    // Walk through the transaction stack with loop protection
    int transaction_count = 0;
    const int MAX_TRANSACTION_DEPTH = 100; // Prevent infinite loops
    ulong t = transaction_stack;

    while (t && transaction_count < MAX_TRANSACTION_DEPTH) {
        transaction_count++;
        void *binder_transaction_buf = read_struct(t, "binder_transaction");
        if (!binder_transaction_buf) {
            LOGE("Failed to read transaction at %#lx\n", t);
            break;
        }

        // Read all transaction fields at once
        ulong t_from = ULONG(binder_transaction_buf + field_offset(binder_transaction, from));
        ulong t_from_parent = ULONG(binder_transaction_buf + field_offset(binder_transaction, from_parent));
        ulong t_to_parent = ULONG(binder_transaction_buf + field_offset(binder_transaction, to_parent));
        ulong t_to_thread = ULONG(binder_transaction_buf + field_offset(binder_transaction, to_thread));

        FREEBUF(binder_transaction_buf);

        // Determine transaction direction and traverse the stack
        if (t_from == thread) {
            print_binder_transaction_ilocked(proc, "    outgoing transaction", t);
            t = t_from_parent;
        } else if (t_to_thread == thread) {
            print_binder_transaction_ilocked(proc, "    incoming transaction", t);
            t = t_to_parent;
        } else {
            LOGE("Bad transaction linkage detected at depth %d\n", transaction_count);
            print_binder_transaction_ilocked(proc, "    bad transaction", t);
            break;
        }
    }

    if (transaction_count >= MAX_TRANSACTION_DEPTH) {
        LOGE("Transaction stack depth exceeded maximum (%d), possible circular reference\n", MAX_TRANSACTION_DEPTH);
    }

    LOGD("Thread has %d transaction(s) in stack\n", transaction_count);
}

/**
 * Print detailed binder transaction information
 * @param proc_addr: Address of the process owning this transaction
 * @param prefix: Prefix string for output formatting
 * @param transaction: Address of binder_transaction structure
 */
void Binder::print_binder_transaction_ilocked(ulong proc_addr, const char* prefix, ulong transaction) {
    if (!is_kvaddr(transaction)) {
        LOGE("Invalid transaction address %#lx\n", transaction);
        return;
    }

    void *binder_transaction_buf = read_struct(transaction, "binder_transaction");
    if (!binder_transaction_buf) {
        LOGE("Failed to read transaction at %#lx\n", transaction);
        return;
    }

    // Read all transaction fields at once for efficiency
    ulong t_from = ULONG(binder_transaction_buf + field_offset(binder_transaction, from));
    ulong t_to_thread = ULONG(binder_transaction_buf + field_offset(binder_transaction, to_thread));
    ulong t_to_proc = ULONG(binder_transaction_buf + field_offset(binder_transaction, to_proc));
    ulong t_buffer = ULONG(binder_transaction_buf + field_offset(binder_transaction, buffer));
    unsigned int t_debug_id = UINT(binder_transaction_buf + field_offset(binder_transaction, debug_id));
    unsigned int t_code = UINT(binder_transaction_buf + field_offset(binder_transaction, code));
    unsigned int t_flags = UINT(binder_transaction_buf + field_offset(binder_transaction, flags));
    unsigned int t_need_reply = UINT(binder_transaction_buf + field_offset(binder_transaction, need_reply));
    unsigned int t_sched_policy = UINT(binder_transaction_buf + field_offset(binder_transaction, priority) + field_offset(binder_priority, sched_policy));
    unsigned int t_prio = UINT(binder_transaction_buf + field_offset(binder_transaction, priority) + field_offset(binder_priority, prio));
    FREEBUF(binder_transaction_buf);

    // Initialize process/thread info for source and destination
    int from_proc_pid = 0, from_thread_pid = 0;
    int to_proc_pid = 0, to_thread_pid = 0;

    // Read source thread/proc info if available
    if (is_kvaddr(t_from)) {
        void *binder_thread_buf = read_struct(t_from, "binder_thread");
        if (binder_thread_buf) {
            ulong from_thread_proc = ULONG(binder_thread_buf + field_offset(binder_thread, proc));
            from_thread_pid = INT(binder_thread_buf + field_offset(binder_thread, pid));
            FREEBUF(binder_thread_buf);

            if (is_kvaddr(from_thread_proc)) {
                void *binder_proc_buf = read_struct(from_thread_proc, "binder_proc");
                if (binder_proc_buf) {
                    from_proc_pid = INT(binder_proc_buf + field_offset(binder_proc, pid));
                    FREEBUF(binder_proc_buf);
                }
            }
        }
    }

    // Read destination proc/thread info
    if (is_kvaddr(t_to_proc)) {
        void *binder_proc_buf = read_struct(t_to_proc, "binder_proc");
        if (binder_proc_buf) {
            to_proc_pid = INT(binder_proc_buf + field_offset(binder_proc, pid));
            FREEBUF(binder_proc_buf);
        }
    }

    if (is_kvaddr(t_to_thread)) {
        void *binder_thread_buf = read_struct(t_to_thread, "binder_thread");
        if (binder_thread_buf) {
            to_thread_pid = INT(binder_thread_buf + field_offset(binder_thread, pid));
            FREEBUF(binder_thread_buf);
        }
    }

    PRINT("%s:%#lx id:%u from %d:%d to %d:%d code:%u flags:%u pri:%s[%u] reply:%u",
            prefix, transaction, t_debug_id, from_proc_pid, from_thread_pid,
            to_proc_pid, to_thread_pid, t_code, t_flags,
            sched_name[t_sched_policy], t_prio, t_need_reply);

    if (proc_addr != t_to_proc) {
        PRINT("\n");
        return;
    }

    if (!is_kvaddr(t_buffer)) {
        PRINT(" buffer free\n");
        return;
    }

    void *binder_buffer_buf = read_struct(t_buffer, "binder_buffer");
    if (!binder_buffer_buf) {
        LOGE("Failed to read binder_buffer structure at address %lx\n", t_buffer);
        PRINT("\n");
        return;
    }

    ulong user_data = ULONG(binder_buffer_buf + field_offset(binder_buffer, user_data));
    ulong data_size = ULONG(binder_buffer_buf + field_offset(binder_buffer, data_size));
    ulong offsets_size = ULONG(binder_buffer_buf + field_offset(binder_buffer, offsets_size));
    ulong target_node = ULONG(binder_buffer_buf + field_offset(binder_buffer, target_node));
    FREEBUF(binder_buffer_buf);

    if (is_kvaddr(target_node)) {
        int debug_id = read_structure_field(target_node, "binder_node", "debug_id");
        PRINT(" target_node:%#lx id:%d", target_node, debug_id);
    }
    PRINT(" binder_buffer:%#lx size:%zu offset:%zu data:%p\n",
            t_buffer, data_size, offsets_size, (void*)user_data);
}

/**
 * Print binder work item information
 * @param proc_addr: Address of the process owning this work
 * @param prefix: Prefix string for output formatting
 * @param transaction_prefix: Prefix for transaction-specific output
 * @param work: Address of binder_work structure
 */
void Binder::print_binder_work_ilocked(ulong proc_addr, const char* prefix, const char* transaction_prefix, ulong work) {
    if (!is_kvaddr(work)) {
        LOGE("Invalid work address %#lx\n", work);
        return;
    }

    struct binder_work w;
    if (!read_struct(work, &w, sizeof(w), "binder_work")) {
        LOGE("Failed to read binder_work at %#lx\n", work);
        return;
    }

    // Handle different work types using jump table approach for better performance
    switch (w.type) {
        case BINDER_WORK_TRANSACTION: {
            // Transaction work: calculate transaction address from work address
            const ulong transaction = work - field_offset(binder_transaction, work);
            print_binder_transaction_ilocked(proc_addr, transaction_prefix, transaction);
            break;
        }
        case BINDER_WORK_RETURN_ERROR: {
            // Error work: read and display error information
            const ulong error_addr = work - offsetof(struct binder_error, work);
            struct binder_error e;
            if (read_struct(error_addr, &e, sizeof(e), "binder_error")) {
                PRINT("%stransaction error: %u\n", prefix, e.cmd);
            } else {
                LOGE("Failed to read error at %#lx\n", error_addr);
            }
            break;
        }
        case BINDER_WORK_TRANSACTION_COMPLETE:
            // Transaction completion notification
            PRINT("%stransaction complete\n", prefix);
            break;
        case BINDER_WORK_NODE: {
            // Node work: display node information
            const ulong node_addr = work - field_offset(binder_node, work);
            void *binder_node_buf = read_struct(node_addr, "binder_node");
            if (!binder_node_buf) {
                LOGE("Failed to read binder_node structure at address %lx\n", node_addr);
                return;
            }

            // Read all node fields at once for better cache locality
            const int debug_id = INT(binder_node_buf + field_offset(binder_node, debug_id));
            const unsigned long long ptr = ULONGLONG(binder_node_buf + field_offset(binder_node, ptr));
            const unsigned long long cookie = ULONGLONG(binder_node_buf + field_offset(binder_node, cookie));
            FREEBUF(binder_node_buf);

            PRINT("%snode:%#lx work %d: u%lx c%lx\n", prefix, node_addr, debug_id, ptr, cookie);
            break;
        }
        case BINDER_WORK_DEAD_BINDER:
            // Dead binder notification
            PRINT("%shas dead binder\n", prefix);
            break;
        case BINDER_WORK_DEAD_BINDER_AND_CLEAR:
            // Dead binder cleared notification
            PRINT("%shas cleared dead binder\n", prefix);
            break;
        case BINDER_WORK_CLEAR_DEATH_NOTIFICATION:
            // Death notification cleared
            PRINT("%shas cleared death notification\n", prefix);
            break;
        default:
            // Unknown work type
            PRINT("%sunknown work: type %d\n", prefix, w.type);
            break;
    }
}
#pragma GCC diagnostic pop
