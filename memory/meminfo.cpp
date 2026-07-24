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


#include "meminfo.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(Meminfo)
#endif

void Meminfo::cmd_main(void) {
    int c;
    if (argcnt < 2) cmd_usage(pc->curcmd, SYNOPSIS);
    if (node_page_state.empty() or zone_page_state.empty()){
        parse_meminfo();
    }
    while ((c = getopt(argcnt, args, "abv")) != EOF) {
        switch(c) {
            case 'a':
                print_meminfo();
                break;
            case 'b':
                print_mem_breakdown();
                break;
            case 'v':
                print_vmstat();
                break;
            default:
                argerrs++;
                break;
        }
    }
    if (argerrs)
        cmd_usage(pc->curcmd, SYNOPSIS);
}

void Meminfo::init_offset(void) {
    struct_init(page);
    struct_init(zone);
    field_init(zone, watermark);
    field_init(zone, _watermark);
    field_init(zone, watermark_boost);
    field_init(zone, present_pages);
    field_init(zone, spanned_pages);
    field_init(zone, managed_pages);
    field_init(zone, cma_pages);
    field_init(zone, vm_stat);
    field_init(zone, name);
    field_init(pglist_data, node_zones);
    field_init(super_block, s_inodes);
    field_init(inode, i_mapping);
    field_init(inode, i_sb_list);
    field_init(address_space, nrpages);
    field_init(swap_info_struct, flags);
    field_init(swap_info_struct, inuse_pages);
    field_init(percpu_counter, count);
    field_init(percpu_counter, counters);
    struct_init(cma);
    struct_init(reserved_mem);
    struct_init(page);
    struct_init(hlist_bl_head);
    struct_init(hlist_head);
    field_init(cma, count);
    field_init(reserved_mem,size);
    field_init(reserved_mem,name);
    field_init(dma_buf,list_node);
    field_init(dma_buf,size);
    field_init(vmap_area,list);
    field_init(vmap_area,vm);
    field_init(vm_struct,nr_pages);
    field_init(vm_struct,next);
    field_init(vm_struct,size);
    field_init(vmap_node,pool);
    field_init(vmap_pool,len);
    struct_init(vmap_node);
    struct_init(vmap_pool);
    if (get_config_val("CONFIG_HUGETLB_PAGE") == "y") {
        struct_init(hstate);
        field_init(hstate, order);
        field_init(hstate, nr_huge_pages);
    }
}

void Meminfo::init_command(void) {
    cmd_name = "meminfo";
    help_str_list={
        "meminfo",                                /* command name */
        "display comprehensive system memory information and analysis",  /* short description */
        "[-a] [-b] [-v]\n"
        "  This command provides detailed analysis of system memory usage,\n"
        "  including kernel memory statistics, zone information, and memory\n"
        "  breakdown analysis from crash dump data.\n"
        "\n"
        "    -a              display complete memory information (similar to /proc/meminfo)\n"
        "    -b              display memory breakdown analysis with categorized usage\n"
        "    -v              display detailed vmstat information with zone statistics\n",
        "\n",
        "EXAMPLES",
        "  Display complete system memory information:",
        "    %s> meminfo -a",
        " ",
        "    MemTotal:        11445568 KB",
        "    MemFree:          9862060 KB",
        "    MemAvailable:    10147000 KB",
        "    ... ...",
        "    Active(anon):        2892 KB",
        "    Inactive(anon):    114068 KB",
        "\n",
        "  Display memory breakdown analysis:",
        "    %s> meminfo -b",
        "    RAM                :      7.93GB",
        "       MemTotal        :      7.44GB",
        "       MemFree         :      7.04GB",
        "       Buffers         :      9.73MB",
        "       Cached          :     94.65MB",
        "       SwapCached      :          0B",
        "       AonPage         :     98.32MB",
        "       FilePage        :     95.14MB",
        "       Slab            :    127.79MB",
        "       KernelStack     :      4.02MB",
        "       PageTables      :      1.97MB",
        "       Shmem           :      9.23MB",
        "       Cma             :        32MB",
        "       Dmabuf          :          0B",
        "       Vmalloc         :     35.59MB",
        "       Other           :     28.53MB",
        "",
        "       Struct Page     :    126.88MB",
        "       Kernel Code     :     23.38MB",
        "       Kernel Data     :      5.38MB",
        "       Dentry Cache    :         8MB",
        "       Inode  Cache    :        64KB",
        "",
        "       NON_HLOS        :    308.34MB",
        "\n",
        "  Display detailed vmstat and zone information:",
        "    %s> meminfo -v",
        "    Zone Normal",
        "    present_pages                 :     148440        579MB",
        "    spanned_pages                 :     183296        716MB",
        "    managed_pages                 :     135130        527MB",
        "    WMARK_MIN                     :        722          2MB",
        "    WMARK_LOW                     :       1185          4MB",
        "    WMARK_HIGH                    :       1365          5MB",
        "    NR_FREE_PAGES                 :       1846          7MB",
        "    NR_ZONE_LRU_BASE              :       1846          7MB",
        "    ... ...",
        "\n",
    };
}

Meminfo::Meminfo(){}

