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

#include "thermal.h"
#include "logger/logger_core.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(Thermal)
#endif

/**
 * @brief Main command entry point for thermal plugin
 *
 * Parses command line arguments and dispatches to appropriate handler functions
 * for displaying thermal zones, cooling devices, or specific zone details.
 * Initializes thermal zone parsing on first invocation.
 */
void Thermal::cmd_main(void) {
    int c;
    std::string cppString;
    if (argcnt < 2) cmd_usage(pc->curcmd, SYNOPSIS);

    // Parse thermal zones on first use
    if (zone_list.size() == 0){
        init_offset();
        parser_thrermal_zone();
    }
    // Parse command line options
    while ((c = getopt(argcnt, args, "czZ:")) != EOF) {
        switch(c) {
            case 'z':
                print_zone_device();
                break;
            case 'c':
                print_cooling_device();
                break;
            case 'Z':
                cppString.assign(optarg);
                print_zone_device(cppString);
                break;
            default:
                argerrs++;
                break;
        }
    }

    if (argerrs)
        cmd_usage(pc->curcmd, SYNOPSIS);
}

void Thermal::init_command(void) {
    cmd_name = "tm";
    help_str_list={
        "tm",                                /* command name */
        "display thermal management system information and analysis",  /* short description */
        "[-z] [-c] [-Z zone_name]\n"
        "  This command analyzes the thermal management subsystem from crash dumps,\n"
        "  providing comprehensive information about thermal zones, cooling devices,\n"
        "  and thermal control policies including trip points and cooling actions.\n"
        "\n"
        "    -z              display all thermal zones with temperature and governor information\n"
        "    -c              display all cooling devices with their addresses and names\n"
        "    -Z zone_name    display detailed information for specific thermal zone\n",
        "\n",
        "EXAMPLES",
        "  Display all thermal zone info:",
        "    %s> tm -z",
        "    11    e9ae1c00   gpu-step           step_wise    38700      33600",
        "    12    e9ae1000   cpuss-0-step       step_wise    34300      34600",
        "    13    e9ae4c00   cpuss-1-step       step_wise    34600      35300",
        "\n",
        "  Display the temperature gear and cooling action of specified thermal zone:",
        "    %s> tm -Z cpuss-0-step",
        "    cpuss-0-step:",
        "        temperature:100000",
        "            [4]thermal_cooling_device:0xe574c000 --> cpu-isolate0",
        "            [6]thermal_cooling_device:0xe574a800 --> cpu-isolate2",
        "\n",
        "  Display all cooling device:",
        "    %s> tm -c",
        "    ID    ADDR               Name",
        "    3     ffffff801d7d1000   bcl-off",
        "    1     ffffff801ace0800   cpu-cluster0",
        "    0     ffffff800832a000   cpufreq-cpu0",
        "\n",
    };
}

/**
 * @brief Initialize kernel structure field offsets for thermal subsystem
 *
 * Initializes offsets for thermal_zone_device, thermal_trip, thermal_instance,
 * thermal_cooling_device, and thermal_governor structures to enable proper
 * memory access in kernel crash dumps.
 */
void Thermal::init_offset(void) {
    // Initialize thermal_zone_device structure fields
    field_init(thermal_zone_device, node);
    field_init(thermal_zone_device, id);
    field_init(thermal_zone_device, type);
    field_init(thermal_zone_device, temperature);
    field_init(thermal_zone_device, last_temperature);
    field_init(thermal_zone_device, num_trips);
    field_init(thermal_zone_device, trips);
    field_init(thermal_zone_device, devdata);
    field_init(thermal_zone_device, thermal_instances);
    field_init(thermal_zone_device, governor);
    struct_init(thermal_zone_device);

    // Initialize __thermal_zone structure fields (alternative trip storage)
    field_init(__thermal_zone, ntrips);
    field_init(__thermal_zone, trips);
    struct_init(__thermal_zone);

    // Initialize thermal_trip structure fields
    field_init(thermal_trip, temperature);
    struct_init(thermal_trip);

    // Initialize thermal_instance structure fields
    field_init(thermal_instance, tz_node);
    field_init(thermal_instance, cdev_node);
    field_init(thermal_instance, trip);
    field_init(thermal_instance, cdev);
    struct_init(thermal_instance);

    // Initialize thermal_governor structure fields
    field_init(thermal_governor, name);

    // Initialize thermal_cooling_device structure fields
    field_init(thermal_cooling_device, id);
    field_init(thermal_cooling_device, type);
    field_init(thermal_cooling_device, node);
}

/**
 * @brief Constructor for Thermal plugin
 *
 * Disables automatic offset initialization as it's done manually
 * during first command invocation.
 */
Thermal::Thermal(){
    do_init_offset = false;
}

/**
 * @brief Print detailed information for a specific thermal zone
 *
 * Displays trip points and associated cooling devices for the specified
 * thermal zone. Shows temperature thresholds and cooling actions.
 *
 * @param dev_name Name of the thermal zone to display
 */
