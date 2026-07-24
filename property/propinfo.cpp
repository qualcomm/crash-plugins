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

#include "propinfo.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

/**
 * cmd_main - Base command entry point (not used in PropInfo)
 *
 * This is a placeholder implementation as PropInfo is a base class.
 * Actual command handling is done in the derived Prop class.
 */
void PropInfo::cmd_main(void) {}

/**
 * get_prop - Retrieve the value of a specific Android system property
 * @name: Property name to query
 *
 * This function implements lazy loading of properties. If properties haven't
 * been loaded yet, it attempts to parse them from the crash dump using either
 * symbol-based parsing or VMA-based parsing as a fallback.
 *
 * Returns: Property value string if found, empty string otherwise
 */
std::string PropInfo::get_prop(std::string name){
    // Lazy initialization: load properties if not already loaded
    if (prop_map.size() == 0) {
        LOGD("Property map is empty, attempting to load properties");

        // First attempt: Parse using symbol information
        if (!parser_propertys_by_sym()){
            LOGE("Failed to parse properties using symbols, trying VMA method");
            // Fallback: Parse from init process VMA
            parser_prop_by_init_vma();
        }

        // Clean up task pointer after parsing
        if(task_ptr != nullptr){
            task_ptr.reset();
        }
        LOGD("Property map loaded with %zu entries", prop_map.size());
    }

    // Search for the requested property
    if (prop_map.find(name) != prop_map.end()) {
        LOGD("Property '%s' found with value: %s", name.c_str(), prop_map[name].c_str());
        return prop_map[name];
    }

    LOGD("Property '%s' not found in map", name.c_str());
    return "";
}

PropInfo::~PropInfo(){
    swap_ptr = nullptr;
}

PropInfo::PropInfo(std::shared_ptr<Swapinfo> swap) : swap_ptr(swap){}

void PropInfo::init_command(void) {}

/**
 * get_symbol_file - Retrieve the path of a loaded symbol file by name
 * @name: Name of the symbol file to search for
 *
 * This function searches through the symbol_list to find a symbol file
 * that has been successfully loaded and returns its full path.
 *
 * Returns: Full path to the symbol file if found, empty string otherwise
 */
std::string PropInfo::get_symbol_file(std::string name){
    LOGD("Searching for symbol file: %s", name.c_str());

    for (const auto& symbol : symbol_list) {
        if (symbol.name == name){
            LOGD("Found symbol file: %s at path: %s", name.c_str(), symbol.path.c_str());
            return symbol.path;
        }
    }

    LOGD("Symbol file '%s' not found in symbol list", name.c_str());
    return "";
}

/**
 * print_propertys - Display all Android system properties (base implementation)
 *
 * This function formats and prints all properties stored in prop_map.
 * Properties are displayed with index numbers and aligned for readability.
 * This is the base implementation used by PropInfo.
 */
void PropInfo::print_propertys(){
    // Calculate maximum property name length for alignment
    size_t max_len = 0;
    for (const auto& pair : prop_map) {
        max_len = std::max(max_len, pair.first.size());
    }
    // Format and print each property with index
    size_t index = 1;
    std::ostringstream oss;
    for (const auto& pair : prop_map) {
        oss << "[" << std::setw(4) << std::setfill('0') << index << "]"
            << std::left << std::setw(max_len) << std::setfill(' ') << pair.first << " "
            << std::left << pair.second << "\n";
        index++;
    }
    PRINT("%s \n", oss.str().c_str());

    LOGD("Successfully printed %zu properties", prop_map.size());
}

/**
 * init_offset - Initialize structure field offsets for property parsing
 *
 * This function initializes the offsets of various structure fields used in
 * Android property system. It first attempts to get offsets from debug symbols.
 * If symbols are not available, it falls back to hardcoded offsets based on
 * architecture (32-bit vs 64-bit).
 *
 * The offsets are critical for correctly parsing property data structures
 * from the crash dump memory.
 */
