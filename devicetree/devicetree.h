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

#ifndef DEVICE_TREE_DEFS_H_
#define DEVICE_TREE_DEFS_H_

#include "plugin.h"

struct DdrRange {
    uint64_t address;
    size_t size;
};

struct Property {
    ulong addr;
    int length;
    std::string name;
    std::vector<char> value;
};

struct device_node {
    ulong addr;
    std::string name;
    std::string full_name;
    std::string node_path;
    std::vector<std::shared_ptr<Property>> props;
    std::shared_ptr<device_node> child;
    std::shared_ptr<device_node> sibling;
};

class Devicetree : public ParserPlugin {
private:
    std::vector<std::string> str_props={
        "model",
        "compatible",
        "bootargs",
        "name",
        "function",
        "device_type",
        "type",
        "label",
        "fsmgr_flags",
        "regulator-type",
        "iommu-faults",
        "pins",
        "parts",
        "serial",
        "hsuart",
        "names",
    };

    std::vector<std::string> int_props={
        "phandle",
        "reg",
        "size",
        "cells",
        "addr",
        "offset",
        "id",
        "strength",
    };

private:
    std::unordered_map<ulong, std::shared_ptr<device_node>> node_addr_maps;
    std::unordered_map<std::string, std::shared_ptr<device_node>> node_path_maps;

public:
    ulong root_addr;
    std::shared_ptr<device_node> root_node;

    Devicetree();
    void cmd_main(void) override;
    void init_offset(void) override;
    void init_command(void) override;
    std::shared_ptr<Property> getprop(ulong node_addr,const std::string& name);
    std::vector<std::shared_ptr<device_node>> find_node_by_name(const std::string& name);
    std::shared_ptr<device_node> find_node_by_addr(ulong addr);
    bool is_str_prop(const std::string& name);
    bool is_int_prop(const std::string& name);
    std::vector<DdrRange> get_ddr_size();
    std::vector<DdrRange> parse_memory_regs(const std::shared_ptr<Property>& prop);
    std::vector<std::shared_ptr<Property>> read_propertys(ulong addr);
    std::shared_ptr<device_node> read_node(const std::string& path, ulong node_addr);

};

#endif // DEVICE_TREE_DEFS_H_
