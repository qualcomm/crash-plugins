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

#ifndef CLOCK_DEFS_H_
#define CLOCK_DEFS_H_

#include "plugin.h"

/**
 * @brief Structure representing a clock provider in the kernel
 *
 * Contains information about a clock provider including its address,
 * name, and list of associated clock cores.
 */
struct clk_provider {
    ulong addr;                          // Address of the clock provider structure
    std::string name;                    // Name of the clock provider (from device tree)
    std::vector<ulong> core_list;        // List of clock core addresses managed by this provider
};

/**
 * @brief Statistics structure for clock analysis
 *
 * Aggregated statistics about clock usage across all providers.
 */
struct ClockStatistics {
    size_t total_providers = 0;          // Total number of clock providers
    size_t total_clocks = 0;             // Total number of clock cores
    size_t enabled_clocks = 0;           // Number of enabled clocks
    size_t prepared_clocks = 0;          // Number of prepared clocks
    double average_frequency = 0.0;      // Average frequency across all clocks (MHz)
};

/**
 * @brief Clock plugin class for analyzing Common Clock Framework (CCF)
 *
 * This plugin provides comprehensive analysis of the Linux Common Clock Framework,
 * including clock providers, clock trees, enable/disable states, and usage statistics.
 * It supports various Qualcomm-specific clock implementations including QCOM CC,
 * RPM SMD, RPMH, and other clock provider types.
 */
class Clock : public ParserPlugin {
private:
    std::vector<std::shared_ptr<clk_provider>> provider_list;  // List of all clock providers

    // Clock provider parsing functions
    void parser_clk_simple(std::shared_ptr<clk_provider> prov_ptr, ulong data);
    void parser_clk_hw_simple(std::shared_ptr<clk_provider> prov_ptr, ulong data);
    void parser_clk_rpmh(std::shared_ptr<clk_provider> prov_ptr, ulong data);
    void parser_clk_of_msm_provider(std::shared_ptr<clk_provider> prov_ptr, ulong data);
    void parser_clk_onecell(std::shared_ptr<clk_provider> prov_ptr, ulong data);
    void parser_clk_spmi_pmic(std::shared_ptr<clk_provider> prov_ptr, ulong data);
    void parser_rpm_smd_clk(std::shared_ptr<clk_provider> prov_ptr, ulong data);
    void parser_clk_qcom_cc(std::shared_ptr<clk_provider> prov_ptr, ulong data);
    void parser_clk_virtio(std::shared_ptr<clk_provider> prov_ptr, ulong data);

    // Clock core analysis functions
    void parser_clk_core(ulong addr);

    // Display functions
    void print_enable_clock();
    void print_disable_clock();
    void print_prepare_clock();
    void print_clk_providers();
    void print_clk_tree();

    // Main parsing function
    void parser_clk_providers();

    // Helper functions
    ClockStatistics calculate_clock_statistics();
    std::string format_frequency(ulong frequency_hz);
    bool validate_clk_core_address(ulong addr);

public:
    /**
     * @brief Default constructor
     */
    Clock();

    /**
     * @brief Main command entry point
     */
    void cmd_main(void) override;

    /**
     * @brief Initialize kernel structure field offsets
     */
    void init_offset(void) override;

    /**
     * @brief Initialize command help and usage information
     */
    void init_command(void) override;

    DEFINE_PLUGIN_INSTANCE(Clock)
};

#endif // CLOCK_DEFS_H_
