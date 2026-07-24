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

#include "regulator.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(Regulator)
#endif

/**
 * @brief Main command entry point for regulator analysis
 *
 * Parses command line arguments and dispatches to appropriate handler functions:
 * -a: Display all regulators with comprehensive consumer information
 * -r: Display regulator device summary table
 * -c <name>: Display consumers for specific regulator by name
 */
void Regulator::cmd_main(void) {
    // Check minimum argument count
    if (argcnt < 2) {
        LOGD("Insufficient arguments provided, showing usage\n");
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }

    // Parse regulator devices if not already done
    if (regulator_list.empty()) {
        LOGD("Regulator list is empty, parsing regulator devices from kernel\n");
        parser_regulator_dev();
    } else {
        LOGD("Using cached regulator list with %zu devices\n", regulator_list.size());
    }

    int argerrs = 0;
    int c;
    std::string regulator_name;

    // Parse command line options
    while ((c = getopt(argcnt, args, "arc:")) != EOF) {
        switch(c) {
            case 'a':
                print_regulator_info();
                break;
            case 'r':
                print_regulator_dev();
                break;
            case 'c':
                regulator_name.assign(optarg);
                print_regulator_consumer(regulator_name);
                break;
            default:
                LOGD("Unknown option: -%c\n", c);
                argerrs++;
                break;
        }
    }

    // Handle argument errors
    if (argerrs) {
        LOGE("Command line argument errors detected: %d\n", argerrs);
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }
}

/**
 * @brief Initialize kernel structure field offsets for regulator framework
 *
 * Sets up field offsets for all regulator framework related kernel structures.
 * These offsets are essential for reading regulator data from kernel memory
 * across different kernel versions and configurations.
 */
void Regulator::init_offset(void) {
    // Initialize device class and subsystem structure offsets
    field_init(class,p);                        // Class private data pointer
    field_init(subsys_private,klist_devices);   // Device list in subsystem
    field_init(klist,k_list);                   // Kernel list head
    field_init(klist_node, n_node);             // Kernel list node
    field_init(device_private,knode_class);     // Class node in device private
    field_init(device_private,device);          // Device pointer in private data

    // Initialize regulator_dev structure offsets
    field_init(regulator_dev,dev);              // Device structure
    field_init(regulator_dev,constraints);      // Regulation constraints pointer
    field_init(regulator_dev,desc);             // Regulator descriptor pointer
    field_init(regulator_dev,use_count);        // Usage reference count
    field_init(regulator_dev,open_count);       // Open handle count
    field_init(regulator_dev,bypass_count);     // Bypass mode reference count
    field_init(regulator_dev,consumer_list);    // Consumer list head

    // Initialize regulation_constraints structure offsets
    field_init(regulation_constraints,name);    // Constraint name string
    field_init(regulation_constraints,min_uV);  // Minimum voltage constraint
    field_init(regulation_constraints,max_uV);  // Maximum voltage constraint
    field_init(regulation_constraints,input_uV); // Input voltage

    // Initialize regulator_desc structure offsets
    field_init(regulator_desc,name);            // Regulator descriptor name

    // Initialize regulator (consumer) structure offsets
    field_init(regulator,list);                 // List linkage for consumer chain
    field_init(regulator,uA_load);              // Current load in microamps
    field_init(regulator,enable_count);         // Enable reference count
    field_init(regulator,voltage);              // Voltage constraint array
    field_init(regulator,supply_name);          // Supply name string pointer

    // Initialize regulator_voltage structure offsets
    field_init(regulator_voltage,min_uV);       // Minimum voltage in microvolts
    field_init(regulator_voltage,max_uV);       // Maximum voltage in microvolts

    // Initialize structure sizes
    struct_init(regulator_voltage);             // Voltage constraint structure size
}

