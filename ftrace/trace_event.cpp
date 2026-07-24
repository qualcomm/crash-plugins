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

#include "trace_event.h"
#include "logger/logger_core.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

/**
 * @brief Constructor for TraceEvent
 *
 * Initializes a new trace event instance with default values.
 */
TraceEvent::TraceEvent() {
}

/**
 * @brief Decode DMA direction value to string
 *
 * Converts DMA direction enumeration to human-readable string.
 *
 * @param dir DMA direction value (0-3)
 * @return String representation of DMA direction
 */
std::string TraceEvent::decode_dma_direction(int dir) {
    switch (dir) {
        case 0: return "BIDIRECTIONAL";
        case 1: return "TO_DEVICE";
        case 2: return "FROM_DEVICE";
        case 3: return "NONE";
        default: return "NONE";
    }
}

/**
 * @brief Decode DMA attributes value to string
 *
 * Converts DMA attributes bitmask to human-readable string.
 *
 * @param attrs DMA attributes bitmask
 * @return String representation of DMA attributes
 */
std::string TraceEvent::decode_dma_attrs(ulong attrs) {
    switch (attrs) {
        case 1UL << 1: return "WEAK_ORDERING";
        case 1UL << 2: return "WRITE_COMBINE";
        case 1UL << 4: return "NO_KERNEL_MAPPING";
        case 1UL << 5: return "SKIP_CPU_SYNC";
        case 1UL << 6: return "FORCE_CONTIGUOUS";
        case 1UL << 7: return "ALLOC_SINGLE_PAGES";
        case 1UL << 8: return "NO_WARN";
        case 1UL << 9: return "PRIVILEGED";
        default: return "NO_WARN";
    }
}

/**
 * @brief Print DMA unmap event information
 *
 * Formats and outputs DMA unmap event data including device name, DMA address,
 * size, direction, and attributes. This is used for debugging DMA operations.
 *
 * @param addr Address of the event data in memory
 */
void TraceEvent::print_dma_unmap_event(ulong addr) {
    // Read basic fields
    arg_list[0]->name = "device";
    arg_list[2]->name = "dma_addr";
    arg_list[3]->name = "size";
    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);

    // Decode and store DMA direction
    std::shared_ptr<trace_field> field_ptr = field_maps["dir"];
    if (field_ptr) {
        int dir = plugin_ptr->read_uint(addr + field_ptr->offset, "dir");
        copy_str(arg_list[1], decode_dma_direction(dir));
    }

    // Decode and store DMA attributes
    field_ptr = field_maps["attrs"];
    if (field_ptr) {
        ulong attrs = plugin_ptr->read_ulong(addr + field_ptr->offset, "attrs");
        copy_str(arg_list[4], decode_dma_attrs(attrs));
    }
}

/**
 * @brief Print DMA map event information
 *
 * Formats and outputs DMA map event data including device name, DMA address,
 * size, physical address, direction, and attributes. This is used for debugging
 * DMA mapping operations.
 *
 * @param addr Address of the event data in memory
 */
void TraceEvent::print_dma_map_event(ulong addr) {
    // Read basic fields
    arg_list[0]->name = "device";
    arg_list[2]->name = "dma_addr";
    arg_list[3]->name = "size";
    arg_list[4]->name = "phys_addr";
    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);

    // Decode and store DMA direction
    std::shared_ptr<trace_field> field_ptr = field_maps["dir"];
    if (field_ptr) {
        int dir = plugin_ptr->read_uint(addr + field_ptr->offset, "dir");
        copy_str(arg_list[1], decode_dma_direction(dir));
    }

    // Decode and store DMA attributes
    field_ptr = field_maps["attrs"];
    if (field_ptr) {
        ulong attrs = plugin_ptr->read_ulong(addr + field_ptr->offset, "attrs");
        copy_str(arg_list[5], decode_dma_attrs(attrs));
    }
}

/**
 * @brief Decode DWC3 TRB type to string
 *
 * Converts TRB (Transfer Request Block) type value to human-readable string.
 *
 * @param trb_type TRB type value (6-bit field)
 * @return String representation of TRB type
 */
std::string TraceEvent::decode_dwc3_trb_type(uint trb_type) {
    switch (trb_type) {
        case 1: return "Normal";
        case 2: return "Setup";
        case 3: return "Status2";
        case 4: return "Status3";
        case 5: return "Data";
        case 6: return "Isoc-First";
        case 7: return "Isoc";
        case 8: return "Link";
        default: return "Unknown";
    }
}

/**
 * @brief Print DWC3 TRB (Transfer Request Block) event information
 *
 * Formats and outputs DWC3 USB controller TRB event data including endpoint
 * name, TRB address, enqueue/dequeue pointers, buffer addresses, size, and
 * control information. This is used for debugging USB transfers.
 *
 * @param addr Address of the event data in memory
 */
void TraceEvent::print_dwc3_trb_event(ulong addr) {
    // Format: "%s: trb %p (E%d:D%d) buf %08x%08x size %s%d ctrl %08x (%c%c%c%c:%c%c:%s)"

    // Read basic TRB information
    arg_list[0]->name = "name";
    arg_list[1]->name = "trb";
    arg_list[2]->name = "enqueue";
    arg_list[3]->name = "dequeue";
    arg_list[4]->name = "bph";  // Buffer pointer high
    arg_list[5]->name = "bpl";  // Buffer pointer low

    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);

    // Read type and size fields for size prefix calculation
    std::shared_ptr<trace_field> field_ptr = field_maps["type"];
    if (!field_ptr) return;
    uint type = plugin_ptr->read_uint(addr + field_ptr->offset, "type");

    field_ptr = field_maps["size"];
    if (!field_ptr) return;
    uint size_val = plugin_ptr->read_uint(addr + field_ptr->offset, "size");

    // Calculate and store size prefix (for packet count multiplier)
    std::string size_prefix;
    int pcm = ((size_val >> 24) & 3) + 1;

    if (type == 3 || type == 1) {
        switch (pcm) {
            case 1: size_prefix = "1x "; break;
            case 2: size_prefix = "2x "; break;
            case 3:
            default: size_prefix = "3x "; break;
        }
    }
    copy_str(arg_list[6], size_prefix);

    // Store masked size value (lower 24 bits)
    uint masked_size = size_val & 0x00ffffff;
    *reinterpret_cast<uint*>(arg_list[7]->data) = masked_size;

    // Read control field
    arg_list[8]->name = "ctrl";
    read_trace_field(addr, arg_list[8]);

    field_ptr = field_maps["ctrl"];
    if (!field_ptr) return;
    uint ctrl = plugin_ptr->read_uint(addr + field_ptr->offset, "ctrl");

    // Decode control flags (H/h, L/l, C/c, S/s, S/s, C/c)
    copy_str(arg_list[9], (ctrl & (1UL << 0)) ? "H" : "h");   // HWO (Hardware Owned)
    copy_str(arg_list[10], (ctrl & (1UL << 1)) ? "L" : "l");  // LST (Last)
    copy_str(arg_list[11], (ctrl & (1UL << 2)) ? "C" : "c");  // CHN (Chain)
    copy_str(arg_list[12], (ctrl & (1UL << 3)) ? "S" : "s");  // CSP (Continue on Short Packet)
    copy_str(arg_list[13], (ctrl & (1UL << 10)) ? "S" : "s"); // Stream ID/SOF Number
    copy_str(arg_list[14], (ctrl & (1UL << 11)) ? "C" : "c"); // IOC (Interrupt on Complete)

    // Decode TRB type (bits 4-9, 6 bits)
    uint trb_type = (ctrl >> 4) & 0x3f;
    copy_str(arg_list[15], decode_dwc3_trb_type(trb_type));
}

void TraceEvent::print_rwmmio_event(std::ostringstream& oss) {
    for (const auto& arg_ptr : arg_list) {
        oss << format_arg(arg_ptr);
    }
    ulong addr_value = *reinterpret_cast<ulong*>(arg_list[4]->data);
    physaddr_t paddr = 0;
    if (kvtop(NULL, addr_value, &paddr, 0)) {
        oss << "(0x" << std::hex << paddr << ")";
    } else {
        oss << "(N/A)";
    }
}

/**
 * @brief Decode softirq vector to string
 *
 * Converts softirq vector number to human-readable string.
 *
 * @param vec Softirq vector number (0-9)
 * @return String representation of softirq type
 */
std::string TraceEvent::decode_softirq_vec(unsigned int vec) {
    switch (vec) {
        case 0: return "HI";          // High priority tasklet
        case 1: return "TIMER";       // Timer softirq
        case 2: return "NET_TX";      // Network transmit
        case 3: return "NET_RX";      // Network receive
        case 4: return "BLOCK";       // Block device
        case 5: return "IRQ_POLL";    // IRQ polling
        case 6: return "TASKLET";     // Normal priority tasklet
        case 7: return "SCHED";       // Scheduler
        case 8: return "HRTIMER";     // High resolution timer
        case 9: return "RCU";         // Read-Copy-Update
        default: return "Unknown";
    }
}

/**
 * @brief Decode alarm type to string
 *
 * Converts alarm type value to human-readable string using __print_flags logic.
 * The alarm_type is converted to a bit flag (1 << alarm_type) and matched against
 * known alarm types.
 *
 * @param alarm_type Alarm type value (0-4)
 * @return String representation of alarm type
 */
std::string TraceEvent::decode_alarm_type(uint alarm_type) {
    struct flag_map {
        unsigned int mask;
        const char *name;
    };

    struct flag_map maps[] = {
        { 1 << 0, "REALTIME" },
        { 1 << 1, "BOOTTIME" },
        { 1 << 3, "REALTIME Freezer" },
        { 1 << 4, "BOOTTIME Freezer" },
    };

    std::string type_str;
    uint alarm_flag = 1 << alarm_type;
    int first = 1;
    for (int i = 0; i < 4; i++) {
        if (alarm_flag & maps[i].mask) {
            if (!first) type_str += " | ";
            type_str += maps[i].name;
            first = 0;
        }
    }

    if (type_str.empty()) {
        type_str = "UNKNOWN";
    }

    return type_str;
}

/**
 * @brief Decode LRU page flags to string
 *
 * Converts LRU page flags to human-readable string.
 * Format: "M" or " ", "a" or "f", "s" or " ", "b" or " ", "d" or " ", "B" or " "
 *
 * @param flags Page flags value
 * @return String representation of LRU flags (6 characters)
 */
std::string TraceEvent::decode_lru_flags(unsigned int flags) {
    std::string result;
    result += (flags & 0x0001u) ? "M" : " ";  // Mapped
    result += (flags & 0x0002u) ? "a" : "f";  // Active/inactive
    result += (flags & 0x0008u) ? "s" : " ";  // Swapcache
    result += (flags & 0x0010u) ? "b" : " ";  // Buddy
    result += (flags & 0x0020u) ? "d" : " ";  // Dirty
    result += (flags & 0x0040u) ? "B" : " ";  // Writeback
    return result;
}

/**
 * @brief Decode GFP (Get Free Pages) flags to string
 *
 * Converts GFP allocation flags to human-readable string using __print_flags logic.
 * Handles complex GFP flag combinations used in memory allocation.
 *
 * @param gfp_flags GFP flags value
 * @return String representation of GFP flags (pipe-separated)
 */
std::string TraceEvent::decode_gfp_flags(unsigned long gfp_flags) {
    if (gfp_flags == 0) {
        return "none";
    }

    struct flag_map {
        unsigned long mask;
        const char *name;
    };

    // GFP flag definitions (ordered by priority for matching)
    struct flag_map maps[] = {
        // Compound flags (check these first)
        { 0x1400c0u | 0x02u | 0x08u | 0x4000000u | 0x2000000u | 0x40000u | 0x80000u | 0x2000u, "GFP_TRANSHUGE" },
        { 0x1400c0u | 0x02u | 0x08u | 0x4000000u | 0x2000000u | 0x40000u | 0x80000u | 0x2000u, "GFP_TRANSHUGE_LIGHT" },
        { 0x1400c0u | 0x02u | 0x08u | 0x4000000u | 0x2000000u, "GFP_HIGHUSER_MOVABLE" },
        { 0x1400c0u | 0x02u, "GFP_HIGHUSER" },
        { 0x1400c0u, "GFP_USER" },
        { 0x14c0u | 0x400000u, "GFP_KERNEL_ACCOUNT" },
        { 0x14c0u, "GFP_KERNEL" },
        { 0x1440u, "GFP_NOFS" },
        { 0xa20u, "GFP_ATOMIC" },
        { 0xc00u, "GFP_NOIO" },
        { 0x800u, "GFP_NOWAIT" },

        // Individual flags
        { 0x01u, "GFP_DMA" },
        { 0x02u, "__GFP_HIGHMEM" },
        { 0x04u, "GFP_DMA32" },
        { 0x20u, "__GFP_HIGH" },
        { 0x200u, "__GFP_ATOMIC" },
        { 0x40u, "__GFP_IO" },
        { 0x80u, "__GFP_FS" },
        { 0x2000u, "__GFP_NOWARN" },
        { 0x4000u, "__GFP_RETRY_MAYFAIL" },
        { 0x8000u, "__GFP_NOFAIL" },
        { 0x10000u, "__GFP_NORETRY" },
        { 0x40000u, "__GFP_COMP" },
        { 0x100u, "__GFP_ZERO" },
        { 0x80000u, "__GFP_NOMEMALLOC" },
        { 0x20000u, "__GFP_MEMALLOC" },
        { 0x100000u, "__GFP_HARDWALL" },
        { 0x200000u, "__GFP_THISNODE" },
        { 0x10u, "__GFP_RECLAIMABLE" },
        { 0x08u, "__GFP_MOVABLE" },
        { 0x400000u, "__GFP_ACCOUNT" },
        { 0x1000u, "__GFP_WRITE" },
        { 0xc00u, "__GFP_RECLAIM" },
        { 0x400u, "__GFP_DIRECT_RECLAIM" },
        { 0x800u, "__GFP_KSWAPD_RECLAIM" },
        { 0x800000u, "__GFP_ZEROTAGS" },
        { 0x1000000u, "__GFP_SKIP_ZERO" },
        { 0x4000000u, "__GFP_SKIP_KASAN_POISON" },
        { 0x2000000u, "__GFP_SKIP_KASAN_UNPOISON" },
    };

    std::string result;
    unsigned long remaining = gfp_flags;
    int first = 1;

    // Try to match compound flags first
    for (int i = 0; i < 11; i++) {
        if ((gfp_flags & maps[i].mask) == maps[i].mask) {
            if (!first) result += "|";
            result += maps[i].name;
            remaining &= ~maps[i].mask;
            first = 0;
            break;  // Only match one compound flag
        }
    }

    // Match individual flags
    for (long unsigned int i = 11; i < sizeof(maps) / sizeof(maps[0]); i++) {
        if (remaining & maps[i].mask) {
            if (!first) result += "|";
            result += maps[i].name;
            remaining &= ~maps[i].mask;
            first = 0;
        }
    }

    // If there are unmatched flags, append them as hex
    if (remaining != 0) {
        char buf[32];
        snprintf(buf, sizeof(buf), "|0x%lx", remaining);
        result += buf;
    }

    return result.empty() ? "none" : result;
}

