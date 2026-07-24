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

#ifndef DTS_DEFS_H_
#define DTS_DEFS_H_

#include "plugin.h"
#include "devicetree/devicetree.h"

class Dts : public Devicetree {
private:
    static const int DTS_SHOW = 0x0001;
    static const int DTS_ADDR = 0x0002;
    static const int DTS_PATH = 0x0004;
    static const int DTB_MAX_SIZE = 0x100000;

    void print_ddr_info();
    void read_dtb(std::string& path);
    void print_node(std::shared_ptr<device_node> node_ptr,int level,int flag);
    void print_node(std::shared_ptr<device_node> node_ptr,int flag);
    void print_properties(std::vector<std::shared_ptr<Property>> props,int level,bool is_symbol,int flag);

public:
    Dts();
    void cmd_main(void) override;
    void init_offset(void) override;
    void init_command(void) override;
    DEFINE_PLUGIN_INSTANCE(Dts)
};


#endif // DTS_DEFS_H_
