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

#include "plugin.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

ParserPlugin::ParserPlugin(){
    // Inode structure
    struct_init(inode);

    // Dentry structure and related fields
    struct_init(dentry);
    field_init(dentry, d_u);
    field_init(dentry, d_subdirs);
    field_init(dentry, d_children);
    field_init(dentry, d_child);
    field_init(dentry, d_inode);
    field_init(dentry, d_sib);
    field_init(dentry, d_iname);
    field_init(dentry, d_name);
    field_init(dentry, d_sb);

    // String structure for dentry names
    field_init(qstr, name);
    field_init(qstr, len);

    // Mount structures (modern and legacy)
    field_init(mount, mnt_parent);
    field_init(mount, mnt_mountpoint);
    field_init(mount, mnt);
    field_init(mount, mnt_sb);

    field_init(vfsmount, mnt_parent);
    field_init(vfsmount, mnt_mountpoint);
    field_init(vfsmount, mnt_root);
    field_init(vfsmount, mnt_sb);

    // Address space for file mapping
    struct_init(address_space);
    field_init(address_space, host);
    field_init(address_space, a_ops);
    field_init(address_space, nrpages);
    field_init(address_space, i_pages);
    field_init(address_space, page_tree);

    // Super block structure
    field_init(super_block, s_inodes);
    field_init(inode, i_sb_list);
    // Task structure
    struct_init(task_struct);
    field_init(task_struct, active_mm);
    field_init(task_struct, mm);
    field_init(task_struct, tasks);
    field_init(task_struct, files);

    // File descriptor structures
    field_init(files_struct, fdt);
    field_init(fdtable, max_fds);
    field_init(fdtable, fd);

    // Memory management structures
    struct_init(mm_struct);
    field_init(mm_struct, pgd);
    field_init(mm_struct, arg_start);
    field_init(mm_struct, arg_end);
    field_init(mm_struct, mmap);
    field_init(mm_struct, mm_mt);

    // Maple tree for VMA management (newer kernels)
    field_init(maple_tree, ma_root);

    // Virtual memory area structures
    struct_init(vm_area_struct);
    field_init(vm_area_struct, vm_start);
    field_init(vm_area_struct, vm_end);
    field_init(vm_area_struct, vm_flags);
    field_init(vm_area_struct, vm_next);
    // Page structure
    struct_init(page);
    field_init(page, _mapcount);
    field_init(page, freelist);
    field_init(page, units);
    field_init(page, index);
    field_init(page, private);
    field_init(page, page_type);
    field_init(page, _count);
    field_init(page, _refcount);
    field_init(page, mapping);

    // Generic list structure
    struct_init(list_head);
    field_init(list_head, prev);
    field_init(list_head, next);
    // Block device structures
    field_init(block_device, bd_disk);
    field_init(block_device, bd_list);
    field_init(block_device, bd_device);
    field_init(bdev_inode, vfs_inode);
    field_init(bdev_inode, bdev);

    // Driver and device private structures
    field_init(driver_private, driver);
    field_init(driver_private, klist_devices);
    field_init(driver_private, knode_bus);
    field_init(device_private, knode_bus);
    field_init(device_private, device);
    field_init(device_private, knode_driver);
    field_init(device_private, knode_class);

    // Subsystem private structures
    field_init(subsys_private, klist_drivers);
    field_init(subsys_private, klist_devices);
    field_init(subsys_private, bus);
    field_init(subsys_private, subsys);
    field_init(subsys_private, class);

    // Kernel list and kobject structures
    field_init(klist_node, n_node);
    field_init(klist, k_list);
    field_init(device_driver, p);
    field_init(device_driver, name);            // Driver name
    field_init(device_driver, mod_name);        // Module name
    field_init(device_driver, probe);           // Probe function pointer
    field_init(device_driver, of_match_table);  // Device tree match table
    field_init(device, driver_data);
    field_init(device, kobj);           // Embedded kobject for sysfs
    field_init(device, driver);         // Pointer to bound driver
    field_init(bus_type, p);
    field_init(bus_type, name);
    field_init(bus_type, probe);
    field_init(class, p);
    field_init(class, name);
    field_init(kset, kobj);
    field_init(kset, list);
    field_init(kobject, entry);
    field_init(kobject, name);          // Device name
    field_init(of_device_id,compatible);

    // Character device structures
    field_init(char_device_struct, next);
    field_init(char_device_struct, cdev);
    field_init(char_device_struct, name);
    field_init(miscdevice, list);

    // Handle and probe structures
    field_init(handle_parts, pool_index);
    field_init(handle_parts, pool_index_plus_1);
    field_init(kobj_map, probes);
    field_init(probe, data);
    field_init(probe, next);

    field_init(page_owner,order);
    field_init(page_owner,last_migrate_reason);
    field_init(page_owner,gfp_mask);
    field_init(page_owner,handle);
    field_init(page_owner,free_handle);
    field_init(page_owner,ts_nsec);
    field_init(page_owner,free_ts_nsec);
    field_init(page_owner,pid);
    field_init(page_owner,tgid);
    field_init(page_owner,comm);
    field_init(page_owner,free_pid);
    field_init(page_owner,free_tgid);
    struct_init(page_owner);
    field_init(mem_section,page_ext);
    field_init(pglist_data,node_page_ext);
    field_init(page_ext,flags);
    struct_init(page_ext);

    field_init(page_ext_operations,offset);
    field_init(page_ext_operations,size);
    struct_init(page_ext_operations);

    field_init(stack_record,next);
    field_init(stack_record,size);
    field_init(stack_record,handle);
    field_init(stack_record,entries);
    struct_init(stack_record);

    if (BITS64()){
        std::string config = get_config_val("CONFIG_ARM64_VA_BITS");
        int va_bits = std::stoi(config);
        kaddr_mask = GENMASK_ULL((va_bits ? va_bits : 39) - 1, 0);
    } else {
        kaddr_mask = GENMASK_ULL(32 - 1, 0);
    }
    if (csymbol_exists("depot_index")){
        depot_index = read_int(csymbol_value("depot_index"), "depot_index");
    } else if (csymbol_exists("pool_index")){
        depot_index = read_int(csymbol_value("pool_index"), "pool_index");
    } else if (csymbol_exists("pools_num")){
        depot_index = read_int(csymbol_value("pools_num"), "pools_num");
    }
    if (csymbol_exists("stack_slabs")){
        stack_slabs = csymbol_value("stack_slabs");
    } else if (csymbol_exists("stack_pools")){ /* 6.3 and later */
        stack_slabs = csymbol_value("stack_pools");
    }
    /* max_pfn */
    max_pfn = csymbol_exists("max_pfn") ?
        (try_get_symbol_data(TO_CONST_STRING("max_pfn"), sizeof(ulong), &max_pfn), max_pfn) : 0;
    /* min_low_pfn */
    min_low_pfn = csymbol_exists("min_low_pfn") ?
        (try_get_symbol_data(TO_CONST_STRING("min_low_pfn"), sizeof(ulong), &min_low_pfn), min_low_pfn) : 0;

    // Get the offset of page_owner within page_ext structure
    if (csymbol_exists("page_owner_ops")){
        ulong ops_addr = csymbol_value("page_owner_ops");
        page_ext_ops_offset = read_ulong(ops_addr + field_offset(page_ext_operations,offset),"page_owner_ops.offset");
        LOGD("ops_offset: %zu", page_ext_ops_offset);
    }
    // Read page extension flag bit positions
    PAGE_EXT_OWNER = read_enum_val("PAGE_EXT_OWNER");
    PAGE_EXT_OWNER_ALLOCATED = read_enum_val("PAGE_EXT_OWNER_ALLOCATED");
    //print_table();
}

/**
 * Formats a nanosecond timestamp into a human-readable uptime string
 *
 * Converts a timestamp in nanoseconds to a formatted string showing days, hours,
 * minutes, seconds, and nanoseconds in the format "Xd HH:MM:SS.nnnnnnnnn (uptime)".
 * Days are only shown if greater than 0.
 *
 * @param timestamp_ns The timestamp in nanoseconds to format
 * @return A formatted string representation of the uptime
 *
 * Example outputs:
 * - "00:01:30.123456789 (uptime)" for 90.123456789 seconds
 * - "1d 02:30:45.987654321 (uptime)" for 1 day, 2 hours, 30 minutes, 45.987654321 seconds
 */
std::string ParserPlugin::formatTimestamp(uint64_t timestamp_ns) {
    constexpr uint64_t NS_PER_SEC = 1000000000ULL;
    constexpr uint64_t SEC_PER_MIN = 60;
    constexpr uint64_t SEC_PER_HOUR = 3600;
    constexpr uint64_t SEC_PER_DAY = 86400;
    uint64_t total_seconds = timestamp_ns / NS_PER_SEC;
    uint64_t ns_part = timestamp_ns % NS_PER_SEC;
    uint64_t days = total_seconds / SEC_PER_DAY;
    uint64_t remaining_seconds = total_seconds % SEC_PER_DAY;
    uint64_t hours = remaining_seconds / SEC_PER_HOUR;
    remaining_seconds %= SEC_PER_HOUR;
    uint64_t minutes = remaining_seconds / SEC_PER_MIN;
    uint64_t seconds = remaining_seconds % SEC_PER_MIN;
    std::ostringstream oss;
    oss.imbue(std::locale::classic());
    if (days > 0) {
        oss << days << "d ";
    }
    oss << std::setfill('0')
        << std::setw(2) << hours << ':'
        << std::setw(2) << minutes << ':'
        << std::setw(2) << seconds << '.'
        << std::setw(9) << ns_part << " (uptime)";
    return oss.str();
}

bool ParserPlugin::isNumber(const std::string& str) {
    regex_t decimal, hex;
    bool result = false;
    if (regcomp(&decimal, "^-?[0-9]+$", REG_EXTENDED)) {
        LOGE("Could not compile decimal regex\n");
        return false;
    }
    if (regcomp(&hex, "^0[xX][0-9a-fA-F]+$", REG_EXTENDED)) {
        LOGE("Could not compile hex regex \n");
        regfree(&decimal);
        return false;
    }
    if (!regexec(&decimal, str.c_str(), 0, NULL, 0) || !regexec(&hex, str.c_str(), 0, NULL, 0)) {
        result = true;
    }
    regfree(&decimal);
    regfree(&hex);
    return result;
}

/**
 * Formats a byte size into a human-readable string with appropriate units
 *
 * Converts a size in bytes to a formatted string using the most appropriate
 * unit (B, KB, MB, GB). The function automatically selects the largest unit
 * where the value is >= 1, and uses appropriate decimal precision to avoid
 * unnecessary decimal places for whole numbers.
 *
 * @param size The size in bytes to format
 * @return A formatted string with the size and appropriate unit
 *
 * Example outputs:
 * - "512B" for 512 bytes
 * - "1KB" for 1024 bytes
 * - "1.5KB" for 1536 bytes
 * - "2MB" for 2097152 bytes
 * - "1.25GB" for 1342177280 bytes
 */
std::string ParserPlugin::csize(uint64_t size){
    constexpr uint64_t KB = 1024ULL;
    constexpr uint64_t MB = KB * 1024ULL;
    constexpr uint64_t GB = MB * 1024ULL;
    std::ostringstream oss;
    oss.imbue(std::locale::classic());
    if (size < KB) {
        oss << size << " B";
    } else if (size < MB) {
        double sizeInKB = static_cast<double>(size) / KB;
        oss << std::fixed << std::setprecision(sizeInKB == static_cast<uint64_t>(sizeInKB) ? 0 : 2) << sizeInKB << " KB";
    } else if (size < GB) {
        double sizeInMB = static_cast<double>(size) / MB;
        oss << std::fixed << std::setprecision(sizeInMB == static_cast<uint64_t>(sizeInMB) ? 0 : 2) << sizeInMB << " MB";
    } else {
        double sizeInGB = static_cast<double>(size) / GB;
        oss << std::fixed << std::setprecision(sizeInGB == static_cast<uint64_t>(sizeInGB) ? 0 : 2) << sizeInGB << " GB";
    }
    return oss.str();
}

/**
 * Formats a byte size into a human-readable string with specified unit and precision
 *
 * Converts a size in bytes to a formatted string using the specified unit
 * (KB, MB, GB, or B for bytes). Unlike the automatic unit selection version,
 * this function forces the output to use the exact unit specified by the caller
 * with the given decimal precision.
 *
 * @param size The size in bytes to format
 * @param unit The unit to use for formatting (KB=1024, MB=1048576, GB=1073741824, or other for bytes)
 * @param precision The number of decimal places to display
 * @return A formatted string with the size in the specified unit
 *
 * Example outputs:
 * - csize(1536, 1024, 2) returns "1.50 KB"
 * - csize(2097152, 1048576, 0) returns "2 MB"
 * - csize(512, 0, 0) returns "512B"
 */
std::string ParserPlugin::csize(uint64_t size, int unit, int precision){
    constexpr uint64_t KB = 1024ULL;
    constexpr uint64_t MB = KB * 1024ULL;
    constexpr uint64_t GB = MB * 1024ULL;
    std::ostringstream oss;
    oss.imbue(std::locale::classic());
    switch (unit) {
        case KB:
            oss << std::fixed << std::setprecision(precision) << (static_cast<double>(size) / KB) << " KB";
            break;
        case MB:
            oss << std::fixed << std::setprecision(precision) << (static_cast<double>(size) / MB) << " MB";
            break;
        case GB:
            oss << std::fixed << std::setprecision(precision) << (static_cast<double>(size) / GB) << " GB";
            break;
        default:
            oss << size << "B";
            break;
    }
    return oss.str();
}

/**
 * Find a task context by process ID (PID)
 *
 * This function searches for a task context structure corresponding to the given PID.
 * It first attempts to use the task table directly for better performance if the PID
 * is within the range of running tasks. If not found there, it falls back to iterating
 * through all processes in the system.
 *
 * @param pid The process ID to search for
 * @return Pointer to the task_context structure if found, nullptr if not found or invalid PID
 */
struct task_context* ParserPlugin::find_proc(ulong pid){
    if (pid == 0) {
        return nullptr;
    }
    // Check if we can use the task table directly for better performance
    if (pid <= RUNNING_TASKS()) {
        struct task_context *tc = FIRST_CONTEXT();
        for (size_t i = 0; i < RUNNING_TASKS(); i++, tc++) {
            if (tc->pid == pid) {
                return tc;
            }
        }
    }
    // Fallback to process iteration
    for(ulong task_addr: for_each_process()){
        struct task_context *tc = task_to_context(task_addr);
        if (!tc){
            continue;
        }
        if (tc->pid == pid){
            return tc;
        }
    }
    return nullptr;
}

/**
 * Find a task context by process name (command)
 *
 * This function searches for a task context structure corresponding to the given process name.
 * It first attempts to use the task table directly for better performance if available,
 * then falls back to iterating through all processes in the system.
 *
 * @param name The process name (command) to search for
 * @return Pointer to the task_context structure if found, nullptr if not found or empty name
 */
struct task_context* ParserPlugin::find_proc(const std::string& name){
    if (name.empty()) {
        return nullptr;
    }
    // Check running tasks first for better performance
    struct task_context *tc = FIRST_CONTEXT();
    for (size_t i = 0; i < RUNNING_TASKS(); i++, tc++) {
        if (tc->comm == name) {
            return tc;
        }
    }
    // Fallback to process iteration
    for(ulong task_addr: for_each_process()){
        struct task_context *tc = task_to_context(task_addr);
        if (!tc){
            continue;
        }
        if (tc->comm == name){
            return tc;
        }
    }
    return nullptr;
}

/**
 * Check if a page is a buddy system page (free page)
 *
 * This function determines whether a given page is currently managed by the
 * kernel's buddy allocator system as a free page. The detection method varies
 * based on kernel version due to changes in the page structure fields used
 * to track buddy pages.
 *
 * For kernels >= 4.19.0:
 * - Uses the page_type field with specific bit patterns to identify buddy pages
 * - Checks if (page_type & 0xf0000080) == 0xf0000000 to detect buddy pages
 *
 * For older kernels:
 * - Uses the _mapcount field with a magic value (0xffffff80) to identify buddy pages
 * - This was the traditional method before page_type was introduced
 *
 * @param page_addr The kernel virtual address of the page structure to check
 * @return true if the page is a buddy (free) page, false otherwise
 */
bool ParserPlugin::page_buddy(ulong page_addr){
    if (THIS_KERNEL_VERSION >= LINUX(4, 19, 0)){
        uint page_type = read_uint(page_addr + field_offset(page,page_type),"page_type");
        return ((page_type & 0xf0000080) == 0xf0000000);
    }
    uint mapcount = read_int(page_addr + field_offset(page,_mapcount),"_mapcount");
    return (mapcount == 0xffffff80);
}

/**
 * Get the reference count of a page structure
 *
 * This function retrieves the reference count (usage count) of a page structure,
 * which indicates how many references exist to this page. The implementation
 * varies based on kernel version due to changes in the page structure fields
 * used to track reference counts.
 *
 * For kernels >= 4.6.0:
 * - Uses the _refcount field which replaced the older _count field
 * - This change was part of the atomic_t -> refcount_t conversion
 *
 * For older kernels (< 4.6.0):
 * - Uses the legacy _count field for reference counting
 * - This was the original field name before the refcount_t introduction
 *
 * @param page_addr The kernel virtual address of the page structure to check
 * @return The reference count of the page (0 means the page is free)
 */
int ParserPlugin::page_count(ulong page_addr){
    if (THIS_KERNEL_VERSION < LINUX(4, 6, 0)){
        return read_int(page_addr + field_offset(page,_count),"_count");
    }
    return read_int(page_addr + field_offset(page,_refcount),"_refcount");
}

void ParserPlugin::initialize(void){
    cmd_help = new char*[help_str_list.size()+1];
    for (size_t i = 0; i < help_str_list.size(); ++i) {
        cmd_help[i] = TO_CONST_STRING(help_str_list[i].c_str());
    }
    cmd_help[help_str_list.size()] = nullptr;
}

/**
 * Initialize type information for a structure type
 *
 * This function creates and stores type information for a given structure type
 * in the global type table. It creates a Typeinfo object that contains metadata
 * about the structure such as its size and other properties needed for memory
 * analysis and field access operations.
 *
 * @param type The name of the structure type to initialize (e.g., "task_struct", "mm_struct")
 */
void ParserPlugin::type_init(const std::string& type, bool is_anon){
    std::string name = type;
    typetable[name] = std::make_unique<Typeinfo>(type, is_anon);
}

/**
 * Initialize type information for a specific field within a structure type
 *
 * This function creates and stores type information for a given field within a structure type
 * in the global type table. It creates a Typeinfo object that contains metadata about the
 * specific field such as its offset within the structure and size, which are needed for
 * memory analysis and field access operations.
 *
 * @param type The name of the structure type containing the field (e.g., "task_struct", "mm_struct")
 * @param field The name of the field within the structure (e.g., "pid", "comm", "mm")
 *
 */
void ParserPlugin::type_init(const std::string& type,const std::string& field, bool is_anon){
    std::string name = type + "@" + field;
    typetable[name] = std::make_unique<Typeinfo>(type,field, is_anon);
}

/**
 * Get the byte offset of a specific field within a structure type
 *
 * This function retrieves the byte offset of a field within a structure from
 * the global type table. The offset information is essential for accessing
 * structure members when reading kernel memory or performing field-based
 * operations on kernel data structures.
 *
 * @param type The name of the structure type containing the field (e.g., "task_struct", "mm_struct")
 * @param field The name of the field within the structure (e.g., "pid", "comm", "mm")
 * @return The byte offset of the field within the structure, or -1 if the type information is not found
 *
 */
int ParserPlugin::type_offset(const std::string& type,const std::string& field){
    std::string name = type + "@" + field;
    auto it = typetable.find(name);
    if (it != typetable.end()) {
        return it->second->offset();
    } else {
        LOGE("Error: Typeinfo not found for %s, pls add field_init(%s,%s)\n",name.c_str(),type.c_str(),field.c_str());
        return -1;
    }
}

/**
 * Get the byte size of a specific field within a structure type
 *
 * This function retrieves the byte size of a field within a structure from
 * the global type table. The size information is essential for memory
 * allocation, buffer management, and ensuring proper data type handling
 * when working with kernel data structures.
 *
 * @param type The name of the structure type containing the field (e.g., "task_struct", "mm_struct")
 * @param field The name of the field within the structure (e.g., "pid", "comm", "mm")
 * @return The byte size of the field, or -1 if the type information is not found
 *
 */
int ParserPlugin::type_size(const std::string& type,const std::string& field){
    std::string name = type + "@" + field;
    auto it = typetable.find(name);
    if (it != typetable.end()) {
        return it->second->size();
    } else {
        LOGE("Error: Typeinfo not found for %s, pls add field_size(%s,%s)\n",name.c_str(),type.c_str(),field.c_str());
        return -1;
    }
}

/**
 * Get the byte size of a structure type
 *
 * This function retrieves the byte size of a structure type from
 * the global type table. The size information is essential for memory
 * allocation, buffer management, and ensuring proper data type handling
 * when working with kernel data structures.
 *
 * @param type The name of the structure type (e.g., "task_struct", "mm_struct")
 * @return The byte size of the structure, or -1 if the type information is not found
 *
 */
int ParserPlugin::type_size(const std::string& type){
    std::string name = type;
    auto it = typetable.find(name);
    if (it != typetable.end()) {
        return it->second->size();
    } else {
        LOGE("Error: Typeinfo not found for %s, pls add field_size(%s)\n",name.c_str(),type.c_str());
        return -1;
    }
}

/**
 * Print a backtrace of the current call stack
 *
 * This function generates and prints a backtrace showing the current call stack
 * using the GNU backtrace functionality. It captures up to MAX_FRAMES stack frames
 * and prints each frame's symbol information to the output file pointer.
 *
 * The backtrace includes function names, addresses, and shared library information
 * when available. If backtrace generation fails or no frames are available,
 * appropriate error messages are printed.
 *
 */
void ParserPlugin::print_backtrace(){
    constexpr int MAX_FRAMES = 100;
    void *buffer[MAX_FRAMES];
    int nptrs = backtrace(buffer, MAX_FRAMES);
    if (nptrs <= 0) {
        LOGE("No backtrace available\n");
        return;
    }
    char **strings = backtrace_symbols(buffer, nptrs);
    if (strings == nullptr) {
        LOGE("backtrace_symbols failed\n");
        return;
    }
    for (int i = 0; i < nptrs; i++) {
        PRINT("%s\n", strings[i]);
    }
    std::free(strings);
}

/**
 * Print a formatted table of all registered type information
 *
 * This function outputs a formatted table showing all type information stored in the
 * global typetable map. For each type/field combination, it displays the name,
 * offset within the structure, and size of the field. The output is formatted in
 * columns for easy reading.
 *
 * The table format is:
 * - Column 1: Type name or "type@field" (45 characters, left-justified)
 * - Column 2: Offset information (15 characters, left-justified)
 * - Column 3: Size information
 *
 */
void ParserPlugin::print_table(){
    char buf[BUFSIZE];
    for (const auto& pair : typetable) {
        sprintf(buf, "%s", pair.first.c_str());
        PRINT("%s",mkstring(buf, 45, LJUST, buf));
        sprintf(buf, ": offset:%d", pair.second.get()->m_offset);
        PRINT("%s",mkstring(buf, 15, LJUST, buf));
        PRINT(" size:%d\n",pair.second.get()->m_size);
    }
}

