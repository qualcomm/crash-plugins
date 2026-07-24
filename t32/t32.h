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

#ifndef T32_DEFS_H_
#define T32_DEFS_H_

#include "plugin.h"
#include <random>

class TCR_EL1 : public ParserPlugin {
private:
    // TCR_EL1 default values
    static constexpr uint64_t DEFAULT_VALUE_NON_CORTEX = 0x00000032B5193519ULL;
    static constexpr uint64_t DEFAULT_VALUE_CORTEX = 0x00000012B5193519ULL;
    uint64_t value;
    void set_field(uint8_t msb, uint8_t lsb, uint64_t field_value);
    uint8_t get_tg1_value(uint64_t page_size);
    uint8_t get_tg0_value(uint64_t page_size);

public:
    explicit TCR_EL1(bool is_cortexa);
    uint64_t get_value() const { return value; }

    void cmd_main() override {}
    void init_offset() override {}
    void init_command() override {}
};


class T32 : public ParserPlugin {
private:
    const std::string t32_launch_bat = "launch_t32.bat";
    const std::string t32_launch_config = "t32_config.t32";
    const std::string t32_launch_cmm = "t32_startup_script.cmm";
    std::string windows_path;
    std::vector<std::string> cpu_types = {"CORTEXA53", "CORTEXA7", "ARMv8.2-A", "ARMV9-A"};

    void parser_t32(std::string& win_path, std::string& cpu_type);
    void parser_t32_launch_config();
    void parser_t32_launch_cmm(std::string& cpu_type);
    void parser_t32_launch_bat();
    std::string create_t32_path(const std::string& filename);
    std::ostringstream extract_load_binary(const std::string& file_path);

public:
    T32();
    void init_offset(void) override;
    void init_command(void) override;
    void cmd_main(void) override;
    DEFINE_PLUGIN_INSTANCE(T32)
};

#endif // T32_DEFS_H_
