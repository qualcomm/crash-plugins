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

#include "sf.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(SF)
#endif

void SF::cmd_main(void) {
    int c;
    std::string cppString;
    if (argcnt < 2) cmd_usage(pc->curcmd, SYNOPSIS);
    if(!init_env()){
        return;
    }
    while ((c = getopt(argcnt, args, "ab")) != EOF) {
        switch(c) {
            case 'a':
                parser_gralloc4();
                break;
            case 'b':
                parser_gralloc5();
                break;
            default:
                argerrs++;
                break;
        }
    }
    if (argerrs)
        cmd_usage(pc->curcmd, SYNOPSIS);
}

void SF::init_offset(void) {}

/*
static android::KeyedVector<native_handle const*, android::GraphicBufferAllocator::alloc_rec_t> sAllocList;
ptype android::SortedVector<android::key_value_pair_t<native_handle const*, android::GraphicBufferAllocator::alloc_rec_t> >
ptype /o  android::VectorImpl
*/
void SF::parser_gralloc4(){
    LOGD("Parsing gralloc4 buffer information");
    ulong sAllocList_vaddr = get_sAllocList_from_vma();
    if (sAllocList_vaddr && !is_uvaddr(sAllocList_vaddr, tc_sf)){
        LOGE("Invalid sAllocList address: %#lx", sAllocList_vaddr);
        task_ptr.reset();
        return;
    }
    LOGD("Found sAllocList at %#lx", sAllocList_vaddr);
    dump_GraphicBufferAllocator(sAllocList_vaddr);
    /*
    64bit
    0000000000061a58 g     O .bss   0000000000000028              _ZN7android22GraphicBufferAllocator10sAllocListE --> sAllocList
    0000000000061a28 g     O .bss   0000000000000008              _ZN7android9SingletonINS_22GraphicBufferAllocatorEE9sInstanceE --> GraphicBufferAllocator::sInstance
    GraphicBufferAllocator::sAllocList = GraphicBufferAllocator::sInstance + 0x30
    32bit
    00040f74 g     O .bss   00000014              _ZN7android22GraphicBufferAllocator10sAllocListE
    00040f6c g     O .bss   00000004              _ZN7android9SingletonINS_22GraphicBufferAllocatorEE9sInstanceE
    GraphicBufferAllocator::sAllocList = GraphicBufferAllocator::sInstance + 0x8
    */
    ulong sInstance_addr = sAllocList_vaddr - ((BITS64() && !task_ptr->is_compat()) ? 0x30 : 0x8);
    if (sInstance_addr && !is_uvaddr(sInstance_addr, tc_sf)){
        LOGE("Invalid sInstance address: %#lx", sInstance_addr);
        task_ptr.reset();
        return;
    }
    LOGD("Found sInstance at %#lx", sInstance_addr);
    // "Singleton TYPE* sInstance"
    sInstance_addr = task_ptr->uread_pointer(sInstance_addr);
    dumpDebugInfo(sInstance_addr);
    LOGW("Successfully parsed gralloc4 buffer information");
    task_ptr.reset();
}