/**
 * @brief Decode RSS stat member to string
 *
 * Converts RSS (Resident Set Size) stat member enumeration to human-readable string.
 *
 * @param member RSS stat member value (0-3)
 * @return String representation of RSS stat member
 */
std::string TraceEvent::decode_rss_stat_member(int member) {
    switch (member) {
        case 0: return "MM_FILEPAGES";   // File-backed pages
        case 1: return "MM_ANONPAGES";   // Anonymous pages
        case 2: return "MM_SWAPENTS";    // Swap entries
        case 3: return "MM_SHMEMPAGES";  // Shared memory pages
        default: return "UNKNOWN";
    }
}

/**
 * @brief Decode LRU type to string
 */
std::string TraceEvent::decode_lru_type(int lru) {
    switch (lru) {
        case 0: return "inactive_anon";
        case 1: return "active_anon";
        case 2: return "inactive_file";
        case 3: return "active_file";
        case 4: return "unevictable";
        default: return "unknown";
    }
}

/**
 * @brief Decode reclaim flags to string
 */
std::string TraceEvent::decode_reclaim_flags(unsigned int flags) {
    if (flags == 0) return "RECLAIM_WB_NONE";

    struct flag_map {
        unsigned int mask;
        const char *name;
    };

    struct flag_map maps[] = {
        { 0x0001u, "RECLAIM_WB_ANON" },
        { 0x0002u, "RECLAIM_WB_FILE" },
        { 0x0010u, "RECLAIM_WB_MIXED" },
        { 0x0004u, "RECLAIM_WB_SYNC" },
        { 0x0008u, "RECLAIM_WB_ASYNC" },
    };

    std::string result;
    int first = 1;
    for (int i = 0; i < 5; i++) {
        if (flags & maps[i].mask) {
            if (!first) result += "|";
            result += maps[i].name;
            first = 0;
        }
    }

    return result.empty() ? "RECLAIM_WB_NONE" : result;
}

/**
 * @brief Decode compaction status to string
 */
std::string TraceEvent::decode_compaction_status(int status) {
    switch (status) {
        case 0: return "not_suitable_zone";
        case 1: return "skipped";
        case 2: return "deferred";
        case 3: return "no_suitable_page";
        case 4: return "continue";
        case 5: return "complete";
        case 6: return "partial_skipped";
        case 7: return "contended";
        case 8: return "success";
        default: return "unknown";
    }
}

/**
 * @brief Decode zone type to string
 */
std::string TraceEvent::decode_zone_type(int idx) {
    switch (idx) {
        case 0: return "DMA32";
        case 1: return "Normal";
        case 2: return "Movable";
        default: return "Unknown";
    }
}

/**
 * @brief Decode migrate mode to string
 */
