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

#include "icc.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(ICC)
#endif

/**
 * @brief Main command entry point for ICC (Interconnect) analysis
 *
 * Parses command line arguments and dispatches to appropriate handler functions:
 * -a: Display comprehensive interconnect topology and bandwidth information
 * -p: Display interconnect provider summary table
 * -n <name>: Display nodes for specific interconnect provider
 * -r <name>: Display bandwidth requests for specific interconnect node
 */
void ICC::cmd_main(void) {
    // Check minimum argument count
    if (argcnt < 2) {
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }

    // Parse interconnect providers if not already done
    if (provider_list.empty()) {
        LOGD("Provider list is empty, parsing ICC providers from kernel");
        parser_icc_provider();
    } else {
        LOGD("Using cached provider list with %zu providers", provider_list.size());
    }

    int argerrs = 0;
    int c;
    std::string target_name;
    // Parse command line options
    while ((c = getopt(argcnt, args, "apn:r:")) != EOF) {
        switch(c) {
            case 'a':
                print_icc_info();
                break;
            case 'p':
                print_icc_provider();
                break;
            case 'n':
                target_name.assign(optarg);
                print_icc_nodes(target_name);
                break;
            case 'r':
                target_name.assign(optarg);
                print_icc_request(target_name);
                break;
            default:
                LOGD("Unknown option: -%c", c);
                argerrs++;
                break;
        }
    }

    // Handle argument errors
    if (argerrs) {
        LOGE("Command line argument errors detected: %d", argerrs);
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }
}

/**
 * @brief Initialize kernel structure field offsets for ICC framework
 *
 * Sets up field offsets for all interconnect framework related kernel structures.
 * These offsets are essential for reading ICC data from kernel memory across
 * different kernel versions and configurations.
 */
void ICC::init_offset(void) {
    // Initialize icc_provider structure offsets
    field_init(icc_provider,provider_list);     // Provider list linkage
    field_init(icc_provider,nodes);             // List head for managed nodes
    field_init(icc_provider,dev);               // Associated device structure
    field_init(icc_provider,users);             // Number of active users
    field_init(icc_provider,data);              // Provider-specific data
    struct_init(icc_provider);                  // Provider structure size

    // Initialize icc_node structure offsets
    field_init(icc_node,id);                    // Unique node identifier
    field_init(icc_node,name);                  // Node name string pointer
    field_init(icc_node,node_list);             // Node list linkage
    field_init(icc_node,avg_bw);                // Current average bandwidth
    field_init(icc_node,peak_bw);               // Current peak bandwidth
    field_init(icc_node,init_avg);              // Initial average bandwidth
    field_init(icc_node,init_peak);             // Initial peak bandwidth
    field_init(icc_node,data);                  // Node-specific data pointer
    field_init(icc_node,req_list);              // Request list head
    struct_init(icc_node);                      // Node structure size

    // Initialize icc_req structure offsets
    field_init(icc_req,req_node);               // Request node linkage
    field_init(icc_req,dev);                    // Requesting device pointer
    field_init(icc_req,enabled);                // Request enable state
    field_init(icc_req,tag);                    // Request tag for grouping
    field_init(icc_req,avg_bw);                 // Requested average bandwidth
    field_init(icc_req,peak_bw);                // Requested peak bandwidth
    struct_init(icc_req);                       // Request structure size
}

/**
 * @brief Initialize command metadata and help documentation
 *
 * Sets up the command name and comprehensive help text including:
 * - Command synopsis and usage patterns
 * - Detailed examples for each command option
 * - Expected output formats for different query types
 */
