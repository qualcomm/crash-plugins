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

#include "swap.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(Swap)
#endif

/**
 * @brief Main command handler for swap information commands
 *
 * Processes command-line arguments and dispatches to appropriate handlers:
 */
void Swap::cmd_main(void) {
    int c;
    std::string cppString;

    // Validate minimum argument count
    if (argcnt < 2) {
        LOGD("Insufficient arguments, displaying usage\n");
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }
    // Parse command-line options
    while ((c = getopt(argcnt, args, "ap:")) != EOF) {
        switch(c) {
            case 'a':
                print_swaps();
                break;
            case 'p':
                cppString.assign(optarg);
                print_page_memory(cppString);
                break;
            default:
                argerrs++;
                break;
        }
    }
    // Display usage if there were argument errors
    if (argerrs) {
        LOGD("Argument errors detected, displaying usage\n");
        cmd_usage(pc->curcmd, SYNOPSIS);
    }
}

/**
 * @brief Constructor with ZRAM information pointer
 *
 * Initializes the Swap command handler with a shared ZRAM information object.
 * This allows integration with ZRAM devices for swap operations.
 *
 * @param zram Shared pointer to Zraminfo object for ZRAM device access
 */
Swap::Swap(std::shared_ptr<Zraminfo> zram) : Swapinfo(zram) {
}

/**
 * @brief Default constructor
 *
 * Initializes the Swap command handler with a new ZRAM information object.
 * Creates a default Zraminfo instance for ZRAM device management.
 */
Swap::Swap() : Swapinfo(std::make_shared<Zraminfo>()) {

}

/**
 * @brief Initialize structure field offsets (empty implementation)
 *
 * This function is overridden from the base class but has no implementation
 * as offset initialization is handled by the Swapinfo base class.
 */
void Swap::init_offset(void) {

}

/**
 * @brief Initialize command metadata and help information
 *
 * Sets up the command name, description, and detailed help text including:
 *
 * This information is displayed when the user requests help or provides
 * invalid arguments.
 */
void Swap::init_command(void) {
    cmd_name = "swapinfo";
    help_str_list = {
        "swapinfo",                                /* command name */
        "display swap device information and memory analysis",  /* short description */
        "[-a] [-p vaddr]\n"
        "  This command analyzes swap devices and provides detailed information\n"
        "  about swap space usage, device configuration, and memory content.\n"
        "  It supports both ZRAM and traditional swap devices.\n"
        "\n"
        "    -a              display all swap devices with usage statistics\n"
        "    -p vaddr        display page memory content at specified virtual address\n",
        "\n",
        "EXAMPLES",
        "  Display all swap devices with detailed information:",
        "    %s> swapinfo -a",
        "       swap_info_struct   size       used       address_space      file",
        "       ========================================================================",
        "       ffffff804d3d1800   1.50Gb     292.25Mb   ffffff8026a40000   /dev/block/zram0",
        "       ========================================================================",
        "\n",
        "  Display page memory content at virtual address:",
        "    %s> swapinfo -p 12c000e0",
        "       12c00000: 153F 7F98 0000 0000 7051 4C80 0000 0000 .?......pQL.....",
        "       12c00010: 0000 0000 0000 0000 0000 0000 12C0 0020 ...............",
        "       12c00020: 7098 2630 0000 0000 706C 18B8 0000 0000 p.&0....pl......",
        "       12c00030: 0000 0000 706C 3798 0000 0000 12C0 0020 ....pl7........",
        "       12c00040: 13B5 2680 12D4 01D8 0000 0000 0000 0000 ..&.............",
        "\n",
        "  Enable debug log:",
        "    %s> swapinfo -d",
        "\n",
    };
}

/**
 * @brief Print memory content of a page at specified virtual address
 *
 * Reads and displays the memory content of a page containing the specified
 * virtual address. The page data is read from the current task context and
 * displayed in hexadecimal dump format.
 *
 * @param addr Virtual address as hexadecimal string (e.g., "12c000e0")
 *
 */
void Swap::print_page_memory(std::string addr) {
    // Get current task context
    struct task_context *tc = CURRENT_CONTEXT();
    if (!tc) {
        LOGD("please set current task context by command set <pid>\n");
        return;
    }
    LOGD("Current task: pid=%lu, comm=%s\n", tc->pid, tc->comm);
    // Convert address string to numeric value
    ulong uaddr = std::stoul(addr, nullptr, 16);
    LOGD("print memory for virtual address: 0x%lx\n", uaddr);

    // Read page data from user space
    std::vector<char> page_data = uread_memory(tc->task, uaddr, page_size, "read page");
    if (page_data.size() == 0) {
        PRINT("Failed to read page at address 0x%lx (not mapped)\n", uaddr);
        return;
    } else {
        PRINT("\n%s \n",hexdump(uaddr, page_data.data(), page_size).c_str());
    }
}

/**
 * @brief Display information about all swap devices
 *
 * Prints a formatted table showing all swap devices in the system with:
 * - swap_info_struct address: Kernel structure address
 * - size: Total swap device size
 * - used: Currently used swap space
 * - address_space: Address space structure pointer
 * - file: Swap device file path (e.g., /dev/block/zram0)
 *
 */
void Swap::print_swaps() {
    // Read number of swap files from kernel
    nr_swap = read_int(csymbol_value("nr_swapfiles"), "nr_swapfiles");
    LOGD("Number of swap files: %d\n", nr_swap);
    // Parse swap information if not already cached
    if (swap_list.size() == 0 && nr_swap > 0) {
        parser_swap_info();
    }
    LOGD("Swap list contains %zu entries\n", swap_list.size());
    // Build formatted output
    std::ostringstream oss;
    PRINT("========================================================================\n");
    // Print table header
    oss << std::left << std::setw(VADDR_PRLEN + 2) << "swap_info_struct" << " "
        << std::left << std::setw(10) << "size" << " "
        << std::left << std::setw(10) << "used" << " "
        << std::left << std::setw(VADDR_PRLEN + 2) << "address_space" << " "
        << "file" << "\n";
    // Print each swap device
    for (const auto& swap_ptr : swap_list) {
        oss << std::left << std::setw(VADDR_PRLEN + 2) << std::hex << swap_ptr->addr << " "
            << std::left << std::setw(10) << csize(swap_ptr->pages * page_size) << " "
            << std::left << std::setw(10) << csize(swap_ptr->inuse_pages * page_size) << " "
            << std::left << std::setw(VADDR_PRLEN + 2) << std::hex << swap_ptr->swap_space << " "
            << swap_ptr->swap_file;
    }
    PRINT("%s \n", oss.str().c_str());
    PRINT("========================================================================\n");
}

#pragma GCC diagnostic pop
