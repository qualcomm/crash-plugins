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

#include "events.h"
#include "logger/logger_core.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

/**
 * @brief Handle tick stop event
 *
 * Processes tick stop event data including success status and dependency reason.
 */
void tick_stop_event::handle(ulong addr) {
    // Read success field
    arg_list[0]->name = "success";
    read_trace_field(addr, arg_list[0]);

    // Decode and display dependency reason
    std::shared_ptr<trace_field> field_ptr = field_maps["dependency"];
    if (field_ptr) {
        int dependency = plugin_ptr->read_int(addr + field_ptr->offset, "dependency");
        copy_str(arg_list[1], decode_tick_dependency(dependency));
    }
}

/**
 * @brief Handle timer start event
 *
 * Processes timer start event including timer address, function, expiration,
 * timeout calculation, CPU, index, and flags.
 */
void timer_start_event::handle(ulong addr) {
    // Read basic timer fields
    arg_list[0]->name = "timer";
    arg_list[1]->name = "function";
    arg_list[2]->name = "expires";
    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);

    ulong expires = *reinterpret_cast<unsigned long*>(arg_list[2]->data);

    // Read current time and calculate timeout
    std::shared_ptr<trace_field> field_ptr = field_maps["now"];
    if (!field_ptr) return;
    ulong now = plugin_ptr->read_ulong(addr + field_ptr->offset, "now");

    // Read and decode timer flags
    field_ptr = field_maps["flags"];
    if (!field_ptr) return;
    uint flags = plugin_ptr->read_uint(addr + field_ptr->offset, "flags");

    // Store timeout (expires - now)
    if (arg_list[3]) {
        *reinterpret_cast<ulong*>(arg_list[3]->data) = expires - now;
    }

    // Store CPU (lower 18 bits)
    if (arg_list[4]) {
        *reinterpret_cast<unsigned int*>(arg_list[4]->data) = flags & 0x0003FFFF;
    }

    // Store index (upper bits)
    if (arg_list[5]) {
        *reinterpret_cast<unsigned int*>(arg_list[5]->data) = flags >> 22;
    }

    // Decode and store flag string
    if (arg_list[6]) {
        static const flag_map maps[] = {
            { 0x00040000, "M" },
            { 0x00080000, "D" },
            { 0x00100000, "P" },
            { 0x00200000, "I" },
        };
        copy_str(arg_list[6], build_flag_string(flags, maps, 4));
    }
}

/**
 * @brief Handle system call enter event
 *
 * Processes syscall entry event including syscall ID and up to 6 arguments.
 */
void sys_enter_event::handle(ulong addr) {
    arg_list[0]->name = "id";
    read_trace_field(addr, arg_list[0]);

    // Read syscall arguments array
    std::shared_ptr<trace_field> field_ptr = field_maps["args"];
    if (!field_ptr) return;

    for (int i = 0; i < 6; i++) {
        if (arg_list[i + 1]) {
            *reinterpret_cast<unsigned long*>(arg_list[i + 1]->data) =
                plugin_ptr->read_ulong(addr + field_ptr->offset + i * sizeof(ulong), "args");
        }
    }
}

/**
 * @brief Handle hrtimer init event
 *
 * Processes hrtimer initialization event including timer address, clock ID, and mode.
 */
void hrtimer_init_event::handle(ulong addr) {
    arg_list[0]->name = "hrtimer";
    read_trace_field(addr, arg_list[0]);

    // Decode and store clock ID
    std::shared_ptr<trace_field> field_ptr = field_maps["clockid"];
    if (field_ptr) {
        uint clockid = plugin_ptr->read_uint(addr + field_ptr->offset, "clockid");
        copy_str(arg_list[1], decode_clockid(clockid));
    }

    // Decode and store timer mode
    field_ptr = field_maps["mode"];
    if (field_ptr) {
        uint mode = plugin_ptr->read_uint(addr + field_ptr->offset, "mode");
        copy_str(arg_list[2], decode_hrtimer_mode(mode));
    }
}

/**
 * @brief Handle hrtimer start event
 *
 * Processes hrtimer start event including timer address, function, expiration times, and mode.
 */
void hrtimer_start_event::handle(ulong addr) {
    // Read basic hrtimer fields
    arg_list[0]->name = "hrtimer";
    arg_list[1]->name = "function";
    arg_list[2]->name = "expires";
    arg_list[3]->name = "softexpires";
    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);

    // Decode and store timer mode
    std::shared_ptr<trace_field> field_ptr = field_maps["mode"];
    if (field_ptr) {
        uint mode = plugin_ptr->read_uint(addr + field_ptr->offset, "mode");
        copy_str(arg_list[4], decode_hrtimer_mode(mode));
    }
}

/**
 * @brief Handle DMA map page event
 * Format: "%s dir=%s dma_addr=%llx size=%zu phys_addr=%llx attrs=%s"
 */
void dma_map_page_event::handle(ulong addr) {
    // Read device string (__get_str(device))
    arg_list[0]->name = "device";
    read_trace_field(addr, arg_list[0]);

    // Decode and store DMA direction using __print_symbolic
    std::shared_ptr<trace_field> field_ptr = field_maps["dir"];
    if (field_ptr) {
        int dir = plugin_ptr->read_uint(addr + field_ptr->offset, "dir");
        std::string res;
        switch (dir) {
            case 0:
                res = "BIDIRECTIONAL";
                break;
            case 1:
                res = "TO_DEVICE";
                break;
            case 2:
                res = "FROM_DEVICE";
                break;
            case 3:
                res = "NONE";
                break;
            default:
                res = "NONE";
                break;
        }
        copy_str(arg_list[1], res);
    }

    // Read dma_addr (REC->dma_addr)
    arg_list[2]->name = "dma_addr";
    read_trace_field(addr, arg_list[2]);

    // Read size (REC->size)
    arg_list[3]->name = "size";
    read_trace_field(addr, arg_list[3]);

    // Read phys_addr (REC->phys_addr)
    arg_list[4]->name = "phys_addr";
    read_trace_field(addr, arg_list[4]);

    // Decode and store DMA attributes using __print_flags
    field_ptr = field_maps["attrs"];
    if (field_ptr) {
        ulong attrs = plugin_ptr->read_ulong(addr + field_ptr->offset, "attrs");

        // Build flags string using __print_flags logic
        std::string flags_str;
        bool first = true;

        struct flag_map {
            unsigned long mask;
            const char *name;
        };

        struct flag_map maps[] = {
            { 1UL << 1, "WEAK_ORDERING" },
            { 1UL << 2, "WRITE_COMBINE" },
            { 1UL << 4, "NO_KERNEL_MAPPING" },
            { 1UL << 5, "SKIP_CPU_SYNC" },
            { 1UL << 6, "FORCE_CONTIGUOUS" },
            { 1UL << 7, "ALLOC_SINGLE_PAGES" },
            { 1UL << 8, "NO_WARN" },
            { 1UL << 9, "PRIVILEGED" },
        };

        for (int i = 0; i < 8; i++) {
            if (attrs & maps[i].mask) {
                if (!first) {
                    flags_str += "|";
                }
                flags_str += maps[i].name;
                first = false;
            }
        }

        if (flags_str.empty()) {
            flags_str = "0";
        }

        copy_str(arg_list[5], flags_str);
    }
}

/**
 * @brief Handle DMA unmap page event
 */
void dma_unmap_page_event::handle(ulong addr) {
    print_dma_unmap_event(addr);
}

/**
 * @brief Handle DMA map resource event
 */
void dma_map_resource_event::handle(ulong addr) {
    print_dma_map_event(addr);
}

/**
 * @brief Handle DMA unmap resource event
 */
void dma_unmap_resource_event::handle(ulong addr) {
    print_dma_unmap_event(addr);
}

/**
 * @brief Handle DMA allocation event
 *
 * Processes DMA allocation event including device, addresses, size, flags, and attributes.
 */
void dma_alloc_event::handle(ulong addr) {
    // Read basic DMA allocation fields
    arg_list[0]->name = "device";
    arg_list[1]->name = "dma_addr";
    arg_list[2]->name = "size";
    arg_list[3]->name = "virt_addr";
    arg_list[4]->name = "flags";
    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);

    // Decode and store DMA attributes
    std::shared_ptr<trace_field> field_ptr = field_maps["attrs"];
    if (field_ptr) {
        ulong attrs = plugin_ptr->read_ulong(addr + field_ptr->offset, "attrs");
        copy_str(arg_list[5], decode_dma_attrs(attrs));
    }
}

/**
 * @brief Handle DMA free event
 *
 * Processes DMA free event including device, addresses, size, and attributes.
 */
void dma_free_event::handle(ulong addr) {
    // Read basic DMA free fields
    arg_list[0]->name = "device";
    arg_list[1]->name = "dma_addr";
    arg_list[2]->name = "size";
    arg_list[3]->name = "virt_addr";
    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);

    // Decode and store DMA attributes
    std::shared_ptr<trace_field> field_ptr = field_maps["attrs"];
    if (field_ptr) {
        ulong attrs = plugin_ptr->read_ulong(addr + field_ptr->offset, "attrs");
        copy_str(arg_list[4], decode_dma_attrs(attrs));
    }
}

/**
 * @brief Read and format dynamic array
 *
 * Helper function to read and format a dynamic array field.
 *
 * @param addr Base address of event data
 * @param field_maps Map of field names to field descriptors
 * @param plugin_ptr Pointer to plugin for reading memory
 * @param field_name Name of the field to read
 * @param format_func Function to format each element
 * @return Comma-separated string of array values
 */
template<typename T>
static std::string read_dynamic_array(
    ulong addr,
    const std::unordered_map<std::string, std::shared_ptr<trace_field>>& field_maps,
    ParserPlugin* plugin_ptr,
    const std::string& field_name,
    std::function<std::string(T)> format_func) {

    auto field_ptr = field_maps.find(field_name);
    if (field_ptr == field_maps.end() || !field_ptr->second) {
        return "";
    }

    ulong offset = addr + field_ptr->second->offset;
    uint count = field_ptr->second->size / sizeof(T);

    std::string result;
    for (uint i = 0; i < count; i++) {
        if (i > 0) result += ",";
        T val;
        if (sizeof(T) == sizeof(uint64_t)) {
            val = static_cast<T>(plugin_ptr->read_ulong(offset + i * sizeof(T), field_name));
        } else {
            val = static_cast<T>(plugin_ptr->read_uint(offset + i * sizeof(T), field_name));
        }
        result += format_func(val);
    }

    return result;
}

/**
 * @brief Handle DMA scatter-gather map event
 *
 * Processes DMA SG map event including device, direction, DMA addresses, lengths,
 * physical addresses, and attributes.
 */
void dma_map_sg_event::handle(ulong addr) {
    // Read device string
    arg_list[0]->name = "device";
    read_trace_field(addr, arg_list[0]);

    // Decode and store DMA direction
    std::shared_ptr<trace_field> field_ptr = field_maps["dir"];
    if (field_ptr) {
        uint dir = plugin_ptr->read_uint(addr + field_ptr->offset, "dir");
        copy_str(arg_list[1], decode_dma_direction(dir));
    }

    // Read and format DMA addresses array
    if (arg_list[2]) {
        auto format_hex = [](uint64_t val) {
            char buf[32];
            snprintf(buf, sizeof(buf), "0x%llx", (unsigned long long)val);
            return std::string(buf);
        };
        std::string dma_addrs_str = read_dynamic_array<uint64_t>(
            addr, field_maps, plugin_ptr, "dma_addrs", format_hex);
        copy_str(arg_list[2], dma_addrs_str);
    }

    // Read and format lengths array
    if (arg_list[3]) {
        auto format_dec = [](unsigned int val) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%u", val);
            return std::string(buf);
        };
        std::string lengths_str = read_dynamic_array<unsigned int>(
            addr, field_maps, plugin_ptr, "lengths", format_dec);
        copy_str(arg_list[3], lengths_str);
    }

    // Read and format physical addresses array
    if (arg_list[4]) {
        auto format_hex = [](uint64_t val) {
            char buf[32];
            snprintf(buf, sizeof(buf), "0x%llx", (unsigned long long)val);
            return std::string(buf);
        };
        std::string phys_addrs_str = read_dynamic_array<uint64_t>(
            addr, field_maps, plugin_ptr, "phys_addrs", format_hex);
        copy_str(arg_list[4], phys_addrs_str);
    }

    // Decode and store DMA attributes
    field_ptr = field_maps["attrs"];
    if (field_ptr && arg_list[5]) {
        ulong attrs = plugin_ptr->read_ulong(addr + field_ptr->offset, "attrs");
        static const flag_map maps[] = {
            { 1UL << 1, "WEAK_ORDERING" },
            { 1UL << 2, "WRITE_COMBINE" },
            { 1UL << 4, "NO_KERNEL_MAPPING" },
            { 1UL << 5, "SKIP_CPU_SYNC" },
            { 1UL << 6, "FORCE_CONTIGUOUS" },
            { 1UL << 7, "ALLOC_SINGLE_PAGES" },
            { 1UL << 8, "NO_WARN" },
            { 1UL << 9, "PRIVILEGED" },
        };
        copy_str(arg_list[5], build_flag_string(attrs, maps, 8));
    }
}

/**
 * @brief Handle DMA scatter-gather unmap event
 *
 * Processes DMA SG unmap event including device, direction, addresses, and attributes.
 */
void dma_unmap_sg_event::handle(ulong addr) {
    // Read device string
    arg_list[0]->name = "device";
    read_trace_field(addr, arg_list[0]);

    // Decode and store DMA direction
    std::shared_ptr<trace_field> field_ptr = field_maps["dir"];
    if (field_ptr) {
        uint dir = plugin_ptr->read_uint(addr + field_ptr->offset, "dir");
        copy_str(arg_list[1], decode_dma_direction(dir));
    }

    // Read and format addresses array
    if (arg_list[2]) {
        auto format_hex = [](uint64_t val) {
            char buf[32];
            snprintf(buf, sizeof(buf), "0x%llx", (unsigned long long)val);
            return std::string(buf);
        };
        std::string addrs_str = read_dynamic_array<uint64_t>(
            addr, field_maps, plugin_ptr, "addrs", format_hex);
        copy_str(arg_list[2], addrs_str);
    }

    // Decode and store DMA attributes
    field_ptr = field_maps["attrs"];
    if (field_ptr && arg_list[3]) {
        ulong attrs = plugin_ptr->read_ulong(addr + field_ptr->offset, "attrs");
        static const flag_map maps[] = {
            { 1UL << 1, "WEAK_ORDERING" },
            { 1UL << 2, "WRITE_COMBINE" },
            { 1UL << 4, "NO_KERNEL_MAPPING" },
            { 1UL << 5, "SKIP_CPU_SYNC" },
            { 1UL << 6, "FORCE_CONTIGUOUS" },
            { 1UL << 7, "ALLOC_SINGLE_PAGES" },
            { 1UL << 8, "NO_WARN" },
            { 1UL << 9, "PRIVILEGED" },
        };
        copy_str(arg_list[3], build_flag_string(attrs, maps, 8));
    }
}

/**
 * @brief Handle DMA sync single for CPU event
 *
 * Processes DMA sync for CPU event including device, direction, DMA address, and size.
 */
void dma_sync_single_for_cpu_event::handle(ulong addr) {
    // Read device string
    arg_list[0]->name = "device";
    read_trace_field(addr, arg_list[0]);

    // Decode and store DMA direction
    std::shared_ptr<trace_field> field_ptr = field_maps["dir"];
    if (field_ptr) {
        uint dir = plugin_ptr->read_uint(addr + field_ptr->offset, "dir");
        copy_str(arg_list[1], decode_dma_direction(dir));
    }

    // Read DMA address and size
    arg_list[2]->name = "dma_addr";
    arg_list[3]->name = "size";
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
}

/**
 * @brief Handle DMA sync single for device event
 *
 * Processes DMA sync for device event including device, direction, DMA address, and size.
 */
void dma_sync_single_for_device_event::handle(ulong addr) {
    // Read device string
    arg_list[0]->name = "device";
    read_trace_field(addr, arg_list[0]);

    // Decode and store DMA direction
    std::shared_ptr<trace_field> field_ptr = field_maps["dir"];
    if (field_ptr) {
        uint dir = plugin_ptr->read_uint(addr + field_ptr->offset, "dir");
        copy_str(arg_list[1], decode_dma_direction(dir));
    }

    // Read DMA address and size
    arg_list[2]->name = "dma_addr";
    arg_list[3]->name = "size";
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
}

/**
 * @brief Handle DMA sync scatter-gather for CPU event
 *
 * Processes DMA SG sync for CPU event including device, direction, DMA addresses, and lengths.
 */
void dma_sync_sg_for_cpu_event::handle(ulong addr) {
    // Read device string
    arg_list[0]->name = "device";
    read_trace_field(addr, arg_list[0]);

    // Decode and store DMA direction
    std::shared_ptr<trace_field> field_ptr = field_maps["dir"];
    if (field_ptr) {
        uint dir = plugin_ptr->read_uint(addr + field_ptr->offset, "dir");
        copy_str(arg_list[1], decode_dma_direction(dir));
    }

    // Read and format DMA addresses array
    if (arg_list[2]) {
        auto format_hex = [](uint64_t val) {
            char buf[32];
            snprintf(buf, sizeof(buf), "0x%llx", (unsigned long long)val);
            return std::string(buf);
        };
        std::string dma_addrs_str = read_dynamic_array<uint64_t>(
            addr, field_maps, plugin_ptr, "dma_addrs", format_hex);
        copy_str(arg_list[2], dma_addrs_str);
    }

    // Read and format lengths array
    if (arg_list[3]) {
        auto format_dec = [](unsigned int val) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%u", val);
            return std::string(buf);
        };
        std::string lengths_str = read_dynamic_array<unsigned int>(
            addr, field_maps, plugin_ptr, "lengths", format_dec);
        copy_str(arg_list[3], lengths_str);
    }
}

/**
 * @brief Handle DMA sync scatter-gather for device event
 *
 * Processes DMA SG sync for device event including device, direction, DMA addresses, and lengths.
 */
void dma_sync_sg_for_device_event::handle(ulong addr) {
    // Read device string
    arg_list[0]->name = "device";
    read_trace_field(addr, arg_list[0]);

    // Decode and store DMA direction
    std::shared_ptr<trace_field> field_ptr = field_maps["dir"];
    if (field_ptr) {
        uint dir = plugin_ptr->read_uint(addr + field_ptr->offset, "dir");
        copy_str(arg_list[1], decode_dma_direction(dir));
    }

    // Read and format DMA addresses array
    if (arg_list[2]) {
        auto format_hex = [](uint64_t val) {
            char buf[32];
            snprintf(buf, sizeof(buf), "0x%llx", (unsigned long long)val);
            return std::string(buf);
        };
        std::string dma_addrs_str = read_dynamic_array<uint64_t>(
            addr, field_maps, plugin_ptr, "dma_addrs", format_hex);
        copy_str(arg_list[2], dma_addrs_str);
    }

    // Read and format lengths array
    if (arg_list[3]) {
        auto format_dec = [](unsigned int val) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%u", val);
            return std::string(buf);
        };
        std::string lengths_str = read_dynamic_array<unsigned int>(
            addr, field_maps, plugin_ptr, "lengths", format_dec);
        copy_str(arg_list[3], lengths_str);
    }
}

/**
 * @brief Handle SWIOTLB bounced event
 *
 * Processes SWIOTLB bounce buffer event including device name, DMA mask,
 * device address, size, and force flag.
 */
void swiotlb_bounced_event::handle(ulong addr) {
    // Read basic SWIOTLB fields
    arg_list[0]->name = "dev_name";
    arg_list[1]->name = "dma_mask";
    arg_list[2]->name = "dev_addr";
    arg_list[3]->name = "size";
    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);

    // Decode force flag
    std::shared_ptr<trace_field> field_ptr = field_maps["force"];
    if (field_ptr) {
        int force = plugin_ptr->read_int(addr + field_ptr->offset, "force");
        copy_str(arg_list[4], force ? "FORCE" : "NORMAL");
    }
}

/**
 * @brief Handle DWC3 event
 *
 * Processes DWC3 generic event including event value and string representation.
 */
void dwc3_event_event::handle(ulong addr) {
    arg_list[0]->name = "event";
    arg_list[1]->name = "str";
    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
}

/**
 * @brief Decode DWC3 endpoint command to string
 *
 * @param cmd Command value
 * @return String representation of command
 */
static std::string decode_dwc3_ep_cmd(uint cmd) {
    switch (cmd) {
        case DWC3_DEPCMD_DEPSTARTCFG: return "Start New Configuration";
        case DWC3_DEPCMD_ENDTRANSFER: return "End Transfer";
        case DWC3_DEPCMD_UPDATETRANSFER: return "Update Transfer";
        case DWC3_DEPCMD_STARTTRANSFER: return "Start Transfer";
        case DWC3_DEPCMD_CLEARSTALL: return "Clear Stall";
        case DWC3_DEPCMD_SETSTALL: return "Set Stall";
        case DWC3_DEPCMD_GETEPSTATE: return "Get Endpoint State";
        case DWC3_DEPCMD_SETTRANSFRESOURCE: return "Set Endpoint Transfer Resource";
        case DWC3_DEPCMD_SETEPCONFIG: return "Set Endpoint Configuration";
        default: return "UNKNOWN command";
    }
}

/**
 * @brief Decode DWC3 endpoint command status to string
 *
 * @param cmd_status Command status value
 * @return String representation of command status
 */
