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

#include "qlog.h"
#include "logger/logger_core.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(QLog)
#endif

/**
 * @brief Main command handler for qlog plugin
 *
 * Processes command-line arguments and dispatches to appropriate display functions.
 * Supports options for displaying PMIC logs, boot logs, and XBL/SBL logs.
 */
void QLog::cmd_main(void) {
    int c;
    std::string cppString;

    // Validate minimum argument count
    if (argcnt < 2) {
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }
    // Process command-line options
    while ((c = getopt(argcnt, args, "pbx")) != EOF) {
        switch(c) {
            case 'p':
                print_pmic_info();
                break;
            case 'b':
                print_boot_log();
                break;
            case 'x':
                print_sbl_log();
                break;
            default:
                argerrs++;
                break;
        }
    }

    if (argerrs) {
        cmd_usage(pc->curcmd, SYNOPSIS);
    }
}

/**
 * @brief Initialize kernel structure field offsets
 *
 * Initializes structure offsets needed for parsing device information.
 */
void QLog::init_offset(void) {
    // Initialize kobject structure fields
    field_init(kobject, name);

    // Initialize device structure fields
    field_init(device, kobj);
    field_init(device, driver_data);

    // Initialize pmic_pon_trigger_mapping structure
    field_init(pmic_pon_trigger_mapping, code);
    field_init(pmic_pon_trigger_mapping, label);
    struct_init(pmic_pon_trigger_mapping);
}

/**
 * @brief Initialize command metadata and help information
 *
 * Sets up the command name, description, and comprehensive usage examples.
 */
void QLog::init_command(void) {
    cmd_name = "qlog";
    help_str_list = {
        "qlog",                                /* command name */
        "display Qualcomm boot and power management log information",  /* short description */
        "[-p] [-b] [-x]\n"
        "  This command analyzes Qualcomm-specific boot and power management logs\n"
        "  from crash dumps, providing detailed information about PMIC events,\n"
        "  kernel boot sequences, and bootloader (XBL/SBL) initialization logs.\n"
        "\n"
        "    -p              display PMIC power-on log with detailed event analysis\n"
        "    -b              display kernel boot log from early boot buffer\n"
        "    -x              display XBL/SBL bootloader log from UEFI region\n",
        "\n",
        "EXAMPLES",
        "  Display comprehensive PMIC power-on event log:",
        "    %s> qlog -p",
        "    Reset Trigger: PS_HOLD",
        "    Reset Type: WARM_RESET",
        "    Waiting on PS_HOLD",
        "    Warm Reset Count: 2",
        "    Waiting on PS_HOLD",
        "    Warm Reset Count: 2",
        "    PON Successful",
        "\n",
        "  Display kernel boot log with early initialization messages:",
        "    %s> qlog -b",
        "    <6>[    0.000000][    T0] Booting Linux on physical CPU 0x0000000000 [0x412fd050]",
        "    <5>[    0.000000][    T0] Linux version 6.12.23-android16-4-maybe-dirty-debug (kleaf@build-host) (Android (12833971, +pgo, +bolt, +lto, +mlgo, based on r536225) clang version 19.0.1 (https://android.googlesource.com/toolchain/llvm-project b3a530ec6537146650e42be89f1089e9a3588460), LLD 19.0.1) #1 SMP PREEMPT Thu Jan  1 00:00:00 UTC 1970",
        "    <6>[    0.000000][    T0] KASLR enabled",
        "    <5>[    0.000000][    T0] random: crng init done",
        "    <6>[    0.000000][    T0] Enabling dynamic shadow call stack",
        "\n",
        "  Display XBL/SBL bootloader log from UEFI region:",
        "    %s> qlog -x",
        "    Format: Log Type - Time(microsec) - Message - Optional Info",
        "    Log Type: B - Since Boot(Power On Reset),  D - Delta,  S - Statistic",
        "\n",
    };
}

/**
 * @brief Constructor - initializes device tree parser
 */
QLog::QLog() {
    dts = std::make_shared<Devicetree>();
}

