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

#ifndef IMAGE_PARSER_DEFS_H_
#define IMAGE_PARSER_DEFS_H_

#include "plugin.h"

struct Dump_entry;

class ImageParser : public ParserPlugin {
protected:
    uint64_t createMask(int a, int b);
    uint64_t pac_ignore(uint64_t data);
    std::string get_cmm_path(std::string name, bool secure);

public:
    ImageParser();
    void cmd_main(void) override;
    void init_offset(void) override;
    void init_command(void) override;
    virtual void generate_cmm(std::shared_ptr<Dump_entry> entry_ptr)=0;
    virtual void print_stack(std::shared_ptr<Dump_entry> entry_ptr)=0;
    virtual uint32_t get_vcpu_index(uint32_t affinity);
};

#endif // IMAGE_PARSER_DEFS_H_
