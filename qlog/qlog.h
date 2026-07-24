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

#ifndef BOOT_DEFS_H_
#define BOOT_DEFS_H_

#include "plugin.h"
#include "devicetree/devicetree.h"

/**
 * @struct pmic_event_t
 * @brief PMIC power-on event structure
 *
 * Represents a single PMIC power-on log event with state, event type,
 * and associated data fields.
 */
struct pmic_event_t {
    uint8_t state;      // Event state
    uint8_t event;      // Event type (see pmic_pon_event enum)
    uint8_t data1;      // High byte of event data
    uint8_t data0;      // Low byte of event data
};

/**
 * @enum pmic_pon_event
 * @brief PMIC power-on event types
 *
 * Defines all possible PMIC power-on event types that can be logged
 * during the boot sequence.
 */
enum pmic_pon_event {
    PMIC_PON_EVENT_PON_TRIGGER_RECEIVED    = 0x01,
    PMIC_PON_EVENT_OTP_COPY_COMPLETE       = 0x02,
    PMIC_PON_EVENT_TRIM_COMPLETE           = 0x03,
    PMIC_PON_EVENT_XVLO_CHECK_COMPLETE     = 0x04,
    PMIC_PON_EVENT_PMIC_CHECK_COMPLETE     = 0x05,
    PMIC_PON_EVENT_RESET_TRIGGER_RECEIVED  = 0x06,
    PMIC_PON_EVENT_RESET_TYPE              = 0x07,
    PMIC_PON_EVENT_WARM_RESET_COUNT        = 0x08,
    PMIC_PON_EVENT_FAULT_REASON_1_2        = 0x09,
    PMIC_PON_EVENT_FAULT_REASON_3          = 0x0A,
    PMIC_PON_EVENT_PBS_PC_DURING_FAULT     = 0x0B,
    PMIC_PON_EVENT_FUNDAMENTAL_RESET       = 0x0C,
    PMIC_PON_EVENT_PON_SEQ_START           = 0x0D,
    PMIC_PON_EVENT_PON_SUCCESS             = 0x0E,
    PMIC_PON_EVENT_WAITING_ON_PSHOLD       = 0x0F,
    PMIC_PON_EVENT_PMIC_SID1_FAULT         = 0x10,
    PMIC_PON_EVENT_PMIC_SID2_FAULT         = 0x11,
    PMIC_PON_EVENT_PMIC_SID3_FAULT         = 0x12,
    PMIC_PON_EVENT_PMIC_SID4_FAULT         = 0x13,
    PMIC_PON_EVENT_PMIC_SID5_FAULT         = 0x14,
    PMIC_PON_EVENT_PMIC_SID6_FAULT         = 0x15,
    PMIC_PON_EVENT_PMIC_SID7_FAULT         = 0x16,
    PMIC_PON_EVENT_PMIC_SID8_FAULT         = 0x17,
    PMIC_PON_EVENT_PMIC_SID9_FAULT         = 0x18,
    PMIC_PON_EVENT_PMIC_SID10_FAULT        = 0x19,
    PMIC_PON_EVENT_PMIC_SID11_FAULT        = 0x1A,
    PMIC_PON_EVENT_PMIC_SID12_FAULT        = 0x1B,
    PMIC_PON_EVENT_PMIC_SID13_FAULT        = 0x1C,
    PMIC_PON_EVENT_PMIC_VREG_READY_CHECK   = 0x20,
};

/**
 * @enum pmic_pon_reset_type
 * @brief PMIC reset types
 *
 * Defines the different types of resets that can occur.
 */
enum pmic_pon_reset_type {
    PMIC_PON_RESET_TYPE_WARM_RESET        = 0x1,
    PMIC_PON_RESET_TYPE_SHUTDOWN          = 0x4,
    PMIC_PON_RESET_TYPE_HARD_RESET        = 0x7,
};

/**
 * @class QLog
 * @brief Plugin for analyzing Qualcomm boot and PMIC logs
 *
 * Provides commands to display PMIC power-on logs, kernel boot logs,
 * and XBL/SBL logs from device tree reserved memory regions.
 */
class QLog : public ParserPlugin {
private:
    std::unordered_map<uint32_t, std::string> pmic_pon_trigger_map;        // PON trigger code to name mapping
    std::unordered_map<uint32_t, std::string> pmic_pon_reset_trigger_map;  // Reset trigger code to name mapping
    std::vector<std::string> pmic_pon_fault_reason1;                        // Fault reason 1 descriptions
    std::vector<std::string> pmic_pon_fault_reason2;                        // Fault reason 2 descriptions
    std::vector<std::string> pmic_pon_fault_reason3;                        // Fault reason 3 descriptions
    std::vector<std::string> pmic_pon_s3_reset_reason;                      // S3 reset reason descriptions
    std::vector<std::string> pmic_pon_pon_pbl_status;                       // PBL status descriptions
    std::vector<std::string> pmic_pon_reset_type_label;                     // Reset type labels
    std::shared_ptr<Devicetree> dts;                                        // Device tree parser instance

    /**
     * @brief Read PMIC PON trigger mapping tables from kernel
     *
     * Loads all PMIC power-on trigger and fault reason mappings from
     * kernel symbols into internal data structures.
     */
    void read_pmic_pon_trigger_maps();

    /**
     * @brief Print PMIC fault reasons from bitmask
     * @param data Bitmask of fault reasons
     * @param reasons Vector of reason descriptions
     *
     * Decodes a fault reason bitmask and prints all active reasons.
     */
    void pmic_pon_log_print_reason(uint8_t data, std::vector<std::string> reasons);

    /**
     * @brief Parse and display PMIC PON log device
     * @param addr Kernel address of pmic_pon_log_dev structure
     *
     * Reads and interprets all PMIC power-on log events.
     */
    void parser_pmic_pon_log_dev(ulong addr);

    /**
     * @brief Print XBL/SBL log from device tree reserved memory
     *
     * Extracts and displays the bootloader log from the UEFI log region
     * defined in the device tree.
     */
    void print_sbl_log();

    /**
     * @brief Print PMIC power-on information
     *
     * Finds the PMIC PON log device and displays all power-on events.
     */
    void print_pmic_info();

    /**
     * @brief Print kernel boot log
     *
     * Displays the early kernel boot log buffer if available.
     */
    void print_boot_log();

    /**
     * @brief Remove invalid characters from log string
     * @param msg Input log message
     * @return Cleaned log message with only printable characters
     *
     * Filters out non-printable characters while preserving newlines.
     */
    std::string remove_invalid_chars(const std::string& msg);

public:
    /**
     * @brief Constructor - initializes device tree parser
     */
    QLog();

    /**
     * @brief Main command handler
     *
     * Processes command-line arguments and dispatches to appropriate functions.
     */
    void cmd_main(void) override;

    /**
     * @brief Initialize structure field offsets
     *
     * Initializes kernel structure offsets needed for parsing device information.
     */
    void init_offset(void) override;

    /**
     * @brief Initialize command metadata
     *
     * Sets up command name, help text, and usage examples.
     */
    void init_command(void) override;

    DEFINE_PLUGIN_INSTANCE(QLog)
};

#endif // BOOT_DEFS_H_
