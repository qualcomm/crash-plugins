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

#ifndef REGULATOR_DEFS_H_
#define REGULATOR_DEFS_H_

#include "plugin.h"

/**
 * @brief Voltage constraint information structure
 *
 * This structure represents voltage constraints for a regulator consumer,
 * including minimum and maximum voltage requirements in microvolts (uV).
 */
struct voltage {
    int min_uV;     // Minimum voltage in microvolts
    int max_uV;     // Maximum voltage in microvolts
};

/**
 * @brief Regulator consumer information structure
 *
 * This structure contains information about a regulator consumer (device
 * that uses the regulator), including its power requirements, enable state,
 * and voltage constraints.
 */
struct regulator {
    ulong addr;                                     // Address of regulator structure
    std::string name;                               // Consumer name (supply name)
    int load;                                       // Current load in microamps (uA)
    int enable_count;                               // Enable reference count
    std::vector<std::shared_ptr<voltage>> voltages; // Voltage constraints list
};

/**
 * @brief Regulator device information structure
 *
 * This structure represents a regulator device in the Linux regulator framework.
 * Each regulator device can have multiple consumers and maintains state information
 * including usage counts, voltage constraints, and consumer lists.
 */
struct regulator_dev {
    ulong addr;                                     // Address of regulator_dev structure
    ulong desc;                                     // Address of regulator_desc structure
    ulong constraint;                               // Address of regulation_constraints structure
    uint32_t open_count;                            // Number of open handles
    uint32_t use_count;                             // Usage reference count
    uint32_t bypass_count;                          // Bypass mode reference count
    std::string name;                               // Regulator name
    int min_uV;                                     // Minimum voltage constraint (uV)
    int max_uV;                                     // Maximum voltage constraint (uV)
    int input_uV;                                   // Input voltage (uV)
    std::vector<std::shared_ptr<regulator>> consumers; // List of consumers
};

/**
 * @brief Regulator framework analyzer and parser class
 *
 * This class provides functionality to parse and analyze the Linux regulator
 * framework from crash dumps. The regulator framework manages power supplies
 * and voltage regulators in the system, including:
 * - PMIC (Power Management IC) regulators
 * - GDSC (Global Distributed Switch Controller) power domains
 * - LDO (Low Dropout) regulators
 * - Buck converters and switching regulators
 *
 * The class supports:
 * - Listing all regulator devices with their status
 * - Displaying consumer information for specific regulators
 * - Showing voltage constraints and power consumption
 * - Analyzing regulator usage and enable states
 */
class Regulator : public ParserPlugin {
private:
    // List of all discovered regulator devices
    std::vector<std::shared_ptr<regulator_dev>> regulator_list;

    /**
     * @brief Parse device list (unused helper function)
     * @return Vector of device addresses
     *
     * Helper function for parsing device lists. Currently unused
     * but kept for potential future extensions.
     */
    std::vector<ulong> parser_device_list();

    /**
     * @brief Display consumer information for a specific regulator
     * @param reg_name Name of the regulator to display consumers for
     *
     * Shows detailed information about all consumers of the specified
     * regulator, including their load requirements, enable states,
     * and voltage constraints.
     */
    void print_regulator_consumer(std::string reg_name);

    /**
     * @brief Display comprehensive regulator and consumer information
     *
     * Shows detailed information for all regulator devices including
     * their consumers, with hierarchical formatting to show the
     * relationship between regulators and their consumers.
     */
    void print_regulator_info();

    /**
     * @brief Display regulator device summary information
     *
     * Shows a tabular summary of all regulator devices with their
     * key statistics including usage counts, voltage constraints,
     * and device addresses.
     */
    void print_regulator_dev();

    /**
     * @brief Parse all regulator devices from kernel regulator class
     *
     * Traverses the kernel's regulator device class to discover all
     * regulator devices and their associated consumers. Extracts
     * detailed information including constraints, usage statistics,
     * and consumer relationships.
     */
    void parser_regulator_dev();

public:
    /**
     * @brief Default constructor
     *
     * Initializes the regulator parser with default settings.
     */
    Regulator();

    /**
     * @brief Main command entry point
     *
     * Processes command-line arguments and dispatches to appropriate
     * handlers. Supports options for:
     * -a         : Display all regulators with consumer information
     * -r         : Display regulator device summary table
     * -c <name>  : Display consumers for specific regulator
     */
    void cmd_main(void) override;

    /**
     * @brief Initialize kernel structure field offsets
     *
     * Sets up field offsets for regulator framework structures including
     * regulator_dev, regulation_constraints, regulator_desc, and regulator.
     */
    void init_offset(void) override;

    /**
     * @brief Initialize command metadata
     *
     * Sets up command name, description, usage information, and examples
     * for the regulator analysis command.
     */
    void init_command(void) override;

    // Plugin instance definition macro
    DEFINE_PLUGIN_INSTANCE(Regulator)
};

#endif // REGULATOR_DEFS_H_