/**
 * Generate a vector of all valid page frame numbers (PFNs) in the system
 *
 * This function creates a vector containing all page frame numbers from min_low_pfn
 * to max_pfn, representing the range of physical memory pages managed by the kernel.
 * PFNs are used to identify physical memory pages and are essential for memory
 * management operations.
 *
 * The function uses member variables max_pfn and min_low_pfn that are initialized
 * in the constructor from kernel symbols. If these symbols don't exist or have
 * zero values, an empty vector is returned.
 *
 * @return Vector of page frame numbers (PFNs) covering the system's physical memory range
 *         Returns empty vector if kernel symbols are unavailable or invalid
 *
 */
std::vector<ulong> ParserPlugin::for_each_pfn(){
    std::vector<ulong> res;
    // Early return if symbols don't exist
    if (!csymbol_exists("max_pfn") || !csymbol_exists("min_low_pfn")) {
        return res;
    }
    // Use member variables that are already initialized in constructor
    if (max_pfn == 0 || min_low_pfn == 0) {
        return res;
    }
    // Reserve capacity to avoid reallocations
    size_t pfn_count = max_pfn - min_low_pfn;
    res.reserve(pfn_count);
    // Use more efficient loop
    for (ulong pfn = min_low_pfn; pfn < max_pfn; ++pfn) {
        res.push_back(pfn);
    }
    return res;
}

/**
 * Retrieve all page addresses associated with a given address_space mapping
 *
 * This function traverses the page cache of an address_space structure to collect
 * all pages that are currently cached for a particular file or mapping. The pages
 * are stored in either an xarray (newer kernels) or radix tree (older kernels)
 * data structure within the address_space.
 *
 * The function handles kernel version compatibility by checking for different
 * field names used across kernel versions:
 * - Modern kernels: uses i_pages field with xarray structure
 * - Older kernels: uses page_tree field with radix tree structure
 *
 * @param i_mapping The kernel virtual address of the address_space structure
 * @return Vector of page addresses found in the mapping's page cache
 *         Returns empty vector if mapping is invalid or no pages found
 *
 */
std::vector<ulong> ParserPlugin::for_each_address_space(ulong i_mapping) {
    std::vector<ulong> res;
    if (!is_kvaddr(i_mapping)) {
        return res;
    }
    int i_pages_offset = field_offset(address_space, i_pages);
    if (i_pages_offset == -1) {
        i_pages_offset = field_offset(address_space, page_tree);
        if (i_pages_offset == -1) {
            return res;
        }
    }
    ulong root = i_mapping + i_pages_offset;
    std::string i_pages_type = MEMBER_TYPE_NAME(TO_CONST_STRING("address_space"), TO_CONST_STRING("i_pages"));
    return (i_pages_type == "xarray") ? for_each_xarray(root) : for_each_radix(root);
}

/**
 * Retrieve all inode addresses from file pages in the system
 *
 * This function traverses all file pages in the system to extract unique inode
 * addresses. It validates each page's mapping and ensures the inode is properly
 * associated with an address_space structure before including it in the results.
 *
 * The function performs the following validation steps for each file page:
 * 1. Checks if the page has a valid mapping (address_space)
 * 2. Verifies the mapping has valid address_space operations (a_ops)
 * 3. Ensures the mapping has a valid host inode
 * 4. Confirms the inode's i_mapping field points back to the same mapping
 * 5. Uses a set for automatic deduplication of inode addresses
 *
 * @return Vector of unique inode addresses found in the system's file pages
 *         Returns empty vector if no valid inodes are found
 *
 */
std::vector<ulong> ParserPlugin::for_each_inode(){
    std::vector<ulong> res;
    std::set<ulong> inode_set;
    // Reserve capacity based on estimated file pages
    res.reserve(1024);
    for (const auto& page : for_each_file_page()) {
        ulong mapping = read_pointer(page + field_offset(page,mapping),"mapping");
        if (!is_kvaddr(mapping)){
            continue;
        }
        ulong ops = read_pointer(mapping + field_offset(address_space,a_ops),"a_ops");
        if (!is_kvaddr(ops)){
            continue;
        }
        ulong inode = read_pointer(mapping + field_offset(address_space,host),"host");
        if (!is_kvaddr(inode)){
            continue;
        }
        ulong i_mapping = read_pointer(inode + field_offset(inode,i_mapping),"i_mapping");
        if (!is_kvaddr(i_mapping) || mapping != i_mapping){
            continue;
        }
        // Use set for O(log n) insertion and automatic deduplication
        if (inode_set.insert(inode).second) {
            res.push_back(inode);
        }
    }
    return res;
}

/**
 * Retrieve all file-backed pages in the system
 *
 * This function traverses all page frame numbers (PFNs) in the system to identify
 * and collect pages that are backed by files (as opposed to anonymous pages).
 * File pages are those associated with files in the filesystem and have valid
 * address_space mappings.
 *
 * The function performs the following validation steps for each page:
 * 1. Converts PFN to page structure address and validates it
 * 2. Skips buddy (free) pages and pages with zero reference count
 * 3. Checks if the page has a valid mapping (address_space)
 * 4. Filters out anonymous pages by checking the mapping's LSB (bit 0)
 *    - If bit 0 is set, it's an anonymous page (skipped)
 *    - If bit 0 is clear, it's a file page (included)
 *
 * @return Vector of page addresses for all file-backed pages in the system
 *         Returns empty vector if no file pages are found
 *
 */
std::vector<ulong> ParserPlugin::for_each_file_page(){
    std::vector<ulong> res;
    res.reserve(4096); // Reserve capacity for better performance
    for (const auto& pfn : for_each_pfn()) {
        ulong page = pfn_to_page(pfn);
        if (!is_kvaddr(page)){
            continue;
        }
        if(page_buddy(page) || page_count(page) == 0){
            continue;
        }
        ulong mapping = read_pointer(page + field_offset(page,mapping),"mapping");
        if (!is_kvaddr(mapping)){
            continue;
        }
        if((mapping & 0x1) == 1){ // skip anon page
            continue;
        }
        res.push_back(page);
    }
    return res;
}

/**
 * Retrieve all anonymous pages in the system
 *
 * This function traverses all page frame numbers (PFNs) in the system to identify
 * and collect pages that are anonymous (not backed by files). Anonymous pages
 * include stack pages, heap pages, and other memory allocations that don't
 * correspond to files in the filesystem.
 *
 * The function performs the following validation steps for each page:
 * 1. Converts PFN to page structure address and validates it
 * 2. Skips buddy (free) pages and pages with zero reference count
 * 3. Checks if the page has a valid mapping (address_space)
 * 4. Filters out file pages by checking the mapping's LSB (bit 0)
 *    - If bit 0 is clear, it's a file page (skipped)
 *    - If bit 0 is set, it's an anonymous page (included)
 *
 * @return Vector of page addresses for all anonymous pages in the system
 *         Returns empty vector if no anonymous pages are found
 *
 */
std::vector<ulong> ParserPlugin::for_each_anon_page(){
    std::vector<ulong> res;
    res.reserve(4096); // Reserve capacity for better performance
    for (const auto& pfn : for_each_pfn()) {
        ulong page = pfn_to_page(pfn);
        if (!is_kvaddr(page)){
            continue;
        }
        if(page_buddy(page) || page_count(page) == 0){
            continue;
        }
        ulong mapping = read_pointer(page + field_offset(page,mapping),"mapping");
        if (!is_kvaddr(mapping)){
            continue;
        }
        if((mapping & 0x1) == 0){ // skip file page
            continue;
        }
        res.push_back(page);
    }
    return res;
}

/**
 * Retrieve all entries from a radix tree data structure
 *
 * This function traverses a radix tree starting from the given root node and
 * collects all valid kernel virtual addresses stored within the tree. Radix trees
 * are used extensively in the Linux kernel for efficient storage and retrieval
 * of data indexed by integer keys (such as page cache lookups by page index).
 *
 * The function performs a two-pass operation:
 * 1. First pass: Count the total number of entries in the radix tree
 * 2. Second pass: Gather all the entries into a temporary buffer
 * 3. Filter and return only valid kernel virtual addresses
 *
 * @param root_rnode The kernel virtual address of the radix tree root node
 * @return Vector of kernel virtual addresses found in the radix tree
 *         Returns empty vector if root is invalid or no entries found
 *
 */
std::vector<ulong> ParserPlugin::for_each_radix(ulong root_rnode){
    std::vector<ulong> res;
    if (!is_kvaddr(root_rnode)) {
        return res;
    }
    size_t entry_num = do_radix_tree(root_rnode, RADIX_TREE_COUNT, NULL);
    if (entry_num == 0) {
        return res;
    }
    struct list_pair *entry_list = (struct list_pair *)GETBUF((entry_num + 1) * sizeof(struct list_pair));
    entry_list[0].index = entry_num;
    do_radix_tree(root_rnode, RADIX_TREE_GATHER, entry_list);
    res.reserve(entry_num);
    for (size_t i = 0; i < entry_num; ++i){
        ulong addr = (ulong)entry_list[i].value;
        if (is_kvaddr(addr)) {
            res.push_back(addr);
        }
    }
    FREEBUF(entry_list);
    return res;
}

/**
 * Retrieve all entries from a maple tree data structure
 *
 * This function traverses a maple tree starting from the given root address and
 * collects all valid kernel virtual addresses stored within the tree. Maple trees
 * are used in modern Linux kernels (6.1+) as a replacement for red-black trees
 * in memory management, particularly for storing VMA (Virtual Memory Area) entries.
 *
 * The function performs a two-pass operation:
 * 1. First pass: Count the total number of entries in the maple tree
 * 2. Second pass: Gather all the entries into a temporary buffer
 * 3. Filter and return only valid kernel virtual addresses
 *
 * @param maptree_addr The kernel virtual address of the maple tree root
 * @return Vector of kernel virtual addresses found in the maple tree
 *         Returns empty vector if root is invalid or no entries found
 *
 */
std::vector<ulong> ParserPlugin::for_each_mptree(ulong maptree_addr){
    std::vector<ulong> res;
    if (!is_kvaddr(maptree_addr)) {
        return res;
    }
    size_t entry_num = do_maple_tree(maptree_addr, MAPLE_TREE_COUNT, NULL);
    if (entry_num == 0) {
        return res;
    }
    struct list_pair *entry_list = (struct list_pair *)GETBUF(entry_num * sizeof(struct list_pair));
    do_maple_tree(maptree_addr, MAPLE_TREE_GATHER, entry_list);
    res.reserve(entry_num);
    for (size_t i = 0; i < entry_num; ++i){
        ulong addr = (ulong)entry_list[i].value;
        if (is_kvaddr(addr)) {
            res.push_back(addr);
        }
    }
    FREEBUF(entry_list);
    return res;
}

/**
 * Retrieve all entries from an xarray data structure
 *
 * This function traverses an xarray starting from the given root address and
 * collects all valid kernel virtual addresses stored within the array. Xarrays
 * are used in modern Linux kernels as a replacement for radix trees in various
 * subsystems, particularly for page cache management and other indexed data storage.
 *
 * The function performs a two-pass operation:
 * 1. First pass: Count the total number of entries in the xarray
 * 2. Second pass: Gather all the entries into a temporary buffer
 * 3. Filter and return only valid kernel virtual addresses
 *
 * @param xarray_addr The kernel virtual address of the xarray root
 * @return Vector of kernel virtual addresses found in the xarray
 *         Returns empty vector if root is invalid or no entries found
 *
 */
std::vector<ulong> ParserPlugin::for_each_xarray(ulong xarray_addr){
    std::vector<ulong> res;
    if (!is_kvaddr(xarray_addr)) {
        return res;
    }
    size_t entry_num = do_xarray(xarray_addr, XARRAY_COUNT, NULL);
    if (entry_num == 0) {
        return res;
    }
    struct list_pair *entry_list = (struct list_pair *)GETBUF(entry_num * sizeof(struct list_pair));
    do_xarray(xarray_addr, XARRAY_GATHER, entry_list);
    res.reserve(entry_num);
    for (size_t i = 0; i < entry_num; ++i) {
        ulong addr = (ulong)entry_list[i].value;
        if (is_kvaddr(addr)) {
            res.push_back(addr);
        }
    }
    FREEBUF(entry_list);
    return res;
}

/**
 * Retrieve all entries from a red-black tree data structure
 *
 * This function traverses a red-black tree starting from the given root node and
 * collects all valid kernel virtual addresses stored within the tree. Red-black trees
 * are used extensively in the Linux kernel for efficient storage and retrieval
 * of data with guaranteed O(log n) operations, such as in memory management,
 * process scheduling, and file system operations.
 *
 * The function performs a two-pass operation:
 * 1. First pass: Count the total number of entries in the red-black tree
 * 2. Second pass: Gather all the entries into a temporary buffer
 * 3. Filter and return only valid kernel virtual addresses, adjusting for node offset
 *
 * @param rb_root The kernel virtual address of the red-black tree root node
 * @param offset The byte offset of the rb_node within the containing structure
 * @return Vector of kernel virtual addresses found in the red-black tree
 *         Returns empty vector if root is invalid or no entries found
 *
 */
std::vector<ulong> ParserPlugin::for_each_rbtree(ulong rb_root, int offset) {
    std::vector<ulong> res;
    if (!is_kvaddr(rb_root)) {
        return res;
    }
    struct tree_data td;
    BZERO(&td, sizeof(struct tree_data));
    td.flags |= TREE_NODE_POINTER;
    td.start = rb_root;
    td.node_member_offset = offset;
    hq_open();
    int cnt = do_rbtree(&td);
    if (cnt == 0) {
        hq_close();
        return res;
    }
    ulong *treeList = (ulong *)GETBUF(cnt * sizeof(ulong));
    retrieve_list(treeList, cnt);
    res.reserve(cnt);
    for (int i = 0; i < cnt; ++i) {
        if (is_kvaddr(treeList[i])) {
            res.push_back(treeList[i] - td.node_member_offset);
        }
    }
    FREEBUF(treeList);
    hq_close();
    return res;
}

/**
 * Retrieve all entries from a linked list data structure
 *
 * This function traverses a Linux kernel linked list starting from the given
 * list head and collects all valid kernel virtual addresses of the containing
 * structures. It uses the crash utility's do_list() function to handle the
 * list traversal and automatically manages memory for the temporary list buffer.
 *
 * The function performs validation and error handling:
 * 1. Validates the list head address as a kernel virtual address
 * 2. Checks if the list is empty before attempting traversal
 * 3. Handles incomplete lists gracefully with RETURN_ON_LIST_ERROR flag
 * 4. Filters out invalid addresses from the results
 *
 * @param list_head The kernel virtual address of the list_head structure
 * @param offset The byte offset of the list_head within the containing structure
 * @return Vector of kernel virtual addresses of structures containing list nodes
 *         Returns empty vector if list_head is invalid or list is empty
 *
 */
std::vector<ulong> ParserPlugin::for_each_list(ulong list_head, int offset) {
    std::vector<ulong> res;
    if (!is_kvaddr(list_head)) return res;
    // Quick check if list is empty
    ulong next = read_pointer(list_head + field_offset(list_head, next), "list_head_next");
    if (!next || next == list_head) {
        return res;
    }
    struct list_data ld;
    BZERO(&ld, sizeof(struct list_data));
    ld.flags |= (LIST_ALLOCATE|RETURN_ON_LIST_ERROR);
    /*
    case : invalid list entry: 4000000000000000
    readflag = ld->flags & RETURN_ON_LIST_ERROR ? (RETURN_ON_ERROR|QUIET) : FAULT_ON_ERROR; in tools.c
    Even if the list is incomplete, we should ensure that the existing elements can be used normally.
    */
    readmem(list_head, KVADDR, &ld.start,sizeof(ulong), TO_CONST_STRING("for_each_list list_head"), FAULT_ON_ERROR);
    ld.end = list_head;
    // ld.member_offset = offset;
    ld.list_head_offset = offset;
    if (empty_list(ld.start)) return res;
    int cnt = do_list(&ld);
    if (cnt == 0) return res;
    res.reserve(cnt);
    for (int i = 0; i < cnt; ++i) {
        ulong node_addr = ld.list_ptr[i];
        if (is_kvaddr(node_addr)) {
            res.push_back(node_addr);
        }
    }
    FREEBUF(ld.list_ptr);
    return res;
}

/**
 * Retrieve all entries from a hash list (hlist) data structure
 *
 * This function traverses a Linux kernel hash list starting from the given
 * hlist_head and collects all valid kernel virtual addresses of the containing
 * structures. Hash lists are used extensively in the Linux kernel for efficient
 * hash table implementations, such as in the process hash table, inode cache,
 * and various other kernel subsystems.
 *
 * The function performs validation and error handling:
 * 1. Validates the hlist_head address as a kernel virtual address
 * 2. Reads the first node pointer from the hlist_head
 * 3. Uses the crash utility's do_list() function for traversal
 * 4. Handles incomplete lists gracefully with RETURN_ON_LIST_ERROR flag
 * 5. Filters out invalid addresses from the results
 *
 * @param hlist_head The kernel virtual address of the hlist_head structure
 * @param offset The byte offset of the hlist_node within the containing structure
 * @return Vector of kernel virtual addresses of structures containing hlist nodes
 *         Returns empty vector if hlist_head is invalid or list is empty
 */
std::vector<ulong> ParserPlugin::for_each_hlist(ulong hlist_head, int offset) {
    std::vector<ulong> res;
    if (!is_kvaddr(hlist_head)) {
        return res;
    }
    ulong first = read_pointer(hlist_head, "hlist_head");
    if (!is_kvaddr(first)) {
        return res;
    }
    struct list_data ld;
    BZERO(&ld, sizeof(struct list_data));
    ld.flags |= (LIST_ALLOCATE | RETURN_ON_LIST_ERROR);
    ld.start = first;
    ld.list_head_offset = offset;
    if (empty_list(ld.start)) {
        return res;
    }
    int cnt = do_list(&ld);
    if (cnt == 0) {
        return res;
    }
    res.reserve(cnt);
    for (int i = 0; i < cnt; ++i) {
        ulong node_addr = ld.list_ptr[i];
        if (is_kvaddr(node_addr)) {
            res.push_back(node_addr);
        }
    }
    FREEBUF(ld.list_ptr);
    return res;
}

/**
 * Retrieve all process task addresses from the kernel's process list
 *
 * This function traverses the kernel's global process list to collect all
 * task_struct addresses for processes that have their own memory management
 * context (mm_struct). It filters out kernel threads and duplicate entries
 * to return only valid user processes.
 *
 * The function performs the following operations:
 * 1. Starts from the init_task and traverses the circular task list
 * 2. Filters out tasks without memory management structures (kernel threads)
 * 3. Uses a hash set to eliminate duplicate task addresses for efficiency
 * 4. Validates each task exists in the system before including it
 *
 * @return Vector of task_struct addresses for all valid processes in the system
 *         Returns empty vector if no processes found or init_task is invalid
 *
 */
std::vector<ulong> ParserPlugin::for_each_process(){
    std::vector<ulong> res_list;
    res_list.reserve(RUNNING_TASKS() + 256); // Reserve space for better performance
    ulong init_task = csymbol_value("init_task");
    int offset = field_offset(task_struct,tasks);
    ulong list_head_addr = init_task + offset;
    std::vector<ulong> task_list = for_each_list(list_head_addr, offset);
    std::unordered_set<ulong> seen_tasks; // Use hash set for O(1) lookup
    seen_tasks.reserve(task_list.size());
    for (const auto& task_addr : task_list) {
        if (task_addr == init_task) continue;
        // Check if already seen first (fastest check)
        if (seen_tasks.find(task_addr) != seen_tasks.end()) continue;
        ulong mm_struct = read_pointer(task_addr + field_offset(task_struct,mm), "task_struct_mm");
        if (mm_struct == 0) continue;
        if (!task_exists(task_addr)) continue;
        seen_tasks.insert(task_addr);
        res_list.push_back(task_addr);
    }
    return res_list;
}

/**
 * Retrieve all thread task addresses from the kernel's task table
 *
 * This function collects all task_struct addresses from the crash utility's
 * internal task table, which includes both processes and threads. Unlike
 * for_each_process() which filters for processes with memory management
 * contexts, this function returns all tasks including kernel threads.
 *
 * The function uses the crash utility's task context table which provides
 * efficient access to all running tasks without needing to traverse kernel
 * data structures. This makes it faster than iterating through kernel lists
 * but limits results to tasks that were active when the crash dump was taken.
 *
 * @return Vector of task_struct addresses for all threads/tasks in the system
 *         Includes both user processes and kernel threads
 *         Returns empty vector if no tasks are found
 *
 */
std::vector<ulong> ParserPlugin::for_each_threads(){
    std::vector<ulong> task_list;
    task_list.reserve(RUNNING_TASKS() + 1); // Reserve capacity for better performance
    struct task_context* tc = FIRST_CONTEXT();
    for (size_t i = 0; i <= RUNNING_TASKS(); i++, tc++){
        task_list.push_back(tc->task);
    }
    return task_list;
}

/**
 * Retrieve all Virtual Memory Area (VMA) addresses for a given task
 *
 * This function traverses the memory management structures of a task to collect
 * all VMA (Virtual Memory Area) addresses. VMAs represent contiguous virtual
 * memory regions within a process's address space, such as code segments,
 * data segments, heap, stack, and memory-mapped files.
 *
 * The function handles kernel version compatibility:
 * - For kernels < 6.1.0: Uses linked list traversal via mm_struct->mmap
 * - For kernels >= 6.1.0: Uses maple tree traversal via mm_struct->mm_mt
 *
 * @param task_addr The kernel virtual address of the task_struct to analyze
 * @return Vector of VMA addresses for the task's memory regions
 *         Returns empty vector if task has no mm_struct or invalid task_addr
 *
 */
std::vector<ulong> ParserPlugin::for_each_vma(ulong task_addr){
    std::vector<ulong> vma_list;
    ulong mm_addr = read_pointer(task_addr + field_offset(task_struct,mm), "task_struct_mm");
    if (!is_kvaddr(mm_addr)) return vma_list;

    if (THIS_KERNEL_VERSION < LINUX(6,1,0)){
        ulong vma_addr = read_pointer(mm_addr + field_offset(mm_struct,mmap), "mm_struct_mmap");
        while (is_kvaddr(vma_addr)){
            vma_list.push_back(vma_addr);
            vma_addr = read_pointer(vma_addr + field_offset(vm_area_struct,vm_next), "vm_area_struct_next");
        }
    } else {
        ulong mm_mt_addr = mm_addr + field_offset(mm_struct,mm_mt);
        vma_list = for_each_mptree(mm_mt_addr);
    }
    return vma_list;
}

/**
 * Retrieve all character device addresses from the kernel's character device table
 *
 * This function traverses the kernel's character device hash table (chrdevs) to collect
 * all character device structure addresses. The chrdevs table is a hash table where each
 * bucket contains a linked list of char_device_struct entries representing registered
 * character devices in the system.
 *
 * The function performs the following operations:
 * 1. Checks if the chrdevs symbol exists in the kernel
 * 2. Gets the array length of the chrdevs hash table
 * 3. Iterates through each hash bucket in the table
 * 4. For each non-empty bucket, traverses the linked list of devices
 * 5. Collects all valid character device addresses
 *
 * @return Vector of char_device_struct addresses for all character devices in the system
 *         Returns empty vector if chrdevs symbol doesn't exist or no devices found
 *
 */
