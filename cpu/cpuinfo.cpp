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

#include "cpuinfo.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(CpuInfo)
#endif

void CpuInfo::cmd_main(void) {
    int c;
    std::string cppString;
    if (argcnt < 2) cmd_usage(pc->curcmd, SYNOPSIS);
    if (cpu_infos.size() == 0){
        parser_cpu_policy();
    }
    while ((c = getopt(argcnt, args, "pfs")) != EOF) {
        switch(c) {
            case 'p':
                print_cpu_policy();
                break;
            case 'f':
                print_freq_table();
                break;
            case 's':
                print_cpu_state();
                break;
            default:
                argerrs++;
                break;
        }
    }
    if (argerrs)
        cmd_usage(pc->curcmd, SYNOPSIS);
}

void CpuInfo::init_offset(void) {
    field_init(cpufreq_policy,cpu);
    field_init(cpufreq_policy,cpuinfo);
    field_init(cpufreq_policy,min);
    field_init(cpufreq_policy,max);
    field_init(cpufreq_policy,cur);
    field_init(cpufreq_policy,policy);
    field_init(cpufreq_policy,governor);
    field_init(cpufreq_policy,freq_table);
    struct_init(cpufreq_policy);

    field_init(cpufreq_frequency_table,frequency);
    struct_init(cpufreq_frequency_table);
}

void CpuInfo::init_command(void) {
    cmd_name = "cpu";
    help_str_list={
        "cpu",                                    /* command name */
        "dump CPU frequency and state information",  /* short description */
        "[-p] [-f] [-s]\n"
        "  This command dumps CPU frequency policy, frequency table, and state information.\n"
        "\n"
        "    -p          display CPU frequency policy for all CPUs\n"
        "    -f          display CPU frequency table\n"
        "    -s          display CPU cluster state information\n",
        "\n",
        "EXAMPLES",
        "  Display CPU frequency policy:",
        "    %s> cpu -p",
        "    CPU cpufreq_policy  cluster cur_freq   min_freq   max_freq   governor",
        "    0   e565fa00        0       1708800    1708800    1708800    schedutil",
        "    1   e565fa00        0       1708800    1708800    1708800    schedutil",
        "    2   e565fa00        0       1708800    1708800    1708800    schedutil",
        "    3   e565fa00        0       1708800    1708800    1708800    schedutil",
        "\n",
        "  Display CPU frequency table:",
        "    %s> cpu -f",
        "    CPU0            CPU1            CPU2            CPU3",
        "    614400          614400          614400          614400",
        "    864000          864000          864000          864000",
        "    1363200         1363200         1363200         1363200",
        "    1708800         1708800         1708800         1708800",
        "\n",
        "  Display CPU cluster state:",
        "    %s> cpu -s",
        "    cluster_data:0xffffffe595428460 Enable:true num_cpus:4 first_cpu:0 active_cpus:2",
        "       cpu_data:0xffffff887a139de0 cpu:0 disabled:false is_busy:false busy_pct:32",
        "       cpu_data:0xffffff887a35cde0 cpu:1 disabled:false is_busy:false busy_pct:50",
        "       cpu_data:0xffffff887a57fde0 cpu:2 disabled:false is_busy:false busy_pct:1",
        "       cpu_data:0xffffff887a7a2de0 cpu:3 disabled:false is_busy:false busy_pct:0",
        "",
        "    cluster_data:0xffffffe595428590 Enable:true num_cpus:1 first_cpu:4 active_cpus:1",
        "       cpu_data:0xffffff887a9c5de0 cpu:4 disabled:false is_busy:false busy_pct:0",
        "\n",
    };
}

CpuInfo::CpuInfo(){}

