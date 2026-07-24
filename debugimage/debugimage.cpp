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

#include "debugimage.h"
#include <inttypes.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(DebugImage)
#endif

/**
 * @brief ARM64 register mapping table
 * Maps register names to their positions in the user_regs array
 */
static const std::unordered_map<std::string, int> arm64_reg_map = {
    // General purpose registers X0-X30
    {"x0", 0},   {"x1", 1},   {"x2", 2},   {"x3", 3},
    {"x4", 4},   {"x5", 5},   {"x6", 6},   {"x7", 7},
    {"x8", 8},   {"x9", 9},   {"x10", 10}, {"x11", 11},
    {"x12", 12}, {"x13", 13}, {"x14", 14}, {"x15", 15},
    {"x16", 16}, {"x17", 17}, {"x18", 18}, {"x19", 19},
    {"x20", 20}, {"x21", 21}, {"x22", 22}, {"x23", 23},
    {"x24", 24}, {"x25", 25}, {"x26", 26}, {"x27", 27},
    {"x28", 28}, {"x29", 29}, {"x30", 30},

    // Special registers
    {"sp", 31},       // Stack pointer (generic name)
    {"sp_el1", 31},   // Stack pointer EL1
    {"pc", 32},       // Program counter
    {"pstate", 33},   // Processor state (generic name)
    {"spsr", 33},     // Saved processor state (generic name)
    {"spsr_el1", 33}, // Saved processor state EL1
};

/**
 * @brief Main command entry point for DebugImage plugin
 *
 */
void DebugImage::cmd_main(void) {
    if (argcnt < 2) {
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }

    // Variables for all operations
    int pid = -1;
    int cpu = -1;
    std::string cmm_file;
    bool has_cpu_option = false;

    // Lazy initialization: parse memdump only when needed
    if (image_list.size() == 0){
        LOGI("Image list empty, parsing memdump");
        parser_memdump();
    } else {
        LOGD("Image list already populated with %zu entries", image_list.size());
    }

    // Process command-line options
    int opt;
    optind = 0; // reset

    while ((opt = getopt(argcnt, args, "adsi:c:l:p:")) != EOF) {
        switch(opt) {
            case 'a':
                // Print memdump info
                LOGI("Printing all memdump info");
                print_memdump();
                return;

            case 'd':
                // Parse CPU context and generate CMM files
                LOGI("Parsing CPU context and generating CMM files");
                parse_cpu_ctx();
                return;

            case 's':
                // Print CPU stack traces
                LOGI("Printing CPU stack traces");
                print_cpu_stack();
                return;

            case 'i':
                // Print IRQ stack
                try {
                    cpu = std::stoi(optarg);
                    LOGI("Printing IRQ stack for CPU: %d", cpu);
                } catch (...) {
                    LOGE("Invalid CPU index: %s", optarg);
                    cpu = 0;
                }
                print_irq_stack(cpu);
                return;

            case 'c':
                // CPU index for register manipulation
                has_cpu_option = true;
                try {
                    cpu = std::stoi(optarg);
                    LOGD("CPU index: %d", cpu);
                } catch (...) {
                    LOGE("Invalid CPU index: %s", optarg);
                    cpu = 0;
                }
                break;

            case 'l':
                // CMM file path
                cmm_file = optarg;
                LOGD("CMM file: %s", cmm_file.c_str());
                break;

            case 'p':
                // Print task stack for PID
                try {
                    pid = std::stoi(optarg);
                    LOGI("Printing task stack for PID: %d", pid);
                    print_task_stack(pid);
                    return;
                } catch (...) {
                    LOGE("Invalid PID: %s", optarg);
                    argerrs++;
                }
                break;

            default:
                argerrs++;
                break;
        }
    }

    // Handle CPU register manipulation (requires -c option)
    if (has_cpu_option && cpu >= 0) {
        LOGD("Processing CPU register manipulation for CPU %d", cpu);

        // Validate architecture first
        if (!machine_type(TO_CONST_STRING("ARM64"))) {
            LOGE("Only ARM64 architecture is supported");
            return;
        }

        // Validate vmcore data
        if (!get_kdump_vmcore_data()) {
            LOGE("This dump does not have vmcore data");
            return;
        }

        // Perform the requested operation
        if (!cmm_file.empty()) {
            LOGI("Loading CPU %d registers from CMM file: %s", cpu, cmm_file.c_str());
            parser_cpu_set(cmm_file, cpu);
        } else {
            LOGE("Must specify -l <cmm_file> with -c <cpu>");
            return;
        }
        return;
    }

    if (argerrs) {
        cmd_usage(pc->curcmd, SYNOPSIS);
    }
}