/**
 * @brief Read PMIC PON trigger mapping tables from kernel
 *
 * Loads all PMIC power-on trigger and fault reason mappings from kernel symbols
 * into internal data structures for event interpretation.
 */
void QLog::read_pmic_pon_trigger_maps() {
    // Read PON trigger map
    size_t len = get_array_length(TO_CONST_STRING("pmic_pon_pon_trigger_map"), NULL, 0);
    for (size_t i = 0; i < len; i++) {
        ulong addr = csymbol_value("pmic_pon_pon_trigger_map") + i * struct_size(pmic_pon_trigger_mapping);
        uint id = read_ushort(addr + field_offset(pmic_pon_trigger_mapping, code), "code");
        ulong name_addr = read_pointer(addr + field_offset(pmic_pon_trigger_mapping, label), "label addr");

        if (!is_kvaddr(name_addr)) {
            continue;
        }

        std::string name = read_cstring(name_addr, 64, "name");
        pmic_pon_trigger_map[id] = name;
        LOGD("PON trigger: 0x%x -> %s", id, name.c_str());
    }

    // Read reset trigger map
    len = get_array_length(TO_CONST_STRING("pmic_pon_reset_trigger_map"), NULL, 0);

    for (size_t i = 0; i < len; i++) {
        ulong addr = csymbol_value("pmic_pon_reset_trigger_map") + i * struct_size(pmic_pon_trigger_mapping);
        uint id = read_ushort(addr + field_offset(pmic_pon_trigger_mapping, code), "code");
        ulong name_addr = read_pointer(addr + field_offset(pmic_pon_trigger_mapping, label), "label addr");
        if (!is_kvaddr(name_addr)) {
            continue;
        }
        std::string name = read_cstring(name_addr, 64, "name");
        pmic_pon_reset_trigger_map[id] = name;
        LOGD("Reset trigger: 0x%x -> %s", id, name.c_str());
    }

    // Read fault reason 1 descriptions
    len = get_array_length(TO_CONST_STRING("pmic_pon_fault_reason1"), NULL, 0);
    for (size_t i = 0; i < len; i++) {
        ulong addr = read_pointer(csymbol_value("pmic_pon_fault_reason1") + i * sizeof(void *), "fault_reason1");
        if (!is_kvaddr(addr)) {
            pmic_pon_fault_reason1.push_back("");
            continue;
        }
        std::string name = read_cstring(addr, 64, "name");
        pmic_pon_fault_reason1.push_back(name);
    }

    // Read fault reason 2 descriptions
    len = get_array_length(TO_CONST_STRING("pmic_pon_fault_reason2"), NULL, 0);
    for (size_t i = 0; i < len; i++) {
        ulong addr = read_pointer(csymbol_value("pmic_pon_fault_reason2") + i * sizeof(void *), "fault_reason2");
        if (!is_kvaddr(addr)) {
            pmic_pon_fault_reason2.push_back("");
            continue;
        }
        std::string name = read_cstring(addr, 64, "name");
        pmic_pon_fault_reason2.push_back(name);
    }

    // Read fault reason 3 descriptions
    len = get_array_length(TO_CONST_STRING("pmic_pon_fault_reason3"), NULL, 0);
    for (size_t i = 0; i < len; i++) {
        ulong addr = read_pointer(csymbol_value("pmic_pon_fault_reason3") + i * sizeof(void *), "fault_reason3");
        if (!is_kvaddr(addr)) {
            pmic_pon_fault_reason3.push_back("");
            continue;
        }
        std::string name = read_cstring(addr, 64, "name");
        pmic_pon_fault_reason3.push_back(name);
    }

    // Read S3 reset reason descriptions
    len = get_array_length(TO_CONST_STRING("pmic_pon_s3_reset_reason"), NULL, 0);
    for (size_t i = 0; i < len; i++) {
        ulong addr = read_pointer(csymbol_value("pmic_pon_s3_reset_reason") + i * sizeof(void *), "s3_reset");
        if (!is_kvaddr(addr)) {
            pmic_pon_s3_reset_reason.push_back("");
            continue;
        }
        std::string name = read_cstring(addr, 64, "name");
        pmic_pon_s3_reset_reason.push_back(name);
    }

    // Read PON PBL status descriptions
    len = get_array_length(TO_CONST_STRING("pmic_pon_pon_pbl_status"), NULL, 0);
    for (size_t i = 0; i < len; i++) {
        ulong addr = read_pointer(csymbol_value("pmic_pon_pon_pbl_status") + i * sizeof(void *), "pon_pbl");
        if (!is_kvaddr(addr)) {
            pmic_pon_pon_pbl_status.push_back("");
            continue;
        }
        std::string name = read_cstring(addr, 64, "name");
        pmic_pon_pon_pbl_status.push_back(name);
    }

    // Read reset type labels
    len = get_array_length(TO_CONST_STRING("pmic_pon_reset_type_label"), NULL, 0);
    for (size_t i = 0; i < len; i++) {
        ulong addr = read_pointer(csymbol_value("pmic_pon_reset_type_label") + i * sizeof(void *), "label");
        if (!is_kvaddr(addr)) {
            pmic_pon_reset_type_label.push_back("");
            continue;
        }
        std::string name = read_cstring(addr, 64, "name");
        pmic_pon_reset_type_label.push_back(name);
    }
}

