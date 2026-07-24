# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 and
# only version 2 as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# SPDX-License-Identifier: GPL-2.0-only

cmake_minimum_required(VERSION 3.21.1)
project(plugins)

# =================== Basic Configuration ===================
string(TOUPPER "${CMAKE_BUILD_TARGET_ARCH}" BUILD_TARGET_ARCH)
add_definitions(
    -D${BUILD_TARGET_ARCH}
    -D__LITTLE_ENDIAN
    -D_FILE_OFFSET_BITS=64)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -Wall")
include_directories(.)

# Architecture-specific configuration
if (CMAKE_BUILD_TARGET_ARCH STREQUAL "arm64")
    set(CMAKE_LIBRARY_ARCHITECTURE x86_64-linux-gnu)
elseif(CMAKE_BUILD_TARGET_ARCH STREQUAL "arm")
    set(CMAKE_LIBRARY_ARCHITECTURE i386-linux-gnu)
endif()

if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm|aarch64")
    add_compile_definitions(IS_ARM)
endif()

# =================== Dependency Library Detection ===================
find_package(PkgConfig REQUIRED)

# Check for required libraries
# sudo apt-get install libzstd-dev libzstd-dev:i386
pkg_check_modules(ZSTD REQUIRED libzstd)
if(NOT ZSTD_FOUND)
    message(FATAL_ERROR "libzstd library not found.")
endif()

# sudo apt-get install libelf-dev libelf-dev:i386
pkg_check_modules(ELF REQUIRED libelf)
if(NOT ELF_FOUND)
    message(FATAL_ERROR "libelf library not found.")
endif()

# sudo apt-get install libsystemd-dev libsystemd-dev:i386
pkg_check_modules(SYSTEMD REQUIRED libsystemd)
if(NOT SYSTEMD_FOUND)
    message(FATAL_ERROR "libsystemd library not found.")
endif()

# =================== Common Source Files ===================
set(PLUGIN_SOURCES plugin.cpp logger/logger_core.cpp)

if(DEFINED BUILD_TARGET_TOGETHER)
add_definitions(-DBUILD_TARGET_TOGETHER)
list(APPEND PLUGIN_SOURCES
    plugins.cpp
    logger/logger.cpp
    utils/utask.cpp
    utils/aarch64_pagetable.cpp
    lib/lzo/lzo1x_decompress_safe.c
    lib/lz4/lz4_decompress.c
    binder/binder.cpp
    memory/cma.cpp
    memory/reserved.cpp
    memory/memblock.cpp
    memory/vmalloc.cpp
    memory/dmabuf/cmd_buf.cpp
    memory/dmabuf/dmabuf.cpp
    memory/dmabuf/heap.cpp
    memory/dmabuf/dma_heap.cpp
    memory/dmabuf/ion_heap.cpp
    memory/buddy.cpp
    memory/slub.cpp
    rcu/rcu.cpp
    memory/zraminfo.cpp
    memory/zram.cpp
    memory/swapinfo.cpp
    memory/swap.cpp
    procrank/procrank.cpp
    devicetree/dts.cpp
    devicetree/devicetree.cpp
    pageowner/pageowner.cpp
    workqueue/workqueue.cpp
    partition/filesystem.cpp
    property/propinfo.cpp
    property/prop.cpp
    logcat/logcat_parser.cpp
    logcat/logcat.cpp
    logcat/logcatS.cpp
    logcat/logcatR.cpp
    logcat/logcatLE.cpp
    device_driver/dd.cpp
    rtb/rtb.cpp
    cpu/cpuinfo.cpp
    coredump/coredump.cpp
    coredump/core.cpp
    thermal/thermal.cpp
    memory/meminfo.cpp
    watchdog/wdt.cpp
    pagecache/cache.cpp
    debugimage/debugimage.cpp
    debugimage/image_parser.cpp
    debugimage/cpu32_ctx.cpp
    debugimage/cpu64_ctx_v13.cpp
    debugimage/cpu64_ctx_v14.cpp
    debugimage/cpu64_ctx_v20.cpp
    ipc/ipc.cpp
    regulator/regulator.cpp
    icc/icc.cpp
    clock/clock.cpp
    pstore/pstore.cpp
    sysinfo/sys.cpp
    ftrace/ftrace.cpp
    ftrace/trace_event.cpp
    ftrace/events.cpp
    qlog/qlog.cpp
    task/task_sched.cpp
    surfaceflinger/sf.cpp
    systemd/journal.cpp
    iommu/iommu.cpp
    t32/t32.cpp
    lockdep/lockdep.cpp)

