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

#include "procrank.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(Procrank)
#endif

void Procrank::cmd_main(void) {
    LOGI("Command started, argcnt=%d\n", argcnt);
    if (argcnt < 2) return cmd_usage(pc->curcmd, SYNOPSIS);
    bool need_memory = false;
    bool need_name = false;
    for (int c; (c = getopt(argcnt, args, "ac")) != EOF;) {
        switch(c) {
            case 'a':
                need_memory = true;
                break;
            case 'c':
                need_name = true;
                break;
            default:
                argerrs++;
                break;
        }
    }
    if (argerrs) return cmd_usage(pc->curcmd, SYNOPSIS);
    if (need_memory) {
        LOGI("Parsing process memory\n");
        parser_process_memory();
    }
    if (need_name) {
        LOGI("Parsing process names\n");
        parser_process_name();
    }
    LOGI("Command completed\n");
}

void Procrank::init_offset(void) {}

Procrank::Procrank(std::shared_ptr<Swapinfo> swap) : swap_ptr(swap){}

Procrank::Procrank(){
    swap_ptr = std::make_unique<Swapinfo>();
}

void Procrank::init_command(void){
    cmd_name = "procrank";
    help_str_list={
        "procrank",                                /* command name */
        "display process memory usage and ranking information",  /* short description */
        "[-a] [-c]\n"
        "  This command analyzes process memory usage and provides detailed ranking\n"
        "  information sorted by RSS (Resident Set Size). It calculates various\n"
        "  memory metrics including VSS, RSS, PSS, USS, and swap usage.\n"
        "\n"
        "    -a              display all processes with detailed memory statistics\n"
        "    -c              display process command lines and names\n",
        "\n",
        "EXAMPLES",
        "  Display all processes ranked by memory usage:",
        "    %s> procrank -a",
        "    PID        Vss        Rss        Pss        Uss        Swap        Comm",
        "    975      1.97Gb     51.09Mb    13.71Mb    3.54Mb     1.99Mb     Binder:975_3",
        "    465      1.69Gb     4.53Mb     286.01Kb   36.00Kb    26.01Mb    main",
        "\n",
        "  Display process command lines and names:",
        "    %s> procrank -c",
        "    PID      Comm                 Cmdline",
        "    1        init                 /system/bin/init",
        "\n",
    };
}

void Procrank::parser_process_memory() {
    LOGI("Start parsing process memory\n");
    if (!swap_ptr->is_zram_enable()) {
        LOGW("ZRAM not enabled, returning\n");
        return;
    }
    uint64_t total_vss = 0, total_rss = 0, total_pss = 0, total_uss = 0, total_swap = 0;
    if (procrank_list.empty()) {
        LOGI("Procrank list empty, parsing all processes\n");
        parser_all_process_memory(total_vss, total_rss, total_pss, total_uss, total_swap);
        LOGI("Sorting %zu processes by RSS\n", procrank_list.size());
        std::sort(procrank_list.begin(), procrank_list.end(),
                 [](const auto& a, const auto& b) { return a->rss > b->rss; });
    }
    total_vss = total_rss = total_pss = total_uss = total_swap = 0;
    for (const auto& proc : procrank_list) {
        total_vss += proc->vss;
        total_rss += proc->rss;
        total_pss += proc->pss;
        total_uss += proc->uss;
        total_swap += proc->swap;
    }
    print_process_memory_table(total_vss, total_rss, total_pss, total_uss, total_swap);
    LOGI("Memory parsing completed\n");
}

void Procrank::parser_all_process_memory(uint64_t& total_vss, uint64_t& total_rss,
                                        uint64_t& total_pss, uint64_t& total_uss, uint64_t& total_swap) {
    LOGI("Start parsing all processes memory\n");
    int process_count = 0;
    for (ulong task_addr : for_each_process()) {
        process_count++;
        struct task_context *tc = task_to_context(task_addr);
        if (!tc) {
            LOGE("Failed to get task context for task_addr=0x%lx\n", task_addr);
            continue;
        }
        auto proc_mem_ptr = parser_single_process_memory(task_addr, tc);
        if (!proc_mem_ptr) {
            LOGE("Failed to parse memory for PID=%d\n", tc->pid);
            continue;
        }
        total_vss += proc_mem_ptr->vss;
        total_rss += proc_mem_ptr->rss;
        total_pss += proc_mem_ptr->pss;
        total_uss += proc_mem_ptr->uss;
        total_swap += proc_mem_ptr->swap;
        procrank_list.push_back(proc_mem_ptr);
    }
    LOGI("Successfully parsed %zu/%d processes\n",
         procrank_list.size(), process_count);
}

std::shared_ptr<procrank> Procrank::parser_single_process_memory(ulong task_addr, struct task_context *tc) {
    auto task_ptr = std::make_shared<UTask>(swap_ptr, task_addr);
    auto proc_mem_ptr = std::make_shared<procrank>();
    proc_mem_ptr->pid = tc->pid;
    proc_mem_ptr->comm = tc->comm;
    proc_mem_ptr->cmdline = task_ptr->read_start_args();
    for (const auto& vma_ptr : task_ptr->for_each_vma_list()) {
        auto vma_mem_ptr = parser_vma(task_ptr, vma_ptr);
        proc_mem_ptr->vss += vma_mem_ptr->vss;
        proc_mem_ptr->rss += vma_mem_ptr->rss;
        proc_mem_ptr->pss += vma_mem_ptr->pss;
        proc_mem_ptr->uss += vma_mem_ptr->uss;
        proc_mem_ptr->swap += vma_mem_ptr->swap;
    }
    task_ptr.reset();
    return proc_mem_ptr;
}