/**
 * @brief Set CPU registers from CMM file
 */
void DebugImage::parser_cpu_set(const std::string& cmm, int cpu) {
#ifdef ARM64
    struct vmcore_data *vmd = get_kdump_vmcore_data();

    // Allocate memory for user data
    size_t user_data_size = sizeof(struct elf64_prstatus) + sizeof(Elf64_Nhdr) + 8;
    char* prstatus = (char*)malloc(user_data_size);
    memset(prstatus, 0, user_data_size);

    // Setup ELF note header
    Elf64_Nhdr note64;
    note64.n_namesz = 5;
    note64.n_descsz = sizeof(struct elf64_prstatus);
    note64.n_type = NT_PRSTATUS;

    // Create magic string
    char magic[8];
    memset(magic, 0, sizeof(magic));
    snprintf(magic, 5, "CPU%d", cpu);

    // Calculate offsets
    size_t len = sizeof(Elf64_Nhdr);
    len = roundup(len + note64.n_namesz, 4);
    len = roundup(len + note64.n_descsz, 4);

    field_init(elf_prstatus,pr_reg);
    char* user_regs = prstatus + len - sizeof(struct elf64_prstatus) + field_offset(elf_prstatus,pr_reg);

    // Copy header and magic
    memcpy(prstatus, &note64, sizeof(Elf64_Nhdr));
    memcpy(prstatus + sizeof(Elf64_Nhdr), magic, 8);

    // Open CMM file using
    FILE* cmm_file = fopen(cmm.c_str(), "r");
    if (!cmm_file) {
        LOGE("Failed to open CMM file: %s", cmm.c_str());
        free(prstatus);
        return;
    }

    // Parse CMM file line by line
    char temp_buffer[512];
    while (fgets(temp_buffer, sizeof(temp_buffer), cmm_file)) {
        // Convert to std::string
        std::string line(temp_buffer);
        // Remove newline character
        if (!line.empty() && line.back() == '\n') {
            line.pop_back();
        }
        // Parse using std::istringstream (keep original logic)
        std::istringstream iss(line);
        std::string ops_name, reg_name;
        uint64_t reg_val;
        // Parse line: type_name regs_name address
        if (!(iss >> ops_name >> reg_name >> std::hex >> reg_val)) {
            continue;  // Skip malformed lines
        }
        // Only process "r.s" type entries
        if (ops_name != "r.s") {
            continue;
        }
        // Update user_regs
        auto it = arm64_reg_map.find(reg_name);
        if (it != arm64_reg_map.end()) {
            if (it->second == 33){
                reg_val |= 1;
            }
            memcpy(user_regs + sizeof(ulong) * it->second, &reg_val, sizeof(ulong));
            LOGD("set %s %#lx \n", reg_name.c_str(), reg_val);
        }
    }
    fclose(cmm_file);
    LOGD("user_regs: \n");
    LOGD("%s \n", hexdump(0x1000, (char*)user_regs, sizeof(struct arm64_pt_regs)).c_str());
    // Initialize panic_task_regs if needed
    if (!machdep->machspec->panic_task_regs) {
        machdep->machspec->panic_task_regs = (struct arm64_pt_regs *)calloc((size_t)kt->cpus, sizeof(struct arm64_pt_regs));
        memset(machdep->machspec->panic_task_regs, 0x0, (size_t)kt->cpus * sizeof(struct arm64_pt_regs));
    }
    // Copy registers to panic_task_regs
    if (machdep->machspec->panic_task_regs) {
        BCOPY(user_regs, &machdep->machspec->panic_task_regs[cpu], sizeof(struct arm64_pt_regs));
    }

    if(vmd->nt_prstatus_percpu[cpu] != nullptr){
        free(vmd->nt_prstatus_percpu[cpu]);
    }
    vmd->nt_prstatus_percpu[cpu] = prstatus;
    PRINT("CPU %d registers loaded from %s\n", cpu, cmm.c_str());
#endif
}