add_library(plugins SHARED ${PLUGIN_SOURCES})
if (CMAKE_BUILD_TARGET_ARCH STREQUAL "arm64")
    target_sources(plugins PRIVATE
        coredump/arm/arm64.cpp
        coredump/arm/compat.cpp)
elseif (CMAKE_BUILD_TARGET_ARCH STREQUAL "arm")
    target_sources(plugins PRIVATE
        coredump/arm/arm.cpp)
endif()

set_target_properties(plugins PROPERTIES PREFIX "")
target_link_libraries(plugins
    ${ELF_LIBRARIES}
    ${ZSTD_LIBRARIES}
    ${SYSTEMD_LIBRARIES})

else()
add_library(plugin_common INTERFACE)
target_link_libraries(plugin_common INTERFACE ${ELF_LIBRARIES})
target_include_directories(plugin_common INTERFACE .)

macro(add_plugin name)
    add_library(${name} SHARED ${PLUGIN_SOURCES} ${ARGN})
    set_target_properties(${name} PROPERTIES PREFIX "")
    target_link_libraries(${name} plugin_common)
endmacro()

# =================== build logger ===================
add_plugin(logger
    logger/logger.cpp)

# =================== build dmabuf ===================
add_plugin(dmabuf
    memory/dmabuf/cmd_buf.cpp
    memory/dmabuf/dmabuf.cpp
    memory/dmabuf/heap.cpp
    memory/dmabuf/dma_heap.cpp
    memory/dmabuf/ion_heap.cpp)

# =================== build binder ===================
add_plugin(binder
    binder/binder.cpp)

# =================== build cma ===================
add_plugin(cma
    memory/cma.cpp)

# =================== build slub ===================
add_plugin(slub
    memory/slub.cpp)

# =================== build pageowner ===================
add_plugin(pageowner
    pageowner/pageowner.cpp)

# =================== build procrank ===================
add_plugin(procrank
    memory/zraminfo.cpp
    memory/swapinfo.cpp
    procrank/procrank.cpp
    lib/lzo/lzo1x_decompress_safe.c
    lib/lz4/lz4_decompress.c
    utils/utask.cpp)
target_link_libraries(procrank ${ZSTD_LIBRARIES})

# =================== build dts ===================
add_plugin(dts
    devicetree/dts.cpp
    devicetree/devicetree.cpp)

# =================== build memblock ===================
add_plugin(memblock
    memory/memblock.cpp)

# =================== build wq ===================
add_plugin(wq
    workqueue/workqueue.cpp)

# =================== build reserved ===================
add_plugin(reserved
    memory/reserved.cpp
    devicetree/devicetree.cpp)

# =================== build vmalloc ===================
add_plugin(vmalloc
    memory/vmalloc.cpp)

# =================== build buddy ===================
add_plugin(buddy
    memory/buddy.cpp)

# =================== build rtb ===================
add_plugin(rtb
    rtb/rtb.cpp)

# =================== build zram ===================
add_plugin(zram
    memory/zraminfo.cpp
    memory/zram.cpp
    lib/lzo/lzo1x_decompress_safe.c
    lib/lz4/lz4_decompress.c)

# =================== build swap ===================
add_plugin(swap
    memory/zraminfo.cpp
    memory/swapinfo.cpp
    memory/swap.cpp
    lib/lzo/lzo1x_decompress_safe.c
    lib/lz4/lz4_decompress.c)

# =================== build prop ===================
add_plugin(prop
    utils/utask.cpp
    memory/zraminfo.cpp
    memory/swapinfo.cpp
    property/propinfo.cpp
    property/prop.cpp
    lib/lzo/lzo1x_decompress_safe.c
    lib/lz4/lz4_decompress.c)