static std::string decode_dwc3_ep_cmd_status(int cmd_status) {
    switch (cmd_status) {
        case -ETIMEDOUT: return "Timed Out";
        case 0: return "Successful";
        case DEPEVT_TRANSFER_NO_RESOURCE: return "No Resource";
        case DEPEVT_TRANSFER_BUS_EXPIRY: return "Bus Expiry";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Handle DWC3 gadget endpoint command event
 *
 * Processes DWC3 endpoint command event including endpoint name, command type,
 * parameters, and command status.
 */
void dwc3_gadget_ep_cmd_event::handle(ulong addr) {
    // Read endpoint name
    arg_list[0]->name = "name";
    read_trace_field(addr, arg_list[0]);

    // Decode command type
    std::shared_ptr<trace_field> field_ptr = field_maps["cmd"];
    if (field_ptr) {
        uint cmd = plugin_ptr->read_uint(addr + field_ptr->offset, "cmd");
        copy_str(arg_list[1], decode_dwc3_ep_cmd(cmd));
    }

    // Read command value and parameters
    arg_list[2]->name = "cmd";
    arg_list[3]->name = "param0";
    arg_list[4]->name = "param1";
    arg_list[5]->name = "param2";
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);

    // Decode command status
    field_ptr = field_maps["cmd_status"];
    if (field_ptr) {
        int cmd_status = plugin_ptr->read_int(addr + field_ptr->offset, "cmd_status");
        copy_str(arg_list[6], decode_dwc3_ep_cmd_status(cmd_status));
    }
}

/**
 * @brief Handle bprint event
 * fmt: "%ps: %s", (void *)REC->ip, REC->fmt
 * TODO: Implement binary print event handling
 */
void bprint_event::handle(ulong addr) {
    arg_list[0]->name = "ip";
    read_trace_field(addr, arg_list[0]);

    arg_list[1]->name = "fmt";
    read_trace_field(addr, arg_list[1]);
}

/**
 * @brief Handle print event
 * fmt: "%ps: %s", (void *)REC->ip, REC->buf
 */
void print_event::handle(ulong addr) {
    arg_list[0]->name = "ip";
    read_trace_field(addr, arg_list[0]);

    arg_list[1]->name = "buf";
    read_trace_field(addr, arg_list[1]);
}

/**
 * @brief Handle bputs event
 * fmt: bputs: "%ps: %s", (void *)REC->ip, REC->str
 */
void bputs_event::handle(ulong addr) {
    arg_list[0]->name = "ip";
    read_trace_field(addr, arg_list[0]);

    arg_list[1]->name = "str";
    read_trace_field(addr, arg_list[1]);
}

/**
 * @brief Handle kernel stack event
 * fmt:"\t=> %ps\n\t=> %ps\n\t=> %ps\n" "\t=> %ps\n\t=> %ps\n\t=> %ps\n" "\t=> %ps\n\t=> %ps\n", (void *)REC->caller[0], (void *)REC->caller[1], (void *)REC->caller[2], (void *)REC->caller[3], (void *)REC->caller[4], (void *)REC->caller[5], (void *)REC->caller[6], (void *)REC->caller[7]
 *
 * struct stack_entry {
 *     struct trace_entry ent;
 *     int size;
 *     unsigned long caller[];
 * }
 */
void kernel_stack_event::handle(ulong addr) {
    // Read the size field to determine how many stack frames we have
    std::shared_ptr<trace_field> field_ptr = field_maps["size"];
    if (!field_ptr) return;

    int size = plugin_ptr->read_int(addr + field_ptr->offset, "size");

    // Read the caller array field
    field_ptr = field_maps["caller"];
    if (!field_ptr) return;

    // Limit to maximum 8 callers as shown in the format string
    int max_callers = (size > 8) ? 8 : size;

    // Read each caller address and resolve to symbol
    for (int i = 0; i < max_callers && i < 8; i++) {
        unsigned long caller_addr = plugin_ptr->read_ulong(addr + field_ptr->offset + i * sizeof(unsigned long), "caller");
        if (is_kvaddr(caller_addr)) {
            ulong offset;
            struct syment *sp = value_search(caller_addr, &offset);
            if (sp) {
                copy_str(arg_list[i], sp->name);
            }else{
                char addr_str[32];
                snprintf(addr_str, sizeof(addr_str), "0x%lx", caller_addr);
                copy_str(arg_list[i], addr_str);
            }
        }else{
            char addr_str[32];
            snprintf(addr_str, sizeof(addr_str), "0x%lx", caller_addr);
            copy_str(arg_list[i], addr_str);
        }
    }
}

/**
 * @brief Handle user stack event
 * fmt: "\t=> %ps\n\t=> %ps\n\t=> %ps\n" "\t=> %ps\n\t=> %ps\n\t=> %ps\n" "\t=> %ps\n\t=> %ps\n", (void *)REC->caller[0], (void *)REC->caller[1], (void *)REC->caller[2], (void *)REC->caller[3], (void *)REC->caller[4], (void *)REC->caller[5], (void *)REC->caller[6], (void *)REC->caller[7]
 *
 * struct userstack_entry {
 *     struct trace_entry ent;
 *     unsigned int tgid;
 *     unsigned long caller[8];
 * }
 */
void user_stack_event::handle(ulong addr) {
    // Read the caller array field
    std::shared_ptr<trace_field> field_ptr = field_maps["caller"];
    if (!field_ptr) return;

    // Read all 8 caller addresses from the fixed-size array
    for (int i = 0; i < 8; i++) {
        unsigned long caller_addr = plugin_ptr->read_ulong(addr + field_ptr->offset + i * sizeof(unsigned long), "caller");
        char addr_str[32];
        snprintf(addr_str, sizeof(addr_str), "0x%lx", caller_addr);
        copy_str(arg_list[i], addr_str);
    }
}

/**
 * @brief Handle GPIO value event
 *
 * Processes GPIO value event including GPIO number, operation type (get/set), and value.
 */
void gpio_value_event::handle(ulong addr) {
    // Read GPIO number
    arg_list[0]->name = "gpio";
    read_trace_field(addr, arg_list[0]);

    // Determine operation type (get or set)
    std::shared_ptr<trace_field> field_ptr = field_maps["get"];
    if (field_ptr) {
        int get = plugin_ptr->read_int(addr + field_ptr->offset, "get");
        copy_str(arg_list[1], get ? "get" : "set");
    }

    // Read GPIO value
    arg_list[2]->name = "value";
    read_trace_field(addr, arg_list[2]);
}

/**
 * @brief Handle GPIO direction event
 *
 * Processes GPIO direction event including GPIO number, direction (in/out), and error status.
 */
void gpio_direction_event::handle(ulong addr) {
    // Read GPIO number
    arg_list[0]->name = "gpio";
    read_trace_field(addr, arg_list[0]);

    // Determine direction (input or output)
    std::shared_ptr<trace_field> field_ptr = field_maps["in"];
    if (field_ptr) {
        int in = plugin_ptr->read_int(addr + field_ptr->offset, "in");
        copy_str(arg_list[1], in ? "in" : "out");
    }

    // Read error status
    arg_list[2]->name = "err";
    read_trace_field(addr, arg_list[2]);
}

/**
 * @brief Handle read MMIO event
 *
 * Processes MMIO read event including caller address, width, and address.
 */
void rwmmio_read_event::handle(ulong addr) {
    arg_list[0]->name = "caller0";
    arg_list[1]->name = "caller";
    arg_list[2]->name = "width";
    arg_list[3]->name = "addr";
    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
}

void rwmmio_read_event::print_log(std::ostringstream& oss) {
    for (const auto& arg_ptr : arg_list) {
        oss << format_arg(arg_ptr);
    }
    ulong addr_value = *reinterpret_cast<ulong*>(arg_list[3]->data);
    physaddr_t paddr = 0;
    if (kvtop(NULL, addr_value, &paddr, 0)) {
        oss << "(0x" << std::hex << paddr << ")";
    } else {
        oss << "(N/A)";
    }
}

/**
 * @brief Handle write MMIO event
 *
 * Processes MMIO write event including caller address, width, value, and address.
 */
void rwmmio_write_event::handle(ulong addr) {
    arg_list[0]->name = "caller0";
    arg_list[1]->name = "caller";
    arg_list[2]->name = "width";
    arg_list[3]->name = "val";
    arg_list[4]->name = "addr";
    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
}

void rwmmio_write_event::print_log(std::ostringstream& oss) {
    print_rwmmio_event(oss);
}

/**
 * @brief Handle post-write MMIO event
 *
 * Processes MMIO post-write event including caller address, width, value, and address.
 */
void rwmmio_post_write_event::handle(ulong addr) {
    arg_list[0]->name = "caller0";
    arg_list[1]->name = "caller";
    arg_list[2]->name = "width";
    arg_list[3]->name = "val";
    arg_list[4]->name = "addr";
    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
}

void rwmmio_post_write_event::print_log(std::ostringstream& oss) {
    print_rwmmio_event(oss);
}

/**
 * @brief Handle post-read MMIO event
 *
 * Processes MMIO post-read event including caller address, width, value, and address.
 */
void rwmmio_post_read_event::handle(ulong addr) {
    arg_list[0]->name = "caller0";
    arg_list[1]->name = "caller";
    arg_list[2]->name = "width";
    arg_list[3]->name = "val";
    arg_list[4]->name = "addr";
    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
}

void rwmmio_post_read_event::print_log(std::ostringstream& oss) {
    print_rwmmio_event(oss);
}

// Binder return command strings
static const char * const binder_return_strings[] = {
    "BR_ERROR",
    "BR_OK",
    "BR_TRANSACTION",
    "BR_REPLY",
    "BR_ACQUIRE_RESULT",
    "BR_DEAD_REPLY",
    "BR_TRANSACTION_COMPLETE",
    "BR_INCREFS",
    "BR_ACQUIRE",
    "BR_RELEASE",
    "BR_DECREFS",
    "BR_ATTEMPT_ACQUIRE",
    "BR_NOOP",
    "BR_SPAWN_LOOPER",
    "BR_FINISHED",
    "BR_DEAD_BINDER",
    "BR_CLEAR_DEATH_NOTIFICATION_DONE",
    "BR_FAILED_REPLY",
    "BR_FROZEN_REPLY",
    "BR_ONEWAY_SPAM_SUSPECT",
    "BR_TRANSACTION_PENDING_FROZEN"
};

// Binder command strings
static const char * const binder_command_strings[] = {
    "BC_TRANSACTION",
    "BC_REPLY",
    "BC_ACQUIRE_RESULT",
    "BC_FREE_BUFFER",
    "BC_INCREFS",
    "BC_ACQUIRE",
    "BC_RELEASE",
    "BC_DECREFS",
    "BC_INCREFS_DONE",
    "BC_ACQUIRE_DONE",
    "BC_ATTEMPT_ACQUIRE",
    "BC_REGISTER_LOOPER",
    "BC_ENTER_LOOPER",
    "BC_EXIT_LOOPER",
    "BC_REQUEST_DEATH_NOTIFICATION",
    "BC_CLEAR_DEATH_NOTIFICATION",
    "BC_DEAD_BINDER_DONE",
    "BC_TRANSACTION_SG",
    "BC_REPLY_SG",
};

/**
 * @brief Handle binder return event
 *
 * Processes binder return event including command value and string representation.
 */
void binder_return_event::handle(ulong addr) {
    arg_list[0]->name = "cmd";
    read_trace_field(addr, arg_list[0]);

    unsigned int cmd = *reinterpret_cast<unsigned int*>(arg_list[0]->data);
    size_t array_size = sizeof(binder_return_strings) / sizeof(binder_return_strings[0]);

    std::string cmd_string;
    if (_IOC_NR(cmd) < array_size) {
        cmd_string = binder_return_strings[_IOC_NR(cmd)];
    } else {
        cmd_string = "UNKNOWN";
    }

    copy_str(arg_list[1], cmd_string);
}

/**
 * @brief Handle binder command event
 *
 * Processes binder command event including command value and string representation.
 */
void binder_command_event::handle(ulong addr) {
    arg_list[0]->name = "cmd";
    read_trace_field(addr, arg_list[0]);

    unsigned int cmd = *reinterpret_cast<unsigned int*>(arg_list[0]->data);
    size_t array_size = sizeof(binder_command_strings) / sizeof(binder_command_strings[0]);

    std::string cmd_string;
    if (_IOC_NR(cmd) < array_size) {
        cmd_string = binder_command_strings[_IOC_NR(cmd)];
    } else {
        cmd_string = "UNKNOWN";
    }

    copy_str(arg_list[1], cmd_string);
}

/**
 * @brief Handle softirq entry event
 */
void softirq_entry_event::handle(ulong addr) {
    print_softirq(addr);
}

/**
 * @brief Handle softirq exit event
 */
void softirq_exit_event::handle(ulong addr) {
    print_softirq(addr);
}

/**
 * @brief Handle softirq raise event
 */
void softirq_raise_event::handle(ulong addr) {
    print_softirq(addr);
}

/**
 * @brief Handle scheduler switch event
 *
 * Processes scheduler context switch event including previous and next task
 * information (comm, pid, priority, state).
 */
void sched_switch_event::handle(ulong addr) {
    // Read previous task information
    arg_list[0]->name = "prev_comm";
    arg_list[1]->name = "prev_pid";
    arg_list[2]->name = "prev_prio";
    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);

    // Decode previous task state
    if (arg_list[3]) {
        std::shared_ptr<trace_field> field_ptr = field_maps["prev_state"];
        if (field_ptr) {
            long prev_state = plugin_ptr->read_long(addr + field_ptr->offset, "prev_state");
            uint16_t mask = (((0x0000 | 0x0001 | 0x0002 | 0x0004 | 0x0008 |
                               0x0010 | 0x0020 | 0x0040) + 1) << 1) - 1;
            copy_str(arg_list[3], decode_task_state(prev_state, mask));
        }
    }

    // Read next task information
    arg_list[5]->name = "next_comm";
    arg_list[6]->name = "next_pid";
    arg_list[7]->name = "next_prio";
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);
    read_trace_field(addr, arg_list[7]);
}

/**
 * @brief Handle IRQ handler exit event
 *
 * Processes IRQ handler exit event including IRQ number and return status.
 */
void irq_handler_exit_event::handle(ulong addr) {
    // Read IRQ number
    arg_list[0]->name = "irq";
    read_trace_field(addr, arg_list[0]);

    // Decode and store return status
    if (arg_list[1]) {
        std::shared_ptr<trace_field> field_ptr = field_maps["ret"];
        if (field_ptr) {
            int ret = plugin_ptr->read_int(addr + field_ptr->offset, "ret");
            copy_str(arg_list[1], ret ? "handled" : "unhandled");
        }
    }
}

/**
 * @brief Handle alarmtimer suspend event
 * Format: "alarmtimer type:%s expires:%llu"
 */
void alarmtimer_suspend_event::handle(ulong addr){
    // Decode alarm_type
    std::shared_ptr<trace_field> field_ptr = field_maps["alarm_type"];
    if (!field_ptr) return;
    uint alarm_type = plugin_ptr->read_uint(addr + field_ptr->offset, "alarm_type");
    copy_str(arg_list[0], decode_alarm_type(alarm_type));

    // Read expires
    arg_list[1]->name = "expires";
    read_trace_field(addr, arg_list[1]);
}

/**
 * @brief Handle alarmtimer fired event
 * Format: "alarmtimer:%p type:%s expires:%llu now:%llu"
 */
void alarmtimer_fired_event::handle(ulong addr){
    // Read alarm pointer
    arg_list[0]->name = "alarm";
    read_trace_field(addr, arg_list[0]);

    // Decode alarm_type
    std::shared_ptr<trace_field> field_ptr = field_maps["alarm_type"];
    if (!field_ptr) return;
    uint alarm_type = plugin_ptr->read_uint(addr + field_ptr->offset, "alarm_type");
    copy_str(arg_list[1], decode_alarm_type(alarm_type));

    // Read expires
    arg_list[2]->name = "expires";
    read_trace_field(addr, arg_list[2]);

    // Read now
    arg_list[3]->name = "now";
    read_trace_field(addr, arg_list[3]);
}

/**
 * @brief Handle alarmtimer start event
 * Format: "alarmtimer:%p type:%s expires:%llu now:%llu"
 */
void alarmtimer_start_event::handle(ulong addr){
    // Read alarm pointer
    arg_list[0]->name = "alarm";
    read_trace_field(addr, arg_list[0]);

    // Decode alarm_type
    std::shared_ptr<trace_field> field_ptr = field_maps["alarm_type"];
    if (!field_ptr) return;
    uint alarm_type = plugin_ptr->read_uint(addr + field_ptr->offset, "alarm_type");
    copy_str(arg_list[1], decode_alarm_type(alarm_type));

    // Read expires
    arg_list[2]->name = "expires";
    read_trace_field(addr, arg_list[2]);

    // Read now
    arg_list[3]->name = "now";
    read_trace_field(addr, arg_list[3]);
}

/**
 * @brief Handle alarmtimer cancel event
 * Format: "alarmtimer:%p type:%s expires:%llu now:%llu"
 */
void alarmtimer_cancel_event::handle(ulong addr){
    // Read alarm pointer
    arg_list[0]->name = "alarm";
    read_trace_field(addr, arg_list[0]);

    // Decode alarm_type
    std::shared_ptr<trace_field> field_ptr = field_maps["alarm_type"];
    if (!field_ptr) return;
    uint alarm_type = plugin_ptr->read_uint(addr + field_ptr->offset, "alarm_type");
    copy_str(arg_list[1], decode_alarm_type(alarm_type));

    // Read expires
    arg_list[2]->name = "expires";
    read_trace_field(addr, arg_list[2]);

    // Read now
    arg_list[3]->name = "now";
    read_trace_field(addr, arg_list[3]);
}

/**
 * @brief Handle CPU idle miss event
 * Format: "cpu_id=%lu state=%lu type=%s"
 */
void cpu_idle_miss_event::handle(ulong addr){
    // Read cpu_id
    arg_list[0]->name = "cpu_id";
    read_trace_field(addr, arg_list[0]);

    // Read state
    arg_list[1]->name = "state";
    read_trace_field(addr, arg_list[1]);

    // Decode below flag to determine type
    std::shared_ptr<trace_field> field_ptr = field_maps["below"];
    if (!field_ptr) return;
    int below = plugin_ptr->read_int(addr + field_ptr->offset, "below");
    copy_str(arg_list[2], below ? "below" : "above");
}

/**
 * @brief Handle suspend/resume event
 * Format: "%s[%u] %s"
 */
void suspend_resume_event::handle(ulong addr){
    // Read action string
    arg_list[0]->name = "action";
    read_trace_field(addr, arg_list[0]);

    // Read val
    arg_list[1]->name = "val";
    read_trace_field(addr, arg_list[1]);

    // Decode start flag to determine begin/end
    std::shared_ptr<trace_field> field_ptr = field_maps["start"];
    if (!field_ptr) return;
    int start = plugin_ptr->read_int(addr + field_ptr->offset, "start");
    copy_str(arg_list[2], start ? "begin" : "end");
}

/**
 * @brief Handle mm_lru_insertion event
 * Format: "page=%p pfn=0x%lx lru=%d flags=%s%s%s%s%s%s"
 */
void mm_lru_insertion_event::handle(ulong addr){
    // Read page pointer
    arg_list[0]->name = "page";
    read_trace_field(addr, arg_list[0]);

    // Read pfn (page frame number)
    arg_list[1]->name = "pfn";
    read_trace_field(addr, arg_list[1]);

    // Read lru
    arg_list[2]->name = "lru";
    read_trace_field(addr, arg_list[2]);

    // Decode flags
    std::shared_ptr<trace_field> field_ptr = field_maps["flags"];
    if (!field_ptr) return;
    unsigned int flags = plugin_ptr->read_uint(addr + field_ptr->offset, "flags");
    copy_str(arg_list[3], decode_lru_flags(flags));
}

/**
 * @brief Handle mm_vmscan_wakeup_kswapd event
 * Format: "nid=%d order=%d gfp_flags=%s"
 */
void mm_vmscan_wakeup_kswapd_event::handle(ulong addr){
    // Read nid (NUMA node ID)
    arg_list[0]->name = "nid";
    read_trace_field(addr, arg_list[0]);

    // Read order (allocation order)
    arg_list[1]->name = "order";
    read_trace_field(addr, arg_list[1]);

    // Decode gfp_flags
    std::shared_ptr<trace_field> field_ptr = field_maps["gfp_flags"];
    if (!field_ptr) return;
    unsigned long gfp_flags = plugin_ptr->read_ulong(addr + field_ptr->offset, "gfp_flags");
    copy_str(arg_list[2], decode_gfp_flags(gfp_flags));
}

/**
 * @brief Handle mm_vmscan_direct_reclaim_begin event
 * Format: "order=%d gfp_flags=%s"
 */
void mm_vmscan_direct_reclaim_begin_event::handle(ulong addr){
    // Read order (allocation order)
    arg_list[0]->name = "order";
    read_trace_field(addr, arg_list[0]);

    // Decode gfp_flags
    std::shared_ptr<trace_field> field_ptr = field_maps["gfp_flags"];
    if (!field_ptr) return;
    unsigned long gfp_flags = plugin_ptr->read_ulong(addr + field_ptr->offset, "gfp_flags");
    copy_str(arg_list[1], decode_gfp_flags(gfp_flags));
}

/**
 * @brief Handle mm_vmscan_memcg_reclaim_begin event
 * Format: "order=%d gfp_flags=%s"
 */
void mm_vmscan_memcg_reclaim_begin_event::handle(ulong addr){
    // Read order (allocation order)
    arg_list[0]->name = "order";
    read_trace_field(addr, arg_list[0]);

    // Decode gfp_flags
    std::shared_ptr<trace_field> field_ptr = field_maps["gfp_flags"];
    if (!field_ptr) return;
    unsigned long gfp_flags = plugin_ptr->read_ulong(addr + field_ptr->offset, "gfp_flags");
    copy_str(arg_list[1], decode_gfp_flags(gfp_flags));
}

/**
 * @brief Handle mm_vmscan_memcg_softlimit_reclaim_begin event
 * Format: "order=%d gfp_flags=%s"
 */
void mm_vmscan_memcg_softlimit_reclaim_begin_event::handle(ulong addr){
    // Read order (allocation order)
    arg_list[0]->name = "order";
    read_trace_field(addr, arg_list[0]);

    // Decode gfp_flags
    std::shared_ptr<trace_field> field_ptr = field_maps["gfp_flags"];
    if (!field_ptr) return;
    unsigned long gfp_flags = plugin_ptr->read_ulong(addr + field_ptr->offset, "gfp_flags");
    copy_str(arg_list[1], decode_gfp_flags(gfp_flags));
}

/**
 * @brief Handle mm_vmscan_node_reclaim_begin event
 * Format: "nid=%d order=%d gfp_flags=%s"
 */
void mm_vmscan_node_reclaim_begin_event::handle(ulong addr){
    // Read nid (NUMA node ID)
    arg_list[0]->name = "nid";
    read_trace_field(addr, arg_list[0]);

    // Read order (allocation order)
    arg_list[1]->name = "order";
    read_trace_field(addr, arg_list[1]);

    // Decode gfp_flags
    std::shared_ptr<trace_field> field_ptr = field_maps["gfp_flags"];
    if (!field_ptr) return;
    unsigned long gfp_flags = plugin_ptr->read_ulong(addr + field_ptr->offset, "gfp_flags");
    copy_str(arg_list[2], decode_gfp_flags(gfp_flags));
}

