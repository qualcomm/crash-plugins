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

#include "dd.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(DDriver)
#endif

/**
 * Initialize field offsets for kernel structures
 */
void DDriver::init_offset(void) {
    // of_device_id: Device tree matching structure
    field_init(of_device_id, compatible);       // Compatible string

    // ========================================================================
    // Character Device Structures
    // ========================================================================
    field_init(char_device_struct, major);      // Major number
    field_init(char_device_struct, baseminor);  // Base minor number
    field_init(char_device_struct, minorct);    // Minor count
    field_init(char_device_struct, cdev);       // Character device structure
    field_init(char_device_struct, name);       // Device name

    // miscdevice: Miscellaneous character device (major 10)
    field_init(miscdevice, minor);              // Minor number
    field_init(miscdevice, name);               // Device name
    field_init(miscdevice, fops);               // File operations

    // ========================================================================
    // Block Device and Disk Structures
    // ========================================================================
    struct_init(gendisk);                       // Initialize structure size
    field_init(gendisk, major);                 // Major number
    field_init(gendisk, minors);                // Number of minors
    field_init(gendisk, disk_name);             // Disk name (e.g., "sda")
    field_init(gendisk, part_tbl);              // Partition table

    // disk_part_tbl: Disk partition table (legacy)
    field_init(disk_part_tbl, len);             // Number of partitions
    field_init(disk_part_tbl, part);            // Partition array

    // block_device: Block device structure
    struct_init(block_device);                  // Initialize structure size
    field_init(block_device, bd_start_sect);    // Start sector
    field_init(block_device, bd_nr_sectors);    // Number of sectors
    field_init(block_device, bd_disk);          // Associated gendisk
    field_init(block_device, bd_mapping);       // Address space mapping
    field_init(block_device, bd_meta_info);     // Partition metadata
    field_init(block_device, bd_super);         // Superblock pointer
    field_init(block_device, bd_part);          // Partition (hd_struct)
    field_init(block_device, bd_block_size);    // Block size
    field_init(block_device, bd_partno);        // Partition number

    // ========================================================================
    // Filesystem and Superblock Structures
    // ========================================================================
    // super_block: Filesystem superblock
    field_init(super_block, s_blocksize);       // Block size
    field_init(super_block, s_type);            // Filesystem type
    field_init(super_block, s_list);            // List of superblocks
    field_init(super_block, s_bdev);            // Block device

    // file_system_type: Filesystem type descriptor
    field_init(file_system_type, name);         // Filesystem name (e.g., "ext4")
    field_init(file_system_type, fs_flags);     // Filesystem flags

    // ========================================================================
    // Partition Structures (Legacy and Modern)
    // ========================================================================
    // hd_struct: Legacy partition structure (pre-5.11 kernels)
    struct_init(hd_struct);                     // Initialize structure size
    field_init(hd_struct, info);                // Partition metadata
    field_init(hd_struct, __dev);               // Device structure
    field_init(hd_struct, start_sect);          // Start sector
    field_init(hd_struct, nr_sects);            // Number of sectors
    field_init(hd_struct, partno);              // Partition number

    // partition_meta_info: Partition metadata (UUID, volume name)
    field_init(partition_meta_info, uuid);      // Partition UUID
    field_init(partition_meta_info, volname);   // Volume name
}

/**
 * Main command entry point
 * Parses command line arguments and dispatches to appropriate handler functions
 */
void DDriver::cmd_main(void) {
    int c;
    std::string cppString;
    // Check if at least one argument is provided
    if (argcnt < 2) {
        LOGE("Insufficient arguments provided (count: %d)\n", argcnt);
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }
    while ((c = getopt(argcnt, args, "bB:cC:lLs:amDdp:")) != EOF) {
        switch(c) {
            case 'b': //list all bus
                print_bus_info();
                break;
            case 'B'://list all device under specified bus
                cppString.assign(optarg);
                print_device_driver_for_bus(cppString);
                break;
            case 'c': //list all class
                print_class_info();
                break;
            case 'C'://list all device under specified class
                cppString.assign(optarg);
                print_device_driver_for_class(cppString);
                break;
            case 'l'://list all device
                print_device_list();
                break;
            case 'L'://list all driver
                print_driver_list();
                break;
            case 's'://list all device under specified driver
                cppString.assign(optarg);
                print_device_list_for_driver(cppString);
                break;
            case 'a'://list all char_device_struct
                print_char_device();
                break;
            case 'm'://list all misc device
                print_misc_device();
                break;
            case 'D'://list all block device
                print_block_device();
                break;
            case 'd'://list all disk
                print_gendisk();
                break;
            case 'p'://list all partition under specified disk
                cppString.assign(optarg);
                print_partition(cppString);
                break;
            default:
                LOGE("Unknown option: %c\n", c);
                argerrs++;
                break;
        }
    }
    if (argerrs) {
        LOGE("Command parsing failed with %d errors\n", argerrs);
        cmd_usage(pc->curcmd, SYNOPSIS);
    }
}

