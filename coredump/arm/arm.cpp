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

#include "arm.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

Arm::Arm(std::shared_ptr<Swapinfo> swap) : Core(swap){
    elf_class = ELFCLASS32;
    field_init(task_struct,thread_info);
    field_init(thread_info,vfpstate);
    field_init(thread_info,fpstate);
    field_init(vfp_state,hard);
    field_init(vfp_hard_struct,fpregs);
    field_init(vfp_hard_struct,fpscr);
}

Arm::~Arm(){

}

/*
see arm_regsets in arch/arm/kernel/ptrace.c
for arm, we only have the below regsets.
NT_PRSTATUS
NT_PRFPREG
NT_ARM_VFP
*/
void* Arm::parser_nt_arm_vfp(ulong task_addr) {
    size_t data_len = sizeof(struct user_vfp);
    struct user_vfp* vfp = (struct user_vfp*)std::malloc(data_len);
    BZERO(vfp, data_len);
    ulong fpregs_addr = task_addr + field_offset(task_struct, thread_info)
            + field_offset(thread_info, vfpstate)
            + field_offset(vfp_state, hard)
            + field_offset(vfp_hard_struct, fpregs);
    if(!read_struct(fpregs_addr, &vfp->fpregs, field_size(vfp_hard_struct, fpregs),"parser_nt_arm_vfp fpregs")){
        LOGE("get fpregs failed \n");
        std::free(vfp);
        return nullptr;
    }
    ulong fpscr_addr = task_addr + field_offset(task_struct, thread_info)
            + field_offset(thread_info, vfpstate)
            + field_offset(vfp_state, hard)
            + field_offset(vfp_hard_struct, fpscr);
    vfp->fpscr = read_ulong(fpscr_addr,"parser_nt_arm_vfp fpscr");
    LOGD("NT_ARM_VFP:\n");
    LOGD("\n%s", hexdump(0x1000,(char*)vfp, data_len).c_str());
    return vfp;
}

void* Arm::parser_nt_prfpreg(ulong task_addr) {
    size_t data_len = sizeof(struct user_fp);
    struct user_fp* ufp = (struct user_fp*)std::malloc(data_len);
    BZERO(ufp, data_len);
    ulong fpstate_addr = task_addr + field_offset(task_struct, thread_info) + field_offset(thread_info, fpstate);
    if(!read_struct(fpstate_addr, ufp, sizeof(*ufp),"parser_nt_prfpreg fpstate")){
        LOGE("get fpstate failed \n");
        std::free(ufp);
        return nullptr;
    }
    LOGD("NT_PRFPREG:\n");
    LOGD("\n%s", hexdump(0x1000,(char*)ufp, data_len).c_str());
    return ufp;
}

void* Arm::parser_nt_arm_tls(ulong task_addr) {
    return nullptr;
}

void* Arm::parser_nt_arm_hw_break(ulong task_addr) {
    return nullptr;
}

void* Arm::parser_nt_arm_hw_watch(ulong task_addr) {
    return nullptr;
}

void* Arm::parser_nt_arm_system_call(ulong task_addr) {
    return nullptr;
}

void* Arm::parser_nt_arm_sve(ulong task_addr) {
    return nullptr;
}

void* Arm::parser_nt_arm_pac_mask(ulong task_addr) {
    return nullptr;
}

void* Arm::parser_nt_arm_pac_enabled_keys(ulong task_addr) {
    return nullptr;
}

void* Arm::parser_nt_arm_paca_keys(ulong task_addr) {
    return nullptr;
}

void* Arm::parser_nt_arm_pacg_keys(ulong task_addr) {
    return nullptr;
}

void* Arm::parser_nt_arm_tagged_addr_ctrl(ulong task_addr) {
    return nullptr;
}

