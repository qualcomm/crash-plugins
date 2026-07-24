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

#include "iommu.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(IOMMU)
#endif

/**
 * @brief Main command entry point for IOMMU analysis
 *
 */
void IOMMU::cmd_main(void) {
    // Validate minimum argument count
    if (argcnt < 2) {
        LOGD("Insufficient arguments provided, showing usage\n");
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }

    int argerrs = 0;
    int c;

    // Lazy initialization: parse IOMMU clients only when needed
    if (smmu_client_list.empty()) {
        parser_iommu_client();
    }

    // Process command-line options
    while ((c = getopt(argcnt, args, "dcgp:k")) != EOF) {
        switch(c) {
            case 'd':
                // List all IOMMU devices in the system
                print_iommu_devices();
                break;
            case 'g':
                // Display IOMMU groups with device associations
                print_iommu_groups();
                break;
            case 'c':
                // Display parsed IOMMU client information
                print_iommu_client();
                break;
            case 'p':
                // Dump page tables for specified domain
                if (optarg) {
                    dump_aarch64_page_tables(std::string(optarg));
                } else {
                    LOGE("Error: -p option requires a domain name argument\n");
                    argerrs++;
                }
                break;
            case 'k':
                // Display KGSL (Kernel Graphics Support Layer) information
                print_kgsl();
                break;
            default:
                LOGD("Unknown option: -%c\n", c);
                argerrs++;
                break;
        }
    }

    // Display usage if any errors occurred
    if (argerrs) {
        LOGE("Command line argument errors detected: %d\n", argerrs);
        cmd_usage(pc->curcmd, SYNOPSIS);
    }
}

/**
 * Helper function: Get device name from device address
 */
std::string IOMMU::get_device_name(ulong dev_addr) {
    if (!is_kvaddr(dev_addr)) {
        return "Unknown";
    }

    ulong name_addr = read_pointer(dev_addr + field_offset(device, kobj) + field_offset(kobject, name), "device name addr");
    if (is_kvaddr(name_addr)) {
        return read_cstring(name_addr, 100, "device name");
    }
    return "Unknown";
}

/**
 * Helper function: Get enum name from enum list
 */
std::string IOMMU::get_enum_name(const std::map<std::string, ulong>& enum_list, ulong value) {
    for (const auto& pair : enum_list) {
        if (pair.second == value) {
            return pair.first;
        }
    }
    return std::to_string(value);
}

/**
 * Helper function: Decode ARM SMMU v3 features bitmask to string
 */
std::string IOMMU::decode_smmu_v3_features(uint features) {
    std::vector<std::string> feature_list;

    if (features & ARM_SMMU_FEAT_2_LVL_STRTAB)  feature_list.push_back("2_LVL_STRTAB");
    if (features & ARM_SMMU_FEAT_2_LVL_CDTAB)   feature_list.push_back("2_LVL_CDTAB");
    if (features & ARM_SMMU_FEAT_TT_LE)         feature_list.push_back("TT_LE");
    if (features & ARM_SMMU_FEAT_TT_BE)         feature_list.push_back("TT_BE");
    if (features & ARM_SMMU_FEAT_PRI)           feature_list.push_back("PRI");
    if (features & ARM_SMMU_FEAT_ATS)           feature_list.push_back("ATS");
    if (features & ARM_SMMU_FEAT_SEV)           feature_list.push_back("SEV");
    if (features & ARM_SMMU_FEAT_MSI)           feature_list.push_back("MSI");
    if (features & ARM_SMMU_FEAT_COHERENCY)     feature_list.push_back("COHERENCY");
    if (features & ARM_SMMU_FEAT_TRANS_S1)      feature_list.push_back("TRANS_S1");
    if (features & ARM_SMMU_FEAT_TRANS_S2)      feature_list.push_back("TRANS_S2");
    if (features & ARM_SMMU_FEAT_STALLS)        feature_list.push_back("STALLS");
    if (features & ARM_SMMU_FEAT_HYP)           feature_list.push_back("HYP");
    if (features & ARM_SMMU_FEAT_STALL_FORCE)   feature_list.push_back("STALL_FORCE");
    if (features & ARM_SMMU_FEAT_VAX)           feature_list.push_back("VAX");
    if (features & ARM_SMMU_FEAT_RANGE_INV)     feature_list.push_back("RANGE_INV");
    if (features & ARM_SMMU_FEAT_BTM)           feature_list.push_back("BTM");
    if (features & ARM_SMMU_FEAT_SVA)           feature_list.push_back("SVA");
    if (features & ARM_SMMU_FEAT_E2H)           feature_list.push_back("E2H");
    if (features & ARM_SMMU_FEAT_NESTING)       feature_list.push_back("NESTING");

    if (feature_list.empty()) {
        return "NONE";
    }

    std::string result;
    for (size_t i = 0; i < feature_list.size(); ++i) {
        if (i > 0) result += " | ";
        result += feature_list[i];
    }
    return result;
}

/**
 * Helper function: Decode ARM SMMU options bitmask to string
 */
std::string IOMMU::decode_smmu_options(uint options) {
    std::vector<std::string> option_list;

    if (options & ARM_SMMU_OPT_SKIP_PREFETCH)   option_list.push_back("SKIP_PREFETCH");
    if (options & ARM_SMMU_OPT_PAGE0_REGS_ONLY) option_list.push_back("PAGE0_REGS_ONLY");
    if (options & ARM_SMMU_OPT_MSIPOLL)         option_list.push_back("MSIPOLL");
    if (options & ARM_SMMU_OPT_CMDQ_FORCE_SYNC) option_list.push_back("CMDQ_FORCE_SYNC");
    if (options & ARM_SMMU_OPT_VIRTIO)          option_list.push_back("VIRTIO");

    if (option_list.empty()) {
        return "NONE";
    }

    std::string result;
    for (size_t i = 0; i < option_list.size(); ++i) {
        if (i > 0) result += " | ";
        result += option_list[i];
    }
    return result;
}
/**
 * Helper function: Print ARM SMMU configuration
 */
void IOMMU::print_arm_smmu_cfg(ulong cfg_addr, const std::map<std::string, ulong>& cbar_enum_list,
                                const std::map<std::string, ulong>& fmt_enum_list) {
    if (!is_kvaddr(cfg_addr)) {
        return;
    }

    uint8_t cbndx = read_uint(cfg_addr + field_offset(arm_smmu_cfg, cbndx), "cbndx") & 0xFF;
    uint16_t vmid = read_uint(cfg_addr + field_offset(arm_smmu_cfg, vmid), "vmid") & 0xFFFF;
    uint32_t procid = read_uint(cfg_addr + field_offset(arm_smmu_cfg, procid), "procid");
    uint32_t cbar_val = read_uint(cfg_addr + field_offset(arm_smmu_cfg, cbar), "cbar");
    uint32_t fmt_val = read_uint(cfg_addr + field_offset(arm_smmu_cfg, fmt), "fmt");

    std::string cbar_str = get_enum_name(cbar_enum_list, cbar_val);
    std::string fmt_str = get_enum_name(fmt_enum_list, fmt_val);

    std::ostringstream cfg_oss;
    cfg_oss << "             arm_smmu_cfg         : " << std::hex << std::showbase << cfg_addr << "\n";
    cfg_oss << "               cbndx              : " << std::dec << (unsigned)cbndx << "\n";
    cfg_oss << "               vmid               : " << std::dec << vmid << "\n";
    cfg_oss << "               procid             : " << std::dec << procid << "\n";
    cfg_oss << "               cbar               : " << cbar_str << "\n";
    cfg_oss << "               fmt                : " << fmt_str << "\n";
    PRINT("%s", cfg_oss.str().c_str());
}

/**
 * Helper function: Print context bank information
 */
void IOMMU::print_context_bank(ulong cb_addr, uint index, const std::map<std::string, ulong>& cbar_enum_list,
                                const std::map<std::string, ulong>& fmt_enum_list) {
    // Read ttbr array (2 x u64)
    ulong ttbr0 = read_ulong(cb_addr + field_offset(arm_smmu_cb, ttbr) + 0 * sizeof(uint64_t), "ttbr0");
    ulong ttbr1 = read_ulong(cb_addr + field_offset(arm_smmu_cb, ttbr) + 1 * sizeof(uint64_t), "ttbr1");

    // Read tcr array (2 x u32)
    uint tcr0 = read_uint(cb_addr + field_offset(arm_smmu_cb, tcr) + 0 * sizeof(uint32_t), "tcr0");
    uint tcr1 = read_uint(cb_addr + field_offset(arm_smmu_cb, tcr) + 1 * sizeof(uint32_t), "tcr1");

    // Read mair array (2 x u32)
    uint mair0 = read_uint(cb_addr + field_offset(arm_smmu_cb, mair) + 0 * sizeof(uint32_t), "mair0");
    uint mair1 = read_uint(cb_addr + field_offset(arm_smmu_cb, mair) + 1 * sizeof(uint32_t), "mair1");

    // Read sctlr (u32)
    uint sctlr = read_uint(cb_addr + field_offset(arm_smmu_cb, sctlr), "sctlr");

    // Read cfg pointer
    ulong cfg_addr = read_pointer(cb_addr + field_offset(arm_smmu_cb, cfg), "cfg");

    // Print context bank info
    PRINT("       CB[%02u]:ttbr[0]=%#lx ttbr[1]=%#lx tcr[0]=%#x tcr[1]=%#x mair[0]=%#x mair[1]=%#x sctlr=%#x\n",
          index, ttbr0, ttbr1, tcr0, tcr1, mair0, mair1, sctlr);

    // Print cfg if valid
    print_arm_smmu_cfg(cfg_addr, cbar_enum_list, fmt_enum_list);
}