void PropInfo::init_offset(void) {
    // Attempt to initialize field offsets from debug symbols
    field_init(SystemProperties, contexts_);
    field_init(ContextsSerialized, context_nodes_);
    field_init(ContextsSerialized, num_context_nodes_);
    field_init(ContextsSerialized, serial_prop_area_);
    field_init(ContextNode, pa_);
    field_init(ContextNode, filename_);
    field_init(ContextNode, context_);
    field_init(prop_bt, prop);
    field_init(prop_area, data_);
    field_init(prop_info, name);
    struct_init(ContextNode);
    // Check if symbol-based initialization succeeded
    if (field_offset(prop_area, data_) == -1){
        LOGD("Symbol-based offset initialization failed, using hardcoded offsets");

        // Use hardcoded offsets based on architecture
        // These offsets are architecture-dependent (32-bit vs 64-bit)
        bool is_64bit = BITS64() && !is_compat;
        LOGD("Using %s architecture offsets", is_64bit ? "64-bit" : "32-bit");

        g_offset.SystemProperties_contexts_ = is_64bit ? 64 : 32;
        g_offset.ContextsSerialized_context_nodes_ = is_64bit ? 32 : 16;
        g_offset.ContextsSerialized_num_context_nodes_ = is_64bit ? 40 : 20;
        g_offset.ContextsSerialized_serial_prop_area_ = is_64bit ? 56 : 28;
        g_offset.ContextNode_pa_ = is_64bit ? 16 : 12;
        g_offset.ContextNode_filename_ = is_64bit ? 32 : 20;
        g_offset.ContextNode_context_ = 8;
        g_offset.prop_bt_prop = 4;
        g_offset.prop_area_data_ = 128;
        g_offset.prop_info_name = 96; // Same for both architectures
        g_size.ContextNode = is_64bit ? 40 : 24;
    } else {
        LOGD("Successfully initialized offsets from debug symbols");
        // Use offsets from debug symbols
        g_offset.SystemProperties_contexts_ = field_offset(SystemProperties, contexts_);
        g_offset.ContextsSerialized_context_nodes_ = field_offset(ContextsSerialized, context_nodes_);
        g_offset.ContextsSerialized_num_context_nodes_ = field_offset(ContextsSerialized, num_context_nodes_);
        g_offset.ContextsSerialized_serial_prop_area_ = field_offset(ContextsSerialized, serial_prop_area_);
        g_offset.ContextNode_pa_ = field_offset(ContextNode, pa_);
        g_offset.ContextNode_filename_ = field_offset(ContextNode, filename_);
        g_offset.ContextNode_context_ = field_offset(ContextNode, context_);
        g_offset.prop_bt_prop = field_offset(prop_bt, prop);
        g_offset.prop_area_data_ = field_offset(prop_area, data_);
        g_offset.prop_info_name = field_offset(prop_info, name);
        g_size.ContextNode = struct_size(ContextNode);
    }
}

/**
 * init_task - Initialize the init process task context
 *
 * This function locates the Android init process (PID 1) and creates a task
 * context for it. The init process is used to access property information
 * stored in its memory space.
 *
 * Returns: true if init task was initialized successfully, false otherwise
 */
bool PropInfo::init_task() {
    // Check if task is already initialized
    if (tc_init && task_ptr) {
        LOGD("Init task already initialized");
        return true;
    }
    LOGD("Initializing init process task context");
    // Try to find init process by name first
    tc_init = find_proc("init");
    if (!tc_init) {
        LOGD("Init process not found by name, trying PID 1");
        // Fallback: try to find by PID 1
        tc_init = find_proc(1);
    }
    // Verify init process was found
    if (!tc_init) {
        LOGE("Cannot find init process (PID 1)");
        return false;
    }
    LOGD("Found init process: PID=%ld, comm=%s", task_to_pid(tc_init->task), tc_init->comm);
    // Create task pointer if not already created
    if (task_ptr == nullptr) {
        LOGD("Creating UTask for init process");
        task_ptr = std::make_shared<UTask>(swap_ptr, tc_init->task);
    }
    return true;
}