std::string TraceEvent::decode_migrate_mode(int mode) {
    switch (mode) {
        case 0: return "MIGRATE_ASYNC";
        case 1: return "MIGRATE_SYNC_LIGHT";
        case 2: return "MIGRATE_SYNC";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Decode migrate reason to string
 */
std::string TraceEvent::decode_migrate_reason(int reason) {
    switch (reason) {
        case 0: return "compaction";
        case 1: return "memory_failure";
        case 2: return "memory_hotplug";
        case 3: return "syscall_or_cpuset";
        case 4: return "mempolicy_mbind";
        case 5: return "numa_misplaced";
        case 6: return "contig_range";
        case 7: return "longterm_pin";
        case 8: return "demotion";
        default: return "unknown";
    }
}

/**
 * @brief Decode khugepaged status to string
 */
std::string TraceEvent::decode_khugepaged_status(int status) {
    switch (status) {
        case 0: return "failed";
        case 1: return "succeeded";
        case 2: return "pmd_null";
        case 3: return "exceed_none_pte";
        case 4: return "exceed_swap_pte";
        case 5: return "exceed_shared_pte";
        case 6: return "pte_non_present";
        case 7: return "pte_uffd_wp";
        case 8: return "no_writable_page";
        case 9: return "lack_referenced_page";
        case 10: return "page_null";
        case 11: return "scan_aborted";
        case 12: return "not_suitable_page_count";
        case 13: return "page_not_in_lru";
        case 14: return "page_locked";
        case 15: return "page_not_anon";
        case 16: return "page_compound";
        case 17: return "no_process_for_page";
        case 18: return "vma_null";
        case 19: return "vma_check_failed";
        case 20: return "not_suitable_address_range";
        case 21: return "page_swap_cache";
        case 22: return "could_not_delete_page_from_lru";
        case 23: return "alloc_huge_page_failed";
        case 24: return "ccgroup_charge_failed";
        case 25: return "truncated";
        case 26: return "page_has_private";
        default: return "unknown";
    }
}

/**
 * @brief Decode inode state flags to string
 */
std::string TraceEvent::decode_inode_state(unsigned long state) {
    struct flag_map {
        unsigned long mask;
        const char *name;
    };

    struct flag_map maps[] = {
        { 1 << 0, "I_DIRTY_SYNC" },
        { 1 << 1, "I_DIRTY_DATASYNC" },
        { 1 << 2, "I_DIRTY_PAGES" },
        { 1 << 3, "I_NEW" },
        { 1 << 4, "I_WILL_FREE" },
        { 1 << 5, "I_FREEING" },
        { 1 << 6, "I_CLEAR" },
        { 1 << 7, "I_SYNC" },
        { 1 << 8, "I_REFERENCED" },
        { 1 << 11, "I_DIRTY_TIME" },
    };

    std::string result;
    int first = 1;
    for (int i = 0; i < 10; i++) {
        if (state & maps[i].mask) {
            if (!first) result += "|";
            result += maps[i].name;
            first = 0;
        }
    }

    return result.empty() ? "0" : result;
}

/**
 * @brief Decode writeback reason to string
 */
std::string TraceEvent::decode_writeback_reason(int reason) {
    switch (reason) {
        case 0: return "background";
        case 1: return "vmscan";
        case 2: return "sync";
        case 3: return "periodic";
        case 4: return "laptop_timer";
        case 5: return "fs_free_space";
        case 6: return "forker_thread";
        case 7: return "foreign_flush";
        default: return "unknown";
    }
}

/**
 * @brief Decode SCSI protection operation to string
 */
std::string TraceEvent::decode_scsi_prot_op(int prot_op) {
    switch (prot_op) {
        case 0: return "SCSI_PROT_NORMAL";
        case 1: return "SCSI_PROT_READ_INSERT";
        case 2: return "SCSI_PROT_WRITE_STRIP";
        case 3: return "SCSI_PROT_READ_STRIP";
        case 4: return "SCSI_PROT_WRITE_INSERT";
        case 5: return "SCSI_PROT_READ_PASS";
        case 6: return "SCSI_PROT_WRITE_PASS";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Decode SCSI opcode to string
 */
std::string TraceEvent::decode_scsi_opcode(unsigned char opcode) {
    switch (opcode) {
        case 0x00: return "TEST_UNIT_READY";
        case 0x01: return "REZERO_UNIT";
        case 0x03: return "REQUEST_SENSE";
        case 0x04: return "FORMAT_UNIT";
        case 0x05: return "READ_BLOCK_LIMITS";
        case 0x07: return "REASSIGN_BLOCKS";
        case 0x08: return "READ_6";
        case 0x09: return "READ_32";
        case 0x0a: return "WRITE_6";
        case 0x0b: return "SEEK_6";
        case 0x0d: return "WRITE_SAME_32";
        case 0x0f: return "READ_REVERSE";
        case 0x10: return "WRITE_FILEMARKS";
        case 0x11: return "SPACE";
        case 0x12: return "INQUIRY";
        case 0x14: return "RECOVER_BUFFERED_DATA";
        case 0x15: return "MODE_SELECT";
        case 0x16: return "RESERVE";
        case 0x17: return "RELEASE";
        case 0x18: return "COPY";
        case 0x19: return "ERASE";
        case 0x1a: return "MODE_SENSE";
        case 0x1b: return "START_STOP";
        case 0x1c: return "RECEIVE_DIAGNOSTIC";
        case 0x1d: return "SEND_DIAGNOSTIC";
        case 0x1e: return "ALLOW_MEDIUM_REMOVAL";
        case 0x24: return "SET_WINDOW";
        case 0x25: return "READ_CAPACITY";
        case 0x28: return "READ_10";
        case 0x2a: return "WRITE_10";
        case 0x2b: return "SEEK_10";
        case 0x2e: return "WRITE_VERIFY";
        case 0x2f: return "VERIFY";
        case 0x30: return "SEARCH_HIGH";
        case 0x31: return "SEARCH_EQUAL";
        case 0x32: return "SEARCH_LOW";
        case 0x33: return "SET_LIMITS";
        case 0x34: return "PRE_FETCH";
        case 0x35: return "SYNCHRONIZE_CACHE";
        case 0x36: return "LOCK_UNLOCK_CACHE";
        case 0x37: return "READ_DEFECT_DATA";
        case 0x38: return "MEDIUM_SCAN";
        case 0x39: return "COMPARE";
        case 0x3a: return "COPY_VERIFY";
        case 0x3b: return "WRITE_BUFFER";
        case 0x3c: return "READ_BUFFER";
        case 0x3d: return "UPDATE_BLOCK";
        case 0x3e: return "READ_LONG";
        case 0x3f: return "WRITE_LONG";
        case 0x40: return "CHANGE_DEFINITION";
        case 0x41: return "WRITE_SAME";
        case 0x42: return "UNMAP";
        case 0x43: return "READ_TOC";
        case 0x4c: return "LOG_SELECT";
        case 0x4d: return "LOG_SENSE";
        case 0x53: return "XDWRITEREAD_10";
        case 0x55: return "MODE_SELECT_10";
        case 0x56: return "RESERVE_10";
        case 0x57: return "RELEASE_10";
        case 0x5a: return "MODE_SENSE_10";
        case 0x5e: return "PERSISTENT_RESERVE_IN";
        case 0x5f: return "PERSISTENT_RESERVE_OUT";
        case 0x7f: return "VARIABLE_LENGTH_CMD";
        case 0x85: return "ATA_16";
        case 0x88: return "READ_16";
        case 0x8a: return "WRITE_16";
        case 0x8f: return "VERIFY_16";
        case 0x93: return "WRITE_SAME_16";
        case 0x94: return "ZBC_OUT";
        case 0x95: return "ZBC_IN";
        case 0x9e: return "SERVICE_ACTION_IN_16";
        case 0xa0: return "REPORT_LUNS";
        case 0xa1: return "ATA_12";
        case 0xa3: return "MAINTENANCE_IN";
        case 0xa4: return "MAINTENANCE_OUT";
        case 0xa5: return "MOVE_MEDIUM";
        case 0xa6: return "EXCHANGE_MEDIUM";
        case 0xa8: return "READ_12";
        case 0xaa: return "WRITE_12";
        case 0xae: return "WRITE_VERIFY_12";
        case 0xb0: return "SEARCH_HIGH_12";
        case 0xb1: return "SEARCH_EQUAL_12";
        case 0xb2: return "SEARCH_LOW_12";
        case 0xb6: return "SEND_VOLUME_TAG";
        case 0xb8: return "READ_ELEMENT_STATUS";
        case 0xea: return "WRITE_LONG_2";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Decode SCSI host status to string
 */
std::string TraceEvent::decode_scsi_host_status(unsigned char host_byte) {
    switch (host_byte) {
        case 0x00: return "DID_OK";
        case 0x01: return "DID_NO_CONNECT";
        case 0x02: return "DID_BUS_BUSY";
        case 0x03: return "DID_TIME_OUT";
        case 0x04: return "DID_BAD_TARGET";
        case 0x05: return "DID_ABORT";
        case 0x06: return "DID_PARITY";
        case 0x07: return "DID_ERROR";
        case 0x08: return "DID_RESET";
        case 0x09: return "DID_BAD_INTR";
        case 0x0a: return "DID_PASSTHROUGH";
        case 0x0b: return "DID_SOFT_ERROR";
        case 0x0c: return "DID_IMM_RETRY";
        case 0x0d: return "DID_REQUEUE";
        case 0x0e: return "DID_TRANSPORT_DISRUPTED";
        case 0x0f: return "DID_TRANSPORT_FAILFAST";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Decode SCSI SAM status to string
 */
std::string TraceEvent::decode_scsi_sam_status(unsigned char status_byte) {
    switch (status_byte) {
        case 0x00: return "SAM_STAT_GOOD";
        case 0x02: return "SAM_STAT_CHECK_CONDITION";
        case 0x04: return "SAM_STAT_CONDITION_MET";
        case 0x08: return "SAM_STAT_BUSY";
        case 0x10: return "SAM_STAT_INTERMEDIATE";
        case 0x14: return "SAM_STAT_INTERMEDIATE_CONDITION_MET";
        case 0x18: return "SAM_STAT_RESERVATION_CONFLICT";
        case 0x22: return "SAM_STAT_COMMAND_TERMINATED";
        case 0x28: return "SAM_STAT_TASK_SET_FULL";
        case 0x30: return "SAM_STAT_ACA_ACTIVE";
        case 0x40: return "SAM_STAT_TASK_ABORTED";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Parse SCSI CDB (Command Descriptor Block) to human-readable string
 */
std::string TraceEvent::scsi_trace_parse_cdb(const unsigned char *cdb, int len) {
    if (!cdb || len <= 0) return "";

    std::string result;
    unsigned char opcode = cdb[0];

    // Add opcode name
    result = decode_scsi_opcode(opcode);

    // Add basic CDB parameters based on opcode
    switch (opcode) {
        case 0x08: // READ_6
        case 0x0a: // WRITE_6
            if (len >= 4) {
                unsigned int lba = ((cdb[1] & 0x1f) << 16) | (cdb[2] << 8) | cdb[3];
                unsigned int blocks = cdb[4];
                char buf[64];
                snprintf(buf, sizeof(buf), " lba=%u blocks=%u", lba, blocks);
                result += buf;
            }
            break;

        case 0x28: // READ_10
        case 0x2a: // WRITE_10
            if (len >= 8) {
                unsigned int lba = (cdb[2] << 24) | (cdb[3] << 16) | (cdb[4] << 8) | cdb[5];
                unsigned int blocks = (cdb[7] << 8) | cdb[8];
                char buf[64];
                snprintf(buf, sizeof(buf), " lba=%u blocks=%u", lba, blocks);
                result += buf;
            }
            break;

        case 0x88: // READ_16
        case 0x8a: // WRITE_16
            if (len >= 14) {
                unsigned long long lba = ((unsigned long long)cdb[2] << 56) |
                                        ((unsigned long long)cdb[3] << 48) |
                                        ((unsigned long long)cdb[4] << 40) |
                                        ((unsigned long long)cdb[5] << 32) |
                                        ((unsigned long long)cdb[6] << 24) |
                                        ((unsigned long long)cdb[7] << 16) |
                                        ((unsigned long long)cdb[8] << 8) |
                                        (unsigned long long)cdb[9];
                unsigned int blocks = (cdb[10] << 24) | (cdb[11] << 16) | (cdb[12] << 8) | cdb[13];
                char buf[96];
                snprintf(buf, sizeof(buf), " lba=%llu blocks=%u", lba, blocks);
                result += buf;
            }
            break;
    }

    return result;
}

/**
 * @brief Format byte array as hex string
 */
std::string TraceEvent::format_hex_string(const unsigned char *data, int len) {
    if (!data || len <= 0) return "";

    std::string result;
    char buf[4];
    for (int i = 0; i < len && i < 32; i++) {  // Limit to 32 bytes
        snprintf(buf, sizeof(buf), "%02x", data[i]);
        result += buf;
        if (i < len - 1) result += " ";
    }
    if (len > 32) result += "...";

    return result;
}

/**
 * @brief Decode memory type to string
 */
std::string TraceEvent::decode_mem_type(int mem_type) {
    switch (mem_type) {
        case 0: return "PAGE_SHARED";
        case 1: return "PAGE_ORDER0";
        case 2: return "PAGE_POOL";
        case 3: return "XSK_BUFF_POOL";
        case -1: return "NONE";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Decode compaction priority to string
 */
std::string TraceEvent::decode_compact_priority(int priority) {
    switch (priority) {
        case 0: return "COMPACT_PRIO_SYNC_FULL";
        case 1: return "COMPACT_PRIO_SYNC_LIGHT";
        case 2: return "COMPACT_PRIO_ASYNC";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Decode compaction result to string
 */
std::string TraceEvent::decode_compact_result(int result) {
    switch (result) {
        case 1: return "failed";
        case 2: return "withdrawn";
        case 3: return "progress";
        default: return "unknown";
    }
}

/**
 * @brief Decode file lock flags to string
 */
std::string TraceEvent::decode_file_lock_flags(unsigned int flags) {
    struct flag_map {
        unsigned int mask;
        const char *name;
    };

    struct flag_map maps[] = {
        { 1, "FL_POSIX" },
        { 2, "FL_FLOCK" },
        { 4, "FL_DELEG" },
        { 8, "FL_ACCESS" },
        { 16, "FL_EXISTS" },
        { 32, "FL_LEASE" },
        { 64, "FL_CLOSE" },
        { 128, "FL_SLEEP" },
        { 256, "FL_DOWNGRADE_PENDING" },
        { 512, "FL_UNLOCK_PENDING" },
        { 1024, "FL_OFDLCK" },
    };

    std::string result;
    int first = 1;
    for (int i = 0; i < 11; i++) {
        if (flags & maps[i].mask) {
            if (!first) result += "|";
            result += maps[i].name;
            first = 0;
        }
    }

    return result.empty() ? "0" : result;
}

/**
 * @brief Decode file lock type to string
 */
std::string TraceEvent::decode_file_lock_type(unsigned char type) {
    switch (type) {
        case 0: return "F_RDLCK";
        case 1: return "F_WRLCK";
        case 2: return "F_UNLCK";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Decode iomap type to string
 */
std::string TraceEvent::decode_iomap_type(int type) {
    switch (type) {
        case 0: return "HOLE";
        case 1: return "DELALLOC";
        case 2: return "MAPPED";
        case 3: return "UNWRITTEN";
        case 4: return "INLINE";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Decode iomap flags to string
 */
std::string TraceEvent::decode_iomap_flags(unsigned int flags) {
    struct flag_map {
        unsigned int mask;
        const char *name;
    };

    struct flag_map maps[] = {
        { 0x01, "NEW" },
        { 0x02, "DIRTY" },
        { 0x04, "SHARED" },
        { 0x08, "MERGED" },
        { 0x10, "BH" },
        { 0x100, "SIZE_CHANGED" },
    };

    std::string result;
    int first = 1;
    for (int i = 0; i < 6; i++) {
        if (flags & maps[i].mask) {
            if (!first) result += "|";
            result += maps[i].name;
            first = 0;
        }
    }

    return result.empty() ? "0" : result;
}

/**
 * @brief Decode iomap iter flags to string
 */
std::string TraceEvent::decode_iomap_iter_flags(unsigned int flags) {
    struct flag_map {
        unsigned int mask;
        const char *name;
    };

    struct flag_map maps[] = {
        { (1 << 0), "WRITE" },
        { (1 << 1), "ZERO" },
        { (1 << 2), "REPORT" },
        { (1 << 3), "FAULT" },
        { (1 << 4), "DIRECT" },
        { (1 << 5), "NOWAIT" },
    };

    std::string result;
    int first = 1;
    for (int i = 0; i < 6; i++) {
        if (flags & maps[i].mask) {
            if (!first) result += "|";
            result += maps[i].name;
            first = 0;
        }
    }

    return result.empty() ? "0" : result;
}

/**
 * @brief Print softirq event information
 *
 * Formats and outputs softirq event data including vector number and type.
 *
 * @param addr Address of the event data in memory
 */
void TraceEvent::print_softirq(ulong addr) {
    arg_list[0]->name = "vec";
    read_trace_field(addr, arg_list[0]);

    if (arg_list[1]) {
        unsigned int vec = *reinterpret_cast<unsigned int*>(arg_list[0]->data);
        copy_str(arg_list[1], decode_softirq_vec(vec));
    }
}

/**
 * @brief Read and format a trace event field value
 *
 * Reads and formats a trace event field based on its type. Handles various
 * data types including strings, integers, pointers, and special types like
 * __data_loc (dynamically located data). This is the core function for
 * formatting individual field values in trace events.
 *
 * @param addr Address of the event data in memory
 * @param arg_ptr Shared pointer to print_arg structure to store the result
 */
void TraceEvent::read_trace_field(ulong addr, std::shared_ptr<print_arg> arg_ptr) {
    if (!arg_ptr || arg_ptr->name.empty()) {
        return;
    }

    // Look up field definition
    std::shared_ptr<trace_field> field_ptr = field_maps[arg_ptr->name];
    if (!field_ptr) {
        return;
    }

    // Initialize data buffer to zeros to avoid garbage data
    memset(arg_ptr->data, 0, sizeof(arg_ptr->data));

    // Handle __data_loc dynamically located string data
    if (field_ptr->type.find("__data_loc") != std::string::npos &&
        field_ptr->type.find("char[]") != std::string::npos) {
        uint temp = plugin_ptr->read_uint(addr + field_ptr->offset, arg_ptr->name);
        int len = (temp >> 16);  // Length in upper 16 bits

        if (len > 0) {
            std::string tempval;
            if (len == 1) {
                tempval = plugin_ptr->read_cstring(addr, field_ptr->offset * 4, arg_ptr->name);
            } else {
                tempval = plugin_ptr->read_cstring(addr + (temp & 0xffff), len, arg_ptr->name);
            }
            copy_str(arg_ptr, tempval);
        }
        return;
    }

    // Handle fixed-size character arrays
    if (field_ptr->type.find("char[]") != std::string::npos) {
        std::string tempval = plugin_ptr->read_cstring(addr + field_ptr->offset, 512, arg_ptr->name);
        copy_str(arg_ptr, tempval);
        return;
    }

    // Handle const char * pointers
    if (field_ptr->type.find("const char *") != std::string::npos) {
        ulong str_addr = plugin_ptr->read_pointer(addr + field_ptr->offset, arg_ptr->name);
        std::string tempval = plugin_ptr->read_cstring(str_addr, 64, arg_ptr->name);
        copy_str(arg_ptr, tempval);
        return;
    }

    // Handle sized character arrays like char[16] or char[TASK_COMM_LEN]
    if (field_ptr->type.find("char[") == 0 && field_ptr->type.find(']') != std::string::npos) {
        std::string tempval = plugin_ptr->read_cstring(addr + field_ptr->offset, field_ptr->size, arg_ptr->name);
        copy_str(arg_ptr, tempval);
        return;
    }

    // Handle signed integers
    if (field_ptr->type == "int" || field_ptr->type == "pid_t") {
        int val = plugin_ptr->read_int(addr + field_ptr->offset, arg_ptr->name);
        *reinterpret_cast<int*>(arg_ptr->data) = val;
        return;
    }

    // Handle long integers
    if (field_ptr->type == "long") {
        long val = plugin_ptr->read_long(addr + field_ptr->offset, arg_ptr->name);
        *reinterpret_cast<long*>(arg_ptr->data) = val;
        return;
    }

    // Handle unsigned long and pointer types
    if (field_ptr->type == "unsigned long" ||
        field_ptr->type == "void *" ||
        field_ptr->type == "size_t" ||
        (field_ptr->type.find("struct") != std::string::npos &&
         field_ptr->type.find("*") != std::string::npos)) {
        ulong tempval = plugin_ptr->read_ulong(addr + field_ptr->offset, arg_ptr->name);

        // Special handling for function pointers - try to resolve to symbol
        if (arg_ptr->name == "function" && field_ptr->type == "void *") {
            if (is_kvaddr(tempval)) {
                ulong offset;
                struct syment *sp = value_search(tempval, &offset);
                if (sp) {
                    strncpy(arg_ptr->data, sp->name, 64);
                    arg_ptr->data[64] = '\0';
                    return;
                }
            }
        }
        *reinterpret_cast<ulong*>(arg_ptr->data) = tempval;
        return;
    }

    // Handle 32-bit unsigned integers
    if (field_ptr->type == "__u32" ||
        field_ptr->type == "u32" ||
        field_ptr->type == "unsigned" ||
        field_ptr->type == "gfp_t" ||
        field_ptr->type == "uint32_t" ||
        field_ptr->type == "unsigned int") {
        uint val = plugin_ptr->read_uint(addr + field_ptr->offset, arg_ptr->name);
        *reinterpret_cast<uint*>(arg_ptr->data) = val;
        return;
    }

    // Handle 64-bit unsigned integers
    if (field_ptr->type == "__u64" ||
        field_ptr->type == "u64" ||
        field_ptr->type == "uint64_t" ||
        field_ptr->type == "initcall_t" ||
        field_ptr->type == "s64") {
        ulonglong val = plugin_ptr->read_ulonglong(addr + field_ptr->offset, arg_ptr->name);
        *reinterpret_cast<ulonglong*>(arg_ptr->data) = val;
        return;
    }

    // Handle boolean values
    if (field_ptr->type == "bool") {
        bool val = plugin_ptr->read_bool(addr + field_ptr->offset, arg_ptr->name);
        *reinterpret_cast<bool*>(arg_ptr->data) = val;
        return;
    }

    // Handle 8-bit unsigned integers
    if (field_ptr->type == "u8") {
        uint8_t val = plugin_ptr->read_byte(addr + field_ptr->offset, arg_ptr->name);
        *reinterpret_cast<uint8_t*>(arg_ptr->data) = val;
        return;
    }
}

/**
 * @brief Safely copy a string to the print argument data buffer
 *
 * Copies a string into the print_arg data buffer with proper bounds checking
 * and null termination. Ensures the string fits within the buffer size and
 * is always null-terminated to prevent buffer overflows and undefined behavior.
 *
 * @param arg_ptr Shared pointer to print_arg structure containing the destination buffer
 * @param data Source string to copy
 */
void TraceEvent::copy_str(std::shared_ptr<print_arg> arg_ptr, const std::string& data) {
    if (!arg_ptr) {
        return;
    }

    // Calculate the maximum number of characters we can safely copy
    // Reserve one byte for null terminator
    size_t max_copy_len = sizeof(arg_ptr->data) - 1;
    size_t copy_len = std::min(data.size(), max_copy_len);

    // Copy the string data
    if (copy_len > 0) {
        memcpy(arg_ptr->data, data.c_str(), copy_len);
    }

    // Always null-terminate the string
    arg_ptr->data[copy_len] = '\0';
}

/**
 * @brief Handle trace event processing
 *
 * Main entry point for processing a trace event. Reads all field values
 * from memory and stores them in the argument list for later formatting.
 *
 * @param addr Address of the event data in memory
 */
void TraceEvent::handle(ulong addr) {
    if (skiped) {
        has_parserd = false;
    }else{
        // Read each print argument value
        for (const auto& arg_ptr : arg_list) {
            if (field_maps.find(arg_ptr->name) == field_maps.end()) {
                continue;  // Field not found in field map
            }
            read_trace_field(addr, arg_ptr);
        }
    }
}

/**
 * @brief Print formatted log output
 *
 * Formats and outputs all arguments according to their format specifiers.
 *
 * @param oss Output string stream to write formatted data to
 */
void TraceEvent::print_log(std::ostringstream& oss) {
    if (!has_parserd){
        oss << "****Parser Failed****";
    }else{
        for (const auto& arg_ptr : arg_list) {
            oss << format_arg(arg_ptr);
        }
    }
}

/**
 * @brief Format a trace event argument according to its format specifier
 *
 * This function takes a print argument with format specifier (e.g., "%d", "%x", "%s")
 * and data stored in a char[512] buffer, then formats it into a string. The function:
 * 1. Constructs the full format string (prefix + format + suffix)
 * 2. Extracts the format type character (d, x, s, etc.)
 * 3. Detects length modifiers (l, ll)
 * 4. Interprets the data buffer according to the format type
 * 5. Uses snprintf to format the value
 *
 * Data storage in the char[512] buffer:
 * - Integers: Stored in the first bytes using reinterpret_cast (e.g., *reinterpret_cast<int*>(data))
 * - Strings: Stored as null-terminated C strings directly in the buffer
 * - Floats: Stored in the first bytes as double values
 *
 * Supported format specifiers:
 * - %d, %i: Signed decimal integer (with optional l, ll modifiers)
 * - %u: Unsigned decimal integer (with optional l, ll modifiers)
 * - %x, %X: Hexadecimal (lowercase/uppercase, with optional l, ll modifiers)
 * - %o: Octal (with optional l, ll modifiers)
 * - %f, %F, %e, %E, %g, %G: Floating point (data buffer contains double)
 * - %s: String (data buffer contains null-terminated string)
 * - %c: Character
 * - %p: Pointer address
 *
 * @param arg_ptr Shared pointer to print_arg containing:
 *                - prefix: Text before format specifier
 *                - format: Printf-style format specifier
 *                - suffix: Text after format specifier
 *                - name: Field name
 *                - data: char[512] buffer containing the value
 * @return Formatted string with prefix, formatted value, and suffix
 */
std::string TraceEvent::format_arg(std::shared_ptr<print_arg> arg_ptr) {
    if (!arg_ptr) {
        return "";
    }

    char buffer[512];

    // Build the complete format string (prefix + format specifier + suffix)
    std::string full_format = arg_ptr->prefix + arg_ptr->format;
    if (!arg_ptr->suffix.empty()) {
        full_format += arg_ptr->suffix;
    }

    // Validate format string - must start with '%'
    const std::string& fmt = arg_ptr->format;
    if (fmt.empty() || fmt[0] != '%') {
        return full_format;  // No format specifier, return as-is
    }

    // Extract the format type character (the last character of the format specifier)
    char format_type = '\0';
    bool has_long = false;       // true if format has 'l' modifier (e.g., %ld)
    bool has_long_long = false;  // true if format has 'll' modifier (e.g., %lld)
    size_t type_pos = std::string::npos;

    // Scan backwards to find the conversion specifier
    for (size_t i = fmt.length() - 1; i > 0; i--) {
        char ch = fmt[i];
        if (strchr("diouxXeEfFgGaAcspn", ch) != nullptr) {
            format_type = ch;
            type_pos = i;
            break;
        }
    }

    // No valid format type found
    if (type_pos == std::string::npos) {
        return full_format;
    }

    // Check for length modifiers before the format type
    if (type_pos >= 2 && fmt[type_pos - 1] == 'l') {
        if (type_pos >= 3 && fmt[type_pos - 2] == 'l') {
            has_long_long = true;  // %lld, %llu, %llx, etc.
        } else {
            has_long = true;  // %ld, %lu, %lx, etc.
        }
    }

    // Convert data based on format type and format the output
    switch (format_type) {
        case 'd':  // Signed decimal integer
        case 'i': {
            if (has_long_long) {
                long long value = *reinterpret_cast<long long*>(arg_ptr->data);
                snprintf(buffer, sizeof(buffer), full_format.c_str(), value);
            } else if (has_long) {
                long value = *reinterpret_cast<long*>(arg_ptr->data);
                snprintf(buffer, sizeof(buffer), full_format.c_str(), value);
            } else {
                int value = *reinterpret_cast<int*>(arg_ptr->data);
                snprintf(buffer, sizeof(buffer), full_format.c_str(), value);
            }
            break;
        }

        case 'u':  // Unsigned decimal integer
        case 'x':  // Hexadecimal lowercase
        case 'X':  // Hexadecimal uppercase
        case 'o': {  // Octal
            if (has_long_long) {
                unsigned long long value = *reinterpret_cast<unsigned long long*>(arg_ptr->data);
                snprintf(buffer, sizeof(buffer), full_format.c_str(), value);
            } else if (has_long) {
                unsigned long value = *reinterpret_cast<unsigned long*>(arg_ptr->data);
                snprintf(buffer, sizeof(buffer), full_format.c_str(), value);
            } else {
                unsigned int value = *reinterpret_cast<unsigned int*>(arg_ptr->data);
                snprintf(buffer, sizeof(buffer), full_format.c_str(), value);
            }
            break;
        }

        case 'f':  // Floating point (lowercase)
        case 'F':  // Floating point (uppercase)
        case 'e':  // Scientific notation (lowercase)
        case 'E':  // Scientific notation (uppercase)
        case 'g':  // Shortest representation (lowercase)
        case 'G': {  // Shortest representation (uppercase)
            double value = *reinterpret_cast<const double*>(arg_ptr->data);
            snprintf(buffer, sizeof(buffer), full_format.c_str(), value);
            break;
        }

        case 's': {  // String
            snprintf(buffer, sizeof(buffer), full_format.c_str(), arg_ptr->data);
            break;
        }

        case 'c': {  // Character
            char value = *reinterpret_cast<char*>(arg_ptr->data);
            snprintf(buffer, sizeof(buffer), full_format.c_str(), value);
            break;
        }

        case 'p': {  // Pointer address and extensions
            // Check for %pS (symbol) format
            if (fmt.length() >= 3 && fmt.substr(fmt.length()-2) == "pS") {
                ulong addr_value = *reinterpret_cast<ulong*>(arg_ptr->data);
                std::string symbol = plugin_ptr->to_symbol(addr_value);
                if (!symbol.empty()) {
                    // Replace %pS with %s and use symbol name
                    std::string modified_format = arg_ptr->prefix + "%s";
                    if (!arg_ptr->suffix.empty()) {
                        modified_format += arg_ptr->suffix;
                    }
                    snprintf(buffer, sizeof(buffer), modified_format.c_str(), symbol.c_str());
                } else {
                    // Fallback to address if symbol not found
                    void* value = *reinterpret_cast<void**>(arg_ptr->data);
                    snprintf(buffer, sizeof(buffer), full_format.c_str(), value);
                }
            } else {
                // Standard %p format
                void* value = *reinterpret_cast<void**>(arg_ptr->data);
                snprintf(buffer, sizeof(buffer), full_format.c_str(), value);
            }
            break;
        }

        default: {
            // Unknown format type - treat as integer by default
            int value = *reinterpret_cast<int*>(arg_ptr->data);
            snprintf(buffer, sizeof(buffer), full_format.c_str(), value);
            break;
        }
    }

    return std::string(buffer);
}

/**
 * @brief Set the print format string for this event
 *
 * Sets the original print format string and parses it to extract
 * formatting information for event display.
 *
 * @param format The kernel's print format string
 */
void TraceEvent::set_print_format(std::string format) {
    org_print_fmt = format;
    if (!format.empty()) {
        parser_output_format(format);
    }
}

/**
 * @brief Parse the output format string
 *
 * Analyzes the format string to determine if it contains advanced features
 * that cannot be easily parsed. If not, extracts field names and format
 * specifications for proper event formatting.
 *
 * @param format The format string to parse
 */
void TraceEvent::parser_output_format(std::string format) {
    // Extract the format string from the full format specification
    print_fmt = extractFormatString(format);

    // Remove newlines and carriage returns from format string
    print_fmt.erase(std::remove_if(print_fmt.begin(), print_fmt.end(),
        [](char c) { return c == '\n' || c == '\r'; }),
        print_fmt.end());

    // Parse format string into prefixes and format specifiers
    arg_list = parseFormatString(print_fmt);

    if (has_advanced_features()) {
        skiped = true;
    } else {
        // Extract field names from the format specification
        std::vector<std::string> name_list = extract_field_names(format);

        // Create print arguments for each field
        for (size_t i = 0; i < arg_list.size() && i < name_list.size(); ++i) {
            arg_list[i]->name = name_list[i];
        }
    }
}

/**
 * @brief Parse format string into tokens with prefixes and format specifiers
 *
 * Parses a printf-style format string and extracts each format specifier along
 * with its prefix text. For example, "pid=%d comm=%s" produces:
 * - Token 1: prefix="pid=", format="%d"
 * - Token 2: prefix=" comm=", format="%s"
 *
 * Supports standard C format specifiers including:
 * - Length modifiers: h, hh, l, ll, L, z, j, t
 * - Flags: -, +, 0, space, #
 * - Width and precision: digits and .
 * - Conversion specifiers: d, i, o, u, x, X, e, E, f, F, g, G, a, A, c, s, p, n, %
 * - Pointer extensions: %pK, %px, %pI4, etc.
 *
 * @param input The format string to parse
 * @return Vector of print_arg structures containing prefix, format, and optional suffix
 */
std::vector<std::shared_ptr<print_arg>> TraceEvent::parseFormatString(const std::string& input) {
    std::vector<std::shared_ptr<print_arg>> tokens;
    size_t i = 0;

    while (i < input.length()) {
        auto token = std::make_shared<print_arg>();

        // Find the next format specifier (starts with %)
        size_t percentPos = input.find('%', i);

        // No more format specifiers found
        if (percentPos == std::string::npos) {
            // Remaining text becomes suffix of the last token
            if (i < input.length() && !tokens.empty()) {
                tokens.back()->suffix = input.substr(i);
            }
            break;
        }

        // Extract prefix text (everything before the %)
        token->prefix = input.substr(i, percentPos - i);

        // Start parsing the format specifier
        i = percentPos + 1;  // Skip the '%'

        // Skip flags: -, +, 0, space, #
        while (i < input.length() &&
               (input[i] == '-' || input[i] == '+' || input[i] == '0' ||
                input[i] == ' ' || input[i] == '#' || std::isdigit(input[i]))) {
            i++;
        }

        // Skip precision: .digits
        if (i < input.length() && input[i] == '.') {
            i++;
            while (i < input.length() && std::isdigit(input[i])) {
                i++;
            }
        }

        // Parse length modifiers: h, hh, l, ll, L, z, j, t
        if (i < input.length()) {
            if (input[i] == 'h' || input[i] == 'l') {
                i++;
                // Check for double length modifiers (hh or ll)
                if (i < input.length() && input[i] == input[i-1]) {
                    i++;
                }
            } else if (input[i] == 'L' || input[i] == 'z' ||
                       input[i] == 'j' || input[i] == 't') {
                i++;
            }
        }

        // Parse conversion specifier
        if (i < input.length()) {
            if (input[i] == 'p') {
                // Handle pointer format with extensions (e.g., %pK, %px, %pI4)
                i++;
                while (i < input.length() && std::isalnum(input[i])) {
                    i++;
                }
            } else if (std::strchr("diouxXeEfFgGaAcspn%", input[i])) {
                // Standard conversion specifiers
                i++;
            }
        }

        // Extract the complete format specifier (from % to end of specifier)
        token->format = input.substr(percentPos, i - percentPos);
        tokens.push_back(token);
    }
    return tokens;
}

/**
 * @brief Trim whitespace from both ends of a string
 *
 * Removes leading and trailing whitespace characters from a string.
 *
 * @param str The string to trim
 * @return Trimmed string
 */
std::string TraceEvent::trim(const std::string& str) {
    if (str.empty()) return str;

    // Find first non-whitespace character
    auto start = std::find_if_not(str.begin(), str.end(), [](int c) {
        return std::isspace(static_cast<unsigned char>(c));
    });

    // Find last non-whitespace character
    auto end = std::find_if_not(str.rbegin(), str.rend(), [](int c) {
        return std::isspace(static_cast<unsigned char>(c));
    }).base();

    return (start < end) ? std::string(start, end) : std::string();
}

/**
 * @brief Extract field names from a format specification string
 *
 * Parses a complete format specification (e.g., "format string", arg1, arg2, ...)
 * and extracts the field names from the argument list. The format string is
 * skipped, and only the arguments after it are processed.
 * Supports two patterns:
 * 1. REC->field_name (or any_object->field_name)
 * 2. __get_str(field_name)
 *
 * @param input The complete format specification string
 * @return Vector of extracted field names
 */
std::vector<std::string> TraceEvent::extract_field_names(const std::string& input) {
    std::vector<std::string> paramNames;
    std::set<std::string> uniqueParams;
    size_t i = 0;

    while (i < input.length()) {
        // Pattern 1: Look for "->" operator (e.g., REC->field_name)
        if (i + 1 < input.length() && input[i] == '-' && input[i + 1] == '>') {
            i += 2;  // Skip "->"

            // Skip whitespace after "->"
            while (i < input.length() && std::isspace(input[i])) {
                i++;
            }

            // Extract the identifier after "->"
            std::string paramName = extractIdentifier(input, i);

            if (!paramName.empty() && uniqueParams.find(paramName) == uniqueParams.end()) {
                uniqueParams.insert(paramName);
                paramNames.push_back(paramName);
            }
            continue;
        }

        // Pattern 2: Look for "__get_str(" pattern
        if (i + 9 <= input.length() && input.substr(i, 9) == "__get_str") {
            size_t j = i + 9;  // Skip "__get_str"

            // Skip whitespace
            while (j < input.length() && std::isspace(input[j])) {
                j++;
            }

            // Check for opening parenthesis
            if (j < input.length() && input[j] == '(') {
                j++;  // Skip '('

                // Skip whitespace inside parentheses
                while (j < input.length() && std::isspace(input[j])) {
                    j++;
                }

                // Extract the identifier inside parentheses
                std::string paramName = extractIdentifier(input, j);

                if (!paramName.empty() && uniqueParams.find(paramName) == uniqueParams.end()) {
                    uniqueParams.insert(paramName);
                    paramNames.push_back(paramName);
                }

                i = j;
                continue;
            }
        }

        // Skip over parentheses (for type casts and other expressions)
        if (input[i] == '(') {
            size_t j = i + 1;
            int parenDepth = 1;
            while (j < input.length() && parenDepth > 0) {
                if (input[j] == '(') {
                    parenDepth++;
                } else if (input[j] == ')') {
                    parenDepth--;
                }
                j++;
            }
        }

        i++;
    }

    return paramNames;
}

/**
 * @brief Check if a character is valid in an identifier
 *
 * @param c Character to check
 * @return true if character is alphanumeric or underscore
 */
bool TraceEvent::isIdentifierChar(char c) {
    return std::isalnum(c) || c == '_';
}

/**
 * @brief Extract an identifier from a string starting at a given position
 *
 * @param str String to extract from
 * @param pos Position to start extraction (updated to position after identifier)
 * @return Extracted identifier string
 */
std::string TraceEvent::extractIdentifier(const std::string& str, size_t& pos) {
    std::string identifier;
    while (pos < str.length() && isIdentifierChar(str[pos])) {
        identifier += str[pos];
        pos++;
    }
    return identifier;
}

/**
 * @brief Skip opening parentheses and spaces in a string
 *
 * @param str String to process
 * @param pos Position to start skipping (updated to position after skipped characters)
 */
void TraceEvent::skipOpenParensAndSpaces(const std::string& str, size_t& pos) {
    while (pos < str.length()) {
        if (str[pos] == '(' || std::isspace(str[pos])) {
            pos++;
        } else {
            break;
        }
    }
}

/**
 * @brief Check if format string contains advanced features
 *
 * Determines if the print format string contains advanced features that
 * cannot be easily parsed, such as:
 * - Ternary operators (? :)
 * - Dynamic arrays (__get_dynamic_array)
 * - Compiler built-ins (__builtin_constant_p)
 * - Byte swapping functions (__fswab16, __fswab32)
 * - Array indexing (REC->data[0])
 * - Symbolic/flag printing functions
 *
 * Events with advanced features are skipped during parsing.
 *
 * @return true if advanced features detected, false otherwise
 */
bool TraceEvent::has_advanced_features() {
    if ((name == "bprint") || (name == "print") || (name == "bputs")) {
        return true;
    }
    // Check for ternary operator
    if (org_print_fmt.find('?') != std::string::npos &&
        org_print_fmt.find(':') != std::string::npos) {
        return true;
    }

    // Check for dynamic array access
    if (org_print_fmt.find("__get_dynamic_array") != std::string::npos) {
        return true;
    }

    // Check for compiler built-ins
    if (org_print_fmt.find("__builtin_constant_p") != std::string::npos) {
        return true;
    }

    // Check for symbolic/flag printing functions
    if (org_print_fmt.find("__print_symbolic") != std::string::npos ||
        org_print_fmt.find("__print_flags") != std::string::npos) {
        return true;
    }

    // Check for byte swapping functions
    if (org_print_fmt.find("__fswab16") != std::string::npos ||
        org_print_fmt.find("__fswab32") != std::string::npos) {
        return true;
    }

    // Check for array indexing pattern: ->identifier[
    size_t pos = 0;
    while (pos < org_print_fmt.length()) {
        size_t arrow_pos = org_print_fmt.find("->", pos);
        if (arrow_pos == std::string::npos) {
            break;
        }

        // Skip past the "->"
        size_t check_pos = arrow_pos + 2;

        // Skip the identifier after "->"
        while (check_pos < org_print_fmt.length() &&
               (std::isalnum(org_print_fmt[check_pos]) || org_print_fmt[check_pos] == '_')) {
            check_pos++;
        }

        // Check if immediately followed by '[' (array indexing)
        if (check_pos < org_print_fmt.length() && org_print_fmt[check_pos] == '[') {
            return true;
        }

        pos = arrow_pos + 1;
    }

    return false;
}

/**
 * @brief Parse a format specifier from the format string
 *
 * Extracts and analyzes a single format specifier from the current position
 * in the format string. Checks if the specifier contains advanced features
 * that cannot be easily parsed.
 *
 * @return true if advanced features detected in specifier, false otherwise
 */
bool TraceEvent::parse_format_spec() {
    // Handle escaped percent sign
    if (pos_ < org_print_fmt.size() && org_print_fmt[pos_] == '%') {
        pos_++;
        return false;
    }

    // Extract format specifier
    std::string spec;
    while (pos_ < org_print_fmt.size()) {
        char c = org_print_fmt[pos_];

        // Stop at whitespace or special characters
        if (isspace(c) || c == '%' || (!isalnum(c) && c != '.' && c != '-' &&
                                        c != '+' && c != '#' && c != '*' &&
                                        c != '?' && c != ':' && c != '(' &&
                                        c != ')' && c != '>' && c != '<' &&
                                        c != '[' && c != ']' && c != ',')) {
            break;
        }
        spec += c;
        pos_++;
    }

    return contains_advanced_features(spec);
}

/**
 * @brief Check if a format specifier contains advanced features
 *
 * Analyzes a format specifier to detect advanced features such as:
 * - Function calls (parentheses)
 * - Ternary operators (? :)
 * - Pointer dereferencing (->)
 * - Multiplication/dereference operators (*)
 * - Array indexing ([])
 * - Bitwise/arithmetic operators
 *
 * @param spec The format specifier to analyze
 * @return true if advanced features detected, false otherwise
 */
bool TraceEvent::contains_advanced_features(const std::string& spec) {
    // Check for function calls
    if (spec.find('(') != std::string::npos &&
        spec.find(')') != std::string::npos) {
        return true;
    }

    // Check for ternary operator
    if (spec.find('?') != std::string::npos &&
        spec.find(':') != std::string::npos) {
        return true;
    }

    // Check for pointer dereferencing
    if (spec.find("->") != std::string::npos) {
        return true;
    }

    // Check for multiplication/dereference operator
    if (spec.find('*') != std::string::npos) {
        size_t pos = spec.find('*');
        if (pos > 0 && spec[pos-1] != '%' && !isalnum(spec[pos-1])) {
            return true;
        }
    }

    // Check for array indexing
    if (spec.find('[') != std::string::npos &&
        spec.find(']') != std::string::npos) {
        return true;
    }

    // Check for bitwise/arithmetic operators
    static const std::unordered_set<char> operators = {
        '&', '|', '^', '~', '+', '-', '*', '/', '%',
        '<', '>', '=', '!'
    };

    for (char c : spec) {
        if (operators.find(c) != operators.end()) {
            // Allow format specifier flags
            if (c == '+' || c == '-' || c == '#' || c == ' ') {
                continue;
            }
            return true;
        }
    }

    return false;
}

/**
 * @brief Extract the format string from a format specification
 *
 * Extracts the quoted format string from a complete format specification.
 * For example, from '"format %s %d", arg1, arg2' extracts 'format %s %d'.
 *
 * @param input The complete format specification
 * @return The extracted format string (without quotes), or empty string if not found
 */
std::string TraceEvent::extractFormatString(const std::string& input) {
    size_t start = input.find('"');
    if (start == std::string::npos) return "";

    size_t end = input.find('"', start + 1);
    if (end == std::string::npos) return "";

    size_t commaPos = input.find(',', end);
    if (commaPos != std::string::npos && commaPos - end <= 2) {
        return input.substr(start + 1, end - start - 1);
    }
    return "";
}

/**
 * @brief Dump event information for debugging
 *
 * Outputs detailed information about this event including its ID, name,
 * format string, arguments, and field structure for debugging purposes.
 */
void TraceEvent::dump() {
    PRINT("[%d] %s: %s \n", id, name.c_str(), org_print_fmt.c_str());
    PRINT("   format: %s\n", print_fmt.c_str());
    PRINT("   args  :\n");
    for (const auto& arg_ptr : arg_list) {
        if (!arg_ptr->name.empty()) {
            PRINT("       %s \n", arg_ptr->name.c_str());
        }
    }

    PRINT("   %s {\n", struct_type.c_str());
    std::ostringstream oss;
    for (const auto& pair : field_maps) {
        std::shared_ptr<trace_field> field = pair.second;
        std::string var = field->type + " " + field->name + ",";
        oss << std::left << "         " << std::setw(25) << var
            << "offset:" << field->offset << ", size:" << field->size;
        PRINT("%s\n", oss.str().c_str());
        oss.str("");
    }
    PRINT("   } \n");
    PRINT("\n\n");
}

void TraceEvent::ufshcd_runtime_event_handle(ulong addr){
    arg_list[0]->name = "dev_name";
    arg_list[1]->name = "usecs";
    arg_list[4]->name = "err";
    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[4]);

    std::shared_ptr<trace_field> field_ptr = field_maps["dev_state"];
    if (field_ptr) {
        int dev_state = plugin_ptr->read_int(addr + field_ptr->offset, "dev_state");
        copy_str(arg_list[2], decode_ufs_pwr_mode(dev_state));
    }
    field_ptr = field_maps["link_state"];
    if (field_ptr) {
        int link_state = plugin_ptr->read_int(addr + field_ptr->offset, "link_state");
        copy_str(arg_list[3], decode_ufs_link_state(link_state));
    }
}

void TraceEvent::ext4_event_fallocate_punch_zero_handle(ulong addr){
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (field_ptr) {
        unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
        *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));
    }
    arg_list[2]->name = "ino";
    arg_list[3]->name = "offset";
    arg_list[4]->name = "len";
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);

    field_ptr = field_maps["mode"];
    if (field_ptr) {
        unsigned int mode = plugin_ptr->read_uint(addr + field_ptr->offset, "mode");
        copy_str(arg_list[5], decode_ext4_fallocate_mode(mode));
    }
}

void TraceEvent::ext4_es_extent_event_handle(ulong addr){
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (field_ptr) {
        unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
        *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));
    }
    arg_list[2]->name = "ino";
    arg_list[3]->name = "lblk";
    arg_list[4]->name = "len";
    arg_list[5]->name = "pblk";
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);

    field_ptr = field_maps["status"];
    if (field_ptr) {
        unsigned int status = plugin_ptr->read_uint(addr + field_ptr->offset, "status");
        copy_str(arg_list[6], decode_ext4_es_status(status));
    }
}

void TraceEvent::lease_event_handle(ulong addr){
    arg_list[0]->name = "fl";
    read_trace_field(addr, arg_list[0]);

    std::shared_ptr<trace_field> field_ptr = field_maps["s_dev"];
    if (!field_ptr) return;
    unsigned int s_dev = plugin_ptr->read_uint(addr + field_ptr->offset, "s_dev");
    *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (s_dev >> 20);
    *reinterpret_cast<unsigned int*>(arg_list[2]->data) = (s_dev & ((1U << 20) - 1));

    arg_list[3]->name = "i_ino";
    arg_list[4]->name = "fl_blocker";
    arg_list[5]->name = "fl_owner";

    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);

    field_ptr = field_maps["fl_flags"];
    if (!field_ptr) return;
    unsigned int fl_flags = plugin_ptr->read_uint(addr + field_ptr->offset, "fl_flags");
    copy_str(arg_list[6], decode_file_lock_flags(fl_flags));

    field_ptr = field_maps["fl_type"];
    if (!field_ptr) return;
    unsigned char fl_type = plugin_ptr->read_byte(addr + field_ptr->offset, "fl_type");
    copy_str(arg_list[7], decode_file_lock_type(fl_type));

    arg_list[8]->name = "fl_break_time";
    arg_list[9]->name = "fl_downgrade_time";

    read_trace_field(addr, arg_list[8]);
    read_trace_field(addr, arg_list[9]);
}

void TraceEvent::dwc3_request_event_handle(ulong addr){
    arg_list[0]->name = "name";
    arg_list[1]->name = "req";
    arg_list[2]->name = "actual";
    arg_list[3]->name = "length";
    arg_list[7]->name = "status";

    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[7]);

    std::shared_ptr<trace_field> field_ptr = field_maps["zero"];
    int zero = field_ptr ? plugin_ptr->read_int(addr + field_ptr->offset, "zero") : 0;
    field_ptr = field_maps["short_not_ok"];
    int short_not_ok = field_ptr ? plugin_ptr->read_int(addr + field_ptr->offset, "short_not_ok") : 0;
    field_ptr = field_maps["no_interrupt"];
    int no_interrupt = field_ptr ? plugin_ptr->read_int(addr + field_ptr->offset, "no_interrupt") : 0;

    copy_str(arg_list[4], decode_usb_request_flags(zero, short_not_ok, no_interrupt));
}

void TraceEvent::xhci_urb_event_handle(ulong addr){
    arg_list[0]->name = "epnum";
    read_trace_field(addr, arg_list[0]);

    std::shared_ptr<trace_field> field_ptr = field_maps["dir_in"];
    int dir_in = field_ptr ? plugin_ptr->read_int(addr + field_ptr->offset, "dir_in") : 0;
    copy_str(arg_list[1], dir_in ? "in" : "out");

    field_ptr = field_maps["type"];
    int type = field_ptr ? plugin_ptr->read_int(addr + field_ptr->offset, "type") : 0;
    copy_str(arg_list[2], decode_usb_pipe_type(type));

    arg_list[3]->name = "urb";
    arg_list[4]->name = "pipe";
    arg_list[5]->name = "slot_id";
    arg_list[6]->name = "actual";
    arg_list[7]->name = "length";
    arg_list[8]->name = "num_mapped_sgs";
    arg_list[9]->name = "num_sgs";
    arg_list[10]->name = "stream";
    arg_list[11]->name = "flags";

    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);
    read_trace_field(addr, arg_list[7]);
    read_trace_field(addr, arg_list[8]);
    read_trace_field(addr, arg_list[9]);
    read_trace_field(addr, arg_list[10]);
    read_trace_field(addr, arg_list[11]);
}

void TraceEvent::spi_transfer_event_handle(ulong addr){
    // Read bus_num, chip_select, xfer, len
    arg_list[0]->name = "bus_num";
    arg_list[1]->name = "chip_select";
    arg_list[2]->name = "xfer";
    arg_list[3]->name = "len";

    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);

    int len = *reinterpret_cast<int*>(arg_list[3]->data);

    // Read tx_buf from dynamic array
    std::shared_ptr<trace_field> field_ptr = field_maps["tx_buf"];
    if (field_ptr && len > 0 && len <= 32) {
        unsigned char tx_buf[32];
        for (int i = 0; i < len; i++) {
            tx_buf[i] = plugin_ptr->read_byte(addr + field_ptr->offset + i, "tx_buf");
        }
        copy_str(arg_list[4], format_buffer_hex(tx_buf, len));
    } else {
        copy_str(arg_list[4], "[]");
    }

    // Read rx_buf from dynamic array
    field_ptr = field_maps["rx_buf"];
    if (field_ptr && len > 0 && len <= 32) {
        unsigned char rx_buf[32];
        for (int i = 0; i < len; i++) {
            rx_buf[i] = plugin_ptr->read_byte(addr + field_ptr->offset + i, "rx_buf");
        }
        copy_str(arg_list[5], format_buffer_hex(rx_buf, len));
    } else {
        copy_str(arg_list[5], "[]");
    }
}

void TraceEvent::dwc3_trb_event_handle(ulong addr){
    arg_list[0]->name = "name";
    arg_list[1]->name = "trb";
    arg_list[2]->name = "enqueue";
    arg_list[3]->name = "dequeue";
    arg_list[4]->name = "bph";
    arg_list[5]->name = "bpl";
    arg_list[8]->name = "ctrl";

    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[8]);

    std::shared_ptr<trace_field> field_ptr = field_maps["size"];
    unsigned int size = field_ptr ? plugin_ptr->read_uint(addr + field_ptr->offset, "size") : 0;
    field_ptr = field_maps["type"];
    int type = field_ptr ? plugin_ptr->read_int(addr + field_ptr->offset, "type") : 0;

    // PCM string
    copy_str(arg_list[6], get_dwc3_pcm_string(type, size));

    // Size (masked)
    *reinterpret_cast<unsigned int*>(arg_list[7]->data) = size & 0x00ffffff;

    // Control flags
    unsigned int ctrl = *reinterpret_cast<unsigned int*>(arg_list[8]->data);
    copy_str(arg_list[9], decode_dwc3_trb_ctrl_flags(ctrl));

    // TRB type string
    copy_str(arg_list[10], decode_dwc3_trb_type((ctrl & (0x3f << 4))));
}

void TraceEvent::usb_gadget_request_event_handle(ulong addr){
    arg_list[0]->name = "name";
    arg_list[1]->name = "req";
    arg_list[2]->name = "actual";
    arg_list[3]->name = "length";
    arg_list[4]->name = "num_mapped_sgs";
    arg_list[5]->name = "num_sgs";
    arg_list[6]->name = "stream_id";
    arg_list[9]->name = "status";
    arg_list[10]->name = "ret";

    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);
    read_trace_field(addr, arg_list[9]);
    read_trace_field(addr, arg_list[10]);

    std::shared_ptr<trace_field> field_ptr = field_maps["zero"];
    int zero = field_ptr ? plugin_ptr->read_int(addr + field_ptr->offset, "zero") : 0;
    field_ptr = field_maps["short_not_ok"];
    int short_not_ok = field_ptr ? plugin_ptr->read_int(addr + field_ptr->offset, "short_not_ok") : 0;
    field_ptr = field_maps["no_interrupt"];
    int no_interrupt = field_ptr ? plugin_ptr->read_int(addr + field_ptr->offset, "no_interrupt") : 0;

    copy_str(arg_list[7], decode_usb_request_flags(zero, short_not_ok, no_interrupt));
}

void TraceEvent::dwc3_gadget_ep_event_handle(ulong addr){
    arg_list[0]->name = "name";
    arg_list[1]->name = "maxpacket";
    arg_list[2]->name = "maxpacket_limit";
    arg_list[3]->name = "max_streams";
    arg_list[4]->name = "maxburst";
    arg_list[5]->name = "trb_enqueue";
    arg_list[6]->name = "trb_dequeue";

    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);

    std::shared_ptr<trace_field> field_ptr = field_maps["flags"];
    unsigned long flags = field_ptr ? plugin_ptr->read_ulong(addr + field_ptr->offset, "flags") : 0;
    field_ptr = field_maps["direction"];
    int direction = field_ptr ? plugin_ptr->read_int(addr + field_ptr->offset, "direction") : 0;

    copy_str(arg_list[7], decode_dwc3_ep_flags(flags, direction));
}

void TraceEvent::usb_ep_event_handle(ulong addr){
    arg_list[0]->name = "name";
    arg_list[1]->name = "maxpacket";
    arg_list[2]->name = "maxpacket_limit";
    arg_list[3]->name = "max_streams";
    arg_list[4]->name = "mult";
    arg_list[5]->name = "maxburst";
    arg_list[6]->name = "address";
    arg_list[9]->name = "ret";

    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);
    read_trace_field(addr, arg_list[9]);

    std::shared_ptr<trace_field> field_ptr = field_maps["claimed"];
    bool claimed = field_ptr ? plugin_ptr->read_int(addr + field_ptr->offset, "claimed") : false;
    field_ptr = field_maps["enabled"];
    bool enabled = field_ptr ? plugin_ptr->read_int(addr + field_ptr->offset, "enabled") : false;

    copy_str(arg_list[7], claimed ? "claimed:" : "released:");
    copy_str(arg_list[8], enabled ? "enabled" : "disabled");
}

void TraceEvent::xhci_dbc_event_handle(ulong addr){
    std::shared_ptr<trace_field> field_ptr = field_maps["dir"];
    int dir = field_ptr ? plugin_ptr->read_int(addr + field_ptr->offset, "dir") : 0;
    copy_str(arg_list[0], dir ? "bulk-in" : "bulk-out");

    arg_list[1]->name = "req";
    arg_list[2]->name = "actual";
    arg_list[3]->name = "length";
    arg_list[4]->name = "status";

    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
}

void TraceEvent::usb_gadget_event_handle(ulong addr){
    arg_list[0]->name = "speed";
    arg_list[1]->name = "max_speed";
    arg_list[2]->name = "state";
    arg_list[3]->name = "mA";
    arg_list[5]->name = "ret";

    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[5]);

    std::shared_ptr<trace_field> field_ptr = field_maps["sg_supported"];
    bool sg_supported = field_ptr ? plugin_ptr->read_int(addr + field_ptr->offset, "sg_supported") : false;
    field_ptr = field_maps["is_otg"];
    bool is_otg = field_ptr ? plugin_ptr->read_int(addr + field_ptr->offset, "is_otg") : false;
    field_ptr = field_maps["is_a_peripheral"];
    bool is_a_peripheral = field_ptr ? plugin_ptr->read_int(addr + field_ptr->offset, "is_a_peripheral") : false;
    field_ptr = field_maps["b_hnp_enable"];
    bool b_hnp_enable = field_ptr ? plugin_ptr->read_int(addr + field_ptr->offset, "b_hnp_enable") : false;
    field_ptr = field_maps["a_hnp_support"];
    bool a_hnp_support = field_ptr ? plugin_ptr->read_int(addr + field_ptr->offset, "a_hnp_support") : false;
    field_ptr = field_maps["hnp_polling_support"];
    bool hnp_polling_support = field_ptr ? plugin_ptr->read_int(addr + field_ptr->offset, "hnp_polling_support") : false;
    field_ptr = field_maps["host_request_flag"];
    bool host_request_flag = field_ptr ? plugin_ptr->read_int(addr + field_ptr->offset, "host_request_flag") : false;
    field_ptr = field_maps["quirk_ep_out_aligned_size"];
    bool quirk_ep_out_aligned_size = field_ptr ? plugin_ptr->read_int(addr + field_ptr->offset, "quirk_ep_out_aligned_size") : false;
    field_ptr = field_maps["quirk_altset_not_supp"];
    bool quirk_altset_not_supp = field_ptr ? plugin_ptr->read_int(addr + field_ptr->offset, "quirk_altset_not_supp") : false;
    field_ptr = field_maps["quirk_stall_not_supp"];
    bool quirk_stall_not_supp = field_ptr ? plugin_ptr->read_int(addr + field_ptr->offset, "quirk_stall_not_supp") : false;
    field_ptr = field_maps["quirk_zlp_not_supp"];
    bool quirk_zlp_not_supp = field_ptr ? plugin_ptr->read_int(addr + field_ptr->offset, "quirk_zlp_not_supp") : false;
    field_ptr = field_maps["is_selfpowered"];
    bool is_selfpowered = field_ptr ? plugin_ptr->read_int(addr + field_ptr->offset, "is_selfpowered") : false;
    field_ptr = field_maps["deactivated"];
    bool deactivated = field_ptr ? plugin_ptr->read_int(addr + field_ptr->offset, "deactivated") : false;
    field_ptr = field_maps["connected"];
    bool connected = field_ptr ? plugin_ptr->read_int(addr + field_ptr->offset, "connected") : false;

    copy_str(arg_list[4], decode_usb_gadget_flags(sg_supported, is_otg, is_a_peripheral,
                                                   b_hnp_enable, a_hnp_support, hnp_polling_support,
                                                   host_request_flag, quirk_ep_out_aligned_size,
                                                   quirk_altset_not_supp, quirk_stall_not_supp,
                                                   quirk_zlp_not_supp, is_selfpowered,
                                                   deactivated, connected));
}

void TraceEvent::f2fs_compress_pages_event_handle(ulong addr){
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (field_ptr) {
        unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
        *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));
    }
    arg_list[2]->name = "ino";
    arg_list[3]->name = "idx";
    arg_list[4]->name = "size";
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    field_ptr = field_maps["algtype"];
    if (field_ptr) {
        int algtype = plugin_ptr->read_int(addr + field_ptr->offset, "algtype");
        copy_str(arg_list[5], decode_f2fs_compress_algorithm(algtype));
    }
}

