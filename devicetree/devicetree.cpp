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

#include "devicetree.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

Devicetree::Devicetree() : root_addr(0), root_node(nullptr) {
    init_offset();
    if (!csymbol_exists("of_root")){
        LOGE( "of_root doesn't exist in this kernel!\n");
        return;
    }
    ulong of_root_addr = csymbol_value("of_root");
    if (!is_kvaddr(of_root_addr)) {
        LOGE("of_root address is invalid!\n");
        return;
    }
    root_addr = read_pointer(of_root_addr,"of_root");
}

void Devicetree::cmd_main(void) {}

void Devicetree::init_command(void) {}

void Devicetree::init_offset(void) {
    field_init(device_node,name);
    field_init(device_node,phandle);
    field_init(device_node,full_name);
    field_init(device_node,fwnode);
    field_init(device_node,properties);
    field_init(device_node,parent);
    field_init(device_node,child);
    field_init(device_node,sibling);
    field_init(property,name);
    field_init(property,length);
    field_init(property,value);
    field_init(property,next);

    struct_init(device_node);
    struct_init(property);
}

std::shared_ptr<Property> Devicetree::getprop(ulong node_addr, const std::string& name) {
    if (root_node == nullptr) {
        root_node = read_node("", root_addr);
    }
    auto node_it = node_addr_maps.find(node_addr);
    if (node_it == node_addr_maps.end()) {
        return nullptr;
    }
    const auto& props = node_it->second->props;
    for (const auto& prop : props) {
        if (prop->name == name) {
            return prop;
        }
    }
    return nullptr;
}

std::shared_ptr<device_node> Devicetree::read_node(const std::string& path, ulong node_addr){
    if (!is_kvaddr(node_addr)) return nullptr;

    // Check if node already exists in cache
    auto node_it = node_addr_maps.find(node_addr);
    if (node_it != node_addr_maps.end()) {
        return node_it->second;
    }

    void *node_buf = read_struct(node_addr,"device_node");
    if(node_buf == nullptr) return nullptr;

    std::shared_ptr<device_node> node_ptr = std::make_shared<device_node>();
    node_ptr->addr = node_addr;

    ulong full_name_addr = ULONG(node_buf + field_offset(device_node,full_name));
    std::string full_name = read_cstring(full_name_addr,64, "device_node_full_name");
    std::string node_path;
    if(node_addr == root_addr){
        full_name = "/";
        node_path = full_name;
    }else{
        node_path = path + "/" + full_name;
    }
    node_ptr->full_name = std::move(full_name);

    if (node_path.size() >= 2 && node_path[0] == '/' && node_path[1] == '/') {
        node_ptr->node_path = node_path.substr(1);
    } else {
        node_ptr->node_path = std::move(node_path);
    }

    ulong name_addr = ULONG(node_buf + field_offset(device_node,name));
    node_ptr->name = read_cstring(name_addr,64, "device_node name");

    ulong prop_addr = ULONG(node_buf + field_offset(device_node,properties));
    if (is_kvaddr(prop_addr)){
        node_ptr->props = read_propertys(prop_addr);
    }

    // Cache the node before reading children to avoid infinite recursion
    node_addr_maps[node_addr] = node_ptr;
    node_path_maps[node_ptr->node_path] = node_ptr;

    ulong child = ULONG(node_buf + field_offset(device_node,child));
    if (is_kvaddr(child)){
        node_ptr->child = read_node(node_ptr->node_path, child);
    }

    ulong sibling = ULONG(node_buf + field_offset(device_node,sibling));
    if (is_kvaddr(sibling)){
        node_ptr->sibling = read_node(path, sibling);
    }

    FREEBUF(node_buf);
    return node_ptr;
}

std::vector<std::shared_ptr<Property>> Devicetree::read_propertys(ulong addr){
    std::vector<std::shared_ptr<Property>> res;
    res.reserve(8); // Reserve space for typical number of properties

    while (is_kvaddr(addr)){
        void *prop_buf = read_struct(addr,"property");
        if(prop_buf == nullptr) break;

        auto prop = std::make_shared<Property>();
        prop->addr = addr;

        ulong name_addr = ULONG(prop_buf + field_offset(property,name));
        prop->name = read_cstring(name_addr,64,"property_name");

        int length = UINT(prop_buf + field_offset(property,length));
        prop->length = length;

        if(length > 0){
            ulong value_addr = ULONG(prop_buf + field_offset(property,value));
            prop->value.resize(length);
            void *prop_val = read_memory(value_addr,length,"property_value");
            if(prop_val != nullptr) {
                memcpy(prop->value.data(), prop_val, length);
                FREEBUF(prop_val);
            }
        }

        res.push_back(std::move(prop));
        addr = ULONG(prop_buf + field_offset(property,next));
        FREEBUF(prop_buf);
    }
    return res;
}