void* Arm::parser_prstatus(ulong task_addr,int* data_size) {
    size_t data_len = sizeof(struct elf32_prstatus);
    struct elf32_prstatus* prstatus = (struct elf32_prstatus*)std::malloc(data_len);
    BZERO(prstatus, data_len);
    prstatus->pr_info.si_signo = prstatus->pr_cursig = 6;
    ulong real_parent_addr = read_pointer(task_addr + field_offset(task_struct, real_parent), "real_parent");
    prstatus->pr_ppid = read_int(real_parent_addr + field_offset(task_struct, pid), "real_parent pid");
    prstatus->pr_pid = task_pid_nr_ns(task_addr, read_enum_val("PIDTYPE_PID"));
    prstatus->pr_pgrp = task_pid_nr_ns(task_addr, read_enum_val("PIDTYPE_PGID"));
    prstatus->pr_sid = task_pid_nr_ns(task_addr, read_enum_val("PIDTYPE_SID"));
    prstatus->pr_fpvalid = 1;
    // #define task_pt_regs(p) ((struct pt_regs *)(THREAD_SIZE - 8 + task->stack) - 1)
    ulong pt_regs_addr = GET_STACKTOP(task_addr) - 8 - sizeof(struct arm32_pt_regs);
    if(!read_struct(pt_regs_addr, &prstatus->pr_reg, sizeof(struct arm32_pt_regs),"gpr64_get user_pt_regs")){
        LOGE("get pt_regs failed \n");
        std::free(prstatus);
        return nullptr;
    }
    *data_size = data_len;
    LOGD("pid: %d, task_addr:%#lx, pt_regs_addr:%#lx, struct_size:%#zx stack_top:%#lx \n", prstatus->pr_pid, task_addr, pt_regs_addr, sizeof(struct arm32_pt_regs), GET_STACKTOP(task_addr));
    LOGD("NT_PRSTATUS:\n");
    LOGD("\n%s", hexdump(0x1000, (char*)prstatus, data_len).c_str());
    return prstatus;
}

void Arm::parser_prpsinfo() {
    size_t data_size = sizeof(struct elf_prpsinfo);
    struct elf_prpsinfo* prpsinfo = (struct elf_prpsinfo*)std::malloc(data_size);
    BZERO(prpsinfo, data_size);
    ulong state = 0;
    if (field_offset(task_struct, state) != -1){
        state = read_ulong(tc->task + field_offset(task_struct, state), "task_struct state");
    } else {
        state = read_int(tc->task + field_offset(task_struct, __state), "task_struct __state");
    }
    std::string args = task_ptr->read_start_args();
    copy_and_fill_char(prpsinfo->pr_psargs, args.c_str(), args.size());
    ulong real_parent_addr = read_pointer(tc->task + field_offset(task_struct, real_parent), "task_struct real_parent");
    prpsinfo->pr_ppid = read_int(real_parent_addr + field_offset(task_struct, pid), "task_struct real_parent pid");
    prpsinfo->pr_pid = task_pid_nr_ns(tc->task, read_enum_val("PIDTYPE_PID"));
    prpsinfo->pr_pgrp = task_pid_nr_ns(tc->task, read_enum_val("PIDTYPE_PGID"));
    prpsinfo->pr_sid = task_pid_nr_ns(tc->task, read_enum_val("PIDTYPE_SID"));
    uint i = state ? ffs(state) + 1 : 0;
    prpsinfo->pr_state = i;
    prpsinfo->pr_sname = (i > 5) ? '.' : "RSDTZW"[i];
    prpsinfo->pr_zomb = prpsinfo->pr_sname == 'Z';
    int static_prio = read_int(tc->task + field_offset(task_struct, static_prio), "task_struct static_prio");
    prpsinfo->pr_nice = static_prio - 120;
    prpsinfo->pr_flag = read_ulong(tc->task + field_offset(task_struct, flags), "task_struct flags");
    ulong cred_addr = read_pointer(tc->task + field_offset(task_struct, cred), "task_struct cred");
    prpsinfo->pr_uid = read_uint(cred_addr + field_offset(cred, uid), "cred uid");
    prpsinfo->pr_gid = read_uint(cred_addr + field_offset(cred, gid), "cred gid");
    copy_and_fill_char(prpsinfo->pr_fname, tc->comm, strlen(tc->comm));
    LOGD("NT_PRPSINFO:\n");
    LOGD("\n%s", hexdump(0x1000, (char*)prpsinfo, data_size).c_str());
    psinfo = std::make_shared<memelfnote>();
    psinfo->name = "CORE";
    psinfo->type = NT_PRPSINFO;
    psinfo->data = prpsinfo;
    psinfo->datasz = data_size;
}

void Arm::parser_siginfo() {
    size_t data_size = sizeof(struct elf_siginfo);
    struct elf_siginfo* sinfo = (struct elf_siginfo*)std::malloc(data_size);
    BZERO(sinfo, data_size);
    sinfo->si_signo = 6;
    LOGD("NT_SIGINFO:\n");
    LOGD("\n%s", hexdump(0x1000,(char*)sinfo,data_size).c_str());
    signote = std::make_shared<memelfnote>();
    signote->name = "CORE";
    signote->type = NT_SIGINFO;
    signote->data = sinfo;
    signote->datasz = data_size;
}

#pragma GCC diagnostic pop