void Thermal::print_zone_device(std::string dev_name){
    bool found = false;
    for (auto& zone_ptr : zone_list) {
        if (zone_ptr->name == dev_name){
            found = true;
            PRINT("%s: \n",zone_ptr->name.c_str());
            // Display each trip point and its cooling devices
            for (auto& trip_ptr : zone_ptr->trip_list) {
                PRINT("   temperature:%d \n",trip_ptr->temp);
                for (auto& cdev_ptr : trip_ptr->cool_list) {
                    PRINT("      [%d]thermal_cooling_device:%#lx --> %s \n",
                            cdev_ptr->id,cdev_ptr->addr,cdev_ptr->name.c_str());
                }
            }
        }
    }
    if (!found) {
        LOGD("Thermal zone '%s' not found", dev_name.c_str());
    }
}

/**
 * @brief Print all cooling devices in the system
 *
 * Iterates through the kernel's thermal_cdev_list and displays
 * ID, address, and name for each cooling device.
 */
void Thermal::print_cooling_device(){
    // Check if thermal_cdev_list symbol exists
    if (!csymbol_exists("thermal_cdev_list")){
        LOGE("thermal_cdev_list doesn't exist in this kernel");
        return;
    }
    ulong list_head = csymbol_value("thermal_cdev_list");
    if (!is_kvaddr(list_head)) {
        LOGE("thermal_cdev_list address 0x%lx is invalid", list_head);
        return;
    }
    LOGD("thermal_cdev_list at 0x%lx", list_head);
    // Build output table
    std::ostringstream oss;
    oss << std::left << std::setw(5) << "ID" << " "
        << std::left << std::setw(VADDR_PRLEN + 2) << "ADDR" << " "
        << std::left << "Name" << "\n";

    int offset = field_offset(thermal_cooling_device, node);
    // Iterate through cooling device list
    for (const auto& addr : for_each_list(list_head,offset)) {
        int id = read_int(addr + field_offset(thermal_cooling_device, id),"id");
        std::string name = "";
        ulong name_addr = read_pointer(addr + field_offset(thermal_cooling_device,type),"type");
        if (is_kvaddr(name_addr)) {
            name = read_cstring(name_addr,64, "type name");
        }
        LOGD("Parsing cooling device: addr=0x%lx, id=%d, %s",addr, id, name.c_str());
        oss << std::left << std::hex << std::setw(5) << std::dec << id << " "
            << std::left << std::setw(VADDR_PRLEN + 2) << std::hex << addr << " "
            << std::left << name << "\n";
    }
    PRINT("%s \n",oss.str().c_str());
}

/**
 * @brief Print summary of all thermal zones
 *
 * Displays a table with ID, address, name, governor, current temperature,
 * and last temperature for all thermal zones in the system.
 */
void Thermal::print_zone_device(){
    size_t name_max_len = 10;
    size_t governor_name_max_len = 10;
    for (auto& zone_ptr : zone_list) {
        name_max_len = std::max(name_max_len,zone_ptr->name.size());
        governor_name_max_len = std::max(governor_name_max_len,zone_ptr->governor.size());
    }
    std::ostringstream oss;
    oss << std::left << std::setw(5) << "ID" << " "
        << std::left << std::setw(VADDR_PRLEN + 2) << "ADDR" << " "
        << std::left << std::setw(name_max_len) << "Name" << " "
        << std::left << std::setw(governor_name_max_len + 2) << "governor" << " "
        << std::left << std::setw(10) << "cur_temp" << " "
        << std::left << std::setw(10) << "last_temp" << " "
        << "\n";
    for (auto& zone_ptr : zone_list) {
        oss << std::left << std::hex << std::setw(5) << std::dec << zone_ptr->id << " "
            << std::left << std::setw(VADDR_PRLEN + 2) << std::hex << zone_ptr->addr << " "
            << std::left << std::setw(name_max_len) << zone_ptr->name << " "
            << std::left << std::setw(governor_name_max_len + 2) << zone_ptr->governor << " "
            << std::left << std::setw(10) << std::dec << zone_ptr->cur_temp << " "
            << std::left << std::setw(10) << std::dec << zone_ptr->last_temp << " "
            << "\n";
    }
    PRINT("%s \n",oss.str().c_str());
}

/**
 * @brief Parse all thermal zones from kernel memory
 *
 * Iterates through the kernel's thermal_tz_list and extracts information
 * about each thermal zone including:
 * - Zone ID, name, and governor
 * - Current and last temperature readings
 * - Trip points with temperature thresholds
 * - Cooling devices associated with each trip point
 *
 * Handles both direct trip storage and __thermal_zone indirection.
 */
