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

#ifndef COMPAT_DEFS_H_
#define COMPAT_DEFS_H_

#include "coredump/core.h"

#define PSR_AA32_DIT_BIT 0x01000000
#define COMPAT_PSR_DIT_BIT 0x00200000

typedef int compat_int_t;
typedef unsigned int compat_ulong_t;
typedef int compat_pid_t;
typedef int old_time32_t;
typedef unsigned short __compat_uid_t;
typedef unsigned short __compat_gid_t;

struct compat_elf_siginfo
{
    compat_int_t si_signo;
    compat_int_t si_code;
    compat_int_t si_errno;
};

struct old_timeval32 {
    old_time32_t    tv_sec;
    int             tv_usec;
};

struct compat_elf_prstatus_common
{
    struct compat_elf_siginfo    pr_info;
    short                        pr_cursig;
    compat_ulong_t               pr_sigpend;
    compat_ulong_t               pr_sighold;
    compat_pid_t                 pr_pid;
    compat_pid_t                 pr_ppid;
    compat_pid_t                 pr_pgrp;
    compat_pid_t                 pr_sid;
    struct old_timeval32         pr_utime;
    struct old_timeval32         pr_stime;
    struct old_timeval32         pr_cutime;
    struct old_timeval32         pr_cstime;
};

struct compat_elf_prpsinfo
{
    char                pr_state;
    char                pr_sname;
    char                pr_zomb;
    char                pr_nice;
    compat_ulong_t      pr_flag;
    __compat_uid_t      pr_uid;
    __compat_gid_t      pr_gid;
    compat_pid_t        pr_pid, pr_ppid, pr_pgrp, pr_sid;
    char                pr_fname[16];
    char                pr_psargs[80];
};

// compat regs
struct compat_pt_reg{
    unsigned int regs[18];
};

struct compat_elf_prstatus
{
    struct compat_elf_prstatus_common   common;
    compat_pt_reg                       pr_reg;
    compat_int_t                        pr_fpvalid;
};

class Compat : public Core {
public:
    Compat(std::shared_ptr<Swapinfo> swap);
    ~Compat();
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

#endif // COMPAT_DEFS_H_