void IOMMU::print_arm_smmu_v2_device(std::shared_ptr<device> dev_ptr) {
    ulong arm_smmu_device_addr = dev_ptr->driver_data;
    if (!is_kvaddr(arm_smmu_device_addr)) {
        return;
    }
    // Read device pointer and get device name
    ulong dev_addr = read_pointer(arm_smmu_device_addr + field_offset(arm_smmu_device, dev), "dev");
    std::string device_name = get_device_name(dev_addr);

    // Format and print the information
    std::ostringstream oss;
    oss << "arm_smmu_device:" << std::hex << std::showbase << arm_smmu_device_addr
        << " [" << device_name << "]\n";

    // Read and print base if field exists
    if (field_offset(arm_smmu_device, base) != -1) {
        ulong base = read_pointer(arm_smmu_device_addr + field_offset(arm_smmu_device, base), "base");
        oss << "    base                 : " << std::hex << std::showbase << base << "\n";
    }

    // Read and print ioaddr if field exists
    if (field_offset(arm_smmu_device, ioaddr) != -1) {
        ulong ioaddr = read_ulong(arm_smmu_device_addr + field_offset(arm_smmu_device, ioaddr), "ioaddr");
        oss << "    ioaddr               : " << std::hex << std::showbase << ioaddr << "\n";
    }

    // Read and print numpage if field exists
    if (field_offset(arm_smmu_device, numpage) != -1) {
        uint numpage = read_uint(arm_smmu_device_addr + field_offset(arm_smmu_device, numpage), "numpage");
        oss << "    numpage              : " << std::dec << numpage << "\n";
    }

    // Read and print pgshift if field exists
    if (field_offset(arm_smmu_device, pgshift) != -1) {
        uint pgshift = read_uint(arm_smmu_device_addr + field_offset(arm_smmu_device, pgshift), "pgshift");
        oss << "    pgshift              : " << std::dec << pgshift << "\n";
    }

    // Read and print features if field exists
    if (field_offset(arm_smmu_device, features) != -1) {
        uint features = read_uint(arm_smmu_device_addr + field_offset(arm_smmu_device, features), "features");
        oss << "    features             : " << std::hex << std::showbase << features << "\n";
    }

    // Read and print options if field exists
    if (field_offset(arm_smmu_device, options) != -1) {
        uint options = read_uint(arm_smmu_device_addr + field_offset(arm_smmu_device, options), "options");
        oss << "    options              : " << std::hex << std::showbase << options << "\n";
    }

    // Read and print version if field exists
    if (field_offset(arm_smmu_device, version) != -1) {
        uint version = read_uint(arm_smmu_device_addr + field_offset(arm_smmu_device, version), "version");
        std::map<std::string, ulong> version_enum_list = read_enum_list("arm_smmu_arch_version");
        std::string version_str = get_enum_name(version_enum_list, version);
        oss << "    version              : " << version_str << "\n";
    }

    // Read and print model if field exists
    if (field_offset(arm_smmu_device, model) != -1) {
        uint model = read_uint(arm_smmu_device_addr + field_offset(arm_smmu_device, model), "model");
        std::map<std::string, ulong> model_enum_list = read_enum_list("arm_smmu_implementation");
        std::string model_str = get_enum_name(model_enum_list, model);
        oss << "    model                : " << model_str << "\n";
    }

    // Read num_context_banks (needed for later logic)
    uint num_context_banks = 0;
    if (field_offset(arm_smmu_device, num_context_banks) != -1) {
        num_context_banks = read_uint(arm_smmu_device_addr + field_offset(arm_smmu_device, num_context_banks), "num_context_banks");
        oss << "    num_context_banks    : " << std::dec << num_context_banks << "\n";
    }

    // Read and print num_s2_context_banks if field exists
    if (field_offset(arm_smmu_device, num_s2_context_banks) != -1) {
        uint num_s2_context_banks = read_uint(arm_smmu_device_addr + field_offset(arm_smmu_device, num_s2_context_banks), "num_s2_context_banks");
        oss << "    num_s2_context_banks : " << std::dec << num_s2_context_banks << "\n";
    }

    // Read and print va_size if field exists
    if (field_offset(arm_smmu_device, va_size) != -1) {
        ulong va_size = read_ulong(arm_smmu_device_addr + field_offset(arm_smmu_device, va_size), "va_size");
        oss << "    va_size              : " << std::dec << va_size << "\n";
    }

    // Read and print ipa_size if field exists
    if (field_offset(arm_smmu_device, ipa_size) != -1) {
        ulong ipa_size = read_ulong(arm_smmu_device_addr + field_offset(arm_smmu_device, ipa_size), "ipa_size");
        oss << "    ipa_size             : " << std::dec << ipa_size << "\n";
    }

    // Read and print pa_size if field exists
    if (field_offset(arm_smmu_device, pa_size) != -1) {
        ulong pa_size = read_ulong(arm_smmu_device_addr + field_offset(arm_smmu_device, pa_size), "pa_size");
        oss << "    pa_size              : " << std::dec << pa_size << "\n";
    }

    // Read and print context bank information
    ulong cbs_addr = read_pointer(arm_smmu_device_addr + field_offset(arm_smmu_device, cbs), "cbs");
    if (is_kvaddr(cbs_addr) && num_context_banks > 0) {
        oss << "    Context Banks:\n";
        PRINT("%s", oss.str().c_str());
        // Load enum name maps for cfg fields
        std::map<std::string, ulong> cbar_enum_list = read_enum_list("arm_smmu_cbar_type");
        std::map<std::string, ulong> fmt_enum_list = read_enum_list("arm_smmu_context_fmt");
        size_t cb_size = field_offset(arm_smmu_cb, cfg) + sizeof(ulong);
        for (uint i = 0; i < num_context_banks; ++i) {
            ulong cb_addr = cbs_addr + i * cb_size;
            print_context_bank(cb_addr, i, cbar_enum_list, fmt_enum_list);
        }
        PRINT("\n");
    } else {
        oss << "\n";
        PRINT("%s", oss.str().c_str());
    }
}

