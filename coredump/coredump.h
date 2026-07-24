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

#ifndef COREDUMP_DEFS_H_
#define COREDUMP_DEFS_H_

#include "plugin.h"
#include "core.h"

#include "arm/arm64.h"
#include "arm/compat.h"
#include "arm/arm.h"

class Coredump : public ParserPlugin {
private:
    static const int PRINT_LINKMAP = 0x0001;
    static const int PRINT_PROCMAP = 0x0002;
    static const int PRINT_COREDUMP = 0x0004;
    std::shared_ptr<Core> core_parser;
    std::shared_ptr<Swapinfo> swap_ptr;
    bool is_compat = false;

    void print_linkmap(int pid);
    void generate_coredump(int pid);
    bool get_core_parser(int pid);
    void print_proc_mapping(int pid);

public:
    Coredump();
    Coredump(std::shared_ptr<Swapinfo> swap);
    void cmd_main(void) override;
    void init_offset(void) override;
    void init_command(void) override;
    DEFINE_PLUGIN_INSTANCE(Coredump)
};

#endif // COREDUMP_DEFS_H_