std::vector<ulong> ParserPlugin::for_each_char_device(){
    std::vector<ulong> chardev_list;
    if (!csymbol_exists("chrdevs")){
        return chardev_list;
    }
    size_t len = get_array_length(TO_CONST_STRING("chrdevs"), NULL, 0);
    ulong devs_addr = csymbol_value("chrdevs");
    // Reserve capacity to avoid reallocations
    chardev_list.reserve(len * 4); // Estimate 4 devices per array slot
    for (size_t i = 0; i < len; i++){
        ulong chardev_addr = read_pointer(devs_addr + (i * sizeof(void *)),"chardev_addr");
        if (!is_kvaddr(chardev_addr)){
            continue;
        }
        // Traverse the linked list for this hash bucket
        for (ulong next_dev_addr = chardev_addr; is_kvaddr(next_dev_addr);
             next_dev_addr = read_pointer(next_dev_addr + field_offset(char_device_struct,next),"next")){
            chardev_list.push_back(next_dev_addr);
        }
    }
    return chardev_list;
}

/**
 * Retrieve all device addresses from a kernel object map (kobj_map)
 *
 * This function traverses a kernel object map structure to collect all device
 * addresses stored within the map. Kernel object maps are used extensively in
 * the Linux kernel for managing character and block devices, providing a hash
 * table-like structure for efficient device lookup and registration.
 *
 * The function performs the following operations:
 * 1. Validates that the specified map symbol exists in the kernel
 * 2. Calculates the number of probe array slots based on the probes field size
 * 3. Reads the map address from the kernel symbol
 * 4. Iterates through each probe array slot in the hash table
 * 5. For each non-empty slot, traverses the linked list of probe structures
 * 6. Extracts the data field from each probe (which contains the device address)
 * 7. Collects all valid kernel virtual addresses
 *
 * @param map_name The name of the kernel symbol containing the kobj_map
 *                 (e.g., "cdev_map" for character devices, "bdev_map" for block devices)
 * @return Vector of device addresses found in the kernel object map
 *         Returns empty vector if map doesn't exist or no devices found
 *
 */
std::vector<ulong> ParserPlugin::for_each_kobj_map(const std::string& map_name){
    std::vector<ulong> dev_list;
    if (!csymbol_exists(map_name)){
        return dev_list;
    }
    size_t len = field_size(kobj_map,probes)/sizeof(void *);
    ulong map_addr = read_pointer(csymbol_value(map_name),"map addr");
    if (!is_kvaddr(map_addr)){
        return dev_list;
    }
    for (size_t i = 0; i < len; i++){
        ulong probe_addr = read_pointer(map_addr + (i * sizeof(void *)),"probe_addr");
        if (!is_kvaddr(probe_addr)){
            continue;
        }
        ulong next_addr = probe_addr;
        while (is_kvaddr(next_addr)){
            ulong data = read_pointer(next_addr + field_offset(probe,data),"data");
            if (is_kvaddr(data)){
                dev_list.push_back(data);
            }
            next_addr = read_pointer(next_addr + field_offset(probe,next),"next");
        }
    }
    return dev_list;
}

/**
 * Retrieve all character device addresses from the kernel's character device map
 *
 * This function traverses the kernel's character device map (cdev_map) to collect
 * all character device structure addresses. The cdev_map is a kernel object map
 * that provides efficient lookup and management of character devices in the system.
 *
 * Character devices are accessed through device files and provide unbuffered,
 * direct access to hardware devices. Examples include terminal devices (/dev/tty*),
 * serial ports (/dev/ttyS*), and various hardware interfaces.
 *
 * @return Vector of character device addresses found in the cdev_map
 *         Returns empty vector if cdev_map doesn't exist or no devices found
 *
 */
std::vector<ulong> ParserPlugin::for_each_cdev(){
    return for_each_kobj_map("cdev_map");
}

/**
 * Retrieve all disk (gendisk) addresses from the kernel
 *
 * This function attempts to find all disk structures in the system using multiple
 * fallback methods to ensure compatibility across different kernel versions and
 * configurations. It tries the most reliable method first (bdev_map) and falls
 * back to alternative approaches if needed.
 *
 * The function uses the following priority order:
 * 1. First tries get_disk_by_bdevmap() - traversing the kernel's block device map
 * 2. If no devices found, tries get_disk_by_block_device() - extracts gendisk from block_device structures
 *
 * @return Vector of gendisk structure addresses found in the system
 *         Returns empty vector if no disk devices are found by any method
 *
 */
std::vector<ulong> ParserPlugin::for_each_disk(){
    std::vector<ulong> dev_list = get_disk_by_bdevmap();
    if (dev_list.empty()){
        dev_list = get_disk_by_block_device();
    }
    return dev_list;
}

/**
 * Retrieve all block device addresses from the kernel
 *
 * This function attempts to find all block devices in the system using multiple
 * fallback methods to ensure compatibility across different kernel versions and
 * configurations. It tries the most reliable method first (bdev_map) and falls
 * back to alternative approaches if needed.
 *
 * The function uses the following priority order:
 * 1. First tries get_block_device_by_bdevs() - traversing the "all_bdevs" global list
 * 2. If no devices found, tries get_block_device_by_class() - traverses block_device class
 * 3. If still no devices found, tries get_disk_by_bdevfs() - uses the blockdev filesystem
 *
 * @return Vector of block device addresses found in the system
 *         Returns empty vector if no block devices are found by any method
 *
 */
std::vector<ulong> ParserPlugin::for_each_bdev(){
    std::vector<ulong> dev_list = get_block_device_by_bdevs();
    if (dev_list.empty()){
        dev_list = get_block_device_by_class();
        if (dev_list.empty()){
            dev_list = get_block_device_by_bdevfs();
        }
    }
    return dev_list;
}

/**
 * Retrieve all miscellaneous device addresses from the kernel's misc device list
 *
 * This function traverses the kernel's global miscellaneous device list (misc_list)
 * to collect all miscdevice structure addresses. Miscellaneous devices are character
 * devices that don't fit into other standard device categories and are managed
 * through a unified interface.
 *
 * The function performs the following operations:
 * 1. Checks if the misc_list symbol exists in the kernel
 * 2. Traverses the linked list using the list field in miscdevice structures
 * 3. Validates each device address as a kernel virtual address
 * 4. Collects all valid miscellaneous device addresses
 *
 * @return Vector of miscdevice structure addresses found in the misc_list
 *         Returns empty vector if misc_list symbol doesn't exist or no devices found
 *
 */
std::vector<ulong> ParserPlugin::for_each_misc_dev(){
    std::vector<ulong> dev_list;
    if (!csymbol_exists("misc_list")){
        return dev_list;
    }

    std::vector<ulong> misc_devices = for_each_list(csymbol_value("misc_list"), field_offset(miscdevice, list));
    dev_list.reserve(misc_devices.size());

    for (ulong addr : misc_devices) {
        if (is_kvaddr(addr)) {
            dev_list.push_back(addr);
        }
    }
    return dev_list;
}

/**
 * Retrieve all device class addresses from the kernel's class subsystem
 *
 * This function traverses the kernel's device class system to collect all
 * class structure addresses. Device classes group devices with similar
 * functionality together (e.g., "block", "net", "input") and provide a
 * unified interface for device management and sysfs representation.
 *
 * The function works by:
 * 1. Checking if the class_kset symbol exists in the kernel
 * 2. Reading the class_kset address from the kernel symbol
 * 3. Traversing the kset's list of kobjects (kernel objects)
 * 4. For each kobject, calculating the corresponding class address by:
 *    - Converting kobject to kset using field offsets
 *    - Converting kset to subsys_private using field offsets
 *    - Reading the class pointer from subsys_private structure
 * 5. Validating each address as a kernel virtual address
 *
 * @return Vector of class structure addresses found in the kernel's class subsystem
 *         Returns empty vector if class_kset symbol doesn't exist or no classes found
 *
 */
std::vector<ulong> ParserPlugin::for_each_class(){
    std::vector<ulong> class_list;
    if (!csymbol_exists("class_kset")){
        return class_list;
    }
    ulong class_kset_addr = read_pointer(csymbol_value("class_kset"),"class_kset");
    if (!is_kvaddr(class_kset_addr)) {
        return class_list;
    }
    ulong list_head = class_kset_addr + field_offset(kset,list);
    int kobj_entry_offset = field_offset(kobject, entry);
    int kset_kobj_offset = field_offset(kset,kobj);
    int subsys_subsys_offset = field_offset(subsys_private,subsys);
    int subsys_class_offset = field_offset(subsys_private,class);
    std::vector<ulong> kobject_list = for_each_list(list_head, kobj_entry_offset);
    class_list.reserve(kobject_list.size());
    for (const auto& kobject_addr : kobject_list) {
        ulong kset_addr = kobject_addr - kset_kobj_offset;
        if (!is_kvaddr(kset_addr)) continue;
        ulong subsys_addr = kset_addr - subsys_subsys_offset;
        if (!is_kvaddr(subsys_addr)) continue;
        ulong class_addr = read_pointer(subsys_addr + subsys_class_offset,"class");
        if (!is_kvaddr(class_addr)) continue;
        class_list.push_back(class_addr);
    }
    return class_list;
}

/**
 * Retrieve all device class type information from the kernel
 *
 * This function iterates through all device classes in the system and parses
 * their information into structured class_type objects. It provides a high-level
 * view of all device classes registered in the kernel.
 *
 * Optimizations:
 * - Pre-allocates vector capacity based on class count to avoid reallocations
 * - Uses move semantics to avoid unnecessary copies of shared_ptr objects
 * - Filters out null entries during iteration for robustness
 *
 * @return Vector of shared pointers to class_type structures containing parsed class information
 *         Returns empty vector if no classes found or parsing fails
 */
std::vector<std::shared_ptr<class_type>> ParserPlugin::for_each_class_type(){
    std::vector<std::shared_ptr<class_type>> class_list;

    // Get all class addresses first to enable capacity pre-allocation
    std::vector<ulong> class_addrs = for_each_class();
    if (class_addrs.empty()) {
        return class_list;
    }

    // Pre-allocate vector capacity to avoid multiple reallocations during push_back
    class_list.reserve(class_addrs.size());

    // Parse each class and add to result list using move semantics
    for (const auto& class_addr : class_addrs) {
        auto class_info = parser_class_info(class_addr);
        if (class_info != nullptr){
            class_list.push_back(std::move(class_info));
        }
    }

    return class_list;
}

/**
 * Retrieve all bus type addresses from the kernel's bus subsystem
 *
 * This function traverses the kernel's device bus system to collect all
 * bus_type structure addresses. Device buses represent different types of
 * hardware buses in the system (e.g., "pci", "usb", "platform") and provide
 * a unified interface for device and driver management.
 *
 * The function works by:
 * 1. Checking if the bus_kset symbol exists in the kernel
 * 2. Reading the bus_kset address from the kernel symbol
 * 3. Traversing the kset's list of kobjects (kernel objects)
 * 4. For each kobject, calculating the corresponding bus_type address by:
 *    - Converting kobject to kset using field offsets
 *    - Converting kset to subsys_private using field offsets
 *    - Reading the bus_type pointer from subsys_private structure
 * 5. Validating each address as a kernel virtual address
 *
 * @return Vector of bus_type structure addresses found in the kernel's bus subsystem
 *         Returns empty vector if bus_kset symbol doesn't exist or no buses found
 *
 */
std::vector<ulong> ParserPlugin::for_each_bus(){
    std::vector<ulong> bus_list;
    if (!csymbol_exists("bus_kset")){
        return bus_list;
    }
    ulong bus_kset_addr = read_pointer(csymbol_value("bus_kset"),"bus_kset");
    if (!is_kvaddr(bus_kset_addr)) {
        return bus_list;
    }
    ulong list_head = bus_kset_addr + field_offset(kset,list);
    int kobj_entry_offset = field_offset(kobject, entry);
    int kset_kobj_offset = field_offset(kset,kobj);
    int subsys_subsys_offset = field_offset(subsys_private,subsys);
    int subsys_bus_offset = field_offset(subsys_private,bus);
    std::vector<ulong> kobject_list = for_each_list(list_head, kobj_entry_offset);
    bus_list.reserve(kobject_list.size());
    for (const auto& kobject_addr : kobject_list) {
        ulong kset_addr = kobject_addr - kset_kobj_offset;
        if (!is_kvaddr(kset_addr)) continue;
        ulong subsys_addr = kset_addr - subsys_subsys_offset;
        if (!is_kvaddr(subsys_addr)) continue;
        ulong bus_addr = read_pointer(subsys_addr + subsys_bus_offset,"bus_type");
        if (!is_kvaddr(bus_addr)) continue;
        bus_list.push_back(bus_addr);
    }
    return bus_list;
}

/**
 * Retrieve all bus type information from the kernel
 *
 * This function iterates through all device buses in the system and parses
 * their information into structured bus_type objects. It provides a high-level
 * view of all device buses registered in the kernel, including bus names,
 * probe functions, and subsystem private data.
 *
 * @return Vector of shared pointers to bus_type structures containing parsed bus information
 *         Returns empty vector if no buses found or parsing fails
 */
std::vector<std::shared_ptr<bus_type>> ParserPlugin::for_each_bus_type(){
    std::vector<std::shared_ptr<bus_type>> bus_list;

    // Get all bus addresses first to enable capacity pre-allocation
    std::vector<ulong> bus_addrs = for_each_bus();
    if (bus_addrs.empty()) {
        return bus_list;
    }

    // Pre-allocate vector capacity to avoid multiple reallocations during push_back
    bus_list.reserve(bus_addrs.size());

    // Parse each bus and add to result list using move semantics
    for (const auto& bus_addr : bus_addrs) {
        auto bus_info = parser_bus_info(bus_addr);
        if (bus_info != nullptr){
            bus_list.push_back(std::move(bus_info));
        }
    }

    return bus_list;
}

/**
 * Retrieve all device information from all buses in the kernel
 *
 * This function iterates through all device buses in the system and collects
 * all devices registered on each bus. It provides a comprehensive view of all
 * hardware devices managed by the kernel's device model, regardless of which
 * bus they're attached to (PCI, USB, platform, etc.).
 *
 * The function performs the following operations:
 * 1. Gets all bus types registered in the system
 * 2. For each bus, retrieves all devices attached to that bus
 * 3. Aggregates all devices from all buses into a single collection
 * 4. Returns the complete device list
 *
 * @return Vector of shared pointers to device structures containing all devices in the system
 *         Returns empty vector if no buses or devices are found
 *
 * Example usage:
 *   auto all_devices = for_each_device();
 *   for (const auto& dev : all_devices) {
 *       PRINT("Device: %s at 0x%lx\n", dev->name.c_str(), dev->addr);
 *   }
 */
std::vector<std::shared_ptr<device>> ParserPlugin::for_each_device(){
    std::vector<std::shared_ptr<device>> device_list;

    // Get all bus types first to enable capacity estimation
    std::vector<std::shared_ptr<bus_type>> bus_types = for_each_bus_type();
    if (bus_types.empty()) {
        return device_list;
    }

    // Pre-allocate capacity based on estimated average devices per bus
    // Typical systems have 10-50 devices per bus, so reserve conservatively
    constexpr size_t ESTIMATED_DEVICES_PER_BUS = 20;
    device_list.reserve(bus_types.size() * ESTIMATED_DEVICES_PER_BUS);

    // Iterate through each bus type and collect all its devices
    for (const auto& bus_ptr : bus_types) {
        // Skip invalid bus entries
        if (!bus_ptr || bus_ptr->name.empty()) {
            continue;
        }

        // Get all devices for this specific bus
        std::vector<std::shared_ptr<device>> bus_devices = for_each_device_for_bus(bus_ptr->name);

        // Skip empty device lists
        if (bus_devices.empty()) {
            continue;
        }

        // Reserve additional capacity if needed to avoid reallocation
        if (device_list.capacity() < device_list.size() + bus_devices.size()) {
            device_list.reserve(device_list.size() + bus_devices.size());
        }

        // Move devices from bus_devices to device_list using move iterator
        // This avoids copying shared_ptr objects and is more efficient
        device_list.insert(
            device_list.end(),
            std::make_move_iterator(bus_devices.begin()),
            std::make_move_iterator(bus_devices.end())
        );
    }

    // Shrink to fit to release unused capacity (optional, trade-off between memory and performance)
    // Uncomment if memory efficiency is more important than potential future insertions
    // device_list.shrink_to_fit();

    return device_list;
}

/**
 * Retrieve all device addresses for a given device class
 *
 * This function traverses the kernel's device class system to collect all
 * device addresses that belong to a specific device class. Device classes
 * group devices with similar functionality together (e.g., "block", "net",
 * "input") and provide a unified interface for device management.
 *
 * The function works by:
 * 1. Finding the subsys_private structure for the specified class
 * 2. Accessing the klist_devices list within the subsys_private structure
 * 3. Traversing the list of klist_node entries
 * 4. For each node, calculating the device_private address using field offsets
 * 5. Reading the device pointer from each device_private structure
 * 6. Collecting all valid device addresses
 *
 * @param class_name The name of the device class to search for (e.g., "block", "net", "input")
 * @return Vector of device structure addresses found in the specified class
 *         Returns empty vector if class doesn't exist or no devices found
 *
 */
std::vector<std::shared_ptr<device>> ParserPlugin::for_each_device_for_class(const std::string& class_name){
    std::vector<std::shared_ptr<device>> device_list;
    ulong private_addr = get_class_subsys_private(class_name);
    if (!is_kvaddr(private_addr)){
        return device_list;
    }
    // Cache field offsets to avoid repeated lookups
    int klist_devices_offset = field_offset(subsys_private, klist_devices);
    int k_list_offset = field_offset(klist, k_list);
    int n_node_offset = field_offset(klist_node, n_node);
    int knode_class_offset = field_offset(device_private, knode_class);
    int device_offset = field_offset(device_private, device);

    ulong list_head = private_addr + klist_devices_offset + k_list_offset;
    std::vector<ulong> nodes = for_each_list(list_head, n_node_offset);
    // Reserve capacity for better performance
    device_list.reserve(nodes.size());
    for (ulong node : nodes) {
        if (!is_kvaddr(node)) continue;

        ulong device_private_addr = node - knode_class_offset;
        if (!is_kvaddr(device_private_addr)) continue;
        ulong device_addr = read_pointer(device_private_addr + device_offset, "device_private");
        if (is_kvaddr(device_addr)) {
            std::shared_ptr<device> dev_ptr = parser_device(device_addr);
            if (dev_ptr != nullptr) {
                device_list.push_back(dev_ptr);
            }
        }
    }
    return device_list;
}

/**
 * Retrieve all device addresses for a given device bus
 *
 * This function traverses the kernel's device bus system to collect all
 * device addresses that belong to a specific device bus. Device buses
 * represent different types of hardware buses in the system (e.g., "pci",
 * "usb", "platform") and provide a unified interface for device management.
 *
 * The function works by:
 * 1. Finding the subsys_private structure for the specified bus
 * 2. Accessing the klist_devices list within the subsys_private structure
 * 3. Traversing the list of klist_node entries
 * 4. For each node, calculating the device_private address using field offsets
 * 5. Reading the device pointer from each device_private structure
 * 6. Collecting all valid device addresses
 *
 * @param bus_name The name of the device bus to search for (e.g., "pci", "usb", "platform")
 * @return Vector of device structure addresses found in the specified bus
 *         Returns empty vector if bus doesn't exist or no devices found
 *
 */
std::vector<std::shared_ptr<device>> ParserPlugin::for_each_device_for_bus(const std::string& bus_name){
    std::vector<std::shared_ptr<device>> device_list;
    ulong private_addr = get_bus_subsys_private(bus_name);
    if (!is_kvaddr(private_addr)){
        return device_list;
    }
    // Cache field offsets to avoid repeated lookups
    int klist_devices_offset = field_offset(subsys_private, klist_devices);
    int k_list_offset = field_offset(klist, k_list);
    int n_node_offset = field_offset(klist_node, n_node);
    int knode_bus_offset = field_offset(device_private, knode_bus);
    int device_offset = field_offset(device_private, device);

    ulong list_head = private_addr + klist_devices_offset + k_list_offset;
    std::vector<ulong> nodes = for_each_list(list_head, n_node_offset);
    // Reserve capacity for better performance
    device_list.reserve(nodes.size());
    for (ulong node : nodes) {
        if (!is_kvaddr(node)) continue;

        ulong device_private_addr = node - knode_bus_offset;
        if (!is_kvaddr(device_private_addr)) continue;
        ulong device_addr = read_pointer(device_private_addr + device_offset, "device_private");
        if (is_kvaddr(device_addr)) {
            std::shared_ptr<device> dev_ptr = parser_device(device_addr);
            if (dev_ptr != nullptr) {
                device_list.push_back(dev_ptr);
            }
        }
    }
    return device_list;
}

/**
 * Retrieve all device addresses for a given device driver
 *
 * This function traverses the kernel's device driver system to collect all
 * device addresses that are bound to a specific device driver. Device drivers
 * manage hardware devices and maintain lists of devices they are responsible for.
 *
 * The function works by:
 * 1. Validating the driver address as a kernel virtual address
 * 2. Caching field offsets to avoid repeated lookups for performance
 * 3. Reading the driver_private structure from the driver's p field
 * 4. Accessing the klist_devices list within the driver_private structure
 * 5. Traversing the list of klist_node entries
 * 6. For each node, calculating the device_private address using field offsets
 * 7. Reading the device pointer from each device_private structure
 * 8. Collecting all valid device addresses
 *
 * @param driver_addr The kernel virtual address of the device_driver structure
 * @return Vector of device structure addresses bound to the specified driver
 *         Returns empty vector if driver doesn't exist or no devices found
 *
 */
std::vector<std::shared_ptr<device>> ParserPlugin::for_each_device_for_driver(ulong driver_addr){
    std::vector<std::shared_ptr<device>> device_list;
    if (!is_kvaddr(driver_addr)){
        return device_list;
    }
    // Cache field offsets to avoid repeated lookups
    int driver_p_offset = field_offset(device_driver, p);
    int klist_devices_offset = field_offset(driver_private, klist_devices);
    int k_list_offset = field_offset(klist, k_list);
    int n_node_offset = field_offset(klist_node, n_node);
    int knode_driver_offset = field_offset(device_private, knode_driver);
    int device_offset = field_offset(device_private, device);
    ulong driver_private_addr = read_pointer(driver_addr + driver_p_offset, "p");
    if (!is_kvaddr(driver_private_addr)) {
        return device_list;
    }
    ulong dev_list_head = driver_private_addr + klist_devices_offset + k_list_offset;
    std::vector<ulong> nodes = for_each_list(dev_list_head, n_node_offset);
    // Reserve capacity for better performance
    device_list.reserve(nodes.size());
    for (ulong node : nodes) {
        if (!is_kvaddr(node)) continue;
        ulong device_private_addr = node - knode_driver_offset;
        if (!is_kvaddr(device_private_addr)) continue;
        ulong device_addr = read_pointer(device_private_addr + device_offset, "device_private");
        if (is_kvaddr(device_addr)) {
            std::shared_ptr<device> dev_ptr = parser_device(device_addr);
            if (dev_ptr != nullptr) {
                device_list.push_back(dev_ptr);
            }
        }
    }
    return device_list;
}

std::shared_ptr<driver> ParserPlugin::find_device_driver(const std::string& driver_name){
    for (const auto& bus_ptr : for_each_bus_type()) {
        for (const auto& driver_addr : for_each_driver(bus_ptr->name)) {
            std::shared_ptr<driver> driv_ptr = parser_driver(driver_addr);
            if (driv_ptr->name == driver_name){
                return driv_ptr;
            }
        }
    }
    return nullptr;
}

