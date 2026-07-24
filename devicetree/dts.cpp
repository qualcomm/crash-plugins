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

#include "dts.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(Dts)
#endif

void Dts::cmd_main(void) {
    if (argcnt < 2) {
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }

    if (root_node == nullptr) {
        root_node = read_node("", root_addr);
    }

    int c;
    int argerrs = 0;

    while ((c = getopt(argcnt, args, "afb:s:m")) != EOF) {
        switch(c) {
            case 'a':
                print_node(root_node, 0, DTS_SHOW);
                break;
            case 'f':
                print_node(root_node, 0, DTS_SHOW | DTS_ADDR);
                break;
            case 'b': {
                std::string dtb_path(optarg);
                read_dtb(dtb_path);
                break;
            }
            case 's': {
                const int flags = DTS_SHOW | DTS_ADDR;
                std::string node_arg(optarg);
                if (isNumber(node_arg)) {
                    unsigned long addr = std::stoul(node_arg, nullptr, 16);
                    if (is_kvaddr(addr)) {
                        if (auto node_ptr = find_node_by_addr(addr)) {
                            print_node(node_ptr, flags);
                        }
                    } else {
                        LOGE("invalid address %lx\n", addr);
                    }
                } else {
                    auto node_list = find_node_by_name(node_arg);
                    for (const auto& node_ptr : node_list) {
                        print_node(node_ptr, flags);
                    }
                }
                break;
            }
            case 'm':
                print_ddr_info();
                break;

            default:
                argerrs++;
                break;
        }
    }
    if (argerrs) {
        cmd_usage(pc->curcmd, SYNOPSIS);
    }
}

void Dts::init_offset(void) {}

void Dts::init_command(void) {
    cmd_name = "dts";
    help_str_list={
        "dts",                                /* command name */
        "display device tree information",    /* short description */
        "[-a] [-f] [-b path] [-s name] [-m]\n"
        "  This command displays device tree (DTS) information.\n"
        "\n"
        "    -a              display whole device tree\n"
        "    -f              display whole device tree with addresses\n"
        "    -b path         read out the whole dtb memory to file\n"
        "    -s name         display one node info by node name or node address\n"
        "    -m              display physical memory ranges\n",
        "\n",
        "EXAMPLES",
        "  Display whole device tree:",
        "    %s> dts -a",
        "       memory{",
        "           ddr_device_hbb_ch0_rank0=< d >;",
        "           ddr_device_rank_ch0=< 1 >;",
        "           ddr_device_type=< 7 >;",
        "           device_type=<memory>;",
        "       };",
        "\n",
        "  Display whole device tree with addresses:",
        "    %s> dts -f",
        "       ffffff806f28a458:memory{",
        "           ffffff806f28a5b8:ddr_device_hbb_ch0_rank0=< d >;",
        "           ffffff806f28a618:ddr_device_rank_ch0=< 1 >;",
        "           ffffff806f28a678:ddr_device_type=< 7 >;",
        "           ffffff806f28a7f8:device_type=<memory>;",
        "       };",
        "\n",
        "  Display one node info by node name or node address:",
        "    %s> dts -s memory",
        "       memory{",
        "           ddr_device_type=< 0x7 >;",
        "           device_type=<memory>;",
        "           reg=< 0x0 0x40000000 0x0 0x3ee00000 0x0 0x80000000 0x0 0x40000000 >;",
        "       };",
        "\n",
        "  Display physical memory ranges:",
        "    %s> dts -m",
        "    ┌────────────────────────────────────────────────────────────┐",
        "    │                   DDR MEMORY RANGES                        │",
        "    ├─────┬────────────────────┬────────────────────┬────────────┤",
        "    │ No. │    Start Address   │     End Address    │    Size    │",
        "    ├─────┼────────────────────┼────────────────────┼────────────┤",
        "    │   1 │ 0x0000000080e00000 │ 0x00000000817fffff │       10MB │",
        "    │   2 │ 0x0000000081cf5000 │ 0x0000000081cfefff │       40KB │",
        "    │   3 │ 0x0000000081f20000 │ 0x000000008249ffff │     5.50MB │",
        "    │   4 │ 0x0000000082800000 │ 0x000000009927ffff │   362.50MB │",
        "    │   5 │ 0x000000009ea9c000 │ 0x000000009eafffff │      400KB │",
        "    │   6 │ 0x000000009f300000 │ 0x00000000a63fffff │      113MB │",
        "    │   7 │ 0x00000000a7000000 │ 0x00000000e05fffff │      918MB │",
        "    │   8 │ 0x00000000e0a00000 │ 0x00000000e88fffff │      127MB │",
        "    │   9 │ 0x00000000ea700000 │ 0x00000000fc7fffff │      289MB │",
        "    │  10 │ 0x00000000fca00000 │ 0x00000000ffffffff │       54MB │",
        "    │  11 │ 0x0000000880000000 │ 0x00000008afbfefff │   764.00MB │",
        "    │  12 │ 0x00000008b0000000 │ 0x00000008ba6fffff │      167MB │",
        "    │  13 │ 0x00000008bf800000 │ 0x00000008bfffffff │        8MB │",
        "    │  14 │ 0x00000008c0000000 │ 0x000000097fffffff │        3GB │",
        "    ├─────┴────────────────────┴────────────────────┴────────────┤",
        "    │ Total Memory Size: 5.75GB                                  │",
        "    └────────────────────────────────────────────────────────────┘",
        "\n",
        "  Read out the whole dtb memory:",
        "    %s> dts -b ./dts.dtb",
        "    save dtb to ./dts.dtb",
        "\n",
        "    please use below command to generate dts file:",
        "        dtc -I dtb -O dts -o ./xx.dts ./dts.dtb",
        "\n",
    };
}

