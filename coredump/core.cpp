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
#include "core.h"
#include <chrono>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

int Core::cmd_flags = 0;
std::string Core::symbols_path;
std::string Core::output_path;

void Core::cmd_main(void) {}

void Core::init_command(void) {}

void Core::init_offset(void) {
    field_init(user_regset_view,name);
    field_init(user_regset_view,regsets);
    field_init(user_regset_view,n);
    field_init(user_regset_view,e_flags);
    field_init(user_regset_view,e_machine);
    field_init(user_regset_view,ei_osabi);
    struct_init(user_regset_view);
    field_init(user_regset,n);
    struct_init(user_regset);
    field_init(task_struct, state);
    field_init(task_struct, __state);
    field_init(task_struct, real_parent);
    field_init(task_struct, pid);
    field_init(task_struct, static_prio);
    field_init(task_struct, flags);
    field_init(task_struct, cred);
    field_init(task_struct, thread_pid);
    field_init(task_struct, signal);
    field_init(task_struct, thread_node);
    field_init(mm_struct, saved_auxv);
    field_init(cred, uid);
    field_init(cred, gid);
    field_init(signal_struct, pids);
    field_init(signal_struct, thread_head);
    field_init(pid, level);
    field_init(pid, numbers);
    struct_init(upid);
    field_init(upid, ns);
    field_init(upid, nr);
    field_init(pid_namespace, level);
    field_init(anon_vma_name, name);
    field_init(inode, i_flags);
    field_init(address_space, host);
    field_init(inode, i_nlink);
}

Core::Core(std::shared_ptr<Swapinfo> swap) : swap_ptr(swap){
    init_offset();
}

Core::~Core(){
    swap_ptr.reset();
}

void Core::parser_core_dump(void) {
    LOGD("Starting parser_core_dump");
    tc = pid_to_context(core_pid);
    std::stringstream ss = get_curpath();
    if (!Core::output_path.empty()) {
        struct stat info;
        if (stat(Core::output_path.c_str(), &info) == 0) {
            ss.str("");
            ss.clear();
            ss << Core::output_path;
        } else {
            LOGE("Cannot access output path: %s", Core::output_path.c_str());
            Core::symbols_path.clear();
            Core::output_path.clear();
            task_ptr.reset();
            return;
        }
    }
    ss << "/core." << std::dec << tc->pid << "." << tc->comm;
    corefile = fopen(ss.str().c_str(), "wb");
    if (!corefile) {
        LOGE("Failed to open core file: %s", ss.str().c_str());
        Core::symbols_path.clear();
        Core::output_path.clear();
        task_ptr.reset();
        return;
    }
    LOGI("Core file opened: %s", ss.str().c_str());
    task_ptr = std::make_shared<UTask>(swap_ptr, tc->task);
    if (BITS64()){
        user_view_var_name = task_ptr->is_compat() ? "user_aarch32_view" : "user_aarch64_view";
    }else{
        user_view_var_name = "user_arm_view";
    }
    LOGD("Using user view: %s", user_view_var_name.c_str());
    parser_user_regset_view();
    parser_prpsinfo();
    parser_siginfo();
    parser_nt_file();
    parser_auvx();
    parser_exec_name(task_ptr->get_auxv(AT_EXECFN));
    parser_thread_core_info();
    write_core_file();
    PRINT("\ncore_path: %s\n", ss.str().c_str());
    Core::symbols_path.clear();
    Core::output_path.clear();
    task_ptr.reset();
}

void Core::parser_exec_name(ulong addr){
    if (IS_UVADDR(addr, tc)){
        exe_name = task_ptr->uread_cstring(addr, 64);
    }else{
        exe_name = tc->comm;
    }
    LOGD("Executable name: %s", exe_name.c_str());
}

bool Core::SearchFile(const std::string& directory, const std::string& name, std::string& result) {
    if (!directory.empty() && !name.empty()) {
        return InnerSearchFile(directory, name, result);
    }
    return false;
}

bool Core::InnerSearchFile(const std::string& path, std::string name, std::string& result) {
    struct stat path_stat;
    if (stat(path.c_str(), &path_stat) != 0) {
        return false;
    }
    if (S_ISDIR(path_stat.st_mode)) {
        ListFiles(path, name, result);
    } else if (S_ISREG(path_stat.st_mode)) {
        size_t last_slash = path.find_last_of('/');
        std::string filename = (last_slash == std::string::npos) ? path : path.substr(last_slash + 1);

        last_slash = name.find_last_of('/');
        name = (last_slash == std::string::npos) ? name : name.substr(last_slash + 1);
        name.erase(std::remove(name.begin(), name.end(), '\0'), name.end());
        if (filename == name) {
            result = path;
        }
    }
    return !result.empty();
}

void Core::ListFiles(const std::string& directory, std::string name, std::string& result) {
    DIR* dirp = opendir(directory.c_str());
    if (!dirp) {
        return;
    }

    struct dirent* dp;
    while ((dp = readdir(dirp))) {
        std::string entry_name(dp->d_name);

        if (entry_name == "." || entry_name == "..") {
            continue;
        }

        std::string full_path = directory;
        if (full_path.back() != '/'){
            full_path += '/';
        }
        full_path += entry_name;
        if (dp->d_type == DT_DIR) {
            ListFiles(full_path, name, result);
        } else {
            size_t last_slash = full_path.find_last_of('/');
            std::string filename = (last_slash == std::string::npos) ? full_path : full_path.substr(last_slash + 1);

            last_slash = name.find_last_of('/');
            name = (last_slash == std::string::npos) ? name : name.substr(last_slash + 1);
            name.erase(std::remove(name.begin(), name.end(), '\0'), name.end());
            if (filename == name) {
                result = full_path;
                break;
            }
        }
        if(!result.empty()){
            break;
        }
    }
    closedir(dirp);
}

bool Core::write_pt_note(void) {
    LOGD("Writing PT_NOTE section");
    size_t note_data_start = get_pt_note_data_start();
    if (fseek(corefile, note_data_start, SEEK_SET) != 0) {
        LOGE("Failed to seek to note data start: %zu", note_data_start);
        fclose(corefile);
        return false;
    }
    LOGI("Writing %zu thread notes", thread_list.size());
    for (size_t i = 0; i < thread_list.size(); i++){
        const auto& thread_ptr = thread_list[i];
        writenote(thread_ptr->prstatus_ptr);
        if (i == 0){
            if(psinfo != nullptr){
                writenote(psinfo);
                psinfo.reset();
            }
            if(signote != nullptr){
                writenote(signote);
                signote.reset();
            }
            if(auxv != nullptr){
                if (BITS64() && !task_ptr->is_compat()){
                    Elf64_Auxv_t* elf_auxv = (Elf64_Auxv_t*)auxv->data;
                    for (const auto& auxv : task_ptr->for_each_auxv()) {
                        elf_auxv->type = auxv.first;
                        elf_auxv->val = auxv.second;
                        elf_auxv++;
                    }
                    elf_auxv->type = 0;
                    elf_auxv->val = 0;
                }else{
                    Elf32_Auxv_t* elf_auxv = (Elf32_Auxv_t*)auxv->data;
                    for (const auto& auxv : task_ptr->for_each_auxv()) {
                        elf_auxv->type = auxv.first;
                        elf_auxv->val = auxv.second;
                        elf_auxv++;
                    }
                    elf_auxv->type = 0;
                    elf_auxv->val = 0;
                }
                writenote(auxv);
                auxv.reset();
            }
            if(files != nullptr){
                writenote(files);
                files.reset();
            }
        }
        for(const auto& note_ptr: thread_ptr->note_list){
            if (note_ptr != thread_ptr->prstatus_ptr && note_ptr->data){
                writenote(note_ptr);
            }
        }
    }
    thread_list.clear();
    LOGD("PT_NOTE section written successfully");
    return true;
}