void ICC::init_command(void) {
    cmd_name = "icc";
    help_str_list={
        "icc",                                         /* command name */
        "dump ICC (Interconnect) information",        /* short description */
        "[-a] [-p] [-n provider_name] [-r node_name]\n"
        "  This command dumps ICC (Interconnect) framework information.\n"
        "\n"
        "    -a              display comprehensive interconnect topology and bandwidth information\n"
        "    -p              display interconnect provider summary table\n"
        "    -n provider_name display nodes for specific interconnect provider\n"
        "    -r node_name    display bandwidth requests for specific interconnect node\n",
        "\n",
        "EXAMPLES",
        "  Display all ICC provider/node/request information:",
        "    %s> icc -a",
        "    icc_provider:ffffff8002241140 soc:interconnect",
        "       icc_node:ffffff8002106980 avg_bw:2 peak_bw:1 [qup0_core_master]",
        "           icc_req:ffffff801adf7990 avg_bw:76800 peak_bw:76800 [4a90000.spi]",
        "           icc_req:ffffff801adf7790 avg_bw:1 peak_bw:1 [4a84000.i2c]",
        "           icc_req:ffffff8019444e10 avg_bw:1 peak_bw:1 [4a80000.i2c]",
        "\n",
        "  Display all ICC providers:",
        "    %s> icc -p",
        "    icc_provider     users data             Name",
        "    ffffff8002241140 10    ffffff8006998040 soc:interconnect",
        "    ffffff8002249d40 12    ffffff800699c040 soc:interconnect@0",
        "    ffffff8002249b40 12    ffffff8002220040 soc:interconnect@1",
        "    ffffff8002249340 30    ffffff8002224040 1900000.interconnect",
        "\n",
        "  Display all ICC nodes of specified provider:",
        "    %s> icc -n 1900000.interconnect",
        "    icc_node         id    avg_bw     peak_bw    qcom_icc_node    Name",
        "    ffffff8002254400 0     478572     2188000    ffffffeedf05e1f0 apps_proc",
        "    ffffff800224b480 1     0          0          ffffffeedf05e360 mas_snoc_bimc_rt",
        "    ffffff800224be80 2     0          0          ffffffeedf05e4d0 mas_snoc_bimc_nrt",
        "    ffffff8002254f80 3     396800     2160000    ffffffeedf05e640 mas_snoc_bimc",
        "\n",
        "  Display all ICC requests for specified node:",
        "    %s> icc -r apps_proc",
        "    icc_req          enabled tag  avg_bw     peak_bw    Name",
        "    ffffff8018162e10 true    0    0          40000      4e00000.hsusb",
        "    ffffff801a28aa10 true    0    0          0          4453000.qrng",
        "    ffffff8018e09410 false   0    76800      76800      4a90000.spi",
        "    ffffff8018e08410 false   0    1          1          4a84000.i2c",
        "    ffffff80157b7a10 true    0    1          1          4a80000.i2c",
        "\n",
    };
}

/**
 * @brief Default constructor
 *
 * Initializes the ICC parser with default settings.
 */
ICC::ICC(){
    // Constructor - no logging needed
}

/**
 * @brief Display bandwidth requests for a specific interconnect node by name
 * @param node_name Name of the interconnect node to display requests for
 *
 * Searches through all discovered interconnect providers and nodes for the
 * specified node name and displays detailed information about all bandwidth
 * requests targeting that node, including request sources, bandwidth values,
 * enable states, and tags in a formatted table layout.
 */
void ICC::print_icc_request(std::string node_name){
    bool found_node = false;
    std::ostringstream oss;
    // Search through all providers and their nodes
    for (const auto& provider_ptr : provider_list) {
        for (const auto& node_ptr : provider_ptr->node_list) {
            if (node_ptr->name != node_name){
                continue;
            }
            found_node = true;
            // Check if node has any requests
            if(node_ptr->req_list.empty()) {
                LOGW("Node '%s' has no bandwidth requests", node_name.c_str());
                return;
            }
            // Print table header
            oss  << std::left << std::setw(VADDR_PRLEN)  << "icc_req" << " "
                    << std::left << std::setw(7)            << "enabled" << " "
                    << std::left << std::setw(4)            << "tag" << " "
                    << std::left << std::setw(10)           << "avg_bw" << " "
                    << std::left << std::setw(10)           << "peak_bw" << " "
                    << std::left << "Name" << "\n";
            // Display each bandwidth request
            for (const auto& req_ptr : node_ptr->req_list) {
                oss << std::left << std::setw(VADDR_PRLEN)  << std::hex << req_ptr->addr << " "
                    << std::left << std::setw(7)            << (req_ptr->enabled ? "true": "false") << " "
                    << std::left << std::setw(4)            << std::dec << req_ptr->tag << " "
                    << std::left << std::setw(10)           << std::dec << req_ptr->avg_bw << " "
                    << std::left << std::setw(10)           << std::dec << req_ptr->peak_bw << " "
                    << std::left << req_ptr->name << "\n";
            }
        }
    }
    // Handle case where node was not found
    if (!found_node) {
        LOGE("ICC node '%s' not found", node_name.c_str());
        return;
    }
    // Output the formatted table
    PRINT("%s \n", oss.str().c_str());
}