/**
 * @brief Print PMIC fault reasons from bitmask
 * @param data Bitmask of fault reasons
 * @param reasons Vector of reason descriptions
 *
 * Decodes a fault reason bitmask and prints all active reasons.
 */
void QLog::pmic_pon_log_print_reason(uint8_t data, std::vector<std::string> reasons) {
    if (data == 0) {
        PRINT("None \n");
        return;
    }

    bool first = true;
    for (size_t i = 0; i < 8; i++) {
        if (data & (1U << i)) {
            if (i < reasons.size() && !reasons[i].empty()) {
                PRINT("%s%s \n", (first ? "" : ", "), reasons[i].c_str());
                first = false;
            }
        }
    }
}

/**
 * @brief Parse and display PMIC PON log device
 * @param addr Kernel address of pmic_pon_log_dev structure
 *
 * Reads and interprets all PMIC power-on log events, displaying detailed
 * information about the boot sequence and any faults that occurred.
 */
void QLog::parser_pmic_pon_log_dev(ulong addr) {
    bool is_important;
    // Initialize pmic_pon_log_dev structure fields
    field_init(pmic_pon_log_dev, log);
    field_init(pmic_pon_log_dev, log_len);

    if (field_offset(pmic_pon_log_dev, log) == -1) {
        LOGE("pmic_pon_log_dev structure not found - kernel module not loaded");
        return;
    }

    // Read trigger mapping tables
    read_pmic_pon_trigger_maps();

    // Read log entry array and length
    ulong entry_addr = read_pointer(addr + field_offset(pmic_pon_log_dev, log), "log_entry");
    size_t log_len = read_int(addr + field_offset(pmic_pon_log_dev, log_len), "log_len");
    // Process each log entry
    for (size_t i = 0; i < log_len; i++) {
        struct pmic_event_t entry;
        ulong addr = entry_addr + i * sizeof(pmic_event_t);
        if (read_struct(addr, &entry, sizeof(entry), "pmic_event_t")) {
            uint16_t data = (entry.data1 << 8) | entry.data0;
            LOGD("Event %zu: type=0x%02x, data=0x%04x", i, entry.event, data);
            // Interpret event based on type
            switch (entry.event) {
                case PMIC_PON_EVENT_PON_TRIGGER_RECEIVED:
                    if (pmic_pon_trigger_map.find(data) != pmic_pon_trigger_map.end()) {
                        PRINT("%s \n", pmic_pon_trigger_map[data].c_str());
                    } else {
                        PRINT("SID=0x%X, PID=0x%02X, IRQ=0x%X \n", entry.data1 >> 4, (data >> 4) & 0xFF,
                                entry.data0 & 0x7);
                    }
                    break;

                case PMIC_PON_EVENT_OTP_COPY_COMPLETE:
                    PRINT("OTP Copy Complete: last addr written=0x%04X \n", data);
                    break;

                case PMIC_PON_EVENT_TRIM_COMPLETE:
                    PRINT("Trim Complete: %u bytes written \n", data);
                    break;

                case PMIC_PON_EVENT_XVLO_CHECK_COMPLETE:
                    PRINT("XVLO Check Complete \n");
                    break;

                case PMIC_PON_EVENT_PMIC_CHECK_COMPLETE:
                    PRINT("PMICs Detected: SID Mask=0x%04X \n", data);
                    break;

                case PMIC_PON_EVENT_RESET_TRIGGER_RECEIVED:
                    if (pmic_pon_reset_trigger_map.find(data) != pmic_pon_reset_trigger_map.end()) {
                        PRINT("Reset Trigger: %s \n", pmic_pon_reset_trigger_map[data].c_str());
                    } else {
                        PRINT("SID=0x%X, PID=0x%02X, IRQ=0x%X \n", entry.data1 >> 4, (data >> 4) & 0xFF,
                                entry.data0 & 0x7);
                    }
                    break;

                case PMIC_PON_EVENT_RESET_TYPE:
                    if (entry.data0 < pmic_pon_reset_type_label.size() && !pmic_pon_reset_type_label[entry.data0].empty()) {
                        PRINT("Reset Type: %s \n", pmic_pon_reset_type_label[entry.data0].c_str());
                    } else {
                        PRINT("Reset Type: UNKNOWN (%u) \n", entry.data0);
                    }
                    break;

                case PMIC_PON_EVENT_WARM_RESET_COUNT:
                    PRINT("Warm Reset Count: %u \n", data);
                    break;

                case PMIC_PON_EVENT_FAULT_REASON_1_2:
                    if (!entry.data0 && !entry.data1) {
                        is_important = false;
                    }
                    if (entry.data0 || !is_important) {
                        PRINT("FAULT_REASON1=");
                        pmic_pon_log_print_reason(entry.data0, pmic_pon_fault_reason1);
                    }
                    if (entry.data1 || !is_important) {
                        PRINT("%sFAULT_REASON2=", (entry.data0 || !is_important) ? "; " : "");
                        pmic_pon_log_print_reason(entry.data1, pmic_pon_fault_reason2);
                    }
                    break;

                case PMIC_PON_EVENT_FAULT_REASON_3:
                    if (!entry.data0) {
                        is_important = false;
                    }
                    PRINT("FAULT_REASON3=");
                    pmic_pon_log_print_reason(entry.data0, pmic_pon_fault_reason3);
                    break;

                case PMIC_PON_EVENT_PBS_PC_DURING_FAULT:
                    PRINT("PBS PC at Fault: 0x%04X \n", data);
                    break;

                case PMIC_PON_EVENT_FUNDAMENTAL_RESET:
                    if (!entry.data0 && !entry.data1) {
                        is_important = false;
                    }
                    PRINT("Fundamental Reset: ");
                    if (entry.data1 || !is_important) {
                        PRINT("PON_PBL_STATUS=");
                        pmic_pon_log_print_reason(entry.data1, pmic_pon_pon_pbl_status);
                    }
                    if (entry.data0 || !is_important) {
                        PRINT("%sS3_RESET_REASON=", (entry.data1 || !is_important) ? "; " : "");
                        pmic_pon_log_print_reason(entry.data0, pmic_pon_s3_reset_reason);
                    }
                    break;

                case PMIC_PON_EVENT_PON_SEQ_START:
                    PRINT("Begin PON Sequence \n");
                    break;

                case PMIC_PON_EVENT_PON_SUCCESS:
                    PRINT("PON Successful \n");
                    break;

                case PMIC_PON_EVENT_WAITING_ON_PSHOLD:
                    PRINT("Waiting on PS_HOLD \n");
                    break;

                case PMIC_PON_EVENT_PMIC_SID1_FAULT ... PMIC_PON_EVENT_PMIC_SID13_FAULT:
                    if (!entry.data0 && !entry.data1) {
                        is_important = false;
                    }
                    PRINT("PMIC SID%u ", entry.event - PMIC_PON_EVENT_PMIC_SID1_FAULT + 1);
                    if (entry.data0 || !is_important) {
                        PRINT("FAULT_REASON1=");
                        pmic_pon_log_print_reason(entry.data0, pmic_pon_fault_reason1);
                    }
                    if (entry.data1 || !is_important) {
                        PRINT("%sFAULT_REASON2=", (entry.data0 || !is_important) ? "; " : "");
                        pmic_pon_log_print_reason(entry.data1, pmic_pon_fault_reason2);
                    }
                    break;

                case PMIC_PON_EVENT_PMIC_VREG_READY_CHECK:
                    if (!data) {
                        is_important = false;
                    }
                    PRINT("VREG Check: %sVREG_FAULT detected \n", data ? "" : "No ");
                    break;

                default:
                    LOGD("Unknown event type: 0x%02x", entry.event);
                    PRINT("Unknown Event (0x%02X): data=0x%04X \n", entry.event, data);
                    break;
            }
        }
    }

    LOGD("Finished processing PMIC PON log entries");
}

