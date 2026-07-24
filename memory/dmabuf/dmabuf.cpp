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

#include "dmabuf.h"
#include "cmd_buf.h"
#include <limits>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

/**
 * @brief Main command handler (not used for Dmabuf class)
 *
 * This class is used as a helper by DmaIon, so cmd_main is not implemented.
 */
void Dmabuf::cmd_main(void) {

}

/**
 * @brief Initialize kernel structure offsets
 *
 * Registers all kernel structure definitions and field offsets needed for
 * parsing DMA buffers. This includes:
 * - dma_buf: Main DMA buffer structure
 * - dma_buf_attachment: Device attachment information
 * - device/driver: Device and driver metadata
 * - file: File descriptor information
 * - ion_buffer/qcom_sg_buffer: Buffer-specific structures
 * - sg_table/scatterlist: Scatter-gather list structures
 */
void Dmabuf::init_offset(void) {
    // Initialize dma_buf structure fields
    field_init(dma_buf, list_node);
    field_init(dma_buf, size);
    field_init(dma_buf, attachments);
    field_init(dma_buf, exp_name);
    field_init(dma_buf, name);
    field_init(dma_buf, priv);
    field_init(dma_buf, file);
    field_init(dma_buf, ops);

    // Initialize dma_buf_attachment structure fields
    field_init(dma_buf_attachment, node);
    field_init(dma_buf_attachment, dev);
    field_init(dma_buf_attachment, priv);
    field_init(dma_buf_attachment, sgt);
    field_init(dma_buf_attachment, importer_priv);
    field_init(dma_buf_attachment, priv);
    field_init(dma_buf_attachment, dma_map_attrs);
    field_init(dma_buf_attachment, dir);

    // Initialize device and driver structure fields
    field_init(device, kobj);
    field_init(device, driver);
    field_init(kobject, name);
    field_init(device_driver, name);

    // Initialize file structure fields
    field_init(file, private_data);
    field_init(file, f_op);
    field_init(file, f_count);
    field_init(file, f_vfsmnt);

    // Initialize buffer-specific structure fields
    field_init(ion_buffer, heap);
    field_init(ion_buffer, sg_table);
    field_init(qcom_sg_buffer, heap);
    field_init(qcom_sg_buffer, sg_table);

    // Initialize scatter-gather table structure fields
    field_init(sg_table, sgl);
    field_init(sg_table, nents);
    field_init(scatterlist, offset);
    field_init(scatterlist, length);
    field_init(scatterlist, dma_address);
    field_init(scatterlist, dma_length);
    field_init(scatterlist, page_link);

    // Initialize complete structures
    struct_init(dma_buf);
    struct_init(dma_buf_attachment);
    struct_init(device);
    struct_init(device_driver);
    struct_init(ion_buffer);
    struct_init(qcom_sg_buffer);
    struct_init(scatterlist);
}

/**
 * @brief Initialize command metadata (not used for Dmabuf class)
 *
 * This class is used as a helper by DmaIon, so init_command is not implemented.
 */
void Dmabuf::init_command(void) {

}

/**
 * @brief Constructor - initializes the Dmabuf parser
 *
 * Calls init_offset() to register all necessary kernel structure definitions.
 */
Dmabuf::Dmabuf() {
    init_offset();
}

/**
 * @brief Parse all DMA buffers from kernel's global list
 *
 * Walks the kernel's db_list (or debugfs_list in older kernels) to find
 * all DMA buffers. For each buffer, parses its structure, attachments,
 * and process references.
 */