void TraceEvent::f2fs_prepare_bio_event_handle(ulong addr){
    std::shared_ptr<trace_field> field_ptr = field_maps["target"];
    if (field_ptr) {
        unsigned int target = plugin_ptr->read_uint(addr + field_ptr->offset, "target");
        *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (target >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (target & ((1U << 20) - 1));
    }
    field_ptr = field_maps["dev"];
    if (field_ptr) {
        unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
        *reinterpret_cast<unsigned int*>(arg_list[2]->data) = (dev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[3]->data) = (dev & ((1U << 20) - 1));
    }
    arg_list[4]->name = "op";
    arg_list[5]->name = "op_flags";
    arg_list[7]->name = "sector";
    arg_list[8]->name = "size";
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[7]);
    read_trace_field(addr, arg_list[8]);

    field_ptr = field_maps["type"];
    if (field_ptr) {
        int type = plugin_ptr->read_int(addr + field_ptr->offset, "type");
        copy_str(arg_list[6], decode_f2fs_page_type(type));
    }
}

void TraceEvent::f2fs_writepage_event_handle(ulong addr){
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (field_ptr) {
        unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
        *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));
    }
    arg_list[2]->name = "ino";
    arg_list[5]->name = "index";
    arg_list[6]->name = "dirty";
    arg_list[7]->name = "uptodate";
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);
    read_trace_field(addr, arg_list[7]);
    field_ptr = field_maps["type"];
    if (field_ptr) {
        int type = plugin_ptr->read_int(addr + field_ptr->offset, "type");
        copy_str(arg_list[3], decode_f2fs_page_type(type));
    }
    field_ptr = field_maps["dir"];
    if (field_ptr) {
        int dir = plugin_ptr->read_int(addr + field_ptr->offset, "dir");
        copy_str(arg_list[4], decode_f2fs_dir_type(dir));
    }
}