std::vector<DdrRange> Devicetree::get_ddr_size(){
    if (root_node == nullptr){
        root_node = read_node("", root_addr);
    }
    std::vector<std::shared_ptr<device_node>> nodes = find_node_by_name("memory");
    if (nodes.empty())
        return {};
    std::vector<DdrRange> res;
    res.reserve(nodes.size()); // Reserve space to avoid reallocations
    for (const auto& node : nodes){
        std::shared_ptr<Property> prop = getprop(node->addr,"device_type");
        if (prop == nullptr || prop->value.empty()){
            continue;
        }
        // Ensure null-terminated string comparison
        std::string tempstr(reinterpret_cast<const char*>(prop->value.data()),
                           strnlen(reinterpret_cast<const char*>(prop->value.data()), prop->value.size()));
        if (tempstr != "memory"){
            continue;
        }
        // read property of reg
        //       <|  start     | |   size     |
        // reg = <0x0 0x40000000 0x0 0x3ee00000 0x0 0x80000000 0x0 0x40000000>
        prop = getprop(node->addr,"reg");
        if (prop != nullptr) {
            auto memory_ranges = parse_memory_regs(prop);
            res.insert(res.end(), memory_ranges.begin(), memory_ranges.end());
        }
    }
    return res;
}

std::vector<DdrRange> Devicetree::parse_memory_regs(const std::shared_ptr<Property>& prop) {
    if (!prop || prop->value.empty() || prop->length < 16) {
        return {};
    }
    std::vector<DdrRange> result;
    const char* ptr = reinterpret_cast<const char*>(prop->value.data());
    const size_t reg_cnt = prop->length / 4;
    const int group_cnt = reg_cnt / 4;
    result.reserve(group_cnt);
    for (int i = 0; i < group_cnt; ++i) {
        const size_t base_idx = i * 4;
        const uint32_t addr_high = ntohl(*reinterpret_cast<const uint32_t*>(ptr + (base_idx + 0) * 4));
        const uint32_t addr_low = ntohl(*reinterpret_cast<const uint32_t*>(ptr + (base_idx + 1) * 4));
        const uint32_t size_high = ntohl(*reinterpret_cast<const uint32_t*>(ptr + (base_idx + 2) * 4));
        const uint32_t size_low = ntohl(*reinterpret_cast<const uint32_t*>(ptr + (base_idx + 3) * 4));

        const uint64_t address = (static_cast<uint64_t>(addr_high) << 32) | addr_low;
        const size_t size = static_cast<size_t>((static_cast<uint64_t>(size_high) << 32) | size_low);

        result.push_back({address, size});
    }
    return result;
}

std::vector<std::shared_ptr<device_node>> Devicetree::find_node_by_name(const std::string& name){
    std::string node_path;
    std::shared_ptr<device_node> node_ptr;
    std::vector<std::shared_ptr<device_node>> res;
    if (root_node == nullptr){
        root_node = read_node("", root_addr);
    }
    for (const auto& node_item : node_path_maps) {
        node_path = node_item.first;
        node_ptr = node_item.second;
        size_t pos = node_path.find_last_of(name);
        if (pos == std::string::npos) continue;
        if(node_ptr->full_name == name || node_ptr->name == name || node_ptr->node_path == name){
            res.push_back(node_ptr);
        }
    }
    return res;
}

std::shared_ptr<device_node> Devicetree::find_node_by_addr(ulong addr){
    if (root_node == nullptr){
        root_node = read_node("", root_addr);
    }
    auto it = node_addr_maps.find(addr);
    return (it != node_addr_maps.end()) ? it->second : nullptr;
}

bool Devicetree::is_str_prop(const std::string& name) {
    for (const auto& str : str_props) {
        if (name.find(str) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool Devicetree::is_int_prop(const std::string& name) {
    for (const auto& str : int_props) {
        if (name.find(str) != std::string::npos) {
            return true;
        }
    }
    return false;
}

#pragma GCC diagnostic pop