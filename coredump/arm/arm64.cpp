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

#include "arm64.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

Arm64::Arm64(std::shared_ptr<Swapinfo> swap) : Core(swap){
    elf_class = ELFCLASS64;
    field_init(task_struct, thread);
    field_init(thread_struct, sctlr_user);
    field_init(thread_struct, mte_ctrl);
    field_init(thread_struct, uw);
    field_init(thread_struct, tpidr2_el0);
    field_init(pt_regs, syscallno);
}

Arm64::~Arm64(){

}

/*
see aarch64_regsets in arch/arm64/kernel/ptrace.c
for arm64, we only have the below regsets.
NT_PRSTATUS
NT_PRFPREG
NT_ARM_TLS
NT_ARM_HW_BREAK
NT_ARM_HW_WATCH
NT_ARM_SYSTEM_CALL
NT_ARM_SVE
NT_ARM_PAC_MASK
NT_ARM_PAC_ENABLED_KEYS
NT_ARM_PACA_KEYS
NT_ARM_PACG_KEYS
NT_ARM_TAGGED_ADDR_CTRL
*/
void* Arm64::parser_nt_arm_vfp(ulong task_addr) {
    return nullptr;
}

void* Arm64::parser_nt_prfpreg(ulong task_addr) {
    size_t data_len = sizeof(struct user_fpsimd_state);
    struct user_fpsimd_state* uregs = (struct user_fpsimd_state*)std::malloc(data_len);
    BZERO(uregs, data_len);
    ulong fpsimd_state_addr = task_addr + field_offset(task_struct, thread) + field_offset(thread_struct, uw) + sizeof(unsigned long) /*tp_value*/ + sizeof(unsigned long) /*tp2_value*/;
    if(!read_struct(fpsimd_state_addr, uregs, sizeof(struct user_fpsimd_state),"parser_nt_prfpreg uregs")){
        LOGE("Failed to read fpsimd_state at %#lx", fpsimd_state_addr);
        std::free(uregs);
        return nullptr;
    }
    LOGD("NT_PRFPREG: task_addr:%#lx \n", task_addr);
    LOGD("\n%s", hexdump(0x1000, (char*)uregs, data_len).c_str());
    return uregs;
}

void* Arm64::parser_nt_arm_tls(ulong task_addr) {
    tls_t *tls = (tls_t *)std::malloc(sizeof(tls_t));
    BZERO(tls, sizeof(tls_t));
    ulong tp_value_addr = task_addr + field_offset(task_struct, thread) + field_offset(thread_struct, uw);
    ulong tpidr2_el0_addr = task_addr + field_offset(task_struct, thread) + field_offset(thread_struct, tpidr2_el0);

    tls->tp_value = read_ulong(tp_value_addr, "uw tp_value");
    tls->tpidr2_el0 = read_ulonglong(tpidr2_el0_addr, "tpidr2_el0");
    LOGD("NT_PRFPREG: task_addr:%#lx \n", task_addr);
    LOGD("\n%s", hexdump(0x1000,(char*)tls, sizeof(tls_t)).c_str());
    return tls;
}

void* Arm64::parser_nt_arm_hw_break(ulong task_addr) {
    return nullptr;
}

void* Arm64::parser_nt_arm_hw_watch(ulong task_addr) {
    return nullptr;
}

void* Arm64::parser_nt_arm_system_call(ulong task_addr) {
    size_t data_len = sizeof(int);
    char *sys_call = (char*)std::malloc(data_len);
    BZERO(sys_call, data_len);

    ulong pt_regs_addr = GET_STACKTOP(task_addr) - machdep->machspec->user_eframe_offset;
    ulong syscallno_addr = pt_regs_addr + field_offset(pt_regs, syscallno);
    int syscallno = read_int(syscallno_addr, "syscallno");
    memcpy(sys_call, &syscallno, data_len);

    LOGD("NT_ARM_SYSTEM_CALL: task_addr:%#lx \n", task_addr);
    LOGD("\n%s", hexdump(0x1000, sys_call, sizeof(int)).c_str());
    return sys_call;
}

void* Arm64::parser_nt_arm_sve(ulong task_addr) {
    return nullptr;
}

