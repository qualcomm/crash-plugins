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

#include "utask.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

/**
 * Main command execution (not used for UTask)
 */
void UTask::cmd_main(void) {

}

/**
 * Initialize command metadata (not used for UTask)
 */
void UTask::init_command(void) {

}

/**
 * Constructor with PID
 * @param swap Shared pointer to Swapinfo instance
 * @param pid Process ID to analyze
 */
UTask::UTask(std::shared_ptr<Swapinfo> swap, int pid): swap_ptr(swap){
    // Get task context from PID
    tc = pid_to_context(pid);
    if (!tc) {
        LOGE("Failed to get task context for PID: %d\n", pid);
        return;
    }
    LOGD("Task context obtained, task address: %#lx\n", tc->task);
    // Delegate to task address constructor
    UTask(swap, tc->task);
}

/**
 * Constructor with task address
 * @param swap Shared pointer to Swapinfo instance
 * @param addr Task structure address in kernel memory
 */
UTask::UTask(std::shared_ptr<Swapinfo> swap, ulong addr): swap_ptr(swap){
    // Get task context from task address
    tc = task_to_context(addr);
    if (!tc) {
        LOGE("Failed to get task context for address: %#lx\n", addr);
        return;
    }
    LOGD("Task context obtained, PID: %d, comm: %s\n", tc->pid, tc->comm);
    // Initialize structure field offsets
    init_offset();

    // Initialize memory management structure
    init_mm_struct();

    // Initialize auxiliary vector
    init_auxv();

    // Initialize virtual memory areas
    init_vma();

    // Get task file descriptors
    task_files = for_each_task_files(tc);
    LOGD("Found %zu file descriptors\n", task_files.size());

    // Calculate min/max read-write VMA addresses for validation
    for (const auto& vma_ptr : for_each_anon_vma()) {
        min_rw_vma_addr = std::min(min_rw_vma_addr,vma_ptr->vm_start);
        max_rw_vma_addr = std::max(max_rw_vma_addr,vma_ptr->vm_end);
    }
    LOGD("VMA address range: [%#lx - %#lx]\n", min_rw_vma_addr, max_rw_vma_addr);
}

void UTask::init_offset(void) {
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
    field_init(task_struct, thread_info);
    field_init(mm_struct, saved_auxv);
    field_init(mm_struct, mm_count);
    field_init(mm_struct, start_code);
    field_init(mm_struct, end_code);
    field_init(mm_struct, start_data);
    field_init(mm_struct, end_data);
    field_init(mm_struct, start_brk);
    field_init(mm_struct, brk);
    field_init(mm_struct, start_stack);
    field_init(mm_struct, env_start);
    field_init(mm_struct, env_end);
    field_init(mm_struct, flags);
    field_init(mm_struct, pgd);
    field_init(mm_struct, saved_auxv);
    field_init(cred, uid);
    field_init(cred, gid);
    field_init(vm_area_struct, vm_mm);
    field_init(vm_area_struct, vm_start);
    field_init(vm_area_struct, vm_end);
    field_init(vm_area_struct, vm_pgoff);
    field_init(vm_area_struct, anon_name);
    field_init(vm_area_struct, anon_vma);
    field_init(vm_area_struct, vm_flags);
    field_init(vm_area_struct, vm_file);
    field_init(vm_area_struct, detached);
    field_init(file, f_vfsmnt);
    field_init(file, f_dentry);
    field_init(file, f_path);
    field_init(file, f_mapping);
    field_init(file, f_inode);
    field_init(task_struct,files);
    field_init(fdtable,max_fds);
    field_init(fdtable,fd);
    field_init(files_struct,fdt);
    field_init(path, dentry);
    field_init(path, mnt);
    field_init(anon_vma_name, name);
    field_init(thread_info,flags);
}