void Dmabuf::parser_dma_bufs() {
    ulong db_list_addr = 0;
    // Try to find the DMA buffer list symbol
    if (csymbol_exists("db_list")) {
        db_list_addr = csymbol_value("db_list");
        LOGD("Found db_list at address: 0x%lx", db_list_addr);
    } else if (csymbol_exists("debugfs_list")) {
        db_list_addr = csymbol_value("debugfs_list");
        LOGD("Found debugfs_list at address: 0x%lx", db_list_addr);
    } else if(csymbol_exists("dmabuf_list")){
        db_list_addr = csymbol_value("dmabuf_list");
        LOGD("Found dmabuf_list at address: 0x%lx", db_list_addr);
    }

    // Validate the list address
    if (!is_kvaddr(db_list_addr)) {
        LOGE("DMA buffer list doesn't exist in this kernel (invalid address: 0x%lx)", db_list_addr);
        return;
    }
    int buf_count = 0;
    // Walk the list and parse each buffer
    for (const auto& buf_addr : for_each_list(db_list_addr, field_offset(dma_buf, list_node))) {
        LOGD("Parsing dma_buf#%d at address: 0x%lx", buf_count, buf_addr);
        std::shared_ptr<dma_buf> buf_ptr = parser_dma_buf(buf_addr);
        if (buf_ptr == nullptr) {
            LOGE("Failed to parse DMA buffer at 0x%lx, skipping", buf_addr);
            continue;
        }

        // Parse attachments for this buffer
        ulong attachments_head = buf_addr + field_offset(dma_buf, attachments);
        buf_ptr->attachments = parser_attachments(attachments_head);
        // Find which processes reference this buffer
        get_proc_info(buf_ptr);
        buf_list.push_back(buf_ptr);
        // Print unified debug information for parsed dma_buf
        LOGD("=== DMA Buffer Parsed Successfully ===");
        LOGD("Address:      0x%lx", buf_ptr->addr);
        LOGD("Size:         %zu bytes (%s)", buf_ptr->size, csize(buf_ptr->size).c_str());
        LOGD("Priv:         0x%lx", buf_ptr->priv);
        LOGD("Heap:         0x%lx", buf_ptr->heap);
        LOGD("SG Table:     0x%lx", buf_ptr->sg_table);
        LOGD("SG Entries:   %zu", buf_ptr->sgl_list.size());
        LOGD("File:         %s", buf_ptr->file.empty() ? "<none>" : buf_ptr->file.c_str());
        LOGD("Ref Count:    %lu", buf_ptr->f_count);
        LOGD("Name:         %s", buf_ptr->name.empty() ? "<none>" : buf_ptr->name.c_str());
        LOGD("Exporter:     %s", buf_ptr->exp_name.empty() ? "<none>" : buf_ptr->exp_name.c_str());
        LOGD("Operations:   %s", buf_ptr->ops_name.empty() ? "<none>" : buf_ptr->ops_name.c_str());
        LOGD("=====================================");
        buf_count++;
    }
}

/**
 * @brief Parse a single DMA buffer structure
 *
 * Reads and parses a dma_buf structure from kernel memory, extracting
 * size, file information, names, and operations.
 *
 * @param addr Kernel address of dma_buf structure
 * @return Shared pointer to parsed dma_buf, or nullptr on failure
 */
std::shared_ptr<dma_buf> Dmabuf::parser_dma_buf(ulong addr) {
    // Validate address
    if (!is_kvaddr(addr)) {
        LOGE("Invalid kernel address: 0x%lx", addr);
        return nullptr;
    }
    // Read dma_buf structure
    void *dmabuf = read_struct(addr, "dma_buf");
    if (dmabuf == nullptr) {
        LOGE("Failed to read dma_buf structure at 0x%lx", addr);
        return nullptr;
    }
    // Create and populate dma_buf object
    std::shared_ptr<dma_buf> buf_ptr = std::make_shared<dma_buf>();
    buf_ptr->addr = addr;
    buf_ptr->size = INT(dmabuf + field_offset(dma_buf, size));
    buf_ptr->priv = ULONG(dmabuf + field_offset(dma_buf, priv));
    // Parse buffer-specific information (heap, sg_table)
    parser_buffer(buf_ptr);

    // Parse file information
    ulong file_addr = ULONG(dmabuf + field_offset(dma_buf, file));
    if (is_kvaddr(file_addr)) {
        char buf[BUFSIZE];
        buf_ptr->f_count = read_long(file_addr + field_offset(file, f_count), "file_f_count");

        // Get file path
        if (field_offset(file, f_vfsmnt) != -1) {
            get_pathname(file_to_dentry(file_addr), buf, BUFSIZE, 1, file_to_vfsmnt(file_addr));
        } else {
            get_pathname(file_to_dentry(file_addr), buf, BUFSIZE, 1, 0);
        }
        buf_ptr->file = buf;
    }
    // Parse buffer name
    ulong name_addr = ULONG(dmabuf + field_offset(dma_buf, name));
    if (is_kvaddr(name_addr)) {
        buf_ptr->name = read_cstring(name_addr, 64, "dma_buf_name");
    }
    // Parse exporter name
    name_addr = ULONG(dmabuf + field_offset(dma_buf, exp_name));
    if (is_kvaddr(name_addr)) {
        buf_ptr->exp_name = read_cstring(name_addr, 64, "dma_buf_exp_name");
    }
    // Parse operations name
    ulong ops_addr = ULONG(dmabuf + field_offset(dma_buf, ops));
    buf_ptr->ops_name = to_symbol(ops_addr);
    FREEBUF(dmabuf);
    return buf_ptr;
}