void IOMMU::print_arm_smmu_v3_device(std::shared_ptr<device> dev_ptr) {
    ulong arm_smmu_device_addr = dev_ptr->driver_data;
    if (!is_kvaddr(arm_smmu_device_addr)) {
        return;
    }

    // Read device pointer and get device name
    ulong dev_addr = read_pointer(arm_smmu_device_addr + field_offset(arm_smmu_device, dev), "dev");
    std::string device_name = get_device_name(dev_addr);

    // Read key fields from arm_smmu_device structure (SMMU v3)
    ulong base = read_pointer(arm_smmu_device_addr + field_offset(arm_smmu_device, base), "base");
    ulong page1 = read_pointer(arm_smmu_device_addr + field_offset(arm_smmu_device, page1), "page1");
    uint features = read_uint(arm_smmu_device_addr + field_offset(arm_smmu_device, features), "features");
    uint options = read_uint(arm_smmu_device_addr + field_offset(arm_smmu_device, options), "options");

    // Read strtab_cfg structure (embedded in arm_smmu_device)
    ulong strtab_cfg_addr = arm_smmu_device_addr + field_offset(arm_smmu_device, strtab_cfg);
    ulong strtab = 0;
    ulong strtab_dma = 0;
    ulong l1_desc = 0;
    uint num_l1_ents = 0;
    ulonglong strtab_base = 0;
    uint strtab_base_cfg = 0;

    if (field_offset(arm_smmu_device, strtab_cfg) != -1) {
        strtab = read_pointer(strtab_cfg_addr + field_offset(arm_smmu_strtab_cfg, strtab), "strtab");
        strtab_dma = read_ulong(strtab_cfg_addr + field_offset(arm_smmu_strtab_cfg, strtab_dma), "strtab_dma");
        l1_desc = read_pointer(strtab_cfg_addr + field_offset(arm_smmu_strtab_cfg, l1_desc), "l1_desc");
        num_l1_ents = read_uint(strtab_cfg_addr + field_offset(arm_smmu_strtab_cfg, num_l1_ents), "num_l1_ents");
        strtab_base = read_ulonglong(strtab_cfg_addr + field_offset(arm_smmu_strtab_cfg, strtab_base), "strtab_base");
        strtab_base_cfg = read_uint(strtab_cfg_addr + field_offset(arm_smmu_strtab_cfg, strtab_base_cfg), "strtab_base_cfg");
    }

    // Read iommu_device structure (embedded in arm_smmu_device)
    ulong iommu_device_addr = arm_smmu_device_addr + field_offset(arm_smmu_device, iommu);

    // Read streams rb_root
    std::vector<ulong> stream_list;
    if (field_offset(arm_smmu_device, streams) != -1) {
        ulong streams_addr = arm_smmu_device_addr + field_offset(arm_smmu_device, streams);
        ulong rb_node = read_pointer(streams_addr, "rb_node");
        if (is_kvaddr(rb_node)) {
            stream_list = for_each_rbtree(streams_addr, field_offset(arm_smmu_stream, node));
        }
    }
    // Format and print the information
    std::ostringstream oss;
    oss << "arm_smmu_device:" << std::hex << std::showbase << arm_smmu_device_addr
        << " [" << device_name << "] (SMMU v3)\n";
    oss << "    base                       : " << std::hex << std::showbase << base << "\n";
    oss << "    page1                      : " << std::hex << std::showbase << page1 << "\n";
    oss << "    features                   : " << std::hex << std::showbase << features << " (" << decode_smmu_v3_features(features) << ")\n";
    oss << "    options                    : " << std::hex << std::showbase << options << " (" << decode_smmu_options(options) << ")\n";

    // Print strtab_cfg information
    oss << "    strtab_cfg:\n";
    oss << "        strtab                 : " << std::hex << std::showbase << strtab << "\n";
    oss << "        strtab_dma             : " << std::hex << std::showbase << strtab_dma << "\n";
    oss << "        l1_desc                : " << std::hex << std::showbase << l1_desc << "\n";
    oss << "        num_l1_ents            : " << std::dec << num_l1_ents << "\n";
    oss << "        strtab_base            : " << std::hex << strtab_base << "\n";
    oss << "        strtab_base_cfg        : " << std::hex << strtab_base_cfg << "\n";

    // Print iommu_device information
    oss << "    iommu_device               : " << std::hex << std::showbase << iommu_device_addr << "\n";

    // Print streams information
    oss << "    arm_smmu_stream (rb_root)  : " << std::dec << stream_list.size() << " streams\n";

    PRINT("%s", oss.str().c_str());

    // Print detailed stream information if available
    if (!stream_list.empty()) {
        for (size_t i = 0; i < stream_list.size(); ++i) {
            ulong stream_addr = stream_list[i];
            if (!is_kvaddr(stream_addr)) {
                continue;
            }
            // Read arm_smmu_stream fields
            uint stream_id = read_uint(stream_addr + field_offset(arm_smmu_stream, id), "id");
            ulong master_addr = read_pointer(stream_addr + field_offset(arm_smmu_stream, master), "master");
            oss << "        stream_id : " << std::dec << stream_id << ", arm_smmu_master:"<< std::hex << master_addr << "\n";
        }
    }

    PRINT("\n");
}

void IOMMU::print_iommu_devices(void) {
    std::vector<std::shared_ptr<device>> device_list;
    std::shared_ptr<driver> driv_ptr = find_device_driver("arm-smmu");
    if (driv_ptr != nullptr && is_kvaddr(driv_ptr->addr)){
        device_list = for_each_device_for_driver(driv_ptr->addr);
        if(device_list.size() > 0){
            for (const auto& dev_ptr : device_list) {
                print_arm_smmu_v2_device(dev_ptr);
            }
        }
    }
    driv_ptr = find_device_driver("arm-smmu-v3");
    if (driv_ptr != nullptr && is_kvaddr(driv_ptr->addr)){
        device_list = for_each_device_for_driver(driv_ptr->addr);
        if(device_list.size() > 0){
            for (const auto& dev_ptr : device_list) {
                print_arm_smmu_v3_device(dev_ptr);
            }
        }
    }
}

/**
 * @brief Parse IOMMU debug attachments from kernel debug tracking
 *
 * This function extracts IOMMU client information from the kernel's debug
 * attachment tracking system (CONFIG_IOMMU_DEBUG_TRACKING). It populates
 * the smmu_client_list with detailed information about each IOMMU client,
 * including domain addresses, page table configuration, and device names.
 *
 */
void IOMMU::parser_iommu_debug_attachments(void) {
    // Locate the global iommu_debug_attachments list head
    ulong list_head_addr = csymbol_value("iommu_debug_attachments");
    if (!is_kvaddr(list_head_addr)) {
        // Symbol not found or invalid address - debug tracking may not be enabled
        return;
    }

    // Clear existing client list to ensure fresh data
    smmu_client_list.clear();

    // Load page table format enum mappings for human-readable output
    // This maps numeric format values to their string representations
    std::map<std::string, ulong> fmt_enum_list = read_enum_list("iommu_logger_pgtable_fmt");

    // Iterate through each iommu_debug_attachment in the linked list
    for (auto& ida_addr : for_each_list(list_head_addr, field_offset(iommu_debug_attachment, list))) {
        // Validate the attachment address before processing
        if (!is_kvaddr(ida_addr)) {
            continue;
        }

        // Create a new SMMU client structure to hold parsed data
        std::shared_ptr<smmu_client> client_ptr = std::make_shared<smmu_client>();

        // Extract core IOMMU domain and group information
        client_ptr->domain = read_pointer(ida_addr + field_offset(iommu_debug_attachment, domain), "domain");
        client_ptr->group = read_pointer(ida_addr + field_offset(iommu_debug_attachment, group), "group");

        // Read page table configuration parameters
        uint fmt = read_uint(ida_addr + field_offset(iommu_debug_attachment, fmt), "fmt");
        client_ptr->levels = read_uint(ida_addr + field_offset(iommu_debug_attachment, levels), "levels");
        client_ptr->ttbr0 = read_pointer(ida_addr + field_offset(iommu_debug_attachment, ttbr0), "ttbr0");
        client_ptr->ttbr1 = read_pointer(ida_addr + field_offset(iommu_debug_attachment, ttbr1), "ttbr1");

        // Resolve client name with fallback mechanism
        ulong client_name_addr = read_pointer(ida_addr + field_offset(iommu_debug_attachment, client_name), "client_name");
        if (is_kvaddr(client_name_addr)) {
            // Primary: Use explicit client name if available
            client_ptr->client_name = read_cstring(client_name_addr, 100, "client_name");
        }

        // Fallback: If no explicit name, try to get device name
        if (client_ptr->client_name.empty()) {
            ulong dev_addr = read_pointer(ida_addr + field_offset(iommu_debug_attachment, dev), "dev");
            if (is_kvaddr(dev_addr)) {
                client_ptr->client_name = get_device_name(dev_addr);
            }
        }

        // Convert page table format enum to human-readable string
        client_ptr->fmt_str = get_enum_name(fmt_enum_list, fmt);

        // Add the parsed client to the global list
        smmu_client_list.push_back(client_ptr);
    }
}

void IOMMU::print_iommu_client(void) {
    std::ostringstream oss;
    oss << "Total SMMU clients found: " << smmu_client_list.size() << "\n";
    PRINT("%s", oss.str().c_str());

    for (const auto& client_ptr : smmu_client_list) {
        std::ostringstream client_oss;
        client_oss << client_ptr->client_name << "\n";
        client_oss << "    domain       : " << std::hex << std::showbase << client_ptr->domain << "\n";
        client_oss << "    levels       : " << std::dec << client_ptr->levels << "\n";
        client_oss << "    ttbr0        : " << std::hex << std::showbase << client_ptr->ttbr0 << "\n";
        client_oss << "    ttbr1        : " << std::hex << std::showbase << client_ptr->ttbr1 << "\n";
        PRINT("%s", client_oss.str().c_str());
    }
}

/**
 * @brief Parse IOMMU devices from the kernel device tree
 *
 * This function walks through all devices in the system and extracts IOMMU
 * configuration for devices that use ARM SMMU. It's used as a fallback when
 * CONFIG_IOMMU_DEBUG_TRACKING is not available. The function performs deep
 * inspection of kernel data structures to reconstruct page table information.
 *
 */