/**
 * parser_propertys_by_sym - Parse Android properties using symbol information
 *
 * This function uses debug symbols from libc.so to locate and parse Android
 * system properties from the crash dump. It accesses the system_properties
 * global variable and traverses the property tree structure.
 *
 * This is the preferred method for parsing properties as it's more reliable
 * than VMA-based parsing.
 *
 * Returns: true if properties were parsed successfully, false otherwise
 */
bool PropInfo::parser_propertys_by_sym(){
    LOGD("Attempting to parse properties using symbol information");
    // Initialize init task context
    if (!init_task()) {
        LOGE("Failed to initialize init task");
        return false;
    }

    // Get libc.so symbol file path
    std::string symbol_file = get_symbol_file("libc.so");
    if (symbol_file.empty()){
        LOGE("libc.so symbol file not found");
        return false;
    }
    LOGD("Using symbol file: %s", symbol_file.c_str());
    // Initialize structure offsets
    init_offset();

    // Read pa_size_ variable from BSS section
    size_t pa_size_addr = task_ptr->get_var_addr_by_bss(symbol_file, "pa_size_");
    if (!is_uvaddr(pa_size_addr,tc_init)){
        LOGE("Invalid pa_size_ address: 0x%zx", pa_size_addr);
        return false;
    }
    pa_size = task_ptr->uread_ulong(pa_size_addr);
    LOGD("pa_size: 0x%zx -> %zu", pa_size_addr, pa_size);

    // Read pa_data_size_ variable from BSS section
    size_t pa_data_size_addr = task_ptr->get_var_addr_by_bss(symbol_file, "pa_data_size_");
    if (!is_uvaddr(pa_data_size_addr,tc_init)){
        LOGE("Invalid pa_data_size_ address: 0x%zx", pa_data_size_addr);
        return false;
    }
    pa_data_size = task_ptr->uread_ulong(pa_data_size_addr);
    LOGD("pa_data_size: 0x%zx -> %zu", pa_data_size_addr, pa_data_size);

    // Read system_properties global variable address
    size_t system_prop_addr = task_ptr->get_var_addr_by_bss(symbol_file, "system_properties");
    if (!is_uvaddr(system_prop_addr,tc_init)){
        LOGE("Invalid system_properties address: 0x%zx", system_prop_addr);
        return false;
    }
    LOGD("system_properties: 0x%zx", system_prop_addr);

    // Read contexts_ field from SystemProperties structure
    size_t contexts_addr = system_prop_addr + g_offset.SystemProperties_contexts_;
    LOGD("contexts field address: 0x%zx (offset: %d)", contexts_addr, g_offset.SystemProperties_contexts_);

    contexts_addr = task_ptr->uread_ulong(contexts_addr) & task_ptr->vaddr_mask;
    if (!is_uvaddr(contexts_addr,tc_init)){
        LOGE("Invalid ContextsSerialized address: 0x%zx", contexts_addr);
        return false;
    }
    LOGD("ContextsSerialized: 0x%zx", contexts_addr);

    // Read context node information
    size_t num_context_nodes = task_ptr->uread_ulong(contexts_addr + g_offset.ContextsSerialized_num_context_nodes_);
    size_t context_nodes_addr = task_ptr->uread_ulong(contexts_addr + g_offset.ContextsSerialized_context_nodes_) & task_ptr->vaddr_mask;
    size_t serial_prop_area_addr = task_ptr->uread_ulong(contexts_addr + g_offset.ContextsSerialized_serial_prop_area_) & task_ptr->vaddr_mask;

    if (!is_uvaddr(serial_prop_area_addr,tc_init)){
        LOGE("Invalid serial_prop_area address: 0x%zx", serial_prop_area_addr);
        return false;
    }
    LOGD("serial_prop_area: 0x%zx", serial_prop_area_addr);

    if (!is_uvaddr(context_nodes_addr,tc_init)){
        LOGE("Invalid context_nodes address: 0x%zx", context_nodes_addr);
        return false;
    }
    LOGD("context_nodes base: 0x%zx, count: %zu", context_nodes_addr, num_context_nodes);

    // Iterate through all context nodes and parse property areas
    size_t parsed_nodes = 0;
    for (size_t i = 0; i < num_context_nodes; i++){
        size_t node_addr = context_nodes_addr + i * g_size.ContextNode;
        if (!is_uvaddr(node_addr,tc_init)){
            LOGE("Invalid context node address at index %zu: 0x%zx", i, node_addr);
            continue;
        }
        // Read context node fields
        size_t prop_area_addr = task_ptr->uread_ulong(node_addr + g_offset.ContextNode_pa_) & task_ptr->vaddr_mask;
        size_t context_addr = task_ptr->uread_ulong(node_addr + g_offset.ContextNode_context_) & task_ptr->vaddr_mask;
        size_t filename_addr = task_ptr->uread_ulong(node_addr + g_offset.ContextNode_filename_) & task_ptr->vaddr_mask;
        std::string context = task_ptr->uread_cstring(context_addr, 100);
        std::string filename = task_ptr->uread_cstring(filename_addr,100);

        LOGD("Parsing context node %zu: context='%s', filename='%s', prop_area=0x%zx",
             i, context.c_str(), filename.c_str(), prop_area_addr);

        // Parse the property area for this context
        if (parser_prop_area(prop_area_addr) == false){
            LOGE("Failed to parse prop_area at 0x%zx for context '%s'", prop_area_addr, context.c_str());
            continue;
        }
        parsed_nodes++;
    }

    LOGD("Symbol-based parsing complete: parsed %zu/%zu context nodes, total properties: %zu",
         parsed_nodes, num_context_nodes, prop_map.size());
    return true;
}