void UTask::init_mm_struct() {
    void *buf = read_struct(tc->mm_struct,"mm_struct");
    if (!buf) return;
    mm_ptr = std::make_shared<mm_struct>();
    mm_ptr->pgd = ULONG(buf + field_offset(mm_struct,pgd));
    mm_ptr->mm_count = ULONG(buf + field_offset(mm_struct,mm_count));
    mm_ptr->start_code = ULONG(buf + field_offset(mm_struct,start_code));
    mm_ptr->end_code = ULONG(buf + field_offset(mm_struct,end_code));
    mm_ptr->start_data = ULONG(buf + field_offset(mm_struct,start_data));
    mm_ptr->end_data = ULONG(buf + field_offset(mm_struct,end_data));
    mm_ptr->start_brk = ULONG(buf + field_offset(mm_struct,start_brk));
    mm_ptr->brk = ULONG(buf + field_offset(mm_struct,brk));
    mm_ptr->start_stack = ULONG(buf + field_offset(mm_struct,start_stack));
    mm_ptr->arg_start = ULONG(buf + field_offset(mm_struct,arg_start));
    mm_ptr->arg_end = ULONG(buf + field_offset(mm_struct,arg_end));
    mm_ptr->env_start = ULONG(buf + field_offset(mm_struct,env_start));
    mm_ptr->env_end = ULONG(buf + field_offset(mm_struct,env_end));
    mm_ptr->flags = ULONG(buf + field_offset(mm_struct,flags));
    FREEBUF(buf);
    fill_thread_info(tc->thread_info);
    if (BITS64() && field_offset(thread_info, flags) != -1){
        ulong thread_info_flags = ULONG(tt->thread_info + field_offset(thread_info, flags));
        if(thread_info_flags & (1 << 22)){
            compat = true;
        }
    }
    pointer_size = (BITS64() && !is_compat()) ? 8 : 4;
    if (BITS64() && !is_compat()){
        std::string config = get_config_val("CONFIG_ARM64_VA_BITS");
        int va_bits = std::stoi(config);
        vaddr_mask = GENMASK_ULL((va_bits ? va_bits : 39) - 1, 0);
    }else{
        vaddr_mask = GENMASK_ULL(32 - 1, 0);
    }
}

bool UTask::is_compat(){
    return compat;
}