void IOMMU::parser_iommu_devices(void) {
    // Clear existing client list to ensure fresh data
    smmu_client_list.clear();

    // Iterate through all devices in the system
    int device_count = 0;
    int iommu_device_count = 0;
    for (auto& dev_ptr : for_each_device()) {
        device_count++;

        // Validate device address
        if (!is_kvaddr(dev_ptr->addr)) {
            continue;
        }

        // Check if device has an IOMMU group
        ulong iommu_group_addr = read_pointer(dev_ptr->addr + field_offset(device, iommu_group), "iommu_group");
        if (!is_kvaddr(iommu_group_addr)) {
            continue;  // Device doesn't use IOMMU
        }

        iommu_device_count++;
        LOGD("Found IOMMU device [%d/%d]: %s, group: %#lx\n",
             iommu_device_count, device_count, dev_ptr->name.c_str(), iommu_group_addr);

        // Get the IOMMU domain for this group
        ulong iommu_domain_addr = read_pointer(iommu_group_addr + field_offset(iommu_group, domain), "iommu_domain");
        if (!is_kvaddr(iommu_domain_addr)) {
            LOGD("  No domain assigned to group %#lx\n", iommu_group_addr);
            continue;  // No domain assigned to this group
        }

        LOGD("  Domain: %#lx\n", iommu_domain_addr);

        // Create client structure with basic info
        std::shared_ptr<smmu_client> client_ptr = std::make_shared<smmu_client>();
        client_ptr->domain = iommu_domain_addr;
        client_ptr->group = iommu_group_addr;
        client_ptr->client_name = dev_ptr->name;

        // Determine IOMMU operations type
        // We need to verify this device uses ARM SMMU (not other IOMMU types)
        // Get device-level IOMMU operations
        ulong dev_iommu = read_pointer(dev_ptr->addr + field_offset(device, iommu), "iommu");
        ulong iommu_device = 0;
        ulong iommu_ops = 0;
        if (is_kvaddr(dev_iommu)) {
            iommu_device = read_pointer(dev_iommu + field_offset(dev_iommu, iommu_dev), "iommu_dev");
            if (is_kvaddr(iommu_device)) {
                iommu_ops = read_pointer(iommu_device + field_offset(iommu_device, ops), "ops");
            }
        }

        // Get domain-level IOMMU operations
        ulong iommu_domain_ops = 0;
        if (is_kvaddr(iommu_domain_addr)) {
            iommu_domain_ops = read_pointer(iommu_domain_addr + field_offset(iommu_domain, ops), "ops");
        }

        // Locate ARM SMMU operations structure
        ulong arm_iommu_ops = 0;
        ulong arm_iommu_domain_ops = 0;

        // Check for Qualcomm-specific IOMMU ops wrapper
        if (struct_size(qcom_iommu_ops) != -1) {
            // Qualcomm wraps ARM SMMU ops in their own structure
            if (field_offset(qcom_iommu_ops, iommu_ops) != -1) {
                arm_iommu_ops = csymbol_value("arm_smmu_ops") + field_offset(qcom_iommu_ops, iommu_ops);
            }
            if (field_offset(qcom_iommu_ops, domain_ops) != -1) {
                arm_iommu_domain_ops = csymbol_value("arm_smmu_ops") + field_offset(qcom_iommu_ops, domain_ops);
            }
        } else {
            // Standard ARM SMMU ops (no vendor wrapper)
            arm_iommu_ops = csymbol_value("arm_smmu_ops");
        }
        // Debug logging for operation addresses
        LOGD("  iommu_ops: %#lx\n", iommu_ops);
        LOGD("  arm_iommu_ops: %#lx\n", arm_iommu_ops);
        LOGD("  iommu_domain_ops: %#lx\n", iommu_domain_ops);
        LOGD("  arm_iommu_domain_ops: %#lx\n", arm_iommu_domain_ops);

        // Verify this is an ARM SMMU domain
        if (iommu_ops != arm_iommu_ops && iommu_domain_ops != arm_iommu_domain_ops) {
            LOGD("  Not an ARM SMMU device, skipping\n");
            continue;  // Not an ARM SMMU device, skip it
        }
        LOGD("  Verified as ARM SMMU device\n");
        // Locate ARM SMMU domain structure
        ulong iommu_domain_priv = 0;
        if (field_offset(iommu_domain, priv) != -1) {
            iommu_domain_priv = read_pointer(iommu_domain_addr + field_offset(iommu_domain, priv), "priv");
        }
        ulong arm_smmu_domain_ptr = 0;
        if (is_kvaddr(iommu_domain_priv)) {
            // Some kernels store arm_smmu_domain in priv field
            arm_smmu_domain_ptr = iommu_domain_priv;
        } else {
            // Calculate by subtracting embedded domain offset
            arm_smmu_domain_ptr = iommu_domain_addr - field_offset(arm_smmu_domain, domain);
        }

        if (!is_kvaddr(arm_smmu_domain_ptr)) {
            LOGD("  Invalid ARM SMMU domain pointer\n");
            continue;  // Invalid ARM SMMU domain pointer
        }
        LOGD("  arm_smmu_domain: %#lx\n", arm_smmu_domain_ptr);

        // Get page table operations pointer
        ulong io_pgtable_ops = read_pointer(arm_smmu_domain_ptr + field_offset(arm_smmu_domain, pgtbl_ops), "pgtbl_ops");
        if (!is_kvaddr(io_pgtable_ops)) {
            LOGD("  No page table operations available\n");
            continue;  // No page table operations available
        }
        LOGD("  io_pgtable_ops: %#lx\n", io_pgtable_ops);

        // Extract page table configuration
        uint level = 0;      // Number of page table levels
        ulong ttbr0 = 0;     // Translation Table Base Register 0
        ulong pgd = 0;       // Page Global Directory (top-level page table)

        // Try to get TTBR from page table configuration
        // ulong qcom_io_pgtable_info = arm_smmu_domain_ptr + field_offset(arm_smmu_domain, pgtbl_info);
        // LOGD("qcom_io_pgtable_info: %#lx\n", qcom_io_pgtable_info);

        // ulong io_pgtable_cfg = qcom_io_pgtable_info + field_offset(qcom_io_pgtable_info, cfg);
        // ulong arm_lpae_s1_cfg = io_pgtable_cfg + field_offset(io_pgtable_cfg, arm_lpae_s1_cfg);
        // LOGD("arm_lpae_s1_cfg: %#llx\n", arm_lpae_s1_cfg);

        // ttbr0 = read_ulonglong(arm_lpae_s1_cfg + field_offset(arm_lpae_s1_cfg, ttbr), "ttbr");
        // LOGD("ttbr0: %#llx\n", ttbr0);

        // Get TTBR from context bank
        ulong arm_smmu_device = read_pointer(arm_smmu_domain_ptr + field_offset(arm_smmu_domain, smmu), "arm_smmu_device");
        LOGD("  arm_smmu_device: %#lx\n", arm_smmu_device);

        ulong cbs_base = read_pointer(arm_smmu_device + field_offset(arm_smmu_device, cbs), "cbs");
        if (is_kvaddr(cbs_base)) {
            LOGD("  cbs_base: %#lx\n", cbs_base);

            // Get context bank index for this domain
            ulong arm_smmu_cfg = arm_smmu_domain_ptr + field_offset(arm_smmu_domain, cfg);
            uint8_t cbndx = read_uint(arm_smmu_cfg + field_offset(arm_smmu_cfg, cbndx), "cbndx") & 0xFF;
            LOGD("  cbndx: %d\n", cbndx);

            // Calculate context bank address
            ulong arm_smmu_cb = cbs_base + cbndx * struct_size("arm_smmu_cb");
            LOGD("  arm_smmu_cb: %#lx\n", arm_smmu_cb);

            // Read TTBR0 from context bank
            ttbr0 = read_ulong(arm_smmu_cb + field_offset(arm_smmu_cb, ttbr), "ttbr[0]");
            LOGD("  ttbr0 (raw): %#lx\n", ttbr0);

            // Apply mask to extract physical address (48-bit address space)
            ttbr0 = ttbr0 & 0xffffffffffffULL;
            LOGD("  ttbr0 (masked): %#lx\n", ttbr0);

            // Calculate page table levels
            // Navigate back to io_pgtable structure to get level information
            ulong io_pgtable = io_pgtable_ops - field_offset(io_pgtable, ops);
            ulong arm_lpae_io_pgtable = io_pgtable - field_offset(arm_lpae_io_pgtable, iop);

            if (is_kvaddr(arm_lpae_io_pgtable)) {
                LOGD("  arm_lpae_io_pgtable: %#lx\n", arm_lpae_io_pgtable);

                // Read start_level (0-3 for ARM LPAE)
                uint start_level = read_uint(arm_lpae_io_pgtable + field_offset(arm_lpae_io_pgtable, start_level), "start_level");
                LOGD("  start_level: %u\n", start_level);

                // Calculate total levels: ARM_LPAE_MAX_LEVELS (4) - start_level
                // For example: start_level=1 means 3 levels (L1, L2, L3)
                level = 4 - start_level;
                LOGD("  calculated levels: %u\n", level);

                // Get page global directory (top-level page table)
                pgd = read_pointer(arm_lpae_io_pgtable + field_offset(arm_lpae_io_pgtable, pgd), "pgd");
                LOGD("  pgd: %#lx\n", pgd);
            } else {
                LOGD("  Invalid arm_lpae_io_pgtable address\n");
            }
        } else {
            LOGD("  Invalid cbs_base address\n");
        }

        // Store extracted information
        client_ptr->levels = level;
        client_ptr->ttbr0 = pgd;
        client_ptr->ttbr1 = 0;  // TTBR1 not used in most configurations

        // Add to global client list
        smmu_client_list.push_back(client_ptr);

        LOGD("  Final - Client: %s, levels: %u, ttbr0: %#lx\n",
             client_ptr->client_name.c_str(), level, pgd);
    }

    LOGD("Total devices: %d, IOMMU devices: %d, Clients added: %zu\n",
         device_count, iommu_device_count, smmu_client_list.size());
}