void SF::parser_gralloc5(){
    LOGD("Parsing gralloc5 buffer information");
    ulong sAllocList_vaddr = get_sAllocList_from_vma();
    if (sAllocList_vaddr && !is_uvaddr(sAllocList_vaddr, tc_sf)){
        LOGE("Invalid sAllocList address: %#lx", sAllocList_vaddr);
        task_ptr.reset();
        return;
    }
    LOGD("Found sAllocList at %#lx", sAllocList_vaddr);
    dump_GraphicBufferAllocator(sAllocList_vaddr);
    LOGD("Searching for snapalloc library");
    std::string libsnapalloc;
    for(const auto& vma_ptr : task_ptr->for_each_file_vma()){
        if(vma_ptr->name.find("vendor.qti.hardware.display.snapalloc-impl.so") != std::string::npos){
            libsnapalloc = vma_ptr->name;
            break;
        }
    }
    if(libsnapalloc.empty()){
        LOGE("Snapalloc library not found, skipping gralloc5 imported buffers");
        task_ptr.reset();
        return;
    }
    LOGD("Found snapalloc library: %s", libsnapalloc.c_str());
    ulong min_vma_start = task_ptr->get_min_vma_start(libsnapalloc);
    LOGD("Library base address: %#lx", min_vma_start);
    // static class snapalloc::SnapAllocCore *instance_;
    // 0000000000050318 g     O .bss   0000000000000008              _ZN9snapalloc13SnapAllocCore9instance_E
    ulong SnapAllocCore_addr = min_vma_start + 0x50318;
    SnapAllocCore_addr = task_ptr->uread_pointer(SnapAllocCore_addr);
    LOGD("SnapAllocCore instance at %#lx", SnapAllocCore_addr);
    // std::unordered_map<SnapHandle *, SnapHandleInternal *> handles_map_ = {};
    ulong core_handles_map = SnapAllocCore_addr + g_offset.SnapAllocCore_handles_map_;
    std::ostringstream oss;
    oss << "Imported gralloc5 buffers:\n";
    oss << std::left << std::setw(40) << "Name"
        << std::left << std::setw(25) << "W/H"
        << std::left << std::setw(22) << "Dmabuf"
        << std::left << std::setw(12) << "Format"
        << std::left << "Usage" << std::endl;
    size_t buffer_count = 0;
    for (const auto& pair : task_ptr->for_each_stdunmap(core_handles_map, task_ptr->get_pointer_size())) {
        ulong shi_addr = task_ptr->uread_pointer(pair.second);
        std::vector<char> tempdata = task_ptr->read_data(shi_addr, sizeof(SnapHandleInternal));
        if (tempdata.size() == 0){
            LOGD("Failed to read SnapHandleInternal at %#lx", shi_addr);
            continue;
        }
        buffer_count++;
        SnapHandleInternal* shi = (SnapHandleInternal*)tempdata.data();
        ulong name_addr = shi->shp.base_metadata + g_offset.SnapMetadata_name;
        std::string name = task_ptr->uread_cstring(name_addr, 256);
        std::vector<ulong> filetabe = task_ptr->for_each_file();
        ulong file_addr = filetabe[shi->fdpair.fd];
        ulong dmabuf = 0;
        if (is_kvaddr(file_addr)){
            dmabuf = read_pointer(file_addr + field_offset(file,private_data), "private_data");
        }

        oss << std::left << std::setw(40) << name
            << std::left << std::setw(25)
            << (std::to_string(shi->shp.unaligned_width) + "(" + std::to_string(shi->shp.aligned_width_in_pixels) + ") x " +
                std::to_string(shi->shp.unaligned_height) + "(" + std::to_string(shi->shp.aligned_height) + ")")
            << std::left << std::setw(22) << std::hex << std::showbase << dmabuf
            << std::left << std::setw(12) << PixelFormatToString(shi->shp.format)
            << std::left << std::hex << std::showbase << shi->shp.usage
            << std::dec  << std::endl;
    }
    PRINT("%s\n", oss.str().c_str());
    LOGD("Successfully parsed gralloc5 buffer information (%zu imported buffers)", buffer_count);
    task_ptr.reset();
}

