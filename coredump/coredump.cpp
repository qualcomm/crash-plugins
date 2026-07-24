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

#include "coredump.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(Coredump)
#endif

void Coredump::cmd_main(void) {
    int c;
    int pid = -1;
    int flags = 0;
    std::string cppString;
    Core::cmd_flags = 0;
    if (argcnt < 2) cmd_usage(pc->curcmd, SYNOPSIS);
    while ((c = getopt(argcnt, args, "p:m:l:s:d:rf")) != EOF) {
        switch(c) {
            case 'p':
                cppString.assign(optarg);
                try {
                    pid = std::stoi(cppString);
                } catch (...) {
                    LOGE("Invalid PID argument: %s", cppString.c_str());
                }
                flags |= PRINT_COREDUMP;
                break;
            case 'm':
                cppString.assign(optarg);
                try {
                    pid = std::stoi(cppString);
                } catch (...) {
                    LOGE("Invalid PID argument: %s", cppString.c_str());
                }
                flags |= PRINT_PROCMAP;
                break;
            case 's':
                Core::symbols_path.assign(optarg);
                break;
            case 'd':
                Core::output_path.assign(optarg);
                break;
            case 'l':
                cppString.assign(optarg);
                try {
                    pid = std::stoi(cppString);
                } catch (...) {
                    LOGE("invaild pid arg %s\n",cppString.c_str());
                }
                flags |= PRINT_LINKMAP;
                break;
            case 'f':
                Core::cmd_flags |= Core::CORE_FAKE_LINKMAP;
                break;
            case 'r':
                Core::cmd_flags |= Core::CORE_REPLACE_HEAD;
                break;
            default:
                argerrs++;
                break;
        }
    }
    if (argerrs){
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }
    if ((Core::cmd_flags & Core::CORE_FAKE_LINKMAP)
            || (Core::cmd_flags & Core::CORE_REPLACE_HEAD)
            || (flags & PRINT_LINKMAP)){
        if(Core::symbols_path.empty()){
            cmd_usage(pc->curcmd, SYNOPSIS);
            return;
        }
    }
    if (flags & PRINT_COREDUMP){
        generate_coredump(pid);
    }else if(flags & PRINT_PROCMAP){
        print_proc_mapping(pid);
    }else if(flags & PRINT_LINKMAP){
        print_linkmap(pid);
    }else{
        cmd_usage(pc->curcmd, SYNOPSIS);
    }
}

void Coredump::init_offset(void) {
    field_init(task_struct, flags);
    field_init(task_struct, thread_info);
    field_init(thread_info, flags);
}

void Coredump::print_linkmap(int pid){
    if(get_core_parser(pid) && core_parser != nullptr){
        core_parser->set_core_pid(pid);
        core_parser->print_linkmap();
    }
}

bool Coredump::get_core_parser(int pid){
    struct task_context *tc = pid_to_context(pid);
    if (!tc) {
        LOGE("No task context found for pid %d", pid);
        return false;
    }
    set_context(tc->task, NO_PID, TRUE);
    ulong task_flags = read_structure_field(tc->task,"task_struct","flags");
    if (task_flags & PF_KTHREAD) {
        LOGE("PID %d is a kernel thread (no VM)", pid);
        return false;
    }
    if (!tc->mm_struct) {
        LOGE("PID %d has no virtual memory space", pid);
        return false;
    }
    fill_thread_info(tc->thread_info);
    if (field_offset(thread_info, flags) != -1){
        ulong thread_flags = read_ulong(tc->task + field_offset(task_struct, thread_info) + field_offset(thread_info, flags), "coredump task_struct thread_info flags");
        if(thread_flags & (1 << 22)){
            is_compat = true;
            LOGI("Process is running in compat mode (32-bit)");
        }
    }
#if defined(ARM64)
    if (is_compat){
        core_parser = std::make_shared<Compat>(swap_ptr);
    }else{
        core_parser = std::make_shared<Arm64>(swap_ptr);
    }
#endif
#if defined(ARM)
    core_parser = std::make_shared<Arm>(swap_ptr);
#endif
    return true;
}

void Coredump::print_proc_mapping(int pid){
    if(get_core_parser(pid) && core_parser != nullptr){
        core_parser->set_core_pid(pid);
        core_parser->print_proc_mapping();
    }
}

void Coredump::generate_coredump(int pid){
    if(get_core_parser(pid) && core_parser != nullptr){
        core_parser->set_core_pid(pid);
        core_parser->parser_core_dump();
        PRINT("Coredump is Done \n");
    }
}

Coredump::Coredump(std::shared_ptr<Swapinfo> swap) : swap_ptr(swap){}

Coredump::Coredump(){
    swap_ptr = std::make_shared<Swapinfo>();
}

void Coredump::init_command(void){
    ParserPlugin::cmd_name = "coredump";
    help_str_list={
        "coredump",                                /* command name */
        "generate process coredump",              /* short description */
        "[-p pid] [-m pid] [-l pid] [-d path] [-s symbols_path] [-f] [-r]\n"
        "  This command generates process coredump files and displays process memory information.\n"
        "\n"
        "    -p pid          generate coredump for specified process ID\n"
        "    -m pid          display process memory mappings (VMA information)\n"
        "    -l pid          display linkmap with main executable symbols\n"
        "    -d path         specify output directory path for coredump file\n"
        "    -s symbols_path specify directory containing symbol files\n"
        "    -f              generate fake coredump with symbols (requires -s)\n"
        "    -r              replace only main executable PHDR (requires -s)\n",
        "\n",
        "EXAMPLES",
        "  Generate process coredump, the default path is the current directory:",
        "    %s> coredump -p 323 -d <path>",
        "\n",
        "  Generate process fake coredump with symbols:",
        "    %s> coredump -p 323 -s <symbols_path> -f",
        "      overwrite /system/bin/logd:[size:0x15000 off:0] to core:[0x6052e20000 - 0x6052e35000]",
        "\n",
        "  Get the linkmap with main exec symbols:",
        "    %s> coredump -l 323 -s <symbols_path>",
        "      addr                ld                  next                prev                name",
        "      56ef6c4000          56ef6cc090          7eafbb4158          0                   /system/bin/app_process64",
        "      7eafa29000          7eafa29848          7eae8eb630          7eafbb4158          [vdso]",
        "\n",
        "  Generate process coredump, only replace the main executable PHDR",
        "    %s> coredump -p 323 -s <symbols_path> -r",
        "      overwrite PHDR /system/bin/logd:[size:0x15000 off:0] to core:[0x6052e20000 - 0x6052e35000]",
        "\n",
        "  Show process maps:",
        "    %s> coredump -m 323",
        "      VMA:ffffff801d653b40 [6052e20000-6052e35000] r--p 0000000000000071 00000000 /system/bin/logd",
        "      VMA:ffffff801d653000 [6052e38000-6052ebd000] r-xp 0000000000000075 00000018 /system/bin/logd",
        "      VMA:ffffff80279c05a0 [7befacb000-7befb14000] rw-p 0000000000100073 07befacb [anon:scudo:secondary]",
        "\n",
    };
}

#pragma GCC diagnostic pop