void IOMMU::parser_iommu_client(void) {
    if (get_config_val("CONFIG_IOMMU_DEBUG_TRACKING") == "y" && csymbol_exists("iommu_debug_attachments")){
        parser_iommu_debug_attachments();
    }else{
        parser_iommu_devices();
    }
}

/**
 * Convert IOMMU domain type to string representation
 */
const char* IOMMU::domain_type_to_string(uint type) {
    switch (type) {
        case IOMMU_DOMAIN_BLOCKED:   return "IOMMU_DOMAIN_BLOCKED";
        case IOMMU_DOMAIN_IDENTITY:  return "IOMMU_DOMAIN_IDENTITY";
        case IOMMU_DOMAIN_UNMANAGED: return "IOMMU_DOMAIN_UNMANAGED";
        case IOMMU_DOMAIN_DMA:       return "IOMMU_DOMAIN_DMA";
        case IOMMU_DOMAIN_DMA_FQ:    return "IOMMU_DOMAIN_DMA_FQ";
        case IOMMU_DOMAIN_SVA:       return "IOMMU_DOMAIN_SVA";
        default:                     return "UNKNOWN";
    }
}

/**
 * Print detailed information about an IOMMU domain
 */
void IOMMU::print_iommu_domain_info(ulong domain_addr) {
    if (!is_kvaddr(domain_addr)) {
        return;
    }

    uint type = read_uint(domain_addr + field_offset(iommu_domain, type), "iommu_domain.type");
    ulong geom_addr = domain_addr + field_offset(iommu_domain, geometry);
    ulong aperture_start = read_ulong(geom_addr + field_offset(iommu_domain_geometry, aperture_start), "aperture_start");
    ulong aperture_end = read_ulong(geom_addr + field_offset(iommu_domain_geometry, aperture_end), "aperture_end");

    const char* type_str = domain_type_to_string(type);
    ulong arm_smmu_domain_addr = domain_addr - field_offset(arm_smmu_domain, domain);
    ulong freelist = read_pointer(arm_smmu_domain_addr + field_offset(arm_smmu_domain, freelist), "freelist");
    size_t page_count = for_each_list(freelist, field_offset(page, lru)).size();
    PRINT("         iommu_domain:%#lx(%zu) %s [%#016lx ~ %#016lx]\n",
          domain_addr, page_count, type_str, aperture_start, aperture_end);
}

void IOMMU::print_kgsl_iommu_context(ulong context_addr, const char* context_type) {
    ulong name_addr = read_pointer(context_addr + field_offset(kgsl_iommu_context, name), "name");
    std::string name = "Unknown";
    if (is_kvaddr(name_addr)) {
        name = read_cstring(name_addr, 100, "name");
    }

    ulong domain = read_pointer(context_addr + field_offset(kgsl_iommu_context, domain), "domain");
    if (is_kvaddr(domain)) {
        ulong arm_smmu_domain_addr = domain - field_offset(arm_smmu_domain, domain);
        ulong freelist = read_pointer(arm_smmu_domain_addr + field_offset(arm_smmu_domain, freelist), "freelist");
        size_t page_count = for_each_list(freelist, field_offset(page, lru)).size();
        PRINT("            kgsl_iommu_context:%#lx name:%s iommu_domain:%#lx(%zu)\n",
              context_addr, name.c_str(), domain, page_count);
    }
}

void IOMMU::print_kgsl_iommu(ulong mmu) {
    if (!is_kvaddr(mmu)) {
        return;
    }

    // Calculate kgsl_iommu address with offset check
    ulong kgsl_iommu_addr = 0;
    if (field_offset(kgsl_mmu, iommu) != -1) {
        kgsl_iommu_addr = mmu + field_offset(kgsl_mmu, iommu);
    }

    if (!is_kvaddr(kgsl_iommu_addr)) {
        return;
    }

    // Read context addresses with offset checks
    ulong user_context_addr = 0;
    if (field_offset(kgsl_iommu, user_context) != -1) {
        user_context_addr = kgsl_iommu_addr + field_offset(kgsl_iommu, user_context);
    }

    ulong secure_context_addr = 0;
    if (field_offset(kgsl_iommu, secure_context) != -1) {
        secure_context_addr = kgsl_iommu_addr + field_offset(kgsl_iommu, secure_context);
    }

    ulong lpac_context_addr = 0;
    if (field_offset(kgsl_iommu, lpac_context) != -1) {
        lpac_context_addr = kgsl_iommu_addr + field_offset(kgsl_iommu, lpac_context);
    }

    // Read smmu_info and pagesize with offset checks
    ulong smmu_info = 0;
    if (field_offset(kgsl_iommu, smmu_info) != -1) {
        smmu_info = read_pointer(kgsl_iommu_addr + field_offset(kgsl_iommu, smmu_info), "smmu_info");
    }

    uint32_t pagesize = 0;
    if (field_offset(kgsl_iommu, pagesize) != -1) {
        pagesize = read_uint(kgsl_iommu_addr + field_offset(kgsl_iommu, pagesize), "pagesize");
    }

    // Print using std::ostringstream
    std::ostringstream oss;
    oss << "        kgsl_iommu:" << std::hex << std::showbase << kgsl_iommu_addr
        << " kgsl_memdesc:" << smmu_info
        << " pagesize:" << pagesize << "\n";
    PRINT("%s", oss.str().c_str());

    // Print contexts if addresses are valid
    if (is_kvaddr(user_context_addr)) {
        print_kgsl_iommu_context(user_context_addr, "user");
    }
    if (is_kvaddr(secure_context_addr)) {
        print_kgsl_iommu_context(secure_context_addr, "secure");
    }
    if (is_kvaddr(lpac_context_addr)) {
        print_kgsl_iommu_context(lpac_context_addr, "lpac");
    }
}

void IOMMU::print_kgsl_pagetable(ulong pagetable) {
    if (!is_kvaddr(pagetable)) {
        return;
    }

    // Calculate kgsl_iommu_pt address with offset check
    ulong kgsl_iommu_pt_addr = 0;
    if (field_offset(kgsl_iommu_pt, base) != -1) {
        kgsl_iommu_pt_addr = pagetable - field_offset(kgsl_iommu_pt, base);
    }

    // Read kgsl_iommu_pt fields with offset checks
    uint64_t ttbr0 = 0;
    if (is_kvaddr(kgsl_iommu_pt_addr) && field_offset(kgsl_iommu_pt, ttbr0) != -1) {
        ttbr0 = read_ulong(kgsl_iommu_pt_addr + field_offset(kgsl_iommu_pt, ttbr0), "ttbr0");
    }

    ulong pgtbl_ops = 0;
    if (is_kvaddr(kgsl_iommu_pt_addr) && field_offset(kgsl_iommu_pt, pgtbl_ops) != -1) {
        pgtbl_ops = read_pointer(kgsl_iommu_pt_addr + field_offset(kgsl_iommu_pt, pgtbl_ops), "pgtbl_ops");
    }

    // Read kgsl_pagetable fields with offset checks
    ulong mmu = 0;
    if (field_offset(kgsl_pagetable, mmu) != -1) {
        mmu = read_pointer(pagetable + field_offset(kgsl_pagetable, mmu), "mmu");
    }

    uint64_t va_start = 0;
    if (field_offset(kgsl_pagetable, va_start) != -1) {
        va_start = read_ulong(pagetable + field_offset(kgsl_pagetable, va_start), "va_start");
    }

    uint64_t va_end = 0;
    if (field_offset(kgsl_pagetable, va_end) != -1) {
        va_end = read_ulong(pagetable + field_offset(kgsl_pagetable, va_end), "va_end");
    }

    uint64_t svm_start = 0;
    if (field_offset(kgsl_pagetable, svm_start) != -1) {
        svm_start = read_ulong(pagetable + field_offset(kgsl_pagetable, svm_start), "svm_start");
    }

    uint64_t svm_end = 0;
    if (field_offset(kgsl_pagetable, svm_end) != -1) {
        svm_end = read_ulong(pagetable + field_offset(kgsl_pagetable, svm_end), "svm_end");
    }

    uint64_t compat_va_start = 0;
    if (field_offset(kgsl_pagetable, compat_va_start) != -1) {
        compat_va_start = read_ulong(pagetable + field_offset(kgsl_pagetable, compat_va_start), "compat_va_start");
    }

    uint64_t compat_va_end = 0;
    if (field_offset(kgsl_pagetable, compat_va_end) != -1) {
        compat_va_end = read_ulong(pagetable + field_offset(kgsl_pagetable, compat_va_end), "compat_va_end");
    }

    // Print using std::ostringstream
    std::ostringstream oss;
    oss << "    kgsl_iommu_pt:" << std::hex << std::showbase << kgsl_iommu_pt_addr
        << " ttbr0:" << ttbr0
        << " io_pgtable_ops:" << pgtbl_ops << "\n";
    oss << "    kgsl_pagetable:" << pagetable
        << " kgsl_mmu:" << mmu
        << " va:[" << va_start << "~" << va_end << "]"
        << " svm:[" << svm_start << "~" << svm_end << "]"
        << " compat_va:[" << compat_va_start << "~" << compat_va_end << "]\n";
    PRINT("%s", oss.str().c_str());

    print_kgsl_iommu(mmu);
}