void Regulator::init_command(void) {
    cmd_name = "reg";
    help_str_list={
        "reg",                                /* command name */
        "display power regulator framework information and analysis",  /* short description */
        "[-a] [-r] [-c regulator_name]\n"
        "  This command analyzes the power regulator framework from crash dumps,\n"
        "  providing comprehensive information about voltage regulators, power domains,\n"
        "  consumer relationships, and power management constraints.\n"
        "\n"
        "    -a              display all regulators with detailed consumer information\n"
        "    -r              display regulator device summary table with key statistics\n"
        "    -c regulator_name  display consumers for specific regulator by name\n",
        "\n",
        "EXAMPLES",
        "  Display all regulators with comprehensive consumer information:",
        "    %s> reg -a",
        "    regulator_dev:ffffff8006994800 regulator-dummy open_count:4 use_count:3 bypass_count:0 min_uV:0 max_uV:0 input_uV:0",
        "       regulator:ffffff801e51c000 enable:0 load:0uA 5e94000,mdss_dsi0_ctrl-refgen",
        "       regulator:ffffff801e51c0c0 enable:0 load:0uA 5e94400,mdss_dsi_phy0-gdsc",
        "       regulator:ffffff801ad6cc00 enable:1 load:0uA soc:usb_nop_phy-vcc",
        "       regulator:ffffff800a036780 enable:1 load:0uA regulator.21-SUPPLY",
        "\n",
        "  Display regulator device summary table:",
        "    %s> reg -r",
        "    regulator_dev    open use bypass regulator_desc   constraints      min_uV   max_uV   input_uV Name",
        "    ffffff8006994800 4    3   0      ffffffeee20a0020 ffffff80067f8d00 0        0        0        regulator-dummy",
        "    ffffff8007811800 17   0   0      ffffff8003999048 ffffff80033ebb00 0        0        0        gcc_camss_top_gdsc",
        "    ffffff8007813000 1    1   0      ffffff8003999448 ffffff80033eb700 0        0        0        gcc_usb20_prim_gdsc",
        "    ffffff8007815000 3    0   0      ffffff800a39bc48 ffffff8007a39e00 0        0        0        gpu_cx_gdsc",
        "    ffffff8007814000 1    0   0      ffffff800a398048 ffffff8007a39c00 0        0        0        gpu_gx_gdsc",
        "    ffffff8007bac800 0    0   0      ffffff8002114400 ffffff8012ee5200 1816000  1904000  1904000  pm5100_s4",
        "    ffffff8002158000 0    0   0      ffffff8002114600 ffffff8012ee5900 1000000  1000000  1000000  pm5100_l11",
        "\n",
        "  Display consumers for specific regulator:",
        "    %s> reg -c pm5100_l12",
        "    Consumers:",
        "       regulator        load       enable Name",
        "       ffffff802a899a80 0uA        0      5c54000,csiphy1-mipi-csi-vdd1",
        "       ffffff802a883780 0uA        0      5c52000,csiphy0-mipi-csi-vdd1",
        "       ffffff801e51ca80 0uA        0      5e94400,mdss_dsi_phy0-vdda-0p9 voltage:[904000uV~904000uV,]",
        "       ffffff801ad6ce40 30000uA    1      1613000.hsphy-vdd voltage:[904000uV~904000uV,]",
        "       ffffff8002204f00 0uA        1      regulator.59-SUPPLY",
        "\n",
    };
}

/**
 * @brief Default constructor
 *
 * Initializes the regulator parser with default settings.
 */
Regulator::Regulator(){

}

/**
 * @brief Display consumer information for a specific regulator by name
 * @param reg_name Name of the regulator to display consumers for
 *
 * Searches through all discovered regulator devices for the specified regulator
 * name and displays detailed information about all its consumers, including
 * their load requirements, enable states, and voltage constraints in a
 * formatted table layout.
 */