/**
 * Retrieve all device driver addresses for a given device bus
 *
 * This function traverses the kernel's device bus system to collect all
 * device driver addresses that belong to a specific device bus. Device buses
 * represent different types of hardware buses in the system (e.g., "pci",
 * "usb", "platform") and maintain lists of registered drivers.
 *
 * The function works by:
 * 1. Finding the subsys_private structure for the specified bus
 * 2. Accessing the klist_drivers list within the subsys_private structure
 * 3. Traversing the list of klist_node entries
 * 4. For each node, calculating the driver_private address using field offsets
 * 5. Reading the driver pointer from each driver_private structure
 * 6. Collecting all valid driver addresses
 *
 * @param bus_name The name of the device bus to search for (e.g., "pci", "usb", "platform")
 * @return Vector of device_driver structure addresses found in the specified bus
 *         Returns empty vector if bus doesn't exist or no drivers found
 *
 */
std::vector<ulong> ParserPlugin::for_each_driver(const std::string& bus_name){
    std::vector<ulong> driver_list;
    ulong private_addr = get_bus_subsys_private(bus_name);
    if (!is_kvaddr(private_addr)){
        return driver_list;
    }
    // Cache field offsets to avoid repeated lookups
    int klist_drivers_offset = field_offset(subsys_private, klist_drivers);
    int k_list_offset = field_offset(klist, k_list);
    int n_node_offset = field_offset(klist_node, n_node);
    int knode_bus_offset = field_offset(driver_private, knode_bus);
    int driver_offset = field_offset(driver_private, driver);
    ulong list_head = private_addr + klist_drivers_offset + k_list_offset;
    std::vector<ulong> nodes = for_each_list(list_head, n_node_offset);
    // Reserve capacity for better performance
    driver_list.reserve(nodes.size());
    for (ulong node : nodes) {
        if (!is_kvaddr(node)) continue;
        ulong driver_private_addr = node - knode_bus_offset;
        if (!is_kvaddr(driver_private_addr)) continue;
        ulong driver_addr = read_pointer(driver_private_addr + driver_offset, "driver_private");
        if (is_kvaddr(driver_addr)) {
            driver_list.push_back(driver_addr);
        }
    }
    return driver_list;
}

/**
 * Retrieve all file structure addresses from a task's file descriptor table
 *
 * This function traverses a task's file descriptor table to collect all valid
 * file structure addresses. It reads the task's files_struct, then the fdtable,
 * and finally extracts all file pointers from the file descriptor array.
 *
 * The function performs the following operations:
 * 1. Validates the task context and reads the files_struct from the task
 * 2. Reads the fdtable from the files_struct
 * 3. Gets the maximum number of file descriptors and the fd array pointer
 * 4. Reads the entire fd array in one memory operation for efficiency
 * 5. Validates each file pointer and stores valid addresses (0 for invalid ones)
 *
 * @param tc The task context structure containing the task information
 * @return Vector of file structure addresses (0 for unused/invalid file descriptors)
 *         Returns empty vector if task context is invalid or has no file table
 *
 */
std::vector<ulong> ParserPlugin::for_each_task_files(struct task_context *tc){
    std::vector<ulong> file_table;
    if (!tc){
        return file_table;
    }
    ulong files = read_pointer(tc->task + field_offset(task_struct,files),"files");
    if (!is_kvaddr(files)){
        return file_table;
    }
    ulong fdt = read_pointer(files + field_offset(files_struct,fdt),"fdt");
    if (!is_kvaddr(fdt)){
        return file_table;
    }
    uint max_fds = read_uint(fdt + field_offset(fdtable,max_fds),"max_fds");
    if (max_fds == 0 || max_fds > 65536) { // Sanity check
        return file_table;
    }
    ulong fds = read_pointer(fdt + field_offset(fdtable,fd),"fds");
    if (!is_kvaddr(fds)){
        return file_table;
    }
    // Read all file pointers in one memory operation
    size_t total_size = max_fds * sizeof(void*);
    void* fd_array = read_memory(fds, total_size, "fd_array");
    if (!fd_array) {
        return file_table;
    }
    file_table.reserve(max_fds);
    ulong* fd_ptr = static_cast<ulong*>(fd_array);
    for (uint i = 0; i < max_fds; i++){
        ulong file_addr = fd_ptr[i];
        file_table.push_back(is_kvaddr(file_addr) ? file_addr : 0);
    }
    FREEBUF(fd_array);
    return file_table;
}

/**
 * Retrieve all subdirectory entries for a given dentry
 *
 * This function traverses the subdirectory list of a dentry structure to collect
 * all child directory entries. It handles kernel version compatibility by checking
 * for both old (d_subdirs/d_child) and new (d_children/d_sib) field naming conventions.
 *
 * @param dentry The parent dentry address to search for subdirectories
 * @return Vector of valid subdirectory dentry addresses, empty if none found or invalid input
 *
 */
std::vector<ulong> ParserPlugin::for_each_subdirs(ulong dentry){
    std::vector<ulong> dentry_list;
    if (!is_kvaddr(dentry)) {
        return dentry_list;
    }
    size_t list_head = 0;
    if (field_offset(dentry,d_subdirs) != -1){
        list_head = dentry + field_offset(dentry,d_subdirs);
    }else{
        list_head = dentry + field_offset(dentry,d_children);
    }
    size_t offset = 0;
    if (field_offset(dentry,d_child) != -1){
        offset = field_offset(dentry,d_child);
    }else{
        offset = field_offset(dentry,d_sib);
    }
    for (const auto& sub_dentry : for_each_list(list_head,offset)) {
        if (!is_kvaddr(sub_dentry)) continue;
        dentry_list.push_back(sub_dentry);
    }
    return dentry_list;
}

/**
 * Retrieve per-CPU variable addresses for all CPUs in the system
 *
 * This function traverses all CPUs in the system and calculates the per-CPU
 * variable addresses by adding the per-CPU offset to the base address. The
 * results are returned in CPU order, where CPU0's address is stored at index 0,
 * CPU1's address at index 1, and so on.
 *
 * The function handles both SMP and non-SMP systems:
 * - For SMP systems with per-CPU offsets: calculates actual per-CPU addresses
 * - For non-SMP systems or systems without per-CPU offsets: returns base address for CPU0
 * - For offline or invalid CPUs: stores 0 at the corresponding index
 *
 * @param addr The base address of the per-CPU variable to resolve
 * @return Vector of per-CPU addresses indexed by CPU number (CPU0 at index 0, etc.)
 *         Returns empty vector if addr is invalid or system doesn't support per-CPU variables
 *         Returns vector with 0 values for offline/invalid CPUs at their respective indices
 *
 */
std::vector<ulong> ParserPlugin::for_each_percpu(ulong addr) {
    std::vector<ulong> cpu_list;

    // Early validation: check if base address is valid
    if (addr == 0) {
        LOGE("Invalid base address (0x0) for per-CPU variable\n");
        return cpu_list;
    }

    // Check if system supports SMP and has per-CPU offsets configured
    if (!(kt->flags & SMP) || !(kt->flags & PER_CPU_OFF)) {
        // Non-SMP system or no per-CPU offsets: return base address for CPU0 only
        cpu_list.reserve(1);
        cpu_list.push_back(addr);
        return cpu_list;
    }

    // Validate that we have a reasonable number of CPUs to prevent excessive memory allocation
    if (kt->cpus == 0 || kt->cpus > 8192) {  // Support up to 8192 CPUs (reasonable upper bound)
        LOGE("Invalid CPU count: %zu (expected 1-8192)\n", kt->cpus);
        return cpu_list;
    }

    // Pre-allocate vector capacity to avoid reallocations during insertion
    // This is a key optimization for performance
    cpu_list.reserve(kt->cpus);

    // Iterate through all possible CPU indices to maintain index correspondence
    for (int cpu_id = 0; cpu_id < kt->cpus; cpu_id++) {
        // Check if this CPU has a valid per-CPU offset
        if (kt->__per_cpu_offset[cpu_id] == 0) {
            // CPU is offline or invalid: store 0 to maintain index correspondence
            // This ensures cpu_list[i] always corresponds to CPU i
            cpu_list.push_back(0);
            continue;
        }

        // Calculate the per-CPU address for this CPU
        // Formula: per_cpu_addr = base_addr + per_cpu_offset[cpu_id]
        ulong percpu_addr = addr + kt->__per_cpu_offset[cpu_id];

        // Validate the calculated address is reasonable (basic sanity check)
        if (!is_kvaddr(percpu_addr)) {
            // Invalid calculated address: store 0 for this CPU
            LOGD("Invalid per-CPU address calculated for CPU%zu: 0x%lx\n", cpu_id, percpu_addr);
            cpu_list.push_back(0);
            continue;
        }

        // Store the valid per-CPU address at the corresponding CPU index
        cpu_list.push_back(percpu_addr);
    }

    return cpu_list;
}

/**
 * Parse device class information from kernel memory
 *
 * This function extracts and structures information about a device class from
 * the kernel's device model. Device classes group devices with similar functionality
 * (e.g., "block", "net", "input") and provide a unified interface for device management.
 *
 * The function performs the following operations:
 * 1. Validates the class structure address as a kernel virtual address
 * 2. Reads the class name string from kernel memory via pointer indirection
 * 3. Cleans up the name string by removing trailing newlines
 * 4. Retrieves the associated subsys_private structure for internal management data
 *
 * Optimizations applied:
 * - Early validation to fail fast on invalid addresses
 * - Efficient string handling with move semantics
 * - Single memory read for name pointer
 * - Conditional newline removal only when needed
 *
 * @param addr The kernel virtual address of the class structure to parse
 * @return Shared pointer to class_type structure containing parsed information,
 *         or nullptr if address is invalid or parsing fails
 *
 * Example usage:
 *   auto class_info = parser_class_info(class_addr);
 *   if (class_info) {
 *       PRINT("Class: %s at 0x%lx\n", class_info->name.c_str(), class_info->addr);
 *   }
 */
std::shared_ptr<class_type> ParserPlugin::parser_class_info(ulong addr){
    LOGD("Processing class at address: 0x%lx\n", addr);

    // Early validation: ensure address is valid kernel virtual address
    if (!is_kvaddr(addr)) {
        LOGE("Invalid class address 0x%lx, skipping\n", addr);
        return nullptr;
    }

    // Create class_type object to store parsed information
    std::shared_ptr<class_type> class_ptr = std::make_shared<class_type>();
    class_ptr->addr = addr;

    // Read the class name from kernel memory via pointer indirection
    // Path: class->name (pointer to string)
    size_t name_addr = read_pointer(addr + field_offset(class, name), "class name addr");
    if (is_kvaddr(name_addr)) {
        // Read the actual name string (max 64 bytes)
        class_ptr->name = read_cstring(name_addr, 64, "class name");

        // Clean up: remove trailing newline if present
        if (!class_ptr->name.empty() && class_ptr->name.back() == '\n') {
            class_ptr->name.pop_back();
        }
    } else {
        // Invalid name pointer, use empty string
        class_ptr->name = "";
    }

    // Retrieve the subsys_private structure for this class
    // This contains internal management data like device/driver lists
    class_ptr->subsys_private = get_class_subsys_private(class_ptr->name);

    return class_ptr;
}

/**
 * Parse device bus information from kernel memory
 *
 * This function extracts and structures information about a device bus from
 * the kernel's device model. Device buses represent different hardware bus types
 * (e.g., "pci", "usb", "platform") and manage device/driver binding and probing.
 *
 * The function performs the following operations:
 * 1. Validates the bus_type structure address as a kernel virtual address
 * 2. Reads the bus name string from kernel memory via pointer indirection
 * 3. Reads and resolves the probe function address to a symbolic name
 * 4. Retrieves the associated subsys_private structure for internal management data
 *
 * Optimizations applied:
 * - Early validation to fail fast on invalid addresses
 * - Efficient string handling with move semantics
 * - Symbol resolution with fallback to raw address
 * - Single memory read for each pointer field
 *
 * @param addr The kernel virtual address of the bus_type structure to parse
 * @return Shared pointer to bus_type structure containing parsed information,
 *         or nullptr if address is invalid or parsing fails
 *
 * Example usage:
 *   auto bus_info = parser_bus_info(bus_addr);
 *   if (bus_info) {
 *       PRINT("Bus: %s, probe: %s\n", bus_info->name.c_str(), bus_info->probe.c_str());
 *   }
 */
std::shared_ptr<bus_type> ParserPlugin::parser_bus_info(ulong addr){
    LOGD("Processing bus at address: 0x%lx\n", addr);

    // Early validation: ensure address is valid kernel virtual address
    if (!is_kvaddr(addr)) {
        LOGE("Invalid bus address 0x%lx, skipping\n", addr);
        return nullptr;
    }

    // Create bus_type object to store parsed information
    std::shared_ptr<bus_type> bus_ptr = std::make_shared<bus_type>();
    bus_ptr->addr = addr;

    // Read the bus name from kernel memory via pointer indirection
    // Path: bus_type->name (pointer to string)
    size_t name_addr = read_pointer(addr + field_offset(bus_type, name), "bus name addr");
    if (is_kvaddr(name_addr)) {
        // Read the actual name string (max 128 bytes)
        bus_ptr->name = read_cstring(name_addr, 128, "bus name");
    } else {
        // Invalid name pointer, use empty string
        bus_ptr->name = "";
    }

    // Read and resolve the probe function address
    // Path: bus_type->probe (function pointer)
    size_t probe_addr = read_pointer(addr + field_offset(bus_type, probe), "probe addr");
    bus_ptr->probe = to_symbol(probe_addr);

    // Retrieve the subsys_private structure for this bus
    // This contains internal management data like device/driver lists
    bus_ptr->subsys_private = get_bus_subsys_private(bus_ptr->name);

    LOGD("Bus name:%s subsys_private:0x%lx\n", bus_ptr->name.c_str(), bus_ptr->subsys_private);
    return bus_ptr;
}

/**
 * Parse device information from kernel memory
 *
 * This function extracts and structures information about a device from the
 * kernel's device model. It reads device metadata including name, bound driver,
 * and driver-specific data from the device structure in kernel memory.
 *
 * The function performs the following operations:
 * 1. Validates the device structure address as a kernel virtual address
 * 2. Reads the device name through the embedded kobject structure
 * 3. Checks if the device is bound to a driver and retrieves driver address
 * 4. Reads driver-specific private data pointer
 *
 * Optimizations applied:
 * - Early validation to fail fast on invalid addresses
 * - Cached field offsets to avoid repeated lookups
 * - Efficient string handling with move semantics
 * - Single memory read for each pointer field
 * - Clear logging for debugging device binding status
 *
 * @param addr The kernel virtual address of the device structure to parse
 * @return Shared pointer to device structure containing parsed information,
 *         or nullptr if address is invalid or parsing fails
 *
 * Example usage:
 *   auto dev_info = parser_device(device_addr);
 *   if (dev_info) {
 *       PRINT("Device: %s, driver: 0x%lx\n", dev_info->name.c_str(), dev_info->driv);
 *   }
 */
std::shared_ptr<device> ParserPlugin::parser_device(ulong addr){
    // Early validation: ensure address is valid kernel virtual address
    if (!is_kvaddr(addr)) {
        LOGE("Invalid device address 0x%lx\n", addr);
        return nullptr;
    }

    LOGD("Parsing device at address 0x%lx\n", addr);

    // Create device object to store parsed information
    std::shared_ptr<device> dev_ptr = std::make_shared<device>();
    dev_ptr->addr = addr;

    // Cache field offsets to avoid repeated lookups
    const size_t kobj_offset = field_offset(device, kobj);
    const size_t name_offset = field_offset(kobject, name);
    const size_t driver_offset = field_offset(device, driver);
    const size_t driver_data_offset = field_offset(device, driver_data);

    // Read device name through kobject->name pointer chain
    // Path: device->kobj.name (embedded kobject structure)
    const size_t name_addr = read_pointer(addr + kobj_offset + name_offset, "device name addr");
    if (is_kvaddr(name_addr)) {
        // Read the device name string from kernel memory (max 100 bytes)
        dev_ptr->name = read_cstring(name_addr, 100, "device name");
    } else {
        // Invalid name address, use empty string
        dev_ptr->name = "";
        LOGE("Invalid device name address 0x%lx, using empty string\n", name_addr);
    }

    // Read associated driver address if device is bound to one
    // Path: device->driver (pointer to device_driver structure)
    const size_t driver_addr = read_pointer(addr + driver_offset, "driver addr");
    if (is_kvaddr(driver_addr)) {
        // Device is bound to a driver, store the driver address
        LOGD("Device '%s' bound to driver at 0x%lx\n", dev_ptr->name.c_str(), driver_addr);
        dev_ptr->driv = driver_addr;
    } else {
        // Device is not bound to any driver (common for unprobed devices)
        LOGD("Device '%s' is not bound to any driver\n", dev_ptr->name.c_str());
        dev_ptr->driv = 0;
    }

    // Read driver-specific private data pointer
    // Path: device->driver_data (void pointer to driver's private data)
    dev_ptr->driver_data = read_pointer(addr + driver_data_offset, "driver_data");

    return dev_ptr;
}

/**
 * Parse device driver information from kernel memory
 *
 * This function extracts and structures information about a device driver from
 * the kernel's device model. It reads driver metadata including name, probe function,
 * and device tree compatibility information from the device_driver structure.
 *
 * The function performs the following operations:
 * 1. Validates the device_driver structure address as a kernel virtual address
 * 2. Reads the driver name string from kernel memory via pointer indirection
 * 3. Reads and resolves the probe function address to a symbolic name
 * 4. Reads device tree compatible string for device matching (if present)
 *
 * Optimizations applied:
 * - Early validation to fail fast on invalid addresses
 * - Cached field offsets to avoid repeated lookups
 * - Efficient string handling with move semantics
 * - Symbol resolution with fallback to raw address
 * - Single memory read for each pointer field
 *
 * @param addr The kernel virtual address of the device_driver structure to parse
 * @return Shared pointer to driver structure containing parsed information,
 *         or nullptr if address is invalid or parsing fails
 *
 * Example usage:
 *   auto driv_info = parser_driver(driver_addr);
 *   if (driv_info) {
 *       PRINT("Driver: %s, probe: %s\n", driv_info->name.c_str(), driv_info->probe.c_str());
 *   }
 */
std::shared_ptr<driver> ParserPlugin::parser_driver(ulong addr){
    // Early validation: ensure address is valid kernel virtual address
    if (!is_kvaddr(addr)) {
        LOGE("Invalid driver address 0x%lx\n", addr);
        return nullptr;
    }

    LOGD("Parsing driver at address 0x%lx\n", addr);

    // Create driver object to store parsed information
    std::shared_ptr<driver> driv_ptr = std::make_shared<driver>();
    driv_ptr->addr = addr;

    // Cache field offsets to avoid repeated lookups
    const size_t name_offset = field_offset(device_driver, name);
    const size_t probe_offset = field_offset(device_driver, probe);
    const size_t match_table_offset = field_offset(device_driver, of_match_table);

    // Read driver name from kernel memory via pointer indirection
    // Path: device_driver->name (pointer to string)
    const size_t name_addr = read_pointer(addr + name_offset, "driver name addr");
    if (is_kvaddr(name_addr)) {
        // Read the driver name string (max 100 bytes)
        driv_ptr->name = read_cstring(name_addr, 100, "driver name");
    } else {
        // Invalid name pointer, use empty string
        driv_ptr->name = "";
        LOGE("Invalid driver name address 0x%lx\n", name_addr);
    }

    // Read and resolve the probe function address
    // Path: device_driver->probe (function pointer)
    const size_t probe_addr = read_pointer(addr + probe_offset, "probe addr");
    driv_ptr->probe = to_symbol(probe_addr);

    // Read device tree compatible string if present
    // Path: device_driver->of_match_table->compatible
    // This is used for device tree based device matching
    const size_t match_table = read_pointer(addr + match_table_offset, "match_table addr");
    if (is_kvaddr(match_table)) {
        // Read the compatible string from the first match table entry (max 128 bytes)
        driv_ptr->compatible = read_cstring(
            match_table + field_offset(of_device_id, compatible),
            128,
            "compatible"
        );
    } else {
        // No device tree match table, use empty string
        driv_ptr->compatible = "";
    }

    return driv_ptr;
}

/**
 * Retrieve the subsys_private structure address for a given device bus
 *
 * This function searches through the kernel's device bus system to find
 * the subsys_private structure associated with a specific bus name. The
 * subsys_private structure contains internal management data for device
 * buses, including lists of devices and drivers.
 *
 * The function works by:
 * 1. Checking if the bus_kset symbol exists in the kernel
 * 2. Reading the bus_kset address from the kernel symbol
 * 3. Caching field offsets to avoid repeated lookups for performance
 * 4. Traversing the kset's list of kobjects (kernel objects)
 * 5. For each kobject, calculating the corresponding bus_type address by:
 *    - Converting kobject to kset using field offsets
 *    - Converting kset to subsys_private using field offsets
 *    - Reading the bus_type pointer from subsys_private structure
 * 6. Comparing the bus name with the requested name
 * 7. Returning the appropriate subsys_private address based on kernel version
 *
 * @param bus_name The name of the device bus to search for (e.g., "pci", "usb", "platform")
 * @return The subsys_private structure address if found, 0 if not found or error occurred
 *
 */
ulong ParserPlugin::get_bus_subsys_private(const std::string& bus_name){
    if (!csymbol_exists("bus_kset")){
        return 0;
    }
    ulong bus_kset_addr = read_pointer(csymbol_value("bus_kset"),"bus_kset");
    if (!is_kvaddr(bus_kset_addr)) {
        return 0;
    }
    // Cache field offsets to avoid repeated lookups
    int kobj_entry_offset = field_offset(kobject, entry);
    int kset_kobj_offset = field_offset(kset, kobj);
    int subsys_subsys_offset = field_offset(subsys_private, subsys);
    int subsys_bus_offset = field_offset(subsys_private, bus);
    int bus_name_offset = field_offset(bus_type, name);
    int bus_p_offset = field_offset(bus_type, p);

    ulong list_head = bus_kset_addr + field_offset(kset,list);
    for (const auto& kobject_addr : for_each_list(list_head, kobj_entry_offset)) {
        ulong kset_addr = kobject_addr - kset_kobj_offset;
        if (!is_kvaddr(kset_addr)) continue;

        ulong subsys_addr = kset_addr - subsys_subsys_offset;
        if (!is_kvaddr(subsys_addr)) continue;

        ulong bus_addr = read_pointer(subsys_addr + subsys_bus_offset,"bus_type");
        if (!is_kvaddr(bus_addr)) continue;

        ulong name_addr = read_pointer(bus_addr + bus_name_offset,"name addr");
        if (!is_kvaddr(name_addr)) continue;

        std::string name = read_cstring(name_addr, 16, "bus name");
        if (name == bus_name) {
            if (bus_p_offset != -1) {
                return read_pointer(bus_addr + bus_p_offset, "subsys_private");
            } else {
                return subsys_addr;
            }
        }
    }
    return 0;
}

/**
 * Retrieve the subsys_private structure address for a given device class
 *
 * This function searches through the kernel's device class system to find
 * the subsys_private structure associated with a specific class name. The
 * subsys_private structure contains internal management data for device
 * classes, including lists of devices and drivers.
 *
 * The function works by:
 * 1. Checking if the class_kset symbol exists in the kernel
 * 2. Reading the class_kset address from the kernel symbol
 * 3. Caching field offsets to avoid repeated lookups for performance
 * 4. Traversing the kset's list of kobjects (kernel objects)
 * 5. For each kobject, calculating the corresponding class address by:
 *    - Converting kobject to kset using field offsets
 *    - Converting kset to subsys_private using field offsets
 *    - Reading the class pointer from subsys_private structure
 * 6. Comparing the class name with the requested name
 * 7. Returning the appropriate subsys_private address based on kernel version
 *
 * @param class_name The name of the device class to search for (e.g., "block", "net", "input")
 * @return The subsys_private structure address if found, 0 if not found or error occurred
 *
 */
