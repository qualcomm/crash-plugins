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

#ifndef PARSER_DEFS_H_
#define PARSER_DEFS_H_

#include <iostream>
#include <string>
#include <vector>
#include <linux/types.h>
#include <unistd.h>
#include <getopt.h>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <algorithm>
#include <bitset>
#include <set>
#include <regex.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <iomanip>
#include <arpa/inet.h>
#include <inttypes.h>
#include "struct_info.h"
#include <sstream>
#include <gelf.h>
#include <map>
#include "logger/logger_core.h"

// Forward declarations
struct driver;
struct device;
struct bus_type;
struct class_type;
struct page_owner;

struct driver {
    size_t addr;
    std::string name;
    std::string probe;
    std::string compatible;
};

struct device {
    size_t addr;
    std::string name;
    ulong driver_data;
    ulong driv;
};

struct bus_type {
    size_t addr;
    std::string name;
    std::string probe;
    size_t subsys_private;
};

struct class_type {
    size_t addr;
    std::string name;
    size_t subsys_private;
};

#define field_init(type,field_name,...) type_init(TO_STD_STRING(type),TO_STD_STRING(field_name), ##__VA_ARGS__)
#define field_size(type,field_name) type_size(TO_STD_STRING(type),TO_STD_STRING(field_name))
#define field_offset(type,field_name) type_offset(TO_STD_STRING(type),TO_STD_STRING(field_name))

#define struct_init(type,...) type_init(TO_STD_STRING(type), ##__VA_ARGS__)
#define struct_size(type) type_size(TO_STD_STRING(type))
#define IS_ALIGNED(x, a)    (((x) & ((typeof(x))(a) - 1)) == 0)

typedef struct {
    long long counter;
} atomic64_t;

typedef struct {
    int counter;
} atomic_t;

typedef struct {
    int counter;
} atomic_long_t;

typedef struct {
    uint32_t type;
    uint32_t val;
} Elf32_Auxv_t;

typedef struct {
    uint64_t type;
    uint64_t val;
} Elf64_Auxv_t;

typedef struct {
    uint slab_index;
    uint record_offset;
    ulong slab_addr;
    ulong record_addr;
} stack_record_t;

/* from lib/stackdepot.c */
#define DEPOT_STACK_ALIGN    4

union handle_parts {
    uint handle;
    struct {
        uint pool_index    : 21;
        uint offset    : 10;
        uint valid    : 1;
    } v1;
    struct {
        uint pool_index : 16;
        uint offset    : 10;
        uint valid    : 1;
        uint extra    : 5;
    } v2;    /* 6.1 and later */
    struct {
        uint pool_index : 17;
        uint offset    : 10;
        uint extra    : 5;
    } v3;    /* 6.8 and later */
};

#define VM_NONE             0x00000000
#define VM_READ             0x00000001    /* currently active flags */
#define VM_WRITE            0x00000002
#define VM_EXEC             0x00000004
#define VM_SHARED           0x00000008
#define VM_MAYREAD          0x00000010    /* limits for mprotect() etc */
#define VM_MAYWRITE         0x00000020
#define VM_MAYEXEC          0x00000040
#define VM_MAYSHARE         0x00000080
#define VM_GROWSDOWN        0x00000100    /* general info on the segment */
#define VM_UFFD_MISSING     0x00000200    /* missing pages tracking */
#define VM_PFNMAP           0x00000400    /* Page-ranges managed without "struct page", just pure PFN */
#define VM_DENYWRITE        0x00000800    /* ETXTBSY on write attempts.. */
#define VM_UFFD_WP          0x00001000    /* wrprotect pages tracking */
#define VM_LOCKED           0x00002000
#define VM_IO               0x00004000    /* Memory mapped I/O or similar */
#define VM_HUGETLB          0x00400000
#define VM_DONTDUMP         0x04000000

#define GENMASK_ULL(h, l) (((1ULL<<(h+1))-1)&(~((1ULL<<l)-1)))

struct mount_point {
    ulong addr;
    std::string path;
    ulong root_dentry;
};

/* Page extension invalid flag from mm/page_ext.c */
#define PAGE_EXT_INVALID    (0x1)