void TraceEvent::f2fs_submit_page_event_handle(ulong addr){
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (field_ptr) {
        unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
        *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));
    }
    arg_list[2]->name = "ino";
    arg_list[3]->name = "index";
    arg_list[4]->name = "old_blkaddr";
    arg_list[5]->name = "new_blkaddr";
    arg_list[6]->name = "op";
    arg_list[7]->name = "op_flags";
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);
    read_trace_field(addr, arg_list[7]);

    field_ptr = field_maps["temp"];
    if (field_ptr) {
        int temp = plugin_ptr->read_int(addr + field_ptr->offset, "temp");
        copy_str(arg_list[8], decode_f2fs_temp_type(temp));
    }
    field_ptr = field_maps["type"];
    if (field_ptr) {
        int type = plugin_ptr->read_int(addr + field_ptr->offset, "type");
        copy_str(arg_list[9], decode_f2fs_page_type(type));
    }
}

void TraceEvent::f2fs_sync_dirty_event_handle(ulong addr){
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (field_ptr) {
        unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
        *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));
    }
    field_ptr = field_maps["type"];
    if (field_ptr) {
        int type = plugin_ptr->read_int(addr + field_ptr->offset, "type");
        copy_str(arg_list[2], decode_f2fs_inode_type(type));
    }
    arg_list[3]->name = "count";
    read_trace_field(addr, arg_list[3]);
}

