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

#ifndef CPU_32_CTX_DEFS_H_
#define CPU_32_CTX_DEFS_H_

#include "image_parser.h"

typedef struct {
    unsigned long long r[13]; //from r0 to r12
    unsigned long long r13_usr;
    unsigned long long r14_usr;
    unsigned long long r13_hyp;
    unsigned long long r14_irq;
    unsigned long long r13_irq;
    unsigned long long r14_svc;
    unsigned long long r13_svc;
    unsigned long long r14_abt;
    unsigned long long r13_abt;
    unsigned long long r14_und;
    unsigned long long r13_und;
    unsigned long long r8_fiq;
    unsigned long long r9_fiq;
    unsigned long long r10_fiq;
    unsigned long long r11_fiq;
    unsigned long long r12_fiq;
    unsigned long long r13_fiq;
    unsigned long long r14_fiq;
    unsigned long long pc;
    unsigned long long cpsr;
    unsigned long long r13_mon;
    unsigned long long r14_mon;
    unsigned long long r14_hyp;
    unsigned long long _reserved;
    unsigned long long __reserved_1;
    unsigned long long __reserved_2;
    unsigned long long __reserved_3;
    unsigned long long __reserved_4;
}sysdbg_cpu32_ctx_t;

typedef struct {
    unsigned int status[4];
    sysdbg_cpu32_ctx_t sc_regs;
    sysdbg_cpu32_ctx_t sc_secure;
}tzbsp_dump_32_t;

class Cpu32_Context : public ImageParser {
public:
    Cpu32_Context();
    void generate_cmm(std::shared_ptr<Dump_entry> entry_ptr) override;
    void print_stack(std::shared_ptr<Dump_entry> entry_ptr) override;
};

#endif // CPU_32_CTX_DEFS_H_
