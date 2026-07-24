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

#ifndef IOMMU_DEFS_H_
#define IOMMU_DEFS_H_

#include "plugin.h"
#include "../utils/aarch64_pagetable.h"

#include <memory>
#include <string>
#include <vector>
#include <map>

// ============================================================================
// IOMMU Group Information Structure
// ============================================================================

/**
 * @brief Structure representing IOMMU group information
 *
 * This structure contains essential information about an IOMMU group,
 * including its address, ID, and associated domain/device information.
 */
struct GroupInfo {
    ulong addr;              // Address of the IOMMU group structure in memory
    int id;                  // Unique identifier for the IOMMU group
    ulong iommu_domain;      // Address of the associated IOMMU domain
    ulong arm_smmu_domain;   // Address of the ARM SMMU-specific domain structure
    ulong arm_smmu_device;   // Address of the ARM SMMU device structure
};

// ============================================================================
// SMMU Client Information Structure
// ============================================================================

/**
 * @brief Structure representing an SMMU (System Memory Management Unit) client
 *
 * This structure contains all necessary information about a device that uses
 * the SMMU for address translation, including domain information, page table
 * configuration, and translation table base registers.
 */
struct smmu_client {
    ulong domain;                // Address of the iommu_domain structure
    ulong group;                 // Address of the iommu_group structure
    std::string client_name;     // Human-readable name of the client device
    std::string fmt_str;         // Page table format enumeration name (e.g., "ARM_64_LPAE_S1")
    uint levels;                 // Number of page table levels (typically 3 or 4)
    ulong ttbr0;                 // Translation Table Base Register 0 (user space)
    ulong ttbr1;                 // Translation Table Base Register 1 (kernel space)
};

// ============================================================================
// ARM SMMU Feature Flags
// ============================================================================

/**
 * ARM SMMU hardware feature capability flags
 * These flags indicate which features are supported by the SMMU hardware
 */

// Two-level stream table support
#define ARM_SMMU_FEAT_2_LVL_STRTAB   (1 << 0)
// Two-level context descriptor table support
#define ARM_SMMU_FEAT_2_LVL_CDTAB    (1 << 1)
// Little-endian translation table format support
#define ARM_SMMU_FEAT_TT_LE          (1 << 2)
// Big-endian translation table format support
#define ARM_SMMU_FEAT_TT_BE          (1 << 3)
// Page Request Interface (PRI) support
#define ARM_SMMU_FEAT_PRI            (1 << 4)
// Address Translation Service (ATS) support
#define ARM_SMMU_FEAT_ATS            (1 << 5)
// System Error (SEV) signaling support
#define ARM_SMMU_FEAT_SEV            (1 << 6)
// Message Signaled Interrupts (MSI) support
#define ARM_SMMU_FEAT_MSI            (1 << 7)
// Hardware coherency support for page table walks
#define ARM_SMMU_FEAT_COHERENCY      (1 << 8)
// Stage 1 translation support
#define ARM_SMMU_FEAT_TRANS_S1       (1 << 9)
// Stage 2 translation support
#define ARM_SMMU_FEAT_TRANS_S2       (1 << 10)
// Transaction stalling support
#define ARM_SMMU_FEAT_STALLS         (1 << 11)
// Hypervisor mode support
#define ARM_SMMU_FEAT_HYP            (1 << 12)
// Forced stall mode support
#define ARM_SMMU_FEAT_STALL_FORCE    (1 << 13)
// Virtual address extension support
#define ARM_SMMU_FEAT_VAX            (1 << 14)
// Range-based invalidation support
#define ARM_SMMU_FEAT_RANGE_INV      (1 << 15)
// Broadcast TLB maintenance support
#define ARM_SMMU_FEAT_BTM            (1 << 16)
// Shared Virtual Addressing (SVA) support
#define ARM_SMMU_FEAT_SVA            (1 << 17)
// Enhanced virtualization (E2H) support
#define ARM_SMMU_FEAT_E2H            (1 << 18)
// Nested translation support
#define ARM_SMMU_FEAT_NESTING        (1 << 19)

