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

#include "compat.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

Compat::Compat(std::shared_ptr<Swapinfo> swap) : Core(swap){
    field_init(task_struct, thread);
    field_init(thread_struct, uw);
    elf_class = ELFCLASS32;
}

Compat::~Compat(){

}

/*
see aarch32_regsets in arch/arm64/kernel/ptrace.c
for compat, we only have the below regsets.
NT_PRSTATUS
NT_ARM_VFP
*/
void* Compat::parser_nt_arm_vfp(ulong task_addr) {
    size_t data_len = sizeof(struct user_fpsimd_state) + sizeof(compat_ulong_t);
    compat_ulong_t fpscr;
    char* buffer = (char *)std::malloc(data_len);
    BZERO(buffer, data_len);

    struct user_fpsimd_state* uregs = (struct user_fpsimd_state*)buffer;
    ulong fpsimd_state_addr = task_addr + field_offset(task_struct, thread) + field_offset(thread_struct, uw) + sizeof(unsigned long) /*tp_value*/ + sizeof(unsigned long) /*tp2_value*/;
    if(!read_struct(fpsimd_state_addr, uregs, sizeof(struct user_fpsimd_state),"parser_nt_prfpreg uregs")){
        LOGE( "fpr_get failed \n");
        std::free(buffer);
        return nullptr;
    }
    fpscr = (uregs->fpsr & VFP_FPSCR_STAT_MASK) | (uregs->fpcr & VFP_FPSCR_CTRL_MASK);
    void* fpscr_ptr = (char*)buffer + sizeof(struct user_fpsimd_state);
    memcpy(fpscr_ptr, &fpscr, sizeof(compat_ulong_t));
    LOGD("NT_ARM_VFP (compat): fpscr=%#x (fpsr=%#x, fpcr=%#x)",
         fpscr, uregs->fpsr, uregs->fpcr);
    LOGD( "NT_ARM_VFP: task_addr%#lx: \n", task_addr);
    LOGD("\n%s", hexdump(0x1000, buffer, data_len).c_str());
    return buffer;
}

void* Compat::parser_nt_prfpreg(ulong task_addr) {
    return nullptr;
}

void* Compat::parser_nt_arm_tls(ulong task_addr) {
    return nullptr;
}

void* Compat::parser_nt_arm_hw_break(ulong task_addr) {
    return nullptr;
}

void* Compat::parser_nt_arm_hw_watch(ulong task_addr) {
    return nullptr;
}

void* Compat::parser_nt_arm_system_call(ulong task_addr) {
    return nullptr;
}

void* Compat::parser_nt_arm_sve(ulong task_addr) {
    return nullptr;
}

// check user_aarch32_view
void* Compat::parser_nt_arm_pac_mask(ulong task_addr) {
    return nullptr;
}

void* Compat::parser_nt_arm_pac_enabled_keys(ulong task_addr) {
    return nullptr;
}

void* Compat::parser_nt_arm_paca_keys(ulong task_addr) {
    return nullptr;
}

void* Compat::parser_nt_arm_pacg_keys(ulong task_addr) {
    return nullptr;
}

void* Compat::parser_nt_arm_tagged_addr_ctrl(ulong task_addr) {
    return nullptr;
}

