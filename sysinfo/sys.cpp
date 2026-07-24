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

#include "sys.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(SysInfo)
#endif

void SysInfo::cmd_main(void) {
    int c;
    std::string cppString;
    if (argcnt < 2) cmd_usage(pc->curcmd, SYNOPSIS);
    while ((c = getopt(argcnt, args, "csp")) != EOF) {
        switch(c) {
            case 'c': //print command line
                print_command_line();
                break;
            case 's': //print soc info
                print_soc_info();
                break;
            case 'p': //print sysctl values
                print_sysctl_values();
                break;
            default:
                argerrs++;
                break;
        }
    }
    if (argerrs)
        cmd_usage(pc->curcmd, SYNOPSIS);
}

void SysInfo::init_offset(void) {
    struct_init(socinfo);
}

void SysInfo::init_command(void) {
    cmd_name = "env";
    help_str_list={
        "env",                            /* command name */
        "dump system information",        /* short description */
        "-c \n"
            "  env -s \n"
            "  env -p \n"
            "  This command dumps the config info.",
        "\n",
        "EXAMPLES",
        "  Display kernel command line:",
        "    %s> env -c",
        "    resume                         : /dev/mmcblk0p76",
        "    init                           : /init",
        "    kernel.panic_on_rcu_stall      : 1",
        "    console                        : ttyMSM0,115200n8",
        "    kasan.stacktrace               : off",
        "\n",
        "  Display soc info:",
        "    %s> env -s",
        "\n",
        "  Display sysctl values:",
        "    %s> env -p",
        "    sysctl_vfs_cache_pressure        :              100",
        "    min_free_kbytes                  :            16384",
        "\n",
    };
}

SysInfo::SysInfo(){}

void SysInfo::print_socinfo(){
    LOGI("Starting socinfo parsing\n");
    field_init(socinfo,id);
    field_init(socinfo,raw_id);
    field_init(socinfo,hw_plat);
    field_init(socinfo,plat_ver);
    field_init(socinfo,pmic_model);
    field_init(socinfo,serial_num);
    field_init(socinfo,num_pmics);
    field_init(socinfo,chip_family);
    field_init(socinfo,nproduct_id);
    field_init(socinfo,chip_id);
    field_init(socinfo,nmodem_supported);
    field_init(socinfo,num_subset_parts);
    field_init(socinfo,nsubset_parts_array_offset);
    ulong soc_addr = read_pointer(csymbol_value("socinfo"),"socinfo");
    if (!is_kvaddr(soc_addr)) {
        LOGE("socinfo address is invalid: %lx\n", soc_addr);
        return;
    }
    LOGD("socinfo structure found at address: %lx\n", soc_addr);
    void *buf = read_struct(soc_addr,"socinfo");
    if (!buf) {
        LOGE("Failed to read socinfo structure at address %lx\n", soc_addr);
        return;
    }
    uint soc_id = UINT(buf + field_offset(socinfo,id));
    std::string chip_id = read_cstring(soc_addr + field_offset(socinfo,chip_id),32,"chip_id");
    uint32_t hw_id = UINT(buf + field_offset(socinfo,hw_plat));
    LOGD("Parsed basic info: soc_id=%u, chip_id=%s, hw_id=%u\n", soc_id, chip_id.c_str(), hw_id);
    std::string hw_name = "";
    if (hw_id < hw_platforms.size()){
        hw_name = hw_platforms[hw_id];
    }
    uint32_t num_parts = UINT(buf + field_offset(socinfo,num_subset_parts));
    uint32_t offset = UINT(buf + field_offset(socinfo,nsubset_parts_array_offset));
    ulong part_addr = soc_addr + offset;
    LOGD("Subset parts: num_parts=%u, offset=%u, part_addr=%lx\n", num_parts, offset, part_addr);
    std::vector<std::string> subset_parts = get_enumerator_list("subset_part_type");
    std::unordered_map<uint32_t, std::string> parts_map;
    for (const auto& part : subset_parts){
        parts_map[read_enum_val(part)] = part;
    }
    std::ostringstream oss;
    oss << std::left << std::setw(20) << "Chip"             << ": " << chip_id << "\n"
        << std::left << std::setw(20) << "Machine"          << ": " << soc_ids[soc_id] << "\n"
        << std::left << std::setw(20) << "Soc ID"           << ": " << soc_id << "\n"
        << std::left << std::setw(20) << "Serial Number"    << ": " << UINT(buf + field_offset(socinfo,serial_num)) << "\n"
        << std::left << std::setw(20) << "HARDWARE"         << ": " << hw_name  << "\n"
        << std::left << std::setw(20) << "Product ID"       << ": " << UINT(buf + field_offset(socinfo,nproduct_id))  << "\n";
    if (num_parts > subset_parts.size()){
        num_parts = subset_parts.size() - 1;
    }
    if (num_parts > 0){
        for (uint32_t i = 1; i < num_parts; i++){
            ulong addr =  part_addr + i * sizeof(uint32_t);
            uint32_t part_entry = read_uint(addr,"part");
            if (part_entry & 1){
                oss << std::left << std::setw(20) << parts_map[i] << ": "  << "Enable" << "\n";
            }else{
                oss << std::left << std::setw(20) << parts_map[i] << ": "  << "Disable" << "\n";
            }
        }
    }
    PRINT("%s \n",oss.str().c_str());
    FREEBUF(buf);
}

