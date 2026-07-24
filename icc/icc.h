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

#ifndef ICC_DEFS_H_
#define ICC_DEFS_H_

#include "plugin.h"

/**
 * @brief ICC (Interconnect) request information structure
 *
 * This structure represents an interconnect bandwidth request from a device
 * or driver. Each request specifies bandwidth requirements and is associated
 * with a specific interconnect node.
 */
struct icc_req {
    ulong addr;                 // Address of icc_req structure in kernel memory
    bool enabled;               // Whether this request is currently enabled
    uint32_t tag;               // Request tag for identification and grouping
    uint32_t avg_bw;            // Average bandwidth requirement (KB/s)
    uint32_t peak_bw;           // Peak bandwidth requirement (KB/s)
    std::string name;           // Device name that made this request
};

/**
 * @brief ICC (Interconnect) node information structure
 *
 * This structure represents an interconnect node in the system topology.
 * Nodes are connection points in the interconnect fabric that can aggregate
 * bandwidth requests from multiple consumers and forward them to providers.
 */
struct icc_node {
    ulong addr;                                     // Address of icc_node structure
    int id;                                         // Unique node identifier
    uint32_t avg_bw;                                // Current aggregated average bandwidth
    uint32_t peak_bw;                               // Current aggregated peak bandwidth
    ulong data;                                     // Provider-specific data pointer
    std::string name;                               // Node name (e.g., "apps_proc", "snoc_bimc")
    std::vector<std::shared_ptr<icc_req>> req_list; // List of bandwidth requests for this node
};

/**
 * @brief ICC (Interconnect) provider information structure
 *
 * This structure represents an interconnect provider that manages a set of
 * interconnect nodes. Providers are typically associated with specific
 * hardware blocks like NoC (Network on Chip) or bus controllers.
 */
struct icc_provider {
    ulong addr;                                       // Address of icc_provider structure
    int users;                                        // Number of active users/consumers
    std::string name;                                 // Provider name (e.g., "soc:interconnect")
    std::vector<std::shared_ptr<icc_node>> node_list; // List of nodes managed by this provider
};

/**
 * @brief ICC (Interconnect) framework analyzer and parser class
 *
 * This class provides functionality to parse and analyze the Linux interconnect
 * framework from crash dumps. The interconnect framework manages bandwidth
 * allocation and Quality of Service (QoS) for system interconnects including:
 * - NoC (Network on Chip) fabrics
 * - System buses and memory controllers
 * - Bandwidth arbitration and throttling
 * - Power and performance optimization
 *
 * The class supports:
 * - Listing all interconnect providers and their topology
 * - Displaying bandwidth requests and aggregation
 * - Analyzing interconnect node relationships
 * - Showing QoS settings and bandwidth utilization
 */
class ICC : public ParserPlugin {
private:
    // List of all discovered interconnect providers
    std::vector<std::shared_ptr<icc_provider>> provider_list;

    /**
     * @brief Display bandwidth requests for a specific interconnect node
     * @param node_name Name of the interconnect node to display requests for
     *
     * Shows detailed information about all bandwidth requests targeting
     * the specified node, including request sources, bandwidth values,
     * and enable states in a formatted table.
     */
    void print_icc_request(std::string node_name);

    /**
     * @brief Display all nodes managed by a specific interconnect provider
     * @param provider_name Name of the interconnect provider
     *
     * Shows a tabular list of all interconnect nodes managed by the
     * specified provider, including node IDs, current bandwidth
     * aggregation, and provider-specific data.
     */
    void print_icc_nodes(std::string provider_name);

    /**
     * @brief Display comprehensive interconnect topology and bandwidth information
     *
     * Shows a hierarchical view of all interconnect providers, their nodes,
     * and associated bandwidth requests. This provides a complete overview
     * of the system's interconnect topology and current bandwidth allocation.
     */
    void print_icc_info();

    /**
     * @brief Display summary information for all interconnect providers
     *
     * Shows a tabular summary of all interconnect providers with their
     * user counts and basic identification information.
     */
    void print_icc_provider();

    /**
     * @brief Parse all interconnect providers from kernel interconnect framework
     *
     * Traverses the kernel's global interconnect provider list to discover
     * all registered providers and their associated nodes and requests.
     */
    void parser_icc_provider();

    /**
     * @brief Parse interconnect nodes for a specific provider
     * @param provider_ptr Shared pointer to the provider structure
     * @param head Address of the node list head
     *
     * Iterates through the node list of a provider and extracts detailed
     * information about each node including bandwidth aggregation and
     * associated requests.
     */
    void parser_icc_node(std::shared_ptr<icc_provider> provider_ptr, ulong head);

    /**
     * @brief Parse bandwidth requests for a specific interconnect node
     * @param node_ptr Shared pointer to the node structure
     * @param head Address of the request list head
     *
     * Iterates through the request list of a node and extracts detailed
     * information about each bandwidth request including source device
     * and bandwidth requirements.
     */
    void parser_icc_req(std::shared_ptr<icc_node> node_ptr, ulong head);

public:
    /**
     * @brief Default constructor
     *
     * Initializes the ICC parser with default settings.
     */
    ICC();

    /**
     * @brief Main command entry point
     *
     * Processes command-line arguments and dispatches to appropriate
     * handlers. Supports options for:
     * -a           : Display comprehensive interconnect information
     * -p           : Display interconnect provider summary
     * -n <name>    : Display nodes for specific provider
     * -r <name>    : Display requests for specific node
     */
    void cmd_main(void) override;

    /**
     * @brief Initialize kernel structure field offsets
     *
     * Sets up field offsets for interconnect framework structures including
     * icc_provider, icc_node, and icc_req structures.
     */
    void init_offset(void) override;

    /**
     * @brief Initialize command metadata
     *
     * Sets up command name, description, usage information, and examples
     * for the interconnect analysis command.
     */
    void init_command(void) override;

    // Plugin instance definition macro
    DEFINE_PLUGIN_INSTANCE(ICC)
};

#endif // ICC_DEFS_H_