void Procrank::print_process_memory_table(uint64_t total_vss, uint64_t total_rss,
                                         uint64_t total_pss, uint64_t total_uss, uint64_t total_swap) {
    std::ostringstream oss;
    oss << std::left << std::setw(8) << "PID" << " "
        << std::left << std::setw(10) << "Vss" << " "
        << std::left << std::setw(10) << "Rss" << " "
        << std::left << std::setw(10) << "Pss" << " "
        << std::left << std::setw(10) << "Uss" << " "
        << std::left << std::setw(10) << "Swap" << " "
        << "Comm" << "\n";
    for (const auto& p : procrank_list) {
        oss << std::left << std::setw(8) << p->pid << " "
            << std::left << std::setw(10) << csize(p->vss) << " "
            << std::left << std::setw(10) << csize(p->rss) << " "
            << std::left << std::setw(10) << csize(p->pss) << " "
            << std::left << std::setw(10) << csize(p->uss) << " "
            << std::left << std::setw(10) << csize(p->swap) << " "
            << p->cmdline << "\n";
    }
    oss << std::left << std::setw(8) << "Total" << " "
        << std::left << std::setw(10) << csize(total_vss) << " "
        << std::left << std::setw(10) << csize(total_rss) << " "
        << std::left << std::setw(10) << csize(total_pss) << " "
        << std::left << std::setw(10) << csize(total_uss) << " "
        << std::left << std::setw(10) << csize(total_swap) << "\n";
    PRINT("%s\n", oss.str().c_str());
}

std::shared_ptr<procrank> Procrank::parser_vma(std::shared_ptr<UTask> task_ptr, std::shared_ptr<vma_struct> vma_ptr) {
    auto procrank_ptr = std::make_shared<procrank>();
    for(ulong vaddr = vma_ptr->vm_start; vaddr < vma_ptr->vm_end; vaddr+= page_size){
        ulong page_vaddr = vaddr & page_mask;
        physaddr_t paddr;
        if (!uvtop(task_ptr->get_task_context(), page_vaddr, &paddr, 0)) { //page not exists
            if(paddr == 0x0){ // pte == 0
                continue;
            }
            // bit0 is 0
            if((paddr & (1UL << 0)) == 0){
                procrank_ptr->swap += page_size;
            }
            continue;
        }
        ulong page_addr = phy_to_page(paddr);
        // typedef struct {
        //     int counter;
        // } atomic_t;
        // SIZE: 4
        ulong page_count = read_structure_field(page_addr,"page","_mapcount");
        // Page was unmapped between the presence check at the beginning of the loop and here.
        if(page_count == 0){
            continue;
        }
        procrank_ptr->rss += page_size;
        procrank_ptr->pss += page_size / page_count;
        procrank_ptr->uss += (page_count == 1) ? page_size : (0);
    }
    procrank_ptr->vss += vma_ptr->vm_end - vma_ptr->vm_start;
    return procrank_ptr;
}

void Procrank::parser_process_name() {
    LOGI("Start parsing process names\n");
    std::ostringstream oss;
    print_process_name_header(oss);
    if (!procrank_list.empty()) {
        LOGI("Using existing procrank_list with %zu processes\n", procrank_list.size());
        for (const auto& proc : procrank_list) {
            print_process_name_row(oss, proc->pid, proc->comm.c_str(), proc->cmdline);
        }
    } else {
        LOGI("Procrank list empty, parsing names only\n");
        parser_process_name_only(oss);
    }
    PRINT("%s\n", oss.str().c_str());
    LOGI("Name parsing completed\n");
}

std::string Procrank::parser_single_process_cmdline(ulong task_addr, struct task_context *tc) {
    if (!swap_ptr->is_zram_enable()) {
        return tc->comm;
    }
    auto task_ptr = std::make_shared<UTask>(swap_ptr, task_addr);
    std::string cmdline = task_ptr->read_start_args();
    task_ptr.reset();
    return cmdline;
}

void Procrank::parser_process_name_only(std::ostringstream& oss) {
    int process_count = 0;
    for (ulong task_addr : for_each_process()) {
        process_count++;
        struct task_context *tc = task_to_context(task_addr);
        if (!tc) continue;
        std::string cmdline = parser_single_process_cmdline(task_addr, tc);
        print_process_name_row(oss, tc->pid, tc->comm, cmdline);
    }
    LOGI("Parsed %d process names\n", process_count);
}

void Procrank::print_process_name_header(std::ostringstream& oss) {
    oss << std::left << std::setw(8) << "PID" << " "
        << std::left << std::setw(20) << "Comm" << " "
        << std::left << std::setw(10) << "Cmdline" << "\n";
}

void Procrank::print_process_name_row(std::ostringstream& oss, int pid,
                                     const char* comm, const std::string& cmdline) {
    oss << std::left << std::setw(8) << pid << " "
        << std::left << std::setw(20) << comm << " "
        << std::left << std::setw(10) << cmdline << "\n";
}

#pragma GCC diagnostic pop
