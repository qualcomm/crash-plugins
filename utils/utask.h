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

#ifndef TASK_DEFS_H_
#define TASK_DEFS_H_

#include "plugin.h"
#include "memory/swapinfo.h"

/**
 * Virtual Memory Area structure
 * Represents a contiguous range of virtual memory in a process
 */
struct vma_struct {
    std::string name;           // Name of the VMA (e.g., [heap], [stack], library name)
    ulong addr;                 // Address of the vm_area_struct in kernel memory
    ulong vm_start;             // Start address of the VMA
    ulong vm_end;               // End address of the VMA
    ulong vm_size;              // Size of the VMA (vm_end - vm_start)
    ulong vm_flags;             // VMA flags (read/write/execute permissions)
    ulong vm_file;              // Pointer to associated file structure (if file-backed)
    ulong vm_pgoff;             // Page offset in the file
    ulong anon_name;            // Anonymous VMA name pointer
    ulong anon_vma;             // Anonymous VMA structure pointer
    std::vector<char> vm_data;  // Cached data from this VMA
};

/**
 * Memory management structure
 * Contains information about a process's memory layout
 */
struct mm_struct {
    int mm_count;               // Reference count
    ulong pgd;                  // Page Global Directory address
    ulong start_code;           // Start address of code segment
    ulong end_code;             // End address of code segment
    ulong start_data;           // Start address of data segment
    ulong end_data;             // End address of data segment
    ulong start_brk;            // Start address of heap
    ulong brk;                  // Current end of heap
    ulong start_stack;          // Start address of stack
    ulong arg_start;            // Start of command line arguments
    ulong arg_end;              // End of command line arguments
    ulong env_start;            // Start of environment variables
    ulong env_end;              // End of environment variables
    ulong flags;                // Memory management flags
};

/**
 * 32-bit linked list node structure
 * Used for parsing std::list in 32-bit processes
 */
typedef struct{
    uint32_t prev;              // Pointer to previous node
    uint32_t next;              // Pointer to next node
    uint32_t data;              // Data or list size
} list_node32_t;

/**
 * 64-bit linked list node structure
 * Used for parsing std::list in 64-bit processes
 */
typedef struct{
    uint64_t prev;              // Pointer to previous node
    uint64_t next;              // Pointer to next node
    uint64_t data;              // Data or list size
} list_node64_t;

/**
 * File-backed VMA structure
 * Groups related VMAs for a single file (text, data, bss segments)
 */
struct file_vma {
    std::shared_ptr<vma_struct> text;                       // Text (code) segment
    std::vector<std::shared_ptr<vma_struct>> data;          // Data segments
    std::shared_ptr<vma_struct> bss;                        // BSS (uninitialized data) segment
};

class UTask : public ParserPlugin {
private:
    bool compat = false;
    int pointer_size = 8;
    struct task_context *tc;
    std::shared_ptr<Swapinfo> swap_ptr;
    std::shared_ptr<mm_struct> mm_ptr;
    std::vector<std::shared_ptr<vma_struct>> vma_list;
    std::vector<std::shared_ptr<vma_struct>> anon_list;
    std::vector<std::shared_ptr<vma_struct>> file_list;
    std::vector<ulong> task_files;
    std::unordered_map <ulong, ulong> auxv_list; // <type, val>
    std::unordered_map<std::string, std::shared_ptr<file_vma>> file_map;
    ulong min_rw_vma_addr = ULONG_MAX;
    ulong max_rw_vma_addr = 0;