/**
 * @brief Parse buffer-specific information (heap, sg_table)
 *
 * Extracts heap and scatter-gather table information from the buffer's
 * private data. Supports both ion_buffer and qcom_sg_buffer structures.
 *
 * @param buf_ptr Pointer to dma_buf structure to populate
 */
void Dmabuf:: parser_buffer(std::shared_ptr<dma_buf> buf_ptr) {
    if (!is_kvaddr(buf_ptr->priv)) {
        LOGE("Invalid priv pointer: 0x%lx", buf_ptr->priv);
        return;
    }
    // Try ion_buffer structure
    if (struct_size(ion_buffer) != -1) {
        buf_ptr->heap = read_pointer(buf_ptr->priv + field_offset(ion_buffer, heap), "heap");
        buf_ptr->sg_table = read_pointer(buf_ptr->priv + field_offset(ion_buffer, sg_table), "sg_table");
        LOGD("Using ion_buffer: heap=0x%lx, sg_table=0x%lx", buf_ptr->heap, buf_ptr->sg_table);
    }
    // Try qcom_sg_buffer structure
    else if (struct_size(qcom_sg_buffer) != -1) {
        buf_ptr->heap = read_pointer(buf_ptr->priv + field_offset(qcom_sg_buffer, heap), "heap");
        buf_ptr->sg_table = buf_ptr->priv + field_offset(qcom_sg_buffer, sg_table);
        LOGD("Using qcom_sg_buffer: heap=0x%lx, sg_table=0x%lx", buf_ptr->heap, buf_ptr->sg_table);
    } else {
        LOGE("No supported buffer structure found");
    }
    // Parse the scatter-gather table
    parser_sg_table(buf_ptr);
}

/**
 * @brief Check if scatterlist entry is a chain pointer
 *
 * In the kernel's scatterlist implementation, bit 0 of page_link indicates
 * whether this entry points to another scatterlist chain.
 *
 * @param page_link Page link value from scatterlist
 * @return true if bit 0 is set (chain pointer)
 */
bool Dmabuf::sg_is_chain(ulong page_link) {
    return (page_link & 0x1) == 1;
}

/**
 * @brief Check if scatterlist entry is the last in the list
 *
 * In the kernel's scatterlist implementation, bit 1 of page_link indicates
 * whether this is the last entry in the list.
 *
 * @param page_link Page link value from scatterlist
 * @return true if bit 1 is set (last entry)
 */
bool Dmabuf::sg_is_last(ulong page_link) {
    return (page_link & 0x2) == 2;
}

/**
 * @brief Extract chain pointer from page_link
 *
 * Clears the low 2 bits (flags) to get the actual pointer address.
 *
 * @param page_link Page link value with chain pointer
 * @return Address of next scatterlist chain
 */
ulong Dmabuf::sg_chain_ptr(ulong page_link) {
    return page_link & ~0x3;
}

/**
 * @brief Get next scatterlist entry
 *
 * Determines the next scatterlist entry based on the page_link flags:
 * - If last: return 0
 * - If chain: follow chain pointer
 * - Otherwise: next entry is immediately after current
 *
 * @param sgl_addr Current scatterlist address
 * @param page_link Current page_link value
 * @return Address of next scatterlist entry, or 0 if last
 */
ulong Dmabuf::sg_next(ulong sgl_addr, ulong page_link) {
    if (sg_is_last(page_link)) {
        return 0;
    }
    if (sg_is_chain(page_link)) {
        return sg_chain_ptr(page_link);
    } else {
        return sgl_addr + struct_size(scatterlist);
    }
}

/**
 * @brief Parse scatter-gather table for a DMA buffer
 *
 * Walks the scatter-gather list to extract all memory segments that
 * comprise the DMA buffer. Handles chained scatterlists and detects loops.
 *
 * @param buf_ptr Buffer whose sg_table to parse
 */
