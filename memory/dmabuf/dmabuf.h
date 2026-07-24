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

#ifndef DMABUF_DEFS_H_
#define DMABUF_DEFS_H_

#include "plugin.h"

/**
 * @enum dma_data_direction
 * @brief DMA data transfer direction enumeration
 *
 * Defines the direction of DMA data transfer between device and memory.
 * These values match the kernel's dma_data_direction enum.
 */
enum dma_data_direction {
    DMA_BIDIRECTIONAL = 0,  /**< Data can be transferred in both directions */
    DMA_TO_DEVICE = 1,      /**< Data is transferred from memory to device */
    DMA_FROM_DEVICE = 2,    /**< Data is transferred from device to memory */
    DMA_NONE = 3,           /**< No data transfer direction specified */
};

/**
 * @struct attachment
 * @brief Represents a DMA buffer attachment to a device
 *
 * Contains information about how a DMA buffer is attached to a specific device,
 * including the scatter-gather table, DMA direction, and device/driver details.
 */
struct attachment {
    ulong addr;                         /**< Address of dma_buf_attachment structure */
    ulong sg_table;                     /**< Address of scatter-gather table */
    enum dma_data_direction dir;        /**< DMA transfer direction */
    ulong importer_priv;                /**< Importer's private data */
    ulong priv;                         /**< Attachment private data */
    ulong dma_map_attrs;                /**< DMA mapping attributes */
    std::string device_name;            /**< Name of the attached device */
    std::string driver_name;            /**< Name of the device driver */
};

/**
 * @struct proc_info
 * @brief Process information for DMA buffer tracking
 *
 * Tracks which processes have file descriptors referencing specific DMA buffers.
 */
struct proc_info {
    struct task_context *tc;                    /**< Task context pointer */
    std::unordered_map<ulong, int> fd_map;      /**< Map of dma_buf address to file descriptor */
};

/**
 * @struct scatterlist
 * @brief Scatter-gather list entry
 *
 * Represents a single entry in a scatter-gather list, describing a contiguous
 * physical memory region that is part of a DMA buffer.
 */
struct scatterlist {
    ulong addr;                 /**< Address of scatterlist structure */
    ulong page_link;            /**< Page pointer with chain/last flags in low bits */
    unsigned int offset;        /**< Offset within the page */
    unsigned int length;        /**< Length of this segment */
    size_t dma_address;         /**< DMA address for this segment */
    unsigned int dma_length;    /**< DMA length for this segment */
};

/**
 * @struct dma_buf
 * @brief Complete DMA buffer information
 *
 * Aggregates all information about a DMA buffer including its memory layout,
 * attachments, process references, and metadata.
 */
struct dma_buf {
    ulong addr;                                         /**< Address of dma_buf structure */
    ulong heap;                                         /**< Heap this buffer was allocated from */
    ulong sg_table;                                     /**< Address of scatter-gather table */
    std::vector<std::shared_ptr<scatterlist>> sgl_list; /**< List of scatter-gather entries */
    size_t size;                                        /**< Total size of the buffer */
    std::string file;                                   /**< Associated file path */
    ulong f_count;                                      /**< File reference count */
    std::vector<std::shared_ptr<attachment>> attachments; /**< List of device attachments */
    std::string ops_name;                               /**< Buffer operations name */
    std::string exp_name;                               /**< Exporter name */
    std::string name;                                   /**< Buffer name */
    ulong priv;                                         /**< Private data pointer */
    std::vector<std::shared_ptr<proc_info>> procs;      /**< Processes using this buffer */
};

/**
 * @class Dmabuf
 * @brief DMA buffer parser and analyzer
 *
 * This class provides comprehensive parsing and analysis of DMA buffers from
 * kernel crash dumps. It can:
 * - Parse DMA buffer structures from kernel memory
 * - Track buffer attachments to devices
 * - Identify processes using buffers
 * - Analyze scatter-gather tables
 * - Export buffer contents to files
 * - Generate various reports and statistics
 */
class Dmabuf : public ParserPlugin {
private:
    /** @brief Direction names for pretty printing */
    std::vector<std::string> directions = {
        "DMA_BIDIRECTIONAL",
        "DMA_TO_DEVICE",
        "DMA_FROM_DEVICE",
        "DMA_NONE"
    };

    /**
     * @brief Parse a single DMA buffer structure
     * @param addr Kernel address of dma_buf structure
     * @return Shared pointer to parsed dma_buf, or nullptr on failure
     */
    std::shared_ptr<dma_buf> parser_dma_buf(ulong addr);

    /**
     * @brief Parse buffer-specific information (heap, sg_table)
     * @param buf_ptr Pointer to dma_buf structure to populate
     */
    void parser_buffer(std::shared_ptr<dma_buf> buf_ptr);