void TraceEvent::v4l2_buf_event_handle(ulong addr){
    arg_list[0]->name = "minor";
    read_trace_field(addr, arg_list[0]);

    std::shared_ptr<trace_field> field_ptr = field_maps["flags"];
    if (field_ptr) {
        unsigned int flags = plugin_ptr->read_uint(addr + field_ptr->offset, "flags");
        copy_str(arg_list[1], decode_v4l2_buffer_flags(flags));
    }

    field_ptr = field_maps["field"];
    if (field_ptr) {
        int field = plugin_ptr->read_int(addr + field_ptr->offset, "field");
        copy_str(arg_list[2], decode_v4l2_field(field));
    }

    arg_list[3]->name = "timestamp";
    read_trace_field(addr, arg_list[3]);

    field_ptr = field_maps["timecode_type"];
    if (field_ptr) {
        int timecode_type = plugin_ptr->read_int(addr + field_ptr->offset, "timecode_type");
        copy_str(arg_list[4], decode_v4l2_timecode_type(timecode_type));
    }

    field_ptr = field_maps["timecode_flags"];
    if (field_ptr) {
        unsigned int timecode_flags = plugin_ptr->read_uint(addr + field_ptr->offset, "timecode_flags");
        copy_str(arg_list[5], decode_v4l2_timecode_flags(timecode_flags));
    }

    arg_list[6]->name = "timecode_frames";
    arg_list[7]->name = "timecode_seconds";
    arg_list[8]->name = "timecode_minutes";
    arg_list[9]->name = "timecode_hours";
    arg_list[10]->name = "timecode_userbits0";
    arg_list[11]->name = "timecode_userbits1";
    arg_list[12]->name = "timecode_userbits2";
    arg_list[13]->name = "timecode_userbits3";
    arg_list[14]->name = "sequence";

    read_trace_field(addr, arg_list[6]);
    read_trace_field(addr, arg_list[7]);
    read_trace_field(addr, arg_list[8]);
    read_trace_field(addr, arg_list[9]);
    read_trace_field(addr, arg_list[10]);
    read_trace_field(addr, arg_list[11]);
    read_trace_field(addr, arg_list[12]);
    read_trace_field(addr, arg_list[13]);
    read_trace_field(addr, arg_list[14]);
}