/**
 * struct page_owner - Represents page ownership information
 *
 * This structure contains allocation/deallocation tracking information
 * for a physical page in the kernel memory management system.
 */
struct page_owner {
    ulong addr;                      // Virtual address of page_owner structure
    ulong pfn;                       // Page Frame Number
    ulong page_ext;
    unsigned short order;            // Allocation order (2^order pages)
    short last_migrate_reason;       // Last page migration reason code
    unsigned int gfp_mask;           // GFP (Get Free Pages) allocation flags
    unsigned int handle;             // Stack trace handle for allocation
    unsigned int free_handle;        // Stack trace handle for deallocation
    unsigned long long ts_nsec;      // Allocation timestamp in nanoseconds
    unsigned long long free_ts_nsec; // Deallocation timestamp in nanoseconds
    size_t pid;                      // Process ID that allocated the page
    size_t tgid;                     // Thread Group ID (process group)
    std::string comm;                // Process command name
    std::shared_ptr<stack_record_t> stack_ptr;
};

class ParserPlugin {
private:
#if defined(ARM)
    ulong* pmd_page_addr(ulong pmd);
#endif
    std::vector<ulong> get_block_device_by_bdevs();
    std::vector<ulong> get_block_device_by_class();
    std::vector<ulong> get_block_device_by_bdevfs();
    std::vector<ulong> for_each_kobj_map(const std::string& map_name);
    std::vector<ulong> get_disk_by_block_device();
    std::vector<ulong> get_disk_by_bdevmap();
    ulong find_vfsmount_by_superblock(ulong dentry);

protected:
    static constexpr double KB = 1024.0;
    static constexpr double MB = 1024.0 * 1024.0;
    static constexpr double GB = 1024.0 * 1024.0 * 1024.0;
    std::unordered_map<std::string, std::unique_ptr<Typeinfo>> typetable;

public:
    const size_t page_size = PAGESIZE();
    const size_t page_shift = PAGESHIFT();
    const size_t page_mask = ~(page_size - 1);
    int depot_index = 0;
    ulong stack_slabs = 0;
    ulong kaddr_mask = 0;
    ulong max_pfn = 0;
    ulong min_low_pfn = 0;
    size_t page_ext_ops_offset = 0;         // Offset to page_owner in page_ext
    size_t page_ext_size = 0;
    long PAGE_EXT_OWNER;                    // Page owner extension flag bit
    long PAGE_EXT_OWNER_ALLOCATED;          // Page allocated flag bit

    std::string cmd_name;
    std::vector<std::string> help_str_list;
    char** cmd_help;
    bool do_init_offset = true;

    ParserPlugin();
    virtual void cmd_main(void)=0;
    virtual void init_offset(void)=0;
    virtual void init_command(void)=0;
    void initialize(void);
    std::string csize(uint64_t size);
    std::string csize(uint64_t size, int unit, int precision);
    struct task_context* find_proc(const std::string& name);
    struct task_context* find_proc(ulong pid);
    bool page_buddy(ulong page_addr);
    int page_count(ulong page_addr);
    void print_table();
    void type_init(const std::string& type, bool is_anon = false);
    void type_init(const std::string& type,const std::string& field, bool is_anon = false);
    int type_offset(const std::string& type,const std::string& field);
    int type_size(const std::string& type,const std::string& field);
    int type_size(const std::string& type);

    std::vector<ulong> for_each_rbtree(ulong rb_root,int offset);
    std::vector<ulong> for_each_list(ulong list_head,int offset);
    std::vector<ulong> for_each_hlist(ulong hlist_head,int offset);
    std::vector<ulong> for_each_xarray(ulong xarray_addr);
    std::vector<ulong> for_each_mptree(ulong maptree_addr);
    std::vector<ulong> for_each_radix(ulong root_rnode);

    std::vector<ulong> for_each_pfn();
    std::vector<ulong> for_each_file_page();
    std::vector<ulong> for_each_anon_page();
    std::vector<ulong> for_each_inode();
    std::vector<ulong> for_each_process();
    std::vector<ulong> for_each_threads();
    std::vector<ulong> for_each_vma(ulong task_addr);
    std::vector<ulong> for_each_char_device();
    std::vector<ulong> for_each_cdev();
    std::vector<ulong> for_each_disk();
    std::vector<ulong> for_each_misc_dev();
    std::vector<ulong> for_each_bdev();
    std::vector<ulong> for_each_bus();

