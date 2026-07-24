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

#ifndef MEMINFO_DEFS_H_
#define MEMINFO_DEFS_H_

#include <map>
#include <cmath>
#include "plugin.h"
#include "devicetree/devicetree.h"

class Meminfo : public ParserPlugin {
private:
    std::vector<ulong> node_page_state;
    std::vector<ulong> zone_page_state;
    std::vector<std::vector<ulong>> vm_event_page_state;
    std::map<std::string, ulong> enums;
    std::map<std::string, ulong> g_param;
    std::map<std::string, std::map<std::string, ulong>> enum_dict;
    std::shared_ptr<Devicetree> dts;

    void parse_meminfo(void);
    ulong get_wmark_low(void);
    ulong get_available(ulong);
    ulong get_blockdev_nr_pages(void);
    ulong get_to_be_unused_nr_pages(void);
    ulong get_vm_commit_pages(ulong);
    ulong get_mm_committed_pages(void);
    size_t get_cma_size();
    size_t get_struct_page_size();
    size_t get_memory_size();
    size_t get_nomap_size();
    size_t get_dmabuf_size();
    size_t get_vmalloc_size();
    size_t parser_vmap_area(ulong addr);
    size_t get_dentry_cache_size();
    size_t get_inode_cache_size();
    ulong get_vmalloc_total(void);
    void print_vmstat(void);
    void print_mem_breakdown(void);
    void print_meminfo(void);

public:
    Meminfo();
    void cmd_main(void) override;
    void init_offset(void) override;
    void init_command(void) override;
    DEFINE_PLUGIN_INSTANCE(Meminfo)
};


#endif // MEMINFO_DEFS_H_
