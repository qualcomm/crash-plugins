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

#ifndef CORE_DEFS_H_
#define CORE_DEFS_H_

#include "plugin.h"
#include <elf.h>
#include <linux/types.h>
#include "memory/swapinfo.h"
#include "../utils/utask.h"
#include <sys/stat.h>
#include <exception>

#define MMF_DUMP_ANON_PRIVATE       2
#define MMF_DUMP_ANON_SHARED        3
#define MMF_DUMP_MAPPED_PRIVATE     4
#define MMF_DUMP_MAPPED_SHARED      5
#define MMF_DUMP_ELF_HEADERS        6
#define MMF_DUMP_HUGETLB_PRIVATE    7
#define MMF_DUMP_HUGETLB_SHARED     8
#define MMF_DUMP_DAX_PRIVATE        9
#define MMF_DUMP_DAX_SHARED         10

#ifndef NT_PRFPREG
#define NT_PRFPREG              0x2
#endif
#define NT_ARM_PAC_ENABLED_KEYS 0x40a
#define NT_ARM_PAC_MASK         0x406
#define NT_ARM_PACA_KEYS        0x407
#define NT_ARM_PACG_KEYS        0x408
#define NT_ARM_TAGGED_ADDR_CTRL 0x409

#define VFP_FPSCR_STAT_MASK 0xf800009f
#define VFP_FPSCR_CTRL_MASK 0x07f79f00

#define FILTER_SPECIAL_VMA          (1 << 0)
#define FILTER_FILE_VMA             (1 << 1)
#define FILTER_SHARED_VMA           (1 << 2)
#define FILTER_SANITIZER_SHADOW_VMA (1 << 3)
#define FILTER_NON_READ_VMA         (1 << 4)

#define FAKE_AUXV_PHDR 0x100000

/*
                                        +--------------------------------+
                                        |           ELF Header           |
                                        +--------------------------------+ AT_PHDR(0x10000)
                                        |    Program Header (PT_NOTE)    |-------------------------+
                                        +--------------------------------+ p_vaddr: 0x10000        |
                                        |  Program Header (PT_LOAD fake) |-------------------------|
                                        +--------------------------------+                         |
                                        |    Program Header (PT_LOAD)    |                         |
                                        +--------------------------------+                         |
                                        |          ...........           |                         |
                note_info_offset------> +--------------------------------+                         |
                                   ^    |                                |                         |
                                   |    |                                |                         |
                       total_note_size  |         PT_NOTE (data)         |                         |
                                   |    |                                |                         |
                                   v    |                                |                         |
+-----------------+\  vma_offset------> +--------------------------------+<------------------------+
|     ET_DYN      | \                 / |           FAKE PHDR            |   ^
+-----------------+  \               / /+--------------------------------+   |
|     PT_PHDR     |   --------------- / |          FAKE DYNAMIC          | PT_LOAD (fake data)
+-----------------+  /---------------/ /+--------------------------------+   |
|    PT_DYNAMIC   | /               / / |         FAKE LINK MAP          |-------------------------+
+-----------------+/               / /  +--------------------------------+   |                     |
                                  / /   |          FAKE STRTAB           |   v                     |
                                 / /    +--------------------------------+<---                     |
                 +-----------+--/ /     |          PT_LOAD (data)        |                         |
                 |  Dynamic  |   /      +--------------------------------+                         |
                 +-----------+  /       |          PT_LOAD (data)        |     linkmap.l_addr      |
                 |   Debug   | /        +--------------------------------+ <-----------------------+
                 +-----------+/         |        Replace Data            |
                                        |     (from symbols file,        |
                                        |     off: pgoff << 12           |
                                        |     size: memze)               |
                                        +--------------------------------+
                                        |          PT_LOAD (data)        |
                                        +--------------------------------+
                                        |          PT_LOAD (data)        |
                                        +--------------------------------+
                                        |          ...........           |
                                        +--------------------------------+
*/

typedef unsigned long long __ull[2];