/**
 * @brief Display all nodes managed by a specific interconnect provider
 * @param provider_name Name of the interconnect provider
 *
 * Searches through all discovered interconnect providers for the specified
 * provider name and displays a tabular list of all interconnect nodes
 * managed by that provider, including node IDs, current bandwidth aggregation,
 * and provider-specific data pointers.
 */
void ICC::print_icc_nodes(std::string provider_name){
    bool found_provider = false;
    std::ostringstream oss;
    // Search for the specified provider by name
    for (const auto& provider_ptr : provider_list) {
        if (provider_ptr->name != provider_name){
            continue;
        }
        found_provider = true;
        // Check if provider has any nodes
        if(provider_ptr->node_list.empty()) {
            LOGW("Provider '%s' has no interconnect nodes", provider_name.c_str());
            return;
        }
        // Print table header
        oss  << std::left << std::setw(VADDR_PRLEN)  << "icc_node" << " "
                << std::left << std::setw(5)            << "id" << " "
                << std::left << std::setw(10)           << "avg_bw" << " "
                << std::left << std::setw(10)           << "peak_bw" << " "
                << std::left << std::setw(16)  << "qcom_icc_node" << " "
                << std::left << "Name" "\n";

        // Display each interconnect node
        for (const auto& node_ptr : provider_ptr->node_list) {
            oss << std::left << std::setw(VADDR_PRLEN)  << std::hex << node_ptr->addr << " "
                << std::left << std::setw(5)            << std::dec << node_ptr->id << " "
                << std::left << std::setw(10)           << std::dec << node_ptr->avg_bw << " "
                << std::left << std::setw(10)           << std::dec << node_ptr->peak_bw << " "
                << std::left << std::setw(16)           << std::hex << node_ptr->data << " "
                << std::left << node_ptr->name << "\n";
        }
    }
    // Handle case where provider was not found
    if (!found_provider) {
        LOGE("ICC provider '%s' not found", provider_name.c_str());
        return;
    }
    // Output the formatted table
    PRINT("%s \n", oss.str().c_str());
}

/**
 * @brief Display comprehensive interconnect topology and bandwidth information
 *
 * Outputs a hierarchical view of the entire interconnect framework showing:
 * - All registered interconnect providers with their kernel addresses
 * - All nodes under each provider with aggregated bandwidth values
 * - All bandwidth requests for each node with source device information
 * This provides a complete picture of the system's interconnect usage.
 */
void ICC::print_icc_info(){
    std::ostringstream oss;
    // Iterate through all providers in the system
    for (const auto& provider_ptr : provider_list) {
        oss << std::left << "icc_provider:"   << std::hex << provider_ptr->addr << " "
            << std::left << provider_ptr->name
            << "\n";
        // Display all nodes managed by this provider
        for (const auto& node_ptr : provider_ptr->node_list) {
            oss << std::left << "      icc_node:"  << std::hex << node_ptr->addr << " "
                << std::left << "avg_bw:" << std::left << std::setw(10) << std::dec << node_ptr->avg_bw << " "
                << std::left << "peak_bw:" << std::left << std::setw(10) << std::dec << node_ptr->peak_bw << " "
                << std::left << "[" << node_ptr->name << "]"
                << "\n";
            // Display all bandwidth requests for this node
            for (const auto& req_ptr : node_ptr->req_list) {
                oss << std::left << "       icc_req:" << std::hex << req_ptr->addr << " "
                    << std::left << "avg_bw:" << std::left << std::setw(10) << std::dec << req_ptr->avg_bw << " "
                    << std::left << "peak_bw:" << std::left << std::setw(10) << std::dec << req_ptr->peak_bw << " "
                    << std::left << "[" << req_ptr->name << "]"
                    << "\n";
            }
            oss << "\n";
        }
        oss << "\n\n";
    }
    PRINT("%s \n",oss.str().c_str());
}