void SF::dump_GraphicBufferAllocator(ulong sAllocList_vaddr){
    LOGD("Dumping GraphicBufferAllocator at %#lx", sAllocList_vaddr);
    // "KeyedVector"
    // void *mStorage; --> android::key_value_pair_t<native_handle const*, android::GraphicBufferAllocator::alloc_rec_t>
    ulong mStorage_addr = 0;
    size_t mItemSize = 0;
    size_t mCount = 0;
    uint32_t mFlags = 0;
    size_t data_size = (BITS64() && !task_ptr->is_compat()) ? sizeof(KeyedVector_64_t) : sizeof(KeyedVector_32_t);
    std::vector<char> tempList = task_ptr->read_data(sAllocList_vaddr, data_size);
    if (tempList.empty()) {
        LOGE("Failed to read sAllocList data");
        return;
    }
    auto extract_alloc_list = [&](auto* keyedvector) {
        mStorage_addr = keyedvector->mStorage_addr;
        mItemSize     = keyedvector->mItemSize;
        mCount        = keyedvector->mCount;
        mFlags        = keyedvector->mFlags;
    };
    if (BITS64() && !task_ptr->is_compat()) {
        extract_alloc_list(reinterpret_cast<KeyedVector_64_t*>(tempList.data()));
    } else {
        extract_alloc_list(reinterpret_cast<KeyedVector_32_t*>(tempList.data()));
    }
    LOGD("KeyedVector: mStorage=%#lx mCount=%zu mItemSize=%zu mFlags=%d",
         mStorage_addr, mCount, mItemSize, mFlags);
    std::ostringstream oss;
    oss  << "GraphicBufferAllocator buffers:\n";
    oss  << std::setw(12) << std::right << "Handle" << " | "
            << std::setw(18) << std::right << "Dmabuf" << " | "
            << std::setw(12) << "Size" << " | "
            << std::setw(18) << "W (Stride) x H" << " | "
            << std::setw(6) << "Layers" << " | "
            << std::setw(12) << "Format" << " | "
            << std::setw(12) << "Usage" << " | "
            << "Requestor" << "\n";
    uint64_t total = 0;
    // *   ----------------------------------------------- <--- mStorage_addr
    // *   |  native_handle*    |    alloc_rec_t         |
    // *   |--------------------| -----------------------| <--- mStorage_addr + i * mItemSize
    // *   |  native_handle*    |    alloc_rec_t         |
    // *   ----------------------------------------------
    for (size_t i = 0; i < mCount; i++) {
        ulong native_handle_addr = (mStorage_addr + i * mItemSize) & task_ptr->vaddr_mask;
        native_handle_addr = task_ptr->uread_pointer(native_handle_addr);
        std::vector<char> tmphandle = task_ptr->read_data(native_handle_addr, sizeof(private_handle_t));
        if(tmphandle.size() == 0){
            LOGE("Failed to read native_handle at %#lx (index %zu)", native_handle_addr, i);
            continue;
        }
        private_handle_t* phandle = (private_handle_t*)tmphandle.data();
        std::vector<ulong> filetabe = task_ptr->for_each_file();
        ulong file_addr = filetabe[phandle->fd];
        ulong dmabuf = 0;
        if (is_kvaddr(file_addr)){
            dmabuf = read_pointer(file_addr + field_offset(file, private_data), "private_data");
        }
        ulong alloc_rec_t_addr = (mStorage_addr + i * mItemSize + 8) & task_ptr->vaddr_mask;
        size_t size = task_ptr->uread_uint(alloc_rec_t_addr + sizeof(alloc_rec_t));
        ulong requestorName_addr = alloc_rec_t_addr + sizeof(alloc_rec_t) + task_ptr->get_pointer_size() /* size_t */;
        std::string requestorName = task_ptr->for_each_stdstring(requestorName_addr);
        std::vector<char> tmprec = task_ptr->read_data(alloc_rec_t_addr, sizeof(alloc_rec_t));
        LOGD("Processing buffer %zu: handle=%#lx alloc_rec=%#lx", i, native_handle_addr, alloc_rec_t_addr);
        if (tmprec.size() == 0){
            LOGE("Failed to read alloc_rec_t at %#lx", alloc_rec_t_addr);
            continue;
        }
        alloc_rec_t* rec = (alloc_rec_t*)tmprec.data();
        // print
        std::ostringstream sizeStream;
        if (size) {
            sizeStream  << std::fixed << std::setprecision(2)
                        << std::setw(7) << static_cast<double>(size) / 1024.0 << " KiB";
        } else {
            sizeStream  << "unknown";
        }
        std::string sizeStr = sizeStream.str();
        oss  << std::setw(12) << std::hex << std::showbase << native_handle_addr << " | "
                << std::setw(18) << std::hex << dmabuf << " | " << std::dec
                << std::setw(12) << sizeStr << " | "
                << std::setw(4) << rec->width << " (" << std::setw(4) << rec->stride << ") x "
                << std::setw(4) << rec->height << " | "
                << std::setw(6) << rec->layerCount << " | "
                << std::setw(12) << PixelFormatToString(rec->format) << " | "
                << std::setw(12) << std::hex << std::showbase << rec->usage << " | "
                << requestorName << "\n";
        total += size;
    }
    oss  << "Total allocated by GraphicBufferAllocator (estimate): "
            << std::fixed << std::setprecision(2)
            << static_cast<double>(total) / 1024.0 << " KB\n";
    PRINT("%s\n", oss.str().c_str());
    LOGD("Found %zu allocated buffers (%.2f KB total)", mCount, static_cast<double>(total) / 1024.0);
}