    void init_mm_struct();
    void init_vma();
    void init_auxv();
    template<typename T, typename P>
    size_t check_object(std::string libname, std::shared_ptr<vma_struct> vma_ptr, std::function<bool (T*)> obj_callback,int vtb_cnt) {
        LOGD("[%#lx-%#lx]:%s \n", vma_ptr->vm_start, vma_ptr->vm_end, vma_ptr->name.c_str());
        // read the whole vma data;
        if(vma_ptr->vm_data.size() == 0){
            vma_ptr->vm_data = read_vma_data(vma_ptr);
        }
        for (size_t addr = vma_ptr->vm_start; addr + sizeof(T) < vma_ptr->vm_end; addr += sizeof(P)) {
            // read the obj data;
            T* obj = reinterpret_cast<T*>(vma_ptr->vm_data.data() + (addr - vma_ptr->vm_start));
            ulong vtable_ptr = obj->vtpr & vaddr_mask;
            // verify the virtual table pointer;
            if (!is_uvaddr(vtable_ptr, tc) || !vtable_ptr /* 0 */) {
                continue;
            }
            // read the virtual function table, it is in the data segment
            P* vtable = nullptr;
            for (const auto& data_vma_ptr : for_each_data_vma(libname)) {
                if (is_contains(data_vma_ptr, vtable_ptr) && is_contains(data_vma_ptr, (vtable_ptr + sizeof(vtb_cnt * sizeof(P))))) {
                    LOGD("vtpr:%#lx \n", vtable_ptr);
                    // LOGD("%s", hexdump(0x1000, data_vma_ptr->vm_data, data_vma_ptr->vm_size).c_str());
                    if(data_vma_ptr->vm_data.size() == 0){
                        data_vma_ptr->vm_data = read_vma_data(data_vma_ptr);
                    }
                    vtable = reinterpret_cast<P*>(data_vma_ptr->vm_data.data() + (vtable_ptr - data_vma_ptr->vm_start));
                }
            }
            // verify the virtual function address is in the text segment.
            if (!vtable) continue;
            std::shared_ptr<vma_struct> text_vma_ptr = get_text_vma(libname);
            bool match = true;
            if(text_vma_ptr){
                for (int i = 0; i < vtb_cnt; ++i){
                    ulong vfun_ptr = vtable[i] & vaddr_mask;
                    if (vfun_ptr == 0) continue;
                    if(!is_contains(text_vma_ptr, vfun_ptr)){
                        match = false;
                        break;
                    }
                }
            }
            if(obj_callback){
                match &= obj_callback(obj);
            }
            if (match){
                LOGD("Found the match vtable, addr:%#" PRIxPTR " vtpr:%#" PRIxPTR "\n", (uintptr_t)addr, (uintptr_t)(vtable_ptr));
                for (int i = 0; i < vtb_cnt; ++i){
                    ulong vfun_ptr = vtable[i] & vaddr_mask;
                    LOGD("  vfunc[%d] addr:%#" PRIxPTR  "\n", i, (uintptr_t)vfun_ptr);
                }
                return addr;
            }
        }
        return 0;
    };

public:
    ulong vaddr_mask = 0;

    void init_offset(void) override;
    void init_command(void) override;
    void cmd_main(void) override;
    UTask(std::shared_ptr<Swapinfo> swap, int pid);
    UTask(std::shared_ptr<Swapinfo> swap, ulong task_addr);
    std::vector<std::shared_ptr<vma_struct>>& for_each_vma_list();
    std::vector<std::shared_ptr<vma_struct>>& for_each_anon_vma();
    std::vector<ulong>& for_each_file();
    std::shared_ptr<vma_struct> get_text_vma(std::string filename);
    std::vector<std::shared_ptr<vma_struct>> for_each_data_vma(std::string filename);
    std::shared_ptr<vma_struct> get_phdr_vma(std::string filename);
    std::shared_ptr<vma_struct> get_vma(ulong addr);
    std::vector<std::shared_ptr<vma_struct>>& for_each_file_vma();
    std::unordered_map<ulong, ulong>& for_each_auxv();
    std::vector<char> read_vma_data(std::shared_ptr<vma_struct> vma_ptr);
    std::vector<char> read_data(ulong addr, int len);
    std::string uread_cstring(ulonglong addr,int len);
    bool uread_bool(ulonglong addr);
    int uread_int(ulonglong addr);
    uint uread_uint(ulonglong addr);
    long uread_long(ulonglong addr);
    ulong uread_ulong(ulonglong addr);
    ulonglong uread_ulonglong(ulonglong addr);
    ushort uread_ushort(ulonglong addr);
    short uread_short(ulonglong addr);
    ulong uread_pointer(ulonglong addr);
    unsigned char uread_byte(ulonglong addr);
    int get_pointer_size();
    std::shared_ptr<vma_struct> get_bss_vma(std::string filename);
    void *read_auxv();
    ulong get_auxv(ulong name);
    struct task_context* get_task_context();
    void set_auxv(ulong name, ulong val);
    bool is_compat();
    bool is_contains(std::shared_ptr<vma_struct> vma_ptr, ulong addr);
    ~UTask();
    std::string read_start_args();
    uint64_t read_sections(std::string &section_name, std::string &libname, int *align);
    uint64_t read_symbol(std::string &symbol_name, std::string &libname);
    ulong get_min_vma_start(std::string libname);
    ulong get_var_addr_by_bss(std::string libname, std::string var_name);
    ulong search_stdlist(std::shared_ptr<vma_struct> vma_ptr, ulong start_addr, std::function<bool (ulong)> node_callback);
    std::vector<size_t> for_each_stdlist(ulong stdlist_addr);
    std::vector<size_t> for_each_stdvector(ulong std_vec_addr, size_t key_size);
    std::string for_each_stdstring(ulong std_string_addr);
    std::unordered_map<size_t, size_t> for_each_stdunmap(ulong std_un_map_addr, size_t key_size);
    std::map<size_t, size_t> for_each_stdmap(ulong std_map_addr, size_t key_size);
    /*
                +----------------------------------------------------+
                |                                                    |
                v                                                    |
        +----------+<-+   +---->+----------+<--+  +----->+----------+<--|----+
    +---|taild_node|  +---|--+  |prev_node |   |  |      |prev_node |   |    |
    |   +----------+      |  |  +----------+   +--|------+----------+   |    |
    |   |head_node |------+  +--|head_node |------+      |head_node |---+    |
    |   +----------+            +----------+             +----------+        |
    |   |list_count|            |chunk     |             |chunk     |        |
    |   +----------+            +----------+             +----------+        |
    |                                                                        |
    +------------------------------------------------------------------------+
    */
    template<typename T, typename U>
    ulong check_stdlist(ulong addr, std::function<bool (ulong)> node_callback, ulong &list_size) {
        std::vector<char> buf = uread_obj<T>(addr);
        if (buf.size() == 0){
            return 0;
        }
        auto* head_node = reinterpret_cast<T*>(buf.data());
        /* operation pointer, mask the dirty data*/
        U tmp_next = head_node->next & vaddr_mask;
        U tmp_prev = head_node->prev & vaddr_mask;
        U tmp_data = head_node->data & vaddr_mask;
        LOGD("  addr:%#" PRIxPTR " tail_node:%#" PRIxPTR " next_node:%#" PRIxPTR " list_size:%#" PRIxPTR "\n",
            (uintptr_t)addr,
            (uintptr_t)(tmp_prev),
            (uintptr_t)(tmp_next),
            (uintptr_t)(tmp_data));
        if (!(tmp_prev >= min_rw_vma_addr && tmp_prev <= max_rw_vma_addr)
            || !(tmp_next >= min_rw_vma_addr && tmp_next <= max_rw_vma_addr)) {
                return 0;
        }
        // tail node
        if (tmp_prev == tmp_next) {
            /*
                for R, We will skip the empty std::list.
                for S, It's normal even the std::list is empty, because the list size is 8.
            */
            list_size = tmp_data;
            return addr;
        }
        U index = 0;
        uintptr_t head_node_addr = addr;
        uintptr_t prev_node_addr = addr;
        uintptr_t next_node_addr = tmp_next;
        while (is_uvaddr(next_node_addr, tc) && index < head_node->data /* list_size */) {
            std::vector<char> buf = uread_obj<T>(next_node_addr);
            if (buf.size() == 0){
                break;
            }
            auto* next_node = reinterpret_cast<T*>(buf.data());
            tmp_next = next_node->next & vaddr_mask;
            tmp_prev = next_node->prev & vaddr_mask;
            tmp_data = next_node->data & vaddr_mask;
            LOGD("    addr:%#" PRIxPTR " prev_node:%#" PRIxPTR " next_node:%#" PRIxPTR " data:%#" PRIxPTR "\n",
                (uintptr_t)next_node_addr,
                (uintptr_t)tmp_prev,
                (uintptr_t)tmp_next,
                (uintptr_t)tmp_data);
            if (!(tmp_prev >= min_rw_vma_addr && tmp_prev <= max_rw_vma_addr)
                || !(tmp_next >= min_rw_vma_addr && tmp_next <= max_rw_vma_addr)) {
                break;
            }
            if (node_callback && !node_callback(next_node_addr)){
                break;
            }
            if (tmp_prev != prev_node_addr) {
                break;
            }
            if (tmp_next == head_node_addr) {
                list_size = tmp_data;
                return head_node_addr;
            }
            prev_node_addr = next_node_addr;
            next_node_addr = tmp_next;
            index++;
        }
        return 0;
    };