void* Arm64::parser_nt_arm_pac_mask(ulong task_addr) {
    size_t data_len = sizeof(struct user_pac_mask);
    struct user_pac_mask* uregs = (struct user_pac_mask*)std::malloc(data_len);
    BZERO(uregs, data_len);
    // #define ptrauth_user_pac_mask()        GENMASK_ULL(54, vabits_actual)
    ulong vabits_actual = machdep->machspec->VA_BITS_ACTUAL;
    uint64_t mask = GENMASK_ULL(54, vabits_actual); // default
    uregs->data_mask = mask;
    uregs->insn_mask = mask;
    LOGD("NT_ARM_PAC_MASK:\n");
    LOGD("\n%s", hexdump(0x1000, (char*)uregs, data_len).c_str());
    return uregs;
}

void* Arm64::parser_nt_arm_pac_enabled_keys(ulong task_addr) {
    size_t data_len = sizeof(uint64_t);
    uint64_t sctlr_user = read_ulonglong(task_addr + field_offset(task_struct, thread) + field_offset(thread_struct, sctlr_user), "task_struct thread_struct sctlr_user");
    uint64_t* retval = (uint64_t*)std::malloc(data_len);
    *retval = 0;
    if (sctlr_user & SCTLR_ELx_ENIA)
        *retval |= PR_PAC_APIAKEY;
    if (sctlr_user & SCTLR_ELx_ENIB)
        *retval |= PR_PAC_APIBKEY;
    if (sctlr_user & SCTLR_ELx_ENDA)
        *retval |= PR_PAC_APDAKEY;
    if (sctlr_user & SCTLR_ELx_ENDB)
        *retval |= PR_PAC_APDBKEY;
    LOGD("NT_ARM_PAC_ENABLED_KEYS:\n");
    LOGD("\n%s", hexdump(0x1000, (char*)retval, data_len).c_str());
    return retval;
}

void Arm64::pac_address_keys_to_user(struct user_pac_address_keys *ukeys,
                                     const struct ptrauth_keys_user *keys){
    ukeys->apiakey[0] = keys->apia.lo;
    ukeys->apiakey[1] = keys->apia.hi;

    ukeys->apibkey[0] = keys->apib.lo;
    ukeys->apibkey[1] = keys->apib.hi;

    ukeys->apdakey[0] = keys->apda.lo;
    ukeys->apdakey[1] = keys->apda.hi;

    ukeys->apdbkey[0] = keys->apdb.lo;
    ukeys->apdbkey[1] = keys->apdb.hi;
}

void* Arm64::parser_nt_arm_paca_keys(ulong task_addr) {
    size_t data_len = sizeof(struct user_pac_address_keys);
    struct user_pac_address_keys* user_keys = (struct user_pac_address_keys*)std::malloc(data_len);
    BZERO(user_keys, data_len);

    struct ptrauth_keys_user keys;
    ulong keys_user_addr = task_addr + field_offset(task_struct, thread) + field_offset(thread_struct, keys_user);
    if(!read_struct(keys_user_addr, &keys, sizeof(struct ptrauth_keys_user),"parser_nt_arm_paca_keys keys_user")){
        LOGE("get user_pac_address_keys failed \n");
        std::free(user_keys);
        return nullptr;
    }
    pac_address_keys_to_user(user_keys, &keys);
    LOGD("NT_ARM_PACA_KEYS:\n");
    LOGD("\n%s", hexdump(0x1000, (char*)user_keys, data_len).c_str());
    return user_keys;
}

void Arm64::pac_generic_keys_to_user(struct user_pac_generic_keys *ukeys,
                                     const struct ptrauth_keys_user *keys) {
    ukeys->apgakey[0] = keys->apga.lo;
    ukeys->apgakey[1] = keys->apga.hi;
}

void* Arm64::parser_nt_arm_pacg_keys(ulong task_addr) {
    size_t data_len = sizeof(struct user_pac_generic_keys);
    struct user_pac_generic_keys* user_keys = (struct user_pac_generic_keys*)std::malloc(data_len);
    BZERO(user_keys, data_len);

    struct ptrauth_keys_user keys;
    ulong keys_user_addr = task_addr + field_offset(task_struct, thread) + field_offset(thread_struct, keys_user);
    if(!read_struct(keys_user_addr, &keys, sizeof(struct ptrauth_keys_user),"parser_nt_arm_paca_keys keys_user")){
        LOGE("get user_pac_generic_keys failed \n");
        std::free(user_keys);
        return nullptr;
    }
    pac_generic_keys_to_user(user_keys, &keys);
    LOGD("NT_ARM_PACG_KEYS:\n");
    LOGD("\n%s", hexdump(0x1000, (char*)user_keys, data_len).c_str());
    return user_keys;
}