void TraceEvent::v4l2_dqbuf_event_handle(ulong addr){
    arg_list[0]->name = "minor";
    arg_list[1]->name = "index";
    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);

    std::shared_ptr<trace_field> field_ptr = field_maps["type"];
    if (field_ptr) {
        int type = plugin_ptr->read_int(addr + field_ptr->offset, "type");
        copy_str(arg_list[2], decode_v4l2_buf_type(type));
    }

    arg_list[3]->name = "bytesused";
    read_trace_field(addr, arg_list[3]);

    field_ptr = field_maps["flags"];
    if (field_ptr) {
        unsigned int flags = plugin_ptr->read_uint(addr + field_ptr->offset, "flags");
        copy_str(arg_list[4], decode_v4l2_buffer_flags(flags));
    }

    field_ptr = field_maps["field"];
    if (field_ptr) {
        int field = plugin_ptr->read_int(addr + field_ptr->offset, "field");
        copy_str(arg_list[5], decode_v4l2_field(field));
    }

    arg_list[6]->name = "timestamp";
    read_trace_field(addr, arg_list[6]);

    field_ptr = field_maps["timecode_type"];
    if (field_ptr) {
        int timecode_type = plugin_ptr->read_int(addr + field_ptr->offset, "timecode_type");
        copy_str(arg_list[7], decode_v4l2_timecode_type(timecode_type));
    }

    field_ptr = field_maps["timecode_flags"];
    if (field_ptr) {
        unsigned int timecode_flags = plugin_ptr->read_uint(addr + field_ptr->offset, "timecode_flags");
        copy_str(arg_list[8], decode_v4l2_timecode_flags(timecode_flags));
    }

    arg_list[9]->name = "timecode_frames";
    arg_list[10]->name = "timecode_seconds";
    arg_list[11]->name = "timecode_minutes";
    arg_list[12]->name = "timecode_hours";
    arg_list[13]->name = "timecode_userbits0";
    arg_list[14]->name = "timecode_userbits1";
    arg_list[15]->name = "timecode_userbits2";
    arg_list[16]->name = "timecode_userbits3";
    arg_list[17]->name = "sequence";

    read_trace_field(addr, arg_list[9]);
    read_trace_field(addr, arg_list[10]);
    read_trace_field(addr, arg_list[11]);
    read_trace_field(addr, arg_list[12]);
    read_trace_field(addr, arg_list[13]);
    read_trace_field(addr, arg_list[14]);
    read_trace_field(addr, arg_list[15]);
    read_trace_field(addr, arg_list[16]);
    read_trace_field(addr, arg_list[17]);
}

void TraceEvent::i2c_event_handle(ulong addr){
    arg_list[0]->name = "adapter_nr";
    arg_list[1]->name = "msg_nr";
    arg_list[2]->name = "addr";
    arg_list[3]->name = "flags";
    arg_list[4]->name = "len";

    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);

    // Read buffer from dynamic array
    std::shared_ptr<trace_field> field_ptr = field_maps["buf"];
    if (field_ptr) {
        int len = *reinterpret_cast<int*>(arg_list[4]->data);
        if (len > 0 && len <= 32) {
            unsigned char buf[32];
            for (int i = 0; i < len; i++) {
                buf[i] = plugin_ptr->read_byte(addr + field_ptr->offset + i, "buf");
            }
            copy_str(arg_list[5], format_buffer_hex(buf, len));
        } else {
            copy_str(arg_list[5], "[]");
        }
    }
}

void TraceEvent::smbus_event_handle(ulong addr){
    arg_list[0]->name = "adapter_nr";
    arg_list[1]->name = "addr";
    arg_list[2]->name = "flags";
    arg_list[3]->name = "command";
    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);

    std::shared_ptr<trace_field> field_ptr = field_maps["protocol"];
    if (field_ptr) {
        int protocol = plugin_ptr->read_int(addr + field_ptr->offset, "protocol");
        copy_str(arg_list[4], decode_smbus_protocol(protocol));
    }

    arg_list[5]->name = "len";
    read_trace_field(addr, arg_list[5]);

    field_ptr = field_maps["buf"];
    if (field_ptr) {
        int len = *reinterpret_cast<int*>(arg_list[5]->data);
        if (len > 0 && len <= 32) {
            unsigned char buf[32];
            for (int i = 0; i < len; i++) {
                buf[i] = plugin_ptr->read_byte(addr + field_ptr->offset + i, "buf");
            }
            copy_str(arg_list[6], format_buffer_hex(buf, len));
        } else {
            copy_str(arg_list[6], "[]");
        }
    }
}

void TraceEvent::dump2() {
    PRINT("%s: %s \n\n\n", name.c_str(), org_print_fmt.c_str());
}

/**
 * @brief Decode ext4 allocation flags to string
 */
std::string TraceEvent::decode_ext4_alloc_flags(unsigned int flags) {
    struct flag_map {
        unsigned int mask;
        const char *name;
    };

    struct flag_map maps[] = {
        { 0x0001, "HINT_MERGE" }, { 0x0002, "HINT_RESV" }, { 0x0004, "HINT_MDATA" },
        { 0x0008, "HINT_FIRST" }, { 0x0010, "HINT_BEST" }, { 0x0020, "HINT_DATA" },
        { 0x0040, "HINT_NOPREALLOC" }, { 0x0080, "HINT_GRP_ALLOC" },
        { 0x0100, "HINT_GOAL_ONLY" }, { 0x0200, "HINT_TRY_GOAL" },
        { 0x0400, "DELALLOC_RESV" }, { 0x0800, "STREAM_ALLOC" },
        { 0x1000, "USE_ROOT_BLKS" }, { 0x2000, "USE_RESV" }, { 0x4000, "STRICT_CHECK" },
    };

    std::string result;
    int first = 1;
    for (int i = 0; i < 15; i++) {
        if (flags & maps[i].mask) {
            if (!first) result += "|";
            result += maps[i].name;
            first = 0;
        }
    }
    return result.empty() ? "0" : result;
}

std::string TraceEvent::decode_ext4_free_flags(unsigned int flags) {
    struct flag_map { unsigned int mask; const char *name; };
    struct flag_map maps[] = {
        { 0x0001, "METADATA" }, { 0x0002, "FORGET" }, { 0x0004, "VALIDATED" },
        { 0x0008, "NO_QUOTA" }, { 0x0010, "1ST_CLUSTER" }, { 0x0020, "LAST_CLUSTER" },
    };
    std::string result;
    int first = 1;
    for (int i = 0; i < 6; i++) {
        if (flags & maps[i].mask) {
            if (!first) result += "|";
            result += maps[i].name;
            first = 0;
        }
    }
    return result.empty() ? "0" : result;
}

std::string TraceEvent::decode_ext4_fallocate_mode(unsigned int mode) {
    struct flag_map { unsigned int mask; const char *name; };
    struct flag_map maps[] = {
        { 0x01, "KEEP_SIZE" }, { 0x02, "PUNCH_HOLE" }, { 0x04, "NO_HIDE_STALE" },
        { 0x08, "COLLAPSE_RANGE" }, { 0x10, "ZERO_RANGE" },
    };
    std::string result;
    int first = 1;
    for (int i = 0; i < 5; i++) {
        if (mode & maps[i].mask) {
            if (!first) result += "|";
            result += maps[i].name;
            first = 0;
        }
    }
    return result.empty() ? "0" : result;
}

std::string TraceEvent::decode_ext4_map_flags(unsigned int flags) {
    struct flag_map { unsigned int mask; const char *name; };
    struct flag_map maps[] = {
        { 0x0001, "CREATE" }, { 0x0002, "UNWRIT" }, { 0x0004, "DELALLOC" },
        { 0x0008, "PRE_IO" }, { 0x0010, "CONVERT" }, { 0x0020, "METADATA_NOFAIL" },
        { 0x0040, "NO_NORMALIZE" }, { 0x0100, "CONVERT_UNWRITTEN" },
        { 0x0200, "ZERO" }, { 0x0400, "IO_SUBMIT" }, { 0x40000000, "EX_NOCACHE" },
    };
    std::string result;
    int first = 1;
    for (int i = 0; i < 11; i++) {
        if (flags & maps[i].mask) {
            if (!first) result += "|";
            result += maps[i].name;
            first = 0;
        }
    }
    return result.empty() ? "0" : result;
}

std::string TraceEvent::decode_ext4_mflags(unsigned int mflags) {
    std::string result;
    if (mflags & (1UL << 5)) result += "N";   // NEW
    if (mflags & (1UL << 4)) result += "M";   // MAPPED
    if (mflags & (1UL << 11)) result += "U";  // UNWRITTEN
    if (mflags & (1UL << 9)) result += "B";   // BOUNDARY
    return result;
}

std::string TraceEvent::decode_ext4_es_status(unsigned int status) {
    std::string result;
    if (status & (1 << 0)) result += "W";  // WRITTEN
    if (status & (1 << 1)) result += "U";  // UNWRITTEN
    if (status & (1 << 2)) result += "D";  // DELAYED
    if (status & (1 << 3)) result += "H";  // HOLE
    if (status & (1 << 4)) result += "R";  // REFERENCED
    return result;
}

/**
 * @brief Decode task state to string
 */
std::string TraceEvent::decode_task_state(long state) {
    // Task state mask
    const long TASK_STATE_MAX = ((0x0000 | 0x0001 | 0x0002 | 0x0004 | 0x0008 | 0x0010 | 0x0020 | 0x0040) + 1) << 1;
    const long state_mask = TASK_STATE_MAX - 1;

    long masked_state = state & state_mask;

    if (masked_state == 0) {
        return "R";  // Running
    }

    struct flag_map {
        long mask;
        const char *name;
    };

    struct flag_map maps[] = {
        { 0x0001, "S" },  // TASK_INTERRUPTIBLE
        { 0x0002, "D" },  // TASK_UNINTERRUPTIBLE
        { 0x0004, "T" },  // TASK_STOPPED
        { 0x0008, "t" },  // TASK_TRACED
        { 0x0010, "X" },  // EXIT_DEAD
        { 0x0020, "Z" },  // EXIT_ZOMBIE
        { 0x0040, "P" },  // TASK_PARKED
        { 0x0080, "I" },  // TASK_IDLE
    };

    std::string result;
    int first = 1;
    for (int i = 0; i < 8; i++) {
        if (masked_state & maps[i].mask) {
            if (!first) result += "|";
            result += maps[i].name;
            first = 0;
        }
    }

    // Check for TASK_NEW flag
    if (state & TASK_STATE_MAX) {
        result += "+";
    }

    return result.empty() ? "R" : result;
}

/**
 * @brief Get task event name
 */
std::string TraceEvent::get_task_event_name(int evt) {
    // Task event names from WALT scheduler
    static const char* task_event_names[] = {
        "PUT_PREV_TASK",
        "PICK_NEXT_TASK",
        "TASK_WAKE",
        "TASK_MIGRATE",
        "TASK_UPDATE",
        "IRQ_UPDATE",
    };

    if (evt >= 0 && evt < 6) {
        return task_event_names[evt];
    }
    return "UNKNOWN";
}

/**
 * @brief Decode UFS trace string type
 */
std::string TraceEvent::decode_ufs_trace_str_t(int str_t) {
    switch (str_t) {
        case 0: return "send_req";
        case 1: return "complete_rsp";
        case 2: return "dev_complete";
        case 3: return "query_send";
        case 4: return "query_complete";
        case 5: return "query_complete_err";
        case 6: return "tm_send";
        case 7: return "tm_complete";
        case 8: return "tm_complete_err";
        default: return "unknown";
    }
}

/**
 * @brief Decode UFS trace TSF type
 */
std::string TraceEvent::decode_ufs_trace_tsf_t(int tsf_t) {
    switch (tsf_t) {
        case 0: return "CDB";
        case 1: return "OSF";
        case 2: return "TM_INPUT";
        case 3: return "TM_OUTPUT";
        default: return "unknown";
    }
}

/**
 * @brief Decode UFS power mode
 */