ulong ParserPlugin::get_class_subsys_private(const std::string& class_name){
    if (!csymbol_exists("class_kset")){
        return 0;
    }
    ulong class_kset_addr = read_pointer(csymbol_value("class_kset"),"class_kset");
    if (!is_kvaddr(class_kset_addr)) {
        return 0;
    }
    // Cache field offsets to avoid repeated lookups
    int kobj_entry_offset = field_offset(kobject, entry);
    int kset_kobj_offset = field_offset(kset, kobj);
    int subsys_subsys_offset = field_offset(subsys_private, subsys);
    int subsys_class_offset = field_offset(subsys_private, class);
    int class_name_offset = field_offset(class, name);
    int class_p_offset = field_offset(class, p);
    ulong list_head = class_kset_addr + field_offset(kset, list);
    for (const auto& kobject_addr : for_each_list(list_head, kobj_entry_offset)) {
        ulong kset_addr = kobject_addr - kset_kobj_offset;
        if (!is_kvaddr(kset_addr)) continue;

        ulong subsys_addr = kset_addr - subsys_subsys_offset;
        if (!is_kvaddr(subsys_addr)) continue;

        ulong class_addr = read_pointer(subsys_addr + subsys_class_offset, "class");
        if (!is_kvaddr(class_addr)) continue;

        ulong name_addr = read_pointer(class_addr + class_name_offset, "name addr");
        if (!is_kvaddr(name_addr)) continue;

        std::string name = read_cstring(name_addr, 64, "class name");
        if (name == class_name) {
            if (class_p_offset != -1) {
                return read_pointer(class_addr + class_p_offset, "subsys_private");
            } else {
                return subsys_addr;
            }
        }
    }
    return 0;
}

/**
 * Retrieve all block device addresses by traversing the "all_bdevs" global list
 *
 * This function finds block devices by examining the kernel's global list of all
 * block devices. The all_bdevs list is a linked list that contains all registered
 * block devices in the system, making it a comprehensive source for block device
 * enumeration.
 *
 * The function works by:
 * 1. Checking if the all_bdevs symbol exists in the kernel
 * 2. Getting the list head address from the kernel symbol
 * 3. Verifying that the bd_list field exists in the block_device structure
 * 4. Traversing the linked list using the bd_list field as the list node
 * 5. Collecting all valid block device addresses from the list
 *
 * @return Vector of block_device structure addresses found in the all_bdevs list
 *         Returns empty vector if all_bdevs symbol doesn't exist, bd_list field
 *         is missing, or no devices found
 *
 */
std::vector<ulong> ParserPlugin::get_block_device_by_bdevs(){
    std::vector<ulong> dev_list;
    if (!csymbol_exists("all_bdevs")){
        return dev_list;
    }
    ulong all_bdevs_addr = csymbol_value("all_bdevs");
    int bd_list_offset = field_offset(block_device, bd_list);
    if (bd_list_offset == -1) {
        return dev_list;
    }
    std::vector<ulong> block_devices = for_each_list(all_bdevs_addr, bd_list_offset);
    dev_list.reserve(block_devices.size());
    for (ulong addr : block_devices) {
        if (is_kvaddr(addr)) {
            dev_list.push_back(addr);
        }
    }
    return dev_list;
}

/**
 * Retrieve all block device addresses by traversing the "block" device class
 *
 * This function finds block devices by examining the kernel's device class system.
 * It looks up all devices registered under the "block" class and calculates the
 * corresponding block_device structure addresses by using the bd_device field offset.
 *
 * The function works by:
 * 1. Checking if the block_device structure has a bd_device field
 * 2. Getting all device addresses from the "block" device class
 * 3. For each device, calculating the block_device address by subtracting the bd_device offset
 * 4. Validating each calculated address as a kernel virtual address
 *
 * @return Vector of block_device structure addresses found through the device class system
 *         Returns empty vector if bd_device field doesn't exist or no devices found
 *
 */
std::vector<ulong> ParserPlugin::get_block_device_by_class(){
    std::vector<ulong> dev_list;
    if (field_offset(block_device, bd_device) == -1){
        return dev_list;
    }
    std::vector<std::shared_ptr<device>> devices = for_each_device_for_class("block");
    dev_list.reserve(devices.size());
    for (const auto& dev_ptr : devices) {
        ulong bd_addr = dev_ptr->addr - field_offset(block_device, bd_device);
        if (is_kvaddr(bd_addr)) {
            dev_list.push_back(bd_addr);
        }
    }
    return dev_list;
}

/**
 * Retrieve all block device addresses by traversing the blockdev filesystem
 *
 * This function finds block devices by examining the special blockdev filesystem's
 * superblock and traversing its inode list. The blockdev filesystem is a virtual
 * filesystem that provides access to block devices through special device files.
 *
 * The function works by:
 * 1. Checking if the blockdev_superblock symbol exists in the kernel
 * 2. Reading the superblock address from the kernel symbol
 * 3. Validating required field offsets for bdev_inode and inode structures
 * 4. Traversing the superblock's inode list (s_inodes)
 * 5. For each inode, calculating the corresponding block_device address
 *    by using the relationship: bdev_inode contains both vfs_inode and bdev fields
 * 6. Converting from inode address to block_device address using field offsets
 *
 * @return Vector of block_device structure addresses found through the blockdev filesystem
 *         Returns empty vector if blockdev_superblock doesn't exist, is invalid,
 *         or required field offsets are missing
 *
 */
std::vector<ulong> ParserPlugin::get_block_device_by_bdevfs(){
    std::vector<ulong> dev_list;
    if (!csymbol_exists("blockdev_superblock")){
        return dev_list;
    }
    ulong sb_addr = read_pointer(csymbol_value("blockdev_superblock"),"blockdev_superblock");
    if (!is_kvaddr(sb_addr)) {
        return dev_list;
    }
    // Check required field offsets exist
    int vfs_inode_offset = field_offset(bdev_inode, vfs_inode);
    int bdev_offset = field_offset(bdev_inode, bdev);
    int sb_list_offset = field_offset(inode, i_sb_list);
    int s_inodes_offset = field_offset(super_block, s_inodes);
    if (vfs_inode_offset == -1 || bdev_offset == -1 ||
        sb_list_offset == -1 || s_inodes_offset == -1) {
        return dev_list;
    }
    ulong list_head = sb_addr + s_inodes_offset;
    std::vector<ulong> inode_list = for_each_list(list_head, sb_list_offset);
    dev_list.reserve(inode_list.size());
    for (const auto& addr : inode_list) {
        ulong bd_addr = addr - vfs_inode_offset + bdev_offset;
        if (is_kvaddr(bd_addr)) {
            dev_list.push_back(bd_addr);
        }
    }
    return dev_list;
}

/**
 * Retrieve all gendisk addresses from the kernel's block device map
 *
 * This function traverses the kernel's block device map (bdev_map) to collect
 * all block device structure addresses. The bdev_map is a kernel object map
 * that provides efficient lookup and management of block devices in the system.
 *
 * Block devices provide buffered access to storage devices and are accessed
 * through device files. Examples include hard drives (/dev/sda*), SSDs,
 * optical drives (/dev/sr*), and other block-oriented storage devices.
 *
 * @return Vector of block device addresses found in the bdev_map
 *         Returns empty vector if bdev_map doesn't exist or no devices found
 *
 */
std::vector<ulong> ParserPlugin::get_disk_by_bdevmap(){
    return for_each_kobj_map("bdev_map");
}

/**
 * Retrieve all gendisk addresses by traversing block device structures
 *
 * This function finds all gendisk (generic disk) structures by examining
 * block device structures and extracting their associated disk pointers.
 * It serves as a fallback method when the bdev_map approach is not available
 * or doesn't return results.
 *
 * The function works by:
 * 1. Checking if the bd_disk field exists in the block_device structure
 * 2. Getting all block device addresses from the system
 * 3. For each block device, reading the bd_disk field to get the gendisk address
 * 4. Validating each gendisk address as a kernel virtual address
 * 5. Collecting all valid gendisk addresses
 *
 * @return Vector of gendisk structure addresses found through block devices
 *         Returns empty vector if bd_disk field doesn't exist or no devices found
 *
 */
std::vector<ulong> ParserPlugin::get_disk_by_block_device(){
    std::vector<ulong> dev_list;
    if (field_offset(block_device, bd_disk) == -1){
        return dev_list;
    }
    for (auto& addr : for_each_bdev()) {
        ulong bd_disk = read_pointer(addr + field_offset(block_device,bd_disk),"bd_disk");
        if (!is_kvaddr(bd_disk)) continue;
        dev_list.push_back(bd_disk);
    }
    return dev_list;
}

/**
 * Read a field value from a kernel structure at a given address
 *
 * This function reads a specific field from a kernel structure by calculating
 * the field's offset within the structure and reading the appropriate number
 * of bytes based on the field's size. It handles different field sizes
 * (1, 2, 4, 8 bytes) and returns the value as an unsigned long long.
 *
 * The function performs the following operations:
 * 1. Validates the input address as a kernel virtual address
 * 2. Looks up the field's offset and size from the type information table
 * 3. Calculates the absolute address of the field (base address + offset)
 * 4. Reads the field data from memory using the crash utility's readmem function
 * 5. Converts the raw bytes to the appropriate integer type based on field size
 * 6. Returns the value as an unsigned long long for consistency
 *
 * @param addr The kernel virtual address of the structure base
 * @param type The name of the structure type (e.g., "task_struct", "mm_struct")
 * @param field The name of the field within the structure (e.g., "pid", "comm")
 * @param virt Whether to treat the address as virtual (true) or physical (false)
 * @return The field value as an unsigned long long, or 0 if reading failed
 *
 */
ulonglong ParserPlugin::read_structure_field(ulong addr,const std::string& type,const std::string& field,bool virt){
    int offset = type_offset(type,field);
    if (offset == -1) {
        return 0;
    }
    int size = type_size(type,field);
    if (size <= 0) {
        return 0;
    }
    std::string note = type + "_" + field;
    addr += offset;
    void *buf = read_memory(addr, size, note, virt);
    if (!buf) {
        return 0;
    }
    ulonglong result;
    switch(size){
        case 1:
            result = UCHAR(buf);
            break;
        case 2:
            result = USHORT(buf);
            break;
        case 4:
            result = UINT(buf);
            break;
        case 8:
            result = ULONGLONG(buf);
            break;
        default:
            result = ULONG(buf);
    }
    FREEBUF(buf);
    return result;
}

/**
 * Read a null-terminated string from kernel memory with automatic length detection
 *
 * This function reads a string from kernel memory by automatically detecting its length
 * through iterative page-aligned reads. It handles strings that span multiple memory
 * pages and includes safety checks to prevent infinite loops and excessive memory usage.
 *
 * The function reads memory in page-aligned chunks to optimize memory access patterns
 * and avoid crossing page boundaries unnecessarily. It continues reading until it
 * encounters a null terminator or reaches safety limits.
 *
 * @param kvaddr The kernel virtual address where the string begins
 * @param note A descriptive note for debugging/error reporting purposes
 * @param virt Whether to treat the address as virtual (true) or physical (false)
 * @return The complete null-terminated string, or empty string if reading failed
 *
 */
std::string ParserPlugin::read_long_string(ulong kvaddr, const std::string& note, bool virt) {
    // mm_vmscan_direct_reclaim_begin
    char strbuf[MIN_PAGE_SIZE + 1] = {0};
    std::string result;
    ulong kp = kvaddr;
    while (true) {
        int page_offset = kp & (MIN_PAGE_SIZE - 1);
        int max_read = MIN_PAGE_SIZE - page_offset;
        BZERO(strbuf, sizeof(strbuf));
        if (!readmem(kp, virt ? KVADDR : PHYSADDR, strbuf, max_read,
                     TO_CONST_STRING(note.c_str()), QUIET | RETURN_ON_ERROR)) {
            return std::string();
        }
        int actual_len = strnlen(strbuf, max_read);
        if (actual_len == 0) {
            break;  // avoid stuck at loop
        }
        result.append(strbuf, actual_len);
        kp += actual_len;
        if (actual_len < max_read) {
            break;  // \0
        }
        if (result.size() > 1 << 20) {  // < 1MB
            LOGE("Warning: string too long at address %#lx\n", kvaddr);
            break;
        }
    }
    return result;
}

/**
 * Read a null-terminated string from kernel memory with specified maximum length
 *
 * This function reads a string from kernel memory at the specified address with
 * a maximum length limit. It uses either stack or heap allocation based on the
 * string length for optimal performance. The function ensures proper null
 * termination and handles memory management automatically.
 *
 * For small strings (≤256 bytes), it uses stack allocation to avoid heap overhead.
 * For larger strings, it uses heap allocation through the crash utility's buffer
 * management system. The function includes error handling for memory allocation
 * failures and invalid memory reads.
 *
 * @param addr The kernel virtual or physical address where the string begins
 * @param len The maximum number of bytes to read (including null terminator)
 * @param note A descriptive note for debugging/error reporting purposes
 * @param virt Whether to treat the address as virtual (true) or physical (false)
 * @return The null-terminated string read from memory, or empty string if reading failed
 *
 */
std::string ParserPlugin::read_cstring(ulong addr, int len, const std::string& note, bool virt) {
    if (len <= 0) {
        return "";
    }
    // Use stack buffer for small strings to avoid heap allocation
    constexpr int STACK_BUFFER_SIZE = 256;
    char stack_buffer[STACK_BUFFER_SIZE];
    char* buffer;
    bool use_heap = len > STACK_BUFFER_SIZE;
    if (use_heap) {
        buffer = static_cast<char*>(GETBUF(len));
        if (!buffer) {
            return "";
        }
    } else {
        buffer = stack_buffer;
    }
    if (!readmem(addr, (virt ? KVADDR : PHYSADDR), buffer, len, TO_CONST_STRING(note.c_str()), RETURN_ON_ERROR | QUIET)) {
        if (use_heap) {
            FREEBUF(buffer);
        }
        return "";
    }
    // Ensure null termination
    buffer[len - 1] = '\0';
    try {
        std::string result(buffer);
        if (use_heap) {
            FREEBUF(buffer);
        }
        return result;
    } catch (const std::exception&) {
        if (use_heap) {
            FREEBUF(buffer);
        }
        return "";
    }
}

/**
 * Read a boolean value from kernel memory
 *
 * This function reads a single boolean value from either virtual or physical
 * memory addresses using the crash utility's memory management system. It
 * handles error conditions gracefully by returning false on failure.
 *
 * The function performs the following operations:
 * 1. Attempts to read a single boolean byte from the specified address
 * 2. Uses either virtual or physical address translation based on the virt parameter
 * 3. Returns the boolean value on success, or false on memory read failure
 *
 * @param addr The memory address to read from (virtual or physical based on virt parameter)
 * @param note A descriptive string used for error reporting and debugging
 * @param virt Whether to treat the address as virtual (true) or physical (false)
 * @return The boolean value read from memory, or false if reading failed
 *
 */
bool ParserPlugin::read_bool(ulong addr,const std::string& note,bool virt){
    bool val;
    if (!readmem(addr, (virt ? KVADDR : PHYSADDR), &val, sizeof(bool), TO_CONST_STRING(note.c_str()), RETURN_ON_ERROR|QUIET)) {
        return false;
    }
    return val;
}

/**
 * Read a signed integer value from kernel memory
 *
 * This function reads a 32-bit signed integer from either virtual or physical
 * memory addresses using the crash utility's memory management system. It
 * handles error conditions gracefully by returning 0 on failure.
 *
 * The function performs the following operations:
 * 1. Attempts to read a 32-bit signed integer from the specified address
 * 2. Uses either virtual or physical address translation based on the virt parameter
 * 3. Returns the integer value on success, or 0 on memory read failure
 *
 * @param addr The memory address to read from (virtual or physical based on virt parameter)
 * @param note A descriptive string used for error reporting and debugging
 * @param virt Whether to treat the address as virtual (true) or physical (false)
 * @return The signed integer value read from memory, or 0 if reading failed
 *
 */
int ParserPlugin::read_int(ulong addr,const std::string& note,bool virt){
    int val;
    if (!readmem(addr, (virt ? KVADDR : PHYSADDR), &val, sizeof(int), TO_CONST_STRING(note.c_str()), RETURN_ON_ERROR|QUIET)) {
        return 0;
    }
    return val;
}

/**
 * Read an unsigned integer value from kernel memory
 *
 * This function reads a 32-bit unsigned integer from either virtual or physical
 * memory addresses using the crash utility's memory management system. It
 * handles error conditions gracefully by returning 0 on failure.
 *
 * The function performs the following operations:
 * 1. Attempts to read a 32-bit unsigned integer from the specified address
 * 2. Uses either virtual or physical address translation based on the virt parameter
 * 3. Returns the unsigned integer value on success, or 0 on memory read failure
 *
 * @param addr The memory address to read from (virtual or physical based on virt parameter)
 * @param note A descriptive string used for error reporting and debugging
 * @param virt Whether to treat the address as virtual (true) or physical (false)
 * @return The unsigned integer value read from memory, or 0 if reading failed
 *
 */
uint ParserPlugin::read_uint(ulong addr,const std::string& note,bool virt){
    uint val;
    if (!readmem(addr, (virt ? KVADDR : PHYSADDR), &val, sizeof(uint), TO_CONST_STRING(note.c_str()), RETURN_ON_ERROR|QUIET)) {
        return 0;
    }
    return val;
}

/**
 * Read a signed long value from kernel memory
 *
 * This function reads a platform-specific signed long integer from either virtual
 * or physical memory addresses using the crash utility's memory management system.
 * It handles error conditions gracefully by returning 0 on failure.
 *
 * The function performs the following operations:
 * 1. Attempts to read a platform-specific long integer from the specified address
 * 2. Uses either virtual or physical address translation based on the virt parameter
 * 3. Returns the long value on success, or 0 on memory read failure
 *
 * @param addr The memory address to read from (virtual or physical based on virt parameter)
 * @param note A descriptive string used for error reporting and debugging
 * @param virt Whether to treat the address as virtual (true) or physical (false)
 * @return The signed long value read from memory, or 0 if reading failed
 *
 */
long ParserPlugin::read_long(ulong addr,const std::string& note,bool virt){
    long val;
    if (!readmem(addr, (virt ? KVADDR : PHYSADDR), &val, sizeof(long), TO_CONST_STRING(note.c_str()), RETURN_ON_ERROR|QUIET)) {
        return 0;
    }
    return val;
}

/**
 * Read an unsigned long value from kernel memory
 *
 * This function reads a platform-specific unsigned long integer from either virtual
 * or physical memory addresses using the crash utility's memory management system.
 * It handles error conditions gracefully by returning 0 on failure.
 *
 * The function performs the following operations:
 * 1. Attempts to read a platform-specific unsigned long integer from the specified address
 * 2. Uses either virtual or physical address translation based on the virt parameter
 * 3. Returns the unsigned long value on success, or 0 on memory read failure
 *
 * @param addr The memory address to read from (virtual or physical based on virt parameter)
 * @param note A descriptive string used for error reporting and debugging
 * @param virt Whether to treat the address as virtual (true) or physical (false)
 * @return The unsigned long value read from memory, or 0 if reading failed
 *
 */
ulong ParserPlugin::read_ulong(ulong addr,const std::string& note,bool virt){
    ulong val;
    if (!readmem(addr, (virt ? KVADDR : PHYSADDR), &val, sizeof(ulong), TO_CONST_STRING(note.c_str()), RETURN_ON_ERROR|QUIET)) {
        return 0;
    }
    return val;
}

/**
 * Read an unsigned long long value from kernel memory
 *
 * This function reads a 64-bit unsigned long long integer from either virtual
 * or physical memory addresses using the crash utility's memory management system.
 * It handles error conditions gracefully by returning 0 on failure.
 *
 * The function performs the following operations:
 * 1. Attempts to read a 64-bit unsigned long long integer from the specified address
 * 2. Uses either virtual or physical address translation based on the virt parameter
 * 3. Returns the unsigned long long value on success, or 0 on memory read failure
 *
 * @param addr The memory address to read from (virtual or physical based on virt parameter)
 * @param note A descriptive string used for error reporting and debugging
 * @param virt Whether to treat the address as virtual (true) or physical (false)
 * @return The unsigned long long value read from memory, or 0 if reading failed
 *
 */
ulonglong ParserPlugin::read_ulonglong(ulong addr,const std::string& note,bool virt){
    ulonglong val;
    if (!readmem(addr, (virt ? KVADDR : PHYSADDR), &val, sizeof(ulonglong), TO_CONST_STRING(note.c_str()), RETURN_ON_ERROR|QUIET)) {
        return 0;
    }
    return val;
}

/**
 * Read an unsigned short value from kernel memory
 *
 * This function reads a 16-bit unsigned short integer from either virtual
 * or physical memory addresses using the crash utility's memory management system.
 * It handles error conditions gracefully by returning 0 on failure.
 *
 * The function performs the following operations:
 * 1. Attempts to read a 16-bit unsigned short integer from the specified address
 * 2. Uses either virtual or physical address translation based on the virt parameter
 * 3. Returns the unsigned short value on success, or 0 on memory read failure
 *
 * @param addr The memory address to read from (virtual or physical based on virt parameter)
 * @param note A descriptive string used for error reporting and debugging
 * @param virt Whether to treat the address as virtual (true) or physical (false)
 * @return The unsigned short value read from memory, or 0 if reading failed
 *
 */
ushort ParserPlugin::read_ushort(ulong addr,const std::string& note,bool virt){
    ushort val;
    if (!readmem(addr, (virt ? KVADDR : PHYSADDR), &val, sizeof(ushort), TO_CONST_STRING(note.c_str()), RETURN_ON_ERROR|QUIET)) {
        return 0;
    }
    return val;
}

/**
 * Read a signed short value from kernel memory
 *
 * This function reads a 16-bit signed short integer from either virtual
 * or physical memory addresses using the crash utility's memory management system.
 * It handles error conditions gracefully by returning 0 on failure.
 *
 * The function performs the following operations:
 * 1. Attempts to read a 16-bit signed short integer from the specified address
 * 2. Uses either virtual or physical address translation based on the virt parameter
 * 3. Returns the short value on success, or 0 on memory read failure
 *
 * @param addr The memory address to read from (virtual or physical based on virt parameter)
 * @param note A descriptive string used for error reporting and debugging
 * @param virt Whether to treat the address as virtual (true) or physical (false)
 * @return The signed short value read from memory, or 0 if reading failed
 *
 */
short ParserPlugin::read_short(ulong addr,const std::string& note,bool virt){
    short val;
    if (!readmem(addr, (virt ? KVADDR : PHYSADDR), &val, sizeof(short), TO_CONST_STRING(note.c_str()), RETURN_ON_ERROR|QUIET)) {
        return 0;
    }
    return val;
}