void CpuInfo::print_cpu_policy(){
    std::ostringstream oss;
    oss  << std::left << std::setw(3)  << "CPU" << " "
            << std::left << std::setw(20) << "cpufreq_policy" << " "
            << std::left << std::setw(7)  << "cluster" << " "
            << std::left << std::setw(10) << "cur_freq" << " "
            << std::left << std::setw(10) << "min_freq" << " "
            << std::left << std::setw(10) << "max_freq" << " "
            << std::left << std::setw(10) << "governor" << " "
            << "\n";
    int index = 0;
    for (const auto& cpu_ptr : cpu_infos) {
        oss << std::left << std::setw(3)  << index << " "
            << std::left << std::setw(20) << std::hex << cpu_ptr->addr << " "
            << std::left << std::setw(7)  << std::dec << cpu_ptr->cluster << " "
            << std::left << std::setw(10) << std::dec << cpu_ptr->cur << " "
            << std::left << std::setw(10) << std::dec << cpu_ptr->min << " "
            << std::left << std::setw(10) << std::dec << cpu_ptr->max << " "
            << std::left << std::setw(10) << cpu_ptr->gov_name << " "
            << "\n";
        index++;
    }
    PRINT("%s \n",oss.str().c_str());
}

void CpuInfo::parser_cpu_policy(){
    LOGI("Starting CPU policy parsing\n");
    if (!csymbol_exists("cpufreq_cpu_data")){
        LOGE("cpufreq_cpu_data doesn't exist in this kernel!\n");
        return;
    }
    ulong cpufreq_cpu_addr = csymbol_value("cpufreq_cpu_data");
    if (!is_kvaddr(cpufreq_cpu_addr)) {
        LOGE("cpufreq_cpu_data address is invalid: %lx\n", cpufreq_cpu_addr);
        return;
    }
    LOGD("cpufreq_cpu_data symbol found at address: %lx\n", cpufreq_cpu_addr);
    std::vector<ulong> percpu_list = for_each_percpu(cpufreq_cpu_addr);
    for (size_t i = 0; i < percpu_list.size(); i++){
        ulong addr = percpu_list[i];
        if (!is_kvaddr(addr)) {
            LOGW("Invalid per-cpu address for CPU %zu: %lx\n", i, addr);
            continue;
        }
        ulong cpufreq_addr = read_pointer(addr,"per cpu cpufreq_cpu_data");
        if (!is_kvaddr(cpufreq_addr)) {
            LOGW("Invalid cpufreq_policy address for CPU %zu: %lx\n", i, cpufreq_addr);
            continue;
        }
        void *cpufreq_buf = read_struct(cpufreq_addr,"cpufreq_policy");
        if (!cpufreq_buf) {
            LOGE("Failed to read cpufreq_policy structure for CPU %zu at address %lx\n", i, cpufreq_addr);
            continue;
        }
        std::shared_ptr<cpu_policy> cpu_ptr = std::make_shared<cpu_policy>();
        cpu_ptr->addr = cpufreq_addr;
        cpu_ptr->cluster = UINT(cpufreq_buf + field_offset(cpufreq_policy,cpu));
        cpu_ptr->cur = UINT(cpufreq_buf + field_offset(cpufreq_policy,cur));
        cpu_ptr->min = UINT(cpufreq_buf + field_offset(cpufreq_policy,min));
        cpu_ptr->max = UINT(cpufreq_buf + field_offset(cpufreq_policy,max));
        ulong gov_addr = ULONG(cpufreq_buf + field_offset(cpufreq_policy,governor));
        cpu_ptr->gov_name = read_cstring(gov_addr,16, "governor name");
        ulong freq_table = ULONG(cpufreq_buf + field_offset(cpufreq_policy,freq_table));
        LOGD("CPU %zu policy parsed: cluster=%u, cur=%u, min=%u, max=%u, governor=%s\n",
             i, cpu_ptr->cluster, cpu_ptr->cur, cpu_ptr->min, cpu_ptr->max, cpu_ptr->gov_name.c_str());
        FREEBUF(cpufreq_buf);

        LOGD("Parsing frequency table for CPU %zu at address %lx\n", i, freq_table);
        int index = 0;
        while (1){
            ulong freq_addr = freq_table + index * struct_size(cpufreq_frequency_table) + field_offset(cpufreq_frequency_table,frequency);
            uint32_t frequency = read_uint(freq_addr,"frequency");
            if ((frequency & 0xffff) == 0xfffe){
                break;
            }
            if ((frequency & 0xffff) == 0xffff){
                continue;
            }
            LOGD("addr:%#lx frequency:%d \n",freq_addr,frequency);
            cpu_ptr->freq_table.push_back(frequency);
            index++;
        }
        LOGD("CPU %zu frequency table parsed: %zu frequencies found\n", i, cpu_ptr->freq_table.size());
        cpu_infos.push_back(cpu_ptr);
    }
    LOGI("CPU policy parsing completed: %zu CPUs processed\n", cpu_infos.size());
}