/**
 * @brief Display summary table of all interconnect providers
 *
 * Outputs a formatted table showing all registered interconnect providers
 * in the system with their kernel addresses, active user counts, and names.
 * This provides a high-level overview of the interconnect infrastructure.
 */
void ICC::print_icc_provider(){
    // Early return if no providers found
    if(provider_list.size() == 0) return;
    std::ostringstream oss;
    // Print table header
    oss  << std::left << std::setw(VADDR_PRLEN)   << "icc_provider" << " "
            << std::left << std::setw(5)    << "users" << " "
            // << std::left << std::setw(VADDR_PRLEN) << "data" << " "
            << std::left << "Name"
            << "\n";
    // Display each provider's summary information
    for (const auto& provider_ptr : provider_list) {
        oss << std::left << std::setw(VADDR_PRLEN)   << std::hex << provider_ptr->addr << " "
            << std::left << std::setw(5)    << std::dec << provider_ptr->users << " "
            // << std::left << std::setw(VADDR_PRLEN) << std::hex << provider_ptr->data << " "
            << std::left << provider_ptr->name
            << "\n";
    }
    PRINT("%s \n",oss.str().c_str());
}

/**
 * @brief Parse all interconnect providers from kernel ICC framework
 *
 * Traverses the kernel's global interconnect provider list to discover all
 * registered providers and their associated nodes and requests. This function
 * processes the complete interconnect topology including NoC fabrics, system
 * buses, and bandwidth management infrastructure.
 */
void ICC::parser_icc_provider(){
    LOGD("Starting to parse ICC providers from kernel interconnect framework");
    // Check if icc_providers symbol exists in kernel
    if (!csymbol_exists("icc_providers")){
        LOGE("icc_providers symbol doesn't exist - ICC framework not available");
        return;
    }

    // Get address of global icc_providers list
    ulong icc_providers_addr = csymbol_value("icc_providers");
    if (!is_kvaddr(icc_providers_addr)) {
        LOGE("icc_providers address 0x%lx is invalid", icc_providers_addr);
        return;
    }
    size_t providers_found = 0;
    // Iterate through all providers in the global list
    int offset = field_offset(icc_provider, provider_list);
    for (const auto& addr : for_each_list(icc_providers_addr,offset)) {
        providers_found++;
        LOGD("Processing icc_provider[%zu] at address 0x%lx", providers_found, addr);
        // Create new provider object
        std::shared_ptr<icc_provider> provider_ptr = std::make_shared<icc_provider>();
        provider_ptr->addr = addr;
        // Read provider user count
        provider_ptr->users = read_int(addr + field_offset(icc_provider,users),"users");
        // Read associated device pointer for name resolution
        ulong dev_addr = read_pointer(addr + field_offset(icc_provider,dev),"dev");
        // Extract provider name from device structure
        if (is_kvaddr(dev_addr)){
            ulong name_addr = read_pointer(dev_addr,"name addr");
            if (is_kvaddr(name_addr)){
                provider_ptr->name = read_cstring(name_addr,64, "name");
            } else {
                provider_ptr->name = "unnamed_provider";
            }
        } else {
            provider_ptr->name = "unknown_provider";
        }
        // Parse nodes managed by this provider
        ulong nodes_addr = addr + field_offset(icc_provider,nodes);
        parser_icc_node(provider_ptr,nodes_addr);
        // Add provider to global list
        provider_list.push_back(provider_ptr);
    }
}

/**
 * @brief Parse all interconnect nodes managed by a specific provider
 * @param provider_ptr Shared pointer to the provider object to populate
 * @param head Kernel address of the node list head
 *
 * Traverses the provider's node list and extracts detailed information for
 * each node including ID, bandwidth aggregation values, and associated data.
 * Also triggers parsing of bandwidth requests for each discovered node.
 */