Dts::Dts(){}

void Dts::print_ddr_info() {
    const auto& ddr_ranges = get_ddr_size();
    PRINT("┌────────────────────────────────────────────────────────────┐\n");
    PRINT("│                   DDR MEMORY RANGES                        │\n");
    if (ddr_ranges.empty()) {
        PRINT("\nNo DDR memory ranges found.\n");
        return;
    }
    uint64_t total_size = 0;
    for (const auto& range : ddr_ranges) {
        total_size += range.size;
    }
    std::vector<DdrRange> sorted_ranges = ddr_ranges;
    std::sort(sorted_ranges.begin(), sorted_ranges.end(),
              [](const DdrRange& a, const DdrRange& b) {
                  return a.address < b.address;
              });
    PRINT("├─────┬────────────────────┬────────────────────┬────────────┤\n");
    PRINT("│ No. │    Start Address   │     End Address    │    Size    │\n");
    PRINT("├─────┼────────────────────┼────────────────────┼────────────┤\n");
    for (size_t i = 0; i < sorted_ranges.size(); ++i) {
        const auto& range = sorted_ranges[i];
        uint64_t end_addr = (range.size > 0) ? (range.address + range.size - 1) : range.address;
        PRINT("│ %3zu │ 0x%016" PRIx64 " │ 0x%016" PRIx64 " │ %10s │\n",
            i + 1,
            range.address,
            end_addr,
            csize(range.size).c_str());
    }
    PRINT("├─────┴────────────────────┴────────────────────┴────────────┤\n");
    PRINT("│ Total Memory Size: %-37s   │\n",csize(total_size).c_str());
    PRINT("└────────────────────────────────────────────────────────────┘\n");
    PRINT("\n");
}

void Dts::print_node(std::shared_ptr<device_node> node_ptr,int flag){
    PRINT("%s\n",node_ptr->node_path.c_str());
    if (flag & DTS_ADDR){
        PRINT("%#lx:%s{\n",node_ptr->addr, node_ptr->full_name.c_str());
    }else{
        PRINT("%s{\n",node_ptr->full_name.c_str());
    }
    bool is_symbol_node = false;
    if (node_ptr->full_name.find("symbols") != std::string::npos || node_ptr->full_name.find("aliases") != std::string::npos) {
        is_symbol_node = true;
    }
    if (node_ptr->props.size() > 0){
        print_properties(node_ptr->props,0,is_symbol_node,flag);
    }
    if (node_ptr->child != nullptr){
        PRINT("\n");
        print_node(node_ptr->child,1,flag);
    }
    PRINT("};\n\n");
}