void IOMMU::print_kgsl_process(ulong kpp_addr) {
    if (!is_kvaddr(kpp_addr)) {
        return;
    }

    // Read fields with offset checks
    ulong priv = 0;
    if (field_offset(kgsl_process_private, priv) != -1) {
        priv = read_ulong(kpp_addr + field_offset(kgsl_process_private, priv), "priv");
    }

    ulong pid_addr = 0;
    if (field_offset(kgsl_process_private, pid) != -1) {
        pid_addr = read_pointer(kpp_addr + field_offset(kgsl_process_private, pid), "pid");
    }

    char comm[17] = {0};
    if (field_offset(kgsl_process_private, comm) != -1) {
        readmem(kpp_addr + field_offset(kgsl_process_private, comm), KVADDR, comm, 16, TO_CONST_STRING("comm"), FAULT_ON_ERROR);
    }

    ulong pagetable = 0;
    if (field_offset(kgsl_process_private, pagetable) != -1) {
        pagetable = read_pointer(kpp_addr + field_offset(kgsl_process_private, pagetable), "pagetable");
    }

    int fd_count = 0;
    if (field_offset(kgsl_process_private, fd_count) != -1) {
        fd_count = read_int(kpp_addr + field_offset(kgsl_process_private, fd_count), "fd_count");
    }

    int ctxt_count = 0;
    if (field_offset(kgsl_process_private, ctxt_count) != -1) {
        ctxt_count = read_int(kpp_addr + field_offset(kgsl_process_private, ctxt_count), "ctxt_count");
    }

    long frame_count = 0;
    if (field_offset(kgsl_process_private, frame_count) != -1) {
        frame_count = read_long(kpp_addr + field_offset(kgsl_process_private, frame_count), "frame_count");
    }

    int unpinned_page_count = 0;
    if (field_offset(kgsl_process_private, unpinned_page_count) != -1) {
        unpinned_page_count = read_int(kpp_addr + field_offset(kgsl_process_private, unpinned_page_count), "unpinned_page_count");
    }

    uint fault_count = 0;
    if (field_offset(kgsl_process_private, fault_count) != -1) {
        fault_count = read_uint(kpp_addr + field_offset(kgsl_process_private, fault_count), "fault_count");
    }

    uint pf_count = 0;
    if (field_offset(kgsl_process_private, pf_count) != -1) {
        pf_count = read_uint(kpp_addr + field_offset(kgsl_process_private, pf_count), "pf_count");
    }

    int pid_value = 0;
    if (is_kvaddr(pid_addr)) {
        pid_value = read_int(pid_addr, "pid_value");
    }

    std::ostringstream oss;
    oss << "kgsl_process_private:" << std::hex << std::showbase << kpp_addr
        << " priv:" << priv
        << " pid:" << std::dec << pid_value
        << " comm:" << comm
        << " fd_count:" << fd_count
        << " ctxt_count:" << ctxt_count
        << " frame_count:" << frame_count
        << " unpinned_page_count:" << unpinned_page_count
        << " fault_count:" << fault_count
        << " pf_count:" << pf_count << "\n";
    PRINT("%s", oss.str().c_str());

    print_kgsl_pagetable(pagetable);
    PRINT("\n");
}

void IOMMU::print_kgsl(void) {
    if (!csymbol_exists("kgsl_driver")){
        return;
    }
    ulong kgsl_driver_addr = csymbol_value("kgsl_driver");
    if (!is_kvaddr(kgsl_driver_addr)) {
        return;
    }

    // Read atomic_long_t values from the embedded stats structure with offset checks
    long vmalloc = 0;
    if (field_offset(kgsl_driver, stats.vmalloc) != -1) {
        vmalloc = read_long(kgsl_driver_addr + field_offset(kgsl_driver, stats.vmalloc), "vmalloc");
    }

    long page_alloc = 0;
    if (field_offset(kgsl_driver, stats.page_alloc) != -1) {
        page_alloc = read_long(kgsl_driver_addr + field_offset(kgsl_driver, stats.page_alloc), "page_alloc");
    }

    long coherent = 0;
    if (field_offset(kgsl_driver, stats.coherent) != -1) {
        coherent = read_long(kgsl_driver_addr + field_offset(kgsl_driver, stats.coherent), "coherent");
    }

    long secure = 0;
    if (field_offset(kgsl_driver, stats.secure) != -1) {
        secure = read_long(kgsl_driver_addr + field_offset(kgsl_driver, stats.secure), "secure");
    }

    long mapped = 0;
    if (field_offset(kgsl_driver, stats.mapped) != -1) {
        mapped = read_long(kgsl_driver_addr + field_offset(kgsl_driver, stats.mapped), "mapped");
    }

    std::ostringstream oss;
    oss << "kgsl_driver:" << std::hex << std::showbase << kgsl_driver_addr << "\n";
    oss << " vmalloc:    " << csize(vmalloc) << "\n";
    oss << " page_alloc: " << csize(page_alloc) << "\n";
    oss << " coherent:   " << csize(coherent) << "\n";
    oss << " secure:     " << csize(secure) << "\n";
    oss << " mapped:     " << csize(mapped) << "\n\n";
    PRINT("%s", oss.str().c_str());

    if (field_offset(kgsl_driver, process_list) != -1 && field_offset(kgsl_process_private, list) != -1) {
        for (const auto& kpp_addr : for_each_list(kgsl_driver_addr + field_offset(kgsl_driver, process_list), field_offset(kgsl_process_private, list))) {
            print_kgsl_process(kpp_addr);
        }
    }
}

/**
 * Print IOMMU groups with detailed information
 *
 * This function displays all IOMMU groups in the system, organized by their
 * associated ARM SMMU devices. For each group, it shows:
 * - Group ID and address
 * - ARM SMMU domain configuration (context bank index, ASID, VMID, etc.)
 * - IOMMU domain information (type, address range)
 * - Associated devices with their Stream IDs (SIDs)
 *
 * The output is sorted by group ID within each SMMU device for better readability.
 */