void Meminfo::parse_meminfo(void){
    const char* enum_names[] = {
        "SWP_USED",
        "SWP_WRITEOK"
    };
    std::vector<std::string> enum_list = {
        "lru_list",
        "zone_type",
        "vm_event_item",
        "zone_stat_item",
        "node_stat_item",
        "zone_watermarks",
    };

    for (size_t i = 0; i < sizeof(enum_names) / sizeof(enum_names[0]); ++i) {
        long numa_cnt = -1;
        enumerator_value(const_cast<char*>(enum_names[i]), &numa_cnt);
        enums.insert(std::make_pair(enum_names[i], numa_cnt));
    }
    for (const auto& enum_name : enum_list) {
        std::map<std::string, ulong> temp = read_enum_list(enum_name);
        enum_dict.insert(std::make_pair(enum_name, temp));
        enums.insert(temp.begin(), temp.end());
    }

    std::vector<std::string> param_names = {
        "vm_stat",  // For kernel which before 4.9
        "vm_node_stat",
        "vm_zone_stat",
        "vm_event_states",
        "vm_committed_as",
        "nr_cpu_ids",
        "nr_swapfiles",
        "nr_swap_pages",
        "nr_vmalloc_pages",
        "pcpu_nr_units",
        "pcpu_nr_populated",
        "swap_info",
        "_totalram_pages",
        "totalcma_pages",
        "total_swap_pages",
        "totalreserve_pages",
        "blockdev_superblock",
        "sysctl_overcommit_kbytes",
        "sysctl_overcommit_ratio",
        "node_data",
        "contig_page_data",
        "__cpu_online_mask",
        "__cpu_dying_mask",
        "__per_cpu_offset"
    };
    if (get_config_val("CONFIG_HUGETLB_PAGE") == "y"){
        param_names.push_back("hstates");
        param_names.push_back("hugetlb_max_hstate");
        param_names.push_back("default_hstate_idx");
    }

    for (const auto& name : param_names){
        ulong param_addr = 0x0;
        if (csymbol_exists(name.c_str())) {
            param_addr = csymbol_value(name.c_str());
        }
        g_param.insert(std::make_pair(name.c_str(), param_addr));
    }

    // Read full list from vm_node_stat
    long* node_page_list = (long *)read_memory(g_param["vm_node_stat"], sizeof(ulong) * enums["NR_VM_NODE_STAT_ITEMS"], "");
    if (get_config_val("CONFIG_SMP") == "y"){
        for (size_t i = 0; i < enums["NR_VM_NODE_STAT_ITEMS"]; ++i){
            node_page_list[i] = node_page_list[i]<0?0:node_page_list[i];
            node_page_state.push_back(node_page_list[i]);
        }
    }
    FREEBUF(node_page_list);

    // Read full list from vm_zone_stat/vm_stat
    ulong vm_stat_addr = (THIS_KERNEL_VERSION < LINUX(4, 9, 0))?g_param["vm_stat"]:g_param["vm_zone_stat"];
    long* zone_page_list = (long *)read_memory(vm_stat_addr, sizeof(ulong) * enums["NR_VM_ZONE_STAT_ITEMS"], "");
    if (get_config_val("CONFIG_SMP") == "y"){
        for (size_t i = 0; i < enums["NR_VM_ZONE_STAT_ITEMS"]; ++i){
            zone_page_list[i] = zone_page_list[i]<0?0:zone_page_list[i];
            zone_page_state.push_back(zone_page_list[i]);
        }
    }
    FREEBUF(zone_page_list);

    // Read full list from vm_event_states
    for (uint cpu = 0; cpu < read_uint(g_param["nr_cpu_ids"], ""); ++cpu) {
        std::vector<ulong> per_cpu_vm_event_page_state;
        ulong per_cpu_offset = read_ulong(g_param["__per_cpu_offset"] + sizeof(ulong)*cpu, "get __per_cpu_offset[cpu]");
        long* vm_event_page_list = (long *)read_memory(g_param["vm_event_states"] + per_cpu_offset, sizeof(ulong) * enums["NR_VM_EVENT_ITEMS"], "");
        if (get_config_val("CONFIG_SMP") == "y"){
            for (size_t i = 0; i < enums["NR_VM_EVENT_ITEMS"]; ++i){
                vm_event_page_list[i] = vm_event_page_list[i]<0?0:vm_event_page_list[i];
                per_cpu_vm_event_page_state.push_back(vm_event_page_list[i]);
            }
        }
        vm_event_page_state.push_back(per_cpu_vm_event_page_state);
        FREEBUF(vm_event_page_list);
    }
}

ulong Meminfo::get_wmark_low(void){
    ulong wmark_low = 0;
    for (int n = 0; n < vt->numnodes; n++) {
        struct node_table *nt = &vt->node_table[n];
        ulong node_zones = nt->pgdat + field_offset(pglist_data, node_zones);

        for (int i = 0; i < vt->nr_zones; i++) {
            ulong zone_addr = (node_zones + (i * struct_size(zone)));
            void *zone_buf = read_struct(zone_addr, "zone");
            ulong watermark_low_offset = field_offset(zone, _watermark) + enums["WMARK_LOW"] * sizeof(ulong);
            ulong watermark_low = ULONG(zone_buf + watermark_low_offset);
            ulong watermark_boost = ULONG(zone_buf + field_offset(zone, watermark_boost));
            wmark_low += (watermark_low + watermark_boost);
        }
    }
    return wmark_low;
}

ulong Meminfo::get_available(ulong freeram)
{
    ulong wmark_low = get_wmark_low();
    ulong totalreserveram = read_ulong(g_param["totalreserve_pages"], "read from totalreserve_pages");
    long available = freeram - totalreserveram;

    ulong pagecache = node_page_state[enums["NR_LRU_BASE"] + enums["LRU_ACTIVE_FILE"]]
         + node_page_state[enums["NR_LRU_BASE"] + enums["LRU_INACTIVE_FILE"]];
    pagecache -= std::min(pagecache / 2, wmark_low);
    available += pagecache;

    ulong reclaimable = node_page_state[enums["NR_KERNEL_MISC_RECLAIMABLE"]]
        + node_page_state[enums[(THIS_KERNEL_VERSION >= LINUX(5, 9, 0))?"NR_SLAB_RECLAIMABLE_B":"NR_SLAB_RECLAIMABLE"]];
    available += reclaimable - std::min(reclaimable / 2, wmark_low);

    return (available < 0)?0:available;
}

ulong Meminfo::get_blockdev_nr_pages(void){
    ulong list_addr = read_ulong(g_param["blockdev_superblock"], "blockdev_superblock") + field_offset(super_block, s_inodes);
    std::vector<ulong> inodes_list = for_each_list(list_addr, field_offset(inode, i_sb_list));
    long ret = 0;
    for (const auto& inode_addr : inodes_list) {
        ulong i_mapping_addr = read_ulong(inode_addr + field_offset(inode, i_mapping), "get i_mapping addr");
        ret += read_ulong(i_mapping_addr + field_offset(address_space, nrpages), "get nrpages");
    }
    return ret;
}

ulong Meminfo::get_to_be_unused_nr_pages(void){
    long ret = 0;
    ulong nr_swapfiles = read_uint(g_param["nr_swapfiles"], "read from nr_swapfiles");
    for (size_t type = 0; type < nr_swapfiles; type++){
        ulong swap_info = read_pointer(g_param["swap_info"] + type * sizeof(void *), "swap_info_struct addr");
        ulong swap_info_flag = read_ulong(swap_info + field_offset(swap_info_struct, flags), "get swap_info flag");
        if ((swap_info_flag & enums["SWP_USED"]) && !(swap_info_flag & enums["SWP_WRITEOK"])){
            ret += read_ulong(swap_info + field_offset(swap_info_struct, inuse_pages), "get swap_info inuse_pages");
        }
    }
    return ret;
}