// GraphicBufferAllocator::sInstance
void SF::dumpDebugInfo(ulong sInstance_addr){
    LOGD("Dumping gralloc4 imported buffers from sInstance %#lx", sInstance_addr);
    ulong GrallocAllocator_mAllocator_addr = task_ptr->uread_pointer(sInstance_addr + g_offset.GraphicBufferAllocator_mAllocator);
    // Gralloc4Allocator : GrallocAllocator
    ulong Gralloc4Allocator_mMapper_addr = task_ptr->uread_pointer(GrallocAllocator_mAllocator_addr + g_offset.Gralloc4Allocator_mMapper);
    ulong Gralloc4Mapper_mMapper_addr = task_ptr->uread_pointer(Gralloc4Allocator_mMapper_addr + g_offset.Gralloc4Mapper_mMapper);
    ulong BsQtiMapper_mImpl_addr = task_ptr->uread_pointer(Gralloc4Mapper_mMapper_addr + g_offset.BsQtiMapper_mImpl);
    ulong QtiMapper_buf_mgr_addr = task_ptr->uread_pointer(BsQtiMapper_mImpl_addr + g_offset.IQtiMapper_buf_mgr_);
    ulong BufferManager_handles_map_addr = QtiMapper_buf_mgr_addr + g_offset.BufferManager_handles_map_;
    LOGD("BufferManager handles_map at %#lx", BufferManager_handles_map_addr);
    /*
    std::unordered_map<const private_handle_t *, std::shared_ptr<Buffer>> handles_map_
    // *   ------------------------------------------------------------- <--- BufferManager_handles_map_addr
    // *   |  private_handle_t*    |    std::shared_ptr<Buffer>         |
    // *   |-----------------------| -----------------------------------|
    // *   |  private_handle_t*    |    std::shared_ptr<Buffer>         |
    // *   --------------------------------------------------------------
    */
    std::ostringstream stream;
    stream << "Imported gralloc4 buffers:\n";
    stream << std::left << std::setw(40) << "Name"
        << std::left << std::setw(20) << "W/H"
        << std::left << std::setw(22) << "Dmabuf"
        << std::left << std::setw(12) << "Format"
        << std::left << "Usage" << std::endl;
    size_t imported_count = 0;
    for (const auto& pair : task_ptr->for_each_stdunmap(BufferManager_handles_map_addr, task_ptr->get_pointer_size())) {
        ulong handle_addr = task_ptr->uread_pointer(pair.first);
        bufferDumpHelper(handle_addr, &stream);
        imported_count++;
    }
    PRINT("%s\n", stream.str().c_str());
    LOGI("Found %zu imported gralloc4 buffers", imported_count);
}

void SF::bufferDumpHelper(ulong handle_addr, std::ostringstream* outDump){
    std::vector<char> handle = task_ptr->read_data(handle_addr, sizeof(private_handle_t));
    if (handle.size() == 0){
        LOGE("Failed to read private_handle_t at %#lx", handle_addr);
        return;
    }
    private_handle_t* phandle = (private_handle_t*)handle.data();
    ulong name_addr = phandle->base_metadata + g_offset.MetaData_t_name;
    std::string name = task_ptr->uread_cstring(name_addr, 256);
    std::vector<ulong> filetabe = task_ptr->for_each_file();
    ulong file_addr = filetabe[phandle->fd];
    ulong dmabuf = 0;
    if (is_kvaddr(file_addr)){
        dmabuf = read_pointer(file_addr + field_offset(file,private_data), "private_data");
    }
    *outDump << std::left << std::setw(40) << name
            << std::left << std::setw(20)
            << (std::to_string(phandle->unaligned_width) + "x" + std::to_string(phandle->unaligned_height))
            << std::left << std::setw(22) << std::hex << std::showbase << dmabuf
            << std::left << std::setw(12) << PixelFormatToString(phandle->format)
            << std::left << std::hex << std::showbase << phandle->usage
            << std::dec << std::endl;
}