void UTask::init_vma(){
    char buf[BUFSIZE];
    int ANON_BUFSIZE = 1024;
    char *file_buf = nullptr;
    std::shared_ptr<file_vma> last_file_vma_ptr;
    for(const auto& vma_addr : for_each_vma(tc->task)){
        void *vma_buf = read_struct(vma_addr, "vm_area_struct");
        if (!vma_buf) {
            continue;
        }
        std::shared_ptr<vma_struct> vma_ptr = std::make_shared<vma_struct>();
        vma_ptr->addr = vma_addr;
        vma_ptr->vm_start = ULONG(vma_buf + field_offset(vm_area_struct, vm_start));
        vma_ptr->vm_end = ULONG(vma_buf + field_offset(vm_area_struct, vm_end));
        vma_ptr->vm_size = vma_ptr->vm_end - vma_ptr->vm_start;
        vma_ptr->vm_file = ULONG(vma_buf + field_offset(vm_area_struct, vm_file));
        vma_ptr->vm_flags = ULONG(vma_buf + field_offset(vm_area_struct, vm_flags));
        vma_ptr->vm_pgoff = ULONG(vma_buf + field_offset(vm_area_struct, vm_pgoff));
        vma_ptr->anon_name = ULONG(vma_buf + field_offset(vm_area_struct, anon_name));
        vma_ptr->anon_vma = ULONG(vma_buf + field_offset(vm_area_struct, anon_vma));
        FREEBUF(vma_buf);
        if (is_kvaddr(vma_ptr->vm_file)){ //file vma
            file_buf = fill_file_cache(vma_ptr->vm_file);
            ulong dentry = ULONG(file_buf + field_offset(file, f_path) + field_offset(path, dentry));
            if(is_kvaddr(dentry)){
                if (field_offset(file, f_path) != -1 && field_offset(path, dentry) != -1 && field_offset(path, mnt) != -1) {
                    ulong vfsmnt = ULONG(file_buf + field_offset(file, f_path) + field_offset(path, mnt));
                    get_pathname(dentry, buf, BUFSIZE, 1, vfsmnt);
                } else {
                    get_pathname(dentry, buf, BUFSIZE, 1, 0);
                }
                vma_ptr->name = buf;
                std::shared_ptr<file_vma> file_ptr;
                if (file_map.find(vma_ptr->name) != file_map.end()) { //exists
                    file_ptr = file_map[vma_ptr->name];
                } else {
                    file_ptr = std::make_shared<file_vma>();
                    file_map[vma_ptr->name] = file_ptr;
                }
                last_file_vma_ptr = file_ptr;
                if ((vma_ptr->vm_flags & VM_READ) && (vma_ptr->vm_flags & VM_EXEC)) {
                    file_ptr->text = vma_ptr;
                }else{
                    file_ptr->data.push_back(vma_ptr);
                }
                file_list.push_back(vma_ptr);
            }
        }else{
            if (vma_ptr->anon_name) { //anon vma
                if (is_kvaddr(vma_ptr->anon_name)){ // // kernel 5.15 in kernelspace
                    if (field_offset(anon_vma_name, name) != -1) {
                        vma_ptr->name = "[anon:" + read_cstring(vma_ptr->anon_name + field_offset(anon_vma_name, name),ANON_BUFSIZE,"anon_name") + "]";
                    }else{
                        vma_ptr->name = "[anon:" + read_cstring(vma_ptr->anon_name,ANON_BUFSIZE,"anon_name") + "]";
                    }
                }else if (is_uvaddr(vma_ptr->anon_name,tc) && swap_ptr.get() != nullptr){ // kernel 5.4 in userspace
    #if defined(ARM64)
                    vma_ptr->anon_name &= (USERSPACE_TOP - 1);
    #endif
                    vma_ptr->name = "[anon:" + swap_ptr->uread_cstring(tc->task,vma_ptr->anon_name, ANON_BUFSIZE, "anon_name") + "]";
                }
            } else {
                if (vma_ptr->vm_end > mm_ptr->start_brk && vma_ptr->vm_start < mm_ptr->brk){
                    vma_ptr->name = "[heap]";
                }
                if (vma_ptr->vm_end >= mm_ptr->start_stack && vma_ptr->vm_start <=  mm_ptr->start_stack){
                    vma_ptr->name = "[stack]";
                }
            }
            if (vma_ptr->name.find("bss") != std::string::npos && last_file_vma_ptr != nullptr){
                last_file_vma_ptr->bss = vma_ptr;
            }
            anon_list.push_back(vma_ptr);
        }
        vma_list.push_back(vma_ptr);
    }
}

void UTask::init_auxv(){
    size_t auxv_size = field_size(mm_struct, saved_auxv);
    size_t data_size = BITS64() && !compat ? sizeof(Elf64_Auxv_t) : sizeof(Elf32_Auxv_t);
    int auxv_cnt = auxv_size / data_size;
    void* auxv_buf = read_memory(tc->mm_struct + field_offset(mm_struct, saved_auxv), auxv_size, "mm_struct saved_auxv");
    if(!auxv_buf){
        return;
    }
    auto parseAuxv = [&](auto* elf_auxv) {
        for (int i = 0; i < auxv_cnt; ++i) {
            using T = decltype(elf_auxv->type);
            T type = elf_auxv->type & vaddr_mask;
            T val = elf_auxv->val & vaddr_mask;
            if (type == 0) {
                continue;
            }
            auxv_list[type] = val;
            ++elf_auxv;
        }
    };
    if (BITS64() && !compat) {
        parseAuxv(reinterpret_cast<Elf64_Auxv_t*>(auxv_buf));
    } else {
        parseAuxv(reinterpret_cast<Elf32_Auxv_t*>(auxv_buf));
    }
    FREEBUF(auxv_buf);
}

std::vector<ulong>& UTask::for_each_file(){
    return task_files;
}

std::vector<std::shared_ptr<vma_struct>>& UTask::for_each_vma_list(){
    return vma_list;
}