void* Arm64::parser_nt_arm_tagged_addr_ctrl(ulong task_addr) {
    uint64_t ret = 0;
    // ulong thread_info_flags = read_ulong(task_addr + field_offset(task_struct, thread_info) + field_offset(thread_info, flags), "arm64 task_struct thread_info flags");

    if (thread_info_flags & (1 << TIF_TAGGED_ADDR)){
        ret = PR_TAGGED_ADDR_ENABLE;
    }
    if (field_offset(thread_struct, mte_ctrl) != -1) {
        uint64_t mte_ctrl = read_ulonglong(task_addr + field_offset(task_struct, thread) + field_offset(thread_struct, mte_ctrl), "task_struct thread_struct mte_ctrl");
        uint64_t incl = (~mte_ctrl >> MTE_CTRL_GCR_USER_EXCL_SHIFT) & SYS_GCR_EL1_EXCL_MASK;
        ret |= incl << PR_MTE_TAG_SHIFT;
        if (mte_ctrl & MTE_CTRL_TCF_ASYNC)
            ret |= PR_MTE_TCF_ASYNC;
        if (mte_ctrl & MTE_CTRL_TCF_SYNC)
            ret |= PR_MTE_TCF_SYNC;
    }
    size_t data_len = sizeof(uint64_t);
    uint64_t* retval = (uint64_t*)std::malloc(data_len);
    *retval = ret;
    LOGD("NT_ARM_TAGGED_ADDR_CTRL:\n");
    LOGD("\n%s", hexdump(0x1000, (char*)retval, data_len).c_str());
    return retval;
}

void* Arm64::parser_prstatus(ulong task_addr,int* data_size) {
    size_t data_len = sizeof(struct elf64_prstatus);
    struct elf64_prstatus* prstatus = (struct elf64_prstatus*)std::malloc(data_len);
    BZERO(prstatus, data_len);
    prstatus->pr_info.si_signo = prstatus->pr_cursig = 6;
    ulong real_parent_addr = read_pointer(task_addr + field_offset(task_struct, real_parent), "real_parent");
    prstatus->pr_ppid = read_int(real_parent_addr + field_offset(task_struct, pid), "real_parent pid");
    prstatus->pr_pid = task_pid_nr_ns(task_addr, read_enum_val("PIDTYPE_PID"));
    prstatus->pr_pgrp = task_pid_nr_ns(task_addr, read_enum_val("PIDTYPE_PGID"));
    prstatus->pr_sid = task_pid_nr_ns(task_addr, read_enum_val("PIDTYPE_SID"));
    prstatus->pr_fpvalid = 1;
    // #define task_pt_regs(p) ((struct pt_regs *)(UL(1) << THREAD_SHIFT + task->stack) - 1)
    // task.stack -> task.stack + 16K = (stack_top) -> stack_top - sizeof(struct pt_regs)
    ulong pt_regs_addr = GET_STACKTOP(task_addr) - machdep->machspec->user_eframe_offset;
    if(!read_struct(pt_regs_addr, &prstatus->pr_reg, sizeof(struct user_pt_regs),"gpr64_get user_pt_regs")){
        LOGE("get user_pt_regs failed \n");
        std::free(prstatus);
        return nullptr;
    }
    *data_size = data_len;
    LOGD("pid: %d, task_addr:%#lx, pt_regs_addr:%#lx, struct_size:%#zx stack_top:%#lx user_eframe_offset:%#lx \n", prstatus->pr_pid, task_addr, pt_regs_addr, sizeof(struct pt_regs), GET_STACKTOP(task_addr), machdep->machspec->user_eframe_offset);
    LOGD("NT_PRSTATUS:\n");
    LOGD("\n%s", hexdump(0x1000,(char*)prstatus,data_len).c_str());
    return prstatus;
}

void Arm64::parser_prpsinfo() {
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
    LOGD("\n%s", hexdump(0x1000,(char*)prpsinfo,data_size).c_str());
    psinfo = std::make_shared<memelfnote>();
    psinfo->name = "CORE";
    psinfo->type = NT_PRPSINFO;
    psinfo->data = prpsinfo;
    psinfo->datasz = data_size;
}

void Arm64::parser_siginfo() {
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