void DebugImage::init_offset(void) {
    field_init(msm_memory_dump,table_phys);
    field_init(msm_dump_table,version);
    field_init(msm_dump_table,num_entries);
    field_init(msm_dump_table,entries);
    field_init(msm_dump_entry,id);
    field_init(msm_dump_entry,name);
    field_init(msm_dump_entry,type);
    field_init(msm_dump_entry,addr);
    field_init(msm_dump_data,version);
    field_init(msm_dump_data,magic);
    field_init(msm_dump_data,name);
    field_init(msm_dump_data,addr);
    field_init(msm_dump_data,len);
    field_init(msm_dump_data,reserved);
    struct_init(msm_dump_table);
    struct_init(msm_dump_entry);
    struct_init(msm_dump_data);
}

void DebugImage::init_command(void) {
    cmd_name = "dbi";
    help_str_list={
        "dbi",                                                    /* command name */
        "dump debug image region information and manage CPU registers",  /* short description */
        "[-a] [-d] [-s] [-i cpu] [-p pid] [-c cpu -l cmm_file]\n"
        "  This command dumps debug image information and manages CPU registers.\n"
        "\n"
        "    -a          display all memdump information\n"
        "    -d          parse CPU context and generate CMM files\n"
        "    -s          print CPU stack traces for all cores\n"
        "    -i cpu      print IRQ stack for specified CPU\n"
        "    -p pid      print task stack for specified process ID\n"
        "    -c cpu      specify CPU index for register operations (use with -l)\n"
        "    -l cmm_file load CPU registers from Trace32 CMM file (ARM64 only, requires -c)\n",
        "\n",
        "EXAMPLES",
        "  Display all memdump information:",
        "    %s> dbi -a",
        "\n",
        "  Generate CMM files from CPU context:",
        "    %s> dbi -d",
        "\n",
        "  Print CPU stack traces for all cores:",
        "    %s> dbi -s",
        "\n",
        "  Print IRQ stack for specific CPU:",
        "    %s> dbi -i 0",
        "\n",
        "  Print task stack for specific PID:",
        "    %s> dbi -p 1234",
        "\n",
        "  Load CPU registers from CMM file (ARM64 only):",
        "    %s> dbi -c 0 -l /path/to/core0_regs.cmm",
        "    CPU 0 registers loaded from /path/to/core0_regs.cmm",
        "\n",
    };

    field_init(msm_dump_cpu_ctx, affinity);
    cpu_index_offset = field_offset(msm_dump_cpu_ctx, affinity);
    if (cpu_index_offset == -1){
        LOGW("msm_dump_cpu_ctx.affinity offset not found, using default 0x10");
        cpu_index_offset = 0x10;
    } else {
        LOGD("msm_dump_cpu_ctx.affinity offset: 0x%x", cpu_index_offset);
    }
}

DebugImage::DebugImage(){
    do_init_offset = false;
}

DebugImage::~DebugImage(){

}

void DebugImage::print_memdump(){
    LOGD("Printing memdump table with %zu entries", image_list.size());
    std::ostringstream oss;
    oss  << std::left << std::setw(4)            << "Id" << " "
            << std::left << std::setw(16)           << "Dump_entry" << " "
            << std::left << std::setw(8)            << "version" << " "
            << std::left << std::setw(VADDR_PRLEN)  << "magic" << " "
            << std::left << std::setw(VADDR_PRLEN)  << "DataAddr" << " "
            << std::left << std::setw(10)           << "DataLen" << " "
            << std::left << "Name" << "\n";;
    // parse_dump_v2
    for (const auto& entry_ptr : image_list) {
        oss << std::left << std::setw(4)            << std::dec << entry_ptr->id << " "
            << std::left << std::setw(16)           << std::hex << entry_ptr->addr << " "
            << std::left << std::setw(8)            << std::dec << entry_ptr->version << " "
            << std::left << std::setw(VADDR_PRLEN)  << std::hex << entry_ptr->magic << " "
            << std::left << std::setw(VADDR_PRLEN)  << std::hex << entry_ptr->data_addr << " "
            << std::left << std::setw(10)           << std::dec << entry_ptr->data_len << " "
            << std::left  << entry_ptr->data_name << "\n";;
    }
    PRINT("%s\n", oss.str().c_str());
}

