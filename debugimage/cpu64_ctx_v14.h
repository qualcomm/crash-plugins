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

#ifndef CPU_64_CTX_V14_DEFS_H_
#define CPU_64_CTX_V14_DEFS_H_

#include "image_parser.h"

typedef struct{
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
    uint64_t cpu_state0;
    uint64_t cpu_state1;
    uint64_t cpu_state3;
    uint64_t cpu_state4;
    uint64_t cpu_state5;
    uint64_t __reserved_1;
    uint64_t __reserved_2;
    uint64_t __reserved_3;
}sysdbg_cpu64_ctx_1_4_t;

typedef struct {
    uint64_t v0_lower;
    uint64_t v0_upper;
    uint64_t v1_lower;
    uint64_t v1_upper;
    uint64_t v2_lower;
    uint64_t v2_upper;
    uint64_t v3_lower;
    uint64_t v3_upper;
    uint64_t v4_lower;
    uint64_t v4_upper;
    uint64_t v5_lower;
    uint64_t v5_upper;
    uint64_t v6_lower;
    uint64_t v6_upper;
    uint64_t v7_lower;
    uint64_t v7_upper;
    uint64_t v8_lower;
    uint64_t v8_upper;
    uint64_t v9_lower;
    uint64_t v9_upper;
    uint64_t v10_lower;
    uint64_t v10_upper;
    uint64_t v11_lower;
    uint64_t v11_upper;
    uint64_t v12_lower;
    uint64_t v12_upper;
    uint64_t v13_lower;
    uint64_t v13_upper;
    uint64_t v14_lower;
    uint64_t v14_upper;
    uint64_t v15_lower;
    uint64_t v15_upper;
    uint64_t v16_lower;
    uint64_t v16_upper;
    uint64_t v17_lower;
    uint64_t v17_upper;
    uint64_t v18_lower;
    uint64_t v18_upper;
    uint64_t v19_lower;
    uint64_t v19_upper;
    uint64_t v20_lower;
    uint64_t v20_upper;
    uint64_t v21_lower;
    uint64_t v21_upper;
    uint64_t v22_lower;
    uint64_t v22_upper;
    uint64_t v23_lower;
    uint64_t v23_upper;
    uint64_t v24_lower;
    uint64_t v24_upper;
    uint64_t v25_lower;
    uint64_t v25_upper;
    uint64_t v26_lower;
    uint64_t v26_upper;
    uint64_t v27_lower;
    uint64_t v27_upper;
    uint64_t v28_lower;
    uint64_t v28_upper;
    uint64_t v29_lower;
    uint64_t v29_upper;
    uint64_t v30_lower;
    uint64_t v30_upper;
    uint64_t v31_lower;
    uint64_t v31_upper;
} sysdbg_neon128_registers_t;

typedef struct {
    unsigned int status[4];
    sysdbg_cpu64_ctx_1_4_t sc_regs;
    sysdbg_cpu64_ctx_1_4_t sc_secure;
    unsigned int neon_status[4];
    sysdbg_neon128_registers_t neon_reg;
}tzbsp_dump_64_1_4_t;

class Cpu64_Context_V14 : public ImageParser {
private:
    void compute_pc(sysdbg_cpu64_ctx_1_4_t reg, sysdbg_neon128_registers_t neon_reg);

public:
    Cpu64_Context_V14();
    void generate_cmm(std::shared_ptr<Dump_entry> entry_ptr) override;
    void print_stack(std::shared_ptr<Dump_entry> entry_ptr) override;
};

#endif // CPU_64_CTX_V14_DEFS_H_
