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

#ifndef CPU_64_CTX_V20_DEFS_H_
#define CPU_64_CTX_V20_DEFS_H_

#include "image_parser.h"
#include "debugimage.h"
#include <map>

// sysdbg_cpu64_gprs_register_names_v2_0
typedef struct{
    uint64_t x[31];
    uint64_t pc;
    uint64_t sp_el0;
    uint64_t pstate;
}sysdbg_cpu64_ctx_2_0_gprs_t;

typedef struct {
    uint64_t cpacr_el1;
    uint64_t csselr_el1;
    uint64_t elr_el1;
    uint64_t esr_el1;
    uint64_t far_el1;
    uint64_t isr_el1;
    uint64_t par_el1;
    uint64_t mair_el1;
    uint64_t sctlr_el1;
    uint64_t sp_el1;
    uint64_t spsr_el1;
    uint64_t tcr_el1;
    uint64_t tpidr_el1;
    uint64_t ttbr0_el1;
    uint64_t ttbr1_el1;
    uint64_t vbar_el1;
} sysdbg_cpu64_ctx_2_0_el1_t;

typedef struct {
    uint64_t tpidr_el0;
    uint64_t tpidrro_el0;
} sysdbg_cpu64_ctx_2_0_el0_t;

typedef struct {
    uint64_t cptr_el2;
    uint64_t hcr_el2;
    uint64_t mdcr_el2;
    uint64_t vtcr_el2;
    uint64_t vttbr_el2;
} sysdbg_cpu64_vm_el2_ctx_2_0_t;

typedef struct {
    uint64_t cntkctl_el1;
    uint64_t cntv_ctl_el0;
    uint64_t cntv_cval_el0;
    uint64_t cntv_tval_el0;
} sysdbg_cpu64_cntv_el10_ctx_2_0_t;

typedef struct {
    uint64_t cntp_ctl_el0;
    uint64_t cntp_cval_el0;
    uint64_t cntp_tval_el0;
} sysdbg_cpu64_cntp_el10_ctx_2_0_t;

typedef struct {
    uint64_t cnthctl_el2;
    uint64_t cnthp_ctl_el2;
    uint64_t cnthp_cval_el2;
    uint64_t cnthp_tval_el2;
} sysdbg_cpu64_cnt_el2_ctx_2_0_t;

typedef struct {
    sysdbg_cpu64_cnt_el2_ctx_2_0_t cnt_regs;
    sysdbg_cpu64_cntp_el10_ctx_2_0_t cntp_regs;
    sysdbg_cpu64_cntv_el10_ctx_2_0_t cntv_regs;
    sysdbg_cpu64_vm_el2_ctx_2_0_t vm_regs;
    sysdbg_cpu64_ctx_2_0_el0_t ctx_el0_regs;
    sysdbg_cpu64_ctx_2_0_el1_t ctx_el1_regs;
    sysdbg_cpu64_ctx_2_0_gprs_t ctx_gprs_regs;
    sysdbg_neon128_registers_t neon_reg;
} tzbsp_dump_64_2_0_t;

typedef struct {
    uint64_t start_addr;
    uint64_t end_addr;
    uint32_t id;
    std::string name;
    std::shared_ptr<tzbsp_dump_64_2_0_t> dump_ptr;
} regset_t;

class Cpu64_Context_V20 : public ImageParser {
private:
    int32_t cpu_type_offset = 0;
    int32_t ctx_type_offset = 0;
    int32_t cpu_id_offset = 0;
    int32_t cpu_index_offset = 0;
    int32_t machine_id_offset = 0;
    int32_t registers_offset = 0;
    int32_t regset_num_register_offset = 0;
    int32_t regset_id_offset = 0;
    int32_t regset_addr_offset = 0;
    int32_t registers_size = 0;
    int32_t regset_size = 0;
    std::map<int, std::string> dump_regset_ids;

    void compute_pc(sysdbg_cpu64_ctx_2_0_gprs_t &reg, sysdbg_cpu64_ctx_2_0_el1_t &ctx_el1_reg);

public:
    Cpu64_Context_V20();
    void generate_cmm(std::shared_ptr<Dump_entry> entry_ptr) override;
    void print_stack(std::shared_ptr<Dump_entry> entry_ptr) override;
    uint32_t get_vcpu_index(uint32_t affinity) override;
};

#endif // CPU_64_CTX_V20_DEFS_H_