/**
 * @brief Handle mm_shrink_slab_start event
 * Format: "%pS %p: nid: %d objects to shrink %ld gfp_flags %s cache items %ld delta %lld total_scan %ld priority %d"
 */
void mm_shrink_slab_start_event::handle(ulong addr){
    // Read shrink function pointer (%pS - symbol)
    arg_list[0]->name = "shrink";
    read_trace_field(addr, arg_list[0]);

    // Read shr pointer
    arg_list[1]->name = "shr";
    read_trace_field(addr, arg_list[1]);

    // Read nid (NUMA node ID)
    arg_list[2]->name = "nid";
    read_trace_field(addr, arg_list[2]);

    // Read nr_objects_to_shrink
    arg_list[3]->name = "nr_objects_to_shrink";
    read_trace_field(addr, arg_list[3]);

    // Decode gfp_flags
    std::shared_ptr<trace_field> field_ptr = field_maps["gfp_flags"];
    if (!field_ptr) return;
    unsigned long gfp_flags = plugin_ptr->read_ulong(addr + field_ptr->offset, "gfp_flags");
    copy_str(arg_list[4], decode_gfp_flags(gfp_flags));

    // Read cache_items
    arg_list[5]->name = "cache_items";
    read_trace_field(addr, arg_list[5]);

    // Read delta
    arg_list[6]->name = "delta";
    read_trace_field(addr, arg_list[6]);

    // Read total_scan
    arg_list[7]->name = "total_scan";
    read_trace_field(addr, arg_list[7]);

    // Read priority
    arg_list[8]->name = "priority";
    read_trace_field(addr, arg_list[8]);
}

/**
 * @brief Handle kmalloc event
 * Format: "call_site=%pS ptr=%p bytes_req=%zu bytes_alloc=%zu gfp_flags=%s"
 */
void kmalloc_event::handle(ulong addr){
    // Read call_site (%pS - symbol)
    arg_list[0]->name = "call_site";
    read_trace_field(addr, arg_list[0]);

    // Read ptr (allocated pointer)
    arg_list[1]->name = "ptr";
    read_trace_field(addr, arg_list[1]);

    // Read bytes_req (requested bytes)
    arg_list[2]->name = "bytes_req";
    read_trace_field(addr, arg_list[2]);

    // Read bytes_alloc (allocated bytes)
    arg_list[3]->name = "bytes_alloc";
    read_trace_field(addr, arg_list[3]);

    // Decode gfp_flags
    std::shared_ptr<trace_field> field_ptr = field_maps["gfp_flags"];
    if (!field_ptr) return;
    unsigned long gfp_flags = plugin_ptr->read_ulong(addr + field_ptr->offset, "gfp_flags");
    copy_str(arg_list[4], decode_gfp_flags(gfp_flags));
}

/**
 * @brief Handle kmem_cache_alloc event
 * Format: "call_site=%pS ptr=%p bytes_req=%zu bytes_alloc=%zu gfp_flags=%s"
 */
void kmem_cache_alloc_event::handle(ulong addr){
    // Read call_site (%pS - symbol)
    arg_list[0]->name = "call_site";
    read_trace_field(addr, arg_list[0]);

    // Read ptr (allocated pointer)
    arg_list[1]->name = "ptr";
    read_trace_field(addr, arg_list[1]);

    // Read bytes_req (requested bytes)
    arg_list[2]->name = "bytes_req";
    read_trace_field(addr, arg_list[2]);

    // Read bytes_alloc (allocated bytes)
    arg_list[3]->name = "bytes_alloc";
    read_trace_field(addr, arg_list[3]);

    // Decode gfp_flags
    std::shared_ptr<trace_field> field_ptr = field_maps["gfp_flags"];
    if (!field_ptr) return;
    unsigned long gfp_flags = plugin_ptr->read_ulong(addr + field_ptr->offset, "gfp_flags");
    copy_str(arg_list[4], decode_gfp_flags(gfp_flags));
}

/**
 * @brief Handle kmalloc_node event
 * Format: "call_site=%pS ptr=%p bytes_req=%zu bytes_alloc=%zu gfp_flags=%s node=%d"
 */
void kmalloc_node_event::handle(ulong addr){
    // Read call_site (%pS - symbol)
    arg_list[0]->name = "call_site";
    read_trace_field(addr, arg_list[0]);

    // Read ptr (allocated pointer)
    arg_list[1]->name = "ptr";
    read_trace_field(addr, arg_list[1]);

    // Read bytes_req (requested bytes)
    arg_list[2]->name = "bytes_req";
    read_trace_field(addr, arg_list[2]);

    // Read bytes_alloc (allocated bytes)
    arg_list[3]->name = "bytes_alloc";
    read_trace_field(addr, arg_list[3]);

    // Decode gfp_flags
    std::shared_ptr<trace_field> field_ptr = field_maps["gfp_flags"];
    if (!field_ptr) return;
    unsigned long gfp_flags = plugin_ptr->read_ulong(addr + field_ptr->offset, "gfp_flags");
    copy_str(arg_list[4], decode_gfp_flags(gfp_flags));

    // Read node (NUMA node)
    arg_list[5]->name = "node";
    read_trace_field(addr, arg_list[5]);
}

/**
 * @brief Handle kmem_cache_alloc_node event
 * Format: "call_site=%pS ptr=%p bytes_req=%zu bytes_alloc=%zu gfp_flags=%s node=%d"
 */
void kmem_cache_alloc_node_event::handle(ulong addr){
    // Read call_site (%pS - symbol)
    arg_list[0]->name = "call_site";
    read_trace_field(addr, arg_list[0]);

    // Read ptr (allocated pointer)
    arg_list[1]->name = "ptr";
    read_trace_field(addr, arg_list[1]);

    // Read bytes_req (requested bytes)
    arg_list[2]->name = "bytes_req";
    read_trace_field(addr, arg_list[2]);

    // Read bytes_alloc (allocated bytes)
    arg_list[3]->name = "bytes_alloc";
    read_trace_field(addr, arg_list[3]);

    // Decode gfp_flags
    std::shared_ptr<trace_field> field_ptr = field_maps["gfp_flags"];
    if (!field_ptr) return;
    unsigned long gfp_flags = plugin_ptr->read_ulong(addr + field_ptr->offset, "gfp_flags");
    copy_str(arg_list[4], decode_gfp_flags(gfp_flags));

    // Read node (NUMA node)
    arg_list[5]->name = "node";
    read_trace_field(addr, arg_list[5]);
}

/**
 * @brief Handle mm_page_free event
 * Format: "page=%p pfn=0x%lx order=%d"
 * Note: page address calculation is complex, we just read pfn directly
 */
void mm_page_free_event::handle(ulong addr){
    // Read pfn (page frame number) - used to calculate page address
    arg_list[1]->name = "pfn";
    read_trace_field(addr, arg_list[1]);

    // For page address, we store pfn value (the complex calculation is done in kernel)
    // arg_list[0] will contain the same pfn value for display purposes
    unsigned long pfn = *reinterpret_cast<unsigned long*>(arg_list[1]->data);
    *reinterpret_cast<unsigned long*>(arg_list[0]->data) = pfn;

    // Read order
    arg_list[2]->name = "order";
    read_trace_field(addr, arg_list[2]);
}

/**
 * @brief Handle mm_page_free_batched event
 * Format: "page=%p pfn=0x%lx order=0"
 */
void mm_page_free_batched_event::handle(ulong addr){
    // Read pfn (page frame number)
    arg_list[1]->name = "pfn";
    read_trace_field(addr, arg_list[1]);

    // For page address, we store pfn value
    unsigned long pfn = *reinterpret_cast<unsigned long*>(arg_list[1]->data);
    *reinterpret_cast<unsigned long*>(arg_list[0]->data) = pfn;
}

/**
 * @brief Handle mm_page_alloc event
 * Format: "page=%p pfn=0x%lx order=%d migratetype=%d gfp_flags=%s"
 */
void mm_page_alloc_event::handle(ulong addr){
    // Read pfn (page frame number)
    arg_list[1]->name = "pfn";
    read_trace_field(addr, arg_list[1]);

    // For page address, we store pfn value
    unsigned long pfn = *reinterpret_cast<unsigned long*>(arg_list[1]->data);
    *reinterpret_cast<unsigned long*>(arg_list[0]->data) = pfn;

    // Read order
    arg_list[2]->name = "order";
    read_trace_field(addr, arg_list[2]);

    // Read migratetype
    arg_list[3]->name = "migratetype";
    read_trace_field(addr, arg_list[3]);

    // Decode gfp_flags
    std::shared_ptr<trace_field> field_ptr = field_maps["gfp_flags"];
    if (!field_ptr) return;
    unsigned long gfp_flags = plugin_ptr->read_ulong(addr + field_ptr->offset, "gfp_flags");
    copy_str(arg_list[4], decode_gfp_flags(gfp_flags));
}

/**
 * @brief Handle mm_page_alloc_zone_locked event
 * Format: "page=%p pfn=0x%lx order=%u migratetype=%d percpu_refill=%d"
 */
void mm_page_alloc_zone_locked_event::handle(ulong addr){
    // Read pfn (page frame number)
    arg_list[1]->name = "pfn";
    read_trace_field(addr, arg_list[1]);

    // For page address, we store pfn value
    unsigned long pfn = *reinterpret_cast<unsigned long*>(arg_list[1]->data);
    *reinterpret_cast<unsigned long*>(arg_list[0]->data) = pfn;

    // Read order
    arg_list[2]->name = "order";
    read_trace_field(addr, arg_list[2]);

    // Read migratetype
    arg_list[3]->name = "migratetype";
    read_trace_field(addr, arg_list[3]);

    // Calculate percpu_refill (order == 0)
    unsigned int order = *reinterpret_cast<unsigned int*>(arg_list[2]->data);
    *reinterpret_cast<int*>(arg_list[4]->data) = (order == 0) ? 1 : 0;
}

/**
 * @brief Handle mm_page_pcpu_drain event
 * Format: "page=%p pfn=0x%lx order=%d migratetype=%d"
 */
void mm_page_pcpu_drain_event::handle(ulong addr){
    // Read pfn (page frame number)
    arg_list[1]->name = "pfn";
    read_trace_field(addr, arg_list[1]);

    // For page address, we store pfn value
    unsigned long pfn = *reinterpret_cast<unsigned long*>(arg_list[1]->data);
    *reinterpret_cast<unsigned long*>(arg_list[0]->data) = pfn;

    // Read order
    arg_list[2]->name = "order";
    read_trace_field(addr, arg_list[2]);

    // Read migratetype
    arg_list[3]->name = "migratetype";
    read_trace_field(addr, arg_list[3]);
}

/**
 * @brief Handle mm_page_alloc_extfrag event
 * Format: "page=%p pfn=0x%lx alloc_order=%d fallback_order=%d pageblock_order=%d alloc_migratetype=%d fallback_migratetype=%d fragmenting=%d change_ownership=%d"
 */
void mm_page_alloc_extfrag_event::handle(ulong addr){
    // Read pfn (page frame number)
    arg_list[1]->name = "pfn";
    read_trace_field(addr, arg_list[1]);

    // For page address, we store pfn value
    unsigned long pfn = *reinterpret_cast<unsigned long*>(arg_list[1]->data);
    *reinterpret_cast<unsigned long*>(arg_list[0]->data) = pfn;

    // Read alloc_order
    arg_list[2]->name = "alloc_order";
    read_trace_field(addr, arg_list[2]);

    // Read fallback_order
    arg_list[3]->name = "fallback_order";
    read_trace_field(addr, arg_list[3]);

    // Set pageblock_order constant (11-1 = 10)
    *reinterpret_cast<int*>(arg_list[4]->data) = 10;

    // Read alloc_migratetype
    arg_list[5]->name = "alloc_migratetype";
    read_trace_field(addr, arg_list[5]);

    // Read fallback_migratetype
    arg_list[6]->name = "fallback_migratetype";
    read_trace_field(addr, arg_list[6]);

    // Calculate fragmenting (fallback_order < pageblock_order)
    int fallback_order = *reinterpret_cast<int*>(arg_list[3]->data);
    *reinterpret_cast<int*>(arg_list[7]->data) = (fallback_order < 10) ? 1 : 0;

    // Read change_ownership
    arg_list[8]->name = "change_ownership";
    read_trace_field(addr, arg_list[8]);
}

/**
 * @brief Handle rss_stat event
 * Format: "mm_id=%u curr=%d type=%s size=%ldB"
 */
void rss_stat_event::handle(ulong addr){
    // Read mm_id
    arg_list[0]->name = "mm_id";
    read_trace_field(addr, arg_list[0]);

    // Read curr
    arg_list[1]->name = "curr";
    read_trace_field(addr, arg_list[1]);

    // Decode member (RSS stat type)
    std::shared_ptr<trace_field> field_ptr = field_maps["member"];
    if (!field_ptr) return;
    int member = plugin_ptr->read_int(addr + field_ptr->offset, "member");
    copy_str(arg_list[2], decode_rss_stat_member(member));

    // Read size
    arg_list[3]->name = "size";
    read_trace_field(addr, arg_list[3]);
}

/**
 * @brief Handle writeback_mark_inode_dirty event
 * Format: "bdi %s: ino=%lu state=%s flags=%s"
 */
void writeback_mark_inode_dirty_event::handle(ulong addr){
    // Read bdi name
    arg_list[0]->name = "name";
    read_trace_field(addr, arg_list[0]);

    // Read ino
    arg_list[1]->name = "ino";
    read_trace_field(addr, arg_list[1]);

    // Decode state
    std::shared_ptr<trace_field> field_ptr = field_maps["state"];
    if (!field_ptr) return;
    unsigned long state = plugin_ptr->read_ulong(addr + field_ptr->offset, "state");
    copy_str(arg_list[2], decode_inode_state(state));

    // Decode flags
    field_ptr = field_maps["flags"];
    if (!field_ptr) return;
    unsigned long flags = plugin_ptr->read_ulong(addr + field_ptr->offset, "flags");
    copy_str(arg_list[3], decode_inode_state(flags));
}

/**
 * @brief Handle writeback_dirty_inode_start event
 * Format: "bdi %s: ino=%lu state=%s flags=%s"
 */
void writeback_dirty_inode_start_event::handle(ulong addr){
    // Read bdi name
    arg_list[0]->name = "name";
    read_trace_field(addr, arg_list[0]);

    // Read ino
    arg_list[1]->name = "ino";
    read_trace_field(addr, arg_list[1]);

    // Decode state
    std::shared_ptr<trace_field> field_ptr = field_maps["state"];
    if (!field_ptr) return;
    unsigned long state = plugin_ptr->read_ulong(addr + field_ptr->offset, "state");
    copy_str(arg_list[2], decode_inode_state(state));

    // Decode flags
    field_ptr = field_maps["flags"];
    if (!field_ptr) return;
    unsigned long flags = plugin_ptr->read_ulong(addr + field_ptr->offset, "flags");
    copy_str(arg_list[3], decode_inode_state(flags));
}

/**
 * @brief Handle writeback_dirty_inode event
 * Format: "bdi %s: ino=%lu state=%s flags=%s"
 */
void writeback_dirty_inode_event::handle(ulong addr){
    // Read bdi name
    arg_list[0]->name = "name";
    read_trace_field(addr, arg_list[0]);

    // Read ino
    arg_list[1]->name = "ino";
    read_trace_field(addr, arg_list[1]);

    // Decode state
    std::shared_ptr<trace_field> field_ptr = field_maps["state"];
    if (!field_ptr) return;
    unsigned long state = plugin_ptr->read_ulong(addr + field_ptr->offset, "state");
    copy_str(arg_list[2], decode_inode_state(state));

    // Decode flags
    field_ptr = field_maps["flags"];
    if (!field_ptr) return;
    unsigned long flags = plugin_ptr->read_ulong(addr + field_ptr->offset, "flags");
    copy_str(arg_list[3], decode_inode_state(flags));
}

/**
 * @brief Handle writeback_queue event
 * Format: "bdi %s: sb_dev %d:%d nr_pages=%ld sync_mode=%d kupdate=%d range_cyclic=%d background=%d reason=%s cgroup_ino=%lu"
 */
void writeback_queue_event::handle(ulong addr){
    // Read bdi name
    arg_list[0]->name = "name";
    read_trace_field(addr, arg_list[0]);

    // Read sb_dev and extract major:minor
    std::shared_ptr<trace_field> field_ptr = field_maps["sb_dev"];
    if (!field_ptr) return;
    unsigned int sb_dev = plugin_ptr->read_uint(addr + field_ptr->offset, "sb_dev");
    *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (sb_dev >> 20);  // major
    *reinterpret_cast<unsigned int*>(arg_list[2]->data) = (sb_dev & ((1U << 20) - 1));  // minor

    // Read simple fields
    arg_list[3]->name = "nr_pages";
    arg_list[4]->name = "sync_mode";
    arg_list[5]->name = "for_kupdate";
    arg_list[6]->name = "range_cyclic";
    arg_list[7]->name = "for_background";

    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);
    read_trace_field(addr, arg_list[7]);

    // Decode reason
    field_ptr = field_maps["reason"];
    if (!field_ptr) return;
    int reason = plugin_ptr->read_int(addr + field_ptr->offset, "reason");
    copy_str(arg_list[8], decode_writeback_reason(reason));

    // Read cgroup_ino
    arg_list[9]->name = "cgroup_ino";
    read_trace_field(addr, arg_list[9]);
}

/**
 * @brief Handle writeback_exec event
 * Format: "bdi %s: sb_dev %d:%d nr_pages=%ld sync_mode=%d kupdate=%d range_cyclic=%d background=%d reason=%s cgroup_ino=%lu"
 */
void writeback_exec_event::handle(ulong addr){
    // Read bdi name
    arg_list[0]->name = "name";
    read_trace_field(addr, arg_list[0]);

    // Read sb_dev and extract major:minor
    std::shared_ptr<trace_field> field_ptr = field_maps["sb_dev"];
    if (!field_ptr) return;
    unsigned int sb_dev = plugin_ptr->read_uint(addr + field_ptr->offset, "sb_dev");
    *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (sb_dev >> 20);
    *reinterpret_cast<unsigned int*>(arg_list[2]->data) = (sb_dev & ((1U << 20) - 1));

    // Read simple fields
    arg_list[3]->name = "nr_pages";
    arg_list[4]->name = "sync_mode";
    arg_list[5]->name = "for_kupdate";
    arg_list[6]->name = "range_cyclic";
    arg_list[7]->name = "for_background";

    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);
    read_trace_field(addr, arg_list[7]);

    // Decode reason
    field_ptr = field_maps["reason"];
    if (!field_ptr) return;
    int reason = plugin_ptr->read_int(addr + field_ptr->offset, "reason");
    copy_str(arg_list[8], decode_writeback_reason(reason));

    // Read cgroup_ino
    arg_list[9]->name = "cgroup_ino";
    read_trace_field(addr, arg_list[9]);
}

/**
 * @brief Handle writeback_start event
 * Format: "bdi %s: sb_dev %d:%d nr_pages=%ld sync_mode=%d kupdate=%d range_cyclic=%d background=%d reason=%s cgroup_ino=%lu"
 */
void writeback_start_event::handle(ulong addr){
    // Read bdi name
    arg_list[0]->name = "name";
    read_trace_field(addr, arg_list[0]);

    // Read sb_dev and extract major:minor
    std::shared_ptr<trace_field> field_ptr = field_maps["sb_dev"];
    if (!field_ptr) return;
    unsigned int sb_dev = plugin_ptr->read_uint(addr + field_ptr->offset, "sb_dev");
    *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (sb_dev >> 20);
    *reinterpret_cast<unsigned int*>(arg_list[2]->data) = (sb_dev & ((1U << 20) - 1));

    // Read simple fields
    arg_list[3]->name = "nr_pages";
    arg_list[4]->name = "sync_mode";
    arg_list[5]->name = "for_kupdate";
    arg_list[6]->name = "range_cyclic";
    arg_list[7]->name = "for_background";

    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);
    read_trace_field(addr, arg_list[7]);

    // Decode reason
    field_ptr = field_maps["reason"];
    if (!field_ptr) return;
    int reason = plugin_ptr->read_int(addr + field_ptr->offset, "reason");
    copy_str(arg_list[8], decode_writeback_reason(reason));

    // Read cgroup_ino
    arg_list[9]->name = "cgroup_ino";
    read_trace_field(addr, arg_list[9]);
}

/**
 * @brief Handle writeback_written event
 * Format: "bdi %s: sb_dev %d:%d nr_pages=%ld sync_mode=%d kupdate=%d range_cyclic=%d background=%d reason=%s cgroup_ino=%lu"
 */
void writeback_written_event::handle(ulong addr){
    // Read bdi name
    arg_list[0]->name = "name";
    read_trace_field(addr, arg_list[0]);

    // Read sb_dev and extract major:minor
    std::shared_ptr<trace_field> field_ptr = field_maps["sb_dev"];
    if (!field_ptr) return;
    unsigned int sb_dev = plugin_ptr->read_uint(addr + field_ptr->offset, "sb_dev");
    *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (sb_dev >> 20);
    *reinterpret_cast<unsigned int*>(arg_list[2]->data) = (sb_dev & ((1U << 20) - 1));

    // Read simple fields
    arg_list[3]->name = "nr_pages";
    arg_list[4]->name = "sync_mode";
    arg_list[5]->name = "for_kupdate";
    arg_list[6]->name = "range_cyclic";
    arg_list[7]->name = "for_background";

    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);
    read_trace_field(addr, arg_list[7]);

    // Decode reason
    field_ptr = field_maps["reason"];
    if (!field_ptr) return;
    int reason = plugin_ptr->read_int(addr + field_ptr->offset, "reason");
    copy_str(arg_list[8], decode_writeback_reason(reason));

    // Read cgroup_ino
    arg_list[9]->name = "cgroup_ino";
    read_trace_field(addr, arg_list[9]);
}

/**
 * @brief Handle writeback_wait event
 * Format: "bdi %s: sb_dev %d:%d nr_pages=%ld sync_mode=%d kupdate=%d range_cyclic=%d background=%d reason=%s cgroup_ino=%lu"
 */