void IOMMU::print_iommu_groups(void) {
    // Load enum mappings for translating numeric values to readable strings
    std::map<std::string, ulong> cbar_enum_list = read_enum_list("arm_smmu_cbar_type");
    std::map<std::string, ulong> fmt_enum_list = read_enum_list("arm_smmu_context_fmt");
    std::map<std::string, ulong> stage_enum_list = read_enum_list("arm_smmu_domain_stage");

    // Collect unique group addresses from SMMU client list
    std::set<ulong> group_set;
    for (const auto& client_ptr : smmu_client_list) {
        group_set.insert(client_ptr->group);
    }

    // Parse group information into structured data
    // This separates data collection from presentation for better code organization
    std::vector<GroupInfo> group_list;
    group_list.reserve(group_set.size()); // Pre-allocate memory for efficiency

    for (const auto& group_addr : group_set) {
        GroupInfo info;
        info.addr = group_addr;

        // Read group ID
        info.id = read_uint(group_addr + field_offset(iommu_group, id), "id");

        // Read IOMMU domain address (try domain first, then default_domain)
        info.iommu_domain = read_pointer(group_addr + field_offset(iommu_group, domain), "domain");
        if (!is_kvaddr(info.iommu_domain)) {
            info.iommu_domain = read_pointer(group_addr + field_offset(iommu_group, default_domain), "default_domain");
        }

        // Calculate ARM SMMU domain address from IOMMU domain
        info.arm_smmu_domain = info.iommu_domain - field_offset(arm_smmu_domain, domain);

        // Read ARM SMMU device address
        info.arm_smmu_device = read_pointer(info.arm_smmu_domain + field_offset(arm_smmu_domain, smmu), "smmu");

        group_list.push_back(info);
    }

    // Sort groups by ID for consistent output ordering
    std::sort(group_list.begin(), group_list.end(),
              [](const GroupInfo& a, const GroupInfo& b) {
                  return a.id < b.id;
              });

    // Group by ARM SMMU device for hierarchical display
    // This organizes groups under their parent SMMU device
    std::map<ulong, std::vector<GroupInfo>> smmu_to_groups;
    for (const auto& group : group_list) {
        smmu_to_groups[group.arm_smmu_device].push_back(group);
    }

    // Print information organized by SMMU device
    for (const auto& smmu_pair : smmu_to_groups) {
        ulong arm_smmu_device_addr = smmu_pair.first;
        const auto& groups = smmu_pair.second;

        // Get and print SMMU device name
        std::shared_ptr<device> arm_smmu_device_ptr = parser_device(
            read_pointer(arm_smmu_device_addr + field_offset(arm_smmu_device, dev), "device"));
        PRINT("arm_smmu_device:%#lx[%s]\n", arm_smmu_device_addr, arm_smmu_device_ptr->name.c_str());

        // Print each group under this SMMU device
        for (const auto& group : groups) {
            PRINT("    [%03d]iommu_group:%#lx\n", group.id, group.addr);

            // Read ARM SMMU configuration from the domain
            ulong arm_smmu_domain = group.iommu_domain - field_offset(arm_smmu_domain, domain);
            ulong cfg_base = arm_smmu_domain + field_offset(arm_smmu_domain, cfg);

            // Read context bank configuration fields
            uint8_t cbndx = read_uint(cfg_base + field_offset(arm_smmu_cfg, cbndx), "cbndx") & 0xFF;
            uint16_t vmid = read_uint(cfg_base + field_offset(arm_smmu_cfg, vmid), "vmid") & 0xFFFF;
            uint32_t procid = read_uint(cfg_base + field_offset(arm_smmu_cfg, procid), "procid");
            uint32_t cbar_val = read_uint(cfg_base + field_offset(arm_smmu_cfg, cbar), "cbar");
            uint32_t fmt_val = read_uint(cfg_base + field_offset(arm_smmu_cfg, fmt), "fmt");

            // Convert enum values to readable strings
            std::string cbar_str = get_enum_name(cbar_enum_list, cbar_val);
            std::string fmt_str = get_enum_name(fmt_enum_list, fmt_val);

            // Read domain-level configuration
            uint32_t stage_val = read_uint(arm_smmu_domain + field_offset(arm_smmu_domain, stage), "stage");
            uint32_t secure_vmid = read_uint(arm_smmu_domain + field_offset(arm_smmu_domain, secure_vmid), "secure_vmid");
            bool skip_tlb = read_bool(arm_smmu_domain + field_offset(arm_smmu_domain, skip_tlb_management), "skip_tlb_management");

            std::string stage_str = get_enum_name(stage_enum_list, stage_val);

            // Print ARM SMMU domain configuration summary
            std::ostringstream domain_oss;
            domain_oss << "         arm_smmu_domain:" << std::hex << std::showbase << arm_smmu_domain
                      << ",cbndx=" << std::dec << (unsigned)cbndx
                      << ",vmid=" << vmid
                      << ",procid=" << procid
                      << ",cbar=" << cbar_str
                      << ",fmt=" << fmt_str
                      << "," << stage_str
                      << ",sec_vmid=" << secure_vmid
                      << ",skip_tlb=" << (skip_tlb ? "true" : "false") << "\n";
            PRINT("%s", domain_oss.str().c_str());

            // Print IOMMU domain details (type, address range, page count)
            print_iommu_domain_info(group.iommu_domain);

            // Print all devices in this group with their Stream IDs
            ulong devices_head = group.addr + field_offset(iommu_group, devices);
            for (const auto& addr : for_each_list(devices_head, field_offset(group_device, list))) {
                ulong dev_addr = read_pointer(addr + field_offset(group_device, dev), "dev addr");
                ulong name_addr = read_pointer(addr + field_offset(group_device, name), "name addr");
                std::string name = read_cstring(name_addr, 100, "device name");

                // Extract Stream ID (SID) from device's IOMMU firmware spec
                ulong dev_iommu_addr = read_pointer(dev_addr + field_offset(device, iommu), "dev_iommu");
                uint sid = 0;
                if (is_kvaddr(dev_iommu_addr)) {
                    ulong fwspec_addr = read_pointer(dev_iommu_addr + field_offset(dev_iommu, fwspec), "iommu_fwspec");
                    uint ids = read_uint(fwspec_addr + field_offset(iommu_fwspec, ids), "ids");
                    sid = ids & 0xffff; // Extract lower 16 bits as SID
                }
                PRINT("            SID:%#-8x device:%#lx[%s] \n", sid, dev_addr, name.c_str());
            }
            PRINT("\n");
        }
    }
}

/**
 * Initialize kernel structure field offsets
 */