    template<typename T>
    std::vector<char> uread_obj(ulonglong addr){
        return read_data(addr,sizeof(T));
    }
    template<typename T, typename P>
    size_t search_obj(std::string libname, bool is_static, std::function<bool (std::shared_ptr<vma_struct>)> vma_callback, std::function<bool (T*)> obj_callback,int vtb_cnt) {
        for (const auto& data_ptr : for_each_data_vma(libname)) {
            LOGD("%s data:[%#lx-%#lx] \n", libname.c_str(), data_ptr->vm_start, data_ptr->vm_end);
        }
        std::shared_ptr<vma_struct> text_ptr = get_text_vma(libname);
        if (text_ptr != nullptr){
            LOGD("%s text:[%#lx-%#lx] \n", libname.c_str(), text_ptr->vm_start, text_ptr->vm_end);
        }
        std::shared_ptr<vma_struct> bss_ptr = get_bss_vma(libname);
        if (bss_ptr != nullptr){
            LOGD("%s bss:[%#lx-%#lx] \n", libname.c_str(), bss_ptr->vm_start, bss_ptr->vm_end);
        }
        std::vector<std::shared_ptr<vma_struct>> vm_list;
        vm_list.clear();
        if (is_static){
            std::shared_ptr<vma_struct> bss_ptr = get_bss_vma(libname);
            if (bss_ptr){
                vm_list.push_back(bss_ptr);
            }
        }else{
            vm_list = for_each_anon_vma();
        }
        if (vm_list.size() == 0){
            return 0;
        }
        for (const auto& vma_ptr : vm_list) {
            if (vma_callback && !vma_callback(vma_ptr)) {
                continue;
            }
            ulong addr = check_object<T,P>(libname,vma_ptr,obj_callback,vtb_cnt);
            if (addr > 0){
                return addr;
            }
        }
        return 0;
    };
};

#endif // TASK_DEFS_H_