/*
KeyedVector hold the mVector(SortedVector)
SortedVector : KeyedVector : VectorImpl
*/
size_t SF::get_sAllocList_from_vma(){
    LOGD("Searching for sAllocList in VMA");
    auto vma_callback = [&](std::shared_ptr<vma_struct> vma_ptr) -> bool {
        return true;
    };
    if (BITS64() && !task_ptr->is_compat()) {
        std::string libui = "/system/lib64/libui.so";
        LOGD("Searching in %s (64-bit)", libui.c_str());
        auto obj_callback = [&](KeyedVector_64_t* obj) -> bool {
            return obj->mItemSize == g_offset.mItemSize;
        };
        return task_ptr->search_obj<KeyedVector_64_t, uint64_t>(libui, true, vma_callback, obj_callback, 21 /*vtbl count*/);
    } else {
        std::string libui = "/system/lib/libui.so";
        LOGD("Searching in %s (32-bit)", libui.c_str());
        auto obj_callback = [&](KeyedVector_32_t* obj) -> bool {
            return obj->mItemSize == g_offset.mItemSize;
        };
        return task_ptr->search_obj<KeyedVector_32_t, uint32_t>(libui, true, vma_callback, obj_callback, 21 /*vtbl count*/);
    }
}

std::string SF::PixelFormatToString(int format) {
    switch (format) {
        case UNSPECIFIED: return "UNSPECIFIED";
        case RGBA_8888: return "RGBA_8888";
        case RGBX_8888: return "RGBX_8888";
        case RGB_888: return "RGB_888";
        case RGB_565: return "RGB_565";
        case BGRA_8888: return "BGRA_8888";
        case YCBCR_422_SP: return "YCBCR_422_SP";
        case YCRCB_420_SP: return "YCRCB_420_SP";
        case YCBCR_422_I: return "YCBCR_422_I";
        case RGBA_FP16: return "RGBA_FP16";
        case RAW16: return "RAW16";
        case BLOB: return "BLOB";
        case IMPLEMENTATION_DEFINED: return "IMPLEMENTATION_DEFINED";
        case YCBCR_420_888: return "YCBCR_420_888";
        case RAW_OPAQUE: return "RAW_OPAQUE";
        case RAW10: return "RAW10";
        case RAW12: return "RAW12";
        case RGBA_1010102: return "RGBA_1010102";
        case Y8: return "Y8";
        case Y16: return "Y16";
        case YV12: return "YV12";
        case DEPTH_16: return "DEPTH_16";
        case DEPTH_24: return "DEPTH_24";
        case DEPTH_24_STENCIL_8: return "DEPTH_24_STENCIL_8";
        case DEPTH_32F: return "DEPTH_32F";
        case DEPTH_32F_STENCIL_8: return "DEPTH_32F_STENCIL_8";
        case STENCIL_8: return "STENCIL_8";
        case YCBCR_P010: return "YCBCR_P010";
        case HSV_888: return "HSV_888";
        case R_8: return "R_8";
        case R_16_UINT: return "R_16_UINT";
        case RG_1616_UINT: return "RG_1616_UINT";
        case RGBA_10101010: return "RGBA_10101010";
        default: return "UNKNOWN_FORMAT";
    }
}