void DebugImage::parser_memdump(){
    LOGI("Starting memdump parsing");
    struct_init(msm_memory_dump);
    if (struct_size(msm_memory_dump) == -1){
        LOGE("memdump doesn't exist in this kernel! load memory_dump_v2.ko");
        return;
    }
    if (!csymbol_exists("memdump")){
        LOGE("memdump symbol not found");
        return;
    }
    ulong dump_addr = csymbol_value("memdump");
    LOGD("memdump symbol address: %#lx", dump_addr);
    if (!is_kvaddr(dump_addr)) {
        LOGE("memdump address is invalid: %#lx", dump_addr);
        return;
    }
    init_offset();
    uint64_t table_phys = read_ulonglong(dump_addr + field_offset(msm_memory_dump,table_phys),"table_phys");
    LOGI("msm_memory_dump: DumpTable base=%#llx", table_phys);
    parser_dump_table(table_phys);
    LOGI("Memdump parsing completed, found %zu entries", image_list.size());
}

void DebugImage::print_cpu_stack(){
    LOGD("Printing CPU stack for all cores");
    int cpu_count = 0;
    for (const auto& entry_ptr : image_list) {
        if (entry_ptr->id >= DATA_CPU_CTX && entry_ptr->id < DATA_L1_INST_TLB){
            cpu_count++;
            parse_cpu_stack(entry_ptr);
        }
    }
    LOGI("Processed %d CPU contexts", cpu_count);
}

void DebugImage::parse_cpu_ctx(){
    LOGD("Parsing CPU context for all cores");
    int cpu_count = 0;
    for (const auto& entry_ptr : image_list) {
        if (entry_ptr->id >= DATA_CPU_CTX && entry_ptr->id < DATA_L1_INST_TLB){
            cpu_count++;
            parse_cpu_ctx(entry_ptr);
        }
    }
    LOGI("Generated CMM files for %d CPU contexts", cpu_count);
}

void DebugImage::parse_cpu_stack(std::shared_ptr<Dump_entry> entry_ptr){
    int core = entry_ptr->id - DATA_CPU_CTX;
    int major = entry_ptr->version >> 4;
    int minor = entry_ptr->version & 0xF;
    LOGD("Parsing CPU stack for core %d, version %d.%d", core, major, minor);
    if (major == 2 && minor == 0){ //v2.0
        uint32_t affinity = read_uint(entry_ptr->data_addr + cpu_index_offset, "affinity", false);
        parser_ptr = std::make_shared<Cpu64_Context_V20>();
        core = parser_ptr->get_vcpu_index(affinity);
        LOGI("%s  core:%d  version:%d.%d (v2.0)", entry_ptr->data_name.c_str(), core, major, minor);
    }else{
        LOGI("%s  core:%d  version:%d.%d", entry_ptr->data_name.c_str(), core, major, minor);
        if (BITS64()){
            if (major == 1 && minor == 3){ //v1.3
                LOGD("Using Cpu64_Context_V13 parser");
                parser_ptr = std::make_shared<Cpu64_Context_V13>();
            }else if (major == 1 && minor == 4){ //v1.4
                LOGD("Using Cpu64_Context_V14 parser");
                parser_ptr = std::make_shared<Cpu64_Context_V14>();
            } else {
                LOGW("Unknown 64-bit version %d.%d, using default parser", major, minor);
            }
        }else{
            LOGD("Using Cpu32_Context parser");
            parser_ptr = std::make_shared<Cpu32_Context>();
        }
    }
    parser_ptr->print_stack(entry_ptr);
}