bool Core::write_pt_load(std::shared_ptr<vma_struct> vma_ptr, size_t phdr_pos, size_t& data_pos) {
    size_t p_flags = 0;
    if (vma_ptr->vm_flags & VM_READ){
        p_flags |= PF_R;
    }
    if (vma_ptr->vm_flags & VM_WRITE){
        p_flags |= PF_W;
    }
    if (vma_ptr->vm_flags & VM_EXEC){
        p_flags |= PF_X;
    }
    size_t p_memsz = vma_ptr->vm_size;
    size_t p_filesz = 0;
    if(!vma_dump_size(vma_ptr)){
        p_filesz = p_memsz;
    }
    if (fseek(corefile, phdr_pos, SEEK_SET) != 0) {
        LOGE("Failed to seek to phdr position: %zu", phdr_pos);
        fclose(corefile);
        return false;
    }
    write_phdr(PT_LOAD,data_pos,vma_ptr->vm_start,p_filesz,p_memsz,p_flags,page_size);

    if (vma_dump_size(vma_ptr)) {
        return true;
    }
    LOGD("Writing PT_LOAD: phdr_pos=%zu data_pos=%zu vma=[%#lx-%#lx]",
         phdr_pos, data_pos, vma_ptr->vm_start, vma_ptr->vm_end);
    if (fseek(corefile, data_pos, SEEK_SET) != 0) {
        LOGE("Failed to seek to data position: %zu", data_pos);
        fclose(corefile);
        return false;
    }
    bool replace = false;
    if (Core::cmd_flags & CORE_REPLACE_HEAD){
        /*
        In function svr4_relocate_main_executable,
        the program header in the core file is compared with the program header of the main executable to
        get the runtime address of the dynamic segment.
        */
        if ((vma_ptr->name.find(exe_name) != std::string::npos || vma_ptr->name == exe_name) &&
            (vma_ptr->vm_start <= task_ptr->get_auxv(AT_PHDR) && vma_ptr->vm_end > task_ptr->get_auxv(AT_PHDR))){
                size_t replace_size = replace_phdr_load(vma_ptr);
                if(replace_size > 0){
                    data_pos += replace_size;
                    replace = true;
                }
        }
    }
    if (Core::cmd_flags & CORE_FAKE_LINKMAP){
        /*
        replace all symbol info to core base on l_addr
        */
        for (const auto& name : lib_list) {
            std::string filepath;
            if(!SearchFile(Core::symbols_path, name, filepath)){
                continue;
            }
            std::shared_ptr<vma_struct> phdr_vma_ptr = task_ptr->get_phdr_vma(name);
            if(phdr_vma_ptr == nullptr || phdr_vma_ptr != vma_ptr){
                continue;
            }
            size_t map_size = 0;
            void *map = map_elf_file(filepath, map_size);
            if (map == nullptr){
                continue;
            }
            size_t memsz = vma_ptr->vm_end - vma_ptr->vm_start;
            size_t pgoff = vma_ptr->vm_pgoff << 12;
            fwrite(reinterpret_cast<char*>(map) + pgoff, memsz, 1, corefile);
            PRINT("overwrite %s:[size:%#zx off:%#lx] to core:[%#lx - %#lx]\n",
                  vma_ptr->name.c_str(), memsz, vma_ptr->vm_pgoff << 12,
                  vma_ptr->vm_start, vma_ptr->vm_end);
            munmap(map, map_size);
            data_pos += memsz;
            replace = true;
        }
    }
    if(replace == false){
        std::vector<char> vma_data = task_ptr->read_vma_data(vma_ptr);
        if (vma_data.size() > 0){
            fwrite(vma_data.data(), vma_ptr->vm_size, 1, corefile);
        }
        data_pos += vma_ptr->vm_size;
    }
    return true;
}