void Dmabuf::parser_sg_table(std::shared_ptr<dma_buf> buf_ptr) {
    if (!is_kvaddr(buf_ptr->sg_table)) {
        LOGE("Invalid sg_table address: 0x%lx", buf_ptr->sg_table);
        return;
    }

    // Read scatter-gather table header
    ulong sgl_addr = read_pointer(buf_ptr->sg_table + field_offset(sg_table, sgl), "sgl");
    uint32_t cnt = read_uint(buf_ptr->sg_table + field_offset(sg_table, nents), "nents");
    LOGD("sg_table at 0x%lx: scatterlist at 0x%lx, nents=%u", buf_ptr->sg_table, sgl_addr, cnt);
    // Sanity check on entry count
    if (cnt >= std::numeric_limits<unsigned int>::max()) {
        LOGE("Invalid nents value: %u", cnt);
        return;
    }
    // Track visited addresses to detect loops
    std::set<ulong> sgl_map;
    int parsed_count = 0;
    // Walk the scatterlist
    while (is_kvaddr(sgl_addr) && cnt) {
        cnt -= 1;
        // Read scatterlist entry
        void *sgl_buf = read_struct(sgl_addr, "scatterlist");
        if (sgl_buf == nullptr) {
            LOGE("Failed to read scatterlist at 0x%lx", sgl_addr);
            break;
        }
        // Parse scatterlist entry
        std::shared_ptr<scatterlist> sgl_ptr = std::make_shared<scatterlist>();
        sgl_ptr->addr = sgl_addr;
        sgl_ptr->offset = UINT(sgl_buf + field_offset(scatterlist, offset));
        sgl_ptr->length = UINT(sgl_buf + field_offset(scatterlist, length));
        sgl_ptr->dma_address = ULONG(sgl_buf + field_offset(scatterlist, dma_address));
        sgl_ptr->dma_length = UINT(sgl_buf + field_offset(scatterlist, dma_length));
        sgl_ptr->page_link = ULONG(sgl_buf + field_offset(scatterlist, page_link));
        FREEBUF(sgl_buf);
        LOGD("  [%d] scatterlist=0x%lx, page=0x%lx, offset=%u, length=%u",
             parsed_count, sgl_addr, sgl_ptr->page_link & ~0x3, sgl_ptr->offset, sgl_ptr->length);
        // Check for termination
        if (sgl_ptr->page_link == 0) {
            LOGD("Reached end of scatterlist (page_link=0)");
            break;
        }
        // Check for loops
        if (sgl_map.find(sgl_addr) != sgl_map.end()) {
            LOGE("Detected loop in scatterlist at 0x%lx", sgl_addr);
            break;
        }
        sgl_map.insert(sgl_addr);
        // Get next entry
        sgl_addr = sg_next(sgl_addr, sgl_ptr->page_link);
        buf_ptr->sgl_list.push_back(sgl_ptr);
        parsed_count++;
    }
    sgl_map.clear();
}

/**
 * @brief Scan all processes to find DMA buffer file descriptors
 *
 * Iterates through all processes and their open file descriptors to identify
 * which ones reference DMA buffers (by checking if f_op == dma_buf_fops).
 * Builds a map of processes to their DMA buffer file descriptors.
 */
void Dmabuf::get_dmabuf_from_proc() {
    // Check if dma_buf_fops symbol exists
    if (!csymbol_exists("dma_buf_fops")) {
        LOGE("dma_buf_fops doesn't exist in this kernel");
        return;
    }
    ulong dma_buf_fops = csymbol_value("dma_buf_fops");
    LOGD("dma_buf_fops at address: 0x%lx", dma_buf_fops);

    // Iterate through all processes
    for (ulong task_addr : for_each_process()) {
        struct task_context *tc = task_to_context(task_addr);
        if (!tc) {
            continue;
        }
        // Build map of dma_buf addresses to file descriptors for this process
        std::unordered_map<ulong, int> map;
        std::vector<ulong> files = for_each_task_files(tc);
        for (size_t i = 0; i < files.size(); i++) {
            if (!is_kvaddr(files[i])) {
                continue;
            }
            // Check if this file is a DMA buffer
            ulong f_op = read_pointer(files[i] + field_offset(file, f_op), "f_op");
            if (f_op != dma_buf_fops) {
                continue;
            }
            // Get the dma_buf address from private_data
            ulong priv = read_pointer(files[i] + field_offset(file, private_data), "private_data");
            if (!is_kvaddr(priv)) {
                continue;
            }
            map[priv] = i;
        }
        // If this process has DMA buffer references, add it to the list
        if (map.size() > 0) {
            std::shared_ptr<proc_info> proc_ptr = std::make_shared<proc_info>();
            proc_ptr->tc = tc;
            proc_ptr->fd_map = map;
            proc_list.push_back(proc_ptr);
            LOGD("Process %d (%s) has %zu dma_buf fds", tc->pid, tc->comm, map.size());
        }
    }
}

/**
 * @brief Find process information for a DMA buffer
 *
 * Searches through the process list to find which processes have file
 * descriptors referencing this specific DMA buffer.
 *
 * @param buf_ptr DMA buffer to find process info for
 */
void Dmabuf::get_proc_info(std::shared_ptr<dma_buf> buf_ptr) {
    int proc_count = 0;
    for (const auto& proc_ptr : proc_list) {
        for (const auto& pair : proc_ptr->fd_map) {
            if (buf_ptr->addr == pair.first) {
                buf_ptr->procs.push_back(proc_ptr);
                LOGD("Process %d (%s) references buffer 0x%lx via fd %d",
                     proc_ptr->tc->pid, proc_ptr->tc->comm, buf_ptr->addr, pair.second);
                proc_count++;
            }
        }
    }
}