#ifndef IS_ARM
struct elf_prpsinfo {
    char pr_state;
    char pr_sname;
    char pr_zomb;
    char pr_nice;
    unsigned long pr_flag;
    __kernel_uid_t pr_uid;
    __kernel_uid_t pr_gid;
    int pr_pid;
    int pr_ppid;
    int pr_pgrp;
    int pr_sid;
    char pr_fname[16];
    char pr_psargs[80];
};

struct elf_siginfo {
    int si_signo;
    int si_errno;
    int si_code;
    // char padding[36];
};
#endif

struct symbol_info {
    uintmax_t dynamic_offset;
    uintmax_t dynamic_vaddr;
    uintmax_t phdr_offset;
    uintmax_t phdr_vaddr;
    std::string lib_path;
    void* map_addr;
    size_t map_size;
};

struct user_regset {
    unsigned int n;
    unsigned int size;
    unsigned int align;
    unsigned int bias;
    unsigned int core_note_type;
};

struct user_regset_view {
    std::string name;
    std::vector<std::shared_ptr<user_regset>> regsets;
    unsigned int n;
    uint32_t e_flags;
    uint16_t e_machine;
    uint8_t ei_osabi;
};

struct memelfnote{
    std::string name;
    int type;
    int datasz;
    void *data;
};

struct elf_thread_info {
    ulong task_addr;
    std::shared_ptr<memelfnote> prstatus_ptr;
    std::vector<std::shared_ptr<memelfnote>> note_list;
};

struct user_pt_regs {
    unsigned long long regs[31];
    unsigned long long  sp;
    unsigned long long  pc;
    unsigned long long  pstate;
};

struct pt_regs {
    union {
        struct user_pt_regs user_regs;
        struct {
            unsigned long long regs[31];
            unsigned long long sp;
            unsigned long long pc;
            unsigned long long pstate;
        };
    };
    unsigned long long orig_x0;
    int syscallno;
    unsigned int unused2;
    unsigned long long sdei_ttbr1;
    unsigned long long pmr_save;
    unsigned long long stackframe[2];
    unsigned long long lockdep_hardirqs;
    unsigned long long exit_rcu;
};

struct user_fpsimd_state {
    __ull/*__uint128_t*/ vregs[32];
    unsigned int   fpsr;
    unsigned int   fpcr;
    unsigned int   __reserved[2];
};

typedef struct {
    ulong version;
    ulong map;
} r_debug_t;

typedef struct {
    ulong type;
    ulong value;
} dynamic_t;

typedef struct {
    ulong addr;
    ulong name;
    ulong ld;
    ulong next;
    ulong prev;
} linkmap_t;

struct tls_t {
    unsigned long tp_value;
    unsigned long long tpidr2_el0;
};