size_t Core::replace_phdr_load(std::shared_ptr<vma_struct> vma_ptr){
    if (Core::symbols_path.empty()){
        LOGW("Symbols path is empty, cannot replace PHDR");
        return -1;
    }
    std::string filepath;
    if(!SearchFile(Core::symbols_path, vma_ptr->name, filepath)){
        LOGW("Symbol file not found for: %s", vma_ptr->name.c_str());
        return -1;
    }
    LOGI("Found symbol file: %s", filepath.c_str());
    size_t map_size = 0;
    void *map = map_elf_file(filepath, map_size);
    if (map == nullptr){
        LOGE("Failed to map ELF file: %s", filepath.c_str());
        return -1;
    }
    size_t memsz = vma_ptr->vm_end - vma_ptr->vm_start;
    size_t pgoff = vma_ptr->vm_pgoff << 12;
    fwrite(reinterpret_cast<char*>(map) + pgoff, memsz, 1, corefile);
    if (munmap(map, map_size) == -1) {
        LOGE("munmap PHDR failed");
    }
    PRINT("overwrite PHDR %s:[size:%#zx off:%#lx] to core:[%#lx - %#lx]\n",
          vma_ptr->name.c_str(), memsz, vma_ptr->vm_pgoff << 12,
          vma_ptr->vm_start, vma_ptr->vm_end);
    return memsz;
}
void Core::write_core_file(void) {
    LOGI("Starting core file generation");
    size_t pt_note_size = 0;
    pt_note_size += notesize(auxv);
    pt_note_size += notesize(psinfo);
    pt_note_size += notesize(signote);
    pt_note_size += notesize(files);
    for(auto& thread_ptr: thread_list){
        for(auto& note_ptr: thread_ptr->note_list){
            if (note_ptr->data) {
                pt_note_size += notesize(note_ptr);
            }
        }
    }
    LOGD("PT_NOTE total size: %zu bytes", pt_note_size);
    int segs = task_ptr->for_each_vma_list().size() + 1; // for PT_NOTE
    if (Core::cmd_flags & CORE_FAKE_LINKMAP){
        segs += lib_list.size() ? 1 : 0; // for Fake
    }
    int e_phnum = segs > PN_XNUM ? PN_XNUM : segs;
    LOGI("Total segments: %d (e_phnum: %d)", segs, e_phnum);
    // ===========================================
    //  Writing ELF header
    // ===========================================
    LOGD("Writing ELF header");
    size_t hdr_size = 0;
    hdr_ptr = fill_elf_header(ET_CORE, e_phnum, hdr_size);
    if (fseek(corefile, 0, SEEK_SET) != 0) {
        LOGE("Failed to seek to file start");
        fclose(corefile);
        std::free(hdr_ptr);
        return;
    }
    fwrite(hdr_ptr, hdr_size, 1, corefile);

    //  ===========================================
    //  Writing PT_NOTE
    //  ===========================================
    LOGD("Writing PT_NOTE program header");
    size_t phdr_pos = get_phdr_start();
    if (fseek(corefile, phdr_pos, SEEK_SET) != 0) {
        LOGE("Failed to seek to phdr start");
        fclose(corefile);
        std::free(hdr_ptr);
        return;
    }
    if (Core::cmd_flags & CORE_FAKE_LINKMAP){
        if(lib_list.size()){ //update before write fake vma
            size_t hdr_size = (BITS64() && !task_ptr->is_compat()) ? sizeof(Elf64_Ehdr) : sizeof(Elf32_Ehdr);
            task_ptr->set_auxv(AT_PHDR,FAKE_AUXV_PHDR + hdr_size);
            task_ptr->set_auxv(AT_EXECFN,0);
            LOGI("Fake linkmap enabled with %zu libraries", lib_list.size());
        }
    }
    write_phdr(PT_NOTE, get_pt_note_data_start(), 0, pt_note_size, 0, 0, 0);
    if (!write_pt_note()){
        LOGE("Failed to write PT_NOTE");
        fclose(corefile);
        std::free(hdr_ptr);
        return;
    }

    //  ===========================================
    //  Writing Fake LOAD
    //  ===========================================
    size_t load_data_pos = roundup((get_pt_note_data_start() + pt_note_size), page_size);
    if (Core::cmd_flags & CORE_FAKE_LINKMAP){
        if(lib_list.size()){
            LOGD("Writing fake LOAD section");
            phdr_pos += get_phdr_size(); // header note
            write_fake_data(load_data_pos, phdr_pos);
        }
    }

    //  ===========================================
    //  Writing PT LOAD
    //  ===========================================
    auto start = std::chrono::high_resolution_clock::now();
    LOGI("Writing %zu PT_LOAD segments", task_ptr->for_each_vma_list().size());
    size_t vma_count = task_ptr->for_each_vma_list().size();
    for (const auto& vma_ptr : task_ptr->for_each_vma_list()) {
        phdr_pos += get_phdr_size();
        if (!write_pt_load(vma_ptr,phdr_pos,load_data_pos)){
            LOGW("Failed to write PT_LOAD for VMA [%#lx-%#lx]",
                 vma_ptr->vm_start, vma_ptr->vm_end);
            --vma_count;
            continue;
        }
        --vma_count;
        PRINT("Written page to core file. Remaining VMA count: %zu\n", vma_count);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    LOGD("time: %.6f s\n",elapsed.count());
    fclose(corefile);
    std::free(hdr_ptr);
    LOGI("Core file generation completed successfully");
}

int Core::vma_dump_size(std::shared_ptr<vma_struct> vma_ptr) {
    int filter_flags = FILTER_SANITIZER_SHADOW_VMA | FILTER_NON_READ_VMA;
    if (filter_flags & FILTER_SPECIAL_VMA) {
        if (vma_ptr->name.find("binder") != std::string::npos || vma_ptr->name.find("mali0") != std::string::npos){
            return 1;
        }
    }
    if (filter_flags & FILTER_FILE_VMA) {
        if (!vma_ptr->anon_vma && vma_ptr->vm_file){
            return 1;
        }
    }
    if (filter_flags & FILTER_SHARED_VMA) {
        if (vma_ptr->vm_flags & (VM_SHARED | VM_MAYSHARE)){
            return 1;
        }
    }
    if (filter_flags &  FILTER_SANITIZER_SHADOW_VMA) {
        if (vma_ptr->name.find("shadow") != std::string::npos || vma_ptr->name.find("hwasan") != std::string::npos){
            return 1;
        }
    }
    if (filter_flags & FILTER_NON_READ_VMA) {
        if (!(vma_ptr->vm_flags & VM_READ)
                && !(vma_ptr->vm_flags & VM_WRITE)
                && !(vma_ptr->vm_flags & VM_EXEC)){ // RX .text
                    return 1;
        }
    }
    return 0;
}

bool Core::parser_user_regset_view(void) {
    LOGD("Parsing user_regset_view: %s", user_view_var_name.c_str());
    ulong addr = csymbol_value(user_view_var_name);
    if(!addr){
        PRINT("ERROR: Can't found %s\n", user_view_var_name.c_str());
        return false;
    }
    void *buf = read_struct(addr,"user_regset_view");
    if (!buf) {
        LOGE("Failed to read user_regset_view structure");
        return false;
    }
    urv_ptr = std::make_shared<user_regset_view>();
    ulong name_addr = ULONG(buf + field_offset(user_regset_view,name));
    if (!is_kvaddr(name_addr)) {
        LOGE("Invalid kernel address for name: %#lx", name_addr);
        return false;
    }
    urv_ptr->name = read_cstring(name_addr, 32, "user_regset_view name");
    urv_ptr->n = UINT(buf + field_offset(user_regset_view,n));
    urv_ptr->e_flags = UINT(buf + field_offset(user_regset_view,e_flags));
    urv_ptr->e_machine = USHORT(buf + field_offset(user_regset_view,e_machine));
    urv_ptr->ei_osabi = UCHAR(buf + field_offset(user_regset_view,ei_osabi));
    LOGI("user_regset_view: name=%s cnt=%d e_flags=%d e_machine=%d ei_osabi=%d",
         urv_ptr->name.c_str(), urv_ptr->n, urv_ptr->e_flags,
         urv_ptr->e_machine, urv_ptr->ei_osabi);
    ulong regset_array_addr = ULONG(buf + field_offset(user_regset_view,regsets));
    FREEBUF(buf);
    for (size_t i = 0; i < urv_ptr->n; i++){
        ulong regsets_addr = regset_array_addr + i * struct_size(user_regset) + field_offset(user_regset,n);
        if (!is_kvaddr(regsets_addr)) continue;
        std::shared_ptr<user_regset> regset_ptr = std::make_shared<user_regset>();
        if(!read_struct(regsets_addr,regset_ptr.get(),sizeof(user_regset),"user_regset")){
            continue;
        }
        LOGD("user_regset[%zu]: core_note_type=%d cnt=%d size=%d",
             i, regset_ptr->core_note_type, regset_ptr->n, regset_ptr->size);
        urv_ptr->regsets.push_back(regset_ptr);
    }
    return true;
}

std::string Core::vma_flags_to_str(unsigned long flags) {
    std::string str(4, '-');
    if (flags & VM_READ) str[0] = 'r';
    if (flags & VM_WRITE) str[1] = 'w';
    if (flags & VM_EXEC) str[2] = 'x';
    if (flags & VM_SHARED) str[3] = 's';
    else str[3] = 'p';
    return str;
}

void Core::print_proc_mapping(){
    LOGD("Printing process mapping for pid %d", core_pid);
    tc = pid_to_context(core_pid);
    task_ptr = std::make_shared<UTask>(swap_ptr, tc->task);
    std::ostringstream oss;
    for (auto &vma_ptr : task_ptr->for_each_vma_list()){
        oss << std::left << "VMA:" << std::hex << vma_ptr->addr << " ["
            << std::hex << vma_ptr->vm_start
            << "-"
            << std::hex << vma_ptr->vm_end << "] "
            << vma_flags_to_str(vma_ptr->vm_flags) << " "
            << std::right << std::setw(16) << std::setfill('0') << vma_ptr->vm_flags << " "
            << std::right << std::hex << std::setw(8) << std::setfill('0') << vma_ptr->vm_pgoff << " "
            << vma_ptr->name << "\n";
    }
    PRINT("%s\n", oss.str().c_str());
    task_ptr.reset();
}

void Core::parser_thread_core_info() {
    LOGD("Parsing thread core info");
    int offset = field_offset(task_struct, thread_node);
    ulong signal_addr = read_pointer(tc->task + field_offset(task_struct, signal), "task_struct signal");
    ulong list_head = signal_addr + field_offset(signal_struct, thread_head);
    for(const auto& thread_addr: for_each_list(list_head, offset)){
        std::shared_ptr<elf_thread_info> thread_ptr = std::make_shared<elf_thread_info>();
        thread_ptr->task_addr = thread_addr;
        for(const auto& regset_ptr: urv_ptr->regsets){
            std::shared_ptr<memelfnote> note_ptr = std::make_shared<memelfnote>();
            LOGD("Processing regset: core_note_type=%#x", regset_ptr->core_note_type);
            switch (regset_ptr->core_note_type)
            {
            case NT_PRSTATUS:
                note_ptr->name = "CORE";
                note_ptr->type = NT_PRSTATUS;
                note_ptr->data = parser_prstatus(thread_addr,&note_ptr->datasz);
                thread_ptr->prstatus_ptr = note_ptr;
                break;
            case NT_PRFPREG:
                note_ptr->name = "CORE";
                note_ptr->type = NT_PRFPREG;
                note_ptr->data = parser_nt_prfpreg(thread_addr);
                note_ptr->datasz = regset_ptr->n * regset_ptr->size;
                break;
            case NT_ARM_VFP:
                note_ptr->name = "LINUX";
                note_ptr->type = NT_ARM_VFP;
                note_ptr->data = parser_nt_arm_vfp(thread_addr);
                note_ptr->datasz = regset_ptr->n * regset_ptr->size;
                break;
            case NT_ARM_TLS:
                note_ptr->name = "LINUX";
                note_ptr->type = NT_ARM_TLS;
                note_ptr->data = parser_nt_arm_tls(thread_addr);
                note_ptr->datasz = regset_ptr->n * regset_ptr->size;
                break;
            case NT_ARM_HW_BREAK:
                note_ptr->name = "LINUX";
                note_ptr->type = NT_ARM_HW_BREAK;
                note_ptr->data = parser_nt_arm_hw_break(thread_addr);
                note_ptr->datasz = regset_ptr->n * regset_ptr->size;
                break;
            case NT_ARM_HW_WATCH:
                note_ptr->name = "LINUX";
                note_ptr->type = NT_ARM_HW_WATCH;
                note_ptr->data = parser_nt_arm_hw_watch(thread_addr);
                note_ptr->datasz = regset_ptr->n * regset_ptr->size;
                break;
            case NT_ARM_SYSTEM_CALL:
                note_ptr->name = "LINUX";
                note_ptr->type = NT_ARM_SYSTEM_CALL;
                note_ptr->data = parser_nt_arm_system_call(thread_addr);
                note_ptr->datasz = regset_ptr->n * regset_ptr->size;
                break;
            case NT_ARM_SVE:
                note_ptr->name = "LINUX";
                note_ptr->type = NT_ARM_SVE;
                note_ptr->data = parser_nt_arm_sve(thread_addr);
                note_ptr->datasz = regset_ptr->n * regset_ptr->size;
                break;
            case NT_ARM_PAC_MASK:
                note_ptr->name = "LINUX";
                note_ptr->type = NT_ARM_PAC_MASK;
                note_ptr->data = parser_nt_arm_pac_mask(thread_addr);
                note_ptr->datasz = regset_ptr->n * regset_ptr->size;
                break;
            case NT_ARM_PAC_ENABLED_KEYS:
                note_ptr->name = "LINUX";
                note_ptr->type = NT_ARM_PAC_ENABLED_KEYS;
                note_ptr->data = parser_nt_arm_pac_enabled_keys(thread_addr);
                note_ptr->datasz = regset_ptr->n * regset_ptr->size;
                break;
            case NT_ARM_PACA_KEYS:
                note_ptr->name = "LINUX";
                note_ptr->type = NT_ARM_PACA_KEYS;
                note_ptr->data = parser_nt_arm_paca_keys(thread_addr);
                note_ptr->datasz = regset_ptr->n * regset_ptr->size;
                break;
            case NT_ARM_PACG_KEYS:
                note_ptr->name = "LINUX";
                note_ptr->type = NT_ARM_PACG_KEYS;
                note_ptr->data = parser_nt_arm_pacg_keys(thread_addr);
                note_ptr->datasz = regset_ptr->n * regset_ptr->size;
                break;
            case NT_ARM_TAGGED_ADDR_CTRL:
                note_ptr->name = "LINUX";
                note_ptr->type = NT_ARM_TAGGED_ADDR_CTRL;
                note_ptr->data = parser_nt_arm_tagged_addr_ctrl(thread_addr);
                note_ptr->datasz = regset_ptr->n * regset_ptr->size;
                break;
            default:
                break;
            }
            thread_ptr->note_list.push_back(note_ptr);
        }
        thread_list.push_back(thread_ptr);
    }
}

void Core::parser_auvx(){
    LOGD("Parsing AUXV");
    size_t auxv_size = field_size(mm_struct, saved_auxv);
    void* auxv_buf = task_ptr->read_auxv();
    if(!auxv_buf){
        LOGE("AUXV buffer is NULL");
        return;
    }
    auxv = std::make_shared<memelfnote>();
    auxv->name = "CORE";
    auxv->type = NT_AUXV;
    auxv->data = std::malloc(auxv_size);
    memcpy(auxv->data, auxv_buf, auxv_size);
    FREEBUF(auxv_buf);
    auxv->datasz = auxv_size;
    LOGD("NT_AUXV:\n");
    LOGD("\n%s", hexdump(0x1000, (char*)auxv->data, auxv_size).c_str());
}

void Core::parser_nt_file() {
    LOGD("Parsing NT_FILE");
    std::unique_ptr<std::vector<char>> data_ptr = std::make_unique<std::vector<char>>();
    size_t files_count = 0;
    int size_data = BITS64() ? (task_ptr->is_compat() ? 4 : sizeof(long)) : sizeof(long);
    size_t total_vma_size = 0;
    size_t total_filename_size = 0;
    for (const auto& vma : task_ptr->for_each_file_vma()) {
        total_vma_size += 3 * size_data;
        if (!Core::symbols_path.empty()){
            std::string filepath;
            if(SearchFile(Core::symbols_path, vma->name, filepath)){
                lib_list.insert(vma->name);
                LOGD("Found symbol for library: %s", vma->name.c_str());
            }
        }
        vma->name += '\0';
        total_filename_size += vma->name.size();
        files_count++;
    }
    LOGI("NT_FILE: %zu files, total_vma_size=%zu, total_filename_size=%zu",
         files_count, total_vma_size, total_filename_size);
    data_ptr->reserve(2 * size_data + total_vma_size + total_filename_size);
    data_ptr->insert(data_ptr->end(), reinterpret_cast<const char*>(&files_count), reinterpret_cast<const char*>(&files_count) + size_data);
    data_ptr->insert(data_ptr->end(), reinterpret_cast<const char*>(&page_size), reinterpret_cast<const char*>(&page_size) + size_data);
    for (const auto& vma : task_ptr->for_each_file_vma()) {
        data_ptr->insert(data_ptr->end(), reinterpret_cast<const char*>(&vma->vm_start), reinterpret_cast<const char*>(&vma->vm_start) + size_data);
        data_ptr->insert(data_ptr->end(), reinterpret_cast<const char*>(&vma->vm_end), reinterpret_cast<const char*>(&vma->vm_end) + size_data);
        data_ptr->insert(data_ptr->end(), reinterpret_cast<const char*>(&vma->vm_pgoff), reinterpret_cast<const char*>(&vma->vm_pgoff) + size_data);
    }
    for (const auto& vma : task_ptr->for_each_file_vma()) {
        data_ptr->insert(data_ptr->end(), vma->name.begin(), vma->name.end());
    }
    LOGD("NT_FILE:\n");
    LOGD("\n%s", hexdump(0x1000, data_ptr->data(), data_ptr->size()).c_str());
    files = std::make_shared<memelfnote>();
    files->name = "CORE";
    files->type = NT_FILE;
    files->data = std::malloc(data_ptr->size());
    memcpy(files->data, data_ptr->data(), data_ptr->size());
    files->datasz = data_ptr->size();
}

void Core::dump_align(std::streampos position, std::streamsize align) {
    if (align <= 0 || (align & (align - 1))) {
        return;
    }
    std::streamsize mod = position & (align - 1);
    if (mod > 0) {
        std::streamsize padding_size = align - mod;
        std::vector<char> padding(padding_size, 0);
        if (fwrite(padding.data(), 1, padding_size, corefile) != static_cast<size_t>(padding_size)) {
            LOGE("Error writing padding to core file");
        }
    }
}

int Core::task_pid_nr_ns(ulong task_addr, long type, ulong ns_addr){
    if(!ns_addr){
        ns_addr = ns_of_pid(read_pointer(task_addr + field_offset(task_struct, thread_pid), "task_struct thread_pid"));
    }
    if (pid_alive(task_addr)){
        return pid_nr_ns(task_pid_ptr(task_addr, type), ns_addr);
    }
    return 0;
}

int Core::pid_nr_ns(ulong pids_addr, ulong pid_ns_addr) {
    if (!is_kvaddr(pids_addr)) {
        return 0;
    }
    uint ns_level = read_uint(pid_ns_addr + field_offset(pid_namespace, level), "pid_namespace level");
    uint pid_level = read_uint(pids_addr + field_offset(pid, level), "pid level");
    if (ns_level > pid_level) {
        return 0;
    }
    ulong upid_addr = pids_addr + field_offset(pid, numbers) + struct_size(upid) * ns_level;
    ulong ns_addr = read_pointer(upid_addr + field_offset(upid, ns), "upid ns");
    if (ns_addr == pid_ns_addr) {
        return read_int(upid_addr + field_offset(upid, nr), "upid nr");
    }
    return 0;
}

int Core::pid_alive(ulong task_addr) {
    ulong thread_pid_addr = read_pointer(task_addr + field_offset(task_struct, thread_pid), "task_struct thread_pid");
    return thread_pid_addr > 0;
}

ulong Core::ns_of_pid(ulong thread_pid_addr) {
    if (!is_kvaddr(thread_pid_addr)) {
        return 0;
    }

    uint level = read_uint(thread_pid_addr + field_offset(pid, level), "task_pid_nr_ns pid level");
    ulong upid_addr = thread_pid_addr + field_offset(pid, numbers) + struct_size(upid) * level;
    return read_pointer(upid_addr + field_offset(upid, ns), "task_pid_nr_ns upid ns");
}

ulong Core::task_pid_ptr(ulong task_addr, long type) {
    if (type == read_enum_val("PIDTYPE_PID")) {
        return read_pointer(task_addr + field_offset(task_struct, thread_pid), "task_struct thread_pid");
    } else {
        ulong signal_addr = read_pointer(task_addr + field_offset(task_struct, signal), "task_struct signal");
        return read_pointer(signal_addr + field_offset(signal_struct, pids) + type * sizeof(void*), "signal_struct pids");
    }
}

void Core::write_phdr(size_t p_type, size_t p_offset, size_t p_vaddr, size_t p_filesz, size_t p_memsz, size_t p_flags, size_t p_align) {
    size_t data_size = BITS64() && !task_ptr->is_compat() ? sizeof(Elf64_Phdr) : sizeof(Elf32_Phdr);
    std::unique_ptr<char[]> phdr(new char[data_size]);
    BZERO(phdr.get(), data_size);

    auto set_phdr = [&](auto* elf_phdr) {
        elf_phdr->p_type = p_type;
        elf_phdr->p_offset = p_offset;
        elf_phdr->p_vaddr = p_vaddr;
        elf_phdr->p_paddr = 0;
        elf_phdr->p_filesz = p_filesz;
        elf_phdr->p_memsz = p_memsz;
        elf_phdr->p_flags = p_flags;
        elf_phdr->p_align = p_align;
        fwrite(elf_phdr, data_size, 1, corefile);
    };

    if (BITS64() && !task_ptr->is_compat()) {
        Elf64_Phdr* elf_phdr = reinterpret_cast<Elf64_Phdr*>(phdr.get());
        set_phdr(elf_phdr);
    } else {
        Elf32_Phdr* elf_phdr = reinterpret_cast<Elf32_Phdr*>(phdr.get());
        set_phdr(elf_phdr);
    }
}

int Core::notesize(std::shared_ptr<memelfnote> note_ptr){
    size_t total_size = BITS64() && !task_ptr->is_compat() ? sizeof(Elf64_Nhdr) : sizeof(Elf32_Nhdr);
    total_size += roundup(note_ptr->name.size() + 1,4);
    total_size += roundup(note_ptr->datasz,4);
    return total_size;
}

void Core::writenote(std::shared_ptr<memelfnote> note_ptr) {
    size_t data_size = BITS64() && !task_ptr->is_compat() ? sizeof(Elf64_Nhdr) : sizeof(Elf32_Nhdr);
    std::unique_ptr<char[]> note(new char[data_size]);
    BZERO(note.get(), data_size);

    auto write_note = [&](auto* elf_note) {
        elf_note->n_namesz = note_ptr->name.size() + 1;
        elf_note->n_descsz = note_ptr->datasz;
        elf_note->n_type = note_ptr->type;

        fwrite(elf_note, sizeof(*elf_note), 1, corefile);
        fwrite(note_ptr->name.c_str(), elf_note->n_namesz, 1, corefile);
        dump_align(ftell(corefile), 4);
        fwrite(reinterpret_cast<const char*>(note_ptr->data), note_ptr->datasz, 1, corefile);
        std::free(note_ptr->data);
        dump_align(ftell(corefile), 4);
    };

    if (BITS64() && !task_ptr->is_compat()) {
        write_note(reinterpret_cast<Elf64_Nhdr*>(note.get()));
    } else {
        write_note(reinterpret_cast<Elf32_Nhdr*>(note.get()));
    }
}

void* Core::fill_elf_header(int type, int phnum, size_t& hdr_size) {
    hdr_size = BITS64() && !task_ptr->is_compat() ? sizeof(Elf64_Ehdr) : sizeof(Elf32_Ehdr);
    size_t phdr_size = BITS64() && !task_ptr->is_compat() ? sizeof(Elf64_Phdr) : sizeof(Elf32_Phdr);
    void* hdr = std::malloc(hdr_size);
    BZERO(hdr, hdr_size);
    auto set_elf_header = [&](auto* elf_hdr) {
        snprintf((char *)elf_hdr->e_ident, 5, ELFMAG);
        elf_hdr->e_ident[EI_CLASS] = elf_class;
        elf_hdr->e_ident[EI_DATA] = ELFDATA2LSB;
        elf_hdr->e_ident[EI_VERSION] = EV_CURRENT;
        elf_hdr->e_ident[EI_OSABI] = urv_ptr->ei_osabi;
        elf_hdr->e_type = type;
        elf_hdr->e_machine = urv_ptr->e_machine;
        elf_hdr->e_version = EV_CURRENT;
        elf_hdr->e_phoff = hdr_size;
        elf_hdr->e_flags = urv_ptr->e_flags;
        elf_hdr->e_ehsize = hdr_size;
        elf_hdr->e_phentsize = phdr_size;
        elf_hdr->e_phnum = phnum;
        LOGD("elf_hdr:\n");
        LOGD("\n%s", hexdump(0, (char*)elf_hdr, hdr_size).c_str());
    };
    if (BITS64() && !task_ptr->is_compat()) {
        Elf64_Ehdr* elf_hdr = reinterpret_cast<Elf64_Ehdr*>(hdr);
        set_elf_header(elf_hdr);
    } else {
        Elf32_Ehdr* elf_hdr = reinterpret_cast<Elf32_Ehdr*>(hdr);
        set_elf_header(elf_hdr);
    }
    return hdr;
}

int Core::get_phdr_start() {
    return (BITS64() && !task_ptr->is_compat()) ?
           reinterpret_cast<Elf64_Ehdr*>(hdr_ptr)->e_phoff :
           reinterpret_cast<Elf32_Ehdr*>(hdr_ptr)->e_phoff;
}

int Core::get_pt_note_data_start() {
    return (BITS64() && !task_ptr->is_compat()) ?
           reinterpret_cast<Elf64_Ehdr*>(hdr_ptr)->e_ehsize + (reinterpret_cast<Elf64_Ehdr*>(hdr_ptr)->e_phnum * reinterpret_cast<Elf64_Ehdr*>(hdr_ptr)->e_phentsize) :
           reinterpret_cast<Elf32_Ehdr*>(hdr_ptr)->e_ehsize + (reinterpret_cast<Elf32_Ehdr*>(hdr_ptr)->e_phnum * reinterpret_cast<Elf32_Ehdr*>(hdr_ptr)->e_phentsize);
}

int Core::get_phdr_size() {
    return (BITS64() && !task_ptr->is_compat()) ?
           reinterpret_cast<Elf64_Ehdr*>(hdr_ptr)->e_phentsize :
           reinterpret_cast<Elf32_Ehdr*>(hdr_ptr)->e_phentsize;
}

void* Core::map_elf_file(std::string filepath, size_t& len){
    int fd = open(filepath.c_str(), O_RDONLY);
    if (fd < 0) {
        return nullptr;
    }
    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        return nullptr;
    }
    void* map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        LOGE("mmap failed \n");
        close(fd);
        return nullptr;
    }
    len = st.st_size;
    close(fd);
    return map;
}