void ICC::parser_icc_node(std::shared_ptr<icc_provider> provider_ptr,ulong head){
    LOGD("Parsing nodes for provider '%s' at address 0x%lx",
         provider_ptr->name.c_str(), head);
    int offset = field_offset(icc_node, node_list);
    // Iterate through all nodes in the provider's node list
    for (const auto& addr : for_each_list(head,offset)) {
        LOGD("Processing icc_node at address 0x%lx", addr);
        // Read the entire icc_node structure from kernel memory
        void *node_buf = read_struct(addr,"icc_node");
        if (!node_buf) {
            LOGW("Failed to read icc_node structure at 0x%lx", addr);
            continue;
        }
        // Create new node object and populate fields
        std::shared_ptr<icc_node> node_ptr = std::make_shared<icc_node>();
        node_ptr->addr = addr;
        node_ptr->id = UINT(node_buf + field_offset(icc_node,id));
        node_ptr->avg_bw = UINT(node_buf + field_offset(icc_node,avg_bw));
        node_ptr->peak_bw = UINT(node_buf + field_offset(icc_node,peak_bw));
        node_ptr->data = ULONG(node_buf + field_offset(icc_node,data));
        FREEBUF(node_buf);
        // Extract node name from kernel string
        ulong name_addr = read_pointer(addr + field_offset(icc_node,name),"name addr");
        if (is_kvaddr(name_addr)){
            node_ptr->name = read_cstring(name_addr,64, "name");
            LOGD("icc_node details: id=%d, name='%s', avg_bw=%u KB/s, peak_bw=%u KB/s",
                 node_ptr->id, node_ptr->name.c_str(), node_ptr->avg_bw, node_ptr->peak_bw);
        } else {
            LOGD("Invalid node name address 0x%lx, using empty name", name_addr);
            node_ptr->name = "";
        }
        // Add node to provider's node list
        provider_ptr->node_list.push_back(node_ptr);
        // Parse bandwidth requests for this node
        parser_icc_req(node_ptr, addr + field_offset(icc_node,req_list));
    }
}

/**
 * @brief Parse all bandwidth requests for a specific interconnect node
 * @param node_ptr Shared pointer to the node object to populate
 * @param head Kernel address of the request hash list head
 *
 * Traverses the node's request hash list and extracts detailed information
 * for each bandwidth request including requested bandwidth values, enable
 * state, tags, and the requesting device name. This provides visibility into
 * which devices are consuming bandwidth on each interconnect path.
 */
void ICC::parser_icc_req(std::shared_ptr<icc_node> node_ptr,ulong head){
    int offset = field_offset(icc_req, req_node);
    // Iterate through all requests in the node's hash list
    for (const auto& addr : for_each_hlist(head,offset)) {
        LOGD("Processing icc_req at address 0x%lx", addr);
        // Read the entire icc_req structure from kernel memory
        void *req_buf = read_struct(addr,"icc_req");
        if (!req_buf) {
            LOGW("Failed to read icc_req structure at 0x%lx", addr);
            continue;
        }
        // Create new request object and populate fields
        std::shared_ptr<icc_req> req_ptr = std::make_shared<icc_req>();
        req_ptr->addr = addr;
        req_ptr->tag = UINT(req_buf + field_offset(icc_req,tag));
        req_ptr->avg_bw = UINT(req_buf + field_offset(icc_req,avg_bw));
        req_ptr->peak_bw = UINT(req_buf + field_offset(icc_req,peak_bw));
        req_ptr->enabled = BOOL(req_buf + field_offset(icc_req,enabled));
        ulong dev_addr = ULONG(req_buf + field_offset(icc_req,dev));
        FREEBUF(req_buf);
        // Extract device name from device structure
        if (is_kvaddr(dev_addr)){
            ulong name_addr = read_pointer(dev_addr,"name addr");
            if (is_kvaddr(name_addr)){
                req_ptr->name = read_cstring(name_addr,64, "name");
            } else {
                req_ptr->name = "";
            }
        } else {
            req_ptr->name = "";
        }
        LOGD("icc_req details: %s tag=%u, avg_bw=%u KB/s, peak_bw=%u KB/s, enabled=%s",req_ptr->name.c_str(),
             req_ptr->tag, req_ptr->avg_bw, req_ptr->peak_bw,
             req_ptr->enabled ? "true" : "false");
        // Add request to node's request list
        node_ptr->req_list.push_back(req_ptr);
    }
}
#pragma GCC diagnostic pop