/**
 * parser_prop_area - Parse a property area structure
 * @area_vaddr: Virtual address of the prop_area structure
 *
 * This function reads and validates a prop_area structure, then parses
 * the property binary tree contained within it. The prop_area contains
 * metadata and a data section with property information.
 *
 * Returns: true if the area was parsed successfully, false otherwise
 */
bool PropInfo::parser_prop_area(size_t area_vaddr){
    // Validate address
    if (!is_uvaddr(area_vaddr,tc_init)){
        LOGE("Invalid prop_area address: 0x%zx", area_vaddr);
        return false;
    }
    // Read prop_area structure from memory
    std::vector<char> prop_area_buf = task_ptr->read_data(area_vaddr,sizeof(prop_area));
    if(prop_area_buf.size() == 0){
        LOGE("Failed to read prop_area at: 0x%zx", area_vaddr);
        return false;
    }
    // Parse prop_area structure
    prop_area area = *reinterpret_cast<prop_area*>(prop_area_buf.data());

    // Verify magic number (0x504f5250 = "PROP" in ASCII)
    if (area.magic_ != 0x504f5250){
        LOGE("Invalid prop_area magic: 0x%x (expected 0x504f5250)", area.magic_);
        return false;
    }
    ulong data_addr = area_vaddr + g_offset.prop_area_data_;
    LOGD("prop_area at 0x%zx: magic=0x%x, version=0x%x, bytes_used=%u, serial=%u, data=0x%lx",
         area_vaddr, area.magic_, area.version_, area.bytes_used_, area.serial_, data_addr);

    // Parse property binary tree if data exists
    if (area.bytes_used_ > 0){
        LOGD("Parsing property binary tree at 0x%lx (bytes_used: %u)", data_addr, area.bytes_used_);
        parser_prop_bt(data_addr,data_addr);
    } else {
        LOGD("prop_area has no data (bytes_used=0)");
    }
    return true;
}

/**
 * parser_prop_bt - Parse a property binary tree node (recursive)
 * @root: Base address of the property data area
 * @prop_bt_addr: Address of the current prop_bt node
 *
 * This function recursively traverses the property binary tree structure.
 * Each node contains offsets to:
 * - prop: The property info structure
 * - left: Left child in the binary tree
 * - right: Right child in the binary tree
 * - children: Child nodes for hierarchical properties
 *
 * The tree is traversed in-order to parse all properties.
 */