bool Core::check_elf_file(void* map){
    auto check_header  = [&](auto* elf_hdr) {
        if (memcmp(elf_hdr->e_ident, ELFMAG, SELFMAG) != 0) {
            LOGE("Not an ELF file\n");
            return false;
        }
        if (elf_hdr->e_type != ET_DYN) {
            LOGE("Not a shared object (ET_DYN) file\n");
            return false;
        }
        return true;
    };
    if (BITS64() && !task_ptr->is_compat()) {
        Elf64_Ehdr* elf_hdr = reinterpret_cast<Elf64_Ehdr*>(map);
        return check_header(elf_hdr);
    } else {
        Elf32_Ehdr* elf_hdr = reinterpret_cast<Elf32_Ehdr*>(map);
        return check_header(elf_hdr);
    }
}

std::shared_ptr<symbol_info> Core::read_elf_file(std::string file_path){
    size_t map_size = 0;
    std::shared_ptr<symbol_info> sym_info = std::make_shared<symbol_info>();
    void* map = map_elf_file(file_path, map_size);
    if (map == nullptr) {
        return nullptr;
    }
    if (!check_elf_file(map)) {
        munmap(map, map_size);
        return nullptr;
    }
    auto process_phdrs = [&](auto* elf_hdr, auto* phdr_table) {
        for (size_t i = 0; i < elf_hdr->e_phnum; ++i) {
            auto& phdr = phdr_table[i];
            if (phdr.p_type == PT_DYNAMIC) {
                sym_info->dynamic_offset = phdr.p_offset;
                sym_info->dynamic_vaddr = phdr.p_vaddr;
            }
            if (phdr.p_type == PT_PHDR) {
                sym_info->phdr_offset = phdr.p_offset;
                sym_info->phdr_vaddr = phdr.p_vaddr;
            }
        }
    };
    if (BITS64() && !task_ptr->is_compat()) {
        Elf64_Ehdr* elf_hdr = reinterpret_cast<Elf64_Ehdr*>(map);
        Elf64_Phdr* phdr_table = reinterpret_cast<Elf64_Phdr*>(reinterpret_cast<char*>(map) + elf_hdr->e_phoff);
        process_phdrs(elf_hdr, phdr_table);
    } else {
        Elf32_Ehdr* elf_hdr = reinterpret_cast<Elf32_Ehdr*>(map);
        Elf32_Phdr* phdr_table = reinterpret_cast<Elf32_Phdr*>(reinterpret_cast<char*>(map) + elf_hdr->e_phoff);
        process_phdrs(elf_hdr, phdr_table);
    }
    sym_info->map_addr = map;
    sym_info->map_size = map_size;
    return sym_info;
}

