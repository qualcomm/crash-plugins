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

#ifndef SWAP_DEFS_H_
#define SWAP_DEFS_H_

#include "swapinfo.h"

/**
 * @class Swap
 * @brief Command-line interface for swap information display and analysis
 *
 * This class extends Swapinfo to provide user-facing commands for:
 * - Displaying all swap devices and their usage statistics
 * - Reading and displaying page memory content from user space
 * - Integrating with ZRAM devices for compressed swap
 *
 * The class handles command-line argument parsing and output formatting
 * for swap-related debugging and analysis tasks.
 */
class Swap : public Swapinfo {
private:
    /**
     * @brief Display information about all swap devices
     *
     * Prints a formatted table showing:
     * - Swap device kernel structure addresses
     * - Total and used swap space
     * - Associated file paths
     *
     * Automatically parses swap information if not already cached.
     */
    void print_swaps();

    /**
     * @brief Display memory content of a page at specified virtual address
     *
     * Reads page data from the current task's address space and displays
     * it in hexadecimal dump format. Handles pages in physical memory,
     * swap cache, and ZRAM devices.
     *
     * @param addr Virtual address as hexadecimal string
     */
    void print_page_memory(std::string addr);

public:
    /**
     * @brief Default constructor
     *
     * Initializes the Swap command handler with a new ZRAM information object.
     */
    Swap();

    /**
     * @brief Constructor with ZRAM information pointer
     *
     * Initializes the Swap command handler with an existing ZRAM information
     * object, allowing shared access to ZRAM device data.
     *
     * @param zram Shared pointer to Zraminfo object
     */
    Swap(std::shared_ptr<Zraminfo> zram);

    /**
     * @brief Main command handler
     *
     * Processes command-line arguments and dispatches to appropriate handlers.
     * Supports options for displaying swap devices, reading page memory,
     * and enabling debug logging.
     */
    void cmd_main(void) override;

    /**
     * @brief Initialize structure field offsets
     *
     * Empty implementation as offset initialization is handled by base class.
     */
    void init_offset(void) override;

    /**
     * @brief Initialize command metadata
     *
     * Sets up command name, description, and help text for the swap
     * information command interface.
     */
    void init_command(void) override;

    DEFINE_PLUGIN_INSTANCE(Swap)
};

#endif // SWAP_DEFS_H_