/**
 * @brief Parse all attachments from a list
 *
 * Walks the attachment list for a DMA buffer and parses each attachment,
 * including device and driver information.
 *
 * @param list_head Address of attachment list head
 * @return Vector of parsed attachments
 */
std::vector<std::shared_ptr<attachment>> Dmabuf::parser_attachments(ulong list_head) {
    std::vector<std::shared_ptr<attachment>> res;
    int offset = field_offset(dma_buf_attachment, node);
    int attach_count = 0;

    for (const auto& attach_addr : for_each_list(list_head, offset)) {
        LOGD("  Parsing attachment at 0x%lx", attach_addr);
        void *buf = read_struct(attach_addr, "dma_buf_attachment");
        if (buf == nullptr) {
            LOGE("  Failed to read attachment at 0x%lx", attach_addr);
            continue;
        }
        // Parse attachment structure
        std::shared_ptr<attachment> attach_ptr = std::make_shared<attachment>();
        attach_ptr->addr = attach_addr;
        attach_ptr->sg_table = ULONG(buf + field_offset(dma_buf_attachment, sgt));
        attach_ptr->importer_priv = ULONG(buf + field_offset(dma_buf_attachment, importer_priv));
        attach_ptr->priv = ULONG(buf + field_offset(dma_buf_attachment, priv));
        attach_ptr->dma_map_attrs = ULONG(buf + field_offset(dma_buf_attachment, dma_map_attrs));
        attach_ptr->dir = (enum dma_data_direction)INT(buf + field_offset(dma_buf_attachment, dir));

        // Parse device information
        ulong dev_addr = ULONG(buf + field_offset(dma_buf_attachment, dev));
        if (is_kvaddr(dev_addr)) {
            void *device_buf = read_struct(dev_addr, "device");
            if (device_buf != nullptr) {
                // Get device name
                ulong addr = ULONG(device_buf + field_offset(device, kobj) + field_offset(kobject, name));
                if (is_kvaddr(addr)) {
                    attach_ptr->device_name = read_cstring(addr, 64, "device_name");
                }

                // Get driver name
                ulong driver_addr = ULONG(device_buf + field_offset(device, driver));
                if (is_kvaddr(driver_addr)) {
                    void *driver_buf = read_struct(driver_addr, "device_driver");
                    addr = ULONG(driver_buf + field_offset(device_driver, name));
                    if (is_kvaddr(addr)) {
                        attach_ptr->driver_name = read_cstring(addr, 64, "driver_name");
                    }
                    FREEBUF(driver_buf);
                }
                FREEBUF(device_buf);
            }
        }
        LOGD("  Attachment: device=%s, driver=%s, dir=%d",
             attach_ptr->device_name.c_str(), attach_ptr->driver_name.c_str(), attach_ptr->dir);

        FREEBUF(buf);
        res.push_back(attach_ptr);
        attach_count++;
    }
    return res;
}

/**
 * @brief Print comprehensive information for all DMA buffers
 *
 * Displays detailed information about all DMA buffers including size,
 * reference count, exporter name, buffer name, and attached devices.
 * Buffers are sorted by size (largest first).
 */
void Dmabuf::print_dma_buf_info() {
    if (buf_list.size() == 0) {
        PRINT("No DMA buffers to display");
        return;
    }
    // Sort buffers by size (descending)
    std::sort(buf_list.begin(), buf_list.end(),
              [&](const std::shared_ptr<dma_buf>& a, const std::shared_ptr<dma_buf>& b) {
                  return a->size > b->size;
              });
    std::ostringstream oss;
    oss << "Dma-buf Objects: \n";
    for (const auto& dma_buf : buf_list) {
        oss << std::left << "  "
            << std::left << std::setw(18) << "dma_buf" << " "
            << std::left << std::setw(10) << "Size" << " "
            << std::left << std::setw(5) << "Ref" << " "
            << std::left << std::setw(25) << "Exp_name" << " "
            << std::left << std::setw(25) << "Name" << "\n";
        oss << std::left << "  "
            << "0x" << std::hex << std::setw(16) << std::setfill('0') << dma_buf->addr << std::setfill(' ') << " "
            << std::left << std::dec << std::setw(10) << csize(dma_buf->size) << " "
            << std::left << std::setw(5) << dma_buf->f_count << " "
            << std::left << std::setw(25) << dma_buf->exp_name << " "
            << std::left << std::setw(25) << dma_buf->name << "\n";
        oss << "        Attached Devices: \n";
        for (const auto& attach : dma_buf->attachments) {
            oss << std::left << "        " << attach->device_name << "\n";
        }
        oss << "  Total " << dma_buf->attachments.size() << " devices attached \n\n";
    }
    PRINT("%s \n", oss.str().c_str());
}