void writeback_wait_event::handle(ulong addr){
    // Read bdi name
    arg_list[0]->name = "name";
    read_trace_field(addr, arg_list[0]);

    // Read sb_dev and extract major:minor
    std::shared_ptr<trace_field> field_ptr = field_maps["sb_dev"];
    if (!field_ptr) return;
    unsigned int sb_dev = plugin_ptr->read_uint(addr + field_ptr->offset, "sb_dev");
    *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (sb_dev >> 20);
    *reinterpret_cast<unsigned int*>(arg_list[2]->data) = (sb_dev & ((1U << 20) - 1));

    // Read simple fields
    arg_list[3]->name = "nr_pages";
    arg_list[4]->name = "sync_mode";
    arg_list[5]->name = "for_kupdate";
    arg_list[6]->name = "range_cyclic";
    arg_list[7]->name = "for_background";

    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);
    read_trace_field(addr, arg_list[7]);

    // Decode reason
    field_ptr = field_maps["reason"];
    if (!field_ptr) return;
    int reason = plugin_ptr->read_int(addr + field_ptr->offset, "reason");
    copy_str(arg_list[8], decode_writeback_reason(reason));

    // Read cgroup_ino
    arg_list[9]->name = "cgroup_ino";
    read_trace_field(addr, arg_list[9]);
}

/**
 * @brief Handle writeback_queue_io event
 * Format: "bdi %s: older=%lu age=%ld enqueue=%d reason=%s cgroup_ino=%lu"
 */
void writeback_queue_io_event::handle(ulong addr){
    arg_list[0]->name = "name";
    arg_list[1]->name = "older";
    arg_list[2]->name = "age";
    arg_list[3]->name = "moved";

    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);

    // Decode reason
    std::shared_ptr<trace_field> field_ptr = field_maps["reason"];
    if (!field_ptr) return;
    int reason = plugin_ptr->read_int(addr + field_ptr->offset, "reason");
    copy_str(arg_list[4], decode_writeback_reason(reason));

    arg_list[5]->name = "cgroup_ino";
    read_trace_field(addr, arg_list[5]);
}

/**
 * @brief Handle writeback_sb_inodes_requeue event
 * Format: "bdi %s: ino=%lu state=%s dirtied_when=%lu age=%lu cgroup_ino=%lu"
 */
void writeback_sb_inodes_requeue_event::handle(ulong addr){
    arg_list[0]->name = "name";
    arg_list[1]->name = "ino";
    arg_list[3]->name = "dirtied_when";
    arg_list[5]->name = "cgroup_ino";

    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[5]);

    // Decode state
    std::shared_ptr<trace_field> field_ptr = field_maps["state"];
    if (!field_ptr) return;
    unsigned long state = plugin_ptr->read_ulong(addr + field_ptr->offset, "state");
    copy_str(arg_list[2], decode_inode_state(state));

    // Calculate age (jiffies - dirtied_when) / HZ
    // Note: This is a placeholder as jiffies is not easily accessible
    *reinterpret_cast<unsigned long*>(arg_list[4]->data) = 0;
}

/**
 * @brief Handle writeback_single_inode_start event
 * Format: "bdi %s: ino=%lu state=%s dirtied_when=%lu age=%lu index=%lu to_write=%ld wrote=%lu cgroup_ino=%lu"
 */
void writeback_single_inode_start_event::handle(ulong addr){
    arg_list[0]->name = "name";
    arg_list[1]->name = "ino";
    arg_list[3]->name = "dirtied_when";
    arg_list[5]->name = "writeback_index";
    arg_list[6]->name = "nr_to_write";
    arg_list[7]->name = "wrote";
    arg_list[8]->name = "cgroup_ino";

    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);
    read_trace_field(addr, arg_list[7]);
    read_trace_field(addr, arg_list[8]);

    // Decode state
    std::shared_ptr<trace_field> field_ptr = field_maps["state"];
    if (!field_ptr) return;
    unsigned long state = plugin_ptr->read_ulong(addr + field_ptr->offset, "state");
    copy_str(arg_list[2], decode_inode_state(state));

    // Calculate age (placeholder)
    *reinterpret_cast<unsigned long*>(arg_list[4]->data) = 0;
}

/**
 * @brief Handle writeback_single_inode event
 * Format: "bdi %s: ino=%lu state=%s dirtied_when=%lu age=%lu index=%lu to_write=%ld wrote=%lu cgroup_ino=%lu"
 */
void writeback_single_inode_event::handle(ulong addr){
    arg_list[0]->name = "name";
    arg_list[1]->name = "ino";
    arg_list[3]->name = "dirtied_when";
    arg_list[5]->name = "writeback_index";
    arg_list[6]->name = "nr_to_write";
    arg_list[7]->name = "wrote";
    arg_list[8]->name = "cgroup_ino";

    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);
    read_trace_field(addr, arg_list[7]);
    read_trace_field(addr, arg_list[8]);

    // Decode state
    std::shared_ptr<trace_field> field_ptr = field_maps["state"];
    if (!field_ptr) return;
    unsigned long state = plugin_ptr->read_ulong(addr + field_ptr->offset, "state");
    copy_str(arg_list[2], decode_inode_state(state));

    // Calculate age (placeholder)
    *reinterpret_cast<unsigned long*>(arg_list[4]->data) = 0;
}

/**
 * @brief Handle writeback_lazytime event
 * Format: "dev %d,%d ino %lu dirtied %lu state %s mode 0%o"
 */
void writeback_lazytime_event::handle(ulong addr){
    // Read dev and extract major:minor
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (!field_ptr) return;
    unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
    *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);  // major
    *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));  // minor

    // Read simple fields
    arg_list[2]->name = "ino";
    arg_list[3]->name = "dirtied_when";
    arg_list[5]->name = "mode";

    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[5]);

    // Decode state
    field_ptr = field_maps["state"];
    if (!field_ptr) return;
    unsigned long state = plugin_ptr->read_ulong(addr + field_ptr->offset, "state");
    copy_str(arg_list[4], decode_inode_state(state));
}

/**
 * @brief Handle writeback_lazytime_iput event
 * Format: "dev %d,%d ino %lu dirtied %lu state %s mode 0%o"
 */
void writeback_lazytime_iput_event::handle(ulong addr){
    // Read dev and extract major:minor
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (!field_ptr) return;
    unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
    *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
    *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));

    // Read simple fields
    arg_list[2]->name = "ino";
    arg_list[3]->name = "dirtied_when";
    arg_list[5]->name = "mode";

    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[5]);

    // Decode state
    field_ptr = field_maps["state"];
    if (!field_ptr) return;
    unsigned long state = plugin_ptr->read_ulong(addr + field_ptr->offset, "state");
    copy_str(arg_list[4], decode_inode_state(state));
}

/**
 * @brief Handle writeback_dirty_inode_enqueue event
 * Format: "dev %d,%d ino %lu dirtied %lu state %s mode 0%o"
 */
void writeback_dirty_inode_enqueue_event::handle(ulong addr){
    // Read dev and extract major:minor
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (!field_ptr) return;
    unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
    *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
    *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));

    // Read simple fields
    arg_list[2]->name = "ino";
    arg_list[3]->name = "dirtied_when";
    arg_list[5]->name = "mode";

    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[5]);

    // Decode state
    field_ptr = field_maps["state"];
    if (!field_ptr) return;
    unsigned long state = plugin_ptr->read_ulong(addr + field_ptr->offset, "state");
    copy_str(arg_list[4], decode_inode_state(state));
}

/**
 * @brief Handle sb_mark_inode_writeback event
 * Format: "dev %d,%d ino %lu dirtied %lu state %s mode 0%o"
 */
void sb_mark_inode_writeback_event::handle(ulong addr){
    // Read dev and extract major:minor
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (!field_ptr) return;
    unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
    *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
    *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));

    // Read simple fields
    arg_list[2]->name = "ino";
    arg_list[3]->name = "dirtied_when";
    arg_list[5]->name = "mode";

    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[5]);

    // Decode state
    field_ptr = field_maps["state"];
    if (!field_ptr) return;
    unsigned long state = plugin_ptr->read_ulong(addr + field_ptr->offset, "state");
    copy_str(arg_list[4], decode_inode_state(state));
}

/**
 * @brief Handle sb_clear_inode_writeback event
 * Format: "dev %d,%d ino %lu dirtied %lu state %s mode 0%o"
 */
void sb_clear_inode_writeback_event::handle(ulong addr){
    // Read dev and extract major:minor
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (!field_ptr) return;
    unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
    *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
    *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));

    // Read simple fields
    arg_list[2]->name = "ino";
    arg_list[3]->name = "dirtied_when";
    arg_list[5]->name = "mode";

    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[5]);

    // Decode state
    field_ptr = field_maps["state"];
    if (!field_ptr) return;
    unsigned long state = plugin_ptr->read_ulong(addr + field_ptr->offset, "state");
    copy_str(arg_list[4], decode_inode_state(state));
}

/**
 * @brief Handle scsi_dispatch_cmd_start event
 * Format: "host_no=%u channel=%u id=%u lun=%u data_sgl=%u prot_sgl=%u prot_op=%s cmnd=(%s %s raw=%s)"
 */
void scsi_dispatch_cmd_start_event::handle(ulong addr){
    // Read basic SCSI device info
    arg_list[0]->name = "host_no";
    arg_list[1]->name = "channel";
    arg_list[2]->name = "id";
    arg_list[3]->name = "lun";
    arg_list[4]->name = "data_sglen";
    arg_list[5]->name = "prot_sglen";

    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);

    // Decode prot_op
    std::shared_ptr<trace_field> field_ptr = field_maps["prot_op"];
    if (!field_ptr) return;
    int prot_op = plugin_ptr->read_int(addr + field_ptr->offset, "prot_op");
    copy_str(arg_list[6], decode_scsi_prot_op(prot_op));

    // Decode opcode
    field_ptr = field_maps["opcode"];
    if (!field_ptr) return;
    unsigned char opcode = plugin_ptr->read_byte(addr + field_ptr->offset, "opcode");
    copy_str(arg_list[7], decode_scsi_opcode(opcode));

    // Read CDB (Command Descriptor Block) from dynamic array
    field_ptr = field_maps["cmnd"];
    if (!field_ptr) return;

    // Read cmd_len
    std::shared_ptr<trace_field> len_field = field_maps["cmd_len"];
    if (!len_field) return;
    int cmd_len = plugin_ptr->read_int(addr + len_field->offset, "cmd_len");

    // Read CDB bytes
    unsigned char cdb[16] = {0};
    int read_len = (cmd_len > 16) ? 16 : cmd_len;
    for (int i = 0; i < read_len; i++) {
        cdb[i] = plugin_ptr->read_byte(addr + field_ptr->offset + i, "cmnd");
    }

    // Parse CDB
    copy_str(arg_list[8], scsi_trace_parse_cdb(cdb, read_len));

    // Format as hex string
    copy_str(arg_list[9], format_hex_string(cdb, read_len));
}

/**
 * @brief Handle scsi_dispatch_cmd_done event
 * Format: "host_no=%u channel=%u id=%u lun=%u data_sgl=%u prot_sgl=%u prot_op=%s cmnd=(%s %s raw=%s) result=(driver=%s host=%s message=%s status=%s) sense=(key=%u asc=%#x ascq=%#x)"
 */
void scsi_dispatch_cmd_done_event::handle(ulong addr){
    // Read basic SCSI device info
    arg_list[0]->name = "host_no";
    arg_list[1]->name = "channel";
    arg_list[2]->name = "id";
    arg_list[3]->name = "lun";
    arg_list[4]->name = "data_sglen";
    arg_list[5]->name = "prot_sglen";

    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);

    // Decode prot_op
    std::shared_ptr<trace_field> field_ptr = field_maps["prot_op"];
    if (!field_ptr) return;
    int prot_op = plugin_ptr->read_int(addr + field_ptr->offset, "prot_op");
    copy_str(arg_list[6], decode_scsi_prot_op(prot_op));

    // Decode opcode
    field_ptr = field_maps["opcode"];
    if (!field_ptr) return;
    unsigned char opcode = plugin_ptr->read_byte(addr + field_ptr->offset, "opcode");
    copy_str(arg_list[7], decode_scsi_opcode(opcode));

    // Read CDB
    field_ptr = field_maps["cmnd"];
    if (!field_ptr) return;
    std::shared_ptr<trace_field> len_field = field_maps["cmd_len"];
    if (!len_field) return;
    int cmd_len = plugin_ptr->read_int(addr + len_field->offset, "cmd_len");

    unsigned char cdb[16] = {0};
    int read_len = (cmd_len > 16) ? 16 : cmd_len;
    for (int i = 0; i < read_len; i++) {
        cdb[i] = plugin_ptr->read_byte(addr + field_ptr->offset + i, "cmnd");
    }

    copy_str(arg_list[8], scsi_trace_parse_cdb(cdb, read_len));
    copy_str(arg_list[9], format_hex_string(cdb, read_len));

    // Driver status (always DRIVER_OK in modern kernels)
    copy_str(arg_list[10], "DRIVER_OK");

    // Decode result field
    field_ptr = field_maps["result"];
    if (!field_ptr) return;
    unsigned int result = plugin_ptr->read_uint(addr + field_ptr->offset, "result");

    // Host byte (bits 16-23)
    unsigned char host_byte = (result >> 16) & 0xff;
    copy_str(arg_list[11], decode_scsi_host_status(host_byte));

    // Message byte (always COMMAND_COMPLETE)
    copy_str(arg_list[12], "COMMAND_COMPLETE");

    // Status byte (bits 0-7)
    unsigned char status_byte = result & 0xff;
    copy_str(arg_list[13], decode_scsi_sam_status(status_byte));

    // Read sense data
    arg_list[14]->name = "sense_key";
    arg_list[15]->name = "asc";
    arg_list[16]->name = "ascq";

    read_trace_field(addr, arg_list[14]);
    read_trace_field(addr, arg_list[15]);
    read_trace_field(addr, arg_list[16]);
}

/**
 * @brief Handle scsi_dispatch_cmd_error event
 * Format: "host_no=%u channel=%u id=%u lun=%u data_sgl=%u prot_sgl=%u prot_op=%s cmnd=(%s %s raw=%s) rtn=%d"
 */
void scsi_dispatch_cmd_error_event::handle(ulong addr){
    // Read basic SCSI device info
    arg_list[0]->name = "host_no";
    arg_list[1]->name = "channel";
    arg_list[2]->name = "id";
    arg_list[3]->name = "lun";
    arg_list[4]->name = "data_sglen";
    arg_list[5]->name = "prot_sglen";

    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);

    // Decode prot_op
    std::shared_ptr<trace_field> field_ptr = field_maps["prot_op"];
    if (!field_ptr) return;
    int prot_op = plugin_ptr->read_int(addr + field_ptr->offset, "prot_op");
    copy_str(arg_list[6], decode_scsi_prot_op(prot_op));

    // Decode opcode
    field_ptr = field_maps["opcode"];
    if (!field_ptr) return;
    unsigned char opcode = plugin_ptr->read_byte(addr + field_ptr->offset, "opcode");
    copy_str(arg_list[7], decode_scsi_opcode(opcode));

    // Read CDB
    field_ptr = field_maps["cmnd"];
    if (!field_ptr) return;
    std::shared_ptr<trace_field> len_field = field_maps["cmd_len"];
    if (!len_field) return;
    int cmd_len = plugin_ptr->read_int(addr + len_field->offset, "cmd_len");

    unsigned char cdb[16] = {0};
    int read_len = (cmd_len > 16) ? 16 : cmd_len;
    for (int i = 0; i < read_len; i++) {
        cdb[i] = plugin_ptr->read_byte(addr + field_ptr->offset + i, "cmnd");
    }

    copy_str(arg_list[8], scsi_trace_parse_cdb(cdb, read_len));
    copy_str(arg_list[9], format_hex_string(cdb, read_len));

    // Read return value
    arg_list[10]->name = "rtn";
    read_trace_field(addr, arg_list[10]);
}

/**
 * @brief Handle scsi_dispatch_cmd_timeout event
 * Format: "host_no=%u channel=%u id=%u lun=%u data_sgl=%u prot_sgl=%u prot_op=%s cmnd=(%s %s raw=%s) result=(driver=%s host=%s message=%s status=%s) sense=(key=%u asc=%#x ascq=%#x)"
 */
void scsi_dispatch_cmd_timeout_event::handle(ulong addr){
    // Same as scsi_dispatch_cmd_done
    // Read basic SCSI device info
    arg_list[0]->name = "host_no";
    arg_list[1]->name = "channel";
    arg_list[2]->name = "id";
    arg_list[3]->name = "lun";
    arg_list[4]->name = "data_sglen";
    arg_list[5]->name = "prot_sglen";

    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);

    // Decode prot_op
    std::shared_ptr<trace_field> field_ptr = field_maps["prot_op"];
    if (!field_ptr) return;
    int prot_op = plugin_ptr->read_int(addr + field_ptr->offset, "prot_op");
    copy_str(arg_list[6], decode_scsi_prot_op(prot_op));

    // Decode opcode
    field_ptr = field_maps["opcode"];
    if (!field_ptr) return;
    unsigned char opcode = plugin_ptr->read_byte(addr + field_ptr->offset, "opcode");
    copy_str(arg_list[7], decode_scsi_opcode(opcode));

    // Read CDB
    field_ptr = field_maps["cmnd"];
    if (!field_ptr) return;
    std::shared_ptr<trace_field> len_field = field_maps["cmd_len"];
    if (!len_field) return;
    int cmd_len = plugin_ptr->read_int(addr + len_field->offset, "cmd_len");

    unsigned char cdb[16] = {0};
    int read_len = (cmd_len > 16) ? 16 : cmd_len;
    for (int i = 0; i < read_len; i++) {
        cdb[i] = plugin_ptr->read_byte(addr + field_ptr->offset + i, "cmnd");
    }

    copy_str(arg_list[8], scsi_trace_parse_cdb(cdb, read_len));
    copy_str(arg_list[9], format_hex_string(cdb, read_len));

    // Driver status
    copy_str(arg_list[10], "DRIVER_OK");

    // Decode result field
    field_ptr = field_maps["result"];
    if (!field_ptr) return;
    unsigned int result = plugin_ptr->read_uint(addr + field_ptr->offset, "result");

    // Host byte
    unsigned char host_byte = (result >> 16) & 0xff;
    copy_str(arg_list[11], decode_scsi_host_status(host_byte));

    // Message byte
    copy_str(arg_list[12], "COMMAND_COMPLETE");

    // Status byte
    unsigned char status_byte = result & 0xff;
    copy_str(arg_list[13], decode_scsi_sam_status(status_byte));

    // Read sense data
    arg_list[14]->name = "sense_key";
    arg_list[15]->name = "asc";
    arg_list[16]->name = "ascq";

    read_trace_field(addr, arg_list[14]);
    read_trace_field(addr, arg_list[15]);
    read_trace_field(addr, arg_list[16]);
}

/**
 * @brief Handle mem_connect event
 * Format: "mem_id=%d mem_type=%s allocator=%p ifindex=%d"
 */
void mem_connect_event::handle(ulong addr){
    // Read mem_id
    arg_list[0]->name = "mem_id";
    read_trace_field(addr, arg_list[0]);

    // Decode mem_type
    std::shared_ptr<trace_field> field_ptr = field_maps["mem_type"];
    if (!field_ptr) return;
    int mem_type = plugin_ptr->read_int(addr + field_ptr->offset, "mem_type");
    copy_str(arg_list[1], decode_mem_type(mem_type));

    // Read allocator
    arg_list[2]->name = "allocator";
    read_trace_field(addr, arg_list[2]);

    // Read ifindex
    arg_list[3]->name = "ifindex";
    read_trace_field(addr, arg_list[3]);
}

/**
 * @brief Handle mem_disconnect event
 * Format: "mem_id=%d mem_type=%s allocator=%p"
 */
void mem_disconnect_event::handle(ulong addr){
    // Read mem_id
    arg_list[0]->name = "mem_id";
    read_trace_field(addr, arg_list[0]);

    // Decode mem_type
    std::shared_ptr<trace_field> field_ptr = field_maps["mem_type"];
    if (!field_ptr) return;
    int mem_type = plugin_ptr->read_int(addr + field_ptr->offset, "mem_type");
    copy_str(arg_list[1], decode_mem_type(mem_type));

    // Read allocator
    arg_list[2]->name = "allocator";
    read_trace_field(addr, arg_list[2]);
}

/**
 * @brief Handle mem_return_failed event
 * Format: "mem_id=%d mem_type=%s page=%p"
 */
void mem_return_failed_event::handle(ulong addr){
    // Read mem_id
    arg_list[0]->name = "mem_id";
    read_trace_field(addr, arg_list[0]);

    // Decode mem_type
    std::shared_ptr<trace_field> field_ptr = field_maps["mem_type"];
    if (!field_ptr) return;
    int mem_type = plugin_ptr->read_int(addr + field_ptr->offset, "mem_type");
    copy_str(arg_list[1], decode_mem_type(mem_type));

    // Read page
    arg_list[2]->name = "page";
    read_trace_field(addr, arg_list[2]);
}

/**
 * @brief Handle mm_filemap_add_to_page_cache event
 * Format: "dev %d:%d ino %lx page=%p pfn=0x%lx ofs=%lu"
 */
void mm_filemap_add_to_page_cache_event::handle(ulong addr){
    // Read s_dev and extract major:minor
    std::shared_ptr<trace_field> field_ptr = field_maps["s_dev"];
    if (!field_ptr) return;
    unsigned int s_dev = plugin_ptr->read_uint(addr + field_ptr->offset, "s_dev");
    *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (s_dev >> 20);  // major
    *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (s_dev & ((1U << 20) - 1));  // minor

    // Read i_ino
    arg_list[2]->name = "i_ino";
    read_trace_field(addr, arg_list[2]);

    // Read pfn
    arg_list[4]->name = "pfn";
    read_trace_field(addr, arg_list[4]);

    // Use pfn as page address
    unsigned long pfn = *reinterpret_cast<unsigned long*>(arg_list[4]->data);
    *reinterpret_cast<unsigned long*>(arg_list[3]->data) = pfn;

    // Calculate offset (index << 12)
    field_ptr = field_maps["index"];
    if (!field_ptr) return;
    unsigned long index = plugin_ptr->read_ulong(addr + field_ptr->offset, "index");
    *reinterpret_cast<unsigned long*>(arg_list[5]->data) = index << 12;
}

/**
 * @brief Handle mm_filemap_delete_from_page_cache event
 * Format: "dev %d:%d ino %lx page=%p pfn=0x%lx ofs=%lu"
 */