    std::vector<std::shared_ptr<bus_type>> for_each_bus_type();
    std::vector<std::shared_ptr<class_type>> for_each_class_type();
    std::vector<ulong> for_each_class();
    std::vector<ulong> for_each_address_space(ulong i_mapping);
    std::vector<ulong> for_each_subdirs(ulong dentry);
    std::vector<ulong> for_each_percpu(ulong addr);
    std::vector<std::shared_ptr<device>> for_each_device();
    std::vector<std::shared_ptr<device>> for_each_device_for_bus(const std::string& bus_name);
    std::vector<std::shared_ptr<device>> for_each_device_for_class(const std::string& class_name);
    std::vector<std::shared_ptr<device>> for_each_device_for_driver(ulong driver_addr);
    std::shared_ptr<driver> find_device_driver(const std::string & driver_name);
    std::vector<ulong> for_each_driver(const std::string& bus_name);
    std::vector<ulong> for_each_task_files(task_context *tc);

    std::shared_ptr<class_type> parser_class_info(ulong addr);
    std::shared_ptr<bus_type> parser_bus_info(ulong addr);
    std::shared_ptr<device> parser_device(ulong addr);
    std::shared_ptr<driver> parser_driver(ulong addr);

    ulong get_bus_subsys_private(const std::string& bus_name);
    ulong get_class_subsys_private(const std::string& class_name);

    ulonglong read_structure_field(ulong addr, const std::string &type, const std::string &field, bool virt = true);
    std::string read_long_string(ulong kvaddr, const std::string &note, bool virt = true);
    std::string read_cstring(ulong addr, int len, const std::string &note, bool virt = true);
    void* read_struct(ulong addr,const std::string& type,bool virt=true);
    bool read_struct(ulong addr,void* buf, int len, const std::string& type,bool virt=true);
    void* read_memory(ulong addr,int len, const std::string& note,bool virt=true);
    ulong read_pointer(ulong addr, const std::string& note,bool virt=true);
    short read_short(ulong addr,const std::string& note,bool virt=true);
    ushort read_ushort(ulong addr,const std::string& note,bool virt=true);
    ulonglong read_ulonglong(ulong addr,const std::string& note,bool virt=true);
    ulong read_ulong(ulong addr,const std::string& note,bool virt=true);
    long read_long(ulong addr,const std::string& note,bool virt=true);
    uint read_uint(ulong addr,const std::string& note,bool virt=true);
    int read_int(ulong addr,const std::string& note,bool virt=true);
    bool read_bool(ulong addr,const std::string& note,bool virt=true);
    unsigned char read_byte(ulong addr, const std::string& note,bool virt=true);
    int csymbol_exists(const std::string& note);
    ulong csymbol_value(const std::string& note);
    bool is_kvaddr(ulong addr);
    bool is_uvaddr(ulong addr, struct task_context *);
    int page_to_nid(ulong page);
    ulong virt_to_phy(ulong vaddr);
    ulong phy_to_virt(ulong paddr);
    ulong page_to_pfn(ulong page);
    ulong pfn_to_page(ulong pfn);
    ulong phy_to_page(ulong paddr);
    physaddr_t page_to_phy(ulong page);
    physaddr_t pfn_to_phy(ulong pfn);
    ulong phy_to_pfn(ulong paddr);
    std::string get_config_val(const std::string& conf_name);
    void cfill_pgd(ulonglong pgd, int type, ulong size);
    void cfill_pmd(ulonglong pmd, int type, ulong size);
    void cfill_ptbl(ulonglong ptbl, int type, ulong size);
    void print_backtrace();