void DDriver::init_command(void) {
    cmd_name = "dd";
    help_str_list={
        "dd",                                     /* command name */
        "display device driver information",      /* short description */
        "[-b] [-B bus_name] [-c] [-C class_name] [-l] [-L] [-s driver_name] [-a] [-m] [-D] [-d] [-p disk_name]\n"
        "  This command displays device driver information.\n"
        "\n"
        "    -b              display all bus info\n"
        "    -B bus_name     display all device and driver info for specified bus\n"
        "    -c              display all class info\n"
        "    -C class_name   display all device and driver info for specified class\n"
        "    -l              display all device info\n"
        "    -L              display all driver info\n"
        "    -s driver_name  display all device for specified driver\n"
        "    -a              display all char device\n"
        "    -m              display all misc device\n"
        "    -D              display all block device\n"
        "    -d              display all disk\n"
        "    -p disk_name    display all partition for specified disk\n",
        "\n",
        "EXAMPLES",
        "  Display all bus info:",
        "    %s> dd -b",
        "    name         bus_type           subsys_private     probe func            dev_cnt drv_cnt",
        "    platform     0xc209bf98         0xf6d5a000         platform_probe+0      45      12",
        "    cpu          0xc209c060         0xf6d5b000                               8       2",
        "    pci          0xc205f608         0xf645b200         pci_device_probe+0    15      8",
        "\n",
        "  Display all device and driver info for specified bus:",
        "    %s> dd -B platform",
        "",
        "    ================================================================================",
        "      Bus: platform       Devices: 45  (Bound: 38 , Unbound: 7  )  Drivers: 12",
        "    ================================================================================",
        "",
        "    [Devices]",
        "    --------------------------------------------------------------------------------",
        "    name                 device               driver_name          device_driver",
        "    --------------------------------------------------------------------------------",
        "    reg-dummy            0xf6410810           reg-dummy            0xc2074488",
        "    f120000.timer        0xf64a3410",
        "    soc:cpu-pmu          0xf64a1010           armv7-pmu            0xc2018658",
        "",
        "    [Drivers]",
        "    --------------------------------------------------------------------------------",
        "    name                 device_driver        compatible              probe_func",
        "    --------------------------------------------------------------------------------",
        "    syscon-reboot        0xc20b7aa0           syscon-reboot           platform_drv_probe+0",
        "    iommu-debug          0xc208c360           iommu-debug-test        platform_drv_probe+0",
        "\n",
        "  Display all class info:",
        "    %s> dd -c",
        "    name                 class                subsys_private",
        "    regulator            0xc20741b0           0xf6ffe400",
        "    bdi                  0xf643a180           0xf645a800",
        "    gpio                 0xc205f010           0xf645b600",
        "\n",
        "  Display all device and driver info for specified class:",
        "    %s> dd -C rpmsg",
        "    ============================================================================",
        "                                      All devices",
        "    ============================================================================",
        "    name                 device               driver_name          driver",
        "    rpmsg_ctrl0          0xf67db068",
        "    rpmsg_ctrl1          0xf20b5068",
        "    rpmsg_ctrl2          0xefaa4c68",
        "\n",
        "  Display all device info:",
        "    %s> dd -l",
        "    name                 device               Bus             driver_name          driver",
        "    reg-dummy            0xf6410810           platform        reg-dummy            0xc2074488",
        "    soc                  0xf64a0410           platform",
        "    soc:psci             0xf64a2410           platform",
        "    soc:timer            0xf64a2010           platform",
        "\n",
        "  Display all driver info:",
        "    %s> dd -L",
        "    name                 device_driver        Bus             compatible              probe func",
        "    dcc                  0xc2071e88           platform        dcc-v2                  platform_drv_probe+0",
        "    watchdog             0xc2072b68           platform        watchdog                platform_drv_probe+0",
        "\n",
        "  Display all device for specified driver:",
        "    %s> dd -s rpm-smd-regulator-resource",
        "       name                             device",
        "       rpm-smd:rpm-regulator-smpa1      0xf60b8810",
        "       rpm-smd:rpm-regulator-smpa3      0xf60ba810",
        "       rpm-smd:rpm-regulator-smpa4      0xf60bac10",
        "       rpm-smd:rpm-regulator-smpa5      0xf60b8410",
        "\n",
        "  Display all char device:",
        "    %s> dd -a ",
        "    name                 char_device_struct   major      minorct    cdev",
        "    mem                  0xffffff80095b5b00   1          256        0xffffff800a3aac00",
        "    /dev/tty             0xffffff80095b5800   5          1          0x0",
        "    /dev/console         0xffffff80095bbe80   5          1          0x0",
        "    misc                 0xffffff80099b7f80   10         256        0xffffff800a39ec00",
        "\n",
        "  Display all misc device:",
        "    %s> dd -m ",
        "    name                 miscdevice           minor      ops",
        "    gunyah               0xffffffd0d7f763f0   124        gh_dev_fops",
        "    cpu_dma_latency      0xffffffd0dd022060   125        cpu_latency_qos_fops",
        "    ashmem               0xffffffd0dd193370   127        ashmem_fops",
        "\n",
        "  Display all block device:",
        "    %s> dd -D ",
        "    devname              block_device     super_block      type  bsize volname              UUID",
        "    mmcblk0p1            0xffffff8809154280  0xffffff885d8d4000  ext4  4096  xbl_a                0c768d03-3a3c-a990-0600-daf94674e882",
        "    loop13               0xffffff8809156a40  0xffffff885d32c000  ext4  4096  xbl_b                fcb4ebd9-7f11-c81c-ef48-43b90e410a24",
        "    mmcblk0p5            0xffffff8818e60d80  0xffffff885d32c000  ext4  4096  shrm_a               4717f043-65f9-3aa8-0e23-877e2c76ca7f",
        "\n",
        "  Display all disk:",
        "    %s> dd -d ",
        "    name         gendisk              minor major partitions",
        "    ram1         0xffffff8802ffa000   1     1     1",
        "    ram2         0xffffff8802ffb800   1     1     1",
        "    ram0         0xffffff8802ffe800   1     1     1",
        "\n",
        "  Display all partition for specified disk:",
        "    %s> dd -p mmcblk0",
        "    devname              block_device     partno start_sect sectors    size       bsize  type   volname              UUID",
        "    mmcblk0p69           0xffffff8818e9a800  0      21364736   471040     230MB      0             vm-persist           05bdafd5-4ec6-238c-7366-42ba33c98fb2",
        "    mmcblk0p82           0xffffff8818ea1ac0  0      30670848   512        256KB      0             apdp                 dafea201-493e-7f88-ae4b-590a34946832",
        "    mmcblk0p81           0xffffff8818ea5d00  0      30539776   8          4KB        0             devinfo              2e547183-c9aa-f37a-0bbc-84d081cc4394",
        "\n",
    };
}

DDriver::DDriver(){
}

/**
 * Print all device class information in formatted table
 *
 * Displays a formatted table containing all device classes found in the system.
 * The table includes class name, address, and subsys_private pointer.
 *
 * Output format:
 *   name                 class                subsys_private
 *   <class_name>         <addr>               <subsys_private_addr>
 *
 */
void DDriver::print_class_info(){
    size_t name_max_len = 10;
    for (auto& class_ptr : for_each_class_type()) {
        name_max_len = std::max(name_max_len, class_ptr->name.size());
    }

    std::ostringstream oss;
    // Output header with name first
    oss << std::left << std::setw(name_max_len)     << "name" << " "
        << std::left << std::setw(VADDR_PRLEN + 2)  << "class" << " "
        << std::left                                << "subsys_private"
        << "\n";

    // Output each class with name first and 0x prefix for hex addresses
    for (auto& class_ptr : for_each_class_type()) {
        oss << std::left << std::setw(name_max_len) << class_ptr->name << " "
            << std::hex << std::showbase << std::left << std::setw(VADDR_PRLEN + 2) << class_ptr->addr << " "
            << std::hex << std::showbase << class_ptr->subsys_private
            << "\n";
    }
    PRINT("%s \n", oss.str().c_str());
}

/**
 * Print all bus type information in formatted table
 *
 * Displays a formatted table containing all bus types found in the system.
 * The table includes bus name, address, subsys_private pointer, probe function, and device/driver counts.
 *
 * Output format:
 *   name         bus_type         subsys_private   probe func            dev_cnt drv_cnt
 *   <bus_name>   <addr>           <subsys_addr>    <probe_func>          <count> <count>
 *
 */