void PropInfo::parser_prop_bt(size_t root, size_t prop_bt_addr){
    // Validate address
    if (!is_uvaddr(prop_bt_addr,tc_init)){
        LOGE("Invalid prop_bt address: 0x%zx", prop_bt_addr);
        return;
    }

    // Read prop_bt structure from memory
    std::vector<char> prop_bt_buf = task_ptr->read_data(prop_bt_addr,sizeof(prop_bt));
    if(prop_bt_buf.size() == 0){
        LOGE("Failed to read prop_bt at: 0x%zx", prop_bt_addr);
        return;
    }

    // Parse prop_bt structure
    prop_bt bt = *reinterpret_cast<prop_bt*>(prop_bt_buf.data());

    LOGD("prop_bt at 0x%zx: prop=%u, left=%u, right=%u, children=%u",
         prop_bt_addr, bt.prop, bt.left, bt.right, bt.children);

    // Parse property info if offset is valid
    if (bt.prop > 0 && bt.prop < pa_data_size) {
        LOGD("Parsing prop_info at offset %u (address: 0x%zx)", bt.prop, root + bt.prop);
        parser_prop_info(root + bt.prop);
    }

    // Recursively traverse left subtree
    if (bt.left > 0 && bt.left < pa_data_size) {
        LOGD("Traversing left child at offset %u", bt.left);
        parser_prop_bt(root, (root + bt.left));
    }

    // Recursively traverse right subtree
    if (bt.right > 0 && bt.right < pa_data_size) {
        LOGD("Traversing right child at offset %u", bt.right);
        parser_prop_bt(root, (root + bt.right));
    }

    // Recursively traverse children
    if (bt.children > 0 && bt.children < pa_data_size) {
        LOGD("Traversing children at offset %u", bt.children);
        parser_prop_bt(root, (root + bt.children));
    }
}

/**
 * parser_prop_info - Parse a property info structure and extract property name/value
 * @prop_info_addr: Virtual address of the prop_info structure
 *
 * This function reads a prop_info structure which contains the property's
 * serial number, value, and name. The parsed property is added to prop_map.
 */
void PropInfo::parser_prop_info(size_t prop_info_addr){
    // Validate address
    if (!is_uvaddr(prop_info_addr,tc_init)){
        LOGE("Invalid prop_info address: 0x%zx", prop_info_addr);
        return;
    }

    // Read prop_info structure from memory
    std::vector<char> prop_info_buf = task_ptr->read_data(prop_info_addr,sizeof(prop_info));
    if(prop_info_buf.size() == 0){
        LOGE("Failed to read prop_info at: 0x%zx", prop_info_addr);
        return;
    }

    // Parse prop_info structure
    prop_info info = *reinterpret_cast<prop_info*>(prop_info_buf.data());

    // Read property name (located after the prop_info structure)
    ulong name_addr = prop_info_addr + g_offset.prop_info_name;
    std::string name = task_ptr->uread_cstring(name_addr,100);

    if (!name.empty()){
        LOGD("Parsed prop_info at 0x%zx [%s] = %s",prop_info_addr, name.c_str(), info.value);
        // Add property to map
        prop_map[name] = info.value;
    } else {
        LOGD("Property at 0x%zx has empty name, skipping", prop_info_addr);
    }
}

/**
 * parser_prop_by_init_vma - Parse properties from init process VMA (fallback method)
 *
 * This function parses Android properties by scanning the Virtual Memory Areas
 * (VMAs) of the init process. It looks for memory-mapped property files
 * (identified by "u:object_r:" in their SELinux context) and parses the
 * property data directly from memory.
 *
 * This is a fallback method used when symbol-based parsing is not available.
 */