    bool is_binary_stripped(std::string& filename);
    bool add_symbol_file(std::string& filename);
    std::string formatTimestamp(uint64_t timestamp_ns);
    bool isNumber(const std::string& str);
    std::string extract_string(const char *input);
    int is_bigendian(void);
    std::vector<std::string> get_enumerator_list(const std::string &enum_name);
    long read_enum_val(const std::string &enum_name);
    std::map<std::string, ulong> read_enum_list(const std::string& enum_list_name);
    std::string hexdump(uint64_t addr, const char* buf, size_t length, bool little_endian = true);
    std::stringstream get_curpath();
    bool load_symbols(std::string& path, std::string name);
    void uwind_irq_back_trace(int cpu, ulong x30);
    void uwind_task_back_trace(int pid, ulong x30);
    std::shared_ptr<stack_record_t> get_stack_record(uint handle);
    std::string get_call_stack(std::shared_ptr<stack_record_t> record_ptr);
    std::vector<std::shared_ptr<mount_point>> get_mntpoint_list(task_context *tc);
    std::string get_dentry_path(ulong dentry);
    std::string get_dentry_name(ulong dentry);
    ulong path_to_dentry(const std::string& orig_path);
    ulong find_file_in_dir(ulong dentry, const std::string& name);
    void normalize_path(std::string &path);
    ulong get_inode(ulong dentry);
    bool create_directories_recursive(const std::string& path);
    void write_pagecache_to_file(ulong inode_addr, const std::string& filename, const std::string& dst_dir, bool show_log=false);
#if defined(ARM)
    ulong get_arm_pte(ulong task_addr, ulong page_vaddr);
#endif

    // Page owner related functions
    bool is_enable_pageowner();
    std::shared_ptr<page_owner> parse_page_owner_by_page(ulong page_addr);
    std::shared_ptr<page_owner> parse_page_owner_by_pfn(ulong pfn);
    std::shared_ptr<page_owner> parse_page_owner_by_phys(ulong phys_addr);
    std::shared_ptr<page_owner> parse_page_owner_by_vaddr(ulong virt_addr);
    ulong lookup_page_ext(ulong page);
    bool page_ext_invalid(ulong page_ext);
    ulong get_entry(ulong base, ulong pfn);
    bool is_page_allocated(std::shared_ptr<page_owner> owner_ptr);
    std::string to_symbol(ulong addr);
    unsigned long jiffies_to_msecs(unsigned long jiffies_val);
    virtual cmd_func_t get_wrapper_func();
    void print_page_owner_info(std::shared_ptr<page_owner> owner_ptr);
};

#define DEFINE_PLUGIN_INSTANCE(class_name)                                                                      \
    static std::shared_ptr<class_name> instance;                                                                \
    static void wrapper_func() {                                                                                \
        if (instance) {                                                                                         \
            SimpleLogger::set_context(instance->cmd_name);                                                      \
            instance->cmd_main();                                                                               \
            SimpleLogger::clear_context();                                                                      \
        }                                                                                                       \
    }                                                                                                           \
    cmd_func_t get_wrapper_func() override {                                                                    \
        return &class_name::wrapper_func;                                                                       \
    }

#ifndef BUILD_TARGET_TOGETHER
#define DEFINE_PLUGIN_COMMAND(class_name)                                                                       \
    extern "C" void class_name##_init(void);                                                                    \
    extern "C" void class_name##_fini(void);                                                                    \
    std::shared_ptr<class_name> class_name::instance = nullptr;                                                 \
    static struct command_table_entry command_table[] = {                                                       \
        { nullptr, nullptr, nullptr, 0 },                                                                       \
        { NULL }                                                                                                \
    };                                                                                                          \
    extern "C" void __attribute__((constructor)) class_name##_init(void) {                                      \
        class_name::instance = std::make_shared<class_name>();                                                  \
        class_name::instance->init_command();                                                                   \
        class_name::instance->initialize();                                                                     \
        if (class_name::instance->do_init_offset) {                                                             \
            class_name::instance->init_offset();                                                                \
        }                                                                                                       \
        command_table[0] = {&class_name::instance->cmd_name[0],                                                 \
            class_name::instance->get_wrapper_func(),                                                           \
            class_name::instance->cmd_help,                                                                     \
            0};                                                                                                 \
        register_extension(command_table);                                                                      \
    }                                                                                                           \
    extern "C" void __attribute__((destructor)) class_name##_fini(void) {                                       \
        class_name::instance.reset();                                                                           \
    }
#endif // BUILD_TARGET_TOGETHER

#endif // PARSER_DEFS_H_