void Thermal::parser_thrermal_zone(){
    LOGD("Starting thermal zone parsing");
    // Check if thermal_tz_list symbol exists
    if (!csymbol_exists("thermal_tz_list")){
        LOGE("thermal_tz_list doesn't exist in this kernel");
        return;
    }

    ulong list_head = csymbol_value("thermal_tz_list");
    if (!is_kvaddr(list_head)) {
        LOGE("thermal_tz_list address 0x%lx is invalid", list_head);
        return;
    }
    LOGD("thermal_tz_list at 0x%lx", list_head);
    int offset = field_offset(thermal_zone_device, node);
    // Iterate through thermal zone list
    for (const auto& addr : for_each_list(list_head,offset)) {
        void *dev_buf = read_struct(addr,"thermal_zone_device");
        std::shared_ptr<zone_dev> zone_ptr = std::make_shared<zone_dev>();
        // Read basic zone information
        zone_ptr->addr = addr;
        zone_ptr->id = INT(dev_buf + field_offset(thermal_zone_device,id));
        zone_ptr->cur_temp = INT(dev_buf + field_offset(thermal_zone_device,temperature));
        zone_ptr->last_temp = INT(dev_buf + field_offset(thermal_zone_device,last_temperature));
        zone_ptr->name = read_cstring(addr + field_offset(thermal_zone_device,type),20, "type");

        LOGD("Parsing thermal_zone_device: addr=0x%lx, id=%d, name=%s, cur_temp=%d, last_temp=%d",
             addr, zone_ptr->id, zone_ptr->name.c_str(), zone_ptr->cur_temp, zone_ptr->last_temp);

        // Read thermal governor name (handle both inline and pointer storage)
        ulong governor_addr = ULONG(dev_buf + field_offset(thermal_zone_device,governor));
        if ((long unsigned int)field_size(thermal_governor,name) > sizeof(void*)){
            // Governor name stored inline
            zone_ptr->governor = read_cstring(governor_addr,20, "governor name");
        }else{
            // Governor name stored as pointer
            ulong name_addr = read_pointer(governor_addr + field_offset(thermal_governor,name),"name addr");
            if (is_kvaddr(name_addr)) {
                zone_ptr->governor = read_cstring(name_addr,64, "governor name");
            }
        }

        // Read trip point information (handle both storage methods)
        size_t trip_cnt = 0;
        ulong trip_addr = 0;

        if (field_offset(thermal_zone_device,trips) != -1){
            // Direct trip storage in thermal_zone_device
            trip_cnt = INT(dev_buf + field_offset(thermal_zone_device,num_trips));
            trip_addr = ULONG(dev_buf + field_offset(thermal_zone_device,trips));
        }else{
            // Trip storage via __thermal_zone indirection
            ulong tz_addr = ULONG(dev_buf + field_offset(thermal_zone_device,devdata));
            void *tz_buf = read_struct(tz_addr,"__thermal_zone");
            trip_cnt = INT(tz_buf + field_offset(__thermal_zone,ntrips));
            trip_addr = ULONG(tz_buf + field_offset(__thermal_zone,trips));
            FREEBUF(tz_buf);
        }

        // Parse trip points
        if (is_kvaddr(trip_addr)) {
            for (size_t i = 0; i < trip_cnt; i++){
                ulong tt_addr = trip_addr + i * struct_size(thermal_trip);
                std::shared_ptr<trip> trip_ptr = std::make_shared<trip>();
                trip_ptr->addr = tt_addr;
                trip_ptr->temp = read_int(tt_addr + field_offset(thermal_trip,temperature),"temperature");
                zone_ptr->trip_list.push_back(trip_ptr);
            }
        }

        // Parse cooling devices for each trip point
        if (zone_ptr->trip_list.size() > 0){
            int node_offset = field_offset(thermal_instance, tz_node);
            ulong head_addr = addr + field_offset(thermal_zone_device,thermal_instances);
            // Iterate through thermal instances (trip-cooling device bindings)
            for (const auto& ins_addr : for_each_list(head_addr,node_offset)) {
                void *ins_buf = read_struct(ins_addr,"thermal_instance");
                size_t trip = INT(ins_buf + field_offset(thermal_instance,trip));

                // Validate trip index
                if (trip >= zone_ptr->trip_list.size()) {
                    LOGE("Invalid trip index %zu (max:%zu)", trip, zone_ptr->trip_list.size());
                    FREEBUF(ins_buf);
                    continue;
                }

                ulong cdev = ULONG(ins_buf + field_offset(thermal_instance,cdev));
                FREEBUF(ins_buf);

                if (!is_kvaddr(cdev)) {
                    continue;
                }

                // Read cooling device information
                std::shared_ptr<cool_dev> cdev_ptr = std::make_shared<cool_dev>();
                cdev_ptr->addr = cdev;
                ulong type_addr = read_pointer(cdev + field_offset(thermal_cooling_device,type),"type addr");
                if (is_kvaddr(type_addr)){
                    cdev_ptr->name = read_cstring(type_addr,64, "type");
                }else{
                    cdev_ptr->name = "";
                }
                cdev_ptr->id = read_int(cdev + field_offset(thermal_cooling_device,id), "id");
                zone_ptr->trip_list[trip]->cool_list.push_back(cdev_ptr);
            }
        }
        FREEBUF(dev_buf);
        zone_list.push_back(zone_ptr);
    }
}

#pragma GCC diagnostic pop