void IOMMU::init_offset(void) {
    //  Core Kernel Structures
    field_init(page, lru);
    field_init(device, iommu_group);
    field_init(device, iommu);
    field_init(device, driver_data);

    //  IOMMU Group Structures
    field_init(iommu_group, domain);
    field_init(iommu_group, id);
    field_init(iommu_group, devices);

    field_init(group_device, list);
    field_init(group_device, name);
    field_init(group_device, dev);

    //  Per-Device IOMMU Structures
    field_init(dev_iommu, iommu_dev);
    field_init(dev_iommu, fwspec);
    field_init(iommu_fwspec, ids);

    field_init(iommu_device, dev);
    field_init(iommu_device, ops);

    //  IOMMU Domain Structures
    field_init(iommu_domain, type);
    field_init(iommu_domain, ops);
    field_init(iommu_domain, pgsize_bitmap);
    field_init(iommu_domain, geometry);
    field_init(iommu_domain, iova_cookie);
    field_init(iommu_domain, iopf_handler);
    field_init(iommu_domain, fault_data);
    field_init(iommu_domain, priv);

    field_init(iommu_domain_geometry, aperture_start);
    field_init(iommu_domain_geometry, aperture_end);
    field_init(iommu_domain_geometry, force_aperture);

    //  ARM SMMU Device Structures (v2/v3)
    field_init(arm_smmu_device, dev);
    field_init(arm_smmu_device, base);
    field_init(arm_smmu_device, page1);
    field_init(arm_smmu_device, ioaddr);
    field_init(arm_smmu_device, numpage);
    field_init(arm_smmu_device, pgshift);
    field_init(arm_smmu_device, features);
    field_init(arm_smmu_device, options);
    field_init(arm_smmu_device, version);
    field_init(arm_smmu_device, model);
    field_init(arm_smmu_device, num_context_banks);
    field_init(arm_smmu_device, num_s2_context_banks);
    field_init(arm_smmu_device, va_size);
    field_init(arm_smmu_device, ipa_size);
    field_init(arm_smmu_device, pa_size);
    field_init(arm_smmu_device, cbs);
    field_init(arm_smmu_device, strtab_cfg);
    field_init(arm_smmu_device, iommu);
    field_init(arm_smmu_device, streams);

    //  ARM SMMU v3 Specific Structures
    field_init(arm_smmu_strtab_cfg, strtab);
    field_init(arm_smmu_strtab_cfg, strtab_dma);
    field_init(arm_smmu_strtab_cfg, l1_desc);
    field_init(arm_smmu_strtab_cfg, num_l1_ents);
    field_init(arm_smmu_strtab_cfg, strtab_base);
    field_init(arm_smmu_strtab_cfg, strtab_base_cfg);

    field_init(arm_smmu_stream, id);
    field_init(arm_smmu_stream, master);
    field_init(arm_smmu_stream, node);

    field_init(arm_smmu_master, node);
    field_init(arm_smmu_master, dev);
    field_init(arm_smmu_master, smmu);
    field_init(arm_smmu_master, streams);
    field_init(arm_smmu_master, num_streams);

    //  ARM SMMU Domain Structures
    field_init(arm_smmu_domain, domain);
    field_init(arm_smmu_domain, smmu);
    field_init(arm_smmu_domain, cfg);
    field_init(arm_smmu_domain, stage);
    field_init(arm_smmu_domain, secure_vmid);
    field_init(arm_smmu_domain, skip_tlb_management);
    field_init(arm_smmu_domain, freelist);
    field_init(arm_smmu_domain, pgtbl_ops);
    field_init(arm_smmu_domain, pgtbl_info);

    //  ARM SMMU Context Bank Structures
    struct_init("arm_smmu_cb");
    field_init(arm_smmu_cb, ttbr);
    field_init(arm_smmu_cb, tcr);
    field_init(arm_smmu_cb, mair);
    field_init(arm_smmu_cb, sctlr);
    field_init(arm_smmu_cb, cfg);

    //  ARM SMMU Configuration Structures
    field_init(arm_smmu_cfg, cbndx);
    field_init(arm_smmu_cfg, irptndx);
    field_init(arm_smmu_cfg, asid);
    field_init(arm_smmu_cfg, vmid);
    field_init(arm_smmu_cfg, procid);
    field_init(arm_smmu_cfg, sctlr);
    field_init(arm_smmu_cfg, cbar);
    field_init(arm_smmu_cfg, fmt);

    //  Page Table Structures
    field_init(io_pgtable_ops, map);
    field_init(io_pgtable_ops, map_pages);
    field_init(io_pgtable, ops);

    field_init(arm_lpae_io_pgtable, iop);
    field_init(arm_lpae_io_pgtable, levels);
    field_init(arm_lpae_io_pgtable, start_level);
    field_init(arm_lpae_io_pgtable, pgd);

    field_init(qcom_io_pgtable_info, vmid);
    field_init(qcom_io_pgtable_info, iova_base);
    field_init(qcom_io_pgtable_info, iova_end);
    field_init(qcom_io_pgtable_info, cfg);

    field_init(io_pgtable_cfg, arm_lpae_s1_cfg);
    field_init(arm_lpae_s1_cfg, ttbr);

    //  IOMMU Debug Structures
    field_init(iommu_debug_attachment, domain);
    field_init(iommu_debug_attachment, group);
    field_init(iommu_debug_attachment, client_name);
    field_init(iommu_debug_attachment, fmt);
    field_init(iommu_debug_attachment, levels);
    field_init(iommu_debug_attachment, ttbr0);
    field_init(iommu_debug_attachment, ttbr1);
    field_init(iommu_debug_attachment, list);
    field_init(iommu_debug_attachment, dev);

    //  Vendor-Specific IOMMU Operations
    struct_init(qcom_iommu_ops);
    struct_init(msm_iommu_ops);
    field_init(qcom_iommu_ops, iommu_ops);
    field_init(qcom_iommu_ops, domain_ops);

    field_init(kgsl_driver, process_list);
    field_init(kgsl_driver, stats.vmalloc, true);
    field_init(kgsl_driver, stats.page_alloc, true);
    field_init(kgsl_driver, stats.coherent, true);
    field_init(kgsl_driver, stats.secure, true);
    field_init(kgsl_driver, stats.mapped, true);
    field_init(kgsl_process_private, list);
    field_init(kgsl_process_private, priv);
    field_init(kgsl_process_private, pid);
    field_init(kgsl_process_private, comm);
    field_init(kgsl_process_private, pagetable);
    field_init(kgsl_process_private, fd_count);
    field_init(kgsl_process_private, ctxt_count);
    field_init(kgsl_process_private, frame_count);
    field_init(kgsl_process_private, unpinned_page_count);
    field_init(kgsl_process_private, fault_count);
    field_init(kgsl_process_private, pf_count);
    field_init(kgsl_pagetable, mmu);
    field_init(kgsl_pagetable, va_start);
    field_init(kgsl_pagetable, va_end);
    field_init(kgsl_pagetable, svm_start);
    field_init(kgsl_pagetable, svm_end);
    field_init(kgsl_pagetable, compat_va_start);
    field_init(kgsl_pagetable, compat_va_end);
    field_init(kgsl_mmu, iommu);
    field_init(kgsl_iommu_pt, base);
    field_init(kgsl_iommu_pt, ttbr0);
    field_init(kgsl_iommu_pt, pgtbl_ops);
    field_init(kgsl_iommu, user_context);
    field_init(kgsl_iommu, secure_context);
    field_init(kgsl_iommu, lpac_context);
    field_init(kgsl_iommu, smmu_info);
    field_init(kgsl_iommu, pdev);
    field_init(kgsl_iommu, pagesize);
    field_init(kgsl_iommu_context, name);
    field_init(kgsl_iommu_context, domain);
}

/**
 * Initialize command help and usage information
 */
void IOMMU::init_command(void) {
    cmd_name = "iommu";
    help_str_list = {
        "iommu",
        "display IOMMU (Input-Output Memory Management Unit) information",
        "-d | -c | -g | -p <client_name>",
        "  This command displays IOMMU device, client, and domain information.",
        "\n",
        "OPTIONS",
        "  -d",
        "    List all ARM SMMU devices with detailed hardware configuration.",
        "    Displays device base address, features, context banks, and more.",
        "",
        "  -c",
        "    Display IOMMU client information including domain addresses,",
        "    page table levels, and translation table base registers (TTBR).",
        "",
        "  -g",
        "    Display IOMMU groups with their associated devices and domains.",
        "    Shows domain configuration, context bank settings, and device mappings.",
        "",
        "  -p <client_name>",
        "    Dump AArch64 page tables for the specified SMMU client.",
        "    Displays virtual-to-physical address mappings with attributes.",
        "\n",
        "EXAMPLES",
        "  List all IOMMU devices:",
        "    %s> iommu -d",
        "    arm_smmu_device:0xffffff880a234000 [15000000.iommu]",
        "        base                 : 0x15000000",
        "        ioaddr               : 0x15000000",
        "        numpage              : 256",
        "        version              : ARM_SMMU_V2",
        "        model                : QCOM_SMMUV2",
        "        num_context_banks    : 64",
        "        va_size              : 48",
        "        Context Banks:",
        "           CB[05]:ttbr[0]=0x... ttbr[1]=0x...",
        "             arm_smmu_cfg         : 0xffffff8808a5c100",
        "               cbndx              : 5",
        "               vmid               : 200",
        "               cbar               : CBAR_TYPE_S2_TRANS",
        "               fmt                : ARM_SMMU_CTX_FMT_AARCH64",
        "\n",
        "  Display IOMMU client information:",
        "    %s> iommu -c",
        "    Total SMMU clients found: 5",
        "    client_device_name",
        "        domain       : 0xffffff8808a5c000",
        "        levels       : 3",
        "        ttbr0        : 0x123456789000",
        "        ttbr1        : 0x0",
        "\n",
        "  Display IOMMU groups:",
        "    %s> iommu -g",
        "    arm_smmu_device:0xffffff880a234000[15000000.iommu]",
        "        [003]iommu_group:0xffffff8808a5b000",
        "             iommu_domain:0xffffff8808a5c000(10) IOMMU_DOMAIN_DMA [0x0 ~ 0xffffffffffffffff]",
        "             arm_smmu_domain      : 0xffffff8808a5bf00",
        "               cbndx              : 5",
        "               vmid               : 200",
        "               procid             : 0",
        "               cbar               : CBAR_TYPE_S2_TRANS",
        "               fmt                : ARM_SMMU_CTX_FMT_AARCH64",
        "               stage              : ARM_SMMU_DOMAIN_S2",
        "               secure_vmid        : 0",
        "               skip_tlb           : false",
        "             SID:0x100    device:0xffffff8808a5d000[device_name]",
        "\n",
        "  Dump page tables for a specific client:",
        "    %s> iommu -p client_device_name",
        "    Client: client_device_name",
        "    TTBR0: 0x123456789000",
        "    Levels: 3",
        "    [VA Start -- VA End  ] [Size] [PA Start -- PA End] [Attributes]...",
        "    0x0000000000000000--0x00000000001fffff [0x200000] A:0x... [R/W][2M]...",
        "\n",
    };
}

/**
 * Default constructor
 */
IOMMU::IOMMU() {

}

/**
 * Dump AArch64 page tables for all SMMU clients
 */
void IOMMU::dump_aarch64_page_tables(const std::string& name) {
    if (smmu_client_list.empty()) {
        parser_iommu_client();
    }
    if (smmu_client_list.empty()) {
        PRINT("No SMMU clients found\n");
        return;
    }
    for (const auto& client : smmu_client_list) {
        if (client->client_name == name && client->ttbr0 != 0 && client->levels > 0) {
            // Create an instance of AArch64PTParser and use it
            AArch64PTParser parser;
            parser.parse_and_print_tables(client->ttbr0, client->levels, client->client_name);
        }
    }
}

#pragma GCC diagnostic pop