/*
   ---->+-----------------+<-----               [FAKE_AUXV_PHDR]
     ^  |     ET_DYN      |
     |  +-----------------+<----- PT_PHDR
 page|  |     PT_PHDR     |
     |  +-----------------+
     v  |    PT_DYNAMIC   |
   ---->+-----------------+<----- PT_DYNAMIC    [FAKE_AUXV_PHDR + page]
     ^  |    dynamic_t    |
     |  +-----------------+<----- dynamic_t.value
 page|  |    r_debug_t    |
     v  |                 |
   ---->+-----------------+<----- debug_t.map   [FAKE_AUXV_PHDR + page + page]
        |                 |
        |    link_map     |
        |                 |
        |                 |
        +-----------------+<-----               [FAKE_AUXV_PHDR + page + page + linkmap_size]
        |                 |
        |    strtab       |
        |                 |
        +-----------------+
*/
void Core::write_fake_data(size_t &data_pos, size_t phdr_pos){
    if (Core::symbols_path.empty()){
        return;
    }
    if (fseek(corefile, data_pos, SEEK_SET) != 0) {
        fclose(corefile);
        return;
    }
    size_t fake_header_offset = data_pos;
    size_t hdr_size = 0;
    void* hdr = fill_elf_header(ET_DYN, 2, hdr_size);
    fwrite(hdr, hdr_size, 1, corefile);
    LOGD("fake elf_hdr:\n");
    LOGD("\n%s", hexdump(0, (char*)hdr, hdr_size).c_str());
    std::free(hdr);
    /*
       we can see the comment as below in GDB.
       at_phdr is real address in memory. pt_phdr is what pheader says it is.
       Relocation offset is the difference between the two.
       sect_addr = sect_addr + (at_phdr - pt_phdr);
    */
    write_phdr(PT_PHDR, hdr_size, hdr_size, 0, page_size, PF_R, 0x8);
    write_phdr(PT_DYNAMIC, page_size, page_size, page_size, page_size, PF_R | PF_W,0x8);
    dump_align(ftell(corefile), page_size);

    data_pos += page_size;

    dynamic_t dyn;
    BZERO(&dyn, sizeof(dynamic_t));
    dyn.type = DT_DEBUG;
    dyn.value = FAKE_AUXV_PHDR + page_size /* first page */ + sizeof(dynamic_t);
    LOGD("fake DYNAMINC:\n");
    LOGD("\n%s", hexdump(0, (char*)&dyn, sizeof(dynamic_t)).c_str());
    fwrite(&dyn, sizeof(dynamic_t), 1, corefile);
    r_debug_t r_debug;
    BZERO(&r_debug, sizeof(r_debug_t));
    r_debug.version = 1;
    r_debug.map = FAKE_AUXV_PHDR + page_size /* first page */ + page_size /* second page */;
    LOGD("fake DT_DEBUG:\n");
    LOGD("\n%s", hexdump(0, (char*)&r_debug, sizeof(r_debug_t)).c_str());
    fwrite(&r_debug, sizeof(r_debug_t), 1, corefile);
    dump_align(ftell(corefile), page_size);

    data_pos += page_size;

    size_t cur_linkmap_vaddr = r_debug.map;
    // size_t linkmap_size = roundup(sizeof(linkmap_t) * (lib_list.size() + 1/* FAKECORE */), page_size);
    size_t linkmap_size = sizeof(linkmap_t) * (lib_list.size() + 1/* FAKECORE */);
    std::unique_ptr<char[]> linkmap_buf(new char[linkmap_size]);
    BZERO(linkmap_buf.get(), linkmap_size);

    data_pos += linkmap_size;

    size_t cur_strtab_vaddr = r_debug.map + linkmap_size;
    std::string name = "[FAKECORE]";
    name += '\0';
    std::stringstream strtab;
    strtab << name;

    linkmap_t* linkmap = (linkmap_t*)linkmap_buf.get();
    linkmap->addr = FAKE_AUXV_PHDR;
    linkmap->name = cur_strtab_vaddr;
    linkmap->ld = 0x0;
    linkmap->next = cur_linkmap_vaddr + sizeof(linkmap_t);
    linkmap->prev = 0x0;
    cur_strtab_vaddr += name.length();
    for (const auto& name : lib_list) {
        std::string filepath;
        if(!SearchFile(Core::symbols_path, name, filepath)){
            continue;
        }
        // get the elf symbol info, including mmap_addr, phdr, pt_dynamic
        std::shared_ptr<symbol_info> sym_ptr = read_elf_file(filepath);
        if(sym_ptr == nullptr){
            continue;
        }
        std::shared_ptr<vma_struct> phdr_vma_ptr = task_ptr->get_phdr_vma(name);
        if(phdr_vma_ptr == nullptr){
            continue;
        }
        linkmap++;
        // the base addr of library
        linkmap->addr = phdr_vma_ptr->vm_start;
        if (sym_ptr->phdr_offset != sym_ptr->phdr_vaddr){
            linkmap->addr += (sym_ptr->phdr_offset - sym_ptr->phdr_vaddr);
        }
        // the addr of dynamic
        linkmap->ld = linkmap->addr + sym_ptr->dynamic_vaddr;
        LOGD("linkmap->addr:%#lx linkmap->ld:%#lx %s",
             linkmap->addr, linkmap->ld, phdr_vma_ptr->name.c_str());
        linkmap->name = cur_strtab_vaddr;
        linkmap->prev = cur_linkmap_vaddr;
        cur_linkmap_vaddr = cur_linkmap_vaddr + sizeof(linkmap_t);
        linkmap->next = cur_linkmap_vaddr + sizeof(linkmap_t);

        strtab << phdr_vma_ptr->name;
        cur_strtab_vaddr += phdr_vma_ptr->name.length();
        munmap(sym_ptr->map_addr, sym_ptr->map_size);
    }
    linkmap->next = 0;
    // strtab_len = roundup(strtab.str().size(), page_size);
    data_pos += strtab.str().size();
    LOGD("LINKMAP:\n");
    LOGD("\n%s", hexdump(FAKE_AUXV_PHDR + 2 * page_size, (char*)linkmap_buf.get(), linkmap_size).c_str());

    LOGD("STRTAB:\n");
    LOGD("\n%s", hexdump(FAKE_AUXV_PHDR + 2 * page_size + linkmap_size, (char*)strtab.str().c_str(), strtab.str().size()).c_str());
    fwrite(linkmap_buf.get(), linkmap_size, 1, corefile);
    fwrite(strtab.str().c_str(), strtab.str().size(), 1, corefile);

    dump_align(ftell(corefile), page_size);
    //  ===========================================
    //  Writing Fake LOAD Header
    //  ===========================================
    /*
        roundup(linkmap_size + strtab, page_size),
    */
    size_t p_filesz = (page_size                 /* FAKE_PHDR */
                    + page_size                  /* FAKE_DYNAMIC */
                    + linkmap_size               /* FAKE_LINK_MAP */
                    + strtab.str().size());      /* FAKE_STRTAB */
    if (fseek(corefile, phdr_pos, SEEK_SET) != 0) {
        fclose(corefile);
        return;
    }
    write_phdr(PT_LOAD, fake_header_offset, FAKE_AUXV_PHDR, p_filesz, p_filesz, PF_R | PF_W, page_size);
}

