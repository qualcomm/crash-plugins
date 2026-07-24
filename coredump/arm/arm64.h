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

#ifndef ARM64_DEFS_H_
#define ARM64_DEFS_H_

#include "coredump/core.h"

#define BIT(nr)         (1UL << (nr))

#define SCTLR_ELx_ENIA (BIT(31))
#define SCTLR_ELx_ENIB (BIT(30))
#define SCTLR_ELx_ENDA (BIT(27))
#define SCTLR_ELx_ENDB (BIT(13))

#define PR_PAC_APIAKEY (1UL << 0)
#define PR_PAC_APIBKEY (1UL << 1)
#define PR_PAC_APDAKEY (1UL << 2)
#define PR_PAC_APDBKEY (1UL << 3)
#define PR_PAC_APGAKEY (1UL << 4)

#define TIF_TAGGED_ADDR 26
#define PR_TAGGED_ADDR_ENABLE (1UL << 0)
#define MTE_CTRL_GCR_USER_EXCL_SHIFT 0
#define SYS_GCR_EL1_EXCL_MASK 0xffffUL

#define MTE_CTRL_TCF_SYNC (1UL << 16)
#define MTE_CTRL_TCF_ASYNC (1UL << 17)

#define PR_MTE_TAG_SHIFT 3
#define PR_MTE_TCF_SYNC (1UL << 1)
#define PR_MTE_TCF_ASYNC (1UL << 2)

struct user_pac_mask {
    uint64_t data_mask;
    uint64_t insn_mask;
};

struct __kernel_old_timeval {
    long tv_sec;
    long tv_usec;
};

struct elf64_prstatus {
    struct elf_siginfo pr_info;
    short pr_cursig;
    unsigned long pr_sigpend;
    unsigned long pr_sighold;
    pid_t pr_pid;
    pid_t pr_ppid;
    pid_t pr_pgrp;
    pid_t pr_sid;
    long pr_utime[2]; // __kernel_old_timeval
    long pr_stime[2];
    long pr_cutime[2];
    long pr_cstime[2];
    struct user_pt_regs pr_reg;
    int pr_fpvalid;
};

struct ptrauth_key {
    unsigned long lo;
    unsigned long hi;
};

struct ptrauth_keys_user {
    struct ptrauth_key apia;
    struct ptrauth_key apib;
    struct ptrauth_key apda;
    struct ptrauth_key apdb;
    struct ptrauth_key apga;
};

struct user_pac_address_keys {
    // __uint128_t
    __ull apiakey;
    __ull apibkey;
    __ull apdakey;
    __ull apdbkey;
};

struct user_pac_generic_keys {
    // __uint128_t
    __ull apgakey;
};

class Arm64 : public Core {
private:
    void pac_generic_keys_to_user(user_pac_generic_keys *ukeys, const ptrauth_keys_user *keys);
    void pac_address_keys_to_user(user_pac_address_keys *ukeys, const ptrauth_keys_user *keys);

public:
    Arm64(std::shared_ptr<Swapinfo> swap);
    ~Arm64();
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

#endif // ARM64_DEFS_H_