std::vector<std::shared_ptr<vma_struct>>& UTask::for_each_anon_vma(){
    return anon_list;
}

std::vector<std::shared_ptr<vma_struct>> UTask::for_each_data_vma(std::string filename){
    std::vector<std::shared_ptr<vma_struct>> res;
    if (file_map.find(filename) != file_map.end()) {
        auto file_ptr = file_map[filename];
        res = file_ptr->data;
    }
    return res;
}

std::vector<std::shared_ptr<vma_struct>>& UTask::for_each_file_vma(){
    return file_list;
}

std::unordered_map <ulong, ulong>& UTask::for_each_auxv(){
    return auxv_list;
}

int UTask::get_pointer_size(){
    return pointer_size;
}

std::shared_ptr<vma_struct> UTask::get_bss_vma(std::string filename){
    if (file_map.find(filename) != file_map.end()) {
        auto file_ptr = file_map[filename];
        return file_ptr->bss;
    }
    return nullptr;
}

std::shared_ptr<vma_struct> UTask::get_text_vma(std::string filename){
    if (file_map.find(filename) != file_map.end()) {
        auto file_ptr = file_map[filename];
        return file_ptr->text;
    }
    return nullptr;
}

std::shared_ptr<vma_struct> UTask::get_phdr_vma(std::string filename){
    std::vector<std::shared_ptr<vma_struct>> data_list = for_each_data_vma(filename);
    if (data_list.size() > 0){
        std::sort(data_list.begin(), data_list.end(),[&](const std::shared_ptr<vma_struct>& a, const std::shared_ptr<vma_struct>& b){
            return a->vm_start < b->vm_start;
        });
        return data_list[0];
    }
    return nullptr;
}

std::shared_ptr<vma_struct> UTask::get_vma(ulong addr){
    for(const auto& vma_ptr : for_each_vma_list()){
        if (is_contains(vma_ptr, addr)) {
            return vma_ptr;
        }
    }
    return nullptr;
}

struct task_context* UTask::get_task_context(){
    return tc;
}

std::vector<char> UTask::read_data(ulong addr,int len){
    std::shared_ptr<vma_struct> vma_ptr = get_vma(addr);
    if (vma_ptr == nullptr){ //maybe miss vma
        return swap_ptr->uread_memory(tc->task,addr,len, "read data");
    }
    if (!is_contains(vma_ptr, addr)) {
        return {};
    }
    if(vma_ptr->vm_data.size() == 0){
        vma_ptr->vm_data = read_vma_data(vma_ptr);
    }
    if(vma_ptr->vm_data.size() == 0){
        return {};
    }
    int remain = vma_ptr->vm_end - addr;
    if (remain < len) {
        len = remain;
    }
    std::vector<char> buffer(len);
    std::copy_n(vma_ptr->vm_data.data() + (addr - vma_ptr->vm_start),len, buffer.begin());
    // memcpy(buffer.data(),vma_ptr->vm_data + (addr - vma_ptr->vm_start),len);
    return buffer;
}

std::string UTask::uread_cstring(ulonglong addr,int len){
    std::vector<char> buf = read_data(addr,len);
    if(buf.size() == 0){
        return "";
    }
    std::string res = buf.data();
    return res;
}

bool UTask::uread_bool(ulonglong addr){
    std::vector<char> buf = read_data(addr,sizeof(bool));
    if(buf.size() == 0){
        return false;
    }
    return BOOL(buf.data());
}

int UTask::uread_int(ulonglong addr){
    std::vector<char> buf = read_data(addr,sizeof(int));
    if(buf.size() == 0){
        return 0;
    }
    return INT(buf.data());
}

uint UTask::uread_uint(ulonglong addr){
    std::vector<char> buf = read_data(addr,sizeof(uint));
    if(buf.size() == 0){
        return 0;
    }
    return UINT(buf.data());
}

long UTask::uread_long(ulonglong addr){
    std::vector<char> buf = read_data(addr,sizeof(long));
    if(buf.size() == 0){
        return 0;
    }
    return LONG(buf.data());
}