void DDriver::print_bus_info(){
    // Collect all bus information with device/driver counts
    struct BusInfo {
        std::shared_ptr<bus_type> bus_ptr;
        size_t dev_cnt;
        size_t drv_cnt;
    };
    std::vector<BusInfo> bus_list;

    for (auto& bus_ptr : for_each_bus_type()) {
        BusInfo info;
        info.bus_ptr = bus_ptr;
        info.dev_cnt = for_each_device_for_bus(bus_ptr->name).size();
        info.drv_cnt = for_each_driver(bus_ptr->name).size();
        bus_list.push_back(info);
    }

    // Calculate column widths
    size_t name_max_len = 10;
    size_t probe_max_len = 10;
    size_t dev_cnt_max_len = 7;  // "dev_cnt"
    size_t drv_cnt_max_len = 7;  // "drv_cnt"

    for (auto& info : bus_list) {
        name_max_len = std::max(name_max_len, info.bus_ptr->name.size());
        probe_max_len = std::max(probe_max_len, info.bus_ptr->probe.size());
        dev_cnt_max_len = std::max(dev_cnt_max_len, std::to_string(info.dev_cnt).size());
        drv_cnt_max_len = std::max(drv_cnt_max_len, std::to_string(info.drv_cnt).size());
    }

    std::ostringstream oss;
    // Output header with name first
    oss << std::left << std::setw(name_max_len)     << "name" << " "
        << std::left << std::setw(VADDR_PRLEN + 2)  << "bus_type" << " "
        << std::left << std::setw(VADDR_PRLEN + 2)  << "subsys_private" << " "
        << std::left << std::setw(probe_max_len)    << "probe func" << " "
        << std::right << std::setw(dev_cnt_max_len) << "dev_cnt" << " "
        << std::right << std::setw(drv_cnt_max_len) << "drv_cnt"
        << "\n";

    // Output each bus with name first and counts at the end
    for (auto& info : bus_list) {
        oss << std::left << std::setw(name_max_len) << info.bus_ptr->name << " "
            << std::hex << std::showbase << std::left << std::setw(VADDR_PRLEN + 2) << info.bus_ptr->addr << " "
            << std::hex << std::showbase << std::left << std::setw(VADDR_PRLEN + 2) << info.bus_ptr->subsys_private << " "
            << std::left << std::setw(probe_max_len) << info.bus_ptr->probe << " "
            << std::dec << std::right << std::setw(dev_cnt_max_len) << info.dev_cnt << " "
            << std::dec << std::right << std::setw(drv_cnt_max_len) << info.drv_cnt
            << "\n";
    }
    PRINT("%s \n",oss.str().c_str());
}

/**
 * Print all devices across all buses in formatted table
 *
 * Displays a comprehensive table of all devices found on all buses in the system.
 * For each device, shows its name, address, associated bus, and bound driver (if any).
 *
 * Output format:
 *   name                 device               Bus             driver_name          driver
 *   <device_name>        <addr>               <bus_name>      <driver_name>        <drv_addr>
 *
 */
void DDriver::print_device_list(){
    size_t name_max_len = 10;
    size_t driver_name_max_len = 11; // "driver_name"
    for (auto& bus_ptr : for_each_bus_type()) {
        for (auto& dev_ptr : for_each_device_for_bus(bus_ptr->name)) {
            name_max_len = std::max(name_max_len, dev_ptr->name.size());
            if (dev_ptr->driv != 0){
                std::shared_ptr<driver> driv_ptr = parser_driver(dev_ptr->driv);
                if (driv_ptr) {
                    driver_name_max_len = std::max(driver_name_max_len, driv_ptr->name.size());
                }
            }
        }
    }

    std::ostringstream oss;
    // Output header with name first
    oss << std::left << std::setw(name_max_len)        << "name" << " "
        << std::left << std::setw(VADDR_PRLEN + 2)     << "device" << " "
        << std::left << std::setw(15)                  << "Bus" << " "
        << std::left << std::setw(driver_name_max_len) << "driver_name" << " "
        << std::left                                   << "driver"
        << "\n";

    // Output each device with name first and 0x prefix for hex addresses
    for (auto& bus_ptr : for_each_bus_type()) {
        for (auto& dev_ptr : for_each_device_for_bus(bus_ptr->name)) {
            oss << std::left << std::setw(name_max_len) << dev_ptr->name << " "
                << std::hex << std::showbase << std::left << std::setw(VADDR_PRLEN + 2) << dev_ptr->addr << " "
                << std::left << std::setw(15) << bus_ptr->name << " ";
            if (dev_ptr->driv != 0){
                std::shared_ptr<driver> driv_ptr = parser_driver(dev_ptr->driv);
                if (driv_ptr) {
                    oss << std::left << std::setw(driver_name_max_len) << driv_ptr->name << " "
                        << std::hex << std::showbase << driv_ptr->addr;
                }
            }
            oss << "\n";
        }
    }
    PRINT("%s \n", oss.str().c_str());
}

/**
 * Print all devices and drivers for a specific bus
 *
 * Displays two formatted tables with summary statistics:
 * 1. All devices on the specified bus with their bound drivers
 * 2. All drivers registered on the specified bus with their properties
 *
 * Device table format:
 *   name                 device               driver_name          device_driver
 *   <device_name>        <addr>               <driver_name>        <drv_addr>
 *
 * Driver table format:
 *   name                 device_driver        compatible              probe_func
 *   <driver_name>        <addr>               <compatible_str>        <probe_func>
 *
 * @param bus_name Name of the bus to query (e.g., "platform", "pci", "usb")
 */
void DDriver::print_device_driver_for_bus(std::string bus_name){
    // Collect device and driver information
    std::vector<std::shared_ptr<device>> device_list = for_each_device_for_bus(bus_name);
    std::vector<std::shared_ptr<driver>> driver_list;
    for (const auto& driver_addr : for_each_driver(bus_name)) {
        std::shared_ptr<driver> driv_ptr = parser_driver(driver_addr);
        driver_list.push_back(driv_ptr);
    }

    // Count bound devices
    size_t bound_dev_cnt = 0;
    for (auto& dev_ptr : device_list) {
        if (is_kvaddr(dev_ptr->driv)) {
            bound_dev_cnt++;
        }
    }

    std::ostringstream oss;

    // Print summary header
    oss << "\n";
    oss << "================================================================================\n";
    oss << "  Bus: " << std::left << std::setw(15) << bus_name
        << "Devices: " << std::setw(3) << device_list.size()
        << " (Bound: " << std::setw(3) << bound_dev_cnt
        << ", Unbound: " << std::setw(3) << (device_list.size() - bound_dev_cnt) << ")"
        << "  Drivers: " << driver_list.size() << "\n";
    oss << "================================================================================\n\n";

    // Calculate column widths for devices
    size_t name_max_len = 10;
    size_t driver_name_max_len = 11; // "driver_name"
    for (auto& dev_ptr : device_list) {
        name_max_len = std::max(name_max_len, dev_ptr->name.size());
        if (is_kvaddr(dev_ptr->driv)){
            std::shared_ptr<driver> driv_ptr = parser_driver(dev_ptr->driv);
            if (driv_ptr) {
                driver_name_max_len = std::max(driver_name_max_len, driv_ptr->name.size());
            }
        }
    }

    // Calculate separator line length for devices
    size_t dev_sep_len = name_max_len + (VADDR_PRLEN + 2) * 2 + driver_name_max_len + 3;

    // Print devices section
    oss << "[Devices]\n";
    oss << std::string(dev_sep_len, '-') << "\n";
    oss << std::left << std::setw(name_max_len)        << "name"   << " "
        << std::left << std::setw(VADDR_PRLEN + 2)     << "device" << " "
        << std::left << std::setw(driver_name_max_len) << "driver_name" << " "
        << std::left                                   << "device_driver"
        << "\n";
    oss << std::string(dev_sep_len, '-') << "\n";

    for (auto& dev_ptr : device_list) {
        oss << std::left << std::setw(name_max_len) << dev_ptr->name << " "
            << std::hex << std::showbase << std::left << std::setw(VADDR_PRLEN + 2) << dev_ptr->addr << " ";
        if (is_kvaddr(dev_ptr->driv)){
            std::shared_ptr<driver> driv_ptr = parser_driver(dev_ptr->driv);
            if (driv_ptr) {
                oss << std::left << std::setw(driver_name_max_len) << driv_ptr->name << " "
                    << std::hex << std::showbase << driv_ptr->addr;
            }
        }
        oss << "\n";
    }

    // Print drivers section
    if (driver_list.size() > 0) {
        oss << "\n";
        oss << "[Drivers]\n";

        name_max_len = 10;
        size_t compat_max_len = 10;
        size_t probe_max_len = 10;
        for (auto& driv_ptr : driver_list) {
            name_max_len = std::max(name_max_len, driv_ptr->name.size());
            compat_max_len = std::max(compat_max_len, driv_ptr->compatible.size());
            probe_max_len = std::max(probe_max_len, driv_ptr->probe.size());
        }

        // Calculate separator line length for drivers
        size_t drv_sep_len = name_max_len + (VADDR_PRLEN + 2) + compat_max_len + probe_max_len + 3;

        oss << std::string(drv_sep_len, '-') << "\n";
        oss << std::left << std::setw(name_max_len)    << "name" << " "
            << std::left << std::setw(VADDR_PRLEN + 2) << "device_driver" << " "
            << std::left << std::setw(compat_max_len)  << "compatible" << " "
            << std::left                               << "probe_func"
            << "\n";
        oss << std::string(drv_sep_len, '-') << "\n";

        for (auto& driv_ptr : driver_list) {
            oss << std::left << std::setw(name_max_len) << driv_ptr->name << " "
                << std::hex << std::showbase << std::left << std::setw(VADDR_PRLEN + 2) << driv_ptr->addr << " "
                << std::left << std::setw(compat_max_len) << driv_ptr->compatible << " "
                << std::left << driv_ptr->probe << "\n";
        }
    }

    oss << "\n";

    PRINT("%s\n", oss.str().c_str());
}