void Regulator::print_regulator_consumer(std::string reg_name){
    LOGD("Searching for regulator: %s in %zu devices\n", reg_name.c_str(), regulator_list.size());
    bool found_regulator = false;
    std::ostringstream oss;

    // Search for the specified regulator by name
    for (const auto& dev_ptr : regulator_list) {
        if (dev_ptr->name != reg_name){
            continue;
        }

        found_regulator = true;
        LOGD("Found regulator %s with %zu consumers\n", reg_name.c_str(), dev_ptr->consumers.size());

        // Display consumers header
        PRINT("Consumers: \n");
        // Calculate maximum name length for proper column alignment
        size_t max_len = 0;
        for (const auto& c_ptr : dev_ptr->consumers) {
            max_len = std::max(max_len, c_ptr->name.size());
        }
        // Print table header with proper column alignment
        oss << std::left << std::setw(16) << "regulator" << " "
            << std::left << std::setw(10) << "load" << " "
            << std::left << std::setw(6)  << "enable" << " "
            << std::left << std::setw(max_len) << "Name"
            << std::left << "voltage";
        PRINT("   %s \n", oss.str().c_str());

        // Display each consumer with detailed information
        for (const auto& c_ptr : dev_ptr->consumers) {
            oss.str("");
            oss << std::left << std::setw(16) << std::hex << c_ptr->addr << " "
                << std::left << std::setw(10) << std::dec << std::to_string(c_ptr->load) + "uA" << " "
                << std::left << std::setw(6)  << std::dec << c_ptr->enable_count << " "
                << std::left << std::setw(max_len) << c_ptr->name;
            // Add voltage constraints if present
            if (c_ptr->voltages.size() > 0){
                oss << "[";
                for (const auto& vol : c_ptr->voltages) {
                    oss << std::to_string(vol->min_uV) << "uV~" << std::to_string(vol->max_uV) << "uV,";
                }
                oss << "]";
            }
            PRINT("   %s \n", oss.str().c_str());
        }
    }
    // Handle case where regulator was not found
    if (!found_regulator) {
        LOGE("Regulator '%s' not found in %zu available regulators\n",
             reg_name.c_str(), regulator_list.size());
    }
}

/**
 * @brief Display comprehensive regulator and consumer information
 *
 * Shows detailed information for all regulator devices including their
 * consumers, with hierarchical formatting to show the relationship between
 * regulators and their consumers. Each regulator is displayed with its
 * usage statistics and voltage constraints, followed by all its consumers
 * with their individual load requirements and voltage constraints.
 */
void Regulator::print_regulator_info(){
    // Check if we have any regulators to display
    if(regulator_list.empty()) {
        LOGD("No regulator devices found in regulator_list\n");
        return;
    }

    std::ostringstream oss;
    // Display each regulator device with its consumers
    for (const auto& dev_ptr : regulator_list) {
        // Format regulator device information
        oss.str("");
        oss << std::left << "regulator_dev:" << std::hex << dev_ptr->addr << " "
            << std::left << dev_ptr->name << " "
            << std::left << "open_count:" << std::dec << dev_ptr->open_count << " "
            << std::left << "use_count:" << std::dec << dev_ptr->use_count << " "
            << std::left << "bypass_count:" << std::dec << dev_ptr->bypass_count << " "
            << std::left << "min_uV:" << std::dec << dev_ptr->min_uV << " "
            << std::left << "max_uV:" << std::dec << dev_ptr->max_uV << " "
            << std::left << "input_uV:" << std::dec << dev_ptr->input_uV;
        PRINT("%s \n", oss.str().c_str());

        // Display all consumers for this regulator
        for (const auto& c_ptr : dev_ptr->consumers) {
            // Format consumer information with indentation
            oss.str("");
            oss << std::left << "regulator:" << std::hex << c_ptr->addr << " "
                << std::left << "enable:" << std::dec << c_ptr->enable_count << " "
                << std::left << "load:" << std::dec << std::to_string(c_ptr->load) + "uA" << " ";
            // Add voltage constraints if present
            if (c_ptr->voltages.size() > 0){
                oss << " voltage:[";
                for (const auto& vol : c_ptr->voltages) {
                    oss << std::to_string(vol->min_uV) << "uV~" << std::to_string(vol->max_uV) << "uV,";
                }
                oss << "] ";
            }
            // Add consumer name
            oss << c_ptr->name;
            PRINT("   %s \n", oss.str().c_str());
        }
        // Add spacing between regulators for readability
        PRINT("\n\n");
    }
}

/**
 * @brief Display regulator device summary information in tabular format
 *
 * Shows a tabular summary of all regulator devices with their key statistics
 * including usage counts, voltage constraints, and device addresses. This
 * provides a concise overview of all regulators in the system without the
 * detailed consumer information.
 */