void SysInfo::print_qsocinfo(){
    LOGI("Starting qcom-socinfo device parsing\n");
    field_init(device,kobj);
    field_init(kobject,name);
    field_init(device,driver_data);
    field_init(qcom_socinfo,attr);
    field_init(qcom_socinfo,info);
    field_init(soc_device_attribute,machine);
    field_init(soc_device_attribute,family);
    field_init(soc_device_attribute,revision);
    field_init(soc_device_attribute,serial_number);
    field_init(soc_device_attribute,soc_id);
    field_init(socinfo_params,raw_device_family);
    field_init(socinfo_params,hw_plat_subtype);
    field_init(socinfo_params,raw_ver);
    field_init(socinfo_params,hw_plat);
    field_init(socinfo_params,fmt);
    field_init(socinfo_params,nproduct_id);
    struct_init(socinfo_params);
    std::ostringstream oss;
    LOGD("Searching for qcom-socinfo device on platform bus\n");
    for (const auto& dev : for_each_device_for_bus("platform")) {
        if (dev->name.empty() || dev->name != "qcom-socinfo"){
            continue;
        }
        LOGD("Found qcom-socinfo device at address %lx\n", dev->addr);
        ulong socinfo_addr = read_pointer(dev->addr + field_offset(device,driver_data) ,"driver_data");
        if (!is_kvaddr(socinfo_addr)){
            LOGW("Invalid driver_data address for qcom-socinfo device\n");
            continue;
        }
        LOGD("qcom_socinfo structure at address: %lx\n", socinfo_addr);
        ulong attr_addr = socinfo_addr + field_offset(qcom_socinfo,attr);
        std::string machine;
        ulong name_addr = read_pointer(attr_addr + field_offset(soc_device_attribute,machine),"machine addr");
        if (is_kvaddr(name_addr)){
            machine = read_cstring(name_addr,100, "machine");
        }
        std::string family;
        name_addr = read_pointer(attr_addr + field_offset(soc_device_attribute,family),"family addr");
        if (is_kvaddr(name_addr)){
            family = read_cstring(name_addr,100, "family");
        }
        std::string serial_number;
        name_addr = read_pointer(attr_addr + field_offset(soc_device_attribute,serial_number),"serial_number addr");
        if (is_kvaddr(name_addr)){
            serial_number = read_cstring(name_addr,100, "serial_number");
        }
        std::string soc_id;
        name_addr = read_pointer(attr_addr + field_offset(soc_device_attribute,soc_id),"soc_id addr");
        if (is_kvaddr(name_addr)){
            soc_id = read_cstring(name_addr,100, "soc_id");
        }
        LOGD("Parsed device attributes: machine=%s, family=%s, soc_id=%s\n", machine.c_str(), family.c_str(), soc_id.c_str());
        ulong param_addr = socinfo_addr + field_offset(qcom_socinfo,info);
        void *param_buf = read_struct(param_addr,"socinfo_params");
        if (!param_buf) {
            LOGE("Failed to read socinfo_params structure at address %lx\n", param_addr);
            continue;
        }
        uint32_t hw_id = UINT(param_buf + field_offset(socinfo_params,hw_plat));
        std::string hw_name = "";
        if (hw_id < hw_platforms.size()){
            hw_name = hw_platforms[hw_id];
        }
        oss << std::left << std::setw(12) << "Chip Family       : " << family << "\n"
            << std::left << std::setw(12) << "Machine           : " << machine << "\n"
            << std::left << std::setw(12) << "Soc ID            : " << soc_id << "\n"
            << std::left << std::setw(12) << "Serial Number     : " << serial_number << "\n"
            << std::left << std::setw(12) << "HARDWARE          : " << hw_name  << "\n"
            << std::left << std::setw(12) << "Product ID        : " << UINT(param_buf + field_offset(socinfo_params,nproduct_id))  << "\n";
        FREEBUF(param_buf);
    }
    PRINT("%s \n",oss.str().c_str());
}