/**
 * Print all devices for a specific device class
 *
 * Displays a formatted table of all devices belonging to the specified device class.
 * Shows device name, address, and bound driver information (if any).
 *
 * Output format:
 *   name                 device               driver_name          driver
 *   <device_name>        <addr>               <driver_name>        <drv_addr>
 *
 * @param class_name Name of the device class to query (e.g., "block", "net", "input")
 */
void DDriver::print_device_driver_for_class(std::string class_name){
    PRINT("============================================================================\n");
    PRINT("                                All devices                                 \n");
    PRINT("============================================================================\n");

    std::vector<std::shared_ptr<device>> device_list = for_each_device_for_class(class_name);

    // Calculate column widths
    size_t name_max_len = 10;
    size_t driver_name_max_len = 11; // "driver_name"
    for (auto& dev_ptr : device_list) {
        name_max_len = std::max(name_max_len, dev_ptr->name.size());
        if (is_kvaddr(dev_ptr->driv)){
            std::shared_ptr<driver> driv_ptr = parser_driver(dev_ptr->driv);
            if (driv_ptr) {
                driver_name_max_len = std::max(driver_name_max_len, driv_ptr->name.size());
            }
        }
    }

    std::ostringstream oss;
    // Output header with name first
    oss << std::left << std::setw(name_max_len)        << "name" << " "
        << std::left << std::setw(VADDR_PRLEN + 2)     << "device" << " "
        << std::left << std::setw(driver_name_max_len) << "driver_name" << " "
        << std::left                                   << "driver"
        << "\n";

    // Output each device with name first and 0x prefix for hex addresses
    for (auto& dev_ptr : device_list) {
        oss << std::left << std::setw(name_max_len) << dev_ptr->name << " "
            << std::hex << std::showbase << std::left << std::setw(VADDR_PRLEN + 2) << dev_ptr->addr << " ";
        if (is_kvaddr(dev_ptr->driv)){
            std::shared_ptr<driver> driv_ptr = parser_driver(dev_ptr->driv);
            if (driv_ptr) {
                oss << std::left << std::setw(driver_name_max_len) << driv_ptr->name << " "
                    << std::hex << std::showbase << driv_ptr->addr;
            }
        }
        oss << "\n";
    }
    PRINT("%s \n",oss.str().c_str());
}

/**
 * Print all character device structures in formatted table
 *
 * Displays information about all character devices registered in the system.
 * Shows device name, structure address, major number, minor count, and cdev pointer.
 *
 * Output format:
 *   name                 char_device_struct   major      minorct    cdev
 *   <dev_name>           <addr>               <major>    <count>    <cdev_addr>
 */
void DDriver::print_char_device(){
    // Collect all character devices and calculate column widths
    struct CharDevInfo {
        ulong addr;
        uint32_t major;
        uint32_t minorct;
        ulong cdev_addr;
        std::string name;
    };
    std::vector<CharDevInfo> chardev_list;

    for (auto& addr : for_each_char_device()) {
        CharDevInfo info;
        info.addr = addr;
        info.major = read_uint(addr + field_offset(char_device_struct, major), "major");
        info.minorct = read_uint(addr + field_offset(char_device_struct, minorct), "minorct");
        info.cdev_addr = read_pointer(addr + field_offset(char_device_struct, cdev), "cdev");
        info.name = read_cstring(addr + field_offset(char_device_struct, name), 64, "name");
        chardev_list.push_back(info);
    }

    // Calculate column widths
    size_t name_max_len = 10;
    for (auto& info : chardev_list) {
        name_max_len = std::max(name_max_len, info.name.size());
    }

    std::ostringstream oss;
    // Output header with name first
    oss << std::left << std::setw(name_max_len)        << "name" << " "
        << std::left << std::setw(VADDR_PRLEN + 2)     << "char_device_struct" << " "
        << std::left << std::setw(10)                  << "major" << " "
        << std::left << std::setw(10)                  << "minorct" << " "
        << std::left                                   << "cdev"
        << "\n";

    // Output each character device with name first and 0x prefix for hex addresses
    for (auto& info : chardev_list) {
        oss << std::left << std::setw(name_max_len) << info.name << " "
            << std::hex << std::showbase << std::left << std::setw(VADDR_PRLEN + 2) << info.addr << " "
            << std::dec << std::left << std::setw(10) << info.major << " "
            << std::dec << std::left << std::setw(10) << info.minorct << " "
            << std::hex << std::showbase << info.cdev_addr
            << "\n";
    }
    PRINT("%s \n", oss.str().c_str());
}

/**
 * Print all generic disk (gendisk) structures in formatted table
 *
 * Displays information about all block devices (disks) in the system.
 * Shows disk name, address, minor number, major number, and partition count.
 *
 * Output format:
 *   name         gendisk              minor major partitions
 *   <disk_name>  <addr>               <min> <maj> <count>
 *
 */