void Regulator::print_regulator_dev(){
    // Check if we have any regulators to display
    if(regulator_list.empty()) {
        LOGD("No regulator devices found in regulator_list\n");
        return;
    }
    std::ostringstream oss;
    // Print table header with proper column alignment
    oss << std::left << std::setw(16) << "regulator_dev" << " "
        << std::left << std::setw(4)  << "open" << " "
        << std::left << std::setw(3)  << "use" << " "
        << std::left << std::setw(6)  << "bypass" << " "
        << std::left << std::setw(16) << "regulator_desc" << " "
        << std::left << std::setw(16) << "constraints" << " "
        << std::left << std::setw(8)  << "min_uV" << " "
        << std::left << std::setw(8)  << "max_uV" << " "
        << std::left << std::setw(8)  << "input_uV" << " "
        << std::left << "Name";
    PRINT("%s \n", oss.str().c_str());
    // Display each regulator device in tabular format
    for (const auto& dev_ptr : regulator_list) {
        // Format regulator information in table row
        oss.str("");
        oss << std::left << std::setw(16) << std::hex << dev_ptr->addr << " "
            << std::left << std::setw(4)  << std::dec << dev_ptr->open_count << " "
            << std::left << std::setw(3)  << dev_ptr->use_count << " "
            << std::left << std::setw(6)  << dev_ptr->bypass_count << " "
            << std::left << std::setw(16) << std::hex << dev_ptr->desc << " "
            << std::left << std::setw(16) << dev_ptr->constraint << " "
            << std::left << std::setw(8)  << std::dec << dev_ptr->min_uV << " "
            << std::left << std::setw(8)  << dev_ptr->max_uV << " "
            << std::left << std::setw(8)  << dev_ptr->input_uV << " "
            << std::left << dev_ptr->name;
        PRINT("%s \n", oss.str().c_str());
    }
}

/**
 * @brief Parse all regulator devices from kernel regulator class
 *
 * Traverses the kernel's regulator device class to discover all regulator
 * devices and their associated consumers. Extracts detailed information
 * including constraints, usage statistics, and consumer relationships.
 *
 * This function processes:
 * - PMIC regulators (LDO, Buck converters)
 * - GDSC power domains
 * - Voltage regulators and current sources
 * - Consumer-regulator relationships and constraints
 */
