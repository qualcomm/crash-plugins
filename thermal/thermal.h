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

/**
 * @file thermal.h
 * @brief Thermal management plugin for analyzing kernel thermal zones and cooling devices
 *
 * This plugin provides functionality to extract and display thermal zone information,
 * including temperature readings, trip points, cooling devices, and thermal governors.
 * It parses kernel thermal subsystem structures to provide insights into thermal
 * management state and configuration.
 */

#ifndef THERMAL_DEFS_H_
#define THERMAL_DEFS_H_

#include "plugin.h"

/**
 * @brief Cooling device information structure
 *
 * Represents a thermal cooling device that can be used to reduce heat,
 * such as CPU frequency scaling, fan control, or device throttling.
 */
struct cool_dev {
    ulong addr;         ///< Kernel virtual address of thermal_cooling_device structure
    int id;             ///< Unique cooling device identifier
    std::string name;   ///< Cooling device name/type (e.g., "cpu-cluster0", "fan")
};

/**
 * @brief Thermal trip point structure
 *
 * Represents a temperature threshold that triggers cooling actions.
 * Each trip point has an associated temperature and list of cooling devices.
 */
struct trip {
    ulong addr;                                      ///< Kernel virtual address of thermal_trip structure
    int temp;                                        ///< Trip point temperature in millidegrees Celsius
    std::vector<std::shared_ptr<cool_dev>> cool_list; ///< List of cooling devices for this trip point
};

/**
 * @brief Thermal zone device structure
 *
 * Represents a thermal zone that monitors temperature and manages cooling.
 * Contains current/last temperature, trip points, and associated cooling devices.
 */
struct zone_dev {
    ulong addr;                                    ///< Kernel virtual address of thermal_zone_device structure
    int id;                                        ///< Unique thermal zone identifier
    std::string name;                              ///< Thermal zone name (e.g., "cpu-thermal", "gpu-thermal")
    std::string governor;                          ///< Thermal governor name (e.g., "step_wise", "power_allocator")
    std::vector<std::shared_ptr<trip>> trip_list;  ///< List of trip points for this zone
    int cur_temp;                                  ///< Current temperature in millidegrees Celsius
    int last_temp;                                 ///< Last recorded temperature in millidegrees Celsius
};

/**
 * @brief Thermal management plugin class
 *
 * Provides commands to display thermal zone information, cooling devices,
 * and detailed trip point configurations from kernel crash dumps.
 */
class Thermal : public ParserPlugin {
private:
    std::vector<std::shared_ptr<zone_dev>> zone_list; ///< List of all thermal zones

    /**
     * @brief Print all thermal zone devices with summary information
     */
    void print_zone_device();

    /**
     * @brief Print detailed information for a specific thermal zone
     * @param dev_name Name of the thermal zone to display
     */
    void print_zone_device(std::string dev_name);

    /**
     * @brief Print all cooling devices in the system
     */
    void print_cooling_device();

    /**
     * @brief Parse thermal zone structures from kernel memory
     */
    void parser_thrermal_zone();

public:
    /**
     * @brief Constructor for Thermal plugin
     */
    Thermal();

    /**
     * @brief Initialize kernel structure field offsets
     */
    void init_offset(void) override;

    /**
     * @brief Initialize command help and usage information
     */
    void init_command(void) override;

    /**
     * @brief Main command entry point
     */
    void cmd_main(void) override;

    DEFINE_PLUGIN_INSTANCE(Thermal)
};

#endif // THERMAL_DEFS_H_