void SysInfo::print_soc_info(){
    LOGI("Starting SOC information collection\n");
    struct_init(socinfo);
    if (struct_size(socinfo) == -1){
        LOGE("pls load the socinfo.ko at first\n");
        return;
    }
    // read_pmic_models();
    read_hw_platforms();
    read_soc_ids();
    if (!csymbol_exists("socinfo")){
        LOGD("Using qcom-socinfo device method\n");
        print_qsocinfo();
    }else{
        LOGD("Using legacy socinfo symbol method\n");
        print_socinfo();
    }
}

void SysInfo::read_pmic_models(){
    size_t len = get_array_length(TO_CONST_STRING("pmic_models"), NULL, 0);
    for (size_t i = 0; i < len; i++){
        ulong name_addr = read_pointer(csymbol_value("pmic_models") + i * sizeof(void*),"addr");
        if (!is_kvaddr(name_addr)) {
            continue;
        }
        std::string name = read_cstring(name_addr,64,"name");
        pmic_models.push_back(name);
    }
}

void SysInfo::read_hw_platforms(){
    LOGI("Reading hardware platforms\n");
    size_t len = get_array_length(TO_CONST_STRING("hw_platform"), NULL, 0);
    LOGD("Found %zu hardware platform entries\n", len);
    for (size_t i = 0; i < len; i++){
        ulong name_addr = read_pointer(csymbol_value("hw_platform") + i * sizeof(void*),"addr");
        if (!is_kvaddr(name_addr)) {
            LOGW("Invalid hw_platform name address at index %zu\n", i);
            hw_platforms.push_back("");
            continue;
        }
        std::string name = read_cstring(name_addr,64,"name");
        hw_platforms.push_back(name);
        LOGD("hw_platform[%zu] = %s\n", i, name.c_str());
    }
    LOGI("Hardware platforms reading completed: %zu entries\n", hw_platforms.size());
}

void SysInfo::read_soc_ids(){
    LOGI("Reading SOC IDs\n");
    field_init(soc_id,id);
    field_init(soc_id,name);
    struct_init(soc_id);
    size_t len = get_array_length(TO_CONST_STRING("soc_id"), NULL, 0);
    LOGD("Found %zu SOC ID entries\n", len);
    for (size_t i = 0; i < len; i++){
        ulong addr = csymbol_value("soc_id") + i * struct_size(soc_id);
        uint id = read_uint(addr + field_offset(soc_id,id),"id");
        ulong name_addr = read_pointer(addr + field_offset(soc_id,name),"name addr");
        if (!is_kvaddr(name_addr)) {
            LOGW("Invalid soc_id name address at index %zu\n", i);
            continue;
        }
        std::string name = read_cstring(name_addr,64,"name");
        soc_ids[id] = name;
        LOGD("soc_id[%u] = %s\n", id, name.c_str());
    }
    LOGI("SOC IDs reading completed: %zu entries\n", soc_ids.size());
}