bool SF::init_env(){
    LOGD("Initializing SurfaceFlinger environment");
    tc_sf = find_proc("surfaceflinger");
    if(!tc_sf){
        LOGE("SurfaceFlinger process not found");
        return false;
    }
    LOGD("Found surfaceflinger process (pid=%d)", tc_sf->pid);
    if(!task_ptr){
        task_ptr = std::make_shared<UTask>(swap_ptr, tc_sf->task);
    }
    field_init(file, private_data);
    LOGD("Initializing structure offsets (arch=%s)",
         (BITS64() && !task_ptr->is_compat()) ? "64-bit" : "32-bit");
    // ptype /o  android::GraphicBufferAllocator
    g_offset.GraphicBufferAllocator_mAllocator = (BITS64() && !task_ptr->is_compat()) ? 8 : 4;
    // ptype /o android::Gralloc4Allocator
    g_offset.Gralloc4Allocator_mMapper = (BITS64() && !task_ptr->is_compat()) ? 8 : 4;
    // ptype /o android::Gralloc4Mapper
    g_offset.Gralloc4Mapper_mMapper = (BITS64() && !task_ptr->is_compat()) ? 8 : 4;
    // ptype /o 'vendor::qti::hardware::display::mapper::V4_0::BsQtiMapper'
    g_offset.BsQtiMapper_mImpl = (BITS64() && !task_ptr->is_compat()) ? 96 : 48;
    // ptype /o 'vendor::qti::hardware::display::mapper::V4_0::implementation::QtiMapper'
    g_offset.IQtiMapper_buf_mgr_ = (BITS64() && !task_ptr->is_compat()) ? 16 : 8;
    // ptype /o 'gralloc::BufferManager'
    g_offset.BufferManager_handles_map_ = (BITS64() && !task_ptr->is_compat()) ? 48 : 8;
    // p 'android::GraphicBufferAllocator::sAllocList'
    g_offset.mItemSize = (BITS64() && !task_ptr->is_compat()) ? 72 : 56;
    // ptype /o 'snapalloc ::SnapAllocCore'
    g_offset.SnapAllocCore_handles_map_ = 112;
    // ptype /o 'snapalloc::SnapMetadata'
    g_offset.SnapMetadata_name = 24484;
    // ptype /o MetaData_t
    g_offset.MetaData_t_name = 24768;
    LOGW("SurfaceFlinger environment initialized successfully");
    return true;
}

SF::SF(std::shared_ptr<Swapinfo> swap) : swap_ptr(swap){}

SF::SF(){
    swap_ptr = std::make_shared<Swapinfo>();
}

void SF::init_command(void){
    cmd_name = "sfg";
    help_str_list={
        "sfg",                            /* command name */
        "dump surfaceflinger information",        /* short description */
        "-a \n"
            "  sfg -b \n"
            "  This command dump surfaceflinger info.",
        "\n",
        "EXAMPLES",
        "  Display gralloc4 info:",
        "    %s> sfg -a",
        "     GraphicBufferAllocator buffers:",
        "           Handle |             Dmabuf |         Size |     W (Stride) x H | Layers |       Format |        Usage | Requestor",
        "     0x7486014340 | 0xffffff804f37e400 |   908.00 KiB |  454 ( 512) x  454 |      1 |    RGBA_8888 |       0x1b00 | FramebufferSurface",
        "     Total allocated by GraphicBufferAllocator (estimate): 1350.00 KB",
        "     Imported gralloc4 buffers:",
        "     Name                                    W/H                      Dmabuf                Format      Usage",
        "     FramebufferSurface                      454(512) x 454(464)      0xffffff804ea87000    RGBA_8888   0x1b00",
        "\n",
        "  Display gralloc5 info:",
        "    %s> sfg -b",
        "     GraphicBufferAllocator buffers:",
        "           Handle |             Dmabuf |         Size |     W (Stride) x H | Layers |       Format |        Usage | Requestor",
        "     0x7486014340 | 0xffffff804f37e400 |   908.00 KiB |  454 ( 512) x  454 |      1 |    RGBA_8888 |       0x1b00 | FramebufferSurface",
        "     Total allocated by GraphicBufferAllocator (estimate): 1350.00 KB",
        "     Imported gralloc5 buffers:",
        "     Name                                    W/H                      Dmabuf                Format      Usage",
        "     FramebufferSurface                      454(512) x 454(464)      0xffffff804ea87000    RGBA_8888   0x1b00",
        "\n",
    };
}

#pragma GCC diagnostic pop