/**
 * Read a block of memory from kernel address space
 *
 * This function reads a specified number of bytes from either virtual or physical
 * memory addresses using the crash utility's memory management system. It allocates
 * a buffer for the data and handles error conditions gracefully by returning nullptr
 * on failure.
 *
 * The function performs the following operations:
 * 1. Validates the length parameter to ensure it's positive
 * 2. Allocates a buffer using the crash utility's memory management (GETBUF)
 * 3. Attempts to read memory using readmem() with error handling flags
 * 4. Returns the allocated buffer on success, or cleans up and returns nullptr on failure
 *
 * @param addr The memory address to read from (virtual or physical based on virt parameter)
 * @param len The number of bytes to read from memory
 * @param note A descriptive string used for error reporting and debugging
 * @param virt Whether to treat the address as virtual (true) or physical (false)
 * @return Pointer to allocated buffer containing the read data, or nullptr if reading failed
 *
 */
void* ParserPlugin::read_memory(ulong addr, int len, const std::string& note, bool virt) {
    if (len <= 0) {
        return nullptr;
    }
    void* buf = GETBUF(len);
    if (!buf) {
        return nullptr;
    }
    if (!readmem(addr, (virt ? KVADDR : PHYSADDR), buf, len, TO_CONST_STRING(note.c_str()), RETURN_ON_ERROR | QUIET)) {
        FREEBUF(buf);
        return nullptr;
    }
    return buf;
}

/**
 * Read a complete kernel structure from memory
 *
 * This function reads an entire kernel structure from either virtual or physical
 * memory addresses using the crash utility's memory management system. It allocates
 * a buffer sized to match the structure and handles error conditions gracefully
 * by returning nullptr on failure.
 *
 * The function performs the following operations:
 * 1. Validates the input address as a kernel virtual address
 * 2. Looks up the structure size from the type information table
 * 3. Allocates a buffer using the crash utility's memory management (GETBUF)
 * 4. Attempts to read the entire structure using readmem() with error handling flags
 * 5. Returns the allocated buffer on success, or cleans up and returns nullptr on failure
 *
 * @param addr The memory address to read from (virtual or physical based on virt parameter)
 * @param type The name of the structure type to read (e.g., "task_struct", "mm_struct")
 * @param virt Whether to treat the address as virtual (true) or physical (false)
 * @return Pointer to allocated buffer containing the structure data, or nullptr if reading failed
 *
 */
void* ParserPlugin::read_struct(ulong addr, const std::string& type, bool virt) {
    if (!is_kvaddr(addr)) {
        return nullptr;
    }
    int size = type_size(type);
    if (size <= 0) {
        return nullptr;
    }
    void* buf = GETBUF(size);
    if (!buf) {
        return nullptr;
    }
    if (!readmem(addr, (virt ? KVADDR : PHYSADDR), buf, size, TO_CONST_STRING(type.c_str()), RETURN_ON_ERROR|QUIET)) {
        FREEBUF(buf);
        return nullptr;
    }
    return buf;
}


/**
 * Read a kernel structure into a provided buffer
 *
 * This function reads a specified number of bytes from either virtual or physical
 * memory addresses into a user-provided buffer using the crash utility's memory
 * management system. It handles error conditions gracefully by returning false
 * on failure and includes error reporting for debugging purposes.
 *
 * The function performs the following operations:
 * 1. Validates the provided buffer pointer and length parameters
 * 2. Attempts to read the specified number of bytes from the given address
 * 3. Uses either virtual or physical address translation based on the virt parameter
 * 4. Reports errors to the output file pointer if the read operation fails
 * 5. Returns success/failure status to the caller
 *
 * @param addr The memory address to read from (virtual or physical based on virt parameter)
 * @param buf Pointer to the buffer where the read data will be stored
 * @param len The number of bytes to read from memory
 * @param note A descriptive string used for error reporting and debugging
 * @param virt Whether to treat the address as virtual (true) or physical (false)
 * @return true if the read operation succeeded, false if it failed
 *
 */
bool ParserPlugin::read_struct(ulong addr, void* buf, int len, const std::string& note, bool virt) {
    if (!buf || len <= 0) {
        return false;
    }
    if (!readmem(addr, (virt ? KVADDR : PHYSADDR), buf, len, TO_CONST_STRING(note.c_str()), RETURN_ON_ERROR|QUIET)) {
        LOGE("Can't read %s at %lx\n",TO_CONST_STRING(note.c_str()),addr);
        return false;
    }
    return true;
}

/**
 * Read a pointer value from kernel memory
 *
 * This function reads a platform-specific pointer value from either virtual
 * or physical memory addresses using the crash utility's memory management system.
 * It handles error conditions gracefully by returning 0 on failure.
 *
 * The function performs the following operations:
 * 1. Attempts to read a platform-specific pointer from the specified address
 * 2. Uses either virtual or physical address translation based on the virt parameter
 * 3. Returns the pointer value on success, or 0 on memory read failure
 *
 * @param addr The memory address to read from (virtual or physical based on virt parameter)
 * @param note A descriptive string used for error reporting and debugging
 * @param virt Whether to treat the address as virtual (true) or physical (false)
 * @return The pointer value read from memory, or 0 if reading failed
 *
 */
ulong ParserPlugin::read_pointer(ulong addr, const std::string& note, bool virt) {
    ulong val;
    if (!readmem(addr, (virt ? KVADDR : PHYSADDR), &val, sizeof(ulong), TO_CONST_STRING(note.c_str()), RETURN_ON_ERROR|QUIET)) {
        return 0;
    }
    return val;
}

/**
 * Read a single byte value from kernel memory
 *
 * This function reads a single unsigned byte (8-bit value) from either virtual
 * or physical memory addresses using the crash utility's memory management system.
 * It handles error conditions gracefully by returning 0 on failure.
 *
 * The function performs the following operations:
 * 1. Attempts to read a single unsigned byte from the specified address
 * 2. Uses either virtual or physical address translation based on the virt parameter
 * 3. Returns the byte value on success, or 0 on memory read failure
 *
 * @param addr The memory address to read from (virtual or physical based on virt parameter)
 * @param note A descriptive string used for error reporting and debugging
 * @param virt Whether to treat the address as virtual (true) or physical (false)
 * @return The unsigned byte value read from memory, or 0 if reading failed
 *
 */
unsigned char ParserPlugin::read_byte(ulong addr, const std::string& note, bool virt) {
    unsigned char val;
    if (!readmem(addr, (virt ? KVADDR : PHYSADDR), &val, sizeof(unsigned char), TO_CONST_STRING(note.c_str()), RETURN_ON_ERROR|QUIET)) {
        return 0;
    }
    return val;
}

/**
 * Check if a kernel symbol exists in the symbol table
 *
 * This function checks whether a given symbol name exists in the kernel's
 * symbol table. It's a wrapper around the crash utility's symbol_exists()
 * function that provides a more convenient C++ string interface.
 *
 * @param note The name of the kernel symbol to check for (e.g., "init_task", "jiffies")
 * @return Non-zero if the symbol exists, 0 if it doesn't exist
 *
 */
int ParserPlugin::csymbol_exists(const std::string& note){
    return symbol_exists(TO_CONST_STRING(note.c_str()));
}

/**
 * Get the kernel virtual address of a symbol by name
 *
 * This function retrieves the kernel virtual address of a symbol from the
 * kernel's symbol table. It's a wrapper around the crash utility's symbol_value()
 * function that provides a more convenient C++ string interface for symbol lookup.
 *
 * @param note The name of the kernel symbol to look up (e.g., "init_task", "jiffies")
 * @return The kernel virtual address of the symbol if found
 *
 */
ulong ParserPlugin::csymbol_value(const std::string& note){
    return symbol_value(TO_CONST_STRING(note.c_str()));
}

/**
 * Check if an address is a valid kernel virtual address
 *
 * This function determines whether a given address falls within the kernel's
 * virtual address space range. It's a wrapper around the crash utility's
 * IS_KVADDR() macro that validates kernel address ranges based on the
 * target architecture and kernel configuration.
 *
 * @param addr The address to validate as a kernel virtual address
 * @return true if the address is within kernel virtual address space, false otherwise
 *
 */
bool ParserPlugin::is_kvaddr(ulong addr){
    return IS_KVADDR(addr);
}

/**
 * Check if an address is a valid user virtual address for a given task
 *
 * This function determines whether a given address falls within the user
 * virtual address space range for a specific task context. It's a wrapper
 * around the crash utility's IS_UVADDR() macro that validates user address
 * ranges based on the target architecture and task's memory layout.
 *
 * @param addr The address to validate as a user virtual address
 * @param tc The task context structure containing the task information
 * @return true if the address is within user virtual address space, false otherwise
 *
 */
bool ParserPlugin::is_uvaddr(ulong addr, struct task_context* tc){
    return !addr ? false : IS_UVADDR(addr,tc);
}

/**
 * Get the NUMA node ID for a given page structure
 *
 * This function determines which NUMA (Non-Uniform Memory Access) node
 * a given page structure belongs to. It converts the page structure to
 * its corresponding physical address and then searches through the
 * system's node table to find which node contains that physical address.
 *
 * The function performs the following operations:
 * 1. Validates the page address as a kernel virtual address
 * 2. Converts the page structure to its physical address
 * 3. Caches frequently accessed values for performance
 * 4. Iterates through all NUMA nodes to find the containing node
 * 5. Checks if the physical address falls within each node's memory range
 *
 * @param page The kernel virtual address of the page structure
 * @return The NUMA node ID (0-based) if found, -1 if not found or invalid page
 *
 */
int ParserPlugin::page_to_nid(ulong page){
    if (!is_kvaddr(page)) {
        return -1;
    }
    physaddr_t paddr = page_to_phy(page);
    if (paddr == 0) {
        return -1;
    }
    // Cache frequently accessed values
    int numnodes = vt->numnodes;
    ulong page_size_val = page_size;

    for (int i = 0; i < numnodes; i++) {
        struct node_table *nt = &vt->node_table[i];
        physaddr_t end_paddr = nt->start_paddr + ((physaddr_t)nt->size * (physaddr_t)page_size_val);
        if ((paddr >= nt->start_paddr) && (paddr < end_paddr)) {
            return i;
        }
    }
    return -1;
}

/**
 * Convert a virtual address to its corresponding physical address
 *
 * This function translates a virtual memory address to its corresponding
 * physical memory address using the crash utility's virtual-to-physical
 * address translation mechanism. It handles the page table traversal
 * and address mapping automatically.
 *
 * @param vaddr The virtual address to convert
 * @return The corresponding physical address, or 0 if translation fails
 *
 */
ulong ParserPlugin::virt_to_phy(ulong vaddr){
    return VTOP(vaddr);
}

/**
 * Convert a physical address to its corresponding virtual address
 *
 * This function translates a physical memory address to its corresponding
 * virtual memory address using the crash utility's physical-to-virtual
 * address translation mechanism. It handles the reverse mapping from
 * physical addresses back to kernel virtual addresses.
 *
 * @param paddr The physical address to convert
 * @return The corresponding virtual address, or 0 if translation fails
 *
 */
ulong ParserPlugin::phy_to_virt(ulong paddr){
    return PTOV(paddr);
}

/**
 * Convert a physical address to its corresponding page frame number (PFN)
 *
 * This function calculates the page frame number from a physical address
 * by dividing the address by the page size. PFNs are used extensively
 * in kernel memory management to identify physical memory pages.
 *
 * @param paddr The physical address to convert
 * @return The page frame number corresponding to the physical address
 *
 */
ulong ParserPlugin::phy_to_pfn(ulong paddr){
    return BTOP(paddr);
}

/**
 * Convert a page frame number (PFN) to its corresponding physical address
 *
 * This function calculates the physical address from a page frame number
 * by multiplying the PFN by the page size. This gives the base physical
 * address of the memory page identified by the PFN.
 *
 * @param pfn The page frame number to convert
 * @return The physical address of the page's base address
 *
 */
physaddr_t ParserPlugin::pfn_to_phy(ulong pfn){
    return PTOB(pfn);
}

/**
 * Convert a page structure address to its corresponding page frame number (PFN)
 *
 * This function extracts the page frame number from a kernel page structure
 * by first converting the page structure to its physical address, then
 * converting that physical address to a PFN. This is useful for memory
 * analysis operations that need to work with PFN-based interfaces.
 *
 * @param page The kernel virtual address of the page structure
 * @return The page frame number corresponding to the page structure
 *
 */
ulong ParserPlugin::page_to_pfn(ulong page){
    if (!is_kvaddr(page)) {
        return 0;
    }
    physaddr_t paddr = page_to_phy(page);
    return paddr ? phy_to_pfn(paddr) : 0;
}

/**
 * Convert a page frame number (PFN) to its corresponding page structure address
 *
 * This function converts a page frame number to the kernel virtual address of
 * the corresponding page structure. It first converts the PFN to a physical
 * address, then converts that physical address to the page structure address.
 * This is a convenience function that combines pfn_to_phy() and phy_to_page()
 * operations.
 *
 * @param pfn The page frame number to convert
 * @return The kernel virtual address of the page structure, or 0 if conversion fails
 *
 */
ulong ParserPlugin::pfn_to_page(ulong pfn){
    return phy_to_page(pfn_to_phy(pfn));
}

/**
 * Convert a physical address to its corresponding page structure address
 *
 * This function finds the kernel page structure address for a given physical
 * address using the crash utility's physical-to-page conversion mechanism.
 * The page structure contains metadata about the physical memory page,
 * including reference counts, mapping information, and flags.
 *
 * @param paddr The physical address to convert
 * @return The kernel virtual address of the page structure, or 0 if conversion fails
 *
 */
ulong ParserPlugin::phy_to_page(ulong paddr){
    ulong page;
    return phys_to_page(paddr, &page) ? page : 0;
}

/**
 * Convert a page structure address to its corresponding physical address
 *
 * This function converts a kernel page structure address to the corresponding
 * physical memory address. It uses the crash utility's is_page_ptr() function
 * to validate the page pointer and extract the physical address.
 *
 * @param page The kernel virtual address of the page structure
 * @return The physical address corresponding to the page, or 0 if invalid
 *
 */
physaddr_t ParserPlugin::page_to_phy(ulong page){
    if (!is_kvaddr(page)) {
        return 0;
    }
    physaddr_t paddr = 0;
    return is_page_ptr(page, &paddr) ? paddr : 0;
}

/**
 * Retrieve a kernel configuration value by name
 *
 * This function queries the kernel's configuration settings to retrieve the
 * value of a specific configuration option. It uses the crash utility's
 * get_kernel_config() function to access the kernel's built-in configuration
 * data (CONFIG_IKCONFIG).
 *
 * @param conf_name The name of the configuration option to retrieve (e.g., "CONFIG_ARM64_VA_BITS")
 * @return The configuration value as a string, or "n" if not found or disabled
 *
 */
std::string ParserPlugin::get_config_val(const std::string& conf_name){
    char *config_val;
    if (get_kernel_config(TO_CONST_STRING(conf_name.c_str()), &config_val) != IKCONFIG_N){
        return std::string(config_val);
    }
    return "n";
}

/**
 * Fill the page global directory (PGD) cache with data from memory
 *
 * This function reads page global directory data from memory into the crash
 * utility's internal PGD cache. It implements caching to avoid redundant
 * memory reads by checking if the requested PGD page is already loaded.
 * The PGD is the top level of the page table hierarchy in virtual memory
 * management.
 *
 * @param pgd The physical or virtual address of the PGD page to read
 * @param type The address type (KVADDR for virtual, PHYSADDR for physical)
 * @param size The size of the PGD page to read (typically page size)
 *
 */
void ParserPlugin::cfill_pgd(ulonglong pgd, int type, ulong size){
    if (!IS_LAST_PGD_READ(pgd)) {
        readmem(pgd, type, machdep->pgd, size, TO_CONST_STRING("pgd page"), FAULT_ON_ERROR);
        machdep->last_pgd_read = (ulong)(pgd);
    }
}

void ParserPlugin::cfill_pmd(ulonglong pmd, int type, ulong size){
    if (!IS_LAST_PMD_READ(pmd)) {
        readmem(pmd, type, machdep->pmd, size, TO_CONST_STRING("pmd page"), FAULT_ON_ERROR);
        machdep->last_pmd_read = (ulong)(pmd);
    }
}

void ParserPlugin::cfill_ptbl(ulonglong ptbl, int type, ulong size){
    if (!IS_LAST_PTBL_READ(ptbl)) {
        readmem(ptbl, type, machdep->ptbl, size, TO_CONST_STRING("page table"), FAULT_ON_ERROR);
        machdep->last_ptbl_read = (ulong)(ptbl);
    }
}

// maybe we can refer to symbols.c is_binary_stripped
bool ParserPlugin::is_binary_stripped(std::string& filename) {
    int fd = open(filename.c_str(), O_RDONLY);
    if (fd < 0) {
        LOGE("Failed to open file: %s\n", filename.c_str());
        return false;
    }
    if (elf_version(EV_CURRENT) == EV_NONE) {
        LOGE("ELF library initialization failed\n");
        close(fd);
        return false;
    }
    Elf *elf = elf_begin(fd, ELF_C_READ, nullptr);
    if (!elf) {
        LOGE("Failed to read ELF file\n");
        close(fd);
        return false;
    }
    bool is_stripped = true;
    Elf_Scn *scn = nullptr;
    while ((scn = elf_nextscn(elf, scn)) != nullptr) {
        GElf_Shdr shdr;
        if (gelf_getshdr(scn, &shdr) != &shdr) {
            continue;
        }
        if (shdr.sh_type == SHT_SYMTAB) {
            is_stripped = false;
            break;
        }
    }
    elf_end(elf);
    close(fd);
    return is_stripped;
}

bool ParserPlugin::add_symbol_file(std::string& filename){
    if(is_elf_file(TO_CONST_STRING(filename.c_str())) && is_binary_stripped(filename)){
        LOGE("This file is not symbols file \n");
        return false;
    }
    char buf[BUFSIZE];
    sprintf(buf, "add-symbol-file %s", filename.c_str());
    if(!gdb_pass_through(buf, NULL, GNU_RETURN_ON_ERROR)){
        return false;
    }
    return true;
}

std::string ParserPlugin::extract_string(const char *input) {
    std::string result;
    const char *ptr = input;
    while (*ptr != '\0') {
        if (!result.empty()) {
            result += ' ';
        }
        result += std::string(ptr);
        ptr += strlen(ptr) + 1;
    }
    return result;
}

/**
 * Check if the system uses big-endian byte ordering
 *
 * This function determines the endianness of the target system by examining
 * how a multi-byte integer is stored in memory. It creates a test value and
 * checks whether the most significant byte is stored at the lowest memory
 * address (big-endian) or the least significant byte is stored at the lowest
 * memory address (little-endian).
 *
 * The function uses a test integer value 0x12345678 and examines the first
 * byte in memory. If the first byte is 0x12 (the most significant byte),
 * then the system is big-endian. If it's 0x78 (the least significant byte),
 * then the system is little-endian.
 *
 * @return TRUE if the system uses big-endian byte ordering, FALSE if little-endian
 *
 */
int ParserPlugin::is_bigendian(void){
    constexpr int test_value = 0x12345678;
    return (*(const char*)&test_value == 0x12) ? TRUE : FALSE;
}

/**
 * Retrieve all enumerator names from a kernel enumeration type
 *
 * This function extracts all enumerator names from a specified kernel enumeration
 * type using the crash utility's dump_enumerator_list() function. It parses the
 * output to extract just the enumerator names, excluding their values.
 *
 * The function performs the following operations:
 * 1. Opens a temporary file for capturing the enumeration dump output
 * 2. Uses dump_enumerator_list() to get all enumerators and their values
 * 3. Parses each line to extract the enumerator name (before the '=' sign)
 * 4. Trims whitespace from the extracted names
 * 5. Collects all valid enumerator names into a vector
 *
 * @param enum_name The name of the kernel enumeration type to query (e.g., "zone_type", "migrate_mode")
 * @return Vector of enumerator name strings found in the enumeration
 *         Returns empty vector if enumeration doesn't exist or has no enumerators
 *
 */
std::vector<std::string> ParserPlugin::get_enumerator_list(const std::string &enum_name){
    std::vector<std::string> result;
    result.reserve(32); // Reserve space for typical enum size
    char buf[BUFSIZE];
    open_tmpfile();
    if (dump_enumerator_list(TO_CONST_STRING(enum_name.c_str()))){
        rewind(pc->tmpfile);
        while (fgets(buf, BUFSIZE, pc->tmpfile)){
            std::string line = buf;
            size_t pos = line.find('=');
            if (pos == std::string::npos) {
                continue;
            }else{
                std::string name = line.substr(0, pos - 1);
                size_t first = name.find_first_not_of(' ');
                size_t last = name.find_last_not_of(' ');
                if (first == std::string::npos || last == std::string::npos) {
                    continue;
                }
                result.push_back(name.substr(first, (last - first + 1)));
            }
        }
    }
    close_tmpfile();
    return result;
}

/**
 * Retrieve the numeric value of a kernel enumeration constant
 *
 * This function looks up the numeric value associated with a specific
 * enumerator name in the kernel's symbol table. It uses the crash utility's
 * enumerator_value() function to perform the lookup and returns the
 * corresponding integer value.
 *
 * The function is useful for obtaining compile-time constant values that
 * are defined as enumerators in kernel header files. These values can
 * vary between kernel versions or configurations, so runtime lookup is
 * often necessary for accurate analysis.
 *
 * @param enum_name The name of the enumerator to look up (e.g., "ZONE_DMA", "GFP_KERNEL")
 * @return The numeric value of the enumerator if found, or 0 if not found
 *
 */
long ParserPlugin::read_enum_val(const std::string& enum_name){
     long enum_val = 0;
     enumerator_value(TO_CONST_STRING(enum_name.c_str()), &enum_val);
     return enum_val;
}

std::map<std::string, ulong> ParserPlugin::read_enum_list(const std::string& enum_list_name){
    char cmd_buf[BUFSIZE], ret_buf[BUFSIZE*5];
    FILE *tmp_fp = fmemopen(ret_buf, sizeof(ret_buf), "w");
    sprintf(cmd_buf, "ptype enum %s", enum_list_name.c_str());
    gdb_pass_through(cmd_buf, tmp_fp, GNU_RETURN_ON_ERROR);
    fclose(tmp_fp);
    std::string input(ret_buf);
    std::string content = input.substr(input.find('{') + 1, input.find('}') - input.find('{')-1);

    std::map<std::string, ulong> enum_list;
    std::istringstream ss(content);
    std::string item;
    int currentValue = 0;
    while (std::getline(ss, item, ',')) {
        size_t equalPos = item.find('=');
        std::string key = (equalPos != std::string::npos)?item.substr(0, equalPos):item;
        int value = (equalPos != std::string::npos)?std::stoi(item.substr(equalPos+1)):currentValue++;
        key.erase(0, key.find_first_not_of(" \t\n\r"));
        key.erase(key.find_last_not_of(" \t\n\r") + 1);
        enum_list.insert(std::make_pair(key, value));
    }
    return enum_list;
}

