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

#ifndef SF_DEFS_H_
#define SF_DEFS_H_

#include "plugin.h"
#include "utils/utask.h"

// PixelFormat.aidl
enum PixelFormat {
    UNSPECIFIED = 0,
    RGBA_8888 = 0x1,
    RGBX_8888 = 0x2,
    RGB_888 = 0x3,
    RGB_565 = 0x4,
    BGRA_8888 = 0x5,
    YCBCR_422_SP = 0x10,
    YCRCB_420_SP = 0x11,
    YCBCR_422_I = 0x14,
    RGBA_FP16 = 0x16,
    RAW16 = 0x20,
    BLOB = 0x21,
    IMPLEMENTATION_DEFINED = 0x22,
    YCBCR_420_888 = 0x23,
    RAW_OPAQUE = 0x24,
    RAW10 = 0x25,
    RAW12 = 0x26,
    RGBA_1010102 = 0x2B,
    Y8 = 0x20203859,
    Y16 = 0x20363159,
    YV12 = 0x32315659,
    DEPTH_16 = 0x30,
    DEPTH_24 = 0x31,
    DEPTH_24_STENCIL_8 = 0x32,
    DEPTH_32F = 0x33,
    DEPTH_32F_STENCIL_8 = 0x34,
    STENCIL_8 = 0x35,
    YCBCR_P010 = 0x36,
    HSV_888 = 0x37,
    R_8 = 0x38,
    R_16_UINT = 0x39,
    RG_1616_UINT = 0x3a,
    RGBA_10101010 = 0x3b,
};

// ptype /o native_handle_t
struct native_handle_t {
    int version;
    int numFds;
    int numInts;
};

struct private_handle_t {
    native_handle_t nht;
    int fd;
    int fd_metadata;
    int magic;
    int flags;
    int width;
    int height;
    int unaligned_width;
    int unaligned_height;
    int format;
    int buffer_type;
    unsigned int layer_count;
    uint64_t id;
    uint64_t usage;
    unsigned int size;
    unsigned int offset;
    unsigned int offset_metadata;
    uint64_t base;
    uint64_t base_metadata;
    uint64_t gpuaddr;
    unsigned int reserved_size;
    unsigned int custom_content_md_reserved_size;
    unsigned int linear_size;
    int ubwcp_format;
} __attribute__ ((__packed__));

// ptype /o 'android::GraphicBufferAllocator::alloc_rec_t'
struct alloc_rec_t {
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    int format;
    uint32_t layerCount;
    uint64_t usage;
    // uint64_t size;
};

/*
ptype /o  android::VectorImpl
*/
struct KeyedVector_64_t {
    uint64_t vtpr;
    uint64_t mStorage_addr;
    uint64_t mCount;
    uint32_t mFlags;
    uint64_t mItemSize;
};

struct KeyedVector_32_t {
    uint32_t vtpr;
    uint32_t mStorage_addr;
    uint32_t mCount;
    uint32_t mFlags;
    uint32_t mItemSize;
};

//  ptype /o 'snapalloc::SnapHandleInternal::FdPair'
struct FdPair {
  int fd;
  int fd_metadata;
};

//  ptype /o 'snapalloc::SnapHandleInternal::SnapHandleProperties'
struct SnapHandleProperties {
    uint32_t view;
    int flags;
    int aligned_width_in_bytes;
    int aligned_width_in_pixels;
    int aligned_height;
    int unaligned_width;
    int unaligned_height;
    int format;
    int buffer_type;
    unsigned int layer_count;
    uint64_t id;
    uint64_t usage;
    unsigned int size;
    char padding[4];
    uint64_t base;
    uint64_t base_metadata;
    uint64_t fb_id;
    unsigned int reserved_size;
    unsigned int custom_content_md_reserved_size;
    uint64_t pixel_format_modifier;
    uint64_t reserved_region_base;
    uint64_t custom_content_md_region_base;
    unsigned int flush;
    int lock_count;
    int ref_count;
    char padding1[4];
} __attribute__ ((__packed__));

struct SnapHandleInternal {
    native_handle_t handle;
    FdPair fdpair;
    char padding1[4];
    SnapHandleProperties shp;
};

struct sf_offset_table {
    int GraphicBufferAllocator_mAllocator;
    int Gralloc4Allocator_mMapper;
    int Gralloc4Mapper_mMapper;
    int BsQtiMapper_mImpl;
    int IQtiMapper_buf_mgr_;
    int BufferManager_handles_map_;
    size_t mItemSize;
    int SnapAllocCore_handles_map_;
    int SnapMetadata_name;
    int MetaData_t_name;
};

class SF : public ParserPlugin {
protected:
    bool debug = false;
    std::shared_ptr<UTask> task_ptr = nullptr;
    std::shared_ptr<Swapinfo> swap_ptr;
    struct task_context *tc_sf = nullptr;
    struct sf_offset_table g_offset;

    bool init_env();
    void parser_gralloc4();
    void parser_gralloc5();
    void dump_GraphicBufferAllocator(ulong sAllocList_vaddr);
    void dumpDebugInfo(ulong sInstance_addr);
    void bufferDumpHelper(ulong handle_addr, std::ostringstream *outDump);
    std::string PixelFormatToString(int format);
    size_t get_sAllocList_from_vma();

public:
    SF(std::shared_ptr<Swapinfo> swap);
    SF();
    void init_offset(void) override;
    void init_command(void) override;
    void cmd_main(void) override;
    DEFINE_PLUGIN_INSTANCE(SF)
};

#endif // SF_DEFS_H_