/**
 * @brief Print concise list of all DMA buffers
 *
 * Displays a summary table of all DMA buffers with address, reference count,
 * operations, exporter name, and size. Includes total size calculation.
 */
void Dmabuf::print_dma_buf_list() {
    int index = 1;
    uint64_t total_size = 0;

    if (buf_list.size() == 0) {
        PRINT("No DMA buffers to display");
        return;
    }
    // Sort buffers by size (descending)
    std::sort(buf_list.begin(), buf_list.end(),
              [&](const std::shared_ptr<dma_buf>& a, const std::shared_ptr<dma_buf>& b) {
                  return a->size > b->size;
              });
    PRINT("=======================================================================================\n");
    std::ostringstream oss;
    for (const auto& dma_buf : buf_list) {
        total_size += dma_buf->size;
        oss << "[" << std::setw(3) << std::setfill('0') << std::dec << std::right << index << "]"
            << "dma_buf:" << std::hex << std::setfill(' ') << dma_buf->addr << " "
            << "ref:" << std::left << std::dec << std::setw(2) << dma_buf->f_count << " "
            << "priv:" << std::left << std::hex << dma_buf->priv << " "
            << "ops::" << std::left << std::setw(12) << dma_buf->ops_name << " ["
            << std::left << dma_buf->exp_name << "] "
            << "size:" << std::left << std::setw(9) << csize(dma_buf->size)
            << "\n";
        index += 1;
    }
    PRINT("%s \n", oss.str().c_str());
    PRINT("=======================================================================================\n");
    PRINT("Total size:%s\n", csize(total_size).c_str());
    PRINT(" \n");
}

/**
 * @brief Print attachment information for a buffer
 *
 * Displays all device attachments for a DMA buffer including direction,
 * device name, and driver name.
 *
 * @param buf_ptr Buffer whose attachments to print
 */
void Dmabuf::print_attachment(std::shared_ptr<dma_buf> buf_ptr) {
    std::ostringstream oss;
    for (const auto& attach : buf_ptr->attachments) {
        oss << "        dma_buf_attachment:" << std::hex << std::setfill(' ') << attach->addr << " "
            << "dir:" << std::left << directions[attach->dir] << " "
            << "priv:" << std::left << std::hex << attach->priv << " "
            << "device:[" << std::left << attach->device_name << "] "
            << "driver:[" << std::left << attach->driver_name << "]"
            << "\n";
    }
    PRINT("%s \n", oss.str().c_str());
}

/**
 * @brief Print process information for a buffer
 *
 * Displays all processes that have file descriptors referencing this buffer.
 *
 * @param buf_ptr Buffer whose process info to print
 */
void Dmabuf::print_proc_info(std::shared_ptr<dma_buf> buf_ptr) {
    std::ostringstream oss;
    for (const auto& proc : buf_ptr->procs) {
        oss << "        pid:" << std::dec << std::left << std::setw(5) << proc->tc->pid << " "
            << "[" << std::left << proc->tc->comm << "] "
            << "fd:" << std::left << std::dec << proc->fd_map[buf_ptr->addr] << " "
            << "\n";
    }
    PRINT("%s \n", oss.str().c_str());
}

/**
 * @brief Print scatter-gather table for a buffer
 *
 * Displays all scatterlist entries showing physical pages, offsets,
 * lengths, and DMA addresses.
 *
 * @param buf_ptr Buffer whose sg_table to print
 */
void Dmabuf::print_sg_table(std::shared_ptr<dma_buf> buf_ptr) {
    std::ostringstream oss;
    for (const auto& sgl_ptr : buf_ptr->sgl_list) {
        oss << "        scatterlist:" << std::hex << std::left << sgl_ptr->addr << " "
            << "page:" << std::left << std::hex << (sgl_ptr->page_link & ~0x3) << " "
            << "offset:" << std::left << std::dec << sgl_ptr->offset << " "
            << "length:" << std::left << std::dec << csize(sgl_ptr->length) << " "
            << "dma_address:" << std::left << std::hex << sgl_ptr->dma_address << " "
            << "dma_length:" << std::left << std::dec << csize(sgl_ptr->dma_length)
            << "\n";
    }
    PRINT("%s \n", oss.str().c_str());
}

/**
 * @brief Print detailed information for a DMA buffer
 *
 * Displays complete information including buffer metadata, attachments,
 * process references, and scatter-gather table.
 *
 * @param buf_ptr Pointer to the buffer to print
 */