void DebugImage::parse_cpu_ctx(std::shared_ptr<Dump_entry> entry_ptr){
    int core = entry_ptr->id - DATA_CPU_CTX;
    int major = entry_ptr->version >> 4;
    int minor = entry_ptr->version & 0xF;
    LOGD("Generating CMM for core %d, version %d.%d", core, major, minor);
    if (major == 2 && minor == 0){ //v2.0
        uint32_t affinity = read_uint(entry_ptr->data_addr + cpu_index_offset, "affinity", false);
        parser_ptr = std::make_shared<Cpu64_Context_V20>();
        core = parser_ptr->get_vcpu_index(affinity);
        LOGI("%s  core:%d  version:%d.%d (v2.0)", entry_ptr->data_name.c_str(), core, major, minor);
    }else{
        LOGI("%s  core:%d  version:%d.%d", entry_ptr->data_name.c_str(), core, major, minor);
        if (BITS64()){
            if (major == 1 && minor == 3){ //v1.3
                LOGD("Using Cpu64_Context_V13 parser");
                parser_ptr = std::make_shared<Cpu64_Context_V13>();
            }else if (major == 1 && minor == 4){ //v1.4
                LOGD("Using Cpu64_Context_V14 parser");
                parser_ptr = std::make_shared<Cpu64_Context_V14>();
            } else {
                LOGW("Unknown 64-bit version %d.%d, using default parser", major, minor);
            }
        }else{
            LOGD("Using Cpu32_Context parser");
            parser_ptr = std::make_shared<Cpu32_Context>();
        }
    }
    parser_ptr->generate_cmm(entry_ptr);
}

void DebugImage::parser_dump_data(std::shared_ptr<Dump_entry> entry_ptr, int depth){
    std::string indent(depth * 2, ' ');

    entry_ptr->version = read_uint(entry_ptr->addr + field_offset(msm_dump_data,version),"version",false);
    entry_ptr->magic = read_uint(entry_ptr->addr + field_offset(msm_dump_data,magic),"magic",false);
    if ((entry_ptr->magic != MAGIC_NUMBER) && (entry_ptr->magic != HYP_MAGIC_NUMBER)){
        LOGW("%smsm_dump_data: invalid magic=%#x at addr=%#lx (expected %#x or %#x)",
             indent.c_str(), entry_ptr->magic, (ulong)entry_ptr->addr, MAGIC_NUMBER, HYP_MAGIC_NUMBER);
        return;
    }
    if (entry_ptr->id > DATA_MAX){
        LOGD("%smsm_dump_data: skipping ID=%#x (exceeds DATA_MAX %#x)", indent.c_str(), entry_ptr->id, DATA_MAX);
        return;
    }
    entry_ptr->data_name= read_cstring(entry_ptr->addr + field_offset(msm_dump_data,name),32, "name",false);
    entry_ptr->data_addr = read_ulonglong(entry_ptr->addr + field_offset(msm_dump_data,addr),"addr",false);
    entry_ptr->data_len = read_ulonglong(entry_ptr->addr + field_offset(msm_dump_data,len),"len",false);
    LOGD("%smsm_dump_data: ID=%#x, name=%s, addr=%#lx, len=%lu",
         indent.c_str(), entry_ptr->id, entry_ptr->data_name.c_str(), (ulong)entry_ptr->data_addr, (ulong)entry_ptr->data_len);
    image_list.push_back(entry_ptr);
}

void DebugImage::parser_dump_table(uint64_t paddr){
    static int depth = 0;
    depth++;
    std::string indent(depth * 2, ' ');

    LOGD("%smsm_dump_table: parsing at address %#lx", indent.c_str(), (ulong)paddr);
    uint32_t num_entries = read_uint(paddr + field_offset(msm_dump_table,num_entries),"num_entries",false);
    if (num_entries == 0 || num_entries > 100){
        LOGE("%smsm_dump_table: invalid num_entries=%u at address %#lx (expected 1-100)", indent.c_str(), num_entries, (ulong)paddr);
        depth--;
        return;
    }
    LOGI("%smsm_dump_table: found %u entries", indent.c_str(), num_entries);
    uint64_t entries = paddr + field_offset(msm_dump_table,entries);
    for (size_t i = 0; i < num_entries; i++){
        uint64_t entry_addr = entries + struct_size(msm_dump_entry) * i;
        std::shared_ptr<Dump_entry> entry_ptr = std::make_shared<Dump_entry>();
        entry_ptr->id = read_uint(entry_addr + field_offset(msm_dump_entry,id),"id",false);
        int type = read_uint(entry_addr + field_offset(msm_dump_entry,type),"type",false);
        entry_ptr->addr = read_ulonglong(entry_addr + field_offset(msm_dump_entry,addr),"addr",false);
        LOGD("%s  msm_dump_entry[%zu/%u]: ID=%#x, type=%d, addr=%#lx",
             indent.c_str(), i+1, num_entries, entry_ptr->id, type, (ulong)entry_ptr->addr);
        if (type == Entry_type::ENTRY_TYPE_DATA){
            parser_dump_data(entry_ptr, depth + 1);
        }else if (type == Entry_type::ENTRY_TYPE_TABLE){
            LOGD("%s  msm_dump_entry: found nested msm_dump_table at %#lx", indent.c_str(), (ulong)entry_ptr->addr);
            parser_dump_table(entry_ptr->addr);
        } else {
            LOGW("%s  msm_dump_entry: unknown type=%d", indent.c_str(), type);
        }
    }
    depth--;
}