void SysInfo::print_sysctl_values(){
    std::ostringstream oss;

    // Helper lambda to read and format sysctl value
    auto read_sysctl = [&](const char* name) {
        if (!csymbol_exists(name)) {
            LOGW("Symbol %s doesn't exist in this kernel\n", name);
            return;
        }
        ulong addr = csymbol_value(name);
        if (!is_kvaddr(addr)) {
            LOGW("Invalid address for %s: %lx\n", name, addr);
            return;
        }
        int value = read_int(addr, name);
        oss << std::left << std::setw(35) << name << ": "
            << std::right << std::setw(16) << value << "\n";
        LOGD("Read %s = %d\n", name, value);
    };

    // Read VFS cache related
    read_sysctl("sysctl_vfs_cache_pressure");

    // Read dirty page management parameters
    read_sysctl("dirtytime_expire_interval");
    read_sysctl("dirty_expire_interval");
    read_sysctl("dirty_background_bytes");
    read_sysctl("dirty_background_ratio");
    read_sysctl("dirty_writeback_interval");
    read_sysctl("vm_dirty_ratio");
    read_sysctl("vm_dirty_bytes");

    // Read memory management parameters
    read_sysctl("min_free_kbytes");
    read_sysctl("watermark_boost_factor");
    read_sysctl("watermark_scale_factor");

    // Read OOM related parameters
    read_sysctl("sysctl_panic_on_oom");
    read_sysctl("sysctl_oom_kill_allocating_task");
    read_sysctl("sysctl_oom_dump_tasks");

    // Read other parameters
    read_sysctl("sysctl_extfrag_threshold");
    read_sysctl("vm_swappiness");

    // Read lowmem_reserve_ratio array
    if (csymbol_exists("sysctl_lowmem_reserve_ratio") &&
        csymbol_exists("contig_page_data")) {

        LOGD("Reading lowmem_reserve_ratio array\n");
        oss << "\n/proc/sys/vm/lowmem_reserve_ratio\n\n";

        // Get MAX_NR_ZONES
        int max_nr_zones = 0;
        max_nr_zones = read_enum_val("__MAX_NR_ZONES");
        LOGD("MAX_NR_ZONES = %d\n", max_nr_zones);

        // Initialize zone structure
        field_init(zone, name);
        field_init(pglist_data, node_zones);
        struct_init(zone);
        struct_init(pglist_data);

        ulong contig_page_data_addr = csymbol_value("contig_page_data");
        ulong zone_addr = contig_page_data_addr + field_offset(pglist_data, node_zones);
        ulong ratio_addr = csymbol_value("sysctl_lowmem_reserve_ratio");

        std::ostringstream zone_names;
        std::ostringstream ratios;

        for (int i = 0; i < max_nr_zones; i++) {
            // Read zone name
            ulong name_addr = read_pointer(zone_addr + field_offset(zone, name), "zone name");
            if (is_kvaddr(name_addr)) {
                std::string zone_name = read_cstring(name_addr, 12, "zone name");
                zone_names << std::right << std::setw(16) << zone_name << "\t";
                LOGD("Zone[%d] name: %s\n", i, zone_name.c_str());
            } else {
                zone_names << std::right << std::setw(16) << "UNKNOWN" << "\t";
            }

            // Read ratio value
            ulong ratio_elem_addr = ratio_addr + i * sizeof(int);
            int ratio_value = read_int(ratio_elem_addr, "ratio");
            ratios << std::right << std::setw(16) << ratio_value << "\t";
            LOGD("Zone[%d] ratio: %d\n", i, ratio_value);

            zone_addr += struct_size(zone);
        }

        oss << zone_names.str() << "\n";
        oss << ratios.str() << "\n";
    } else {
        LOGW("Cannot read lowmem_reserve_ratio: required symbols not found\n");
    }

    PRINT("%s\n", oss.str().c_str());
}

void SysInfo::print_command_line(){
    LOGI("Starting kernel command line parsing\n");
    if (!csymbol_exists("saved_command_line")){
        LOGE( "saved_command_line doesn't exist in this kernel!\n");
        return;
    }
    ulong cmd_addr = read_pointer(csymbol_value("saved_command_line"),"saved_command_line");
    if (!is_kvaddr(cmd_addr)) {
        LOGE("saved_command_line address is invalid: %lx\n", cmd_addr);
        return;
    }
    LOGD("Reading command line from address: %lx\n", cmd_addr);
    std::string cmd_line = read_cstring(cmd_addr,2048,"saved_command_line");
    size_t pos = 0;
    while ((pos = cmd_line.find('\0')) != std::string::npos) {
        cmd_line.replace(pos, 1, " ");
    }
    LOGD("Raw command line: %s\n", cmd_line.c_str());
    std::unordered_map<std::string, std::string> kconfigs;
    std::istringstream cmdarg(cmd_line);
    std::string config;
    std::string mapKey;
    size_t max_len = 0;
    while (cmdarg >> config) {
        size_t pos = config.find('=');
        if (pos != std::string::npos) {
            mapKey = config.substr(0, pos);
            max_len = std::max(max_len,mapKey.size());
            kconfigs[mapKey] = config.substr(pos + 1);
        } else {
            mapKey = config;
            max_len = std::max(max_len,mapKey.size());
            kconfigs[mapKey] = "";
        }
    }
    LOGD("Parsed %zu command line parameters\n", kconfigs.size());
    std::ostringstream oss;
    for (const auto& pair : kconfigs) {
        oss << std::left << std::setw(max_len + 1) << pair.first
            << ": " << pair.second << "\n";
    }
    PRINT("%s \n",oss.str().c_str());
    LOGI("Command line parsing completed\n");
}

#pragma GCC diagnostic pop