void PropInfo::parser_prop_by_init_vma(){
    // Second attempt: Parse from init process VMA (fallback method)
    LOGD("Attempting to parse properties from init VMA");
    // Initialize init task context
    if (!init_task()) {
        LOGE("Failed to initialize init task for VMA parsing");
        return;
    }
    // Initialize structure offsets
    init_offset();

    // Track processed property files to avoid duplicates
    std::set<std::string> prop_files;
    size_t vma_count = 0;
    size_t prop_file_count = 0;

    // Iterate through all file-backed VMAs in init process
    for(const auto& vma_ptr : task_ptr->for_each_file_vma()){
        vma_count++;

        // Filter for property files (identified by SELinux context)
        if (vma_ptr->name.find("u:object_r:") == std::string::npos) {
            continue;
        }

        // Skip duplicate property files
        if (prop_files.find(vma_ptr->name) != prop_files.end()) {
            LOGE("Skipping duplicate property file: %s", vma_ptr->name.c_str());
            continue;
        }

        prop_files.insert(vma_ptr->name);
        prop_file_count++;
        LOGD("Processing property file VMA: %s (size: %zu bytes)",
             vma_ptr->name.c_str(), vma_ptr->vm_size);

        // Read VMA data if not already loaded
        if (vma_ptr->vm_data.size() == 0){
            LOGD("Reading VMA data for: %s", vma_ptr->name.c_str());
            vma_ptr->vm_data = task_ptr->read_vma_data(vma_ptr);
        }

        // Verify VMA data was read successfully
        if (vma_ptr->vm_data.size() == 0){
            LOGE("Failed to read VMA data for: %s", vma_ptr->name.c_str());
            continue;
        }

        // Parse property binary tree from VMA data
        // The prop_bt structure starts after the prop_area header
        prop_bt pb = *reinterpret_cast<prop_bt*>(vma_ptr->vm_data.data() + sizeof(prop_area));

        if(pb.children != 0){
            LOGD("Parsing property tree with children offset: %u", pb.children);
            size_t props_before = prop_map.size();
            for_each_prop(pb.children, vma_ptr->vm_size, vma_ptr->vm_data.data());
            size_t props_added = prop_map.size() - props_before;
            LOGD("Parsed %zu properties from VMA: %s", props_added, vma_ptr->name.c_str());
        } else {
            LOGD("No children in property tree for: %s", vma_ptr->name.c_str());
        }
    }

    LOGD("VMA-based parsing complete: scanned %zu VMAs, processed %zu property files, total properties: %zu",
         vma_count, prop_file_count, prop_map.size());
}

/*
                                                  |<----------- prop3      ---------------------------------------------------------------------------------------------->|
                                                  |<----------- children2  ----------------------------------------------------->|                                        |
                                                  |<----------- children1  --------------->|                                     |                                        |
+-----------+-------+------+--------+-------------+-------+-----+----+-----+---------+-----+-------+-----+----+-----+---------+--+-------+-----+----+-----+---------+-----+------+---------------+-------+
|bytes_used_|serial_|magic_|version_|reserved_[28]|namelen|prop1|left|right|children1|     |namelen|prop2|left|right|children2|  |namelen|prop3|left|right|children3|.....|serial|value/long_prop|name[0]|
+-----------+-------+------+--------+-------------+-------+-----+----+-----+---------+-----+-------+-----+----+-----+---------+--+-------+-----+----+-----+---------+-----+------+---------------+-------+
|<-----------        prop_area        ----------->|<----------- prop_bt   ---------->|     |<----------- prop_bt   ---------->|  |<----------- prop_bt   ---------->|     |<-------   prop_info   ------>|
|<-----------------------------------------------------------------------------          init vma         ---------------------------------------------------------------------------------------------->|
*/
/**
 * for_each_prop - Recursively traverse property binary tree in VMA data
 * @prop_bt_off: Offset of the prop_bt node within the VMA
 * @vma_len: Total length of the VMA data
 * @vma_data: Pointer to the VMA data buffer
 *
 * This function recursively traverses the property binary tree structure
 * stored in a VMA. It extracts property names and values directly from
 * the memory buffer without needing to read from the crash dump.
 *
 * The tree structure is similar to parser_prop_bt but operates on a
 * contiguous memory buffer rather than reading from virtual addresses.
 *
 * Returns: true if traversal succeeded, false on error
 */