void Core::print_linkmap(){
#if defined(ARM)
#define Elf(type) Elf32_##type
#elif defined(ARM64)
#define Elf(type) Elf64_##type
#endif
    tc = pid_to_context(core_pid);
    if (!tc) {
        LOGE("pid_to_context failed for pid %d", core_pid);
        return;
    }
    task_ptr = std::make_shared<UTask>(swap_ptr, tc->task);
    if (BITS64() && task_ptr->is_compat()) {
        /*
         * If you want to enable compatibility, you need to add the following code:
         * Every struct should use 32-bit(uint32_t and Elf32_type), including all parameters passed in swapinfo.
         */
        LOGW("Compat mode not supported for linkmap feature");
        return;
    }
    size_t at_phdr = task_ptr->get_auxv(AT_PHDR);
    size_t at_phnum = task_ptr->get_auxv(AT_PHNUM);
    size_t at_phent = task_ptr->get_auxv(AT_PHENT);
    size_t at_entry = task_ptr->get_auxv(AT_ENTRY);
    LOGD("AUXV values: AT_PHDR=%#lx AT_PHNUM=%zu AT_PHENT=%zu AT_ENTRY=%#lx",
         at_phdr, at_phnum, at_phent, at_entry);
    parser_exec_name(task_ptr->get_auxv(AT_EXECFN));
    std::string exec_file_path;
    if(!SearchFile(Core::symbols_path, exe_name, exec_file_path)){
        LOGE("Cannot find executable: %s", exe_name.c_str());
        return;
    }
    LOGI("Found executable: %s", exec_file_path.c_str());
    int fd = open(exec_file_path.c_str(), O_RDONLY);
    if (fd == -1) {
        LOGE("Failed to open executable file: %s", exec_file_path.c_str());
        return;
    }
    struct stat st;
    if (fstat(fd, &st) == -1) {
        LOGE("Failed to fstat executable file");
        close(fd);
        return;
    }
    void* exec_map = mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (exec_map == MAP_FAILED) {
        LOGE("Failed to mmap executable file");
        close(fd);
        return;
    }
    Elf(Ehdr)* ehdr = reinterpret_cast<Elf(Ehdr)*>(exec_map);
    Elf(Phdr)* phdr = reinterpret_cast<Elf(Phdr)*>((static_cast<char*>(exec_map) + ehdr->e_phoff));
    std::vector<char> core_phdr = task_ptr->read_data(at_phdr,at_phnum * at_phent);
    if(core_phdr.size() == 0){
        LOGE("Failed to read core PHDR data");
        munmap(exec_map, st.st_size);
        close(fd);
        return;
    }
    ulong exec_pt_Load_x_vaddr = ehdr->e_entry;
    ulong exec_dynamic_vaddr = 0;
    Elf(Dyn)* dyn = nullptr;
    bool flags = true;
    for (size_t i = 0; i < at_phnum; ++i) {
        if (phdr[i].p_type == PT_DYNAMIC) {
            dyn = reinterpret_cast<Elf(Dyn)*>((static_cast<char*>(exec_map) + phdr[i].p_offset));
            exec_dynamic_vaddr = phdr[i].p_vaddr;
            LOGD("Found PT_DYNAMIC at vaddr=%#lx", exec_dynamic_vaddr);
        }
        // see the svr4_exec_displacement in GDB.
        if (memcmp(&phdr[i], &core_phdr[i * sizeof(Elf(Phdr))], sizeof(Elf(Phdr))) != 0) {
            if(flags){
                LOGW("PHDR mismatch detected: core phnum=%zu, exec phnum=%d",
                     at_phnum, ehdr->e_phnum);
                flags = false;
            }
        }
    }
    ulong linkmap_addr = 0;
    ulong exec_displacement = at_entry - exec_pt_Load_x_vaddr;
    LOGD("exec_displacement=%#lx (at_entry=%#lx - exec_pt_Load_x_vaddr=%#lx)",
         exec_displacement, at_entry, exec_pt_Load_x_vaddr);
    if (dyn) {
        for (int count = 0;; ++count) {
            if (dyn[count].d_tag == DT_NULL) {
                break;
            }
            if (dyn[count].d_tag == DT_DEBUG) {
                ulong dyn_ptr = exec_displacement + exec_dynamic_vaddr + count * sizeof(Elf(Dyn)) + sizeof(void*) /* 8 & 4*/;
                linkmap_addr = task_ptr->uread_ulong(dyn_ptr);
                LOGD("Found DT_DEBUG: dyn_ptr=%#lx, linkmap_addr=%#lx", dyn_ptr, linkmap_addr);
            }
        }
    }
    if (!is_uvaddr(linkmap_addr, tc)) {
        LOGE("Invalid linkmap address: %#lx", linkmap_addr);
        munmap(exec_map, st.st_size);
        close(fd);
        return;
    }
    linkmap_addr += sizeof(void*);
    linkmap_addr = task_ptr->uread_ulong(linkmap_addr);
    LOGD("First linkmap address: %#lx", linkmap_addr);
    std::vector<char> lp = task_ptr->read_data(linkmap_addr,sizeof(linkmap_t));
    if(lp.size() == 0){
        LOGE("Failed to read linkmap data");
        munmap(exec_map, st.st_size);
        close(fd);
        return;
    }
    linkmap_t* linkmap = reinterpret_cast<linkmap_t*>(lp.data());
    std::ostringstream oss;
    oss << std::left
        << std::setw(20)  << "addr"
        << std::setw(20) << "ld"
        << std::setw(20) << "next"
        << std::setw(20) << "prev"
        << std::setw(20) << "name" << "\n";
    oss << std::hex;
    oss << std::setw(20) << linkmap->addr
        << std::setw(20) << linkmap->ld
        << std::setw(20) << linkmap->next
        << std::setw(20) << linkmap->prev
        << std::setw(20) << task_ptr->uread_cstring(linkmap->name, 128)
        << "\n";
    while (linkmap->next) {
        ulong linkmap_next = linkmap->next;
        if (!is_uvaddr(linkmap_next, tc)) {
            LOGE("Invalid linkmap next address: %#lx", linkmap_next);
            break;
        }
        std::vector<char> lp = task_ptr->read_data(linkmap_next,sizeof(linkmap_t));
        if(lp.size() == 0){
            LOGE("Failed to read linkmap data at %#lx", linkmap_next);
            break;
        }
        linkmap = reinterpret_cast<linkmap_t*>(lp.data());
        oss << std::setw(20) << linkmap->addr
            << std::setw(20) << linkmap->ld
            << std::setw(20) << linkmap->next
            << std::setw(20) << linkmap->prev
            << std::setw(20) << task_ptr->uread_cstring(linkmap->name, 128)
            << "\n";
    }
    PRINT("%s", oss.str().c_str());
    munmap(exec_map, st.st_size);
    close(fd);
    task_ptr.reset();
}
#pragma GCC diagnostic pop