void mm_filemap_delete_from_page_cache_event::handle(ulong addr){
    // Read s_dev and extract major:minor
    std::shared_ptr<trace_field> field_ptr = field_maps["s_dev"];
    if (!field_ptr) return;
    unsigned int s_dev = plugin_ptr->read_uint(addr + field_ptr->offset, "s_dev");
    *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (s_dev >> 20);
    *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (s_dev & ((1U << 20) - 1));

    // Read i_ino
    arg_list[2]->name = "i_ino";
    read_trace_field(addr, arg_list[2]);

    // Read pfn
    arg_list[4]->name = "pfn";
    read_trace_field(addr, arg_list[4]);

    // Use pfn as page address
    unsigned long pfn = *reinterpret_cast<unsigned long*>(arg_list[4]->data);
    *reinterpret_cast<unsigned long*>(arg_list[3]->data) = pfn;

    // Calculate offset (index << 12)
    field_ptr = field_maps["index"];
    if (!field_ptr) return;
    unsigned long index = plugin_ptr->read_ulong(addr + field_ptr->offset, "index");
    *reinterpret_cast<unsigned long*>(arg_list[5]->data) = index << 12;
}

/**
 * @brief Handle reclaim_retry_zone event
 * Format: "node=%d zone=%-8s order=%d reclaimable=%lu available=%lu min_wmark=%lu no_progress_loops=%d wmark_check=%d"
 */
void reclaim_retry_zone_event::handle(ulong addr){
    // Read node
    arg_list[0]->name = "node";
    read_trace_field(addr, arg_list[0]);

    // Decode zone_idx
    std::shared_ptr<trace_field> field_ptr = field_maps["zone_idx"];
    if (!field_ptr) return;
    int zone_idx = plugin_ptr->read_int(addr + field_ptr->offset, "zone_idx");
    copy_str(arg_list[1], decode_zone_type(zone_idx));

    // Read remaining fields
    arg_list[2]->name = "order";
    arg_list[3]->name = "reclaimable";
    arg_list[4]->name = "available";
    arg_list[5]->name = "min_wmark";
    arg_list[6]->name = "no_progress_loops";
    arg_list[7]->name = "wmark_check";

    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);
    read_trace_field(addr, arg_list[7]);
}

/**
 * @brief Handle compact_retry event
 * Format: "order=%d priority=%s compaction_result=%s retries=%d max_retries=%d should_retry=%d"
 */
void compact_retry_event::handle(ulong addr){
    // Read order
    arg_list[0]->name = "order";
    read_trace_field(addr, arg_list[0]);

    // Decode priority
    std::shared_ptr<trace_field> field_ptr = field_maps["priority"];
    if (!field_ptr) return;
    int priority = plugin_ptr->read_int(addr + field_ptr->offset, "priority");
    copy_str(arg_list[1], decode_compact_priority(priority));

    // Decode result
    field_ptr = field_maps["result"];
    if (!field_ptr) return;
    int result = plugin_ptr->read_int(addr + field_ptr->offset, "result");
    copy_str(arg_list[2], decode_compact_result(result));

    // Read remaining fields
    arg_list[3]->name = "retries";
    arg_list[4]->name = "max_retries";
    arg_list[5]->name = "ret";

    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);
}

/**
 * @brief Handle posix_lock_inode event
 * Format: "fl=%p dev=0x%x:0x%x ino=0x%lx fl_blocker=%p fl_owner=%p fl_pid=%u fl_flags=%s fl_type=%s fl_start=%lld fl_end=%lld ret=%d"
 */
void posix_lock_inode_event::handle(ulong addr){
    // Read fl
    arg_list[0]->name = "fl";
    read_trace_field(addr, arg_list[0]);

    // Read s_dev and extract major:minor
    std::shared_ptr<trace_field> field_ptr = field_maps["s_dev"];
    if (!field_ptr) return;
    unsigned int s_dev = plugin_ptr->read_uint(addr + field_ptr->offset, "s_dev");
    *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (s_dev >> 20);
    *reinterpret_cast<unsigned int*>(arg_list[2]->data) = (s_dev & ((1U << 20) - 1));

    // Read remaining fields
    arg_list[3]->name = "i_ino";
    arg_list[4]->name = "fl_blocker";
    arg_list[5]->name = "fl_owner";
    arg_list[6]->name = "fl_pid";

    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);

    // Decode fl_flags
    field_ptr = field_maps["fl_flags"];
    if (!field_ptr) return;
    unsigned int fl_flags = plugin_ptr->read_uint(addr + field_ptr->offset, "fl_flags");
    copy_str(arg_list[7], decode_file_lock_flags(fl_flags));

    // Decode fl_type
    field_ptr = field_maps["fl_type"];
    if (!field_ptr) return;
    unsigned char fl_type = plugin_ptr->read_byte(addr + field_ptr->offset, "fl_type");
    copy_str(arg_list[8], decode_file_lock_type(fl_type));

    // Read fl_start, fl_end, ret
    arg_list[9]->name = "fl_start";
    arg_list[10]->name = "fl_end";
    arg_list[11]->name = "ret";

    read_trace_field(addr, arg_list[9]);
    read_trace_field(addr, arg_list[10]);
    read_trace_field(addr, arg_list[11]);
}

/**
 * @brief Handle fcntl_setlk event (same format as posix_lock_inode)
 */
void fcntl_setlk_event::handle(ulong addr){
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
    arg_list[6]->name = "fl_pid";

    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);

    field_ptr = field_maps["fl_flags"];
    if (!field_ptr) return;
    unsigned int fl_flags = plugin_ptr->read_uint(addr + field_ptr->offset, "fl_flags");
    copy_str(arg_list[7], decode_file_lock_flags(fl_flags));

    field_ptr = field_maps["fl_type"];
    if (!field_ptr) return;
    unsigned char fl_type = plugin_ptr->read_byte(addr + field_ptr->offset, "fl_type");
    copy_str(arg_list[8], decode_file_lock_type(fl_type));

    arg_list[9]->name = "fl_start";
    arg_list[10]->name = "fl_end";
    arg_list[11]->name = "ret";

    read_trace_field(addr, arg_list[9]);
    read_trace_field(addr, arg_list[10]);
    read_trace_field(addr, arg_list[11]);
}

/**
 * @brief Handle locks_remove_posix event (same format)
 */
void locks_remove_posix_event::handle(ulong addr){
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
    arg_list[6]->name = "fl_pid";

    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);

    field_ptr = field_maps["fl_flags"];
    if (!field_ptr) return;
    unsigned int fl_flags = plugin_ptr->read_uint(addr + field_ptr->offset, "fl_flags");
    copy_str(arg_list[7], decode_file_lock_flags(fl_flags));

    field_ptr = field_maps["fl_type"];
    if (!field_ptr) return;
    unsigned char fl_type = plugin_ptr->read_byte(addr + field_ptr->offset, "fl_type");
    copy_str(arg_list[8], decode_file_lock_type(fl_type));

    arg_list[9]->name = "fl_start";
    arg_list[10]->name = "fl_end";
    arg_list[11]->name = "ret";

    read_trace_field(addr, arg_list[9]);
    read_trace_field(addr, arg_list[10]);
    read_trace_field(addr, arg_list[11]);
}

/**
 * @brief Handle flock_lock_inode event (same format)
 */
void flock_lock_inode_event::handle(ulong addr){
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
    arg_list[6]->name = "fl_pid";

    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);

    field_ptr = field_maps["fl_flags"];
    if (!field_ptr) return;
    unsigned int fl_flags = plugin_ptr->read_uint(addr + field_ptr->offset, "fl_flags");
    copy_str(arg_list[7], decode_file_lock_flags(fl_flags));

    field_ptr = field_maps["fl_type"];
    if (!field_ptr) return;
    unsigned char fl_type = plugin_ptr->read_byte(addr + field_ptr->offset, "fl_type");
    copy_str(arg_list[8], decode_file_lock_type(fl_type));

    arg_list[9]->name = "fl_start";
    arg_list[10]->name = "fl_end";
    arg_list[11]->name = "ret";

    read_trace_field(addr, arg_list[9]);
    read_trace_field(addr, arg_list[10]);
    read_trace_field(addr, arg_list[11]);
}

/**
 * @brief Handle break_lease_noblock event
 * Format: "fl=%p dev=0x%x:0x%x ino=0x%lx fl_blocker=%p fl_owner=%p fl_flags=%s fl_type=%s fl_break_time=%lu fl_downgrade_time=%lu"
 */
void break_lease_noblock_event::handle(ulong addr){
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

/**
 * @brief Handle break_lease_block event (same format as break_lease_noblock)
 */
void break_lease_block_event::handle(ulong addr){
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

/**
 * @brief Handle break_lease_unblock, generic_delete_lease, time_out_leases events (same format)
 */
void break_lease_unblock_event::handle(ulong addr){
    lease_event_handle(addr);
}

void generic_delete_lease_event::handle(ulong addr){
    lease_event_handle(addr);
}

void time_out_leases_event::handle(ulong addr){
    lease_event_handle(addr);
}

/**
 * @brief Handle generic_add_lease event
 * Format: "dev=0x%x:0x%x ino=0x%lx wcount=%d rcount=%d icount=%d fl_owner=%p fl_flags=%s fl_type=%s"
 */
void generic_add_lease_event::handle(ulong addr){
    std::shared_ptr<trace_field> field_ptr = field_maps["s_dev"];
    if (!field_ptr) return;
    unsigned int s_dev = plugin_ptr->read_uint(addr + field_ptr->offset, "s_dev");
    *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (s_dev >> 20);
    *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (s_dev & ((1U << 20) - 1));

    arg_list[2]->name = "i_ino";
    arg_list[3]->name = "wcount";
    arg_list[4]->name = "rcount";
    arg_list[5]->name = "icount";
    arg_list[6]->name = "fl_owner";

    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);

    field_ptr = field_maps["fl_flags"];
    if (!field_ptr) return;
    unsigned int fl_flags = plugin_ptr->read_uint(addr + field_ptr->offset, "fl_flags");
    copy_str(arg_list[7], decode_file_lock_flags(fl_flags));

    field_ptr = field_maps["fl_type"];
    if (!field_ptr) return;
    unsigned char fl_type = plugin_ptr->read_byte(addr + field_ptr->offset, "fl_type");
    copy_str(arg_list[8], decode_file_lock_type(fl_type));
}

/**
 * @brief Handle leases_conflict event
 * Format: "conflict %d: lease=%p fl_flags=%s fl_type=%s; breaker=%p fl_flags=%s fl_type=%s"
 */
void leases_conflict_event::handle(ulong addr){
    arg_list[0]->name = "conflict";
    arg_list[1]->name = "lease";

    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);

    // Decode lease fl_flags
    std::shared_ptr<trace_field> field_ptr = field_maps["l_fl_flags"];
    if (!field_ptr) return;
    unsigned int l_fl_flags = plugin_ptr->read_uint(addr + field_ptr->offset, "l_fl_flags");
    copy_str(arg_list[2], decode_file_lock_flags(l_fl_flags));

    // Decode lease fl_type
    field_ptr = field_maps["l_fl_type"];
    if (!field_ptr) return;
    unsigned char l_fl_type = plugin_ptr->read_byte(addr + field_ptr->offset, "l_fl_type");
    copy_str(arg_list[3], decode_file_lock_type(l_fl_type));

    // Read breaker
    arg_list[4]->name = "breaker";
    read_trace_field(addr, arg_list[4]);

    // Decode breaker fl_flags
    field_ptr = field_maps["b_fl_flags"];
    if (!field_ptr) return;
    unsigned int b_fl_flags = plugin_ptr->read_uint(addr + field_ptr->offset, "b_fl_flags");
    copy_str(arg_list[5], decode_file_lock_flags(b_fl_flags));

    // Decode breaker fl_type
    field_ptr = field_maps["b_fl_type"];
    if (!field_ptr) return;
    unsigned char b_fl_type = plugin_ptr->read_byte(addr + field_ptr->offset, "b_fl_type");
    copy_str(arg_list[6], decode_file_lock_type(b_fl_type));
}

/**
 * @brief Handle iomap_iter event
 * Format: "dev %d:%d ino 0x%llx pos 0x%llx length 0x%llx flags %s (0x%x) ops %ps caller %pS"
 */
void iomap_iter_event::handle(ulong addr){
    // Read dev and extract major:minor
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (!field_ptr) return;
    unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
    *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
    *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));

    // Read ino, pos, length
    arg_list[2]->name = "ino";
    arg_list[3]->name = "pos";
    arg_list[4]->name = "length";

    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);

    // Decode flags
    field_ptr = field_maps["flags"];
    if (!field_ptr) return;
    unsigned int flags = plugin_ptr->read_uint(addr + field_ptr->offset, "flags");
    copy_str(arg_list[5], decode_iomap_iter_flags(flags));

    // Store raw flags value
    *reinterpret_cast<unsigned int*>(arg_list[6]->data) = flags;

    // Read ops and caller
    arg_list[7]->name = "ops";
    arg_list[8]->name = "caller";

    read_trace_field(addr, arg_list[7]);
    read_trace_field(addr, arg_list[8]);
}

/**
 * @brief Handle ext4_allocate_blocks event
 * Format: "dev %d,%d ino %lu flags %s len %u block %llu lblk %u goal %llu lleft %u lright %u pleft %llu pright %llu"
 */
void ext4_allocate_blocks_event::handle(ulong addr){
    // Read dev and extract major:minor
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (!field_ptr) return;
    unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
    *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
    *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));

    // Read ino
    arg_list[2]->name = "ino";
    read_trace_field(addr, arg_list[2]);

    // Decode flags
    field_ptr = field_maps["flags"];
    if (!field_ptr) return;
    unsigned int flags = plugin_ptr->read_uint(addr + field_ptr->offset, "flags");
    copy_str(arg_list[3], decode_ext4_alloc_flags(flags));

    // Read remaining fields
    arg_list[4]->name = "len";
    arg_list[5]->name = "block";
    arg_list[6]->name = "logical";
    arg_list[7]->name = "goal";
    arg_list[8]->name = "lleft";
    arg_list[9]->name = "lright";
    arg_list[10]->name = "pleft";
    arg_list[11]->name = "pright";

    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);
    read_trace_field(addr, arg_list[7]);
    read_trace_field(addr, arg_list[8]);
    read_trace_field(addr, arg_list[9]);
    read_trace_field(addr, arg_list[10]);
    read_trace_field(addr, arg_list[11]);
}

/**
 * @brief Handle ext4_free_blocks event
 * Format: "dev %d,%d ino %lu mode 0%o block %llu count %lu flags %s"
 */
void ext4_free_blocks_event::handle(ulong addr){
    // Read dev and extract major:minor
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (!field_ptr) return;
    unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
    *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
    *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));

    // Read ino, mode, block, count
    arg_list[2]->name = "ino";
    arg_list[3]->name = "mode";
    arg_list[4]->name = "block";
    arg_list[5]->name = "count";

    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);

    // Decode flags
    field_ptr = field_maps["flags"];
    if (!field_ptr) return;
    unsigned int flags = plugin_ptr->read_uint(addr + field_ptr->offset, "flags");
    copy_str(arg_list[6], decode_ext4_free_flags(flags));
}

/**
 * @brief Handle ext4_ext_map_blocks_enter event
 * Format: "dev %d,%d ino %lu lblk %u len %u flags %s"
 */
void ext4_ext_map_blocks_enter_event::handle(ulong addr){
    // Read dev and extract major:minor
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (!field_ptr) return;
    unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
    *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
    *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));

    // Read ino, lblk, len
    arg_list[2]->name = "ino";
    arg_list[3]->name = "lblk";
    arg_list[4]->name = "len";

    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);

    // Decode flags
    field_ptr = field_maps["flags"];
    if (!field_ptr) return;
    unsigned int flags = plugin_ptr->read_uint(addr + field_ptr->offset, "flags");
    copy_str(arg_list[5], decode_ext4_map_flags(flags));
}

/**
 * @brief Handle ext4_ext_map_blocks_exit event
 * Format: "dev %d,%d ino %lu flags %s lblk %u pblk %llu len %u mflags %s ret %d"
 */
void ext4_ext_map_blocks_exit_event::handle(ulong addr){
    // Read dev and extract major:minor
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (!field_ptr) return;
    unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
    *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
    *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));

    // Read ino
    arg_list[2]->name = "ino";
    read_trace_field(addr, arg_list[2]);

    // Decode flags
    field_ptr = field_maps["flags"];
    if (!field_ptr) return;
    unsigned int flags = plugin_ptr->read_uint(addr + field_ptr->offset, "flags");
    copy_str(arg_list[3], decode_ext4_map_flags(flags));

    // Read lblk, pblk, len
    arg_list[4]->name = "lblk";
    arg_list[5]->name = "pblk";
    arg_list[6]->name = "len";

    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);

    // Decode mflags
    field_ptr = field_maps["mflags"];
    if (!field_ptr) return;
    unsigned int mflags = plugin_ptr->read_uint(addr + field_ptr->offset, "mflags");
    copy_str(arg_list[7], decode_ext4_mflags(mflags));

    // Read ret
    arg_list[8]->name = "ret";
    read_trace_field(addr, arg_list[8]);
}

/**
 * @brief Handle sched_switch_with_ctrs event
 * Format: "prev_comm=%s prev_pid=%d prev_state=%s%s ==> next_comm=%s next_pid=%d CCNTR=%u CTR0=%u CTR1=%u CTR2=%u CTR3=%u CTR4=%u CTR5=%u, CYC: %lu, INST: %lu"
 */
void sched_switch_with_ctrs_event::handle(ulong addr){
    // Read prev_comm
    arg_list[0]->name = "prev_comm";
    read_trace_field(addr, arg_list[0]);

    // Read prev_pid
    arg_list[1]->name = "prev_pid";
    read_trace_field(addr, arg_list[1]);

    // Decode prev_state
    std::shared_ptr<trace_field> field_ptr = field_maps["prev_state"];
    if (!field_ptr) return;
    long prev_state = plugin_ptr->read_long(addr + field_ptr->offset, "prev_state");
    copy_str(arg_list[2], decode_task_state(prev_state));

    // Check for TASK_NEW flag (+)
    const long TASK_STATE_MAX = ((0x0000 | 0x0001 | 0x0002 | 0x0004 | 0x0008 | 0x0010 | 0x0020 | 0x0040) + 1) << 1;
    if (prev_state & TASK_STATE_MAX) {
        copy_str(arg_list[3], "+");
    } else {
        copy_str(arg_list[3], "");
    }

    // Read next_comm and next_pid
    arg_list[4]->name = "next_comm";
    arg_list[5]->name = "next_pid";
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);

    // Read performance counters
    arg_list[6]->name = "cctr";
    arg_list[7]->name = "ctr0";
    arg_list[8]->name = "ctr1";
    arg_list[9]->name = "ctr2";
    arg_list[10]->name = "ctr3";
    arg_list[11]->name = "ctr4";
    arg_list[12]->name = "ctr5";
    arg_list[13]->name = "amu0";  // CYC
    arg_list[14]->name = "amu1";  // INST

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

/**
 * @brief Handle sched_enq_deq_task event
 * Format: "cpu=%d %s comm=%s pid=%d prio=%d nr_running=%u rt_nr_running=%u affine=%x demand=%u pred_demand_scaled=%u is_compat_t=%d mvp=%d"
 */
void sched_enq_deq_task_event::handle(ulong addr){
    // Read cpu
    arg_list[0]->name = "cpu";
    read_trace_field(addr, arg_list[0]);

    // Decode enqueue flag
    std::shared_ptr<trace_field> field_ptr = field_maps["enqueue"];
    if (!field_ptr) return;
    int enqueue = plugin_ptr->read_int(addr + field_ptr->offset, "enqueue");
    copy_str(arg_list[1], enqueue ? "enqueue" : "dequeue");

    // Read comm, pid, prio
    arg_list[2]->name = "comm";
    arg_list[3]->name = "pid";
    arg_list[4]->name = "prio";
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);

    // Read nr_running, rt_nr_running
    arg_list[5]->name = "nr_running";
    arg_list[6]->name = "rt_nr_running";
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);

    // Read cpus_allowed (affine)
    arg_list[7]->name = "cpus_allowed";
    read_trace_field(addr, arg_list[7]);

    // Read demand, pred_demand_scaled
    arg_list[8]->name = "demand";
    arg_list[9]->name = "pred_demand_scaled";
    read_trace_field(addr, arg_list[8]);
    read_trace_field(addr, arg_list[9]);

    // Read compat_thread, mvp
    arg_list[10]->name = "compat_thread";
    arg_list[11]->name = "mvp";
    read_trace_field(addr, arg_list[10]);
    read_trace_field(addr, arg_list[11]);
}

/**
 * @brief Handle devfreq_monitor event
 * Format: "dev_name=%-30s freq=%-12lu polling_ms=%-3u load=%-2lu"
 */
void devfreq_monitor_event::handle(ulong addr){
    // Read dev_name from dynamic string
    arg_list[0]->name = "dev_name";
    read_trace_field(addr, arg_list[0]);

    // Read freq, polling_ms
    arg_list[1]->name = "freq";
    arg_list[2]->name = "polling_ms";
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);

    // Calculate load: total_time == 0 ? 0 : (100 * busy_time) / total_time
    std::shared_ptr<trace_field> field_ptr = field_maps["total_time"];
    if (!field_ptr) return;
    unsigned long total_time = plugin_ptr->read_ulong(addr + field_ptr->offset, "total_time");

    field_ptr = field_maps["busy_time"];
    if (!field_ptr) return;
    unsigned long busy_time = plugin_ptr->read_ulong(addr + field_ptr->offset, "busy_time");

    unsigned long load = (total_time == 0) ? 0 : (100 * busy_time) / total_time;
    *reinterpret_cast<unsigned long*>(arg_list[3]->data) = load;
}

/**
 * @brief Handle devfreq_frequency event
 * Format: "dev_name=%-30s freq=%-12lu prev_freq=%-12lu load=%-2lu"
 */
void devfreq_frequency_event::handle(ulong addr){
    // Read dev_name from dynamic string
    arg_list[0]->name = "dev_name";
    read_trace_field(addr, arg_list[0]);

    // Read freq, prev_freq
    arg_list[1]->name = "freq";
    arg_list[2]->name = "prev_freq";
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);

    // Calculate load
    std::shared_ptr<trace_field> field_ptr = field_maps["total_time"];
    if (!field_ptr) return;
    unsigned long total_time = plugin_ptr->read_ulong(addr + field_ptr->offset, "total_time");

    field_ptr = field_maps["busy_time"];
    if (!field_ptr) return;
    unsigned long busy_time = plugin_ptr->read_ulong(addr + field_ptr->offset, "busy_time");

    unsigned long load = (total_time == 0) ? 0 : (100 * busy_time) / total_time;
    *reinterpret_cast<unsigned long*>(arg_list[3]->data) = load;
}

/**
 * @brief Handle ufshcd_command event
 * Format: "%s: %s: tag: %u, DB: 0x%x, size: %d, IS: %u, LBA: %llu, opcode: 0x%x (%s), group_id: 0x%x"
 */
