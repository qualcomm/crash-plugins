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

#ifndef SYS_DEFS_H_
#define SYS_DEFS_H_

#include "plugin.h"

#define SOCINFO_MAJOR(ver) (((ver) >> 16) & 0xffff)
#define SOCINFO_MINOR(ver) ((ver) & 0xffff)
#define SOCINFO_VERSION(maj, min)  ((((maj) & 0xffff) << 16)|((min) & 0xffff))

class SysInfo : public ParserPlugin {
private:
    uint32_t socinfo_format;
    std::vector<std::string> pmic_models;
    std::vector<std::string> hw_platforms;
    std::unordered_map<uint32_t, std::string> soc_ids;

    void print_socinfo();
    void print_qsocinfo();
    void print_command_line();
    void print_sysctl_values();
    void print_soc_info();
    void print_soc_info5();
    void read_pmic_models();
    void read_hw_platforms();
    void read_soc_ids();
    uint32_t socinfo_get_raw_id(void *buf);

public:
    SysInfo();
    void init_offset(void) override;
    void init_command(void) override;
    void cmd_main(void) override;
    DEFINE_PLUGIN_INSTANCE(SysInfo)
};

#endif // SYS_DEFS_H_