// ============================================================================
// ARM SMMU Option Flags
// ============================================================================

/**
 * ARM SMMU driver configuration option flags
 * These flags control various driver behaviors and optimizations
 */

// Skip prefetching of translation table entries
#define ARM_SMMU_OPT_SKIP_PREFETCH      (1 << 0)
// Use only page 0 registers (for limited MMIO access)
#define ARM_SMMU_OPT_PAGE0_REGS_ONLY    (1 << 1)
// Use MSI polling instead of interrupts
#define ARM_SMMU_OPT_MSIPOLL            (1 << 2)
// Force synchronous command queue operations
#define ARM_SMMU_OPT_CMDQ_FORCE_SYNC    (1 << 3)
// VirtIO device support
#define ARM_SMMU_OPT_VIRTIO             (1 << 4)

// ============================================================================
// IOMMU Domain Type Flags
// ============================================================================

/**
 * IOMMU domain type internal flags
 * These are building blocks for the public domain types
 */

// Domain supports paging/translation
#define __IOMMU_DOMAIN_PAGING   (1U << 0)
// Domain uses DMA API
#define __IOMMU_DOMAIN_DMA_API  (1U << 1)
// Domain is pass-through (no translation)
#define __IOMMU_DOMAIN_PT       (1U << 2)
// Domain uses flush queue for performance
#define __IOMMU_DOMAIN_DMA_FQ   (1U << 3)
// Domain supports Shared Virtual Addressing
#define __IOMMU_DOMAIN_SVA      (1U << 4)

/**
 * IOMMU domain types
 * These define the different operational modes for IOMMU domains
 */

// Blocked domain - all transactions are blocked
#define IOMMU_DOMAIN_BLOCKED    (0U)
// Identity domain - pass-through mode, no translation
#define IOMMU_DOMAIN_IDENTITY   (__IOMMU_DOMAIN_PT)
// Unmanaged domain - manually managed by device driver
#define IOMMU_DOMAIN_UNMANAGED  (__IOMMU_DOMAIN_PAGING)
// DMA domain - managed by DMA API with standard flush
#define IOMMU_DOMAIN_DMA        (__IOMMU_DOMAIN_PAGING | __IOMMU_DOMAIN_DMA_API)
// DMA domain with flush queue - optimized for performance
#define IOMMU_DOMAIN_DMA_FQ     (__IOMMU_DOMAIN_PAGING | __IOMMU_DOMAIN_DMA_API | __IOMMU_DOMAIN_DMA_FQ)
// Shared Virtual Addressing domain
#define IOMMU_DOMAIN_SVA        (__IOMMU_DOMAIN_SVA)

/**
 * @brief IOMMU parser plugin for crash dump analysis
 *
 * This class provides comprehensive IOMMU analysis capabilities including:
 *
 * Supports both ARM SMMU v2 and v3 architectures.
 */
class IOMMU : public ParserPlugin {
    void print_kgsl_process(ulong kpp_addr);
    void print_kgsl(void);
    void print_kgsl_pagetable(ulong pagetable);
    void print_kgsl_iommu(ulong mmu);
    void print_kgsl_iommu_context(ulong context_addr, const char* context_type);
public:
    // Maximum length for device name strings
    static constexpr int MAX_DEVICE_NAME_LEN = 100;
    // Maximum length for command/process name strings
    static constexpr int MAX_COMM_LEN = 16;
    // Number of contiguous page table entries for large page support (64KB/32MB)
    static constexpr int CONTIGUOUS_ENTRY_COUNT = 15;

    /**
     * @brief Default constructor
     * Initializes the IOMMU parser plugin
     */
    IOMMU();

    void dump_aarch64_page_tables(std::string& name);

    /**
     * @brief Main command entry point
     * Called when the plugin command is invoked
     */
    void cmd_main(void) override;

    /**
     * @brief Initialize kernel structure field offsets
     * Reads and caches offsets for kernel data structures
     */
    void init_offset(void) override;

    /**
     * @brief Initialize command help and usage information
     * Sets up command syntax and help text
     */
    void init_command(void) override;

