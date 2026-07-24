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

#ifndef PROCRANK_DEFS_H_
#define PROCRANK_DEFS_H_
#include "plugin.h"
#include "memory/swapinfo.h"
#include "../utils/utask.h"

struct procrank{
    uint64_t vss;
    uint64_t rss;
    uint64_t pss;
    uint64_t uss;
    uint64_t swap;
    int pid;
    std::string comm;
    std::string cmdline;
};

class Procrank : public ParserPlugin {
public:
    Procrank();
    Procrank(std::shared_ptr<Swapinfo> swap);
    void cmd_main(void) override;
    void init_offset(void) override;
    void init_command(void) override;
    DEFINE_PLUGIN_INSTANCE(Procrank)

private:
    std::shared_ptr<Swapinfo> swap_ptr;
    std::vector<std::shared_ptr<procrank>> procrank_list;

    void parser_process_memory();
    void parser_all_process_memory(uint64_t &total_vss, uint64_t &total_rss, uint64_t &total_pss, uint64_t &total_uss, uint64_t &total_swap);
    std::shared_ptr<procrank> parser_single_process_memory(ulong task_addr, task_context *tc);
    void print_process_memory_table(uint64_t total_vss, uint64_t total_rss, uint64_t total_pss, uint64_t total_uss, uint64_t total_swap);
    std::shared_ptr<procrank> parser_vma(std::shared_ptr<UTask> task_ptr, std::shared_ptr<vma_struct> vma_ptr);
    void parser_process_name();
    std::string parser_single_process_cmdline(ulong task_addr, task_context *tc);
    void parser_process_name_only(std::ostringstream &oss);
    void print_process_name_header(std::ostringstream &oss);
    void print_process_name_row(std::ostringstream &oss, int pid, const char *comm, const std::string &cmdline);
};
#endif // PROCRANK_DEFS_H_