void ufshcd_command_event::handle(ulong addr){
    // Decode str_t
    std::shared_ptr<trace_field> field_ptr = field_maps["str_t"];
    if (!field_ptr) return;
    int str_t = plugin_ptr->read_int(addr + field_ptr->offset, "str_t");
    copy_str(arg_list[0], decode_ufs_trace_str_t(str_t));

    // Read dev_name (from sdev->sdev_dev)
    // Note: This is complex, we'll read it as a string field if available
    arg_list[1]->name = "dev_name";
    read_trace_field(addr, arg_list[1]);

    // Read tag, doorbell, transfer_len, intr, lba
    arg_list[2]->name = "tag";
    arg_list[3]->name = "doorbell";
    arg_list[4]->name = "transfer_len";
    arg_list[5]->name = "intr";
    arg_list[6]->name = "lba";

    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);

    // Read opcode
    arg_list[7]->name = "opcode";
    read_trace_field(addr, arg_list[7]);

    // Decode opcode to string
    unsigned char opcode = *reinterpret_cast<unsigned char*>(arg_list[7]->data);
    std::string opcode_str;
    switch (opcode) {
        case 0x8a: opcode_str = "WRITE_16"; break;
        case 0x2a: opcode_str = "WRITE_10"; break;
        case 0x88: opcode_str = "READ_16"; break;
        case 0x28: opcode_str = "READ_10"; break;
        case 0x35: opcode_str = "SYNC"; break;
        case 0x42: opcode_str = "UNMAP"; break;
        case 0x95: opcode_str = "ZBC_IN"; break;
        case 0x94: opcode_str = "ZBC_OUT"; break;
        default: opcode_str = "UNKNOWN"; break;
    }
    copy_str(arg_list[8], opcode_str);

    // Read group_id
    arg_list[9]->name = "group_id";
    read_trace_field(addr, arg_list[9]);
}

/**
 * @brief Handle ufshcd_clk_gating event
 * Format: "%s: gating state changed to %s"
 */
void ufshcd_clk_gating_event::handle(ulong addr){
    // Read dev_name
    arg_list[0]->name = "dev_name";
    read_trace_field(addr, arg_list[0]);

    // Decode state
    std::shared_ptr<trace_field> field_ptr = field_maps["state"];
    if (!field_ptr) return;
    int state = plugin_ptr->read_int(addr + field_ptr->offset, "state");
    copy_str(arg_list[1], decode_ufs_clk_gating_state(state));
}

void ufshcd_wl_runtime_resume_event::handle(ulong addr){
    ufshcd_runtime_event_handle(addr);
}

void ufshcd_wl_runtime_suspend_event::handle(ulong addr){ ufshcd_runtime_event_handle(addr); }
void ufshcd_wl_resume_event::handle(ulong addr){ ufshcd_runtime_event_handle(addr); }
void ufshcd_wl_suspend_event::handle(ulong addr){ ufshcd_runtime_event_handle(addr); }
void ufshcd_init_event::handle(ulong addr){ ufshcd_runtime_event_handle(addr); }
void ufshcd_runtime_resume_event::handle(ulong addr){ ufshcd_runtime_event_handle(addr); }
void ufshcd_runtime_suspend_event::handle(ulong addr){ ufshcd_runtime_event_handle(addr); }
void ufshcd_system_resume_event::handle(ulong addr){ ufshcd_runtime_event_handle(addr); }
void ufshcd_system_suspend_event::handle(ulong addr){ ufshcd_runtime_event_handle(addr); }

/**
 * @brief Handle ufshcd_upiu event
 * Format: "%s: %s: HDR:%s, %s:%s"
 */
void ufshcd_upiu_event::handle(ulong addr){
    std::shared_ptr<trace_field> field_ptr = field_maps["str_t"];
    if (field_ptr) {
        int str_t = plugin_ptr->read_int(addr + field_ptr->offset, "str_t");
        copy_str(arg_list[0], decode_ufs_trace_str_t(str_t));
    }

    arg_list[1]->name = "dev_name";
    read_trace_field(addr, arg_list[1]);

    // HDR is a byte array - format as hex
    field_ptr = field_maps["hdr"];
    if (field_ptr) {
        unsigned char hdr[12];
        for (int i = 0; i < 12 && i < field_ptr->size; i++) {
            hdr[i] = plugin_ptr->read_byte(addr + field_ptr->offset + i, "hdr");
        }
        copy_str(arg_list[2], format_hex_string(hdr, 12));
    }

    field_ptr = field_maps["tsf_t"];
    if (field_ptr) {
        int tsf_t = plugin_ptr->read_int(addr + field_ptr->offset, "tsf_t");
        copy_str(arg_list[3], decode_ufs_trace_tsf_t(tsf_t));
    }

    // TSF is a byte array
    field_ptr = field_maps["tsf"];
    if (field_ptr) {
        unsigned char tsf[16];
        for (int i = 0; i < 16 && i < field_ptr->size; i++) {
            tsf[i] = plugin_ptr->read_byte(addr + field_ptr->offset + i, "tsf");
        }
        copy_str(arg_list[4], format_hex_string(tsf, 16));
    }
}

/**
 * @brief Handle ufshcd_uic_command event
 * Format: "%s: %s: cmd: 0x%x, arg1: 0x%x, arg2: 0x%x, arg3: 0x%x"
 */
void ufshcd_uic_command_event::handle(ulong addr){
    std::shared_ptr<trace_field> field_ptr = field_maps["str_t"];
    if (field_ptr) {
        int str_t = plugin_ptr->read_int(addr + field_ptr->offset, "str_t");
        copy_str(arg_list[0], decode_ufs_trace_str_t(str_t));
    }

    arg_list[1]->name = "dev_name";
    arg_list[2]->name = "cmd";
    arg_list[3]->name = "arg1";
    arg_list[4]->name = "arg2";
    arg_list[5]->name = "arg3";

    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);
}

// Ext4 fallocate events (same format)
void ext4_fallocate_enter_event::handle(ulong addr){
    ext4_event_fallocate_punch_zero_handle(addr);
}
void ext4_punch_hole_event::handle(ulong addr){
    ext4_event_fallocate_punch_zero_handle(addr);
}

void ext4_zero_range_event::handle(ulong addr){
    ext4_event_fallocate_punch_zero_handle(addr);
}

// Ext4 map blocks events
void ext4_ind_map_blocks_enter_event::handle(ulong addr){
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (field_ptr) {
        unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
        *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));
    }
    arg_list[2]->name = "ino";
    arg_list[3]->name = "lblk";
    arg_list[4]->name = "len";
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);

    field_ptr = field_maps["flags"];
    if (field_ptr) {
        unsigned int flags = plugin_ptr->read_uint(addr + field_ptr->offset, "flags");
        copy_str(arg_list[5], decode_ext4_map_flags(flags));
    }
}

void ext4_ind_map_blocks_exit_event::handle(ulong addr){
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (field_ptr) {
        unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
        *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));
    }
    arg_list[2]->name = "ino";
    read_trace_field(addr, arg_list[2]);

    field_ptr = field_maps["flags"];
    if (field_ptr) {
        unsigned int flags = plugin_ptr->read_uint(addr + field_ptr->offset, "flags");
        copy_str(arg_list[3], decode_ext4_map_flags(flags));
    }

    arg_list[4]->name = "lblk";
    arg_list[5]->name = "pblk";
    arg_list[6]->name = "len";
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);

    field_ptr = field_maps["mflags"];
    if (field_ptr) {
        unsigned int mflags = plugin_ptr->read_uint(addr + field_ptr->offset, "mflags");
        copy_str(arg_list[7], decode_ext4_mflags(mflags));
    }

    arg_list[8]->name = "ret";
    read_trace_field(addr, arg_list[8]);
}

// Ext4 extent status events (similar format)
void ext4_es_insert_extent_event::handle(ulong addr){
    ext4_es_extent_event_handle(addr);
}
void ext4_es_cache_extent_event::handle(ulong addr){ ext4_es_extent_event_handle(addr); }
void ext4_es_find_extent_range_exit_event::handle(ulong addr){ ext4_es_extent_event_handle(addr); }

void ext4_es_lookup_extent_exit_event::handle(ulong addr){
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (field_ptr) {
        unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
        *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));
    }
    arg_list[2]->name = "ino";
    arg_list[3]->name = "found";
    arg_list[4]->name = "lblk";
    arg_list[5]->name = "len";
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);

    int found = *reinterpret_cast<int*>(arg_list[3]->data);
    if (found) {
        arg_list[6]->name = "pblk";
        read_trace_field(addr, arg_list[6]);
        field_ptr = field_maps["status"];
        if (field_ptr) {
            unsigned int status = plugin_ptr->read_uint(addr + field_ptr->offset, "status");
            copy_str(arg_list[7], decode_ext4_es_status(status));
        }
    } else {
        *reinterpret_cast<unsigned long long*>(arg_list[6]->data) = 0;
        copy_str(arg_list[7], "");
    }
}

void ext4_es_insert_delayed_block_event::handle(ulong addr){
    ext4_es_extent_event_handle(addr);
    arg_list[7]->name = "allocated";
    read_trace_field(addr, arg_list[7]);
}

/**
 * @brief Handle spmi_read_end event
 * Format: "opc=%d sid=%02d addr=0x%04x ret=%d len=%02d buf=0x[%*phD]"
 */
void spmi_read_end_event::handle(ulong addr){
    // Read opcode, sid, addr
    arg_list[0]->name = "opcode";
    arg_list[1]->name = "sid";
    arg_list[2]->name = "addr";
    arg_list[3]->name = "ret";
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

/**
 * @brief Handle spmi_write_begin event
 * Format: "opc=%d sid=%02d addr=0x%04x len=%d buf=0x[%*phD]"
 */
void spmi_write_begin_event::handle(ulong addr){
    // Read opcode, sid, addr, len
    arg_list[0]->name = "opcode";
    arg_list[1]->name = "sid";
    arg_list[2]->name = "addr";
    arg_list[3]->name = "len";

    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);

    // Read buffer from dynamic array
    std::shared_ptr<trace_field> field_ptr = field_maps["buf"];
    if (field_ptr) {
        int len = *reinterpret_cast<int*>(arg_list[3]->data);
        if (len > 0 && len <= 32) {
            unsigned char buf[32];
            for (int i = 0; i < len; i++) {
                buf[i] = plugin_ptr->read_byte(addr + field_ptr->offset + i, "buf");
            }
            copy_str(arg_list[4], format_buffer_hex(buf, len));
        } else {
            copy_str(arg_list[4], "[]");
        }
    }
}

/**
 * @brief Handle spi_transfer_start event
 * Format: "spi%d.%d %p len=%d tx=[%*phD] rx=[%*phD]"
 */
void spi_transfer_start_event::handle(ulong addr){
    spi_transfer_event_handle(addr);
}

/**
 * @brief Handle spi_transfer_stop event (same as start)
 */
void spi_transfer_stop_event::handle(ulong addr){
    spi_transfer_event_handle(addr);
}

/**
 * @brief Handle spi_set_cs event
 * Format: "spi%d.%d %s%s"
 */
void spi_set_cs_event::handle(ulong addr){
    // Read bus_num, chip_select
    arg_list[0]->name = "bus_num";
    arg_list[1]->name = "chip_select";

    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);

    // Decode enable flag
    std::shared_ptr<trace_field> field_ptr = field_maps["enable"];
    if (field_ptr) {
        int enable = plugin_ptr->read_int(addr + field_ptr->offset, "enable");
        copy_str(arg_list[2], enable ? "activate" : "deactivate");
    }

    // Check cs_high mode flag
    field_ptr = field_maps["mode"];
    if (field_ptr) {
        unsigned long mode = plugin_ptr->read_ulong(addr + field_ptr->offset, "mode");
        if (mode & (1UL << 2)) {
            copy_str(arg_list[3], ", cs_high");
        } else {
            copy_str(arg_list[3], "");
        }
    }
}

/**
 * @brief Handle spi_setup event
 * Format: "spi%d.%d setup mode %lu, %s%s%s%s%u bits/w, %u Hz max --> %d"
 */
void spi_setup_event::handle(ulong addr){
    // Read bus_num, chip_select
    arg_list[0]->name = "bus_num";
    arg_list[1]->name = "chip_select";

    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);

    // Decode mode
    std::shared_ptr<trace_field> field_ptr = field_maps["mode"];
    if (field_ptr) {
        unsigned long mode = plugin_ptr->read_ulong(addr + field_ptr->offset, "mode");
        copy_str(arg_list[2], decode_spi_mode_flags(mode));
    }

    // Read bits_per_word, max_speed_hz, status
    arg_list[3]->name = "bits_per_word";
    arg_list[4]->name = "max_speed_hz";
    arg_list[5]->name = "status";

    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);
}

// USB Gadget request events (same format for 5 events)
void usb_gadget_giveback_request_event::handle(ulong addr){
    usb_gadget_request_event_handle(addr);
}
void usb_ep_dequeue_event::handle(ulong addr){ usb_gadget_request_event_handle(addr); }
void usb_ep_queue_event::handle(ulong addr){ usb_gadget_request_event_handle(addr); }
void usb_ep_free_request_event::handle(ulong addr){ usb_gadget_request_event_handle(addr); }
void usb_ep_alloc_request_event::handle(ulong addr){ usb_gadget_request_event_handle(addr); }

// USB EP events (same format for 8 events)
void usb_ep_enable_event::handle(ulong addr){
    usb_ep_event_handle(addr);
}
void usb_ep_disable_event::handle(ulong addr){ usb_ep_event_handle(addr); }
void usb_ep_set_halt_event::handle(ulong addr){ usb_ep_event_handle(addr); }
void usb_ep_clear_halt_event::handle(ulong addr){ usb_ep_event_handle(addr); }
void usb_ep_set_wedge_event::handle(ulong addr){ usb_ep_event_handle(addr); }
void usb_ep_fifo_status_event::handle(ulong addr){ usb_ep_event_handle(addr); }
void usb_ep_fifo_flush_event::handle(ulong addr){ usb_ep_event_handle(addr); }
void usb_ep_set_maxpacket_limit_event::handle(ulong addr){ usb_ep_event_handle(addr); }

// USB Gadget device events (same format for 13 events)
void usb_gadget_wakeup_event::handle(ulong addr){ usb_gadget_event_handle(addr); }
void usb_gadget_set_remote_wakeup_event::handle(ulong addr){ usb_gadget_event_handle(addr); }
void usb_gadget_set_selfpowered_event::handle(ulong addr){ usb_gadget_event_handle(addr); }
void usb_gadget_clear_selfpowered_event::handle(ulong addr){ usb_gadget_event_handle(addr); }
void usb_gadget_vbus_connect_event::handle(ulong addr){ usb_gadget_event_handle(addr); }
void usb_gadget_vbus_draw_event::handle(ulong addr){ usb_gadget_event_handle(addr); }
void usb_gadget_vbus_disconnect_event::handle(ulong addr){ usb_gadget_event_handle(addr); }
void usb_gadget_connect_event::handle(ulong addr){ usb_gadget_event_handle(addr); }
void usb_gadget_disconnect_event::handle(ulong addr){ usb_gadget_event_handle(addr); }
void usb_gadget_deactivate_event::handle(ulong addr){ usb_gadget_event_handle(addr); }
void usb_gadget_activate_event::handle(ulong addr){ usb_gadget_event_handle(addr); }

// xHCI DBC events (same format for 4 events)
void xhci_dbc_alloc_request_event::handle(ulong addr){
    xhci_dbc_event_handle(addr);
}
void xhci_dbc_free_request_event::handle(ulong addr){ xhci_dbc_event_handle(addr); }
void xhci_dbc_queue_request_event::handle(ulong addr){ xhci_dbc_event_handle(addr); }
void xhci_dbc_giveback_request_event::handle(ulong addr){ xhci_dbc_event_handle(addr); }

// xHCI URB events (same format for 3 events)
void xhci_urb_enqueue_event::handle(ulong addr){
    xhci_urb_event_handle(addr);
}
void xhci_urb_giveback_event::handle(ulong addr){ xhci_urb_event_handle(addr);; }
void xhci_urb_dequeue_event::handle(ulong addr){ xhci_urb_event_handle(addr);; }

// DWC3 request events (same format for 5 events)
void dwc3_alloc_request_event::handle(ulong addr){
    dwc3_request_event_handle(addr);
}
void dwc3_free_request_event::handle(ulong addr){ dwc3_request_event_handle(addr); }
void dwc3_ep_queue_event::handle(ulong addr){ dwc3_request_event_handle(addr); }
void dwc3_ep_dequeue_event::handle(ulong addr){ dwc3_request_event_handle(addr); }
void dwc3_gadget_giveback_event::handle(ulong addr){ dwc3_request_event_handle(addr); }

// DWC3 TRB events (same format for 2 events)
void dwc3_prepare_trb_event::handle(ulong addr){
    dwc3_trb_event_handle(addr);
}
void dwc3_complete_trb_event::handle(ulong addr){ dwc3_trb_event_handle(addr); }

// DWC3 endpoint events (same format for 2 events)
void dwc3_gadget_ep_enable_event::handle(ulong addr){
    dwc3_gadget_ep_event_handle(addr);
}
void dwc3_gadget_ep_disable_event::handle(ulong addr){ dwc3_gadget_ep_event_handle(addr); }

// F2FS compression events
void f2fs_compress_pages_start_event::handle(ulong addr){
    f2fs_compress_pages_event_handle(addr);
}
void f2fs_decompress_pages_start_event::handle(ulong addr){ f2fs_compress_pages_event_handle(addr); }

// F2FS page operation events
void f2fs_writepage_event::handle(ulong addr){
    f2fs_writepage_event_handle(addr);
}
void f2fs_do_write_data_page_event::handle(ulong addr){ f2fs_writepage_event_handle(addr); }
void f2fs_readpage_event::handle(ulong addr){ f2fs_writepage_event_handle(addr); }
void f2fs_set_page_dirty_event::handle(ulong addr){ f2fs_writepage_event_handle(addr); }
void f2fs_vm_page_mkwrite_event::handle(ulong addr){ f2fs_writepage_event_handle(addr); }

// F2FS writepages event
void f2fs_writepages_event::handle(ulong addr){
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (field_ptr) {
        unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
        *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));
    }
    arg_list[2]->name = "ino";
    read_trace_field(addr, arg_list[2]);
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
    arg_list[5]->name = "nr_to_write";
    arg_list[6]->name = "pages_skipped";
    arg_list[7]->name = "range_start";
    arg_list[8]->name = "range_end";
    arg_list[9]->name = "writeback_index";
    arg_list[10]->name = "sync_mode";
    arg_list[11]->name = "for_kupdate";
    arg_list[12]->name = "for_background";
    arg_list[13]->name = "tagged_writepages";
    arg_list[14]->name = "for_reclaim";
    arg_list[15]->name = "range_cyclic";
    arg_list[16]->name = "for_sync";
    for (int i = 5; i <= 16; i++) {
        read_trace_field(addr, arg_list[i]);
    }
}

// F2FS sync events
void f2fs_sync_file_exit_event::handle(ulong addr){
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (field_ptr) {
        unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
        *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));
    }
    arg_list[2]->name = "ino";
    arg_list[4]->name = "datasync";
    arg_list[5]->name = "ret";
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);
    field_ptr = field_maps["cp_reason"];
    if (field_ptr) {
        int cp_reason = plugin_ptr->read_int(addr + field_ptr->offset, "cp_reason");
        copy_str(arg_list[3], decode_f2fs_cp_reason(cp_reason));
    }
}

void f2fs_sync_fs_event::handle(ulong addr){
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (field_ptr) {
        unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
        *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));
    }
    field_ptr = field_maps["dirty"];
    if (field_ptr) {
        int dirty = plugin_ptr->read_int(addr + field_ptr->offset, "dirty");
        copy_str(arg_list[2], dirty ? "dirty" : "not dirty");
    }
    arg_list[3]->name = "wait";
    read_trace_field(addr, arg_list[3]);
}

void f2fs_sync_dirty_inodes_enter_event::handle(ulong addr){
    f2fs_sync_dirty_event_handle(addr);
}
void f2fs_sync_dirty_inodes_exit_event::handle(ulong addr){ f2fs_sync_dirty_event_handle(addr); }

void f2fs_shutdown_event::handle(ulong addr){
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (field_ptr) {
        unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
        *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));
    }
    field_ptr = field_maps["mode"];
    if (field_ptr) {
        int mode = plugin_ptr->read_int(addr + field_ptr->offset, "mode");
        copy_str(arg_list[2], decode_f2fs_shutdown_mode(mode));
    }
    arg_list[3]->name = "ret";
    read_trace_field(addr, arg_list[3]);
}

// F2FS extent tree events
void f2fs_lookup_extent_tree_start_event::handle(ulong addr){
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (field_ptr) {
        unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
        *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));
    }
    arg_list[2]->name = "ino";
    arg_list[3]->name = "pgofs";
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    field_ptr = field_maps["type"];
    if (field_ptr) {
        int type = plugin_ptr->read_int(addr + field_ptr->offset, "type");
        copy_str(arg_list[4], decode_f2fs_extent_type(type));
    }
}

void f2fs_shrink_extent_tree_event::handle(ulong addr){
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (field_ptr) {
        unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
        *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));
    }
    arg_list[2]->name = "node_cnt";
    arg_list[3]->name = "tree_cnt";
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    field_ptr = field_maps["type"];
    if (field_ptr) {
        int type = plugin_ptr->read_int(addr + field_ptr->offset, "type");
        copy_str(arg_list[4], decode_f2fs_extent_type(type));
    }
}

void f2fs_destroy_extent_tree_event::handle(ulong addr){
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (field_ptr) {
        unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
        *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));
    }
    arg_list[2]->name = "ino";
    arg_list[3]->name = "node_cnt";
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    field_ptr = field_maps["type"];
    if (field_ptr) {
        int type = plugin_ptr->read_int(addr + field_ptr->offset, "type");
        copy_str(arg_list[4], decode_f2fs_extent_type(type));
    }
}

// F2FS other events
void f2fs_truncate_partial_nodes_event::handle(ulong addr){
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (field_ptr) {
        unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
        *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));
    }
    arg_list[2]->name = "ino";
    arg_list[3]->name = "nid[0]";
    arg_list[4]->name = "nid[1]";
    arg_list[5]->name = "nid[2]";
    arg_list[6]->name = "depth";
    arg_list[7]->name = "err";
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);
    read_trace_field(addr, arg_list[7]);
}