ulong Meminfo::get_vm_commit_pages(ulong totalram_pg){
    long allowed = 0;
    ulong overcommit = read_ulong(g_param["sysctl_overcommit_kbytes"], "read from sysctl_overcommit_kbytes");
    if (overcommit){
        allowed = overcommit >> (PAGESHIFT() - 10);
    } else{
        ulong hugetlb_pg = 0;
        if (get_config_val("CONFIG_HUGETLB_PAGE") == "y") {
            int hugetlb_max_hstate = read_int(g_param["hugetlb_max_hstate"], "read hugetlb_max_hstate");
            for (int i=0; i < hugetlb_max_hstate; i++){
                ulong h = g_param["hstates"] + i * struct_size(hstate);
                ulong nr_huge_pages = read_ulong(h + field_offset(hstate, nr_huge_pages), "read nr_huge_pages");
                ulong page_per_huge_page = 1UL << read_ulong(h + field_offset(hstate, order), "read order");
                hugetlb_pg += nr_huge_pages + page_per_huge_page;
            }
        }
        allowed = (totalram_pg - hugetlb_pg) * read_int(g_param["sysctl_overcommit_ratio"], "") / 100;
    }
    allowed += read_long(g_param["nr_swap_pages"], "read nr_swap_pages");
    return allowed;
}

ulong Meminfo::get_mm_committed_pages(void){
    ulong allowed = read_ulong(g_param["vm_committed_as"] + field_offset(percpu_counter, count), "get percpu_counter count");
    std::string config_nr_cpus = get_config_val("CONFIG_NR_CPUS");
    ulong cpu_online_mask = read_ulong(g_param["__cpu_online_mask"], "get cpu_online_mask");
    uint nr_cpu_ids = read_uint(g_param["nr_cpu_ids"], "get nr_cpu_ids");
    for (uint cpu = 0; cpu < nr_cpu_ids; ++cpu) {
        if (cpu_online_mask & (1UL << cpu)) {
            ulong per_cpu_offset = read_ulong(g_param["__per_cpu_offset"] + sizeof(ulong)*cpu, "get __per_cpu_offset[cpu]");
            uint temp = read_uint(g_param["vm_committed_as"] + per_cpu_offset + field_offset(percpu_counter, counters),
                "get percpu_counter->counters for specific cpu");
            allowed += temp;
        }
    }
    return allowed;
}

size_t Meminfo::get_cma_size(){
    if (!csymbol_exists("cma_areas")){
        return 0;
    }
    ulong cma_areas_addr = csymbol_value("cma_areas");
    ulong cnt = read_ulong(csymbol_value("cma_area_count"),"cma_area_count");
    if (cnt == 0) {
        return 0;
    }
    size_t total_pages = 0;
    for (ulong i = 0; i < cnt; ++i) {
        ulong cma_addr = cma_areas_addr + i * struct_size(cma);
        total_pages += read_ulong(cma_addr + field_offset(cma,count), "count");
    }
    return total_pages * page_size;
}

size_t Meminfo::get_struct_page_size(){
    return (max_pfn - min_low_pfn) * struct_size(page);
}

size_t Meminfo::get_memory_size(){
    if (dts == nullptr){
        dts = std::make_shared<Devicetree>();  /* code */
    }
    size_t total_size = 0;
    for (const auto& range : dts->get_ddr_size()) {
        total_size += range.size;
    }
    return total_size;
}

size_t Meminfo::get_nomap_size(){
    if (!csymbol_exists("reserved_mem")){
        return 0;
    }
    ulong reserved_mem_addr = csymbol_value("reserved_mem");
    int reserved_mem_count = read_pointer(csymbol_value("reserved_mem_count"),"reserved_mem_count");
    int cnt = reserved_mem_count == 0 ? get_array_length(TO_CONST_STRING("reserved_mem"), NULL, 0) : reserved_mem_count;
    if (cnt == 0) {
        return 0;
    }
    size_t total_size = 0;
    for (int i = 0; i < cnt; ++i) {
        ulong reserved_addr = reserved_mem_addr + i * struct_size(reserved_mem);
        size_t mem_size = read_ulong(reserved_addr + field_offset(reserved_mem,size), "size");
        if (mem_size == 0){
            continue;
        }
        ulong name_addr = read_pointer(reserved_addr + field_offset(reserved_mem,name),"name");
        if (!is_kvaddr(name_addr)) continue;
        std::string name = read_cstring(name_addr,64, "reserved_mem_name");
        if (name.empty()) continue;
        std::vector<std::shared_ptr<device_node>> nodes = dts->find_node_by_name(name);
        if (nodes.size() == 0)continue;
        for (const auto& node : nodes) {
            std::shared_ptr<Property> prop = dts->getprop(node->addr,"no-map");
            if (prop.get() != nullptr){
                total_size += mem_size ;
            }
        }
    }
    return total_size;
}

size_t Meminfo::get_dmabuf_size(){
    ulong db_list_addr = 0;
    if (csymbol_exists("db_list")){
        db_list_addr = csymbol_value("db_list");
    }else if (csymbol_exists("debugfs_list")){
        db_list_addr = csymbol_value("debugfs_list");
    }
    if (!is_kvaddr(db_list_addr)){
        return 0;
    }
    size_t total_size = 0;
    int offset = field_offset(dma_buf,list_node);
    for (const auto& buf_addr : for_each_list(db_list_addr,offset)) {
        total_size += read_ulong(buf_addr + field_offset(dma_buf,size), "size");
    }
    return total_size;
}

size_t Meminfo::get_vmalloc_size(){
    size_t total_pages = 0;
    int offset = field_offset(vmap_area,list);
    if (csymbol_exists("vmap_area_list")){
        ulong area_list_addr = csymbol_value("vmap_area_list");
        if (!is_kvaddr(area_list_addr)) {
            return total_pages;
        }
        for (const auto& area_addr : for_each_list(area_list_addr,offset)) {
            total_pages += parser_vmap_area(area_addr);
        }
    }else if(csymbol_exists("vmap_nodes")){
        ulong nodes_addr = read_pointer(csymbol_value("vmap_nodes"),"vmap_nodes pages");
        if (!is_kvaddr(nodes_addr)) return total_pages;
        int nr_node = read_int(csymbol_value("nr_vmap_nodes"),"nr_vmap_nodes");
        int pool_cnt = field_size(vmap_node,pool)/struct_size(vmap_pool);
        for (int i = 0; i < nr_node; i++){
            ulong pools_addr = nodes_addr + i * struct_size(vmap_node) + field_offset(vmap_node,pool);
            for (int p = 0; p < pool_cnt; p++){
                ulong pool_addr = pools_addr + p * struct_size(vmap_pool);
                if (!is_kvaddr(pool_addr)) continue;
                ulong len = read_ulong(pool_addr + field_offset(vmap_pool,len),"vmap_pool len");
                if (len == 0){
                    continue;
                }
                for (const auto& area_addr : for_each_list(pool_addr,offset)) {
                    total_pages += parser_vmap_area(area_addr);
                }
            }
        }
    }
    return total_pages * page_size;
}