void Dmabuf::print_dma_buf(std::shared_ptr<dma_buf> buf_ptr) {
    std::ostringstream oss;
    oss << "dma_buf:" << std::hex << std::setfill(' ') << buf_ptr->addr << " "
        << "ref:" << std::left << std::dec << std::setw(2) << buf_ptr->f_count << " "
        << "priv:" << std::left << std::hex << buf_ptr->priv << " "
        << " [" << std::left << buf_ptr->exp_name << "] "
        << "sg_table:" << std::left << std::hex << buf_ptr->sg_table << " "
        << "size:" << std::left << std::setw(9) << csize(buf_ptr->size);
    PRINT("%s \n", oss.str().c_str());

    print_attachment(buf_ptr);
    print_proc_info(buf_ptr);
    print_sg_table(buf_ptr);
    PRINT(" \n");
}

/**
 * @brief Print detailed information for a specific DMA buffer by address
 *
 * Searches for a buffer with the given address and displays its information.
 *
 * @param addr Hexadecimal address string of the buffer
 */
void Dmabuf::print_dma_buf(std::string addr) {
    unsigned long number = std::stoul(addr, nullptr, 16);
    if (number <= 0) {
        LOGE("Invalid address: %s", addr.c_str());
        return;
    }
    for (const auto& dma_buf : buf_list) {
        if (dma_buf->addr == number) {
            LOGD("Found buffer at 0x%lx", number);
            print_dma_buf(dma_buf);
            return;
        }
    }
}

/**
 * @brief Save DMA buffer contents to a file
 *
 * Reads the physical memory pages referenced by the buffer's scatter-gather
 * list and writes them to a file in the current output directory.
 *
 * @param addr Hexadecimal address string of the buffer
 */
void Dmabuf::save_dma_buf(std::string addr) {
    unsigned long number = std::stoul(addr, nullptr, 16);
    if (number <= 0) {
        LOGE("Invalid address: %s", addr.c_str());
        return;
    }

    for (const auto& buf_ptr : buf_list) {
        if (buf_ptr->addr == number) {
            LOGD("Found buffer at 0x%lx, saving to file", number);
            // Generate output filename
            std::stringstream ss = get_curpath();
            ss << "/dma_buf@" << std::hex << buf_ptr->addr << ".data";
            FILE* dma_file = fopen(ss.str().c_str(), "wb");
            if (!dma_file) {
                LOGE("Failed to open file: %s", ss.str().c_str());
                return;
            }
            // Write each scatterlist segment to file
            size_t total_written = 0;
            for (const auto& sgl_ptr : buf_ptr->sgl_list) {
                ulong page = sgl_ptr->page_link & ~0x3;
                physaddr_t paddr = page_to_phy(page) + sgl_ptr->offset;
                size_t len = sgl_ptr->length;
                LOGD("  Writing segment: page=0x%lx, paddr=0x%lx, len=%zu", page, paddr, len);
                void* buf = read_memory(paddr, len, "dmabuf", false);
                fwrite(buf, len, 1, dma_file);
                FREEBUF(buf);
                total_written += len;
            }
            fclose(dma_file);
            PRINT("Successfully saved %zu bytes to %s", total_written, ss.str().c_str());
            return;
        }
    }
}

/**
 * @brief Print DMA buffer usage summary for all processes
 *
 * Displays a table showing each process's PID, command name, number of
 * DMA buffer file descriptors, and total buffer size.
 */
void Dmabuf::print_procs() {
    std::ostringstream oss;
    oss << std::left << std::setw(5) << "PID" << " "
        << std::left << std::setw(20) << "Comm" << " "
        << std::left << std::setw(8) << "buf_cnt" << " "
        << std::left << "total_size"
        << "\n";
    for (const auto& proc_ptr : proc_list) {
        // Calculate total buffer size for this process
        size_t total_size = 0;
        for (const auto& pair : proc_ptr->fd_map) {
            for (const auto& buf_ptr : buf_list) {
                if (pair.first == buf_ptr->addr) {
                    total_size += buf_ptr->size;
                }
            }
        }
        oss << std::left << std::setw(5) << proc_ptr->tc->pid << " "
            << std::left << std::setw(20) << proc_ptr->tc->comm << " "
            << std::left << std::setw(8) << proc_ptr->fd_map.size() << " "
            << std::left << csize(total_size)
            << "\n";
    }
    PRINT("%s \n", oss.str().c_str());
}

/**
 * @brief Print DMA buffers used by a specific process
 *
 * Displays detailed information for all DMA buffers referenced by the
 * specified process ID.
 *
 * @param pid Process ID to query
 */