/**
 * @brief Print XBL/SBL log from device tree reserved memory
 *
 * Extracts and displays the bootloader log from the UEFI log region
 * defined in the device tree.
 */
void QLog::print_sbl_log() {
    LOGD("Searching for UEFI log region in device tree");
    // Try to find uefi_log_region node
    auto nodes = dts->find_node_by_name("uefi_log_region");
    bool is_merged_region = false;

    if (nodes.empty()) {
        LOGD("uefi_log_region not found, trying aop_tme_uefi_merged_region");
        nodes = dts->find_node_by_name("aop_tme_uefi_merged_region");

        if (nodes.empty()) {
            LOGE("Cannot find UEFI log region in device tree");
            return;
        }
        is_merged_region = true;
    }
    LOGD("Found UEFI log region node at 0x%lx", nodes[0]->addr);

    // Get reg property
    auto prop = dts->getprop(nodes[0]->addr, "reg");
    if (!prop) {
        LOGE("Failed to get reg property from UEFI log node");
        return;
    }
    // Parse reg property
    const char* ptr = reinterpret_cast<const char*>(prop->value.data());
    size_t reg_cnt = prop->length / sizeof(uint32_t);
    if (reg_cnt < 4) {
        LOGE("Invalid reg property length: %zu", reg_cnt);
        return;
    }

    std::vector<uint32_t> regs(reg_cnt);
    for (size_t i = 0; i < reg_cnt; ++i) {
        regs[i] = ntohl(UINT(ptr + i * sizeof(uint32_t)));
    }

    // Extract base address and size
    ulong base_addr = (static_cast<uint64_t>(regs[0]) << 32) | regs[1];
    ulong size = (static_cast<uint64_t>(regs[2]) << 32) | regs[3];

    // Adjust for merged region
    if (is_merged_region) {
        LOGD("Using merged region, adjusting offset");
        base_addr += 0x64000;
        size = 0x10000;
    }
    LOGD("Reading UEFI log from 0x%lx, size=%lu bytes", base_addr, size);

    // Read and display log
    std::string logstring = read_cstring(base_addr, size, "uefi_log_region", false);
    PRINT("%s", logstring.c_str());

    LOGD("UEFI log displayed successfully");
}