void DDriver::print_gendisk(){
    // Collect all disks and calculate column widths
    struct DiskInfo {
        ulong addr;
        int major;
        int minor;
        int partition_cnt;
        std::string name;
    };
    std::vector<DiskInfo> disk_list;

    std::set<ulong> disk_addrs;
    for (auto& addr : for_each_disk()) {
        disk_addrs.insert(addr);
    }

    for (auto& addr : disk_addrs) {
        DiskInfo info;
        info.addr = addr;
        info.major = read_uint(addr + field_offset(gendisk, major), "major");
        info.minor = read_uint(addr + field_offset(gendisk, minors), "minors");
        info.name = read_cstring(addr + field_offset(gendisk, disk_name), 32, "name");

        ulong tbl_addr = 0;
        if (field_size(gendisk, part_tbl) == sizeof(void *)){
            tbl_addr = read_pointer(addr + field_offset(gendisk, part_tbl), "part_tbl");
            if (is_kvaddr(tbl_addr)){
                info.partition_cnt = read_uint(tbl_addr + field_offset(disk_part_tbl, len), "len");
            }
        }else{
            tbl_addr = addr + field_offset(gendisk, part_tbl);
            std::vector<ulong> ptbl = for_each_xarray(tbl_addr);
            info.partition_cnt = ptbl.size();
        }
        disk_list.push_back(info);
    }

    // Calculate column widths
    size_t name_max_len = 10;
    for (auto& info : disk_list) {
        name_max_len = std::max(name_max_len, info.name.size());
    }

    std::ostringstream oss;
    // Output header with name first
    oss << std::left << std::setw(name_max_len)    << "name" << " "
        << std::left << std::setw(VADDR_PRLEN + 2) << "gendisk" << " "
        << std::left << std::setw(5)               << "minor" << " "
        << std::left << std::setw(5)               << "major" << " "
        << std::left                               << "partitions"
        << "\n";

    // Output each disk with name first and 0x prefix for hex addresses
    for (auto& info : disk_list) {
        oss << std::left << std::setw(name_max_len) << info.name << " "
            << std::hex << std::showbase << std::left << std::setw(VADDR_PRLEN + 2) << info.addr << " "
            << std::dec << std::left << std::setw(5) << info.minor << " "
            << std::dec << std::left << std::setw(5) << info.major << " "
            << std::dec << info.partition_cnt
            << "\n";
    }
    PRINT("%s \n", oss.str().c_str());
}

/**
 * Print all partitions for a specific disk in formatted table
 *
 * Displays detailed information about all partitions on the specified disk.
 * Shows device name, partition address, partition number, start sector, size, filesystem type, volume name, and UUID.
 *
 * Output format (for hd_struct):
 *   devname              hd_struct        partno start_sect sectors    size       bsize  type   volname              UUID
 *   <devname>            <addr>           <num>  <start>    <sectors>  <size_str> <bsz>  <fs>   <volname>            <uuid>
 *
 * Output format (for block_device):
 *   devname              block_device     partno start_sect sectors    size       bsize  type   volname              UUID
 *   <devname>            <addr>           <num>  <start>    <sectors>  <size_str> <bsz>  <fs>   <volname>            <uuid>
 *
 * @param disk_name Name of the disk to query (e.g., "sda", "mmcblk0", "nvme0n1")
 */
void DDriver::print_partition(std::string disk_name){
    std::vector<ulong> ptbl;
    for (auto& addr : for_each_disk()) {
        std::string name = read_cstring(addr + field_offset(gendisk,disk_name),32, "name");
        if (name != disk_name){
            continue;
        }
        ulong tbl_addr = 0;
        if (field_size(gendisk,part_tbl) == sizeof(void *)){
            tbl_addr = read_pointer(addr + field_offset(gendisk,part_tbl),"part_tbl");
            if (!is_kvaddr(tbl_addr)){
                break;
            }
            size_t len = read_uint(tbl_addr + field_offset(disk_part_tbl,len),"len");
            ulong ptb_addr = tbl_addr + field_offset(disk_part_tbl,part);
            for (size_t i = 0; i < len; i++){
                ulong hd_addr = read_pointer(ptb_addr + (i * sizeof(void *)),"part");
                if (!is_kvaddr(hd_addr)){
                    continue;
                }
                ptbl.push_back(hd_addr);
            }
        }else{
            tbl_addr = addr + field_offset(gendisk,part_tbl);
            if (!is_kvaddr(tbl_addr)){
                break;
            }
            ptbl = for_each_xarray(tbl_addr);
        }
    }
    if (ptbl.size() == 0){
        return;
    }
    std::vector<std::shared_ptr<partition>> part_list;
    for (auto& addr : ptbl) {
        std::shared_ptr<partition> part_ptr;
        if (struct_size(hd_struct) != -1){
            part_ptr = parser_hd_struct(addr);
        }else{
            part_ptr = parser_block_device(addr);
        }
        if (part_ptr == nullptr){
            continue;
        }
        part_list.push_back(part_ptr);
    }
    std::sort(part_list.begin(), part_list.end(),[&](std::shared_ptr<partition> a, std::shared_ptr<partition> b){
        return a->partno < b->partno;
    });
    if (struct_size(hd_struct) == -1 && field_offset(block_device,bd_nr_sectors) == -1){
        for (size_t i = 1; i < part_list.size() - 1; i++){
            part_list[i]->nr_sectors = part_list[i + 1]->start_sect - part_list[i]->start_sect;
        }
    }

    // Calculate column widths
    size_t devname_max_len = 10;
    size_t volname_max_len = 10;
    for (auto& part_ptr : part_list) {
        devname_max_len = std::max(devname_max_len, part_ptr->devname.size());
        volname_max_len = std::max(volname_max_len, part_ptr->volname.size());
    }

    std::ostringstream oss;
    if (struct_size(hd_struct) != -1){
        // Output header with devname first
        oss << std::left << std::setw(devname_max_len) << "devname" << " "
            << std::left << std::setw(VADDR_PRLEN + 2) << "hd_struct" << " "
            << std::left << std::setw(6)               << "partno" << " "
            << std::left << std::setw(10)              << "start_sect" << " "
            << std::left << std::setw(10)              << "sectors" << " "
            << std::left << std::setw(10)              << "size" << " "
            << std::left << std::setw(6)               << "bsize" << " "
            << std::left << std::setw(6)               << "type" << " "
            << std::left << std::setw(volname_max_len) << "volname" << " "
            << std::left << "UUID" << "\n";
    }else{
        // Output header with devname first
        oss << std::left << std::setw(devname_max_len) << "devname" << " "
            << std::left << std::setw(VADDR_PRLEN + 2) << "block_device" << " "
            << std::left << std::setw(6)               << "partno" << " "
            << std::left << std::setw(10)              << "start_sect" << " "
            << std::left << std::setw(10)              << "sectors" << " "
            << std::left << std::setw(10)              << "size" << " "
            << std::left << std::setw(6)               << "bsize" << " "
            << std::left << std::setw(6)               << "type" << " "
            << std::left << std::setw(volname_max_len) << "volname" << " "
            << std::left << "UUID" << "\n";
    }

    // Output each partition with devname first and 0x prefix for hex addresses
    for (auto& part_ptr : part_list) {
        oss << std::left << std::setw(devname_max_len) << part_ptr->devname << " "
            << std::hex << std::showbase << std::left << std::setw(VADDR_PRLEN + 2) << part_ptr->addr << " "
            << std::dec << std::left << std::setw(6) << part_ptr->partno << " "
            << std::dec << std::left << std::setw(10) << part_ptr->start_sect << " "
            << std::dec << std::left << std::setw(10) << part_ptr->nr_sectors << " "
            << std::left << std::setw(10) << csize(part_ptr->nr_sectors*512) << " "
            << std::dec << std::left << std::setw(6) << part_ptr->block_size << " "
            << std::left << std::setw(6) << part_ptr->fs_type << " "
            << std::left << std::setw(volname_max_len) << part_ptr->volname << " "
            << std::left << part_ptr->uuid
            << "\n";
    }
    PRINT("%s \n",oss.str().c_str());
}