ulong UTask::uread_ulong(ulonglong addr){
    std::vector<char> buf = read_data(addr,pointer_size);
    if(buf.size() == 0){
        return 0;
    }
    switch(pointer_size){
        case 4:
            return UINT(buf.data());
        case 8:
            return ULONG(buf.data());
        default:
            return ULONG(buf.data());
    }
}

ulonglong UTask::uread_ulonglong(ulonglong addr){
    std::vector<char> buf = read_data(addr,sizeof(ulonglong));
    if(buf.size() == 0){
        return 0;
    }
    return ULONGLONG(buf.data());
}

ushort UTask::uread_ushort(ulonglong addr){
    std::vector<char> buf = read_data(addr,sizeof(ushort));
    if(buf.size() == 0){
        return 0;
    }
    return USHORT(buf.data());
}

short UTask::uread_short(ulonglong addr){
    std::vector<char> buf = read_data(addr,sizeof(short));
    if(buf.size() == 0){
        return 0;
    }
    return SHORT(buf.data());
}

ulong UTask::uread_pointer(ulonglong addr){
    std::vector<char> buf = read_data(addr,pointer_size);
    if(buf.size() == 0){
        return 0;
    }
    return ((ulong)VOID_PTR(buf.data()) & vaddr_mask);
}

unsigned char UTask::uread_byte(ulonglong addr){
    std::vector<char> buf = read_data(addr,1);
    if(buf.size() == 0){
        return 0;
    }
    return UCHAR(buf.data());
}

std::vector<char> UTask::read_vma_data(std::shared_ptr<vma_struct> vma_ptr){
    return swap_ptr->uread_memory(tc->task, vma_ptr->vm_start, vma_ptr->vm_size, "read vma data");
}

void* UTask::read_auxv(){
    size_t auxv_size = field_size(mm_struct, saved_auxv);
    void* auxv_buf = read_memory(tc->mm_struct + field_offset(mm_struct, saved_auxv), auxv_size, "mm_struct saved_auxv");
    if(auxv_buf){
        return auxv_buf;
    }
    return nullptr;
}

ulong UTask::get_auxv(ulong name){
    return auxv_list[name];
}

void UTask::set_auxv(ulong name,ulong val){
    auxv_list[name] = val;
}

bool UTask::is_contains(std::shared_ptr<vma_struct> vma_ptr, ulong addr){
    return (vma_ptr->vm_start <= addr && addr < vma_ptr->vm_end);
}

UTask::~UTask(){
    for (const auto& vma_ptr : vma_list) {
        if(vma_ptr->vm_data.size() > 0){
            vma_ptr->vm_data.clear();
        }
    }
}

ulong UTask::search_stdlist(std::shared_ptr<vma_struct> vma_ptr, ulong start_addr, std::function<bool (ulong)> node_callback) {
    ulong list_size = 0;
    for (size_t addr = start_addr; addr < vma_ptr->vm_end; addr += pointer_size) {
        ulong list_addr = 0;
        if (BITS64() && !is_compat()) {
            list_addr = check_stdlist<list_node64_t, uint64_t>(addr, node_callback, list_size);
        } else {
            list_addr = check_stdlist<list_node32_t, uint32_t>(addr, node_callback, list_size);
        }
        if (list_addr != 0 && is_uvaddr(list_addr, tc) && list_size != 0) {
            // this is a probility list addr
            return list_addr;
        }
    }
    return 0;
}

