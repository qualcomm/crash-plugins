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

#include "binder/binder.h"
#include "procrank/procrank.h"
#include "memory/cma.h"
#include "devicetree/dts.h"
#include "memory/memblock.h"
#include "memory/reserved.h"
#include "memory/vmalloc.h"
#include "memory/dmabuf/cmd_buf.h"
#include "memory/slub.h"
#include "memory/zram.h"
#include "memory/swap.h"
#include "memory/buddy.h"
#include "device_driver/dd.h"
#include "pageowner/pageowner.h"
#include "workqueue/workqueue.h"
#include "partition/filesystem.h"
#include "cpu/cpuinfo.h"
#include "rtb/rtb.h"
#include "property/prop.h"
#include "logcat/logcat_parser.h"
#include "coredump/coredump.h"
#include "thermal/thermal.h"
#include "memory/meminfo.h"
#include "watchdog/wdt.h"
#include "pagecache/cache.h"
#include "debugimage/debugimage.h"
#include "ipc/ipc.h"
#include "regulator/regulator.h"
#include "icc/icc.h"
#include "clock/clock.h"
#include "pstore/pstore.h"
#include "sysinfo/sys.h"
#include "ftrace/ftrace.h"
#include "qlog/qlog.h"
#include "task/task_sched.h"
#include "surfaceflinger/sf.h"
#include "systemd/journal.h"
#include "t32/t32.h"
#include "logger/logger.h"
#include "iommu/iommu.h"
#include "lockdep/lockdep.h"
#include "rcu/rcu.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifdef BUILD_TARGET_TOGETHER
extern "C" void plugin_init(void);
extern "C" void plugin_fini(void);

static std::vector<std::shared_ptr<ParserPlugin>> plugins;
static struct command_table_entry* command_table;
std::chrono::duration<double> total_construct_time(0);
std::chrono::duration<double> total_init_command_time(0);
std::chrono::duration<double> total_initialize_time(0);
std::chrono::duration<double> total_init_offset_time(0);

template <typename T, typename... Args>
std::shared_ptr<T> make_and_init(Args&&... args) {
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    T::instance = std::shared_ptr<T>(new T(std::forward<Args>(args)...));
    std::chrono::high_resolution_clock::time_point after_construct = std::chrono::high_resolution_clock::now();
    T::instance->init_command();
    std::chrono::high_resolution_clock::time_point after_init_command = std::chrono::high_resolution_clock::now();
    T::instance->initialize();
    std::chrono::high_resolution_clock::time_point after_initialize = std::chrono::high_resolution_clock::now();
    if (T::instance->do_init_offset) {
        T::instance->init_offset();
    }
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
    total_construct_time += std::chrono::duration<double>(after_construct - start);
    total_init_command_time += std::chrono::duration<double>(after_init_command - after_construct);
    total_initialize_time += std::chrono::duration<double>(after_initialize - after_init_command);
    if (T::instance->do_init_offset) {
        total_init_offset_time += std::chrono::duration<double>(end - after_initialize);
    }
    return T::instance;
}

std::shared_ptr<Binder>        Binder::instance        = nullptr;
std::shared_ptr<Slub>          Slub::instance          = nullptr;
std::shared_ptr<Procrank>      Procrank::instance      = nullptr;
std::shared_ptr<Cma>           Cma::instance           = nullptr;
std::shared_ptr<Dts>           Dts::instance           = nullptr;
std::shared_ptr<Memblock>      Memblock::instance      = nullptr;
std::shared_ptr<DDriver>       DDriver::instance       = nullptr;
std::shared_ptr<DmaIon>        DmaIon::instance        = nullptr;
std::shared_ptr<Workqueue>     Workqueue::instance     = nullptr;
std::shared_ptr<Reserved>      Reserved::instance      = nullptr;
std::shared_ptr<Vmalloc>       Vmalloc::instance       = nullptr;
std::shared_ptr<FileSystem>    FileSystem::instance    = nullptr;
std::shared_ptr<Pageowner>     Pageowner::instance     = nullptr;
std::shared_ptr<Buddy>         Buddy::instance         = nullptr;
std::shared_ptr<Zram>          Zram::instance          = nullptr;
std::shared_ptr<Swap>          Swap::instance          = nullptr;
std::shared_ptr<Prop>          Prop::instance          = nullptr;
std::shared_ptr<Logcat_Parser> Logcat_Parser::instance = nullptr;
std::shared_ptr<Rtb>           Rtb::instance           = nullptr;
std::shared_ptr<CpuInfo>       CpuInfo::instance       = nullptr;
std::shared_ptr<Coredump>      Coredump::instance      = nullptr;
std::shared_ptr<Thermal>       Thermal::instance       = nullptr;
std::shared_ptr<Meminfo>       Meminfo::instance       = nullptr;
std::shared_ptr<Watchdog>      Watchdog::instance      = nullptr;
std::shared_ptr<Cache>         Cache::instance         = nullptr;
std::shared_ptr<DebugImage>    DebugImage::instance    = nullptr;
std::shared_ptr<IPCLog>        IPCLog::instance        = nullptr;
std::shared_ptr<Regulator>     Regulator::instance     = nullptr;
std::shared_ptr<ICC>           ICC::instance           = nullptr;
std::shared_ptr<Clock>         Clock::instance         = nullptr;
std::shared_ptr<Pstore>        Pstore::instance        = nullptr;
std::shared_ptr<SysInfo>       SysInfo::instance       = nullptr;
std::shared_ptr<Ftrace>        Ftrace::instance        = nullptr;
std::shared_ptr<QLog>          QLog::instance          = nullptr;
std::shared_ptr<TaskSched>     TaskSched::instance     = nullptr;
std::shared_ptr<SF>            SF::instance            = nullptr;
std::shared_ptr<Journal>       Journal::instance       = nullptr;
std::shared_ptr<T32>           T32::instance           = nullptr;
std::shared_ptr<Logger>        Logger::instance        = nullptr;
std::shared_ptr<IOMMU>         IOMMU::instance         = nullptr;
std::shared_ptr<Lockdep>       Lockdep::instance       = nullptr;
std::shared_ptr<Rcu>           Rcu::instance           = nullptr;

