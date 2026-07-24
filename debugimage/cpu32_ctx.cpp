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

#include "cpu32_ctx.h"
#include "debugimage.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

Cpu32_Context::Cpu32_Context(){

}

void Cpu32_Context::print_stack(std::shared_ptr<Dump_entry> entry_ptr){
    int core = entry_ptr->id - DATA_CPU_CTX;
    LOGD("Cpu32_Context::print_stack() for core %d", core);
    void* buf = read_memory(entry_ptr->data_addr,sizeof(tzbsp_dump_32_t),"tzbsp_dump_32_t",false);
    if (!buf) {
        LOGE("Failed to read memory for tzbsp_dump_32_t at %#lx", entry_ptr->data_addr);
        return;
    }
    tzbsp_dump_32_t reg_dump = *reinterpret_cast<tzbsp_dump_32_t*>(buf);

    unsigned long long lr = reg_dump.sc_regs.r14_svc;
    // unsigned long long cpsr = reg_dump.sc_regs.cpsr;
    // unsigned long long fp1 = 0;
    // if (cpsr & 0x20){
    //     fp1 = reg_dump.sc_regs.r[7];
    // }else{
    //     fp1 = reg_dump.sc_regs.r[11];
    // }
    unsigned long long pc = pac_ignore(reg_dump.sc_regs.pc);
    LOGD("Core %d: PC=%#llx, LR=%#llx", core, pc, lr);
    std::ostringstream oss;
    oss << "Core" << std::dec << core << " " << "\n";
    oss << "PC: " << "<" << std::hex << pc << ">: " << to_symbol(pc) << "\n";
    lr = pac_ignore(lr);
    oss << "LR: " << "<" << std::hex << lr << ">: " << to_symbol(lr) << "\n";
    PRINT("%s \n\n",oss.str().c_str());
    FREEBUF(buf);
}

