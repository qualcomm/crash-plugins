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

#ifndef ZRAM_DEFS_H_
#define ZRAM_DEFS_H_

#include "memory/zraminfo.h"

/**
 * @class Zram
 * @brief Command-line interface for ZRAM debugging and analysis
 *
 * This class extends Zraminfo to provide user-facing commands for
 * inspecting ZRAM devices, memory pools, pages, and objects.
 */
class Zram : public Zraminfo {
private:
    // Print flags for object display
    static const int PRINT_SIZE_CLASS = 0x0001;  ///< Flag to print size class objects
    static const int PRINT_ZSPAGE = 0x0002;      ///< Flag to print zspage objects
    static const int PRINT_PAGE = 0x0004;        ///< Flag to print page objects

    /**
     * @brief Print summary information for all ZRAM devices
     */
    void print_zrams();

    /**
     * @brief Print detailed information for a specific ZRAM device
     * @param zram_addr Address of the ZRAM device (hex string)
     */
    void print_zram_full_info(std::string zram_addr);

    /**
     * @brief Print memory pool information for a ZRAM device
     * @param zram_addr Address of the ZRAM device (hex string)
     */
    void print_mem_pool(std::string zram_addr);

    /**
     * @brief Print statistics for a ZRAM device
     * @param zram_addr Address of the ZRAM device (hex string)
     */
    void print_zram_stat(std::string zram_addr);

    /**
     * @brief Print all pages for a ZRAM device
     * @param zram_addr Address of the ZRAM device (hex string)
     */
    void print_pages(std::string zram_addr);

    /**
     * @brief Print all zspages for a ZRAM device
     * @param zram_addr Address of the ZRAM device (hex string)
     */
    void print_zspages(std::string zram_addr);

    /**
     * @brief Print objects by size_class/zspage/page address
     * @param addr Address to query (hex string)
     */
    void print_objs(std::string addr);

    /**
     * @brief Print all objects in a size class
     * @param class_ptr Pointer to size_class structure
     */
    void print_size_class_obj(std::shared_ptr<size_class> class_ptr);

    /**
     * @brief Print all objects in a zspage
     * @param zspage_ptr Pointer to zpage structure
     */
    void print_zspage_obj(std::shared_ptr<zpage> zspage_ptr);

    /**
     * @brief Print all objects in a page
     * @param page_ptr Pointer to pageinfo structure
     */
    void print_page_obj(std::shared_ptr<pageinfo> page_ptr);

public:
    /**
     * @brief Constructor - initializes ZRAM command interface
     */
    Zram();

    /**
     * @brief Main command handler - processes user commands
     */
    void cmd_main(void) override;

    /**
     * @brief Initialize structure offsets (empty for this class)
     */
    void init_offset(void) override;

    /**
     * @brief Initialize command help and usage information
     */
    void init_command(void) override;

    DEFINE_PLUGIN_INSTANCE(Zram)
};

#endif // ZRAM_DEFS_H_