void* Compat::parser_prstatus(ulong task_addr,int* data_size) {
    size_t data_len = sizeof(struct compat_elf_prstatus);
    struct compat_elf_prstatus* prstatus = (struct compat_elf_prstatus*)std::malloc(data_len);
    BZERO(prstatus, data_len);
    prstatus->common.pr_info.si_signo = prstatus->common.pr_cursig = 6;
    ulong real_parent_addr = read_pointer(task_addr + field_offset(task_struct, real_parent), "real_parent");
    prstatus->common.pr_ppid = read_int(real_parent_addr + field_offset(task_struct, pid), "real_parent pid");
    prstatus->common.pr_pid = task_pid_nr_ns(task_addr, read_enum_val("PIDTYPE_PID"));
    prstatus->common.pr_pgrp = task_pid_nr_ns(task_addr, read_enum_val("PIDTYPE_PGID"));
    prstatus->common.pr_sid = task_pid_nr_ns(task_addr, read_enum_val("PIDTYPE_SID"));
    prstatus->pr_fpvalid = 1;
    // #define task_pt_regs(p) ((struct pt_regs *)(UL(1) << THREAD_SHIFT + task->stack) - 1)
    // task.stack -> task.stack + 16K = (stack_top) -> stack_top - sizeof(struct pt_regs)
    ulong pt_regs_addr = GET_STACKTOP(task_addr) - machdep->machspec->user_eframe_offset;
    struct pt_regs pt_regs_t;
    BZERO(&pt_regs_t, sizeof(struct pt_regs));
    if(!read_struct(pt_regs_addr, &pt_regs_t, sizeof(struct pt_regs), "compat_get_user_reg")){
        LOGE( "get compat_get_user_reg failed \n");
        std::free(prstatus);
        return nullptr;
    }
    // compat_get_user_reg() in ptrace.c  uint <-> ulong
    for(int i = 0; i < 18; i++){
        if(i == 15){
            prstatus->pr_reg.regs[i] = pt_regs_t.pc;
        } else if(i == 16){
            prstatus->pr_reg.regs[i] = (pt_regs_t.pstate & ~PSR_AA32_DIT_BIT) | ((pt_regs_t.pstate & PSR_AA32_DIT_BIT) ? COMPAT_PSR_DIT_BIT : 0);
        } else if(i == 17){
            prstatus->pr_reg.regs[i] = pt_regs_t.orig_x0; // orig_x0
        } else {
            prstatus->pr_reg.regs[i] = pt_regs_t.regs[i];
        }
    }
    *data_size = data_len;
    LOGD( "pid: %d, task_addr:%#lx, pt_regs_addr:%#lx, struct_size:%#zx stack_top:%#lx user_eframe_offset:%#lx \n",prstatus->common.pr_pid, task_addr, pt_regs_addr, sizeof(struct pt_regs), GET_STACKTOP(task_addr), machdep->machspec->user_eframe_offset);
    LOGD( "NT_PRSTATUS:\n");
    LOGD("\n%s", hexdump(0x1000,(char*)prstatus, data_len).c_str());
    return prstatus;
}

void Compat::parser_prpsinfo() {
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
    prpsinfo->pr_flag = read_uint(tc->task + field_offset(task_struct, flags), "task_struct flags");
    ulong cred_addr = read_pointer(tc->task + field_offset(task_struct, cred), "task_struct cred");
    prpsinfo->pr_uid = read_ushort(cred_addr + field_offset(cred, uid), "cred uid");
    prpsinfo->pr_gid = read_ushort(cred_addr + field_offset(cred, gid), "cred gid");
    copy_and_fill_char(prpsinfo->pr_fname, tc->comm, strlen(tc->comm));
    LOGD( "NT_PRPSINFO:\n");
    LOGD("\n%s", hexdump(0x1000,(char*)prpsinfo,data_size).c_str());
    psinfo = std::make_shared<memelfnote>();
    psinfo->name = "CORE";
    psinfo->type = NT_PRPSINFO;
    psinfo->data = prpsinfo;
    psinfo->datasz = data_size;
}

void Compat::parser_siginfo() {
    size_t data_size = sizeof(struct elf_siginfo);
    struct elf_siginfo* sinfo = (struct elf_siginfo*)std::malloc(data_size);
    BZERO(sinfo, data_size);
    sinfo->si_signo = 6;
    LOGD( "NT_SIGINFO:\n");
    LOGD("\n%s", hexdump(0x1000,(char*)sinfo,data_size).c_str());
    signote = std::make_shared<memelfnote>();
    signote->name = "CORE";
    signote->type = NT_SIGINFO;
    signote->data = sinfo;
    signote->datasz = data_size;
}

#pragma GCC diagnostic pop