class Core : public ParserPlugin {
private:
    void parser_exec_name(ulong addr);
    bool SearchFile(const std::string &directory, const std::string &name, std::string &result);
    bool InnerSearchFile(const std::string &path, std::string name, std::string &result);
    void ListFiles(const std::string &directory, std::string name, std::string &result);
    bool write_pt_note(void);
    bool write_pt_load(std::shared_ptr<vma_struct> vma_ptr, size_t phdr_pos, size_t& data_pos);
    void write_core_file(void);
    bool parser_user_regset_view(void);
    std::string vma_flags_to_str(unsigned long flags);
    void write_phdr(size_t p_type, size_t p_offset, size_t p_vaddr, size_t p_filesz, size_t p_memsz, size_t p_flags, size_t p_align);
    void parser_nt_file();
    void parser_thread_core_info();
    void parser_auvx();
    int vma_dump_size(std::shared_ptr<vma_struct> vma_ptr);
    void dump_align(std::streampos position, std::streamsize align);
    void writenote(std::shared_ptr<memelfnote> note_ptr);
    void *fill_elf_header(int type, int phnum, size_t &hdr_size);
    int notesize(std::shared_ptr<memelfnote> note_ptr);
    int get_phdr_start();
    int get_phdr_size();
    void *map_elf_file(std::string filepath, size_t &len);
    bool check_elf_file(void * map);
    std::shared_ptr<symbol_info> read_elf_file(std::string file_path);
    void free_lib_map();
    size_t replace_phdr_load(std::shared_ptr<vma_struct> vma_ptr);
    void write_fake_data(size_t &data_pos, size_t phdr_pos);
    int get_pt_note_data_start();

protected:
    void* hdr_ptr;
    int core_pid;
    FILE* corefile;
    ulong thread_info_flags;
    int elf_class;
    std::string exe_name;
    std::string user_view_var_name;
    std::shared_ptr<user_regset_view> urv_ptr;
    std::set<std::string> lib_list;
    struct task_context *tc;
    std::vector<std::shared_ptr<elf_thread_info>> thread_list;
    std::shared_ptr<memelfnote> psinfo;
    std::shared_ptr<memelfnote> signote;
    std::shared_ptr<memelfnote> auxv;
    std::shared_ptr<memelfnote> files;
    std::shared_ptr<Swapinfo> swap_ptr;

public:
    static int cmd_flags;
    static std::string symbols_path;
    static std::string output_path;
    static const int CORE_REPLACE_HEAD = 0x0001;
    static const int CORE_FAKE_LINKMAP = 0x0002;
    std::shared_ptr<UTask> task_ptr;

    Core(std::shared_ptr<Swapinfo> swap);
    ~Core();
    void cmd_main(void) override;
    void init_offset(void) override;
    void init_command(void) override;
    void parser_core_dump(void);
    void set_core_pid(int pid){
        core_pid = pid;
    };
    void print_linkmap();
    void print_proc_mapping();
    int task_pid_nr_ns(ulong task_addr, long type, ulong ns_addr = 0);
    int pid_nr_ns(ulong pids_addr, ulong pid_ns_addr);
    int pid_alive(ulong task_addr);
    ulong ns_of_pid(ulong thread_pid_addr);
    ulong task_pid_ptr(ulong task_addr, long type);
    template <size_t N>
    void copy_and_fill_char(char (&dest)[N], const char* src, size_t src_len){
        size_t len = (src_len < (N - 1)) ? src_len : (N - 1);
        std::copy_n(src, len, dest);
        dest[len] = '\0';
    }
    virtual void parser_prpsinfo()=0;
    virtual void parser_siginfo()=0;
    virtual void* parser_prstatus(ulong task_addr,int* data_size)=0;
    virtual void* parser_nt_prfpreg(ulong task_addr)=0; // NT_PRFPREG
    virtual void* parser_nt_arm_tls(ulong task_addr)=0; // NT_ARM_TLS
    virtual void* parser_nt_arm_hw_break(ulong task_addr)=0; // NT_ARM_HW_BREAK
    virtual void* parser_nt_arm_hw_watch(ulong task_addr)=0; // NT_ARM_HW_WATCH
    virtual void* parser_nt_arm_system_call(ulong task_addr)=0; // NT_ARM_SYSTEM_CALL
    virtual void* parser_nt_arm_sve(ulong task_addr)=0; // NT_ARM_SVE
    virtual void* parser_nt_arm_pac_mask(ulong task_addr)=0; // NT_ARM_PAC_MASK
    virtual void* parser_nt_arm_pac_enabled_keys(ulong task_addr)=0; // NT_ARM_PAC_ENABLED_KEYS
    virtual void* parser_nt_arm_paca_keys(ulong task_addr)=0; // NT_ARM_PACA_KEYS
    virtual void* parser_nt_arm_pacg_keys(ulong task_addr)=0; // NT_ARM_PACG_KEYS
    virtual void* parser_nt_arm_tagged_addr_ctrl(ulong task_addr)=0; // NT_ARM_TAGGED_ADDR_CTRL
    virtual void* parser_nt_arm_vfp(ulong task_addr)=0; // NT_ARM_VFP
};

#endif // CORE_DEFS_H_