bool PropInfo::for_each_prop(uint32_t prop_bt_off, size_t vma_len, char* vma_data){
    // Validate that prop_bt structure fits within VMA bounds
    if(sizeof(prop_area) + prop_bt_off + sizeof(prop_bt) > vma_len) {
        LOGE("prop_bt offset %u exceeds VMA length %zu", prop_bt_off, vma_len);
        return false;
    }

    // Parse prop_bt structure from VMA data
    prop_bt pb = *reinterpret_cast<prop_bt*>(vma_data + sizeof(prop_area) + prop_bt_off);
    uint prop = pb.prop;
    uint left = pb.left;
    uint right = pb.right;
    uint children = pb.children;

    LOGD("Traversing prop_bt at offset %u: prop=%u, left=%u, right=%u, children=%u",
         prop_bt_off, prop, left, right, children);

    // Recursively traverse left subtree
    if (left != 0 && !for_each_prop(left, vma_len, vma_data)){
        LOGE("Failed to traverse left child at offset %u", left);
        return false;
    }

    // Parse property info if offset is valid
    if(prop != 0 && (sizeof(prop_area) + prop + 92/*PROP_VALUE_MAX*/) < vma_len){
        // Extract property value (92 bytes max, PROP_VALUE_MAX)
        std::string value = std::string(vma_data + sizeof(prop_area) + prop + 0x4/* serial*/, 92/*PROP_VALUE_MAX*/);

        /*
         * Extract property name
         * Note: PROP_NAME_MAX limit was removed in Android API level 26+
         * Property names can now be longer, but values are still limited to PROP_VALUE_MAX
         */
        std::string name = std::string(vma_data + sizeof(prop_area) + prop + sizeof(prop_info), 100);

        // Clean up strings (remove null terminators, whitespace, etc.)
        auto cleanedValue = cleanString(value);
        auto cleanedName = cleanString(name);

        // Add valid properties to map (skip empty or malformed entries)
        if (!cleanedValue.empty() && !cleanedName.empty()) {
            LOGD("Found property: [%s] = %s", cleanedName.c_str(), cleanedValue.c_str());
            prop_map[cleanedName] = cleanedValue;
        } else {
            LOGD("Skipping invalid property at offset %u (empty name or value)", prop);
        }
    }

    // Recursively traverse children
    if (children != 0 && !for_each_prop(children, vma_len, vma_data)){
        LOGE("Failed to traverse children at offset %u", children);
        return false;
    }

    // Recursively traverse right subtree
    if (right != 0 && !for_each_prop(right, vma_len, vma_data)){
        LOGE("Failed to traverse right child at offset %u", right);
        return false;
    }

    return true;
}

/**
 * cleanString - Clean and validate a property string
 * @str: Input string to clean
 *
 * This function performs several cleaning operations on property strings:
 * 1. Removes null terminators and everything after them
 * 2. Trims leading and trailing whitespace
 * 3. Validates that the string contains at least one non-whitespace character
 *
 * This is necessary because property data in memory may contain padding,
 * null terminators, or other artifacts that need to be removed.
 *
 * Returns: Cleaned string, or empty string if invalid
 */
std::string PropInfo::cleanString(const std::string& str) {
    std::string tempStr = str;

    // Remove null terminator and everything after it
    tempStr.erase(std::find(tempStr.begin(), tempStr.end(), '\0'), tempStr.end());

    // Trim leading whitespace
    tempStr.erase(0, tempStr.find_first_not_of(" \t\n\r\f\v"));

    // Trim trailing whitespace
    tempStr.erase(tempStr.find_last_not_of(" \t\n\r\f\v") + 1);

    // Validate that string contains at least one non-whitespace character
    bool hasValidChar = false;
    for (char c : tempStr) {
        if (c != '\0' && !std::isspace(c)) {
            hasValidChar = true;
            break;
        }
    }

    // Return empty string if no valid characters found
    if (!hasValidChar) {
        LOGD("String contains no valid characters after cleaning");
        return "";
    }

    return tempStr;
}

#pragma GCC diagnostic pop