void Regulator::parser_regulator_dev(){
    size_t devices_found = 0;
    size_t devices_processed = 0;
    size_t total_consumers = 0;

    // Iterate through all devices in the regulator class
    for (auto& dev : for_each_device_for_class("regulator")) {
        devices_found++;

        // Adjust address to point to regulator_dev structure
        // (device address points to embedded dev field, need to get container)
        ulong addr = dev->addr - field_offset(regulator_dev,dev);

        LOGD("Processing regulator device %zu at address 0x%lx\n", devices_found, addr);

        // Create new regulator device object
        std::shared_ptr<regulator_dev> dev_ptr = std::make_shared<regulator_dev>();
        dev_ptr->addr = addr;

        // Read regulation constraints pointer
        dev_ptr->constraint = read_pointer(addr + field_offset(regulator_dev,constraints),"regulation_constraints");
        LOGD("Regulator constraints pointer: 0x%lx\n", dev_ptr->constraint);

        // Extract regulator name and voltage constraints from regulation_constraints
        ulong name_addr = 0;
        if (is_kvaddr(dev_ptr->constraint)){
            // Read constraint name
            name_addr = read_pointer(dev_ptr->constraint + field_offset(regulation_constraints,name),"name");
            if (is_kvaddr(name_addr)){
                dev_ptr->name = read_cstring(name_addr,64, "regulator name");
                LOGD("Regulator name from constraints: %s\n", dev_ptr->name.c_str());
            }

            // Read voltage constraints
            dev_ptr->min_uV = read_int(dev_ptr->constraint + field_offset(regulation_constraints,min_uV),"min_uV");
            dev_ptr->max_uV = read_int(dev_ptr->constraint + field_offset(regulation_constraints,max_uV),"max_uV");
            dev_ptr->input_uV = read_int(dev_ptr->constraint + field_offset(regulation_constraints,input_uV),"input_uV");

            LOGD("Voltage constraints: min=%d uV, max=%d uV, input=%d uV\n",
                 dev_ptr->min_uV, dev_ptr->max_uV, dev_ptr->input_uV);
        }

        // Read regulator descriptor pointer
        dev_ptr->desc = read_pointer(addr + field_offset(regulator_dev,desc),"regulator_desc");
        LOGD("Regulator descriptor pointer: 0x%lx\n", dev_ptr->desc);

        // Fallback: try to get name from regulator descriptor if not found in constraints
        if(dev_ptr->name.empty()){
            if (is_kvaddr(dev_ptr->desc)){
                name_addr = read_pointer(dev_ptr->desc + field_offset(regulator_desc,name),"name");
                if (is_kvaddr(name_addr)){
                    dev_ptr->name = read_cstring(name_addr,64, "regulator name");
                    LOGD("Regulator name from descriptor: %s\n", dev_ptr->name.c_str());
                }
            }
        }

        // Set default name if still empty
        if(dev_ptr->name.empty()){
            dev_ptr->name = "Unknown";
            LOGD("Using default name 'Unknown' for regulator at 0x%lx\n", addr);
        }

        // Read usage statistics
        dev_ptr->use_count = read_uint(addr + field_offset(regulator_dev,use_count),"use_count");
        dev_ptr->open_count = read_uint(addr + field_offset(regulator_dev,open_count),"open_count");
        dev_ptr->bypass_count = read_uint(addr + field_offset(regulator_dev,bypass_count),"bypass_count");

        LOGD("Usage statistics: use_count=%u, open_count=%u, bypass_count=%u\n",
             dev_ptr->use_count, dev_ptr->open_count, dev_ptr->bypass_count);

        // Parse consumer list
        ulong consumer_list = addr + field_offset(regulator_dev,consumer_list);
        int offset = field_offset(regulator, list);
        size_t consumers_found = 0;

        LOGD("Parsing consumer list at 0x%lx (offset=%d)\n", consumer_list, offset);

        // Iterate through all consumers of this regulator
        for (const auto& reg : for_each_list(consumer_list,offset)) {
            consumers_found++;
            LOGD("Processing consumer %zu at address 0x%lx\n", consumers_found, reg);

            // Create new regulator consumer object
            std::shared_ptr<regulator> reg_ptr = std::make_shared<regulator>();
            reg_ptr->addr = reg;

            // Read consumer load and enable count
            reg_ptr->load = read_int(reg + field_offset(regulator,uA_load),"uA_load");
            reg_ptr->enable_count = read_uint(reg + field_offset(regulator,enable_count),"enable_count");

            LOGD("Consumer load: %d uA, enable_count: %u\n", reg_ptr->load, reg_ptr->enable_count);

            // Read consumer supply name
            ulong name_addr = read_pointer(reg + field_offset(regulator,supply_name),"name");
            if (is_kvaddr(name_addr)){
                reg_ptr->name = read_cstring(name_addr,64, "supply_name");
                LOGD("Consumer supply name: %s\n", reg_ptr->name.c_str());
            } else {
                reg_ptr->name = "unnamed_consumer";
                LOGE("Consumer has no valid supply name\n");
            }

            // Parse voltage constraints array
            size_t cnt = field_size(regulator,voltage)/struct_size(regulator_voltage);
            for(size_t i=0; i < cnt; i++){
                ulong vol_addr = reg + field_offset(regulator,voltage) + i * struct_size(regulator_voltage);
                int min_uV = read_int(vol_addr + field_offset(regulator_voltage,min_uV),"min_uV");
                int max_uV = read_int(vol_addr + field_offset(regulator_voltage,max_uV),"max_uV");

                // Only store non-zero voltage constraints
                if(min_uV != 0 || max_uV != 0){
                    std::shared_ptr<voltage> vol_ptr = std::make_shared<voltage>();
                    vol_ptr->min_uV = min_uV;
                    vol_ptr->max_uV = max_uV;
                    reg_ptr->voltages.push_back(vol_ptr);
                    LOGD("Voltage constraint %zu: %d uV ~ %d uV\n", i, min_uV, max_uV);
                }
            }
            // Add consumer to regulator's consumer list
            dev_ptr->consumers.push_back(reg_ptr);
        }

        LOGD("Regulator %s has %zu consumers\n", dev_ptr->name.c_str(), consumers_found);
        total_consumers += consumers_found;

        // Add regulator device to global list
        regulator_list.push_back(dev_ptr);
        devices_processed++;

        LOGD("Successfully processed regulator device %s (device %zu/%zu)\n",
             dev_ptr->name.c_str(), devices_processed, devices_found);
    }

    // Log final statistics
    LOGD("Regulator device parsing completed:\n");
    LOGD("  - Total devices found: %zu\n", devices_found);
    LOGD("  - Devices successfully processed: %zu\n", devices_processed);
    LOGD("  - Total consumers discovered: %zu\n", total_consumers);
    LOGD("  - Final regulator_list size: %zu\n", regulator_list.size());
}

#pragma GCC diagnostic pop