void DebugImage::print_task_stack(int pid){
#if defined(ARM64)
    LOGI("Printing task stack for PID: %d", pid);
    struct task_context *tc = pid_to_context(pid);
    if(!tc){
        LOGE("No such task_context for pid: %d", pid);
        return;
    }
    LOGD("Found task_context for PID %d, task: %#lx", pid, tc->task);
    field_init(task_struct, thread);
    field_init(thread_struct, cpu_context);

    struct cpu_context cc;
    BZERO(&cc, sizeof(struct cpu_context));
    ulong cpu_ctx_addr = tc->task + field_offset(task_struct, thread) + field_offset(thread_struct, cpu_context);
    LOGD("Reading cpu_context from address: %#lx", cpu_ctx_addr);

    if(!read_struct(cpu_ctx_addr, &cc , sizeof(struct cpu_context) ,"cpu_context in dbi")){
        LOGE("Failed to read cpu_context for PID %d", pid);
        return;
    }

    PRINT("cpu_context:\n");
    PRINT("   X19: %#lx\n", cc.x19);
    PRINT("   X20: %#lx\n", cc.x20);
    PRINT("   X21: %#lx\n", cc.x21);
    PRINT("   X22: %#lx\n", cc.x22);
    PRINT("   X23: %#lx\n", cc.x23);
    PRINT("   X24: %#lx\n", cc.x24);
    PRINT("   X25: %#lx\n", cc.x25);
    PRINT("   X26: %#lx\n", cc.x26);
    PRINT("   X27: %#lx\n", cc.x27);
    PRINT("   X28: %#lx\n", cc.x28);
    PRINT("   fp:  %#lx\n", cc.fp);
    PRINT("   sp:  %#lx\n", cc.sp);
    PRINT("   pc:  %#lx\n\n", cc.pc);

    std::map<ulong, ulong> mem_maps;
    ulong stackbase = GET_STACKBASE(tc->task);
    ulong stacktop = GET_STACKTOP(tc->task);
    LOGD("Stack range: %#lx ~ %#lx (size: %lu bytes)", stackbase, stacktop, stacktop - stackbase);
    PRINT("Stack:%#lx~%#lx\n", stackbase, stacktop);
    LOGD("Scanning stack memory for frame pointers");
    for(ulong addr = stackbase; addr < stacktop; addr += 0x10){
        ulong fp = read_pointer(addr, "frame pointer x29");
        mem_maps[addr] = fp;
    }
    LOGD("Analyzing %zu potential frame pointers", mem_maps.size());
    int cnt = 0;
    for(ulong x29: find_x29(mem_maps)){
        ulong x30 = x29 + 8;
        if(x30 < stackbase && x30 > stacktop){ // out of range
            LOGD("Skipping x29=%#lx (x30=%#lx out of stack range)", x29, x30);
            continue;
        }
        LOGD("Found valid backtrace starting point: FP=%#lx, LR=%#lx", x29, x30);
        PRINT("[%d]Potential backtrace -> FP:%#lx, LR:%#lx\n", cnt, x29, x30);
        uwind_task_back_trace(pid, x30);
        cnt++;
    }
    LOGI("Found %d potential backtraces for PID %d", cnt, pid);
#endif
}