size_t Meminfo::parser_vmap_area(ulong addr){
    size_t total_pages = 0;
    ulong vm_addr = read_pointer(addr + field_offset(vmap_area,vm),"vm addr");
    while (is_kvaddr(vm_addr)){
        int nr_pages = read_uint(vm_addr + field_offset(vm_struct,nr_pages),"nr_pages");
        ulong vm_size = read_ulong(vm_addr + field_offset(vm_struct,size),"size");
        if (vm_size % page_size != 0 || (vm_size / page_size) != (ulong)(nr_pages + 1)) {
            break;
        }
        total_pages += nr_pages;
        vm_addr = read_pointer(vm_addr + field_offset(vm_struct,next),"next");
    }
    return total_pages;
}

size_t Meminfo::get_dentry_cache_size(){
    if (!csymbol_exists("d_hash_shift")) {
        return 0;
    }
    uint d_hash_shift;
    get_symbol_data(TO_CONST_STRING("d_hash_shift"), sizeof(int), &d_hash_shift);
    d_hash_shift = 32 - d_hash_shift;
    return struct_size(hlist_bl_head) << d_hash_shift;
}

size_t Meminfo::get_inode_cache_size(){
    if (!csymbol_exists("i_hash_shift")) {
        return 0;
    }
    uint i_hash_shift;
    get_symbol_data(TO_CONST_STRING("i_hash_shift"), sizeof(int), &i_hash_shift);
    i_hash_shift = 32 - i_hash_shift;
    return struct_size(hlist_head) << i_hash_shift;
}

ulong Meminfo::get_vmalloc_total(void){
    if (get_config_val("CONFIG_MMU") == "y") {
        std::string config_arm64_va_bits = get_config_val("CONFIG_ARM64_VA_BITS");
        ulong arm64_va_bits = isNumber(config_arm64_va_bits)?std::stoi(config_arm64_va_bits):39;   // Default:39
        ulong struct_page_max_shift = static_cast<ulong>(std::ceil(std::log2(struct_size(page))));
        ulong vmemmap_shift = PAGESHIFT() - struct_page_max_shift;
        ulong vmemmap_start = -(1UL << (arm64_va_bits - vmemmap_shift));
        ulong vmalloc_end = vmemmap_start - 0x10000000;     // SZ_256M

        ulong modules_vaddr = -(1UL << ((arm64_va_bits > 48 ? 48 : arm64_va_bits) - 1));
        // SZ_2G: 0x80000000,  SZ_128M: 0x80000000
        ulong modules_vsize = (THIS_KERNEL_VERSION >= LINUX(6, 5, 0))?0x80000000:0x08000000;
        ulong vmalloc_start = modules_vaddr + modules_vsize;

        return (vmalloc_end - vmalloc_start);
    } else {
        return 0;
    }
}

void Meminfo::print_vmstat(void){
    std::ostringstream oss;
    std::map<std::string, ulong> vm_event_enum_list = read_enum_list("vm_event_item");
    ulong nr_zone_base = (get_config_val("CONFIG_NUMA") == "y")?read_ulong(g_param["node_data"],""):g_param["contig_page_data"];

    for (size_t i = 0; i < enums["__MAX_NR_ZONES"]; i++) {
        if (nr_zone_base == 0x0) break;
        ulong zone_addr = nr_zone_base + field_offset(pglist_data, node_zones) + i * struct_size(zone);
        if (zone_addr == 0x0)   continue;
        LOGD("zone[%d]: zone_addr: 0x%lx\n", i, zone_addr);
        ulong present_pg = read_ulong(zone_addr + field_offset(zone, present_pages), "read from present_pages");
        LOGD("zone[%d]: present_pg: %ld\n", i, present_pg);
        if (present_pg > 0) {
            char* zone_name = (char *)read_memory(read_ulong(zone_addr + field_offset(zone, name), ""), 12, "");
            LOGD("zone[%d]: zone_name : %s, len:%d\n", i, zone_name, strlen(zone_name));
            ulong spanned_pg = read_ulong(zone_addr + field_offset(zone, spanned_pages), "read from spanned_pages");
            ulong managed_pg = read_ulong(zone_addr + field_offset(zone, managed_pages), "read from managed_pages");
            ulong cma_pg = read_ulong(zone_addr + field_offset(zone, cma_pages), "read from cma_pages");

            oss << std::left << "Zone (" << zone_name << ")" << std::setw(29 - strlen(zone_name)) << std::right << " Pages |"
                << std::setw(12) << std::right << "Size\n"
                << std::left << "present_pages:        " << std::setw(12) << std::right << present_pg << " |"
                << std::setw(12) << std::right << csize((uint64_t)present_pg*page_size, MB, 1) << "\n"
                << std::left << "spanned_pages:        " << std::setw(12) << std::right << spanned_pg << " |"
                << std::setw(12) << std::right << csize((uint64_t)spanned_pg*page_size, MB, 1) << "\n"
                << std::left << "managed_pages:        " << std::setw(12) << std::right << managed_pg << " |"
                << std::setw(12) << std::right << csize((uint64_t)managed_pg*page_size, MB, 1) << "\n"
                << std::left << "cma_pages:            " << std::setw(12) << std::right << cma_pg << " |"
                << std::setw(12) << std::right << csize((uint64_t)cma_pg*page_size, MB, 1) << "\n";
            FREEBUF(zone_name);

            ulong wtmark_offset = (THIS_KERNEL_VERSION < LINUX(4, 19, 0))?field_offset(zone, watermark):field_offset(zone, _watermark);
            for (const auto& pair : enum_dict["zone_watermarks"]) {
                if (pair.second == enums["NR_WMARK"])   continue;
                ulong wt_pg = read_ulong(zone_addr + wtmark_offset + sizeof(ulong)*pair.second, "read from watermark pages");
                oss << std::left << pair.first << ":" << std::setw(33 - pair.first.length()) << std::right << wt_pg << " |"
                    << std::setw(12) << std::right << csize((uint64_t)wt_pg*page_size, MB, 1) << "\n";
            }

            for (const auto& pair : enum_dict["zone_stat_item"]) {
                if (pair.second == enums["NR_VM_ZONE_STAT_ITEMS"])   continue;
                ulong zone_item_pg = read_ulong(zone_addr + field_offset(zone, vm_stat) + sizeof(ulong)*pair.second, "read from vm_stat");
                oss << std::left << pair.first << ":" << std::setw(33 - pair.first.length()) << std::right << zone_item_pg << " |"
                    << std::setw(12) << std::right << csize((uint64_t)zone_item_pg*page_size, MB, 1) << "\n";
            }
            oss << "\n";
        }
    }

    oss << std::left << "Global Stats" << std::setw(24) << std::right << " Pages |" << std::setw(12) << std::right << "Size\n";
    for (const auto& pair : enum_dict["zone_stat_item"]) {
        if (pair.second == enums["NR_VM_ZONE_STAT_ITEMS"])   continue;
        ulong zone_item_pg = zone_page_state[enums[pair.first]];
        oss << std::left << pair.first << ":" << std::setw(33 - pair.first.length()) << std::right << zone_item_pg << " |"
            << std::setw(12) << std::right << csize((uint64_t)zone_item_pg*page_size, MB, 1) << "\n";
    }
    oss << "\n";
    oss << std::left << "Node Stats" << std::setw(26) << std::right << " Pages |" << std::setw(12) << std::right << "Size\n";
    for (const auto& pair : enum_dict["node_stat_item"]) {
        if (pair.second == enums["NR_VM_NODE_STAT_ITEMS"])   continue;
        ulong node_item_pg = node_page_state[enums[pair.first]];
        oss << std::left << pair.first << ":" << std::setw(33 - pair.first.length()) << std::right << node_item_pg << " |"
            << std::setw(12) << std::right << csize((uint64_t)node_item_pg*page_size, MB, 1) << "\n";
    }
    oss << "\n";
    oss << std::left << "VM EVENT Stats" << std::setw(22) << std::right << " Pages |" << std::setw(12) << std::right << "Size\n";
    for (const auto& pair : enum_dict["vm_event_item"]) {
        if (pair.second == enums["NR_VM_EVENT_ITEMS"])   continue;
        ulong vm_event_item_pg = 0;
        for (uint cpu = 0; cpu < read_uint(g_param["nr_cpu_ids"], ""); ++cpu) {
            vm_event_item_pg += vm_event_page_state[cpu][enums[pair.first]];
        }
        oss << std::left << pair.first << ":" << std::setw(33 - pair.first.length()) << std::right << vm_event_item_pg << " |"
            << std::setw(12) << std::right << csize((uint64_t)vm_event_item_pg*page_size, MB, 1) << "\n";
    }
    oss << "\n";
    PRINT("%s \n",oss.str().c_str());
}