/**
 * @brief Print kernel boot log
 *
 * Displays the early kernel boot log buffer if available. This log contains
 * messages from the kernel's early boot phase before the regular log buffer
 * is initialized.
 */
void QLog::print_boot_log() {
    LOGD("Attempting to read kernel boot log");
    // Check if boot_log_buf symbol exists
    if (!csymbol_exists("boot_log_buf")) {
        LOGE("boot_log_buf symbol not found - kernel module not loaded");
        return;
    }

    // Read boot log buffer address
    ulong logbuf_addr = read_pointer(csymbol_value("boot_log_buf"), "boot_log_buf");
    if (!is_kvaddr(logbuf_addr)) {
        LOGE("Invalid boot log buffer address: 0x%lx", logbuf_addr);
        return;
    }
    LOGD("Boot log buffer at 0x%lx", logbuf_addr);
    // Determine log buffer size
    ulong logbuf_size = 0;
    if (csymbol_exists("boot_log_buf_size")) {
        logbuf_size = read_uint(csymbol_value("boot_log_buf_size"), "boot_log_buf_size");
        LOGD("Boot log size from boot_log_buf_size: %lu", logbuf_size);
    }
    if (logbuf_size == 0) {
        // Calculate size from position and remaining space
        ulong logbuf_pos = read_pointer(csymbol_value("boot_log_pos"), "boot_log_pos");
        ulong logbuf_left = read_uint(csymbol_value("boot_log_buf_left"), "boot_log_buf_left");

        if (logbuf_pos != 0 && logbuf_left != 0) {
            logbuf_size = (logbuf_pos - logbuf_addr) + logbuf_left;
            LOGD("Calculated boot log size: %lu (pos=0x%lx, left=%lu)", logbuf_size, logbuf_pos, logbuf_left);
        } else {
            logbuf_size = 524288;  // Default 512KB
            LOGD("Using default boot log size: %lu", logbuf_size);
        }
    }

    // Read boot log buffer
    void *buf = read_memory(logbuf_addr, logbuf_size, "boot_log");
    if (buf != nullptr) {
        std::string boot_log(static_cast<const char*>(buf), logbuf_size);
        std::string msg = remove_invalid_chars(boot_log);

        LOGD("Displaying %zu characters of cleaned boot log", msg.size());
        PRINT("%s \n", msg.c_str());
        FREEBUF(buf);
    } else {
        LOGE("Failed to read boot log buffer");
    }
}