std::string TraceEvent::decode_ufs_pwr_mode(int mode) {
    switch (mode) {
        case 1: return "UFS_ACTIVE_PWR_MODE";
        case 2: return "UFS_SLEEP_PWR_MODE";
        case 3: return "UFS_POWERDOWN_PWR_MODE";
        case 4: return "UFS_DEEPSLEEP_PWR_MODE";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Decode UFS link state
 */
std::string TraceEvent::decode_ufs_link_state(int state) {
    switch (state) {
        case 0: return "UIC_LINK_OFF_STATE";
        case 1: return "UIC_LINK_ACTIVE_STATE";
        case 2: return "UIC_LINK_HIBERN8_STATE";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Decode UFS clock gating state
 */
std::string TraceEvent::decode_ufs_clk_gating_state(int state) {
    switch (state) {
        case 0: return "CLKS_OFF";
        case 1: return "CLKS_ON";
        case 2: return "REQ_CLKS_OFF";
        case 3: return "REQ_CLKS_ON";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Build flag string from flag map
 *
 * Constructs a string representation of set flags using a flag map.
 *
 * @param flags Flag bitmask value
 * @param maps Array of flag mappings
 * @param count Number of flag mappings
 * @return String with pipe-separated flag names
 */
std::string TraceEvent::build_flag_string(unsigned int flags, const flag_map* maps, int count) {
    std::string result;
    bool first = true;

    for (int i = 0; i < count; i++) {
        if (flags & maps[i].mask) {
            if (!first) result += "|";
            result += maps[i].name;
            first = false;
        }
    }

    return result;
}

/**
 * @brief Decode tick stop dependency flags
 *
 * Converts tick stop dependency bitmask to human-readable string.
 *
 * @param dependency Dependency bitmask value
 * @return String representation of dependency
 */
std::string TraceEvent::decode_tick_dependency(int dependency) {
    switch (dependency) {
        case 0: return "NONE";
        case (1 << 0): return "POSIX_TIMER";
        case (1 << 1): return "PERF_EVENTS";
        case (1 << 2): return "SCHED";
        case (1 << 3): return "CLOCK_UNSTABLE";
        case (1 << 4): return "RCU";
        case (1 << 5): return "RCU_EXP";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Decode clock ID to string
 *
 * Converts clock ID enumeration to human-readable string.
 *
 * @param clockid Clock ID value
 * @return String representation of clock type
 */
std::string TraceEvent::decode_clockid(uint clockid) {
    switch (clockid) {
        case 0: return "CLOCK_REALTIME";
        case 1: return "CLOCK_MONOTONIC";
        case 7: return "CLOCK_BOOTTIME";
        case 11: return "CLOCK_TAI";
        default: return "";
    }
}

/**
 * @brief Decode hrtimer mode to string
 *
 * Converts hrtimer mode flags to human-readable string.
 *
 * @param mode Timer mode value
 * @return String representation of timer mode
 */
std::string TraceEvent::decode_hrtimer_mode(uint mode) {
    switch (mode) {
        case 0: return "ABS";
        case 1: return "REL";
        case 2: return "ABS|PINNED";
        case 3: return "REL|PINNED";
        case 4: return "ABS|SOFT";
        case 5: return "REL|SOFT";
        case 6: return "ABS|PINNED|SOFT";
        case 7: return "REL|PINNED|SOFT";
        case 8: return "ABS|HARD";
        case 9: return "REL|HARD";
        case 10: return "ABS|PINNED|HARD";
        case 11: return "REL|PINNED|HARD";
        default: return "";
    }
}

/**
 * @brief Decode task state to string
 *
 * Converts task state value to human-readable character.
 *
 * @param state Task state value
 * @param mask State mask
 * @return String representation of task state
 */
std::string TraceEvent::decode_task_state(long state, uint16_t mask) {
    switch (state & mask) {
        case 0: return "R";   // Running
        case 1: return "S";   // Sleeping
        case 2: return "D";   // Disk sleep (uninterruptible)
        case 4: return "T";   // Stopped
        case 8: return "t";   // Tracing stop
        case 16: return "X";  // Dead
        case 32: return "Z";  // Zombie
        case 64: return "P";  // Parked
        case 128: return "I"; // Idle
        default: return "Unknown";
    }
}

/**
 * @brief Decode SPI mode flags to string
 */
std::string TraceEvent::decode_spi_mode_flags(unsigned long mode) {
    std::string result;

    // Mode bits (CPOL and CPHA)
    unsigned long mode_bits = mode & ((1UL << 1) | (1UL << 0));
    char mode_str[16];
    snprintf(mode_str, sizeof(mode_str), "%lu", mode_bits);
    result = mode_str;
    result += ", ";

    // Additional flags
    if (mode & (1UL << 2)) result += "cs_high, ";
    if (mode & (1UL << 3)) result += "lsb, ";
    if (mode & (1UL << 4)) result += "3wire, ";
    if (mode & (1UL << 5)) result += "loopback, ";

    // Remove trailing comma and space if present
    if (result.size() >= 2 && result.substr(result.size() - 2) == ", ") {
        result = result.substr(0, result.size() - 2);
    }

    return result;
}

/**
 * @brief Format buffer as hex string with brackets
 */
std::string TraceEvent::format_buffer_hex(const unsigned char *buf, int len) {
    if (!buf || len <= 0) return "[]";

    std::string result = "[";
    char hex[4];
    for (int i = 0; i < len && i < 32; i++) {  // Limit to 32 bytes
        snprintf(hex, sizeof(hex), "%02x", buf[i]);
        result += hex;
        if (i < len - 1) result += " ";
    }
    if (len > 32) result += "...";
    result += "]";

    return result;
}

/**
 * @brief Decode USB request flags
 */
std::string TraceEvent::decode_usb_request_flags(int zero, int short_not_ok, int no_interrupt) {
    std::string result;
    result += zero ? "Z" : "z";
    result += short_not_ok ? "S" : "s";
    result += no_interrupt ? "i" : "I";
    return result;
}

/**
 * @brief Decode USB gadget flags
 */
std::string TraceEvent::decode_usb_gadget_flags(bool sg_supported, bool is_otg, bool is_a_peripheral,
                                                 bool b_hnp_enable, bool a_hnp_support, bool hnp_polling_support,
                                                 bool host_request_flag, bool quirk_ep_out_aligned_size,
                                                 bool quirk_altset_not_supp, bool quirk_stall_not_supp,
                                                 bool quirk_zlp_not_supp, bool is_selfpowered,
                                                 bool deactivated, bool connected) {
    std::string result;
    if (sg_supported) result += "sg:";
    if (is_otg) result += "OTG:";
    if (is_a_peripheral) result += "a_peripheral:";
    if (b_hnp_enable) result += "b_hnp:";
    if (a_hnp_support) result += "a_hnp:";
    if (hnp_polling_support) result += "hnp_poll:";
    if (host_request_flag) result += "hostreq:";
    if (quirk_ep_out_aligned_size) result += "out_aligned:";
    if (quirk_altset_not_supp) result += "no_altset:";
    if (quirk_stall_not_supp) result += "no_stall:";
    if (quirk_zlp_not_supp) result += "no_zlp";
    result += is_selfpowered ? "self-powered:" : "bus-powered:";
    result += deactivated ? "deactivated:" : "activated:";
    result += connected ? "connected" : "disconnected";
    return result;
}

/**
 * @brief Decode USB pipe type
 */
std::string TraceEvent::decode_usb_pipe_type(int type) {
    switch (type) {
        case 0: return "control";
        case 1: return "isoc";
        case 2: return "bulk";
        case 3: return "intr";
        default: return "unknown";
    }
}

/**
 * @brief Decode DWC3 endpoint flags
 */
std::string TraceEvent::decode_dwc3_ep_flags(unsigned long flags, int direction) {
    std::string result;
    result += (flags & (1UL << 0)) ? 'E' : 'e';  // Enabled
    result += ':';
    result += (flags & (1UL << 1)) ? 'S' : 's';  // Stall
    result += (flags & (1UL << 2)) ? 'W' : 'w';  // Wedge
    result += (flags & (1UL << 3)) ? 'B' : 'b';  // Busy
    result += (flags & (1UL << 5)) ? 'P' : 'p';  // Pending
    result += ':';
    result += direction ? '<' : '>';  // Direction
    return result;
}

/**
 * @brief Decode DWC3 TRB control flags
 */
std::string TraceEvent::decode_dwc3_trb_ctrl_flags(unsigned int ctrl) {
    std::string result;
    result += (ctrl & (1UL << 0)) ? 'H' : 'h';   // HWO
    result += (ctrl & (1UL << 1)) ? 'L' : 'l';   // LST
    result += (ctrl & (1UL << 2)) ? 'C' : 'c';   // CHN
    result += (ctrl & (1UL << 3)) ? 'S' : 's';   // CSP
    result += ':';
    result += (ctrl & (1UL << 10)) ? 'S' : 's';  // SPR
    result += (ctrl & (1UL << 11)) ? 'C' : 'c';  // IOC
    return result;
}

/**
 * @brief Get DWC3 PCM string
 */
std::string TraceEvent::get_dwc3_pcm_string(int type, unsigned int size) {
    // For isoc (type 1) and interrupt (type 3)
    if (type == 1 || type == 3) {
        int pcm = ((size >> 24) & 3) + 1;
        switch (pcm) {
            case 1: return "1x ";
            case 2: return "2x ";
            case 3:
            default: return "3x ";
        }
    }
    return "";
}

// F2FS helper functions
std::string TraceEvent::decode_f2fs_compress_algorithm(int algtype) {
    switch (algtype) {
        case 1: return "LZO";
        case 2: return "LZ4";
        case 3: return "ZSTD";
        case 4: return "LZO-RLE";
        default: return "UNKNOWN";
    }
}

std::string TraceEvent::decode_f2fs_shutdown_mode(int mode) {
    switch (mode) {
        case 0x0: return "full sync";
        case 0x1: return "meta sync";
        case 0x2: return "no sync";
        case 0x3: return "meta flush";
        case 0x4: return "need fsck";
        default: return "unknown";
    }
}

std::string TraceEvent::decode_f2fs_inode_type(int type) {
    return (type == 0) ? "FILE" : "DIR";
}

std::string TraceEvent::decode_f2fs_extent_type(int type) {
    return (type == 0) ? "Read" : "Block Age";
}

std::string TraceEvent::decode_f2fs_page_type(int type) {
    switch (type) {
        case 0: return "DATA";
        case 1: return "NODE";
        case 2: return "META";
        case 4: return "META_FLUSH";
        case 5: return "IN-PLACE";
        case 6: return "OUT-OF-PLACE";
        default: return "UNKNOWN";
    }
}

std::string TraceEvent::decode_f2fs_dir_type(int dir) {
    return (dir == 0) ? "FILE" : "DIR";
}

std::string TraceEvent::decode_f2fs_temp_type(int temp) {
    switch (temp) {
        case 0: return "HOT";
        case 1: return "WARM";
        case 2: return "COLD";
        default: return "UNKNOWN";
    }
}

std::string TraceEvent::decode_f2fs_gc_type(int gc_type) {
    return (gc_type == 0) ? "Background GC" : "Foreground GC";
}

std::string TraceEvent::decode_f2fs_alloc_mode(int mode) {
    switch (mode) {
        case 0: return "LFS-mode";
        case 1: return "SSR-mode";
        default: return "AT_SSR-mode";
    }
}

std::string TraceEvent::decode_f2fs_gc_mode(int mode) {
    switch (mode) {
        case 0: return "Cost-Benefit";
        case 1: return "Greedy";
        default: return "Age-threshold";
    }
}

std::string TraceEvent::decode_f2fs_victim_type(int type) {
    switch (type) {
        case 0: return "Hot DATA";
        case 1: return "Warm DATA";
        case 2: return "Cold DATA";
        case 3: return "Hot NODE";
        case 4: return "Warm NODE";
        case 5: return "Cold NODE";
        case 8: return "No TYPE";
        default: return "UNKNOWN";
    }
}

std::string TraceEvent::decode_f2fs_cp_reason(int reason) {
    switch (reason) {
        case 0: return "no needed";
        case 1: return "non regular";
        case 2: return "compressed";
        case 3: return "hardlink";
        case 4: return "sb needs cp";
        case 5: return "wrong pino";
        case 6: return "no space roll forward";
        case 7: return "node needs cp";
        case 8: return "fastboot mode";
        case 9: return "log type is 2";
        case 10: return "dir needs recovery";
        default: return "unknown";
    }
}

// V4L2 helper functions
std::string TraceEvent::decode_v4l2_buffer_flags(unsigned int flags) {
    struct flag_map { unsigned int mask; const char *name; };
    struct flag_map maps[] = {
        { 0x00000001, "MAPPED" }, { 0x00000002, "QUEUED" }, { 0x00000004, "DONE" },
        { 0x00000008, "KEYFRAME" }, { 0x00000010, "PFRAME" }, { 0x00000020, "BFRAME" },
        { 0x00000040, "ERROR" }, { 0x00000100, "TIMECODE" }, { 0x00000400, "PREPARED" },
        { 0x00000800, "NO_CACHE_INVALIDATE" }, { 0x00001000, "NO_CACHE_CLEAN" },
        { 0x00002000, "TIMESTAMP_MONOTONIC" }, { 0x00004000, "TIMESTAMP_COPY" },
        { 0x00100000, "LAST" },
    };
    std::string result;
    int first = 1;
    for (int i = 0; i < 14; i++) {
        if (flags & maps[i].mask) {
            if (!first) result += "|";
            result += maps[i].name;
            first = 0;
        }
    }
    return result.empty() ? "0" : result;
}

std::string TraceEvent::decode_v4l2_field(int field) {
    switch (field) {
        case 0: return "ANY";
        case 1: return "NONE";
        case 2: return "TOP";
        case 3: return "BOTTOM";
        case 4: return "INTERLACED";
        case 5: return "SEQ_TB";
        case 6: return "SEQ_BT";
        case 7: return "ALTERNATE";
        case 8: return "INTERLACED_TB";
        case 9: return "INTERLACED_BT";
        default: return "UNKNOWN";
    }
}

std::string TraceEvent::decode_v4l2_timecode_type(int type) {
    switch (type) {
        case 1: return "24FPS";
        case 2: return "25FPS";
        case 3: return "30FPS";
        case 4: return "50FPS";
        case 5: return "60FPS";
        default: return "UNKNOWN";
    }
}

std::string TraceEvent::decode_v4l2_timecode_flags(unsigned int flags) {
    struct flag_map { unsigned int mask; const char *name; };
    struct flag_map maps[] = {
        { 0x0001, "DROPFRAME" },
        { 0x0002, "COLORFRAME" },
        { 0x0008, "USERBITS_8BITCHARS" },
    };
    std::string result;
    int first = 1;
    for (int i = 0; i < 3; i++) {
        if (flags & maps[i].mask) {
            if (!first) result += "|";
            result += maps[i].name;
            first = 0;
        }
    }
    if (result.empty()) result = "USERBITS_USERDEFINED";
    return result;
}

std::string TraceEvent::decode_v4l2_buf_type(int type) {
    switch (type) {
        case 1: return "VIDEO_CAPTURE";
        case 2: return "VIDEO_OUTPUT";
        case 3: return "VIDEO_OVERLAY";
        case 4: return "VBI_CAPTURE";
        case 5: return "VBI_OUTPUT";
        case 6: return "SLICED_VBI_CAPTURE";
        case 7: return "SLICED_VBI_OUTPUT";
        case 8: return "VIDEO_OUTPUT_OVERLAY";
        case 9: return "VIDEO_CAPTURE_MPLANE";
        case 10: return "VIDEO_OUTPUT_MPLANE";
        case 11: return "SDR_CAPTURE";
        case 12: return "SDR_OUTPUT";
        case 13: return "META_CAPTURE";
        case 128: return "PRIVATE";
        default: return "UNKNOWN";
    }
}

// Thermal helper functions
std::string TraceEvent::decode_thermal_trip_type(int trip_type) {
    switch (trip_type) {
        case 0: return "ACTIVE";
        case 1: return "PASSIVE";
        case 2: return "HOT";
        case 3: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

// SMBus helper functions
std::string TraceEvent::decode_smbus_protocol(int protocol) {
    switch (protocol) {
        case 0: return "QUICK";
        case 1: return "BYTE";
        case 2: return "BYTE_DATA";
        case 3: return "WORD_DATA";
        case 4: return "PROC_CALL";
        case 5: return "BLOCK_DATA";
        case 6: return "I2C_BLOCK_BROKEN";
        case 7: return "BLOCK_PROC_CALL";
        case 8: return "I2C_BLOCK_DATA";
        default: return "UNKNOWN";
    }
}

// SWIOTLB helper functions
std::string TraceEvent::decode_swiotlb_force(int swiotlb_force) {
    switch (swiotlb_force) {
        case 0: return "NORMAL";
        case 1: return "FORCE";
        case 2: return "NO_FORCE";
        default: return "UNKNOWN";
    }
}

// SMC Invoke helper functions
std::string TraceEvent::decode_smcinvoke_cmd(unsigned int cmd) {
    // Simplified - just return the command number
    // Full implementation would decode the ioctl command structure
    switch (cmd & 0xFF) {
        case 1: return "SMCINVOKE_IOCTL_INVOKE_REQ";
        case 2: return "SMCINVOKE_IOCTL_ACCEPT_REQ";
        case 3: return "SMCINVOKE_IOCTL_SERVER_REQ";
        case 4: return "SMCINVOKE_IOCTL_ACK_LOCAL_OBJ";
        case 255: return "SMCINVOKE_IOCTL_LOG";
        default: return "UNKNOWN";
    }
}

#pragma GCC diagnostic pop