void Meminfo::print_mem_breakdown(void){
    ulong totalram_pg = read_ulong(g_param["_totalram_pages"], "read from _totalram_pages");
    ulong freeram_pg = zone_page_state[enums["NR_FREE_PAGES"]];
    ulong blockdev_pg = get_blockdev_nr_pages();
    ulong swap_cached_pg = node_page_state[enums["NR_SWAPCACHE"]];
    ulong cached_pg = node_page_state[enums["NR_FILE_PAGES"]] - swap_cached_pg - blockdev_pg;
    ulong totalmm_pg = get_memory_size();
    ulong no_hlos = get_nomap_size();
    ulong kernel_code = csymbol_value("_sinittext") - csymbol_value("_text");
    ulong kernel_data = csymbol_value("_end") - csymbol_value("_sdata");
    ulong active_aon_pg = node_page_state[enums["NR_LRU_BASE"] + enums["LRU_ACTIVE_ANON"]];
    ulong inactive_aon_pg = node_page_state[enums["NR_LRU_BASE"] + enums["LRU_INACTIVE_ANON"]];
    ulong active_file_pg = node_page_state[enums["NR_LRU_BASE"] + enums["LRU_ACTIVE_FILE"]];
    ulong inactive_file_pg = node_page_state[enums["NR_LRU_BASE"] + enums["LRU_INACTIVE_FILE"]];
    ulong aon_pg = active_aon_pg + inactive_aon_pg;
    ulong file_pg = active_file_pg + inactive_file_pg;
    ulong sharedram_pg = node_page_state[enums["NR_SHMEM"]];
    ulong slab_reclaimable_pg = node_page_state[enums[(THIS_KERNEL_VERSION >= LINUX(5, 9, 0))?"NR_SLAB_RECLAIMABLE_B":"NR_SLAB_RECLAIMABLE"]];
    ulong slab_unreclaim_pg = node_page_state[enums[(THIS_KERNEL_VERSION >= LINUX(5, 9, 0))?"NR_SLAB_UNRECLAIMABLE_B":"NR_SLAB_UNRECLAIMABLE"]];
    ulong slab_pg = slab_unreclaim_pg + slab_reclaimable_pg;
    ulong nr_ks = enums["NR_KERNEL_STACK_KB"];
    ulong kernelstack_bytes = ((THIS_KERNEL_VERSION >= LINUX(5, 9, 0))?node_page_state[nr_ks]:zone_page_state[nr_ks]) * 1024;
    ulong nr_pt = enums["NR_PAGETABLE"];
    ulong pagetables_pg = (THIS_KERNEL_VERSION >= LINUX(5, 11, 0))?node_page_state[nr_pt]:zone_page_state[nr_pt];
    ulong struct_page = get_struct_page_size();
    ulong dentry_cache = get_dentry_cache_size();
    ulong inode_cache = get_inode_cache_size();
    ulong vmalloc = get_vmalloc_size();
    ulong dmabuf = get_dmabuf_size();
    ulong other_size = totalram_pg * page_size - (freeram_pg + blockdev_pg + cached_pg) * page_size
        - (slab_pg * page_size + vmalloc + sharedram_pg * page_size + pagetables_pg * page_size + kernelstack_bytes)
        - aon_pg * page_size;
    std::ostringstream oss;
    oss << std::left << "RAM                :" << std::setw(12) << std::right << csize((uint64_t)totalmm_pg)                  << "\n"
        << std::left << "   MemTotal        :" << std::setw(12) << std::right << csize((uint64_t)totalram_pg * page_size)     << "\n"
        << std::left << "   MemFree         :" << std::setw(12) << std::right << csize((uint64_t)freeram_pg * page_size)      << "\n"
        << std::left << "   Buffers         :" << std::setw(12) << std::right << csize((uint64_t)blockdev_pg * page_size)     << "\n"
        << std::left << "   Cached          :" << std::setw(12) << std::right << csize((uint64_t)cached_pg * page_size)       << "\n"
        << std::left << "   SwapCached      :" << std::setw(12) << std::right << csize((uint64_t)swap_cached_pg * page_size)  << "\n"
        << std::left << "   AonPage         :" << std::setw(12) << std::right << csize((uint64_t)aon_pg * page_size)          << "\n"
        << std::left << "   FilePage        :" << std::setw(12) << std::right << csize((uint64_t)file_pg * page_size)         << "\n"
        << std::left << "   Slab            :" << std::setw(12) << std::right << csize((uint64_t)slab_pg * page_size)         << "\n"
        << std::left << "   KernelStack     :" << std::setw(12) << std::right << csize((uint64_t)kernelstack_bytes)           << "\n"
        << std::left << "   PageTables      :" << std::setw(12) << std::right << csize((uint64_t)pagetables_pg * page_size)   << "\n"
        << std::left << "   Shmem           :" << std::setw(12) << std::right << csize((uint64_t)sharedram_pg * page_size)    << "\n"
        << std::left << "   Cma             :" << std::setw(12) << std::right << csize((uint64_t)get_cma_size())              << "\n"
        << std::left << "   Dmabuf          :" << std::setw(12) << std::right << csize((uint64_t)dmabuf)                      << "\n"
        << std::left << "   Vmalloc         :" << std::setw(12) << std::right << csize((uint64_t)vmalloc)                     << "\n"
        << std::left << "   Other           :" << std::setw(12) << std::right << csize((uint64_t)other_size)                  << "\n\n"
        << std::left << "   Struct Page     :" << std::setw(12) << std::right << csize((uint64_t)struct_page)                 << "\n"
        << std::left << "   Kernel Code     :" << std::setw(12) << std::right << csize((uint64_t)kernel_code)                 << "\n"
        << std::left << "   Kernel Data     :" << std::setw(12) << std::right << csize((uint64_t)kernel_data)                 << "\n"
        << std::left << "   Dentry Cache    :" << std::setw(12) << std::right << csize((uint64_t)dentry_cache)                << "\n"
        << std::left << "   Inode  Cache    :" << std::setw(12) << std::right << csize((uint64_t)inode_cache)                 << "\n\n"
        << std::left << "   NON_HLOS        :" << std::setw(12) << std::right << csize((uint64_t)no_hlos)                     << "\n";
    PRINT("%s \n",oss.str().c_str());
}