// F2FS submit page events
void f2fs_submit_page_bio_event::handle(ulong addr){
    f2fs_submit_page_event_handle(addr);
}
void f2fs_submit_page_write_event::handle(ulong addr){ f2fs_submit_page_event_handle(addr); }

// F2FS BIO events
void f2fs_prepare_write_bio_event::handle(ulong addr){
    f2fs_prepare_bio_event_handle(addr);
}
void f2fs_prepare_read_bio_event::handle(ulong addr){ f2fs_prepare_bio_event_handle(addr); }
void f2fs_submit_read_bio_event::handle(ulong addr){ f2fs_prepare_bio_event_handle(addr); }
void f2fs_submit_write_bio_event::handle(ulong addr){ f2fs_prepare_bio_event_handle(addr); }

// F2FS GC events
void f2fs_gc_begin_event::handle(ulong addr){
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (field_ptr) {
        unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
        *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));
    }
    field_ptr = field_maps["gc_type"];
    if (field_ptr) {
        int gc_type = plugin_ptr->read_int(addr + field_ptr->offset, "gc_type");
        copy_str(arg_list[2], decode_f2fs_gc_type(gc_type));
    }
    arg_list[3]->name = "no_bg_gc";
    arg_list[4]->name = "nr_free_secs";
    arg_list[5]->name = "dirty_nodes";
    arg_list[6]->name = "dirty_dents";
    arg_list[7]->name = "dirty_imeta";
    arg_list[8]->name = "free_sec";
    arg_list[9]->name = "free_seg";
    arg_list[10]->name = "reserved_seg";
    arg_list[11]->name = "prefree_seg";
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

void f2fs_get_victim_event::handle(ulong addr){
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (field_ptr) {
        unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
        *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));
    }
    field_ptr = field_maps["type"];
    if (field_ptr) {
        int type = plugin_ptr->read_int(addr + field_ptr->offset, "type");
        copy_str(arg_list[2], decode_f2fs_victim_type(type));
    }
    field_ptr = field_maps["gc_type"];
    if (field_ptr) {
        int gc_type = plugin_ptr->read_int(addr + field_ptr->offset, "gc_type");
        copy_str(arg_list[3], decode_f2fs_gc_type(gc_type));
    }
    field_ptr = field_maps["alloc_mode"];
    if (field_ptr) {
        int alloc_mode = plugin_ptr->read_int(addr + field_ptr->offset, "alloc_mode");
        copy_str(arg_list[4], decode_f2fs_alloc_mode(alloc_mode));
    }
    field_ptr = field_maps["gc_mode"];
    if (field_ptr) {
        int gc_mode = plugin_ptr->read_int(addr + field_ptr->offset, "gc_mode");
        copy_str(arg_list[5], decode_f2fs_gc_mode(gc_mode));
    }
    arg_list[6]->name = "victim";
    arg_list[7]->name = "cost";
    arg_list[8]->name = "ofs_unit";
    arg_list[9]->name = "pre_victim";
    arg_list[10]->name = "prefree";
    arg_list[11]->name = "free";
    read_trace_field(addr, arg_list[6]);
    read_trace_field(addr, arg_list[7]);
    read_trace_field(addr, arg_list[8]);
    read_trace_field(addr, arg_list[9]);
    read_trace_field(addr, arg_list[10]);
    read_trace_field(addr, arg_list[11]);
}

// F2FS checkpoint events
void f2fs_write_checkpoint_event::handle(ulong addr){
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (field_ptr) {
        unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
        *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));
    }
    arg_list[2]->name = "reason";
    arg_list[3]->name = "dest_msg";
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
}

void f2fs_issue_flush_event::handle(ulong addr){
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (field_ptr) {
        unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
        *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));
    }
    field_ptr = field_maps["nobarrier"];
    if (field_ptr) {
        int nobarrier = plugin_ptr->read_int(addr + field_ptr->offset, "nobarrier");
        copy_str(arg_list[2], nobarrier ? "skip (nobarrier)" : "issue");
    }
    field_ptr = field_maps["flush_merge"];
    if (field_ptr) {
        int flush_merge = plugin_ptr->read_int(addr + field_ptr->offset, "flush_merge");
        copy_str(arg_list[3], flush_merge ? " with flush_merge" : "");
    }
    arg_list[4]->name = "ret";
    read_trace_field(addr, arg_list[4]);
}

// V4L2 videobuf2 events (same format for 4 events)
void vb2_v4l2_buf_done_event::handle(ulong addr){
    v4l2_buf_event_handle(addr);
}
void vb2_v4l2_buf_queue_event::handle(ulong addr){ v4l2_buf_event_handle(addr); }
void vb2_v4l2_dqbuf_event::handle(ulong addr){ v4l2_buf_event_handle(addr); }
void vb2_v4l2_qbuf_event::handle(ulong addr){ v4l2_buf_event_handle(addr); }

// V4L2 events (same format for 2 events)
void v4l2_dqbuf_event::handle(ulong addr){
    v4l2_dqbuf_event_handle(addr);
}
void v4l2_qbuf_event::handle(ulong addr){ v4l2_dqbuf_event_handle(addr); }

// I2C events (same format for write, reply, and read)
void i2c_write_event::handle(ulong addr){
    i2c_event_handle(addr);
}
void i2c_reply_event::handle(ulong addr){ i2c_event_handle(addr); }
void i2c_read_event::handle(ulong addr){ i2c_event_handle(addr); }

// I2C result event
void i2c_result_event::handle(ulong addr){
    arg_list[0]->name = "adapter_nr";
    arg_list[1]->name = "nr_msgs";
    arg_list[2]->name = "ret";

    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);
}

// SMBus events
void smbus_write_event::handle(ulong addr){
    smbus_event_handle(addr);
}

void smbus_read_event::handle(ulong addr){
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
}

void smbus_reply_event::handle(ulong addr){ smbus_event_handle(addr); }

void smbus_result_event::handle(ulong addr){
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

    field_ptr = field_maps["read_write"];
    if (field_ptr) {
        int read_write = plugin_ptr->read_int(addr + field_ptr->offset, "read_write");
        copy_str(arg_list[5], read_write == 0 ? "wr" : "rd");
    }

    arg_list[6]->name = "res";
    read_trace_field(addr, arg_list[6]);
}

// Thermal events
void thermal_zone_trip_event::handle(ulong addr){
    arg_list[0]->name = "thermal_zone";
    arg_list[1]->name = "id";
    arg_list[2]->name = "trip";
    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);

    std::shared_ptr<trace_field> field_ptr = field_maps["trip_type"];
    if (field_ptr) {
        int trip_type = plugin_ptr->read_int(addr + field_ptr->offset, "trip_type");
        copy_str(arg_list[3], decode_thermal_trip_type(trip_type));
    }
}

void thermal_power_cpu_get_power_event::handle(ulong addr){
    arg_list[0]->name = "cpumask";
    arg_list[1]->name = "freq";
    arg_list[3]->name = "dynamic_power";
    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[3]);

    // Load array - simplified, just mark as array
    copy_str(arg_list[2], "[load_array]");
}

void thermal_power_devfreq_get_power_event::handle(ulong addr){
    arg_list[0]->name = "type";
    arg_list[1]->name = "freq";
    arg_list[3]->name = "power";
    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[3]);

    // Calculate load
    std::shared_ptr<trace_field> field_ptr = field_maps["total_time"];
    if (field_ptr) {
        unsigned long total_time = plugin_ptr->read_ulong(addr + field_ptr->offset, "total_time");
        field_ptr = field_maps["busy_time"];
        if (field_ptr) {
            unsigned long busy_time = plugin_ptr->read_ulong(addr + field_ptr->offset, "busy_time");
            unsigned long load = (total_time == 0) ? 0 : (100 * busy_time) / total_time;
            *reinterpret_cast<unsigned long*>(arg_list[2]->data) = load;
        }
    }
}

void thermal_power_allocator_event::handle(ulong addr){
    arg_list[0]->name = "tz_id";
    arg_list[2]->name = "total_req_power";
    arg_list[4]->name = "total_granted_power";
    arg_list[5]->name = "power_range";
    arg_list[6]->name = "max_allocatable_power";
    arg_list[7]->name = "current_temp";
    arg_list[8]->name = "delta_temp";

    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);
    read_trace_field(addr, arg_list[7]);
    read_trace_field(addr, arg_list[8]);

    // Power arrays - simplified
    copy_str(arg_list[1], "[req_power_array]");
    copy_str(arg_list[3], "[granted_power_array]");
}

// RCU events
void rcu_batch_end_event::handle(ulong addr){
    arg_list[0]->name = "rcuname";
    arg_list[1]->name = "callbacks_invoked";
    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);

    std::shared_ptr<trace_field> field_ptr = field_maps["cb"];
    char cb_char = '.';
    if (field_ptr) {
        int cb = plugin_ptr->read_int(addr + field_ptr->offset, "cb");
        cb_char = cb ? 'C' : '.';
    }

    field_ptr = field_maps["nr"];
    char nr_char = '.';
    if (field_ptr) {
        int nr = plugin_ptr->read_int(addr + field_ptr->offset, "nr");
        nr_char = nr ? 'S' : '.';
    }

    field_ptr = field_maps["iit"];
    char iit_char = '.';
    if (field_ptr) {
        int iit = plugin_ptr->read_int(addr + field_ptr->offset, "iit");
        iit_char = iit ? 'I' : '.';
    }

    field_ptr = field_maps["risk"];
    char risk_char = '.';
    if (field_ptr) {
        int risk = plugin_ptr->read_int(addr + field_ptr->offset, "risk");
        risk_char = risk ? 'R' : '.';
    }

    char idle_str[5];
    snprintf(idle_str, sizeof(idle_str), "%c%c%c%c", cb_char, nr_char, iit_char, risk_char);
    copy_str(arg_list[2], idle_str);
}

void rcu_segcb_stats_event::handle(ulong addr){
    arg_list[0]->name = "ctx";
    read_trace_field(addr, arg_list[0]);

    // Read seglen array
    arg_list[1]->name = "seglen[0]";
    arg_list[2]->name = "seglen[1]";
    arg_list[3]->name = "seglen[2]";
    arg_list[4]->name = "seglen[3]";
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);

    // Read gp_seq array
    arg_list[5]->name = "gp_seq[0]";
    arg_list[6]->name = "gp_seq[1]";
    arg_list[7]->name = "gp_seq[2]";
    arg_list[8]->name = "gp_seq[3]";
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);
    read_trace_field(addr, arg_list[7]);
    read_trace_field(addr, arg_list[8]);
}

// SMC Invoke events
void smcinvoke_ioctl_event::handle(ulong addr){
    std::shared_ptr<trace_field> field_ptr = field_maps["cmd"];
    if (field_ptr) {
        unsigned int cmd = plugin_ptr->read_uint(addr + field_ptr->offset, "cmd");
        copy_str(arg_list[0], decode_smcinvoke_cmd(cmd));
    }

    arg_list[1]->name = "ret";
    read_trace_field(addr, arg_list[1]);
}

/**
 * @brief Handle mm_vmscan_lru_isolate event
 * Format: "isolate_mode=%d classzone=%d order=%d nr_requested=%lu nr_scanned=%lu nr_skipped=%lu nr_taken=%lu lru=%s"
 */
void mm_vmscan_lru_isolate_event::handle(ulong addr){
    // Read isolate_mode
    arg_list[0]->name = "isolate_mode";
    read_trace_field(addr, arg_list[0]);

    // Read classzone (zone index)
    std::shared_ptr<trace_field> field_ptr = field_maps["highest_zoneidx"];
    if (field_ptr) {
        int zone_idx = plugin_ptr->read_int(addr + field_ptr->offset, "highest_zoneidx");
        copy_str(arg_list[1], decode_zone_type(zone_idx));
    }

    // Read remaining fields
    arg_list[2]->name = "order";
    arg_list[3]->name = "nr_requested";
    arg_list[4]->name = "nr_scanned";
    arg_list[5]->name = "nr_skipped";
    arg_list[6]->name = "nr_taken";
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);

    // Decode lru type
    field_ptr = field_maps["lru"];
    if (field_ptr) {
        int lru = plugin_ptr->read_int(addr + field_ptr->offset, "lru");
        copy_str(arg_list[7], decode_lru_type(lru));
    }
}

/**
 * @brief Handle mm_vmscan_writepage event
 * Format: "page=%p pfn=0x%lx flags=%s"
 */
void mm_vmscan_writepage_event::handle(ulong addr){
    // Read pfn (page frame number)
    arg_list[1]->name = "pfn";
    read_trace_field(addr, arg_list[1]);

    // For page address, we store pfn value
    unsigned long pfn = *reinterpret_cast<unsigned long*>(arg_list[1]->data);
    *reinterpret_cast<unsigned long*>(arg_list[0]->data) = pfn;

    // Decode reclaim flags
    std::shared_ptr<trace_field> field_ptr = field_maps["reclaim_flags"];
    if (field_ptr) {
        unsigned int flags = plugin_ptr->read_uint(addr + field_ptr->offset, "reclaim_flags");
        copy_str(arg_list[2], decode_reclaim_flags(flags));
    }
}

/**
 * @brief Handle mm_vmscan_lru_shrink_inactive event
 * Format: "nid=%d nr_scanned=%ld nr_reclaimed=%ld nr_dirty=%ld nr_writeback=%ld nr_congested=%ld nr_immediate=%ld nr_activate_anon=%d nr_activate_file=%d nr_ref_keep=%ld nr_unmap_fail=%ld priority=%d flags=%s"
 */
void mm_vmscan_lru_shrink_inactive_event::handle(ulong addr){
    // Read all numeric fields
    arg_list[0]->name = "nid";
    arg_list[1]->name = "nr_scanned";
    arg_list[2]->name = "nr_reclaimed";
    arg_list[3]->name = "nr_dirty";
    arg_list[4]->name = "nr_writeback";
    arg_list[5]->name = "nr_congested";
    arg_list[6]->name = "nr_immediate";
    arg_list[7]->name = "nr_activate_anon";
    arg_list[8]->name = "nr_activate_file";
    arg_list[9]->name = "nr_ref_keep";
    arg_list[10]->name = "nr_unmap_fail";
    arg_list[11]->name = "priority";

    for (int i = 0; i <= 11; i++) {
        read_trace_field(addr, arg_list[i]);
    }

    // Decode reclaim flags
    std::shared_ptr<trace_field> field_ptr = field_maps["reclaim_flags"];
    if (field_ptr) {
        unsigned int flags = plugin_ptr->read_uint(addr + field_ptr->offset, "reclaim_flags");
        copy_str(arg_list[12], decode_reclaim_flags(flags));
    }
}

/**
 * @brief Handle mm_vmscan_lru_shrink_active event
 * Format: "nid=%d nr_taken=%ld nr_active=%ld nr_deactivated=%ld nr_referenced=%ld priority=%d flags=%s"
 */
void mm_vmscan_lru_shrink_active_event::handle(ulong addr){
    // Read all numeric fields
    arg_list[0]->name = "nid";
    arg_list[1]->name = "nr_taken";
    arg_list[2]->name = "nr_active";
    arg_list[3]->name = "nr_deactivated";
    arg_list[4]->name = "nr_referenced";
    arg_list[5]->name = "priority";

    for (int i = 0; i <= 5; i++) {
        read_trace_field(addr, arg_list[i]);
    }

    // Decode reclaim flags
    std::shared_ptr<trace_field> field_ptr = field_maps["reclaim_flags"];
    if (field_ptr) {
        unsigned int flags = plugin_ptr->read_uint(addr + field_ptr->offset, "reclaim_flags");
        copy_str(arg_list[6], decode_reclaim_flags(flags));
    }
}

/**
 * @brief Handle mm_compaction_begin event
 * Format: "zone_start=0x%lx migrate_pfn=0x%lx free_pfn=0x%lx zone_end=0x%lx, mode=%s"
 */
void mm_compaction_begin_event::handle(ulong addr){
    // Read all pfn fields
    arg_list[0]->name = "zone_start";
    arg_list[1]->name = "migrate_pfn";
    arg_list[2]->name = "free_pfn";
    arg_list[3]->name = "zone_end";

    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);

    // Decode sync mode
    std::shared_ptr<trace_field> field_ptr = field_maps["sync"];
    if (field_ptr) {
        int sync = plugin_ptr->read_int(addr + field_ptr->offset, "sync");
        copy_str(arg_list[4], decode_migrate_mode(sync));
    }
}

/**
 * @brief Handle mm_compaction_end event
 * Format: "zone_start=0x%lx migrate_pfn=0x%lx free_pfn=0x%lx zone_end=0x%lx, mode=%s status=%s"
 */
void mm_compaction_end_event::handle(ulong addr){
    // Read all pfn fields
    arg_list[0]->name = "zone_start";
    arg_list[1]->name = "migrate_pfn";
    arg_list[2]->name = "free_pfn";
    arg_list[3]->name = "zone_end";

    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);

    // Decode sync mode
    std::shared_ptr<trace_field> field_ptr = field_maps["sync"];
    if (field_ptr) {
        int sync = plugin_ptr->read_int(addr + field_ptr->offset, "sync");
        copy_str(arg_list[4], decode_migrate_mode(sync));
    }

    // Decode status
    field_ptr = field_maps["status"];
    if (field_ptr) {
        int status = plugin_ptr->read_int(addr + field_ptr->offset, "status");
        copy_str(arg_list[5], decode_compaction_status(status));
    }
}

/**
 * @brief Handle mm_compaction_try_to_compact_pages event
 * Format: "order=%d gfp_mask=%s mode=%s"
 */
void mm_compaction_try_to_compact_pages_event::handle(ulong addr){
    // Read order
    arg_list[0]->name = "order";
    read_trace_field(addr, arg_list[0]);

    // Decode gfp_mask
    std::shared_ptr<trace_field> field_ptr = field_maps["gfp_mask"];
    if (field_ptr) {
        unsigned long gfp_mask = plugin_ptr->read_ulong(addr + field_ptr->offset, "gfp_mask");
        copy_str(arg_list[1], decode_gfp_flags(gfp_mask));
    }

    // Decode mode
    field_ptr = field_maps["mode"];
    if (field_ptr) {
        int mode = plugin_ptr->read_int(addr + field_ptr->offset, "mode");
        copy_str(arg_list[2], decode_migrate_mode(mode));
    }
}

/**
 * @brief Handle mm_compaction_finished event
 * Format: "node=%d zone=%-8s order=%d ret=%s"
 */
void mm_compaction_finished_event::handle(ulong addr){
    // Read node
    arg_list[0]->name = "nid";
    read_trace_field(addr, arg_list[0]);

    // Decode zone
    std::shared_ptr<trace_field> field_ptr = field_maps["idx"];
    if (field_ptr) {
        int idx = plugin_ptr->read_int(addr + field_ptr->offset, "idx");
        copy_str(arg_list[1], decode_zone_type(idx));
    }

    // Read order
    arg_list[2]->name = "order";
    read_trace_field(addr, arg_list[2]);

    // Decode ret
    field_ptr = field_maps["ret"];
    if (field_ptr) {
        int ret = plugin_ptr->read_int(addr + field_ptr->offset, "ret");
        copy_str(arg_list[3], decode_compaction_status(ret));
    }
}

/**
 * @brief Handle mm_compaction_suitable event
 * Format: "node=%d zone=%-8s order=%d ret=%s"
 */
void mm_compaction_suitable_event::handle(ulong addr){
    // Read node
    arg_list[0]->name = "nid";
    read_trace_field(addr, arg_list[0]);

    // Decode zone
    std::shared_ptr<trace_field> field_ptr = field_maps["idx"];
    if (field_ptr) {
        int idx = plugin_ptr->read_int(addr + field_ptr->offset, "idx");
        copy_str(arg_list[1], decode_zone_type(idx));
    }

    // Read order
    arg_list[2]->name = "order";
    read_trace_field(addr, arg_list[2]);

    // Decode ret
    field_ptr = field_maps["ret"];
    if (field_ptr) {
        int ret = plugin_ptr->read_int(addr + field_ptr->offset, "ret");
        copy_str(arg_list[3], decode_compaction_status(ret));
    }
}

/**
 * @brief Handle mm_compaction_deferred event
 * Format: "node=%d zone=%-8s order=%d order_failed=%d"
 */
void mm_compaction_deferred_event::handle(ulong addr){
    // Read node
    arg_list[0]->name = "nid";
    read_trace_field(addr, arg_list[0]);

    // Decode zone
    std::shared_ptr<trace_field> field_ptr = field_maps["idx"];
    if (field_ptr) {
        int idx = plugin_ptr->read_int(addr + field_ptr->offset, "idx");
        copy_str(arg_list[1], decode_zone_type(idx));
    }

    // Read order and order_failed
    arg_list[2]->name = "order";
    arg_list[3]->name = "order_failed";
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
}

/**
 * @brief Handle mm_compaction_defer_compaction event
 * Format: "node=%d zone=%-8s order=%d order_failed=%d"
 */
void mm_compaction_defer_compaction_event::handle(ulong addr){
    // Read node
    arg_list[0]->name = "nid";
    read_trace_field(addr, arg_list[0]);

    // Decode zone
    std::shared_ptr<trace_field> field_ptr = field_maps["idx"];
    if (field_ptr) {
        int idx = plugin_ptr->read_int(addr + field_ptr->offset, "idx");
        copy_str(arg_list[1], decode_zone_type(idx));
    }

    // Read order and order_failed
    arg_list[2]->name = "order";
    arg_list[3]->name = "order_failed";
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
}

/**
 * @brief Handle mm_compaction_defer_reset event
 * Format: "node=%d zone=%-8s order=%d order_failed=%d"
 */
void mm_compaction_defer_reset_event::handle(ulong addr){
    // Read node
    arg_list[0]->name = "nid";
    read_trace_field(addr, arg_list[0]);

    // Decode zone
    std::shared_ptr<trace_field> field_ptr = field_maps["idx"];
    if (field_ptr) {
        int idx = plugin_ptr->read_int(addr + field_ptr->offset, "idx");
        copy_str(arg_list[1], decode_zone_type(idx));
    }

    // Read order and order_failed
    arg_list[2]->name = "order";
    arg_list[3]->name = "order_failed";
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
}

/**
 * @brief Handle mm_compaction_wakeup_kcompactd event
 * Format: "nid=%d order=%d highest_zoneidx=%s"
 */
void mm_compaction_wakeup_kcompactd_event::handle(ulong addr){
    // Read nid and order
    arg_list[0]->name = "nid";
    arg_list[1]->name = "order";
    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);

    // Decode highest_zoneidx
    std::shared_ptr<trace_field> field_ptr = field_maps["highest_zoneidx"];
    if (field_ptr) {
        int idx = plugin_ptr->read_int(addr + field_ptr->offset, "highest_zoneidx");
        copy_str(arg_list[2], decode_zone_type(idx));
    }
}