std::vector<size_t> UTask::for_each_stdlist(ulong stdlist_addr){
    std::vector<size_t> node_list;
    size_t tail_node = uread_ulong(stdlist_addr + 0 * pointer_size) & vaddr_mask;
    size_t next_node = uread_ulong(stdlist_addr + 1 * pointer_size) & vaddr_mask;
    size_t list_size = uread_ulong(stdlist_addr + 2 * pointer_size) & vaddr_mask;
    size_t prev_node = 0;
    size_t current = next_node;
    LOGD("addr:0x%lx tail_node:0x%zx next_node:0x%zx list_size:%zu\n",stdlist_addr,tail_node,next_node,list_size);
    for (size_t i = 1; i <= list_size && is_uvaddr(current, tc); ++i) {
        prev_node = uread_ulong(current + 0 * pointer_size) & vaddr_mask;
        next_node = uread_ulong(current + 1 * pointer_size) & vaddr_mask;
        ulong data_node = current + 2 * pointer_size;
        LOGD("[%zu]addr:0x%zx prev_node:0x%zx next_node:0x%zx data_node:0x%lx\n",i,current,prev_node,next_node,data_node);
        if (next_node == 0 || prev_node == tail_node) {
            break;
        }
        node_list.push_back(data_node);
        current = next_node;
    }
    return node_list;
}

std::vector<size_t> UTask::for_each_stdvector(ulong std_vec_addr, size_t key_size){
    std::vector<size_t> vec;
    size_t begin = uread_pointer(std_vec_addr + 0 * pointer_size);
    size_t end = uread_pointer(std_vec_addr + 1 * pointer_size);
    LOGD("addr:%#lx begin:0x%zx end:0x%zx\n", std_vec_addr, begin, end);
    for (size_t addr = begin; addr < end && is_uvaddr(addr, tc) && addr != 0; addr += key_size) {
        vec.push_back(addr);
    }
    return vec;
}

std::string UTask::for_each_stdstring(ulong std_string_addr){
    size_t len = 0;
    int data = uread_byte(std_string_addr);
    bool is_long = data & 0x1;
    if(is_long){
        len = uread_uint(std_string_addr + 1 * pointer_size);
        std_string_addr = uread_pointer(std_string_addr + 2 * pointer_size);
    }else{
        len = data >> 1;
        std_string_addr = std_string_addr + 1;
    }
    LOGD("is_long:%d len:%zu std_string_addr:%#lx \n", is_long, len, std_string_addr);
    std::vector<char> str= read_data(std_string_addr, len);
    if (!str.empty()) {
        return std::string(str.begin(), str.end());
    } else {
        return std::string();
    }
}

std::unordered_map<size_t, size_t> UTask::for_each_stdunmap(ulong std_un_map_addr, size_t key_size){
    std::unordered_map<size_t, size_t> map;
    size_t node_addr = uread_pointer(std_un_map_addr + 2 * pointer_size);
    size_t map_size = uread_uint(std_un_map_addr + 2 * pointer_size + pointer_size);
    for (size_t i = 0; i <= map_size && is_uvaddr(node_addr, tc) && node_addr != 0; i++) {
        size_t key_addr = node_addr + 2 * pointer_size;
        size_t value_addr = node_addr + 2 * pointer_size + key_size;
        if(is_uvaddr(key_addr, tc) && is_uvaddr(value_addr, tc)){
            map.emplace(std::make_pair(key_addr, value_addr));
        }
        node_addr = uread_pointer(node_addr);
    }
    return map;
}

std::map<size_t, size_t> UTask::for_each_stdmap(ulong std_map_addr, size_t key_size){
    std::map<size_t, size_t> map;
    size_t root = uread_pointer(std_map_addr + pointer_size);
    size_t map_size = uread_uint(std_map_addr + 2 * pointer_size);
    LOGD("addr:%#lx map_size:%zu root:0x%zx \n", std_map_addr, map_size, root);
    if(map_size == 0 || !root || !is_uvaddr(root, tc)){
        return map;
    }
    size_t key_addr = root + 4 * pointer_size;
    size_t value_addr = root + 4 * pointer_size + key_size;
    if(is_uvaddr(key_addr, tc) && is_uvaddr(value_addr, tc)){
        map.emplace(std::make_pair(key_addr, value_addr));
    }
    std::vector<size_t> node_stack;
    node_stack.push_back(root);
    while(!node_stack.empty()){
        size_t root_addr = node_stack.back();
        node_stack.pop_back();
        size_t left_addr = uread_pointer(root_addr + 0 * pointer_size);
        size_t right_addr = uread_pointer(root_addr + 1 * pointer_size);
        key_addr = root_addr + 4 * pointer_size;
        value_addr = root_addr + 4 * pointer_size + key_size;
        if(is_uvaddr(key_addr, tc) && is_uvaddr(value_addr, tc)){
            map.emplace(std::make_pair(key_addr, value_addr));
        }
        LOGD("child root:0x%zx left_addr:0x%zx right_addr:0x%zx \n", root_addr, left_addr, right_addr);
        if(right_addr && is_uvaddr(right_addr, tc)){
            node_stack.push_back(right_addr);
        }
        if(left_addr && is_uvaddr(left_addr, tc)){
            node_stack.push_back(left_addr);
        }
    }
    if(map_size != map.size()){
        LOGD("map size is mismatch, memory size:%zu, real size:%zu \n", map_size, map.size());
    }
    return map;
}