/**
 * Parse legacy hd_struct (hard disk structure) from kernel memory
 *
 * Extracts partition information from the legacy hd_struct kernel structure.
 * This structure was used in older kernel versions to represent disk partitions.
 *
 * Extracted information includes:
 * - Partition number (partno)
 * - Start sector (start_sect)
 * - Number of sectors (nr_sects)
 * - Partition metadata (UUID, volume name)
 * - Device name
 * - Filesystem type and block size (by finding associated block_device)
 *
 * @param addr Kernel virtual address of the hd_struct structure
 * @return Shared pointer to partition object containing parsed data, or nullptr on failure
 */
std::shared_ptr<partition> DDriver::parser_hd_struct(ulong addr){
    std::shared_ptr<partition> part_ptr = std::make_shared<partition>();
    part_ptr->addr = addr;
    if (field_offset(hd_struct,start_sect) != -1){
        part_ptr->start_sect = read_ulonglong(addr + field_offset(hd_struct,start_sect),"start_sect");
    }
    if (field_offset(hd_struct,nr_sects) != -1){
        part_ptr->nr_sectors = read_ulonglong(addr + field_offset(hd_struct,nr_sects),"nr_sects");
    }
    if (field_offset(hd_struct,partno) != -1){
        part_ptr->partno = read_uint(addr + field_offset(hd_struct,partno),"partno");
    }
    if (field_offset(hd_struct,info) != -1){
        ulong meta_info_addr = read_pointer(addr + field_offset(hd_struct,info),"info");
        if (is_kvaddr(meta_info_addr)){
            part_ptr->uuid = read_cstring(meta_info_addr + field_offset(partition_meta_info,uuid),37, "uuid");
            part_ptr->volname = read_cstring(meta_info_addr + field_offset(partition_meta_info,volname),64, "volname");
        }
    }
    if (field_offset(hd_struct,__dev) != -1){
        size_t name_addr = read_pointer(addr + field_offset(hd_struct,__dev) + field_offset(device,kobj) + field_offset(kobject,name),"device name addr");
        if (is_kvaddr(name_addr)){
            part_ptr->devname = read_cstring(name_addr,100, "device name");
        }
    }
    //find the block_device for this hd_struct
    for (auto& bd_addr : for_each_bdev()) {
        ulong bd_part_addr = read_pointer(bd_addr + field_offset(block_device,bd_part),"bd_part");
        if (bd_part_addr != addr){
            continue;
        }
        ulong bd_super = 0;
        if (field_offset(block_device,bd_super) != -1){
            bd_super = read_pointer(bd_addr + field_offset(block_device,bd_super),"bd_super");
        }
        if (is_kvaddr(bd_super)){
            if (field_offset(super_block,s_blocksize) != -1){
                part_ptr->block_size = read_ulong(bd_super + field_offset(super_block,s_blocksize),"s_blocksize");
            }
            if (field_offset(super_block,s_type) != -1){
                ulong fst_addr = read_pointer(bd_super + field_offset(super_block,s_type),"s_type");
                if (is_kvaddr(fst_addr)){
                    size_t name_addr = read_pointer(fst_addr + field_offset(file_system_type,name),"name addr");
                    if (is_kvaddr(name_addr)){
                        part_ptr->fs_type = read_cstring(name_addr,64, "name");
                    }
                }
            }
        }
    }
    LOGD("=== parser_hd_struct(0x%lx) - Complete ===\n", addr);
    return part_ptr;
}

/**
 * Parse block_device structure from kernel memory
 *
 * Extracts partition information from the modern block_device kernel structure.
 * This structure is used in newer kernel versions to represent disk partitions.
 *
 * Extracted information includes:
 * - Partition number (bd_partno)
 * - Start sector (bd_start_sect)
 * - Number of sectors (bd_nr_sectors)
 * - Block size (bd_block_size)
 * - Partition metadata (UUID, volume name from bd_meta_info)
 * - Device name
 * - Filesystem type (from associated super_block)
 *
 * @param addr Kernel virtual address of the block_device structure
 * @return Shared pointer to partition object containing parsed data, or nullptr on failure
 */
std::shared_ptr<partition> DDriver::parser_block_device(ulong addr){
    void *buf = read_struct(addr,"block_device");
    if (!buf) {
        return nullptr;
    }
    std::shared_ptr<partition> part_ptr = std::make_shared<partition>();
    part_ptr->addr = addr;
    if (field_offset(block_device,bd_start_sect) != -1){
        part_ptr->start_sect = ULONGLONG(buf + field_offset(block_device,bd_start_sect));
    }
    if (field_offset(block_device,bd_nr_sectors) != -1){
        part_ptr->nr_sectors = ULONGLONG(buf + field_offset(block_device,bd_nr_sectors));
    }
    if (field_offset(block_device,bd_meta_info) != -1){
        ulong meta_info = ULONG(buf + field_offset(block_device,bd_meta_info));
        if (is_kvaddr(meta_info)){
            part_ptr->uuid = read_cstring(meta_info + field_offset(partition_meta_info,uuid),37, "uuid");
            part_ptr->volname = read_cstring(meta_info + field_offset(partition_meta_info,volname),64, "volname");
        }
    }
    if (field_offset(block_device,bd_block_size) != -1){
        part_ptr->block_size = UINT(buf + field_offset(block_device,bd_block_size));
    }
    if (field_offset(block_device,bd_partno) != -1){
        part_ptr->partno = USHORT(buf + field_offset(block_device,bd_partno));
    }
    if (field_offset(block_device,bd_device) != -1){
        size_t name_addr = read_pointer(addr + field_offset(block_device,bd_device) + field_offset(device,kobj) + field_offset(kobject,name),"device name addr");
        if (is_kvaddr(name_addr)){
            part_ptr->devname = read_cstring(name_addr,64, "device name");
        }
    }
    if (field_offset(block_device,bd_super) != -1){
        ulong bd_super = ULONG(buf + field_offset(block_device,bd_super));
        if (is_kvaddr(bd_super)){
            if (field_offset(super_block,s_blocksize) != -1){
                part_ptr->block_size = read_ulong(bd_super + field_offset(super_block,s_blocksize),"s_blocksize");
            }
            if (field_offset(super_block,s_type) != -1){
                ulong fst_addr = read_pointer(bd_super + field_offset(super_block,s_type),"s_type");
                if (is_kvaddr(fst_addr)){
                    size_t name_addr = read_pointer(fst_addr + field_offset(file_system_type,name),"name addr");
                    if (is_kvaddr(name_addr)){
                        part_ptr->fs_type = read_cstring(name_addr,64, "name");
                    }
                }
            }
        }
    }else{
        //find the super_block for this block_device
        ulong list_head = csymbol_value("super_blocks");
        for (const auto& bd_super : for_each_list(list_head, field_offset(super_block, s_list))) {
            ulong bd_addr = read_pointer(bd_super + field_offset(super_block,s_bdev),"s_bdev");
            if (bd_addr != part_ptr->addr){
                continue;
            }
            if (field_offset(super_block,s_blocksize) != -1){
                part_ptr->block_size = read_ulong(bd_super + field_offset(super_block,s_blocksize),"s_blocksize");
            }
            if (field_offset(super_block,s_type) != -1){
                ulong fst_addr = read_pointer(bd_super + field_offset(super_block,s_type),"s_type");
                if (is_kvaddr(fst_addr)){
                    size_t name_addr = read_pointer(fst_addr + field_offset(file_system_type,name),"name addr");
                    if (is_kvaddr(name_addr)){
                        part_ptr->fs_type = read_cstring(name_addr,64, "name");
                    }
                }
            }
        }
    }
    FREEBUF(buf);
    return part_ptr;
}

