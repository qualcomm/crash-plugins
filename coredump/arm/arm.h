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

#ifndef ARM_DEFS_H_
#define ARM_DEFS_H_

#include "coredump/core.h"

struct user_fp {
    struct fp_reg {
        unsigned int sign1:1;
        unsigned int unused:15;
        unsigned int sign2:1;
        unsigned int exponent:14;
        unsigned int j:1;
        unsigned int mantissa1:31;
        unsigned int mantissa0:32;
    } fpregs[8];
    unsigned int fpsr:32;
    unsigned int fpcr:32;
    unsigned char ftype[8];
    unsigned int init_flag;
};

struct user_vfp {
    unsigned long long fpregs[32];
    unsigned long fpscr;
};

struct arm32_pt_regs{
    unsigned long r0;
    unsigned long r1;
    unsigned long r2;
    unsigned long r3;
    unsigned long r4;
    unsigned long r5;
    unsigned long r6;
    unsigned long r7;
    unsigned long r8;
    unsigned long r9;
    unsigned long r10;
    unsigned long fp;
    unsigned long ip;
    unsigned long sp;
    unsigned long lr;
    unsigned long pc;
    unsigned long cpsr;
    unsigned long ORIG_r0;
};

struct elf32_prstatus {
    struct elf_siginfo pr_info;
    short pr_cursig;
    unsigned long pr_sigpend;
    unsigned long pr_sighold;
    pid_t pr_pid;
    pid_t pr_ppid;
    pid_t pr_pgrp;
    pid_t pr_sid;
    struct timeval pr_utime;
    struct timeval pr_stime;
    struct timeval pr_cutime;
    struct timeval pr_cstime;
    struct arm32_pt_regs pr_reg;
    int pr_fpvalid;
};

class Arm : public Core {
public:
    Arm(std::shared_ptr<Swapinfo> swap);
    ~Arm();
    void parser_prpsinfo() override;
    void parser_siginfo() override;
    void* parser_prstatus(ulong task_addr,int* data_size) override;
    void* parser_nt_prfpreg(ulong task_addr) override;
    void* parser_nt_arm_tls(ulong task_addr) override;
    void* parser_nt_arm_hw_break(ulong task_addr) override;
    void* parser_nt_arm_hw_watch(ulong task_addr) override;
    void* parser_nt_arm_system_call(ulong task_addr) override;
    void* parser_nt_arm_sve(ulong task_addr) override;
    void* parser_nt_arm_pac_mask(ulong task_addr) override;
    void* parser_nt_arm_pac_enabled_keys(ulong task_addr) override;
    void* parser_nt_arm_paca_keys(ulong task_addr) override;
    void* parser_nt_arm_pacg_keys(ulong task_addr) override;
    void* parser_nt_arm_tagged_addr_ctrl(ulong task_addr) override;
    void* parser_nt_arm_vfp(ulong task_addr) override;
};

#endif // ARM_DEFS_H_