void CpuInfo::print_cpu_state(){
    LOGI("Starting CPU state printing\n");
    if (!csymbol_exists("cluster_state")){
        LOGE("cluster_state doesn't exist in this kernel!\n");
        return;
    }
    field_init(cluster_data,lru);
    field_init(cluster_data,enable);
    field_init(cluster_data,num_cpus);
    field_init(cluster_data,first_cpu);
    field_init(cluster_data,active_cpus);
    struct_init(cluster_data);
    ulong state_addr = csymbol_value("cluster_state");
    size_t num_clusters = read_uint(csymbol_value("num_clusters"),"num_clusters");
    LOGD("Found %zu clusters at address %lx\n", num_clusters, state_addr);

    for (size_t i = 0; i < num_clusters; i++){
        LOGD("Processing cluster %zu/%zu\n", i + 1, num_clusters);
        ulong cluster_addr = state_addr + i * struct_size(cluster_data);
        void *buf = read_struct(cluster_addr,"cluster_data");
        if (!buf) {
            LOGE("Failed to read cluster_data structure for cluster %zu at address %lx\n", i, cluster_addr);
            continue;
        }
        PRINT("cluster_data:%#lx Enable:%s num_cpus:%d first_cpu:%d active_cpus:%d \n",cluster_addr,
            UINT(buf + field_offset(cluster_data,enable)) ? "true":"false",
            UINT(buf + field_offset(cluster_data,num_cpus)),
            UINT(buf + field_offset(cluster_data,first_cpu)),
            UINT(buf + field_offset(cluster_data,active_cpus))
        );
        ulong list_head = cluster_addr + field_offset(cluster_data,lru);
        int offset = offsetof(struct cpu_data, sib);
        LOGD("Traversing CPU data list for cluster %zu, list_head=%lx\n", i, list_head);
        for (const auto& data_addr : for_each_list(list_head, offset)) {
            if (!is_kvaddr(data_addr)) {
                LOGW("Invalid cpu_data address in cluster %zu: %lx\n", i, data_addr);
                continue;
            }
            struct cpu_data data;
            if(!read_struct(data_addr,&data,sizeof(data),"cpu_data")){
                LOGE("Failed to read cpu_data structure at address %lx in cluster %zu\n", data_addr, i);
                continue;
            }
            PRINT("   cpu_data:%#lx cpu:%d disabled:%s is_busy:%s busy_pct:%d \n",data_addr,
                data.cpu,
                data.disabled ? "true":"false",
                data.is_busy ? "true":"false",
                data.busy_pct
            );
        }
        FREEBUF(buf);
        PRINT("\n\n");
    }
}

void CpuInfo::print_freq_table(){
    LOGI("Starting frequency table printing\n");
    size_t freq_cnt = 0;
    for (const auto& cpu_ptr : cpu_infos) {
        freq_cnt = std::max(freq_cnt, cpu_ptr->freq_table.size());
        if (cpu_ptr->freq_table.empty()) {
            LOGW("CPU with empty frequency table found at address %lx\n", cpu_ptr->addr);
        }
    }
    LOGD("Maximum frequency count across all CPUs: %zu\n", freq_cnt);

    std::ostringstream oss;
    for (size_t i = 0; i < cpu_infos.size(); i++) {
        std::string name = "CPU" + std::to_string(i);
        oss << std::left << std::setw(15) << name << " ";
    }
    PRINT("%s \n", oss.str().c_str());
    for (size_t i = 0; i < freq_cnt; i++) {
        oss.str("");
        oss.clear();
        for (const auto& cpu_ptr : cpu_infos) {
            if (cpu_ptr->freq_table.size() > i) {
                oss << std::left << std::setw(15) << cpu_ptr->freq_table[i] << " ";
            } else {
                oss << std::left << std::setw(15) << "N/A" << " ";
            }
        }
        PRINT("%s \n", oss.str().c_str());
    }
    LOGI("Frequency table printing completed\n");
}

#pragma GCC diagnostic pop