/**
 * Print all block devices in formatted table
 *
 * Displays comprehensive information about all block devices in the system.
 * Shows device name, block device address, associated super_block, filesystem type, block size, volume name, and UUID.
 *
 * Output format:
 *   devname              block_device     super_block      type  bsize volname              UUID
 *   <devname>            <bd_addr>        <sb_addr>        <fs>  <bsz> <volname>            <uuid>
 */
void DDriver::print_block_device(){
    // Collect all block devices and calculate column widths
    struct BlockDevInfo {
        ulong addr;
        ulong bd_super;
        uint64_t bd_block_size;
        std::string fs_type;
        std::string uuid;
        std::string volname;
        std::string devname;
    };
    std::vector<BlockDevInfo> bd_list;

    std::set<ulong> bd_addrs;
    for (auto& addr : for_each_bdev()) {
        bd_addrs.insert(addr);
    }

    if (bd_addrs.size() == 0){
        return;
    }

    for (auto& addr : bd_addrs) {
        void *buf = read_struct(addr,"block_device");
        if (!buf) {
            continue;
        }

        BlockDevInfo info;
        info.addr = addr;
        info.bd_super = 0;
        info.bd_block_size = 0;

        if (field_offset(block_device,bd_meta_info) != -1){
            ulong bd_meta_info = ULONG(buf + field_offset(block_device,bd_meta_info));
            if (is_kvaddr(bd_meta_info)){
                info.uuid = read_cstring(bd_meta_info + field_offset(partition_meta_info,uuid),37, "uuid");
                info.volname = read_cstring(bd_meta_info + field_offset(partition_meta_info,volname),64, "volname");
            }
        }
        if (field_offset(block_device,bd_super) != -1){
            info.bd_super = ULONG(buf + field_offset(block_device,bd_super));
            if (is_kvaddr(info.bd_super)){
                if (field_offset(super_block,s_blocksize) != -1){
                    info.bd_block_size = read_ulong(info.bd_super + field_offset(super_block,s_blocksize),"s_blocksize");
                }
                if (field_offset(super_block,s_type) != -1){
                    ulong fst_addr = read_pointer(info.bd_super + field_offset(super_block,s_type),"s_type");
                    if (is_kvaddr(fst_addr)){
                        size_t name_addr = read_pointer(fst_addr + field_offset(file_system_type,name),"name addr");
                        if (is_kvaddr(name_addr)){
                            info.fs_type = read_cstring(name_addr,64, "name");
                        }
                    }
                }
            }
        }else{
            //find the super_block for this block_device
            ulong list_head = csymbol_value("super_blocks");
            for (const auto& sb_addr : for_each_list(list_head, field_offset(super_block, s_list))) {
                ulong bd_addr = read_pointer(sb_addr + field_offset(super_block,s_bdev),"s_bdev");
                if (bd_addr != addr){
                    continue;
                }
                info.bd_super = sb_addr;
if (field_offset(super_block,s_blocksize) != -1){
                    info.bd_block_size = read_ulong(sb_addr + field_offset(super_block,s_blocksize),"s_blocksize");
                }
                if (field_offset(super_block,s_type) != -1){
                    ulong fst_addr = read_pointer(sb_addr + field_offset(super_block,s_type),"s_type");
                    if (is_kvaddr(fst_addr)){
                        size_t name_addr = read_pointer(fst_addr + field_offset(file_system_type,name),"name addr");
                        if (is_kvaddr(name_addr)){
                            info.fs_type = read_cstring(name_addr,64, "name");
                        }
                    }
                }
            }
        }
        if (field_offset(block_device,bd_part) != -1){
            ulong bd_part = ULONGLONG(buf + field_offset(block_device,bd_part));
            if (is_kvaddr(bd_part)){
                if (field_offset(hd_struct,info) != -1){
                    ulong meta_info_addr = read_pointer(bd_part + field_offset(hd_struct,info),"info");
                    if (is_kvaddr(meta_info_addr)){
                        info.uuid = read_cstring(meta_info_addr + field_offset(partition_meta_info,uuid),37, "uuid");
                        info.volname = read_cstring(meta_info_addr + field_offset(partition_meta_info,volname),64, "volname");
                    }
                }
                if (field_offset(hd_struct,__dev) != -1){
                    size_t name_addr = read_pointer(bd_part + field_offset(hd_struct,__dev) + field_offset(device,kobj) + field_offset(kobject,name),"device name addr");
                    if (is_kvaddr(name_addr)){
                        info.devname = read_cstring(name_addr,100, "device name");
                    }
                }
            }
        }
        if (field_offset(block_device,bd_block_size) != -1){
            info.bd_block_size = UINT(buf + field_offset(block_device,bd_block_size));
        }
        if (field_offset(block_device,bd_device) != -1){
            size_t name_addr = read_pointer(addr + field_offset(block_device,bd_device) + field_offset(device,kobj) + field_offset(kobject,name),"device name addr");
            if (is_kvaddr(name_addr)){
                info.devname = read_cstring(name_addr,100, "device name");
            }
        }
        FREEBUF(buf);
        bd_list.push_back(info);
    }

    // Calculate column widths
    size_t devname_max_len = 10;
    size_t volname_max_len = 10;
    for (auto& info : bd_list) {
        devname_max_len = std::max(devname_max_len, info.devname.size());
        volname_max_len = std::max(volname_max_len, info.volname.size());
    }

    std::ostringstream oss;
    // Output header with devname first
    oss << std::left << std::setw(devname_max_len) << "devname" << " "
        << std::left << std::setw(VADDR_PRLEN + 2) << "block_device" << " "
        << std::left << std::setw(VADDR_PRLEN + 2) << "super_block" << " "
        << std::left << std::setw(5)               << "type" << " "
        << std::left << std::setw(5)               << "bsize" << " "
        << std::left << std::setw(volname_max_len) << "volname" << " "
        << std::left << "UUID" << "\n";

    // Output each block device with devname first and 0x prefix for hex addresses
    for (auto& info : bd_list) {
        oss << std::left << std::setw(devname_max_len) << info.devname << " "
            << std::hex << std::showbase << std::left << std::setw(VADDR_PRLEN + 2) << info.addr << " "
            << std::hex << std::showbase << std::left << std::setw(VADDR_PRLEN + 2) << info.bd_super << " "
            << std::left << std::setw(5) << info.fs_type << " "
            << std::dec << std::left << std::setw(5) << info.bd_block_size << " "
            << std::left << std::setw(volname_max_len) << info.volname << " "
            << std::left << info.uuid
            << "\n";
    }
    PRINT("%s \n", oss.str().c_str());
}