uint64_t UTask::read_sections(std::string& section_name,std::string& libname, int *align) {
    int fd = open(libname.c_str(), O_RDONLY);
    if (fd == -1) {
        LOGD("Failed to open %s\n", libname.c_str());
        return 0;
    }
    if (elf_version(EV_CURRENT) == EV_NONE) {
        LOGD("ELF library initialization failed: %s\n", elf_errmsg(-1));
        close(fd);
        return 0;
    }
    Elf* e = elf_begin(fd, ELF_C_READ, nullptr);
    if (!e) {
        LOGD("elf_begin() failed: %s\n", elf_errmsg(-1));
        close(fd);
        return 0;
    }
    if (elf_kind(e) != ELF_K_ELF) {
        LOGD("%s is not an ELF file.\n", libname.c_str());
        elf_end(e);
        close(fd);
        return 0;
    }
    size_t shstrndx;
    if (elf_getshdrstrndx(e, &shstrndx) != 0) {
        LOGD("elf_getshdrstrndx() failed: %s\n", elf_errmsg(-1));
        elf_end(e);
        close(fd);
        return 0;
    }
    Elf_Scn* scn = nullptr;
    while ((scn = elf_nextscn(e, scn)) != nullptr) {
        GElf_Shdr shdr;
        if (gelf_getshdr(scn, &shdr) != &shdr) {
            LOGD("gelf_getshdr() failed: %s\n", elf_errmsg(-1));
            elf_end(e);
            close(fd);
            return 0;
        }
        std::string name = elf_strptr(e, shstrndx, shdr.sh_name);
        if (name == section_name) {
            elf_end(e);
            close(fd);
            *align = shdr.sh_addralign;
            return shdr.sh_addr;
        }
    }
    elf_end(e);
    close(fd);
    return 0;
}

uint64_t UTask::read_symbol(std::string& symbol_name,std::string& libname) {
    int fd = open(libname.c_str(), O_RDONLY);
    if (fd == -1) {
        LOGD("Failed to open %s\n", libname.c_str());
        return 0;
    }
    if (elf_version(EV_CURRENT) == EV_NONE) {
        LOGD("ELF library initialization failed: %s\n", elf_errmsg(-1));
        close(fd);
        return 0;
    }
    Elf* e = elf_begin(fd, ELF_C_READ, nullptr);
    if (!e) {
        LOGD("elf_begin() failed: %s\n", elf_errmsg(-1));
        close(fd);
        return 0;
    }
    if (elf_kind(e) != ELF_K_ELF) {
        LOGD("%s is not an ELF file.\n", libname.c_str());
        elf_end(e);
        close(fd);
        return 0;
    }
    size_t shstrndx;
    if (elf_getshdrstrndx(e, &shstrndx) != 0) {
        LOGD("elf_getshdrstrndx() failed: %s\n", elf_errmsg(-1));
        elf_end(e);
        close(fd);
        return 0;
    }
    Elf_Scn* scn = nullptr;
    while ((scn = elf_nextscn(e, scn)) != nullptr) {
        GElf_Shdr shdr;
        if (gelf_getshdr(scn, &shdr) != &shdr) {
            LOGD("gelf_getshdr() failed: %s\n", elf_errmsg(-1));
            elf_end(e);
            close(fd);
            return 0;
        }
        if (shdr.sh_type == SHT_SYMTAB || shdr.sh_type == SHT_DYNSYM) {
            Elf_Data* data = elf_getdata(scn, nullptr);
            int count = shdr.sh_size / shdr.sh_entsize;
            for (int i = 0; i < count; ++i) {
                GElf_Sym sym;
                gelf_getsym(data, i, &sym);
                std::string name = elf_strptr(e, shdr.sh_link, sym.st_name);
                if (name.find(symbol_name) != std::string::npos && name.find("_Z") != std::string::npos){
                    elf_end(e);
                    close(fd);
                    return sym.st_value;
                }
            }
        }
    }
    elf_end(e);
    close(fd);
    return 0;
}