std::string ParserPlugin::hexdump(uint64_t addr, const char* buf, size_t length, bool little_endian) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    uint64_t current_addr = addr;
    for (size_t i = 0; i < length; i += 16) {
        oss << std::setw(8) << current_addr << ":  ";
        for (int block = 0; block < 2; block++) {
            size_t block_start = i + block * 8;
            if (block_start < length) {
                uint64_t value = 0;
                size_t bytes_available = std::min(static_cast<size_t>(8), length - block_start);
                if (little_endian) {
                    memcpy(&value, buf + block_start, bytes_available);
                    uint32_t low_part = static_cast<uint32_t>(value & 0xFFFFFFFF);
                    uint32_t high_part = static_cast<uint32_t>((value >> 32) & 0xFFFFFFFF);
                    oss << std::setw(8) << high_part << std::setw(8) << low_part;
                } else {
                    uint32_t low_part = 0, high_part = 0;
                    size_t bytes_to_read = std::min(static_cast<size_t>(4), bytes_available);
                    for (size_t j = 0; j < bytes_to_read; j++) {
                        low_part = (low_part << 8) | static_cast<uint8_t>(buf[block_start + j]);
                    }
                    if (bytes_available > 4) {
                        bytes_to_read = bytes_available - 4;
                        for (size_t j = 0; j < bytes_to_read; j++) {
                            high_part = (high_part << 8) | static_cast<uint8_t>(buf[block_start + 4 + j]);
                        }
                    }
                    oss << std::setw(8) << high_part << std::setw(8) << low_part;
                }
            } else {
                oss << "                ";
            }
            if (block == 0) oss << " ";
        }
        oss << "   ";
        size_t line_end = std::min(i + 16, length);
        for (size_t j = i; j < line_end; ++j) {
            uint8_t byte = static_cast<uint8_t>(buf[j]);
            oss << static_cast<char>((byte >= 32 && byte <= 126) ? byte : '.');
        }
        for (size_t j = line_end; j < i + 16; ++j) {
            oss << " ";
        }
        if (i + 16 < length) {
            oss << "\n";
        }
        current_addr += 16;
    }
    return oss.str();
}

std::stringstream ParserPlugin::get_curpath() {
    std::stringstream ss;
    char tmp_buf[PATH_MAX];
    if (getcwd(tmp_buf, sizeof(tmp_buf)) != nullptr) {
        ss << tmp_buf;
    }
    return ss;
}

std::shared_ptr<stack_record_t> ParserPlugin::get_stack_record(uint handle) {
    ulong offset;
    int slabindex;
    if (handle <= 0){
        return nullptr;
    }
    union handle_parts parts = { .handle = handle };
    if (field_offset(handle_parts, pool_index_plus_1) != -1){
        offset = parts.v3.offset << DEPOT_STACK_ALIGN;
        slabindex = parts.v3.pool_index;
        // https://lore.kernel.org/all/20240402001500.53533-1-pcc@google.com/T/#u
        slabindex -= 1;
    } else if (field_offset(handle_parts, pool_index) != -1){
        offset = parts.v2.offset << DEPOT_STACK_ALIGN;
        slabindex = parts.v2.pool_index;
    } else {
        offset = parts.v1.offset << DEPOT_STACK_ALIGN;
        slabindex = parts.v1.pool_index;
    }
    if (slabindex > depot_index) return nullptr;
    ulong page_addr = read_pointer(stack_slabs + slabindex * sizeof(void *),"stack_record_page");
    if (!is_kvaddr(page_addr))return nullptr;
    std::shared_ptr<stack_record_t> record_ptr = std::make_shared<stack_record_t>();
    record_ptr->slab_index = slabindex;
    record_ptr->record_offset = offset;
    record_ptr->slab_addr = page_addr;
    record_ptr->record_addr = page_addr + offset;
    return record_ptr;
}

std::string ParserPlugin::get_call_stack(std::shared_ptr<stack_record_t> record_ptr) {
    std::ostringstream oss;
    if (!is_kvaddr(record_ptr->record_addr)){
        return oss.str();
    }
    if (struct_size(stack_record) == -1){
        field_init(stack_record,next);
        field_init(stack_record,size);
        field_init(stack_record,handle);
        field_init(stack_record,entries);
        struct_init(stack_record);
    }
    void *record_buf = read_struct(record_ptr->record_addr, "stack_record");
    if (record_buf == nullptr) return oss.str();
    uint32_t nr_size = UINT(record_buf + field_offset(stack_record, size));
    // uint32_t record_handle = UINT(record_buf + field_offset(stack_record, handle));
    FREEBUF(record_buf);
    ulong entries = record_ptr->record_addr + field_offset(stack_record, entries);
    uint32_t entry_len = field_size(stack_record,entries)/sizeof(unsigned long);
    if (entry_len == 0){
        entry_len = 64;
    }
    if(is_kvaddr(entries) && nr_size < entry_len){
        for(uint i = 0; i < nr_size; i++){
            ulong frame_addr = read_pointer(entries + sizeof(unsigned long) * i, "frame_addr");
            oss << "[<" << std::hex << frame_addr << ">] " << to_symbol(frame_addr) << "\n";
        }
    }
    return oss.str();
}

/**
 * Retrieve a list of mount points from the kernel's mount namespace
 *
 * This function extracts mount point information from the kernel's VFS layer,
 * including mount paths and root dentry addresses. It handles both modern
 * (struct mount + struct vfsmount) and legacy (struct vfsmount only) kernel
 * structures to maintain compatibility across different kernel versions.
 *
 * @param tc Task context to determine the mount namespace (can be NULL for init)
 * @return Vector of mount_point structures containing mount information
 *
 */
std::vector<std::shared_ptr<mount_point>> ParserPlugin::get_mntpoint_list(struct task_context *tc) {
    std::vector<std::shared_ptr<mount_point>> mount_points;
    int cnt = 0;
    ulong *mount_list = get_mount_list(&cnt, tc);
    if (!mount_list || cnt <= 0) {
        return mount_points;
    }
    mount_points.reserve(cnt);
    // Cache field offsets to avoid repeated lookups
    bool use_mount_struct = (field_offset(mount, mnt_parent) != -1);
    int parent_offset, mountpoint_offset, root_offset;
    int mnt_offset = 0;
    if (use_mount_struct) {
        parent_offset = field_offset(mount, mnt_parent);
        mountpoint_offset = field_offset(mount, mnt_mountpoint);
        mnt_offset = field_offset(mount, mnt);
        root_offset = mnt_offset + field_offset(vfsmount, mnt_root);
    } else {
        parent_offset = field_offset(vfsmount, mnt_parent);
        mountpoint_offset = field_offset(vfsmount, mnt_mountpoint);
        root_offset = field_offset(vfsmount, mnt_root);
    }
    char buf[PATH_MAX];
    for (int i = 0; i < cnt; ++i) {
        ulong addr = mount_list[i];
        if (!is_kvaddr(addr)) continue;
        auto mount_ptr = std::make_shared<mount_point>();
        mount_ptr->addr = addr;

        ulong parent = read_ulong(addr + parent_offset, "mnt_parent");
        ulong mountp = read_ulong(addr + mountpoint_offset, "mnt_mountpoint");
        ulong root = read_ulong(addr + root_offset, "mnt_root");

        ulong parent_vfs = use_mount_struct ? (parent + mnt_offset) : parent;
        get_pathname(mountp, buf, PATH_MAX, 1, parent_vfs);

        mount_ptr->path = buf;
        mount_ptr->root_dentry = root;
        mount_points.push_back(std::move(mount_ptr));
    }
    FREEBUF(mount_list);
    return mount_points;
}

/**
 * Find the vfsmount structure associated with a given dentry's superblock
 *
 * This function searches through all mount points in the system to find the
 * vfsmount structure that corresponds to the superblock of the given dentry.
 * It handles kernel version compatibility by checking for both modern
 * (struct mount + struct vfsmount) and legacy (struct vfsmount only) structures.
 *
 * The function performs the following operations:
 * 1. Validates the input dentry address as a kernel virtual address
 * 2. Reads the superblock address from the dentry structure
 * 3. Caches field offsets to avoid repeated lookups for performance
 * 4. Iterates through all mount points in the system (using init process context)
 * 5. For each mount point, reads the superblock address from the mount structure
 * 6. Compares superblock addresses to find the matching mount
 * 7. Returns the vfsmount address (adjusted for struct mount if necessary)
 *
 * @param dentry The kernel virtual address of the dentry structure
 * @return The vfsmount address if found, 0 if not found or invalid dentry
 *
 */
ulong ParserPlugin::find_vfsmount_by_superblock(ulong dentry) {
    if (!is_kvaddr(dentry)) {
        return 0;
    }
    ulong sb_addr = read_pointer(dentry + field_offset(dentry,d_sb),"super_block");
    if (!is_kvaddr(sb_addr)) {
        return 0;
    }
    // Cache field offsets to avoid repeated lookups
    int mount_mnt_sb_offset = field_offset(mount, mnt_sb);
    int mount_mnt_offset = field_offset(mount, mnt);
    int vfsmount_mnt_sb_offset = field_offset(vfsmount, mnt_sb);
    struct task_context *tc;
    ulong pid = 0;
	while ((tc = pid_to_context(pid)) == NULL){
        pid++;
    }
    std::vector<std::shared_ptr<mount_point>> mount_points = get_mntpoint_list(tc);
    for (const auto& mnt : mount_points) {
        ulong mnt_sb = 0;
        if (mount_mnt_sb_offset != -1) {
            mnt_sb = read_pointer(mnt->addr + mount_mnt_sb_offset, "mount.mnt_sb");
        } else if (vfsmount_mnt_sb_offset != -1) {
            mnt_sb = read_pointer(mnt->addr + mount_mnt_offset + vfsmount_mnt_sb_offset, "vfsmount.mnt_sb");
        }
        if (mnt_sb == sb_addr) {
            return mnt->addr + mount_mnt_offset;
        }
    }
    return 0;
}

/**
 * Convert a filesystem path to its corresponding dentry address
 * @param orig_path: The absolute path to resolve (e.g., "/var/log/messages")
 * @return: The dentry address if found, 0 if not found or error occurred
 */
ulong ParserPlugin::path_to_dentry(const std::string& orig_path) {
    // Input validation: ensure path is absolute (starts with '/')
    if (orig_path.empty() || orig_path[0] != '/') {
        LOGE("Invalid path: %s\n", orig_path.c_str());
        return 0;
    }
    std::string input_path = orig_path;
    // Normalize the path (remove duplicate slashes, etc.)
    normalize_path(input_path);
    // Get all mount points in the system using init process context (PID 1)
    struct task_context *tc;
    ulong pid = 0;
	while ((tc = pid_to_context(pid)) == NULL){
        pid++;
    }
    std::vector<std::shared_ptr<mount_point>> mount_points = get_mntpoint_list(tc);
    // Working copy of the path for mount point matching
    std::shared_ptr<mount_point> matched_mount = nullptr;
    // Find matching mount point using longest prefix matching algorithm
    while (true) {
        // Try to find a mount point that matches the current input_path
        for (const auto& mnt_ptr : mount_points) {
            // Check if input_path starts with mount point path
            if (input_path == mnt_ptr->path) {
                matched_mount = mnt_ptr;
            }
        }
        // If we found a matching mount point, stop searching
        if (matched_mount) {
            break;
        }
        // No match found, try parent directory by truncating the path
        size_t slash_pos = input_path.find_last_of('/');
        if (slash_pos != 0) {
            // Normal case: /a/b/c -> /a/b
            input_path = input_path.substr(0, slash_pos);
        } else if (slash_pos == 0 && input_path.length() > 1) {
            // Root directory case: /a -> /
            input_path = "/";
        } else {
            // Already at root "/", cannot truncate further
            break;
        }
        LOGD("Trying parent path: '%s'\n", input_path.c_str());
    }
    // If no mount point was found, path resolution failed
    if (!matched_mount) {
        return 0;
    }
    if (!is_kvaddr(matched_mount->root_dentry)) {
        LOGE("Invalid root dentry: 0x%lx\n", matched_mount->root_dentry);
        return 0;
    }
    LOGD("Found mount point: %s\n", matched_mount->path.c_str());
    LOGD("Root dentry: 0x%lx\n", matched_mount->root_dentry);
    // Calculate the remaining path after the mount point
    std::string remaining_path;
    if (input_path.length() < orig_path.length()) {
        remaining_path = orig_path.substr(input_path.length());
         // Remove leading slash from remaining path
        if (!remaining_path.empty() && remaining_path[0] == '/') {
            remaining_path = remaining_path.substr(1);
        }
    }
    // If there's a remaining path, traverse it component by component
    if (!remaining_path.empty()) {
        LOGD("Remaining path: %s\n", remaining_path.c_str());
        // Start from the mount point's root dentry
        ulong current_dentry = matched_mount->root_dentry;
        size_t start = 0;
        // Parse and traverse each path component
        while (start < remaining_path.length()) {
            size_t slash_pos = remaining_path.find('/', start);
            std::string dir_name;
            if (slash_pos == std::string::npos) {
                // Last path component
                dir_name = remaining_path.substr(start);
                start = remaining_path.length();
            } else {
                // Intermediate path component
                dir_name = remaining_path.substr(start, slash_pos - start);
                start = slash_pos + 1;
            }
            // Skip empty components (caused by double slashes)
            if (!dir_name.empty()) {
                LOGD("Looking for component: '%s'\n", dir_name.c_str());
                // Find the directory entry in the current directory
                current_dentry = find_file_in_dir(current_dentry, dir_name);
                if (current_dentry == 0) {
                    // Component not found, path resolution failed
                    return 0;
                }
            }
        }
        // Return the final dentry found after traversing all components
        return current_dentry;
    }else{
        // No remaining path, return the mount point's root dentry
        return matched_mount->root_dentry;
    }
}

/**
 * Normalize a filesystem path by removing redundant slashes and trailing slashes
 *
 * This function cleans up a filesystem path string by removing duplicate consecutive
 * slashes and trailing slashes (except for the root directory). It performs in-place
 * modification of the input string for efficiency.
 *
 * The function performs the following normalization operations:
 * 1. Removes consecutive duplicate slashes (e.g., "///" becomes "/")
 * 2. Removes trailing slashes except when the path is the root directory "/"
 * 3. Handles empty paths gracefully by returning immediately
 *
 * Examples of normalization:
 * - "/usr//bin/" -> "/usr/bin"
 * - "///home//user///" -> "/home/user"
 * - "/" -> "/" (root directory unchanged)
 * - "//" -> "/" (multiple root slashes become single)
 *
 * @param path Reference to the filesystem path string to normalize (modified in-place)
 *
 */
void ParserPlugin::normalize_path(std::string &path) {
    if (path.empty()) return;
    size_t write_pos = 0;
    size_t len = path.length();
    for (size_t i = 0; i < len; ++i) {
        if (path[i] == '/' && write_pos > 0 && path[write_pos - 1] == '/') {
            continue;
        }
        path[write_pos++] = path[i];
    }
    if (write_pos > 1 && path[write_pos - 1] == '/') {
        --write_pos;
    }
    path.resize(write_pos);
}

/**
 * Find a specific file or directory entry within a parent directory
 *
 * This function searches through the subdirectories of a given dentry to find
 * a child entry with the specified name. It traverses the directory's child
 * list and compares each entry's name against the target name using exact
 * string matching.
 *
 * The function performs the following operations:
 * 1. Validates the parent dentry address as a kernel virtual address
 * 2. Checks that the target name is not empty
 * 3. Retrieves all subdirectory entries from the parent dentry
 * 4. Iterates through each child dentry and compares names
 * 5. Returns the first matching dentry address found
 *
 * @param dentry The kernel virtual address of the parent dentry structure
 * @param name The filename or directory name to search for (case-sensitive)
 * @return The kernel virtual address of the matching child dentry, or 0 if not found
 *
 */
ulong ParserPlugin::find_file_in_dir(ulong dentry, const std::string& name) {
    if (!is_kvaddr(dentry) || name.empty()) {
        return 0;
    }
    std::vector<ulong> subdirs_list = for_each_subdirs(dentry);
    if (subdirs_list.empty()) {
        return 0;
    }
    for (const auto& dentry_addr : subdirs_list) {
        std::string dentry_name = get_dentry_name(dentry_addr);
        if (dentry_name == name) {
            return dentry_addr;
        }
    }
    return 0;
}

/**
 * Get the inode address associated with a dentry structure
 *
 * This function retrieves the inode address from a dentry (directory entry)
 * structure in the Linux kernel's VFS (Virtual File System) layer. The inode
 * contains metadata about a file or directory, such as permissions, timestamps,
 * size, and data block locations.
 *
 * The function performs the following operations:
 * 1. Validates the dentry address as a kernel virtual address
 * 2. Reads the d_inode field from the dentry structure
 * 3. Returns the inode address for further processing
 *
 * @param dentry The kernel virtual address of the dentry structure
 * @return The kernel virtual address of the associated inode structure, or 0 if invalid
 *
 */
ulong ParserPlugin::get_inode(ulong dentry){
    if (is_kvaddr(dentry)){
        return read_pointer(dentry + field_offset(dentry,d_inode),"d_inode addr");
    }
    return 0;
}

/**
 * Get the name of a dentry structure
 *
 * This function extracts the name string from a dentry (directory entry) structure
 * in the Linux kernel's VFS (Virtual File System) layer. It handles both inline
 * names (stored in d_iname) and external names (pointed to by d_name.name) based
 * on the kernel's optimization for short filenames.
 *
 * The function performs the following operations:
 * 1. Validates the dentry address as a kernel virtual address
 * 2. Caches field offsets to avoid repeated lookups for performance
 * 3. Reads the entire dentry structure into a buffer
 * 4. Extracts the name pointer and length from the qstr structure
 * 5. Determines if the name is stored inline (d_iname) or externally
 * 6. Reads the appropriate name string based on storage location
 *
 * @param dentry The kernel virtual address of the dentry structure
 * @return The filename/directory name as a string, or "(unknown)" if reading failed
 *
 */
std::string ParserPlugin::get_dentry_name(ulong dentry){
    if (!is_kvaddr(dentry)) {
        return "(unknown)";
    }
    // Cache field offsets to avoid repeated lookups
    int d_name_offset = field_offset(dentry, d_name);
    int qstr_name_offset = field_offset(qstr, name);
    int qstr_len_offset = field_offset(qstr, len);
    int d_iname_offset = field_offset(dentry, d_iname);
    void *dentry_buf = read_struct(dentry, "dentry");
    if (!dentry_buf) {
        return "(unknown)";
    }
    ulong d_name_name = ULONG(dentry_buf + d_name_offset + qstr_name_offset);
    uint d_name_len = UINT(dentry_buf + d_name_offset + qstr_len_offset);
    ulong d_iname = dentry + d_iname_offset;
    std::string name;
    if (d_name_name == d_iname) {
        name = read_cstring(d_iname, NAME_MAX, "d_iname");
    } else if (is_kvaddr(d_name_name)) {
        name = read_cstring(d_name_name, d_name_len + 1, "d_name.name");
    } else {
        name = "(unknown)";
    }
    FREEBUF(dentry_buf);
    return name;
}

/**
 * Get the full filesystem path for a given dentry structure
 *
 * This function retrieves the complete filesystem path for a dentry by using
 * the crash utility's get_pathname() function. It first finds the appropriate
 * vfsmount structure associated with the dentry's superblock, then constructs
 * the full path from the root of that mount point.
 *
 * The function performs the following operations:
 * 1. Finds the vfsmount structure that corresponds to the dentry's superblock
 * 2. Uses get_pathname() to construct the full path from the mount root
 * 3. Returns the complete filesystem path as a string
 *
 * @param dentry The kernel virtual address of the dentry structure
 * @return The complete filesystem path as a string, or empty string if resolution fails
 *
 */
std::string ParserPlugin::get_dentry_path(ulong dentry){
    if (!is_kvaddr(dentry)) {
        return std::string();
    }
    char buf[BUFSIZE];
    ulong vfsmount_addr = find_vfsmount_by_superblock(dentry);
    if (!vfsmount_addr) {
        return std::string();
    }
    get_pathname(dentry, buf, BUFSIZE, 1, vfsmount_addr);
    return std::string(buf);
}

#if defined(ARM)
ulong* ParserPlugin::pmd_page_addr(ulong pmd){
    ulong ptr;
    if (machdep->flags & PGTABLE_V2) {
        ptr = PAGEBASE(pmd);
    } else {
        ptr = pmd & ~(PTRS_PER_PTE * sizeof(void *) - 1);
        ptr += PTRS_PER_PTE * sizeof(void *);
    }
    return (ulong *)ptr;
}

ulong ParserPlugin::get_arm_pte(ulong task_addr, ulong page_vaddr){
    char buf[BUFSIZE];
    ulong *pgd;
    ulong *page_dir;
    ulong *page_middle;
    ulong *page_table;
    ulong pgd_pte;
    ulong pmd_pte;
    ulong pte;
    struct task_context *tc = task_to_context(task_addr);
    #define PGDIR_SIZE() (4 * PAGESIZE())
    #define PGDIR_OFFSET(X) (((ulong)(X)) & (PGDIR_SIZE() - 1))
    /*
     * Before idmap_pgd was introduced with upstream commit 2c8951ab0c
     * (ARM: idmap: use idmap_pgd when setting up mm for reboot), the
     * panic task pgd was overwritten by soft reboot code, so we can't do
     * any vtop translations.
     */
    if (!(machdep->flags & IDMAP_PGD) && tc->task == tt->panic_task){
        LOGE("panic task pgd is trashed by soft reboot code\n");
    }
    if (is_kernel_thread(tc->task) && IS_KVADDR(page_vaddr)) {
        ulong active_mm = read_structure_field(tc->task,"task_struct","active_mm");
        if (!active_mm){
            LOGE("no active_mm for this kernel thread\n");
        }
        pgd = ULONG_PTR(read_structure_field(active_mm,"mm_struct","pgd"));
    } else {
        ulong mm = task_mm(tc->task, TRUE);
        if (mm){
            pgd = ULONG_PTR(tt->mm_struct + field_offset(mm_struct, pgd));
        }else{
            pgd = ULONG_PTR(read_structure_field(tc->mm_struct,"mm_struct","pgd"));
        }
    }
    LOGD("PAGE DIRECTORY: %lx\n", (ulong)pgd);
    /*
     * pgd_offset(pgd, vaddr)
     */
    page_dir = pgd + PGD_OFFSET(page_vaddr) * 2;
    /* The unity-mapped region is mapped using 1MB pages,
     * hence 1-level translation if bit 20 is set; if we
     * are 1MB apart physically, we move the page_dir in
     * case bit 20 is set.
     */
    if (((page_vaddr) >> (20)) & 1){
        page_dir = page_dir + 1;
    }
    cfill_pgd(PAGEBASE(pgd), KVADDR, PGDIR_SIZE());
    pgd_pte = ULONG(machdep->pgd + PGDIR_OFFSET(page_dir));
    LOGD("  PGD: %s => %lx\n",mkstring(buf, VADDR_PRLEN, RJUST | LONG_HEX,MKSTR((ulong)page_dir)), pgd_pte);
    if (!pgd_pte){
        return 0;
    }
    /*
     * pmd_offset(pgd, vaddr)
     *
     * Here PMD is folded into a PGD.
     */
    pmd_pte = pgd_pte;
    page_middle = page_dir;
    LOGD("  PMD: %s => %lx\n",mkstring(buf, VADDR_PRLEN, RJUST | LONG_HEX,MKSTR((ulong)page_middle)), pmd_pte);
    /*
     * pte_offset_map(pmd, vaddr)
     */
    page_table = pmd_page_addr(pmd_pte) + PTE_OFFSET(page_vaddr);
    cfill_ptbl(PAGEBASE(page_table), PHYSADDR, PAGESIZE());
    pte = ULONG(machdep->ptbl + PAGEOFFSET(page_table));
    LOGD("  PTE: %s => %lx\n\n",mkstring(buf, VADDR_PRLEN, RJUST | LONG_HEX,MKSTR((ulong)page_table)), pte);
    return pte;
}
#endif