void Dts::read_dtb(std::string& path){
    if (!csymbol_exists("initial_boot_params")){
       LOGE("initial_boot_params doesn't exist in this kernel!\n");
       return;
    }
    ulong initial_boot_params_addr = csymbol_value("initial_boot_params");
    if (!is_kvaddr(initial_boot_params_addr)) {
       LOGE("initial_boot_params address is invalid !\n");
       return;
    }
    ulong initial_boot_params = read_pointer(initial_boot_params_addr,"initial_boot_params");
    void* header = read_memory(initial_boot_params,20,"dtb header");
    if (!header) {
       LOGE("Failed to read dtb header at address %lx\n", initial_boot_params);
       return;
    }
    ulong magic = UINT(header);
    if (magic != 0xEDFE0DD0){
        LOGE("magic:%lx is not correct !\n",magic);
        FREEBUF(header);
        return;
    }
    ulong db_size = ULONG(header + 4);
    db_size=((db_size & 0xFF)<<24)|((db_size & 0xFF00)<<8)|((db_size & 0xFF0000)>>8)|((db_size & 0xFF000000)>>24);
    if(db_size > DTB_MAX_SIZE) {
        LOGE("too large dtb size %ld\n",db_size);
        FREEBUF(header);
        return;
    }
    // LOGD("magic:%x\n",magic);
    LOGD("dtb addr:%#lx, size:%ld\n",initial_boot_params,db_size);
    FREEBUF(header);
    FILE *file = fopen(path.c_str(), "wb");
    if (file == nullptr) {
        LOGE("Failed to open file");
        return;
    }
    void *dtb_buf = read_memory(initial_boot_params,db_size,"read dtb");
    fwrite(dtb_buf, db_size, 1, file);
    PRINT("save dtb to %s",path.c_str());
    fclose(file);
    FREEBUF(dtb_buf);
}

void Dts::print_node(std::shared_ptr<device_node> node_ptr,int level,int flag){
    if (flag & DTS_SHOW){
        for (int i = 0; i < level; i++) {
            PRINT("\t");
        }
        if (flag & DTS_ADDR){
            PRINT("%#lx:%s{\n",node_ptr->addr, node_ptr->full_name.c_str());
        }else{
            PRINT("%s{\n",node_ptr->full_name.c_str());
        }
    }
    bool is_symbol_node = false;
    if (node_ptr->full_name.find("symbols") != std::string::npos || node_ptr->full_name.find("aliases") != std::string::npos) {
        is_symbol_node = true;
    }
    if ((flag & DTS_SHOW) && node_ptr->props.size() > 0){
        print_properties(node_ptr->props,level,is_symbol_node,flag);
    }
    int sibl_level = level;
    int chil_level = level;
    if (node_ptr->child != nullptr){
        if (flag & DTS_SHOW){
            PRINT("\n");
        }
        chil_level += 1;
        print_node(node_ptr->child,chil_level,flag);
    }
    if (flag & DTS_SHOW){
        for (int i = 0; i < level; i++) {
            PRINT("\t");
        }
        PRINT("};\n\n");
    }
    if (node_ptr->sibling != nullptr){
        print_node(node_ptr->sibling,sibl_level,flag);
    }
}

void Dts::print_properties(std::vector<std::shared_ptr<Property>> props,int level,bool is_symbol,int flag){
    int prop_level = level + 1;
    for (auto it = props.begin(); it != props.end(); ++it) {
        std::shared_ptr<Property> ptr = *it;
        std::string prop_name = ptr->name;
        void* prop_val = ptr->value.data();
        ulong prop_addr = ptr->addr;
        int prop_length = ptr->length;
        for (int i = 0; i < prop_level; i++) {
            PRINT("\t");
        }
        if (prop_length == 0){
            if (flag & DTS_ADDR){
                PRINT("%#lx:%s;\n",prop_addr,prop_name.c_str());
            }else{
                PRINT("%s;\n",prop_name.c_str());
            }
        }else{
            if (is_symbol || is_str_prop(prop_name)){
                if (flag & DTS_ADDR){
                    PRINT("%#lx:%s=<%s>;\n",prop_addr,prop_name.c_str(),(char*)prop_val);
                }else{
                    PRINT("%s=<%s>;\n",prop_name.c_str(),(char*)prop_val);
                }
            }else if (is_int_prop(prop_name) || ((prop_length % 4) == 0)){
                if (flag & DTS_ADDR){
                    PRINT("%#lx:%s=<",prop_addr,prop_name.c_str());
                }else{
                    PRINT("%s=<",prop_name.c_str());
                }
                for (int i = 0; i < (prop_length / 4); ++i) {
                    int val = UINT(prop_val + i * sizeof(int));
                    if (i == (prop_length / 4)-1){
                        PRINT("%#x",ntohl(val));
                    }else{
                        PRINT("%#x ",ntohl(val));
                    }
                }
                PRINT(">;\n");
            }else{
                if (flag & DTS_ADDR){
                    PRINT("%#lx:%s=<%s>;\n",prop_addr,prop_name.c_str(),(char*)prop_val);
                }else{
                    PRINT("%s=<%s>;\n",prop_name.c_str(),(char*)prop_val);
                }
            }
        }
    }
}

#pragma GCC diagnostic pop