/**
 * @brief Handle mm_compaction_kcompactd_wake event
 * Format: "nid=%d order=%d highest_zoneidx=%s"
 */
void mm_compaction_kcompactd_wake_event::handle(ulong addr){
    // Read nid and order
    arg_list[0]->name = "nid";
    arg_list[1]->name = "order";
    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);

    // Decode highest_zoneidx
    std::shared_ptr<trace_field> field_ptr = field_maps["highest_zoneidx"];
    if (field_ptr) {
        int idx = plugin_ptr->read_int(addr + field_ptr->offset, "highest_zoneidx");
        copy_str(arg_list[2], decode_zone_type(idx));
    }
}

/**
 * @brief Handle mmap_lock_start_locking event
 * Format: "mm=%p memcg_path=%s write=%s"
 */
void mmap_lock_start_locking_event::handle(ulong addr){
    // Read mm pointer
    arg_list[0]->name = "mm";
    read_trace_field(addr, arg_list[0]);

    // Read memcg_path string
    arg_list[1]->name = "memcg_path";
    read_trace_field(addr, arg_list[1]);

    // Decode write flag
    std::shared_ptr<trace_field> field_ptr = field_maps["write"];
    if (field_ptr) {
        int write = plugin_ptr->read_int(addr + field_ptr->offset, "write");
        copy_str(arg_list[2], write ? "true" : "false");
    }
}

/**
 * @brief Handle mmap_lock_acquire_returned event
 * Format: "mm=%p memcg_path=%s write=%s success=%s"
 */
void mmap_lock_acquire_returned_event::handle(ulong addr){
    // Read mm pointer
    arg_list[0]->name = "mm";
    read_trace_field(addr, arg_list[0]);

    // Read memcg_path string
    arg_list[1]->name = "memcg_path";
    read_trace_field(addr, arg_list[1]);

    // Decode write flag
    std::shared_ptr<trace_field> field_ptr = field_maps["write"];
    if (field_ptr) {
        int write = plugin_ptr->read_int(addr + field_ptr->offset, "write");
        copy_str(arg_list[2], write ? "true" : "false");
    }

    // Decode success flag
    field_ptr = field_maps["success"];
    if (field_ptr) {
        int success = plugin_ptr->read_int(addr + field_ptr->offset, "success");
        copy_str(arg_list[3], success ? "true" : "false");
    }
}

/**
 * @brief Handle mmap_lock_released event
 * Format: "mm=%p memcg_path=%s write=%s"
 */
void mmap_lock_released_event::handle(ulong addr){
    // Read mm pointer
    arg_list[0]->name = "mm";
    read_trace_field(addr, arg_list[0]);

    // Read memcg_path string
    arg_list[1]->name = "memcg_path";
    read_trace_field(addr, arg_list[1]);

    // Decode write flag
    std::shared_ptr<trace_field> field_ptr = field_maps["write"];
    if (field_ptr) {
        int write = plugin_ptr->read_int(addr + field_ptr->offset, "write");
        copy_str(arg_list[2], write ? "true" : "false");
    }
}

/**
 * @brief Handle vm_unmapped_area event
 * Format: "addr=0x%lx err=%ld total_vm=%lu flags=0x%lx len=0x%lx lo=0x%lx hi=0x%lx mask=0x%lx ofs=0x%lx"
 */
void vm_unmapped_area_event::handle(ulong addr){
    // Read all fields
    arg_list[0]->name = "addr";
    arg_list[1]->name = "error";
    arg_list[2]->name = "total_vm";
    arg_list[3]->name = "flags";
    arg_list[4]->name = "length";
    arg_list[5]->name = "low_limit";
    arg_list[6]->name = "high_limit";
    arg_list[7]->name = "align_mask";
    arg_list[8]->name = "align_offset";

    for (int i = 0; i <= 8; i++) {
        read_trace_field(addr, arg_list[i]);
    }
}

/**
 * @brief Handle mm_migrate_pages event
 * Format: "nr_succeeded=%lu nr_failed=%lu nr_thp_succeeded=%lu nr_thp_failed=%lu nr_thp_split=%lu mode=%s reason=%s"
 */
void mm_migrate_pages_event::handle(ulong addr){
    // Read all numeric fields
    arg_list[0]->name = "succeeded";
    arg_list[1]->name = "failed";
    arg_list[2]->name = "thp_succeeded";
    arg_list[3]->name = "thp_failed";
    arg_list[4]->name = "thp_split";

    for (int i = 0; i <= 4; i++) {
        read_trace_field(addr, arg_list[i]);
    }

    // Decode mode
    std::shared_ptr<trace_field> field_ptr = field_maps["mode"];
    if (field_ptr) {
        int mode = plugin_ptr->read_int(addr + field_ptr->offset, "mode");
        copy_str(arg_list[5], decode_migrate_mode(mode));
    }

    // Decode reason
    field_ptr = field_maps["reason"];
    if (field_ptr) {
        int reason = plugin_ptr->read_int(addr + field_ptr->offset, "reason");
        copy_str(arg_list[6], decode_migrate_reason(reason));
    }
}

/**
 * @brief Handle mm_migrate_pages_start event
 * Format: "mode=%s reason=%s"
 */
void mm_migrate_pages_start_event::handle(ulong addr){
    // Decode mode
    std::shared_ptr<trace_field> field_ptr = field_maps["mode"];
    if (field_ptr) {
        int mode = plugin_ptr->read_int(addr + field_ptr->offset, "mode");
        copy_str(arg_list[0], decode_migrate_mode(mode));
    }

    // Decode reason
    field_ptr = field_maps["reason"];
    if (field_ptr) {
        int reason = plugin_ptr->read_int(addr + field_ptr->offset, "reason");
        copy_str(arg_list[1], decode_migrate_reason(reason));
    }
}

/**
 * @brief Handle mm_khugepaged_scan_pmd event
 * Format: "mm=%p pfn=0x%lx writable=%d referenced=%d none_or_zero=%d status=%s unmapped=%d"
 */
void mm_khugepaged_scan_pmd_event::handle(ulong addr){
    // Read all fields
    arg_list[0]->name = "mm";
    arg_list[1]->name = "pfn";
    arg_list[2]->name = "writable";
    arg_list[3]->name = "referenced";
    arg_list[4]->name = "none_or_zero";

    for (int i = 0; i <= 4; i++) {
        read_trace_field(addr, arg_list[i]);
    }

    // Decode status
    std::shared_ptr<trace_field> field_ptr = field_maps["status"];
    if (field_ptr) {
        int status = plugin_ptr->read_int(addr + field_ptr->offset, "status");
        copy_str(arg_list[5], decode_khugepaged_status(status));
    }

    // Read unmapped
    arg_list[6]->name = "unmapped";
    read_trace_field(addr, arg_list[6]);
}

/**
 * @brief Handle mm_collapse_huge_page event
 * Format: "mm=%p scan_pfn=0x%lx writable=%d referenced=%d none_or_zero=%d status=%s unmapped=%d"
 */
void mm_collapse_huge_page_event::handle(ulong addr){
    // Read all fields
    arg_list[0]->name = "mm";
    arg_list[1]->name = "pfn";
    arg_list[2]->name = "writable";
    arg_list[3]->name = "referenced";
    arg_list[4]->name = "none_or_zero";

    for (int i = 0; i <= 4; i++) {
        read_trace_field(addr, arg_list[i]);
    }

    // Decode status
    std::shared_ptr<trace_field> field_ptr = field_maps["status"];
    if (field_ptr) {
        int status = plugin_ptr->read_int(addr + field_ptr->offset, "status");
        copy_str(arg_list[5], decode_khugepaged_status(status));
    }

    // Read unmapped
    arg_list[6]->name = "unmapped";
    read_trace_field(addr, arg_list[6]);
}

/**
 * @brief Handle mm_collapse_huge_page_isolate event
 * Format: "pfn=0x%lx none_or_zero=%d referenced=%d writable=%d status=%s"
 */
void mm_collapse_huge_page_isolate_event::handle(ulong addr){
    // Read all fields
    arg_list[0]->name = "pfn";
    arg_list[1]->name = "none_or_zero";
    arg_list[2]->name = "referenced";
    arg_list[3]->name = "writable";

    for (int i = 0; i <= 3; i++) {
        read_trace_field(addr, arg_list[i]);
    }

    // Decode status
    std::shared_ptr<trace_field> field_ptr = field_maps["status"];
    if (field_ptr) {
        int status = plugin_ptr->read_int(addr + field_ptr->offset, "status");
        copy_str(arg_list[4], decode_khugepaged_status(status));
    }
}

/**
 * @brief Handle iomap_iter_dstmap event
 * Format: "dev %d:%d ino 0x%llx bdev %d:%d addr 0x%llx offset 0x%llx length 0x%llx type %s flags %s"
 */
void iomap_iter_dstmap_event::handle(ulong addr){
    // Read dev and extract major:minor
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (field_ptr) {
        unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
        *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));
    }

    // Read ino
    arg_list[2]->name = "ino";
    read_trace_field(addr, arg_list[2]);

    // Read bdev and extract major:minor
    field_ptr = field_maps["bdev"];
    if (field_ptr) {
        unsigned int bdev = plugin_ptr->read_uint(addr + field_ptr->offset, "bdev");
        *reinterpret_cast<unsigned int*>(arg_list[3]->data) = (bdev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[4]->data) = (bdev & ((1U << 20) - 1));
    }

    // Read addr, offset, length
    arg_list[5]->name = "addr";
    arg_list[6]->name = "offset";
    arg_list[7]->name = "length";
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);
    read_trace_field(addr, arg_list[7]);

    // Decode type
    field_ptr = field_maps["type"];
    if (field_ptr) {
        int type = plugin_ptr->read_int(addr + field_ptr->offset, "type");
        copy_str(arg_list[8], decode_iomap_type(type));
    }

    // Decode flags
    field_ptr = field_maps["flags"];
    if (field_ptr) {
        unsigned int flags = plugin_ptr->read_uint(addr + field_ptr->offset, "flags");
        copy_str(arg_list[9], decode_iomap_flags(flags));
    }
}

/**
 * @brief Handle iomap_iter_srcmap event
 * Format: Same as iomap_iter_dstmap
 */
void iomap_iter_srcmap_event::handle(ulong addr){
    // Read dev and extract major:minor
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (field_ptr) {
        unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
        *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));
    }

    // Read ino
    arg_list[2]->name = "ino";
    read_trace_field(addr, arg_list[2]);

    // Read bdev and extract major:minor
    field_ptr = field_maps["bdev"];
    if (field_ptr) {
        unsigned int bdev = plugin_ptr->read_uint(addr + field_ptr->offset, "bdev");
        *reinterpret_cast<unsigned int*>(arg_list[3]->data) = (bdev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[4]->data) = (bdev & ((1U << 20) - 1));
    }

    // Read addr, offset, length
    arg_list[5]->name = "addr";
    arg_list[6]->name = "offset";
    arg_list[7]->name = "length";
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);
    read_trace_field(addr, arg_list[7]);

    // Decode type
    field_ptr = field_maps["type"];
    if (field_ptr) {
        int type = plugin_ptr->read_int(addr + field_ptr->offset, "type");
        copy_str(arg_list[8], decode_iomap_type(type));
    }

    // Decode flags
    field_ptr = field_maps["flags"];
    if (field_ptr) {
        unsigned int flags = plugin_ptr->read_uint(addr + field_ptr->offset, "flags");
        copy_str(arg_list[9], decode_iomap_flags(flags));
    }
}

/**
 * @brief Handle ext4_da_write_pages_extent event
 * Format: "dev %d,%d ino %lu lblk %llu len %u flags %s"
 */
void ext4_da_write_pages_extent_event::handle(ulong addr){
    // Read dev and extract major:minor
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (field_ptr) {
        unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
        *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));
    }

    // Read ino, lblk, len
    arg_list[2]->name = "ino";
    arg_list[3]->name = "lblk";
    arg_list[4]->name = "len";
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);

    // Decode flags
    field_ptr = field_maps["flags"];
    if (field_ptr) {
        unsigned int flags = plugin_ptr->read_uint(addr + field_ptr->offset, "flags");
        copy_str(arg_list[5], decode_ext4_map_flags(flags));
    }
}

/**
 * @brief Handle ext4_request_blocks event
 * Format: "dev %d,%d ino %lu flags %s len %u lblk %llu goal %llu lleft %llu lright %llu pleft %llu pright %llu"
 */
void ext4_request_blocks_event::handle(ulong addr){
    // Read dev and extract major:minor
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (field_ptr) {
        unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
        *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));
    }

    // Read ino
    arg_list[2]->name = "ino";
    read_trace_field(addr, arg_list[2]);

    // Decode flags
    field_ptr = field_maps["flags"];
    if (field_ptr) {
        unsigned int flags = plugin_ptr->read_uint(addr + field_ptr->offset, "flags");
        copy_str(arg_list[3], decode_ext4_alloc_flags(flags));
    }

    // Read remaining fields
    arg_list[4]->name = "len";
    arg_list[5]->name = "logical";
    arg_list[6]->name = "goal";
    arg_list[7]->name = "lleft";
    arg_list[8]->name = "lright";
    arg_list[9]->name = "pleft";
    arg_list[10]->name = "pright";

    for (int i = 4; i <= 10; i++) {
        read_trace_field(addr, arg_list[i]);
    }
}

/**
 * @brief Handle ext4_mballoc_alloc event
 * Format: "dev %d,%d inode %lu orig %u/%d/%u@%u goal %u/%d/%u@%u result %u/%d/%u@%u blks %u grps %u cr %u flags %s tail %u broken %u"
 */
void ext4_mballoc_alloc_event::handle(ulong addr){
    // Read dev and extract major:minor
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (field_ptr) {
        unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
        *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));
    }

    // Read inode
    arg_list[2]->name = "ino";
    read_trace_field(addr, arg_list[2]);

    // Read orig fields (group, start, len, logical)
    arg_list[3]->name = "orig_group";
    arg_list[4]->name = "orig_start";
    arg_list[5]->name = "orig_len";
    arg_list[6]->name = "orig_logical";

    // Read goal fields
    arg_list[7]->name = "goal_group";
    arg_list[8]->name = "goal_start";
    arg_list[9]->name = "goal_len";
    arg_list[10]->name = "goal_logical";

    // Read result fields
    arg_list[11]->name = "result_group";
    arg_list[12]->name = "result_start";
    arg_list[13]->name = "result_len";
    arg_list[14]->name = "result_logical";

    // Read remaining fields
    arg_list[15]->name = "found";
    arg_list[16]->name = "groups";
    arg_list[17]->name = "cr";

    for (int i = 3; i <= 17; i++) {
        read_trace_field(addr, arg_list[i]);
    }

    // Decode flags
    field_ptr = field_maps["flags"];
    if (field_ptr) {
        unsigned int flags = plugin_ptr->read_uint(addr + field_ptr->offset, "flags");
        copy_str(arg_list[18], decode_ext4_alloc_flags(flags));
    }

    // Read tail and broken
    arg_list[19]->name = "tail";
    arg_list[20]->name = "buddy";
    read_trace_field(addr, arg_list[19]);
    read_trace_field(addr, arg_list[20]);
}

/**
 * @brief Handle ext4_ext_handle_unwritten_extents event
 * Format: "dev %d,%d ino %lu m_lblk %u m_len %u flags %s allocated %d newblock %llu"
 */
void ext4_ext_handle_unwritten_extents_event::handle(ulong addr){
    // Read dev and extract major:minor
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (field_ptr) {
        unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
        *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));
    }

    // Read ino, lblk, len
    arg_list[2]->name = "ino";
    arg_list[3]->name = "lblk";
    arg_list[4]->name = "len";
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);

    // Decode flags
    field_ptr = field_maps["flags"];
    if (field_ptr) {
        unsigned int flags = plugin_ptr->read_uint(addr + field_ptr->offset, "flags");
        copy_str(arg_list[5], decode_ext4_map_flags(flags));
    }

    // Read allocated and newblock
    arg_list[6]->name = "allocated";
    arg_list[7]->name = "newblock";
    read_trace_field(addr, arg_list[6]);
    read_trace_field(addr, arg_list[7]);
}

/**
 * @brief Handle ext4_get_implied_cluster_alloc_exit event
 * Format: "dev %d,%d m_lblk %u m_len %u m_pblk %llu m_flags %s ret %d"
 */
void ext4_get_implied_cluster_alloc_exit_event::handle(ulong addr){
    // Read dev and extract major:minor
    std::shared_ptr<trace_field> field_ptr = field_maps["dev"];
    if (field_ptr) {
        unsigned int dev = plugin_ptr->read_uint(addr + field_ptr->offset, "dev");
        *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (dev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (dev & ((1U << 20) - 1));
    }

    // Read lblk, len, pblk
    arg_list[2]->name = "lblk";
    arg_list[3]->name = "len";
    arg_list[4]->name = "pblk";
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);
    read_trace_field(addr, arg_list[4]);

    // Decode flags
    field_ptr = field_maps["flags"];
    if (field_ptr) {
        unsigned int flags = plugin_ptr->read_uint(addr + field_ptr->offset, "flags");
        copy_str(arg_list[5], decode_ext4_mflags(flags));
    }

    // Read ret
    arg_list[6]->name = "ret";
    read_trace_field(addr, arg_list[6]);
}

/**
 * @brief Handle locks_get_lock_context event
 * Format: "dev=0x%x:0x%x ino=0x%lx type=%s ctx=%p"
 */
void locks_get_lock_context_event::handle(ulong addr){
    // Read s_dev and extract major:minor
    std::shared_ptr<trace_field> field_ptr = field_maps["s_dev"];
    if (field_ptr) {
        unsigned int s_dev = plugin_ptr->read_uint(addr + field_ptr->offset, "s_dev");
        *reinterpret_cast<unsigned int*>(arg_list[0]->data) = (s_dev >> 20);
        *reinterpret_cast<unsigned int*>(arg_list[1]->data) = (s_dev & ((1U << 20) - 1));
    }

    // Read i_ino
    arg_list[2]->name = "i_ino";
    read_trace_field(addr, arg_list[2]);

    // Decode type
    field_ptr = field_maps["type"];
    if (field_ptr) {
        unsigned char type = plugin_ptr->read_byte(addr + field_ptr->offset, "type");
        copy_str(arg_list[3], decode_file_lock_type(type));
    }

    // Read ctx pointer
    arg_list[4]->name = "ctx";
    read_trace_field(addr, arg_list[4]);
}

/**
 * @brief Handle sched_update_task_ravg event
 * Format: "comm=%s pid=%d cpu=%d event=%s demand=%u sum=%u irqtime=%u"
 */
void sched_update_task_ravg_event::handle(ulong addr){
    // Read comm, pid, cpu
    arg_list[0]->name = "comm";
    arg_list[1]->name = "pid";
    arg_list[2]->name = "cpu";
    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);
    read_trace_field(addr, arg_list[2]);

    // Decode event
    std::shared_ptr<trace_field> field_ptr = field_maps["evt"];
    if (field_ptr) {
        int evt = plugin_ptr->read_int(addr + field_ptr->offset, "evt");
        copy_str(arg_list[3], get_task_event_name(evt));
    }

    // Read demand, sum, irqtime
    arg_list[4]->name = "demand";
    arg_list[5]->name = "sum";
    arg_list[6]->name = "irqtime";
    read_trace_field(addr, arg_list[4]);
    read_trace_field(addr, arg_list[5]);
    read_trace_field(addr, arg_list[6]);
}

/**
 * @brief Handle sched_update_history event
 * Format: "comm=%s pid=%d runtime=%u samples=%u evt=%s"
 */
void sched_update_history_event::handle(ulong addr){
    // Read comm, pid
    arg_list[0]->name = "comm";
    arg_list[1]->name = "pid";
    read_trace_field(addr, arg_list[0]);
    read_trace_field(addr, arg_list[1]);

    // Read runtime, samples
    arg_list[2]->name = "runtime";
    arg_list[3]->name = "samples";
    read_trace_field(addr, arg_list[2]);
    read_trace_field(addr, arg_list[3]);

    // Decode event
    std::shared_ptr<trace_field> field_ptr = field_maps["evt"];
    if (field_ptr) {
        int evt = plugin_ptr->read_int(addr + field_ptr->offset, "evt");
        copy_str(arg_list[4], get_task_event_name(evt));
    }
}

/**
 * @brief Handle sched_update_pred_demand event
 * Format: "comm=%s pid=%d runtime=%u pred_demand=%u"
 */
void sched_update_pred_demand_event::handle(ulong addr){
    // Read all fields
    arg_list[0]->name = "comm";
    arg_list[1]->name = "pid";
    arg_list[2]->name = "runtime";
    arg_list[3]->name = "pred_demand";

    for (int i = 0; i <= 3; i++) {
        read_trace_field(addr, arg_list[i]);
    }
}

/**
 * @brief Handle usb_gadget_frame_number event
 * Format: "gadget=%s frame=%u"
 */
void usb_gadget_frame_number_event::handle(ulong addr){
    // Read gadget name
    arg_list[0]->name = "name";
    read_trace_field(addr, arg_list[0]);

    // Read frame number
    arg_list[1]->name = "frame";
    read_trace_field(addr, arg_list[1]);
}

/**
 * @brief Handle module_load event
 * Format: "%s %s", __get_str(name), __print_flags(REC->taints, "", { (1UL << 0), "P" }, { (1UL << 12), "O" }, { (1UL << 1), "F" }, { (1UL << 10), "C" }, { (1UL << 13), "E" })
 */
void module_load_event::handle(ulong addr){
    // Read module name string
    arg_list[0]->name = "name";
    read_trace_field(addr, arg_list[0]);

    // Decode taints flags
    std::shared_ptr<trace_field> field_ptr = field_maps["taints"];
    if (field_ptr) {
        uint taints = plugin_ptr->read_uint(addr + field_ptr->offset, "taints");

        // Build flags string using __print_flags logic
        static const flag_map maps[] = {
            { 1UL << 0, "P" },   // Proprietary module
            { 1UL << 12, "O" },  // Out-of-tree module
            { 1UL << 1, "F" },   // Forced module
            { 1UL << 10, "C" },  // Staging driver
            { 1UL << 13, "E" },  // Unsigned module
        };

        copy_str(arg_list[1], build_flag_string(taints, maps, 5));
    }
}
#pragma GCC diagnostic pop