void DebugImage::print_irq_stack(int cpu){
#if defined(ARM64)
    LOGI("Printing IRQ stack for CPU %d", cpu);
    if (cpu > kt->cpus){
        LOGE("Invalid cpu: %d (max: %d)", cpu, kt->cpus);
        return;
    }

    std::map<ulong, ulong> mem_maps;
    ulong irq_stack = machdep->machspec->irq_stacks[cpu];
    ulong irq_stack_size = machdep->machspec->irq_stack_size;
    LOGD("IRQ stack range: %#lx ~ %#lx (size: %lu bytes)", irq_stack, irq_stack + irq_stack_size, irq_stack_size);
    PRINT("CPU[%d] irq stack:%#lx~%#lx\n", cpu, irq_stack, irq_stack + irq_stack_size);
    LOGD("Scanning IRQ stack memory for frame pointers");
    for(ulong addr = irq_stack; addr < irq_stack + irq_stack_size; addr += 0x10){
        ulong fp = read_pointer(addr, "frame pointer x29");
        mem_maps[addr] = fp;
    }

    LOGD("Analyzing %zu potential frame pointers in IRQ stack", mem_maps.size());
    int cnt = 0;
    for(ulong x29: find_x29(mem_maps)){
        ulong x30 = x29 + 8;
        if(x30 < irq_stack && x30 > irq_stack + irq_stack_size){ // out of range
            LOGD("Skipping x29=%#lx (x30=%#lx out of IRQ stack range)", x29, x30);
            continue;
        }
        LOGD("Found valid IRQ backtrace starting point: FP=%#lx, LR=%#lx", x29, x30);
        PRINT("[%d]Potential backtrace -> FP:%#lx, LR:%#lx\n", cnt, x29, x30);
        uwind_irq_back_trace(cpu,x30);
        cnt++;
    }
    LOGI("Found %d potential backtraces for CPU %d IRQ stack", cnt, cpu);
    PRINT("\n");
#endif
}

std::set<ulong> DebugImage::find_x29(const std::map<ulong /* addr */, ulong /* x29 */>& addr_x29) {
    LOGD("Finding x29 frame pointers from %zu address mappings", addr_x29.size());
    // Step 1: Build a set of x29 for fast lookup
    std::set<ulong> x29_sets;
    for (const auto& kv : addr_x29) {
        x29_sets.insert(kv.second);
    }
    // Step 2: Find the first key (starting from the largest) that exists in the value set
    ulong start_addr = 0;
    bool found_start = false;
    // Reverse iterate through the map (from largest key to smallest)
    for (auto it = addr_x29.rbegin(); it != addr_x29.rend(); ++it) {
        bool exists_in_x29 = (x29_sets.find(it->first) != x29_sets.end());
        LOGD("Checking: %#lx - exists in x29? %s", it->first, (exists_in_x29 ? "YES" : "NO"));
        if (exists_in_x29) {
            start_addr = it->first;
            found_start = true;
            LOGD("!!! FOUND START ADDRESS: %#lx", start_addr);
            break;
        }
    }
    if (!found_start) {
        LOGD("No valid addr in x29");
        return {};
    }
    // Step 3: save all addrs that x29 == start_addr
    std::set<ulong> current_addrs;
    for (const auto& kv : addr_x29) {
        if (kv.second == start_addr) {
            current_addrs.insert(kv.first);
            LOGD("  Found mapping addr -> x29: %#lx -> %#lx", kv.first, kv.second);
        }
    }
    LOGD("  Total mappings found: %zu", current_addrs.size());
    // Step 4: Iteratively find final addrs, strictly following decreasing order
    std::set<ulong> result_addrs;
    int iteration = 0;
    while (!current_addrs.empty()) {
        iteration++;
        LOGD("     loop: %d, processing %zu addrs", iteration, current_addrs.size());
        std::set<ulong> next_addrs;
        for (ulong addr : current_addrs) {
            LOGD("  Processing address: %#lx", addr);
            bool found = false;
            // Look for addrs whose value equals the current addr and are smaller than the current addr
            for (const auto& kv : addr_x29) {
                if (kv.second == addr && kv.first < addr) {
                    LOGD("    Found valid child addr -> x29: %#lx -> %#lx", kv.first, kv.second);
                    next_addrs.insert(kv.first);
                    found = true;
                }
            }
            // If not found, current addr is a final addr
            if (!found) {
                LOGD("    No valid child found - marking as final address");
                result_addrs.insert(addr);
            }
        }
        LOGD("  Found %zu addresses for next loop", next_addrs.size());
        current_addrs = next_addrs;
    }
    LOGD("Final addresses found: %zu", result_addrs.size());
    return result_addrs;
}

#pragma GCC diagnostic pop