void Meminfo::print_meminfo(void){
    ulong totalram_pg = read_ulong(g_param["_totalram_pages"], "read from _totalram_pages");
    ulong freeram_pg = zone_page_state[enums["NR_FREE_PAGES"]];
    ulong available_pg = get_available(freeram_pg);
    ulong blockdev_pg = get_blockdev_nr_pages();
    ulong swap_cached_pg = node_page_state[enums["NR_SWAPCACHE"]];
    ulong cached_pg = node_page_state[enums["NR_FILE_PAGES"]] - swap_cached_pg - blockdev_pg;
    ulong active_aon_pg = node_page_state[enums["NR_LRU_BASE"] + enums["LRU_ACTIVE_ANON"]];
    ulong inactive_aon_pg = node_page_state[enums["NR_LRU_BASE"] + enums["LRU_INACTIVE_ANON"]];
    ulong active_file_pg = node_page_state[enums["NR_LRU_BASE"] + enums["LRU_ACTIVE_FILE"]];
    ulong inactive_file_pg = node_page_state[enums["NR_LRU_BASE"] + enums["LRU_INACTIVE_FILE"]];
    ulong active_pg = active_aon_pg + active_file_pg;
    ulong inactive_pg = inactive_aon_pg + inactive_file_pg;
    ulong unevictable_pg = node_page_state[enums["NR_LRU_BASE"] + enums["LRU_UNEVICTABLE"]];
    ulong mlocked_pg = zone_page_state[enums["NR_MLOCK"]];
    ulong totalswap_pg = get_to_be_unused_nr_pages() + read_ulong(g_param["total_swap_pages"], "read total_swap_pages");
    ulong freeswap_pg = get_to_be_unused_nr_pages() + read_ulong(g_param["nr_swap_pages"], "read nr_swap_pages");
    ulong dirty_pg = node_page_state[enums["NR_FILE_DIRTY"]];
    ulong writeback_pg = node_page_state[enums["NR_WRITEBACK"]];
    ulong anon_pg = node_page_state[enums["NR_ANON_MAPPED"]];
    ulong mapped_pg = node_page_state[enums["NR_FILE_MAPPED"]];
    ulong sharedram_pg = node_page_state[enums["NR_SHMEM"]];
    ulong slab_reclaimable_pg = node_page_state[enums[(THIS_KERNEL_VERSION >= LINUX(5, 9, 0))?"NR_SLAB_RECLAIMABLE_B":"NR_SLAB_RECLAIMABLE"]];
    ulong slab_unreclaim_pg = node_page_state[enums[(THIS_KERNEL_VERSION >= LINUX(5, 9, 0))?"NR_SLAB_UNRECLAIMABLE_B":"NR_SLAB_UNRECLAIMABLE"]];
    ulong kreclaimable_pg = slab_reclaimable_pg + node_page_state[enums["NR_KERNEL_MISC_RECLAIMABLE"]];
    ulong slab_pg = slab_unreclaim_pg + slab_reclaimable_pg;
    ulong nr_ks = enums["NR_KERNEL_STACK_KB"];
    ulong kernelstack_bytes = ((THIS_KERNEL_VERSION >= LINUX(5, 9, 0))?node_page_state[nr_ks]:zone_page_state[nr_ks]) * 1024;
    ulong nr_pt = enums["NR_PAGETABLE"];
    ulong pagetables_pg = (THIS_KERNEL_VERSION >= LINUX(5, 11, 0))?node_page_state[nr_pt]:zone_page_state[nr_pt];
    ulong shadowstack_bytes = 0;
    if (get_config_val("CONFIG_SHADOW_CALL_STACK") == "y") {
        shadowstack_bytes = node_page_state[enums["NR_KERNEL_SCS_KB"]] * 1024;
    }
    ulong bounce_pg = zone_page_state[enums["NR_BOUNCE"]];

    ulong writebacktmp_pg = node_page_state[enums["NR_WRITEBACK_TEMP"]];
    ulong vm_commit_pg = get_vm_commit_pages(totalram_pg);
    // ulong mm_committed_pg = get_mm_committed_pages();
    ulong vmalloc_total_bytes = get_vmalloc_total();
    ulong vmalloc_used_pg = read_ulong(g_param["nr_vmalloc_pages"], "");
    ulong pcpu_nr_pg = read_uint(g_param["pcpu_nr_units"], "nr_units") * read_ulong(g_param["pcpu_nr_populated"], "");
    ulong anon_huge_pg = 0, shmem_huge_pg = 0, shmem_pmd_mapped_pg = 0, file_huge_pg = 0, file_pmd_mapped_pg = 0;
    if (get_config_val("CONFIG_TRANSPARENT_HUGEPAGE") == "y") {
        anon_huge_pg = node_page_state[enums["NR_ANON_THPS"]];
        shmem_huge_pg = node_page_state[enums["NR_SHMEM_THPS"]];
        shmem_pmd_mapped_pg = node_page_state[enums["NR_SHMEM_PMDMAPPED"]];
        file_huge_pg = node_page_state[enums["NR_FILE_THPS"]];
        file_pmd_mapped_pg = node_page_state[enums["NR_FILE_PMDMAPPED"]];
    }
    ulong cma_total_pg = 0, cma_free_pg = 0;
    if (get_config_val("CONFIG_CMA") == "y") {
        cma_total_pg = read_ulong(g_param["totalcma_pages"], "totalcma_pages");
        cma_free_pg = zone_page_state[enums["NR_FREE_CMA_PAGES"]];
    }

    std::ostringstream oss;
    oss << std::left << "MemTotal:       " << std::setw(12) << std::right << csize((uint64_t)totalram_pg * page_size, KB, 0)     << " | " << std::right << std::setw(12) << csize((uint64_t)totalram_pg * page_size) << "\n"
        << std::left << "MemFree:        " << std::setw(12) << std::right << csize((uint64_t)freeram_pg * page_size, KB, 0)      << " | " << std::right << std::setw(12) << csize((uint64_t)freeram_pg * page_size) << "\n"
        << std::left << "MemAvailable:   " << std::setw(12) << std::right << csize((uint64_t)available_pg * page_size, KB, 0)    << " | " << std::right << std::setw(12) << csize((uint64_t)available_pg * page_size) << "\n"
        << std::left << "Buffers:        " << std::setw(12) << std::right << csize((uint64_t)blockdev_pg * page_size, KB, 0)     << " | " << std::right << std::setw(12) << csize((uint64_t)blockdev_pg * page_size) << "\n"
        << std::left << "Cached:         " << std::setw(12) << std::right << csize((uint64_t)cached_pg * page_size, KB, 0)       << " | " << std::right << std::setw(12) << csize((uint64_t)cached_pg * page_size) << "\n"
        << std::left << "SwapCached:     " << std::setw(12) << std::right << csize((uint64_t)swap_cached_pg * page_size, KB, 0)  << " | " << std::right << std::setw(12) << csize((uint64_t)swap_cached_pg * page_size) << "\n"
        << std::left << "Active:         " << std::setw(12) << std::right << csize((uint64_t)active_pg * page_size, KB, 0)       << " | " << std::right << std::setw(12) << csize((uint64_t)active_pg * page_size) << "\n"
        << std::left << "Inactive:       " << std::setw(12) << std::right << csize((uint64_t)inactive_pg * page_size, KB, 0)     << " | " << std::right << std::setw(12) << csize((uint64_t)inactive_pg * page_size) << "\n"
        << std::left << "Active(anon):   " << std::setw(12) << std::right << csize((uint64_t)active_aon_pg * page_size, KB, 0)   << " | " << std::right << std::setw(12) << csize((uint64_t)active_aon_pg * page_size) << "\n"
        << std::left << "Inactive(anon): " << std::setw(12) << std::right << csize((uint64_t)inactive_aon_pg * page_size, KB, 0) << " | " << std::right << std::setw(12) << csize((uint64_t)inactive_aon_pg * page_size) << "\n"
        << std::left << "Active(file):   " << std::setw(12) << std::right << csize((uint64_t)active_file_pg * page_size, KB, 0)  << " | " << std::right << std::setw(12) << csize((uint64_t)active_file_pg * page_size) << "\n"
        << std::left << "Inactive(file): " << std::setw(12) << std::right << csize((uint64_t)inactive_file_pg * page_size, KB, 0)<< " | " << std::right << std::setw(12) << csize((uint64_t)inactive_file_pg * page_size) << "\n"
        << std::left << "Unevictable:    " << std::setw(12) << std::right << csize((uint64_t)unevictable_pg * page_size, KB, 0)  << " | " << std::right << std::setw(12) << csize((uint64_t)unevictable_pg * page_size) << "\n"
        << std::left << "Mlocked:        " << std::setw(12) << std::right << csize((uint64_t)mlocked_pg * page_size, KB, 0)      << " | " << std::right << std::setw(12) << csize((uint64_t)mlocked_pg * page_size) << "\n"
        << std::left << "SwapTotal:      " << std::setw(12) << std::right << csize((uint64_t)totalswap_pg * page_size, KB, 0)    << " | " << std::right << std::setw(12) << csize((uint64_t)totalswap_pg * page_size) << "\n"
        << std::left << "SwapFree:       " << std::setw(12) << std::right << csize((uint64_t)freeswap_pg * page_size, KB, 0)     << " | " << std::right << std::setw(12) << csize((uint64_t)freeswap_pg * page_size) << "\n"
        << std::left << "Dirty:          " << std::setw(12) << std::right << csize((uint64_t)dirty_pg * page_size, KB, 0)        << " | " << std::right << std::setw(12) << csize((uint64_t)dirty_pg * page_size) << "\n"
        << std::left << "Writeback:      " << std::setw(12) << std::right << csize((uint64_t)writeback_pg * page_size, KB, 0)    << " | " << std::right << std::setw(12) << csize((uint64_t)writeback_pg * page_size) << "\n"
        << std::left << "AnonPages:      " << std::setw(12) << std::right << csize((uint64_t)anon_pg * page_size, KB, 0)         << " | " << std::right << std::setw(12) << csize((uint64_t)anon_pg * page_size) << "\n"
        << std::left << "Mapped:         " << std::setw(12) << std::right << csize((uint64_t)mapped_pg * page_size, KB, 0)       << " | " << std::right << std::setw(12) << csize((uint64_t)mapped_pg * page_size) << "\n"
        << std::left << "Shmem:          " << std::setw(12) << std::right << csize((uint64_t)sharedram_pg * page_size, KB, 0)    << " | " << std::right << std::setw(12) << csize((uint64_t)sharedram_pg * page_size) << "\n"
        << std::left << "KReclaimable:   " << std::setw(12) << std::right << csize((uint64_t)kreclaimable_pg * page_size, KB, 0) << " | " << std::right << std::setw(12) << csize((uint64_t)kreclaimable_pg * page_size) << "\n"
        << std::left << "Slab:           " << std::setw(12) << std::right << csize((uint64_t)slab_pg * page_size, KB, 0)         << " | " << std::right << std::setw(12) << csize((uint64_t)slab_pg * page_size) << "\n"
        << std::left << "SReclaimable:   " << std::setw(12) << std::right << csize((uint64_t)slab_reclaimable_pg * page_size, KB, 0) << " | " << std::right << std::setw(12) << csize((uint64_t)slab_reclaimable_pg * page_size) << "\n"
        << std::left << "SUnreclaim:     " << std::setw(12) << std::right << csize((uint64_t)slab_unreclaim_pg * page_size, KB, 0)   << " | " << std::right << std::setw(12) << csize((uint64_t)slab_unreclaim_pg * page_size) << "\n"
        << std::left << "KernelStack:    " << std::setw(12) << std::right << csize((uint64_t)kernelstack_bytes, KB, 0)               << " | " << std::right << std::setw(12) << csize((uint64_t)kernelstack_bytes) << "\n";
    if (get_config_val("CONFIG_SHADOW_CALL_STACK") == "y") {
        oss << std::left << "ShadowCallStack:" << std::setw(12) << std::right << csize((uint64_t)shadowstack_bytes, KB, 0)        << " | " << std::right << std::setw(12) << csize((uint64_t)shadowstack_bytes) << "\n";
    }
    oss << std::left << "PageTables:     " << std::setw(12) << std::right << csize((uint64_t)pagetables_pg * page_size, KB, 0)    << " | " << std::right << std::setw(12) << csize((uint64_t)pagetables_pg * page_size) << "\n"
        << std::left << "NFS_Unstable:   " << std::setw(12) << std::right << csize((uint64_t)0, KB, 0)                            << " | " << std::right << std::setw(12) << csize((uint64_t)0) << "\n"
        << std::left << "Bounce:         " << std::setw(12) << std::right << csize((uint64_t)bounce_pg * page_size, KB, 0)        << " | " << std::right << std::setw(12) << csize((uint64_t)bounce_pg * page_size) << "\n"
        << std::left << "WritebackTmp:   " << std::setw(12) << std::right << csize((uint64_t)writebacktmp_pg * page_size, KB, 0)  << " | " << std::right << std::setw(12) << csize((uint64_t)writebacktmp_pg * page_size) << "\n"
        << std::left << "CommitLimit:    " << std::setw(12) << std::right << csize((uint64_t)vm_commit_pg * page_size, KB, 0)     << " | " << std::right << std::setw(12) << csize((uint64_t)vm_commit_pg * page_size) << "\n"
        << std::left << "VmallocTotal:   " << std::setw(12) << std::right << csize((uint64_t)vmalloc_total_bytes, KB, 0)          << " | " << std::right << std::setw(12) << csize((uint64_t)vmalloc_total_bytes) << "\n"
        << std::left << "VmallocUsed:    " << std::setw(12) << std::right << csize((uint64_t)vmalloc_used_pg * page_size, KB, 0)  << " | " << std::right << std::setw(12) << csize((uint64_t)vmalloc_used_pg * page_size) << "\n"
        << std::left << "VmallocChunk:   " << std::setw(12) << std::right << csize((uint64_t)0, KB, 0)                            << " | " << std::right << std::setw(12) << csize((uint64_t)0) << "\n"
        << std::left << "Percpu:         " << std::setw(12) << std::right << csize((uint64_t)pcpu_nr_pg * page_size, KB, 0)       << " | " << std::right << std::setw(12) << csize((uint64_t)pcpu_nr_pg * page_size) << "\n";
    if (get_config_val("CONFIG_TRANSPARENT_HUGEPAGE") == "y") {
        oss << std::left << "AnonHugePages:  " << std::setw(12) << std::right << csize((uint64_t)anon_huge_pg * page_size, KB, 0)        << " | " << std::right << std::setw(12) << csize((uint64_t)anon_huge_pg * page_size) << "\n"
            << std::left << "ShmemHugePages: " << std::setw(12) << std::right << csize((uint64_t)shmem_huge_pg * page_size, KB, 0)       << " | " << std::right << std::setw(12) << csize((uint64_t)shmem_huge_pg * page_size) << "\n"
            << std::left << "ShmemPmdMapped: " << std::setw(12) << std::right << csize((uint64_t)shmem_pmd_mapped_pg * page_size, KB, 0) << " | " << std::right << std::setw(12) << csize((uint64_t)shmem_pmd_mapped_pg * page_size) << "\n"
            << std::left << "FileHugePages:  " << std::setw(12) << std::right << csize((uint64_t)file_huge_pg * page_size, KB, 0)        << " | " << std::right << std::setw(12) << csize((uint64_t)file_huge_pg * page_size) << "\n"
            << std::left << "FilePmdMapped:  " << std::setw(12) << std::right << csize((uint64_t)file_pmd_mapped_pg * page_size, KB, 0)  << " | " << std::right << std::setw(12) << csize((uint64_t)file_pmd_mapped_pg * page_size) << "\n";
    }
    if (get_config_val("CONFIG_CMA") == "y") {
        oss << std::left << "CmaTotal:       " << std::setw(12) << std::right << csize((uint64_t)cma_total_pg * page_size, KB, 0)  << " | " << std::right << std::setw(12) << csize((uint64_t)cma_total_pg * page_size) << "\n"
            << std::left << "CmaFree:        " << std::setw(12) << std::right << csize((uint64_t)cma_free_pg * page_size, KB, 0)   << " | " << std::right << std::setw(12) << csize((uint64_t)cma_free_pg * page_size) << "\n";
    }
    PRINT("%s \n",oss.str().c_str());
}

#pragma GCC diagnostic pop