void Cpu32_Context::generate_cmm(std::shared_ptr<Dump_entry> entry_ptr){
    int core = entry_ptr->id - DATA_CPU_CTX;
    // int major = entry_ptr->version >> 4;
    // int minor = entry_ptr->version & 0xF;
    // LOGI("%s  core:%d  version:%d.%d\n",entry_ptr->data_name.c_str(), core,major,minor);
    LOGI("Cpu32_Context::generate_cmm() for core %d", core);
    void* buf = read_memory(entry_ptr->data_addr,sizeof(tzbsp_dump_32_t),"tzbsp_dump_32_t",false);
    if (!buf) {
        LOGE("Failed to read memory for tzbsp_dump_32_t at %#lx", entry_ptr->data_addr);
        return;
    }
    tzbsp_dump_32_t reg_dump = *reinterpret_cast<tzbsp_dump_32_t*>(buf);
    std::string regs_file = get_cmm_path("core" + std::to_string(core), false);
    LOGD("Creating normal CMM file: %s", regs_file.c_str());
    FILE* cmmfile = fopen(regs_file.c_str(), "wb");
    if (!cmmfile) {
        LOGE("Failed to create CMM file: %s", regs_file.c_str());
        FREEBUF(buf);
        return;
    }
    std::ostringstream oss_regs;
    for (size_t i = 0; i < 13; i++){
        oss_regs << "r.s r"  << std::dec << i << " 0x" << std::hex << reg_dump.sc_regs.r[i] << std::endl;
    }
    oss_regs << "r.s r13_usr 0x"     << std::hex << reg_dump.sc_regs.r13_usr     << std::endl;
    oss_regs << "r.s r14_usr 0x"     << std::hex << reg_dump.sc_regs.r14_usr     << std::endl;
    oss_regs << "r.s r13_hyp 0x"     << std::hex << reg_dump.sc_regs.r13_hyp     << std::endl;
    oss_regs << "r.s r14_irq 0x"     << std::hex << reg_dump.sc_regs.r14_irq     << std::endl;
    oss_regs << "r.s r13_irq 0x"     << std::hex << reg_dump.sc_regs.r13_irq     << std::endl;
    oss_regs << "r.s r14_svc 0x"     << std::hex << reg_dump.sc_regs.r14_svc     << std::endl;
    oss_regs << "r.s r13_svc 0x"     << std::hex << reg_dump.sc_regs.r13_svc     << std::endl;
    oss_regs << "r.s r14_abt 0x"     << std::hex << reg_dump.sc_regs.r14_abt     << std::endl;
    oss_regs << "r.s r13_abt 0x"     << std::hex << reg_dump.sc_regs.r13_abt     << std::endl;
    oss_regs << "r.s r14_und 0x"     << std::hex << reg_dump.sc_regs.r14_und     << std::endl;
    oss_regs << "r.s r13_und 0x"     << std::hex << reg_dump.sc_regs.r13_und     << std::endl;
    oss_regs << "r.s r8_fiq 0x"      << std::hex << reg_dump.sc_regs.r8_fiq      << std::endl;
    oss_regs << "r.s r9_fiq 0x"      << std::hex << reg_dump.sc_regs.r9_fiq      << std::endl;
    oss_regs << "r.s r10_fiq 0x"     << std::hex << reg_dump.sc_regs.r10_fiq     << std::endl;
    oss_regs << "r.s r11_fiq 0x"     << std::hex << reg_dump.sc_regs.r11_fiq     << std::endl;
    oss_regs << "r.s r12_fiq 0x"     << std::hex << reg_dump.sc_regs.r12_fiq     << std::endl;
    oss_regs << "r.s r13_fiq 0x"     << std::hex << reg_dump.sc_regs.r13_fiq     << std::endl;
    oss_regs << "r.s r14_fiq 0x"     << std::hex << reg_dump.sc_regs.r14_fiq     << std::endl;
    oss_regs << "r.s pc 0x"          << std::hex << reg_dump.sc_regs.pc          << std::endl;
    oss_regs << "r.s cpsr 0x"        << std::hex << reg_dump.sc_regs.cpsr        << std::endl;
    oss_regs << "r.s r13_mon 0x"     << std::hex << reg_dump.sc_regs.r13_mon     << std::endl;
    oss_regs << "r.s r14_mon 0x"     << std::hex << reg_dump.sc_regs.r14_mon     << std::endl;
    oss_regs << "r.s r14_hyp 0x"     << std::hex << reg_dump.sc_regs.r14_hyp     << std::endl;
    fwrite(oss_regs.str().c_str(), sizeof(char), oss_regs.str().size(), cmmfile);
    fclose(cmmfile);
    std::string secure_file = get_cmm_path("core" + std::to_string(core), true);
    LOGD("Creating secure CMM file: %s", secure_file.c_str());
    cmmfile = fopen(secure_file.c_str(), "wb");
    if (!cmmfile) {
        LOGE("Failed to create secure CMM file: %s", secure_file.c_str());
        FREEBUF(buf);
        return;
    }
    std::ostringstream oss_secure;
    for (size_t i = 0; i < 13; i++){
        oss_secure << "r.s r"  << std::dec << i << " 0x" << std::hex << reg_dump.sc_secure.r[i] << std::endl;
    }
    oss_secure << "r.s r13_usr 0x"     << std::hex << reg_dump.sc_secure.r13_usr     << std::endl;
    oss_secure << "r.s r14_usr 0x"     << std::hex << reg_dump.sc_secure.r14_usr     << std::endl;
    oss_secure << "r.s r13_hyp 0x"     << std::hex << reg_dump.sc_secure.r13_hyp     << std::endl;
    oss_secure << "r.s r14_irq 0x"     << std::hex << reg_dump.sc_secure.r14_irq     << std::endl;
    oss_secure << "r.s r13_irq 0x"     << std::hex << reg_dump.sc_secure.r13_irq     << std::endl;
    oss_secure << "r.s r14_svc 0x"     << std::hex << reg_dump.sc_secure.r14_svc     << std::endl;
    oss_secure << "r.s r13_svc 0x"     << std::hex << reg_dump.sc_secure.r13_svc     << std::endl;
    oss_secure << "r.s r14_abt 0x"     << std::hex << reg_dump.sc_secure.r14_abt     << std::endl;
    oss_secure << "r.s r13_abt 0x"     << std::hex << reg_dump.sc_secure.r13_abt     << std::endl;
    oss_secure << "r.s r14_und 0x"     << std::hex << reg_dump.sc_secure.r14_und     << std::endl;
    oss_secure << "r.s r13_und 0x"     << std::hex << reg_dump.sc_secure.r13_und     << std::endl;
    oss_secure << "r.s r8_fiq 0x"      << std::hex << reg_dump.sc_secure.r8_fiq      << std::endl;
    oss_secure << "r.s r9_fiq 0x"      << std::hex << reg_dump.sc_secure.r9_fiq      << std::endl;
    oss_secure << "r.s r10_fiq 0x"     << std::hex << reg_dump.sc_secure.r10_fiq     << std::endl;
    oss_secure << "r.s r11_fiq 0x"     << std::hex << reg_dump.sc_secure.r11_fiq     << std::endl;
    oss_secure << "r.s r12_fiq 0x"     << std::hex << reg_dump.sc_secure.r12_fiq     << std::endl;
    oss_secure << "r.s r13_fiq 0x"     << std::hex << reg_dump.sc_secure.r13_fiq     << std::endl;
    oss_secure << "r.s r14_fiq 0x"     << std::hex << reg_dump.sc_secure.r14_fiq     << std::endl;
    oss_secure << "r.s pc 0x"          << std::hex << reg_dump.sc_secure.pc          << std::endl;
    oss_secure << "r.s cpsr 0x"        << std::hex << reg_dump.sc_secure.cpsr        << std::endl;
    oss_secure << "r.s r13_mon 0x"     << std::hex << reg_dump.sc_secure.r13_mon     << std::endl;
    oss_secure << "r.s r14_mon 0x"     << std::hex << reg_dump.sc_secure.r14_mon     << std::endl;
    oss_secure << "r.s r14_hyp 0x"     << std::hex << reg_dump.sc_secure.r14_hyp     << std::endl;
    fwrite(oss_secure.str().c_str(), sizeof(char), oss_secure.str().size(), cmmfile);
    fclose(cmmfile);
    PRINT("Saved CMM files: %s and %s\n", regs_file.c_str(), secure_file.c_str());
    FREEBUF(buf);
}

#pragma GCC diagnostic pop