    /**
     * @brief Print ARM SMMU v2 device configuration
     * @param dev_ptr Shared pointer to the device structure
     */
    void print_arm_smmu_v2_device(std::shared_ptr<device> dev_ptr);

    /**
     * @brief Print ARM SMMU v3 device configuration
     * @param dev_ptr Shared pointer to the device structure
     */
    void print_arm_smmu_v3_device(std::shared_ptr<device> dev_ptr);

    /**
     * @brief Print all IOMMU devices in the system
     * Enumerates and displays information for all registered IOMMU devices
     */
    void print_iommu_devices(void);

    /**
     * @brief Parse IOMMU debug attachment information
     * Extracts device-to-domain attachment details from debug structures
     */
    void parser_iommu_debug_attachments(void);

    /**
     * @brief Parse IOMMU device list
     * Walks the kernel's IOMMU device list and extracts device information
     */
    void parser_iommu_devices(void);

    /**
     * @brief Parse IOMMU client information
     * Extracts SMMU client details including page table configuration
     */
    void parser_iommu_client(void);

    /**
     * @brief Print IOMMU client information
     * Displays parsed SMMU client details in a formatted table
     */
    void print_iommu_client(void);

    /**
     * @brief Print IOMMU groups with hierarchy
     * Shows IOMMU groups and their associated devices in a tree structure
     */
    void print_iommu_groups(void);

    /**
     * @brief Print detailed information about an IOMMU domain
     * @param domain_addr Address of the iommu_domain structure in memory
     */
    void print_iommu_domain_info(ulong domain_addr);

    /**
     * @brief Convert IOMMU domain type to string representation
     * @param type Domain type flags
     * @return Human-readable string describing the domain type
     */
    const char* domain_type_to_string(uint type);

    /**
     * @brief Parse and dump AArch64 page tables for SMMU clients
     * @param name Client name filter (empty string for all clients)
     *
     * Walks the page tables and displays virtual-to-physical mappings
     */
    void dump_aarch64_page_tables(const std::string& name);

    // Plugin instance definition macro
    DEFINE_PLUGIN_INSTANCE(IOMMU)

private:
    // List of all SMMU clients found in the system
    std::vector<std::shared_ptr<smmu_client>> smmu_client_list;

    /**
     * @brief Get device name from device structure address
     * @param dev_addr Address of the device structure
     * @return Device name string
     */
    std::string get_device_name(ulong dev_addr);

    /**
     * @brief Get enumeration name from value
     * @param enum_list Map of enum names to values
     * @param value Enum value to look up
     * @return Enum name string or "UNKNOWN" if not found
     */
    std::string get_enum_name(const std::map<std::string, ulong>& enum_list, ulong value);

    /**
     * @brief Print ARM SMMU configuration structure
     * @param cfg_addr Address of the configuration structure
     * @param cbar_enum_list Context Bank Attribute Register enum values
     * @param fmt_enum_list Page table format enum values
     */
    void print_arm_smmu_cfg(ulong cfg_addr,
                           const std::map<std::string, ulong>& cbar_enum_list,
                           const std::map<std::string, ulong>& fmt_enum_list);

    /**
     * @brief Print context bank information
     * @param cb_addr Address of the context bank structure
     * @param index Context bank index
     * @param cbar_enum_list Context Bank Attribute Register enum values
     * @param fmt_enum_list Page table format enum values
     */
    void print_context_bank(ulong cb_addr,
                           uint index,
                           const std::map<std::string, ulong>& cbar_enum_list,
                           const std::map<std::string, ulong>& fmt_enum_list);

    /**
     * @brief Decode SMMU v3 feature flags to human-readable string
     * @param features Feature flags bitmask
     * @return Comma-separated list of enabled features
     */
    std::string decode_smmu_v3_features(uint features);

    /**
     * @brief Decode SMMU option flags to human-readable string
     * @param options Option flags bitmask
     * @return Comma-separated list of enabled options
     */
    std::string decode_smmu_options(uint options);
};

#endif // IOMMU_DEFS_H_