# =================== build logcat ===================
add_plugin(logcat
    utils/utask.cpp
    memory/zraminfo.cpp
    memory/swapinfo.cpp
    property/propinfo.cpp
    logcat/logcat_parser.cpp
    logcat/logcat.cpp
    logcat/logcatS.cpp
    logcat/logcatR.cpp
    logcat/logcatLE.cpp
    lib/lzo/lzo1x_decompress_safe.c
    lib/lz4/lz4_decompress.c)
target_link_libraries(logcat ${ZSTD_LIBRARIES})

# =================== build dd ===================
add_plugin(dd
    device_driver/dd.cpp)

# =================== build cpuinfo ===================
add_plugin(cpuinfo
    cpu/cpuinfo.cpp)

# =================== build core ===================
add_plugin(core
    utils/utask.cpp
    memory/zraminfo.cpp
    memory/swapinfo.cpp
    coredump/coredump.cpp
    coredump/core.cpp
    lib/lzo/lzo1x_decompress_safe.c
    lib/lz4/lz4_decompress.c)
if (CMAKE_BUILD_TARGET_ARCH STREQUAL "arm64")
    target_sources(core PRIVATE
        coredump/arm/arm64.cpp
        coredump/arm/compat.cpp)
elseif (CMAKE_BUILD_TARGET_ARCH STREQUAL "arm")
    target_sources(core PRIVATE
        coredump/arm/arm.cpp)
endif()

# =================== build thermal ===================
add_plugin(tm
    thermal/thermal.cpp)

# =================== build meminfo ===================
add_plugin(meminfo
    devicetree/devicetree.cpp
    memory/meminfo.cpp)

# =================== build watchdog ===================
add_plugin(wdt
    watchdog/wdt.cpp)

# =================== build pagecache ===================
add_plugin(cache
    pagecache/cache.cpp)

# =================== build debugimage ===================
add_plugin(dbi
    debugimage/debugimage.cpp
    debugimage/image_parser.cpp
    debugimage/cpu32_ctx.cpp
    debugimage/cpu64_ctx_v13.cpp
    debugimage/cpu64_ctx_v14.cpp
    debugimage/cpu64_ctx_v20.cpp)

# =================== build IPC log ===================
add_plugin(ipc
    ipc/ipc.cpp)

# =================== build regulator ===================
add_plugin(reg
    regulator/regulator.cpp)

# =================== build icc ===================
add_plugin(icc
    icc/icc.cpp)

# =================== build clock ===================
add_plugin(ccf
    clock/clock.cpp)

# =================== build pstore ===================
add_plugin(pstore
    utils/utask.cpp
    memory/zraminfo.cpp
    memory/swapinfo.cpp
    lib/lzo/lzo1x_decompress_safe.c
    lib/lz4/lz4_decompress.c
    pstore/pstore.cpp)

# =================== build sys ===================
add_plugin(sys
    sysinfo/sys.cpp)

# =================== build qlog ===================
add_plugin(qlog
    devicetree/devicetree.cpp
    qlog/qlog.cpp)

# =================== build task ===================
add_plugin(sched
    task/task_sched.cpp)

# =================== build ftrace ===================
add_plugin(ftrace
    ftrace/ftrace.cpp
    ftrace/events.cpp
    ftrace/trace_event.cpp)

# =================== build sf ===================
add_plugin(sf
    memory/zraminfo.cpp
    memory/swapinfo.cpp
    utils/utask.cpp
    lib/lzo/lzo1x_decompress_safe.c
    lib/lz4/lz4_decompress.c
    surfaceflinger/sf.cpp)

# =================== build systemd ===================
add_plugin(sd
    memory/zraminfo.cpp
    memory/swapinfo.cpp
    utils/utask.cpp
    lib/lzo/lzo1x_decompress_safe.c
    lib/lz4/lz4_decompress.c
    systemd/journal.cpp)
target_link_libraries(sd ${SYSTEMD_LIBRARIES})

# =================== build t32 ===================
add_plugin(t32
    t32/t32.cpp)

# =================== build lockdep ===================
add_plugin(lockdep
    lockdep/lockdep.cpp)

# =================== build rcu ===================
add_plugin(rcu
    rcu/rcu.cpp)
endif()