extern "C" void __attribute__((constructor)) plugin_init(void) {
    // fprintf(fp, "plugin_init\n");
    plugins.push_back(make_and_init<Binder>());
    plugins.push_back(make_and_init<Slub>());
    plugins.push_back(make_and_init<Procrank>());
    plugins.push_back(make_and_init<Cma>());
    plugins.push_back(make_and_init<Dts>());
    plugins.push_back(make_and_init<Memblock>());
    plugins.push_back(make_and_init<DDriver>());
    plugins.push_back(make_and_init<DmaIon>());
    plugins.push_back(make_and_init<Workqueue>());
    plugins.push_back(make_and_init<Reserved>());
    plugins.push_back(make_and_init<Vmalloc>());
    plugins.push_back(make_and_init<FileSystem>());
    plugins.push_back(make_and_init<Pageowner>());
    plugins.push_back(make_and_init<Buddy>());
    plugins.push_back(make_and_init<Zram>());
    plugins.push_back(make_and_init<Swap>(Zram::instance));
    plugins.push_back(make_and_init<Prop>(Swap::instance));
    plugins.push_back(make_and_init<Logcat_Parser>(Swap::instance, Prop::instance));
    plugins.push_back(make_and_init<Rtb>());
    plugins.push_back(make_and_init<CpuInfo>());
    plugins.push_back(make_and_init<Coredump>(Swap::instance));
    plugins.push_back(make_and_init<Thermal>());
    plugins.push_back(make_and_init<Meminfo>());
    plugins.push_back(make_and_init<Watchdog>());
    plugins.push_back(make_and_init<Cache>());
    plugins.push_back(make_and_init<DebugImage>());
    plugins.push_back(make_and_init<IPCLog>());
    plugins.push_back(make_and_init<Regulator>());
    plugins.push_back(make_and_init<ICC>());
    plugins.push_back(make_and_init<Clock>());
    plugins.push_back(make_and_init<Pstore>());
    plugins.push_back(make_and_init<SysInfo>());
    plugins.push_back(make_and_init<Ftrace>());
    plugins.push_back(make_and_init<QLog>());
    plugins.push_back(make_and_init<TaskSched>());
    plugins.push_back(make_and_init<SF>(Swap::instance));
    plugins.push_back(make_and_init<Journal>(Swap::instance));
    plugins.push_back(make_and_init<T32>());
    plugins.push_back(make_and_init<Logger>());
    plugins.push_back(make_and_init<IOMMU>());
    plugins.push_back(make_and_init<Lockdep>());
    plugins.push_back(make_and_init<Rcu>());
    std::cout << "\033[32m"
            << std::fixed << std::setprecision(6)
            << "[Load] Constructor: " << total_construct_time.count() << " s, "
            << "init_command: " << total_init_command_time.count() << " s, "
            << "initialize: " << total_initialize_time.count() << " s, "
            << "init_offset: " << total_init_offset_time.count() << " s"
            << "\033[0m"
            << std::endl;
    command_table = new command_table_entry[plugins.size() + 1];
    for(size_t i=0; i < plugins.size(); i++){
        command_table[i] = { &plugins[i]->cmd_name[0], plugins[i]->get_wrapper_func(), plugins[i]->cmd_help, 0 };
    }
    command_table[plugins.size()] = { NULL };
    register_extension(command_table);
}

extern "C" void __attribute__((destructor)) plugin_fini(void) {
    // fprintf(fp, "plugin_fini\n");
    for (auto& plugin : plugins) {
        plugin.reset();
    }
    plugins.clear();
    delete[] command_table;
    command_table = nullptr;
}

#endif // BUILD_TARGET_TOGETHER

#pragma GCC diagnostic pop