    /**
     * @brief Check if scatterlist entry is a chain pointer
     * @param page_link Page link value from scatterlist
     * @return true if bit 0 is set (chain pointer)
     */
    bool sg_is_chain(ulong page_link);

    /**
     * @brief Check if scatterlist entry is the last in the list
     * @param page_link Page link value from scatterlist
     * @return true if bit 1 is set (last entry)
     */
    bool sg_is_last(ulong page_link);

    /**
     * @brief Extract chain pointer from page_link
     * @param page_link Page link value with chain pointer
     * @return Address of next scatterlist chain
     */
    ulong sg_chain_ptr(ulong page_link);

    /**
     * @brief Get next scatterlist entry
     * @param sgl_addr Current scatterlist address
     * @param page_link Current page_link value
     * @return Address of next scatterlist entry, or 0 if last
     */
    ulong sg_next(ulong sgl_addr, ulong page_link);

    /**
     * @brief Find process information for a DMA buffer
     * @param buf_ptr DMA buffer to find process info for
     */
    void get_proc_info(std::shared_ptr<dma_buf> buf_ptr);

    /**
     * @brief Parse all attachments from a list
     * @param list_head Address of attachment list head
     * @return Vector of parsed attachments
     */
    std::vector<std::shared_ptr<attachment>> parser_attachments(ulong list_head);

    /**
     * @brief Print attachment information for a buffer
     * @param buf_ptr Buffer whose attachments to print
     */
    void print_attachment(std::shared_ptr<dma_buf> buf_ptr);

    /**
     * @brief Print process information for a buffer
     * @param buf_ptr Buffer whose process info to print
     */
    void print_proc_info(std::shared_ptr<dma_buf> buf_ptr);

    /**
     * @brief Print scatter-gather table for a buffer
     * @param buf_ptr Buffer whose sg_table to print
     */
    void print_sg_table(std::shared_ptr<dma_buf> buf_ptr);

public:
    /** @brief List of all processes with DMA buffer references */
    std::vector<std::shared_ptr<proc_info>> proc_list;

    /** @brief List of all parsed DMA buffers */
    std::vector<std::shared_ptr<dma_buf>> buf_list;

    /**
     * @brief Constructor - initializes the Dmabuf parser
     */
    Dmabuf();

    /**
     * @brief Main command handler (not used for this class)
     */
    void cmd_main(void) override;

    /**
     * @brief Initialize kernel structure offsets
     *
     * Registers all necessary kernel structure definitions for parsing
     * DMA buffers, attachments, scatter-gather tables, etc.
     */
    void init_offset(void) override;

    /**
     * @brief Initialize command metadata (not used for this class)
     */
    void init_command(void) override;

    /**
     * @brief Print detailed information for a specific DMA buffer
     * @param addr Hexadecimal address string of the buffer
     */
    void print_dma_buf(std::string addr);

    /**
     * @brief Print concise list of all DMA buffers
     */
    void print_dma_buf_list();

    /**
     * @brief Print comprehensive information for all DMA buffers
     */
    void print_dma_buf_info();

    /**
     * @brief Print DMA buffer usage summary for all processes
     */
    void print_procs();

    /**
     * @brief Print DMA buffers used by a specific process
     * @param pid Process ID to query
     */
    void print_proc(ulong pid);

    /**
     * @brief Save DMA buffer contents to a file
     * @param addr Hexadecimal address string of the buffer
     */
    void save_dma_buf(std::string addr);

    /**
     * @brief Print detailed information for a DMA buffer
     * @param buf_ptr Pointer to the buffer to print
     */
    void print_dma_buf(std::shared_ptr<dma_buf> buf_ptr);

    /**
     * @brief Scan all processes to find DMA buffer file descriptors
     *
     * Iterates through all processes and identifies which ones have
     * file descriptors pointing to DMA buffers.
     */
    void get_dmabuf_from_proc();

    /**
     * @brief Parse scatter-gather table for a DMA buffer
     * @param buf_ptr Buffer whose sg_table to parse
     */
    void parser_sg_table(std::shared_ptr<dma_buf> buf_ptr);

    /**
     * @brief Parse all DMA buffers from kernel's global list
     *
     * Walks the kernel's db_list or debugfs_list to find and parse
     * all DMA buffers in the system.
     */
    void parser_dma_bufs();

    /**
     * @brief Print page allocation stack traces for all DMA buffers
     *
     * Iterates through all DMA buffers and displays the allocation stack trace
     * for the first valid page of each buffer. This provides a quick overview
     * of where all buffers were allocated from.
     */
    void print_all_dmabuf_page_stacks();
};

#endif // DMABUF_DEFS_H_