bool ParserPlugin::load_symbols(std::string& path, std::string name){
    if (is_directory(TO_CONST_STRING(path.c_str()))){
        char * buf = search_directory_tree(TO_CONST_STRING(path.c_str()), TO_CONST_STRING(name.c_str()), 1);
        if (buf){
            std::string retbuf(buf);
            if (is_elf_file(TO_CONST_STRING(retbuf.c_str())) && add_symbol_file(retbuf)){
                LOGD("Add symbol:%s succ \n",retbuf.c_str());
                path = retbuf;
                return true;
            }
        }
    }else if (file_exists(TO_CONST_STRING(path.c_str()), NULL) && is_elf_file(TO_CONST_STRING(path.c_str()))){
        if (add_symbol_file(path)){
            LOGD("Add symbol:%s succ \n",path.c_str());
            return true;
        }
    }
    return false;
}

void ParserPlugin::uwind_irq_back_trace(int cpu, ulong x30){
#if defined(ARM64)
    ulong *cpus = get_cpumask_buf();
    if (NUM_IN_BITMAP(cpus, cpu)) {
        if (hide_offline_cpu(cpu)) {
            LOGE("cpu:%d is OFFLINE \n", cpu);
            FREEBUF(cpus);
            return;
        }
    }
    struct bt_info bt_setup, *bt;
    struct stack_hook hook;
    BZERO(&hook, sizeof(struct stack_hook));
    bt = &bt_setup;
    BZERO(bt, sizeof(struct bt_info));
    struct task_context *tc = task_to_context(tt->active_set[cpu]);
    clone_bt_info(&bt_setup, bt, tc);
    bt->hp = &hook;

    char arg_buf[BUFSIZE];
    BZERO(arg_buf, BUFSIZE);
    snprintf(arg_buf, BUFSIZE, "%d", cpu);
    make_cpumask(arg_buf, cpus, RETURN_ON_ERROR /* FAULT_ON_ERROR */, NULL);
    bt->cpumask = cpus;
    hook.esp = x30;
    print_task_header(fp, tc, 0);
    back_trace(bt);
    // dump_bt_info(bt, "back_trace");
    FREEBUF(cpus);
    PRINT("\n");
#endif
}

void ParserPlugin::uwind_task_back_trace(int pid, ulong x30){
#if defined(ARM64)
    struct task_context *tc = pid_to_context(pid);
    if(!tc){
        LOGE("No such pid:%d \n", pid);
        return;
    }
    if(strstr(tc->comm, "swapper") != NULL){
        LOGE("Do not support for swapper process\n");
        return;
    }
    struct bt_info bt_setup, *bt;
    struct stack_hook hook;
    BZERO(&hook, sizeof(struct stack_hook));

    bt = &bt_setup;
    BZERO(bt, sizeof(struct bt_info));
    clone_bt_info(&bt_setup, bt, tc);
    bt->hp = &hook;
    hook.esp = x30;
    back_trace(bt);
    // dump_bt_info(bt, "back_trace");
    PRINT("\n");
#endif
}

/**
 * Create directories recursively along a given path
 *
 * This function creates all necessary directories in a path hierarchy, similar
 * to the 'mkdir -p' command. It handles the recursive creation of parent
 * directories and validates the path structure before attempting creation.
 *
 * The function performs the following operations:
 * 1. Validates that the input path is not empty
 * 2. Checks if the path already exists and is a directory
 * 3. Recursively creates parent directories if they don't exist
 * 4. Creates the final directory with standard permissions (0755)
 *
 * @param path The filesystem path to create recursively
 * @return true if all directories were created successfully or already exist, false on failure
 *
 */
bool ParserPlugin::create_directories_recursive(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    struct stat st;
    if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
        return true;
    }
    size_t pos = path.find_last_of('/');
    if (pos != std::string::npos && pos > 0) {
        if (!create_directories_recursive(path.substr(0, pos))) {
            return false;
        }
    }
    mkdir(path.c_str(), 0755);
    return true;
}

void ParserPlugin::write_pagecache_to_file(ulong inode_addr, const std::string& filename, const std::string& dst_dir, bool show_log) {
    // Early validation
    if (!is_kvaddr(inode_addr)) {
        LOGE("Error: Invalid inode address (inode: %#lx)\n", inode_addr);
        return;
    }
    // Read inode structure
    void* inode_buf = read_struct(inode_addr, "inode");
    if (!inode_buf) {
        LOGE("Error: Failed to read inode structure at address %#lx\n", inode_addr);
        return;
    }
    ulong i_mapping = ULONG(inode_buf + field_offset(inode, i_mapping));
    ulonglong i_size = ULONGLONG(inode_buf + field_offset(inode, i_size));
    FREEBUF(inode_buf);
    if (!is_kvaddr(i_mapping)) {
        LOGE("Error: Invalid i_mapping address %#lx\n", i_mapping);
        return;
    }
    // Early exit for empty files
    if (i_size == 0) {
        LOGE("File %s is empty, skipping\n", filename.c_str());
        return;
    }
    if (dst_dir.empty() || filename.empty()) {
        return;
    }
    // Create output directory if it doesn't exist
    if (!create_directories_recursive(dst_dir)) {
        LOGE("Error: Failed to create directory %s: %s\n", dst_dir.c_str(), strerror(errno));
        return;
    }
    // Get all pages at once
    std::vector<ulong> pages = for_each_address_space(i_mapping);
    if (pages.empty()) {
        LOGE("No pages found in mapping\n");
        return;
    }
    // Open output file
    const std::string log_path = dst_dir + "/" + filename;
    FILE* logfile = fopen(log_path.c_str(), "wb");
    if (!logfile) {
        LOGE("Can't open %s\n", log_path.c_str());
        return;
    }
    // Pre-allocate file to avoid fragmentation
    if (ftruncate(fileno(logfile), i_size) != 0) {
        LOGE("Warning: Failed to pre-allocate file: %s\n", strerror(errno));
    }
    ulong pages_written = 0;
    ulong pages_excluded = 0;
    const size_t total_pages = pages.size();
    const ulonglong max_page_index = (i_size + page_size - 1) / page_size;
    // Process pages in order for better I/O performance
    std::sort(pages.begin(), pages.end(), [this](ulong a, ulong b) {
        ulong index_a = read_ulong(a + field_offset(page, index), "page.index");
        ulong index_b = read_ulong(b + field_offset(page, index), "page.index");
        return index_a < index_b;
    });
    for (const ulong page_addr : pages) {
        if (!is_kvaddr(page_addr)) {
            pages_excluded++;
            continue;
        }
        const ulong page_index = read_ulong(page_addr + field_offset(page, index), "page.index");
        // Skip pages beyond file size
        if (page_index >= max_page_index) {
            pages_excluded++;
            continue;
        }
        const ulonglong file_pos = page_index * page_size;
        const physaddr_t phys_addr = page_to_phy(page_addr);
        if (phys_addr == 0) {
            pages_excluded++;
            continue;
        }
        void* page_buf = read_memory(phys_addr, page_size, "page content", false);
        if (!page_buf) {
            pages_excluded++;
            continue;
        }
        // Calculate write size
        const size_t write_size = (file_pos + page_size > i_size) ?
                                  (i_size - file_pos) : page_size;
        // Write page data
        if (fseek(logfile, file_pos, SEEK_SET) == 0 &&
            fwrite(page_buf, 1, write_size, logfile) == write_size) {
            pages_written++;
        } else {
            pages_excluded++;
        }
        FREEBUF(page_buf);
    }
    fclose(logfile);
    // Report results
    if (show_log) {
        PRINT("Save %s to %s\n", filename.c_str(), log_path.c_str());
        PRINT("  - File size: %llu bytes\n", i_size);
        PRINT("  - Pages processed: %zu\n", total_pages);
        PRINT("  - Pages written: %lu\n", pages_written);
        PRINT("  - Pages excluded: %lu\n", pages_excluded);
        PRINT("  - Success rate: %.1f%%\n\n",
                total_pages > 0 ? (pages_written * 100.0 / total_pages) : 0.0);
    }
}

cmd_func_t ParserPlugin::get_wrapper_func() {
    return nullptr;
}

/* ============================================================================
 * Page Owner Interface Functions
 * ============================================================================ */

/**
 * Check if page_owner is enabled in the kernel
 *
 * This function verifies that the page_owner feature is enabled in the kernel
 * by checking configuration settings and runtime initialization status.
 *
 * @return true if page_owner is enabled and initialized, false otherwise
 */
bool ParserPlugin::is_enable_pageowner() {
    // Check if CONFIG_PAGE_OWNER is enabled in kernel configuration
    if (get_config_val("CONFIG_PAGE_OWNER") != "y") {
        LOGE("page_owner is disabled in kernel configuration\n");
        return false;
    }

    // Check if page_owner_inited symbol exists
    if (!csymbol_exists("page_owner_inited")) {
        LOGE("page_owner_inited symbol not found\n");
        return false;
    }

    // Check if page_owner is initialized at runtime
    int inited;
    try_get_symbol_data(TO_CONST_STRING("page_owner_inited"), sizeof(int), &inited);
    if (inited != 1) {
        LOGE("page_owner is not initialized, check page_owner=on in cmdline\n");
        return false;
    }
    // Determine page_ext_size based on kernel version
    // Kernel 5.4+ uses page_ext_size symbol directly
    if (csymbol_exists("page_ext_size")) {
        try_get_symbol_data(TO_CONST_STRING("page_ext_size"), sizeof(ulong), &page_ext_size);
        LOGD("Found page_ext_size symbol: %d", page_ext_size);
    } else if (csymbol_exists("extra_mem") && struct_size(page_ext)) {
        // Older kernels: calculate from struct size + extra_mem
        ulong extra_mem;
        if (try_get_symbol_data(TO_CONST_STRING("extra_mem"), sizeof(ulong), &extra_mem)){
            page_ext_size = struct_size(page_ext) + extra_mem;
            LOGD("Calculated page_ext_size from extra_mem: %d", page_ext_size);
        }
    }
    if (page_ext_size <= 0){
        LOGE("Cannot determine page_ext_size value");
        return false;
    }
    if (page_ext_ops_offset <= 0){
        LOGE("Cannot determine page_ext ops_offset value");
        return false;
    }
    if (!stack_slabs){
        LOGE("stack_slabs not available");
        return false;
    }
    LOGD("page_ext_size: %d bytes", page_ext_size);
    return true;
}

/**
 * Parse page owner information by page structure address
 *
 * This function extracts page owner information for a given page structure address.
 * It validates the page address, looks up the page_ext structure, and parses the
 * page_owner data if available.
 *
 * @param page_addr The kernel virtual address of the page structure
 * @return Shared pointer to page_owner structure, or nullptr if parsing fails
 */
std::shared_ptr<page_owner> ParserPlugin::parse_page_owner_by_page(ulong page_addr) {
    if (!is_kvaddr(page_addr)) {
        LOGD("Invalid page address: 0x%lx\n", page_addr);
        return nullptr;
    }

    // Convert page address to PFN
    ulong pfn = page_to_pfn(page_addr);
    if (pfn == 0) {
        LOGE("Failed to convert page address to PFN\n");
        return nullptr;
    }

    return parse_page_owner_by_pfn(pfn);
}

/**
 * Parse page owner information by page frame number (PFN)
 *
 * This function extracts page owner information for a given PFN by looking up
 * the corresponding page_ext structure and parsing the page_owner data.
 *
 * @param pfn The page frame number to analyze
 * @return Shared pointer to page_owner structure, or nullptr if parsing fails
 */
std::shared_ptr<page_owner> ParserPlugin::parse_page_owner_by_pfn(ulong pfn) {
    // Validate PFN range
    if (pfn < min_low_pfn || pfn > max_pfn) {
        LOGE("Invalid PFN: 0x%lx (valid range: 0x%lx - 0x%lx)\n", pfn, min_low_pfn, max_pfn);
        return nullptr;
    }

    // Convert PFN to page structure address
    ulong page = pfn_to_page(pfn);
    if (!is_kvaddr(page)) {
        LOGD("Invalid page address: 0x%lx\n", page);
        return nullptr;
    }

    // Lookup page_ext structure
    ulong page_ext = lookup_page_ext(page);
    if (!is_kvaddr(page_ext)) {
        LOGD("Cannot find page_ext for PFN: 0x%lx\n", pfn);
        return nullptr;
    }

    // Check if page owner is enabled for this page
    ulong page_ext_flags = read_ulong(page_ext + field_offset(page_ext, flags), "page_ext_flags");
    if (!(page_ext_flags & (1UL << PAGE_EXT_OWNER))) {
        LOGD("Page owner not enabled for PFN: 0x%lx\n", pfn);
        return nullptr;
    }

    // Parse page_owner structure
    auto owner_ptr = std::make_shared<page_owner>();
    owner_ptr->page_ext = page_ext;
    owner_ptr->addr = page_ext + page_ext_ops_offset;
    owner_ptr->pfn = pfn;

    // Read page_owner fields
    void *page_owner = read_struct(owner_ptr->addr, "page_owner");
    if (page_owner == nullptr) {
        LOGE("Cannot parse page owner for PFN: 0x%lx\n", pfn);
        return nullptr;
    }
    owner_ptr->order = SHORT(page_owner + field_offset(page_owner, order));
    owner_ptr->handle = UINT(page_owner + field_offset(page_owner, handle));
    owner_ptr->free_handle = UINT(page_owner + field_offset(page_owner, free_handle));
    owner_ptr->last_migrate_reason = SHORT(page_owner + field_offset(page_owner, last_migrate_reason));
    owner_ptr->ts_nsec = ULONG(page_owner + field_offset(page_owner, ts_nsec));
    owner_ptr->free_ts_nsec = ULONG(page_owner + field_offset(page_owner, free_ts_nsec));
    owner_ptr->gfp_mask = INT(page_owner + field_offset(page_owner, gfp_mask));
    owner_ptr->pid = INT(page_owner + field_offset(page_owner, pid));

    // Handle optional fields
    const auto comm_offset = field_offset(page_owner, comm);
    if (comm_offset > 0) {
        owner_ptr->comm = read_cstring(owner_ptr->addr + comm_offset, 64, "page_owner_comm");
    }

    const auto tgid_offset = field_offset(page_owner, tgid);
    if (tgid_offset > 0) {
        owner_ptr->tgid = INT(page_owner + tgid_offset);
    } else {
        struct task_context *tc = pid_to_context(owner_ptr->pid);
        if (tc) {
            owner_ptr->tgid = task_tgid(tc->task);
            if (comm_offset <= 0) {
                owner_ptr->comm = std::string(tc->comm);
            }
        }
    }

    FREEBUF(page_owner);
    uint handle = is_page_allocated(owner_ptr) ? owner_ptr->handle : owner_ptr->free_handle;
    owner_ptr->stack_ptr = get_stack_record(handle);
    return owner_ptr;
}

/**
 * Parse page owner information by physical address
 *
 * This function extracts page owner information for a given physical address
 * by converting it to a PFN and then parsing the page_owner data.
 *
 * @param phys_addr The physical address to analyze
 * @return Shared pointer to page_owner structure, or nullptr if parsing fails
 */
std::shared_ptr<page_owner> ParserPlugin::parse_page_owner_by_phys(ulong phys_addr) {
    if (phys_addr == 0) {
        LOGE("Invalid physical address: 0x%lx\n", phys_addr);
        return nullptr;
    }

    // Convert physical address to PFN
    ulong pfn = phy_to_pfn(phys_addr);
    return parse_page_owner_by_pfn(pfn);
}

/**
 * Parse page owner information by virtual address
 *
 * This function extracts page owner information for a given virtual address
 * by translating it to a physical address, converting to PFN, and parsing
 * the page_owner data.
 *
 * @param virt_addr The virtual address to analyze
 * @return Shared pointer to page_owner structure, or nullptr if parsing fails
 */
std::shared_ptr<page_owner> ParserPlugin::parse_page_owner_by_vaddr(ulong virt_addr) {
    if (virt_addr == 0) {
        LOGE("Invalid virtual address: 0x%lx\n", virt_addr);
        return nullptr;
    }

    physaddr_t paddr = 0;

    // Try kernel virtual address translation first
    if (kvtop(NULL, virt_addr, &paddr, 0)) {
        return parse_page_owner_by_phys(paddr);
    }

    // Try current task context
    if (CURRENT_TASK() && uvtop(CURRENT_CONTEXT(), virt_addr, &paddr, 0)) {
        return parse_page_owner_by_phys(paddr);
    }

    // Search through all tasks
    struct task_context *tc;
    for (ulong i = 0; i < RUNNING_TASKS(); i++) {
        tc = FIRST_CONTEXT() + i;
        if (tc->task && uvtop(tc, virt_addr, &paddr, 0)) {
            LOGD("Found virtual address in task PID:%ld [%s]\n", task_to_pid(tc->task), tc->comm);
            return parse_page_owner_by_phys(paddr);
        }
    }

    LOGE("Cannot translate virtual address 0x%lx to physical address\n", virt_addr);
    return nullptr;
}

/**
 * Lookup page_ext structure for a given page
 *
 * This is a simplified version of the lookup_page_ext function from pageowner.cpp
 * that handles the basic page extension lookup functionality.
 *
 * @param page The page structure address
 * @return The page_ext structure address, or 0 if not found
 */
ulong ParserPlugin::lookup_page_ext(ulong page) {
    if(get_config_val("CONFIG_PAGE_EXTENSION") != "y"){
        LOGD("Not enable CONFIG_PAGE_EXTENSION \n");
        return 0;
    }
    const ulong pfn = page_to_pfn(page);
    ulong page_ext = 0;

    if(get_config_val("CONFIG_SPARSEMEM") == "y"){
        const ulong section_nr = pfn_to_section_nr(pfn);
        const ulong section = valid_section_nr(section_nr);
        if (!section || !is_kvaddr(section)){
            LOGD("invaild section %#lx \n",section);
            return 0;
        }
        page_ext = read_pointer(section + field_offset(mem_section,page_ext),"mem_section_page_ext");
        if (page_ext_invalid(page_ext)){
            LOGD("invaild page_ext %#lx \n",page_ext);
            return 0;
        }
    } else {
        const int nid = page_to_nid(page);
        const struct node_table *nt = &vt->node_table[nid];
        page_ext = read_pointer(nt->pgdat + field_offset(pglist_data,node_page_ext),"pglist_data_node_page_ext");
    }
    return get_entry(page_ext, pfn);
}

ulong ParserPlugin::get_entry(ulong base, ulong pfn) {
    LOGD("page_ext:%lx pfn:%lx\n", base,pfn);
#ifdef ARM64
    return base + page_ext_size * pfn;
#else
    ulong pfn_index = pfn - phy_to_pfn(machdep->machspec->phys_base);
    return base + page_ext_size * pfn_index;
#endif
}

bool ParserPlugin::page_ext_invalid(ulong page_ext){
    return !is_kvaddr(page_ext) || (((unsigned long)page_ext & PAGE_EXT_INVALID) == PAGE_EXT_INVALID);
}

bool ParserPlugin::is_page_allocated(std::shared_ptr<page_owner> owner_ptr) {
    if (is_kvaddr(owner_ptr->page_ext)) {
        ulong flags = read_ulong(owner_ptr->page_ext + field_offset(page_ext, flags), "page_ext_flags");
        return (flags & (1UL << PAGE_EXT_OWNER_ALLOCATED)) != 0;
    }
    if (owner_ptr->ts_nsec > 0 && owner_ptr->free_ts_nsec > 0) {
        return owner_ptr->ts_nsec > owner_ptr->free_ts_nsec;
    }
    return true;
}

/**
 * Convert a kernel address to its symbolic representation
 *
 * This function converts a kernel virtual address to a human-readable symbolic
 * representation by looking up the symbol in the kernel's symbol table. It
 * provides enhanced formatting and error handling compared to the basic symbol
 * lookup functions.
 *
 * The function performs the following operations:
 * 1. Validates the input address as a kernel virtual address
 * 2. Performs symbol table lookup using value_search()
 * 3. Formats the result based on whether a symbol was found:
 *    - If symbol found: "symbol_name+0xoffset" format
 *    - If no symbol found: "0xaddress" format with appropriate prefix
 * 4. Handles edge cases like zero offset and invalid addresses
 *
 * @param addr The kernel virtual address to resolve to a symbol
 * @return Formatted string containing symbol information or raw address
 *
 * Examples:
 * - to_symbol(0xffffffc008123456) -> "do_sys_open+0x123"
 * - to_symbol(0xffffffc008000000) -> "start_kernel+0x0"
 * - to_symbol(0x12345678) -> "0x12345678" (if not a valid kernel address)
 * - to_symbol(0xdeadbeef) -> "[unknown:0xdeadbeef]" (if symbol lookup fails)
 */
std::string ParserPlugin::to_symbol(ulong addr) {
    // Early validation: check for zero address
    if (addr == 0) {
        return "0x0";
    }

    // Validate kernel virtual address
    if (!is_kvaddr(addr)) {
        // Not a kernel virtual address, return raw address with prefix
        std::ostringstream oss;
        oss << "0x" << std::hex << addr;
        return oss.str();
    }

    // Perform symbol table lookup
    ulong offset = 0;
    struct syment *sp = value_search(addr, &offset);

    std::ostringstream oss;

    if (sp && sp->name) {
        // Symbol found: format as "symbol_name+0xoffset"
        oss << sp->name;

        if (offset > 0) {
            // Non-zero offset: add offset information
            oss << "+0x" << std::hex << offset;
        }
        // Zero offset case: just return symbol name without "+0x0"
    } else {
        // Symbol not found: format as "[unknown:0xaddress]"
        // This helps distinguish between raw addresses and failed lookups
        oss << "[unknown:0x" << std::hex << addr << "]";
    }

    return oss.str();
}

/**
 * Convert jiffies to milliseconds
 *
 * This function converts kernel jiffies (timer ticks) to milliseconds based on
 * the system's HZ value (timer frequency). It provides the same functionality
 * as the kernel's jiffies_to_msecs() function for userspace analysis tools.
 *
 * The conversion formula is: milliseconds = (jiffies * 1000) / HZ
 * where HZ is the kernel's timer frequency (typically 100, 250, 300, or 1000).
 *
 * @param jiffies The number of timer ticks to convert
 * @return The equivalent time in milliseconds
 *
 * Examples (assuming HZ=1000):
 * - jiffies_to_msecs(1000) -> 1000ms (1 second)
 * - jiffies_to_msecs(500) -> 500ms (0.5 seconds)
 * - jiffies_to_msecs(100) -> 100ms (0.1 seconds)
 */
unsigned long ParserPlugin::jiffies_to_msecs(unsigned long jiffies_val) {
    // Get the kernel's HZ value (timer frequency)
    static unsigned long hz_value = 0;

    // Cache HZ value on first call for performance
    if (hz_value == 0) {
        if (csymbol_exists("jiffies_64")) {
            // Try to get HZ from kernel configuration or symbols
            std::string hz_config = get_config_val("CONFIG_HZ");
            if (hz_config != "n" && !hz_config.empty()) {
                try {
                    hz_value = std::stoul(hz_config);
                } catch (const std::exception&) {
                    // Fall back to default if parsing fails
                    hz_value = 1000;  // Common default for modern kernels
                }
            } else {
                // Try to read HZ from kernel symbol if available
                if (csymbol_exists("HZ")) {
                    hz_value = read_ulong(csymbol_value("HZ"), "HZ");
                } else {
                    // Use common default value
                    hz_value = 1000;
                }
            }
        } else {
            // Fallback for systems without jiffies support
            hz_value = 1000;
        }

        LOGD("Using HZ value: %lu", hz_value);
    }

    // Perform the conversion: milliseconds = (jiffies * 1000) / HZ
    // Use 64-bit arithmetic to avoid overflow for large jiffies values
    return (unsigned long)((uint64_t)jiffies_val * 1000ULL / hz_value);
}

#pragma GCC diagnostic pop