/**
 * Print all miscellaneous devices in formatted table
 *
 * Displays information about all miscellaneous character devices registered in the system.
 * Miscellaneous devices are character devices that share major number 10.
 * Shows device name, address, minor number, and file operations structure name.
 *
 * Output format:
 *   name                 miscdevice           minor      ops
 *   <dev_name>           <addr>               <minor>    <fops_symbol_name>
 *
 */
void DDriver::print_misc_device(){
    // Collect all misc devices and calculate column widths
    struct MiscDevInfo {
        ulong addr;
        uint32_t minor;
        std::string name;
        std::string ops_name;
    };
    std::vector<MiscDevInfo> misc_list;

    for (auto& addr : for_each_misc_dev()) {
        MiscDevInfo info;
        info.addr = addr;
        info.minor = read_uint(addr + field_offset(miscdevice, minor), "minor");

        size_t name_addr = read_pointer(addr + field_offset(miscdevice, name), "name addr");
        if (is_kvaddr(name_addr)){
            info.name = read_cstring(name_addr, 64, "name");
        }

        size_t ops_addr = read_pointer(addr + field_offset(miscdevice, fops), "fops addr");
        info.ops_name = to_symbol(ops_addr);
        misc_list.push_back(info);
    }

    // Calculate column widths
    size_t name_max_len = 10;
    size_t ops_max_len = 10;
    for (auto& info : misc_list) {
        name_max_len = std::max(name_max_len, info.name.size());
        ops_max_len = std::max(ops_max_len, info.ops_name.size());
    }

    std::ostringstream oss;
    // Output header with name first
    oss << std::left << std::setw(name_max_len)    << "name" << " "
        << std::left << std::setw(VADDR_PRLEN + 2) << "miscdevice" << " "
        << std::left << std::setw(10)              << "minor" << " "
        << std::left                               << "ops"
        << "\n";

    // Output each misc device with name first and 0x prefix for hex addresses
    for (auto& info : misc_list) {
        oss << std::left << std::setw(name_max_len) << info.name << " "
            << std::hex << std::showbase << std::left << std::setw(VADDR_PRLEN + 2) << info.addr << " "
            << std::dec << std::left << std::setw(10) << info.minor << " "
            << std::left << info.ops_name
            << "\n";
    }
    PRINT("%s \n", oss.str().c_str());
}

/**
 * Print all devices bound to a specific driver
 *
 * Displays a formatted table of all devices that are currently bound to the specified driver.
 * Shows device name and address for each bound device.
 *
 * Output format:
 *   name                 device
 *   <device_name>        <addr>
 *
 * @param driver_name Name of the driver to query (e.g., "ahci", "e1000e", "usbhid")
 */
void DDriver::print_device_list_for_driver(std::string driver_name){
    std::shared_ptr<driver> driv_ptr = find_device_driver(driver_name);
    if (driv_ptr == nullptr || !is_kvaddr(driv_ptr->addr)){
        return;
    }
    std::vector<std::shared_ptr<device>> device_list = for_each_device_for_driver(driv_ptr->addr);

    // Calculate column widths
    size_t name_max_len = 10;
    for (auto& dev_ptr : device_list) {
        name_max_len = std::max(name_max_len, dev_ptr->name.size());
    }

    std::ostringstream oss;
    // Output header with name first
    oss << std::left << "   " << std::setw(name_max_len) << "name" << " "
        << std::left << std::setw(VADDR_PRLEN + 2) << "device"
        << "\n";

    // Output each device with name first and 0x prefix for hex addresses
    for (auto& dev_ptr : device_list) {
        oss << std::left << "   " << std::setw(name_max_len) << dev_ptr->name << " "
            << std::hex << std::showbase << std::left << std::setw(VADDR_PRLEN + 2) << dev_ptr->addr
            << "\n";
    }
    PRINT("%s \n", oss.str().c_str());
}

/**
 * Print all drivers across all buses in formatted table
 *
 * Displays a comprehensive table of all device drivers registered on all buses in the system.
 * Shows driver name, address, associated bus, device tree compatible string, and probe function.
 *
 * Output format:
 *   name                 device_driver        Bus             compatible              probe func
 *   <driver_name>        <addr>               <bus_name>      <compatible_str>        <probe_func>
 *
 */
void DDriver::print_driver_list(){
    // Collect all drivers and calculate column widths
    struct DriverInfo {
        std::shared_ptr<driver> driv_ptr;
        std::string bus_name;
    };
    std::vector<DriverInfo> driver_list;

    for (const auto& bus_ptr : for_each_bus_type()) {
        for (const auto& driver_addr : for_each_driver(bus_ptr->name)) {
            DriverInfo info;
            info.driv_ptr = parser_driver(driver_addr);
            info.bus_name = bus_ptr->name;
            driver_list.push_back(info);
        }
    }

    // Calculate column widths
    size_t name_max_len = 10;
    size_t compat_max_len = 10;
    size_t probe_max_len = 10;

    for (auto& info : driver_list) {
        name_max_len = std::max(name_max_len, info.driv_ptr->name.size());
        compat_max_len = std::max(compat_max_len, info.driv_ptr->compatible.size());
        probe_max_len = std::max(probe_max_len, info.driv_ptr->probe.size());
    }

    std::ostringstream oss;
    // Output header with name first
    oss << std::left << std::setw(name_max_len)    << "name" << " "
        << std::left << std::setw(VADDR_PRLEN + 2) << "device_driver" << " "
        << std::left << std::setw(15)              << "Bus" << " "
        << std::left << std::setw(compat_max_len)  << "compatible" << " "
        << std::left                               << "probe func"
        << "\n";

    // Output each driver with name first and 0x prefix for hex addresses
    for (auto& info : driver_list) {
        oss << std::left << std::setw(name_max_len) << info.driv_ptr->name << " "
            << std::hex << std::showbase << std::left << std::setw(VADDR_PRLEN + 2) << info.driv_ptr->addr << " "
            << std::left << std::setw(15) << info.bus_name << " "
            << std::left << std::setw(compat_max_len) << info.driv_ptr->compatible << " "
            << std::left << info.driv_ptr->probe
            << "\n";
    }
    PRINT("%s \n", oss.str().c_str());
}

#pragma GCC diagnostic pop
