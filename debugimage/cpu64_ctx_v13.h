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

#ifndef CPU_64_CTX_V13_DEFS_H_
#define CPU_64_CTX_V13_DEFS_H_

#include "image_parser.h"

typedef struct {
    uint64_t x[31];
    uint64_t pc;
    uint64_t currentEL;
    uint64_t sp_el3;
    uint64_t elr_el3;
    uint64_t spsr_el3;
    uint64_t sp_el2;
    uint64_t elr_el2;
    uint64_t spsr_el2;
    uint64_t sp_el1;
    uint64_t elr_el1;
    uint64_t spsr_el1;
    uint64_t sp_el0;
    uint64_t cpumerrsr_el1;
    uint64_t l2merrsr_el1;
    uint64_t __reserved_1;
    uint64_t __reserved_2;
}sysdbg_cpu64_ctx_1_3_t;

typedef struct {
    unsigned int status[4];
    sysdbg_cpu64_ctx_1_3_t sc_regs;
    sysdbg_cpu64_ctx_1_3_t sc_secure;
}tzbsp_dump_64_1_3_t;

class Cpu64_Context_V13 : public ImageParser {
public:
    Cpu64_Context_V13();
    void generate_cmm(std::shared_ptr<Dump_entry> entry_ptr) override;
    void print_stack(std::shared_ptr<Dump_entry> entry_ptr) override;
};

#endif // CPU_64_CTX_V13_DEFS_H_