//  f1e79bb0  b61db000  b6206000     71  /system/lib/bootstrap/libc.so
//  f1e79990  b6206000  b628e000     75  /system/lib/bootstrap/libc.so
//  f1e79b28  b628e000  b6293000 100071  /system/lib/bootstrap/libc.so
//  f1e79cc0  b6293000  b6296000 100073  /system/lib/bootstrap/libc.so
//  get the first vma start address,such as b61db000
ulong UTask::get_min_vma_start(std::string libname){
    std::vector<std::shared_ptr<vma_struct>> res;
    if (file_map.find(libname) != file_map.end()) {
        auto file_ptr = file_map[libname];
        res = file_ptr->data;
        res.push_back(file_ptr->text);
    }
    if (res.size() > 0){
        std::sort(res.begin(), res.end(),[&](const std::shared_ptr<vma_struct>& a, const std::shared_ptr<vma_struct>& b){
            return a->vm_start < b->vm_start;
        });
        return res[0].get()->vm_start;
    }
    return 0;
}

ulong UTask::get_var_addr_by_bss(std::string libname, std::string var_name){
    std::string filename = libname;
    size_t pos = libname.find_last_of("/\\");
    if (pos != std::string::npos) {
        filename = libname.substr(pos + 1);
    }
    for(const auto& vma_ptr : for_each_file_vma()){
        if(vma_ptr->name.find(filename) != std::string::npos){
            filename = vma_ptr->name;
            break;
        }
    }
    if(filename.empty()){
        return 0;
    }
    LOGD("filename: %s \n",filename.c_str());
    // get the min vaddr of the lib
    ulong vraddr = get_min_vma_start(filename);
    LOGD("Min vaddr:0x%lx \n",vraddr);
    // get the static addr of bss.
    int bss_align = 0;
    std::string section_name = ".bss";
    size_t bss_saddr = read_sections(section_name,libname,&bss_align);
    LOGD("bss_saddr:%#zx \n",bss_saddr);
    LOGD("bss_align:%d \n",bss_align);
    // calc the runtime addr of bss for lib
    size_t bss_vaddr = vraddr + bss_saddr;
    bss_vaddr = roundup(bss_vaddr,bss_align);
    LOGD("bss_vaddr:%#zx \n",bss_vaddr);
    // get the static addr of var_name
    size_t var_saddr = read_symbol(var_name,libname);
    if (var_saddr == 0){
        return 0;
    }
    LOGD("var_saddr:%#zx \n",var_saddr);
    // calc the runtime addr of var_name
    size_t var_vaddr = bss_vaddr + (var_saddr - bss_saddr);
    LOGD("var_vaddr:%#zx \n",var_vaddr);
    if (!is_uvaddr(var_vaddr,tc)){
        return 0;
    }
    return var_vaddr;
}

std::string UTask::read_start_args(){
    std::string args = uread_cstring(mm_ptr->arg_start,mm_ptr->arg_end - mm_ptr->arg_start);
    if (args.empty()){
        args = tc->comm;
    }
    return args;
}
#pragma GCC diagnostic pop