/**
 * @brief Remove invalid characters from log string
 * @param msg Input log message
 * @return Cleaned log message with only printable characters
 *
 * Filters out non-printable characters while preserving newlines.
 * Returns empty string if no printable content is found.
 */
std::string QLog::remove_invalid_chars(const std::string& msg) {
    std::string vaildStr;
    bool hasPrintable = false;

    // Filter characters
    for (unsigned char c : msg) {
        if (c == '\n') {
            vaildStr += '\n';
        } else if (c >= 0x20 && c <= 0x7E) {
            vaildStr += c;
            if (c != ' ' && c != '\t') {
                hasPrintable = true;
            }
        }
    }

    // Check if result has any printable content
    if (!hasPrintable || std::all_of(vaildStr.begin(), vaildStr.end(), [](unsigned char c) {
        return std::isspace(c);
    })) {
        return "";
    }
    return vaildStr;
}

/**
 * @brief Print PMIC power-on information
 *
 * Finds the PMIC PON log device on the platform bus and displays all
 * power-on events and fault information.
 */
void QLog::print_pmic_info() {
    LOGD("Searching for PMIC PON log device");
    ulong device_addr = 0;

    // Search for pmic-pon-log device on platform bus
    for (const auto& dev_ptr : for_each_device_for_bus("platform")) {
        if (dev_ptr->name.empty() || dev_ptr->name.find("pmic-pon-log") == std::string::npos) {
            continue;
        }
        LOGD("Found PMIC PON log device: %s at 0x%lx", dev_ptr->name.c_str(), dev_ptr->addr);
        if (!is_kvaddr(dev_ptr->driver_data)) {
            LOGE("Invalid driver_data address for device %s", dev_ptr->name.c_str());
            continue;
        }
        device_addr = dev_ptr->addr;
        break;
    }

    // Parse and display PMIC PON log
    if (is_kvaddr(device_addr)) {
        LOGD("Parsing PMIC PON log device at 0x%lx", device_addr);
        parser_pmic_pon_log_dev(device_addr);
    } else {
        LOGE("PMIC PON log device not found on platform bus");
    }
}

#pragma GCC diagnostic pop
