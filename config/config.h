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

#ifndef CONFIG_DEFS_H_
#define CONFIG_DEFS_H_

#include "plugin.h"
#include <vector>
#include <string>
#include <map>

/**
 * Struct representing a single kernel configuration option.
 */
struct KernelConfig {
    std::string name;           // Config name, e.g. "CONFIG_EXT4_FS"
    std::string value;          // Config value: "y", "m", "n", or string
    std::string subsystem;      // Subsystem prefix, e.g. "EXT4", "FS", "NET"
};

class Config : public ParserPlugin {
private:
    std::vector<KernelConfig> collect_all_configs();
    std::vector<KernelConfig> filter_by_subsystem(const std::vector<KernelConfig>& all,
                                                   const std::string& subsystem);
    void print_all_configs();
    void print_single_config(const std::string& key);
    void print_subsystem_filter(const std::string& subsystem);

public:
    Config();
    void cmd_main(void) override;
    void init_offset(void) override;
    void init_command(void) override;
    DEFINE_PLUGIN_INSTANCE(Config)
};

#endif // CONFIG_DEFS_H_