void Dmabuf::print_proc(ulong pid) {
    for (const auto& proc_ptr : proc_list) {
        if (proc_ptr->tc->pid != pid) {
            continue;
        }
        for (const auto& pair : proc_ptr->fd_map) {
            for (const auto& buf_ptr : buf_list) {
                if (pair.first == buf_ptr->addr) {
                    print_dma_buf(buf_ptr);
                }
            }
        }
    }
}

/**
 * @brief Print page allocation stack traces for all DMA buffers
 *
 * Iterates through all DMA buffers and displays the allocation stack trace
 * for the first valid page of each buffer. This provides a quick overview
 * of where all buffers were allocated from.
 */
void Dmabuf::print_all_dmabuf_page_stacks() {
    if (!is_enable_pageowner()) {
        PRINT("Page owner is not enabled in kernel. Cannot display stack traces.\n");
        return;
    }
    if (buf_list.empty()) {
        PRINT("No DMA buffers found.\n");
        return;
    }

    PRINT("================================================================================\n"
          "            ALL DMABUF PAGE ALLOCATION STACKS\n"
          "================================================================================\n"
          "Total DMA Buffers: %zu\n"
          "================================================================================\n\n", buf_list.size());

    size_t buf_index = 0, buffers_with_stack = 0, buffers_without_stack = 0;

    for (const auto& buf_ptr : buf_list) {
        PRINT("[%zu/%zu] dma_buf:0x%lx | Size:%s | Exporter:[%s] | Ref:%lu\n",
              ++buf_index, buf_list.size(), buf_ptr->addr, csize(buf_ptr->size).c_str(),
              buf_ptr->exp_name.c_str(), buf_ptr->f_count);

        if (buf_ptr->sgl_list.empty()) {
            PRINT("No scatterlist entries\n\n");
            buffers_without_stack++;
            continue;
        }

        // Find first valid page owner (allocated or freed)
        std::shared_ptr<page_owner> owner_ptr = nullptr;
        for (const auto& sgl_ptr : buf_ptr->sgl_list) {
            if (!is_kvaddr(sgl_ptr->addr) || sgl_ptr->page_link == 0) continue;

            ulong page = sgl_ptr->page_link & ~0x3UL;
            if (!is_kvaddr(page)) continue;

            ulong pfn = page_to_pfn(page);
            if (pfn > 0) {
                owner_ptr = parse_page_owner_by_pfn(pfn);
                if (owner_ptr) break;
            }
        }

        if (!owner_ptr) {
            PRINT("No page owner information available\n\n");
            buffers_without_stack++;
            continue;
        }

        buffers_with_stack++;

        // Determine current status and prepare variables
        bool is_allocated = is_page_allocated(owner_ptr);
        const char *status = is_allocated ? "ALLOCATED" : "FREED";
        uint handle = is_allocated ? owner_ptr->handle : owner_ptr->free_handle;
        ulong timestamp = is_allocated ? owner_ptr->ts_nsec : owner_ptr->free_ts_nsec;

        // Print basic info
        struct task_context *tc = owner_ptr->comm.empty() ? pid_to_context(owner_ptr->pid) : nullptr;
        const char *comm = owner_ptr->comm.empty() ? (tc ? tc->comm : "unknown") : owner_ptr->comm.c_str();

        PRINT("Status: %s, PID:%zu [%s] | Order:%u (%s) ", status, owner_ptr->pid, comm, owner_ptr->order,
              csize((1UL << owner_ptr->order) * page_size).c_str());

        // Print stack header
        if (timestamp > 0)
            PRINT(" Time: %s", formatTimestamp(timestamp).c_str());
        if (is_allocated && owner_ptr->gfp_mask > 0)
            PRINT(" GFP:0x%x", owner_ptr->gfp_mask);
        PRINT(":\n");

        // Print stack content
        try {
            std::shared_ptr<stack_record_t> record_ptr = get_stack_record(handle);
            if (record_ptr) {
                std::istringstream iss(get_call_stack(record_ptr));
                std::string line;
                while (std::getline(iss, line))
                    if (!line.empty()) PRINT("    %s\n", line.c_str());
            } else {
                PRINT("    [Stack trace not available]\n");
            }
        } catch (...) {
            PRINT("    [Stack trace not available]\n");
        }
        PRINT("\n");
    }

    PRINT("================================================================================\n"
          "Summary: %zu buffers analyzed, %zu with stack info, %zu without\n"
          "================================================================================\n",
          buf_list.size(), buffers_with_stack, buffers_without_stack);
}

#pragma GCC diagnostic pop
