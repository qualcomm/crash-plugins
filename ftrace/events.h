/**
 * @file events.h
 * @brief Specific trace event handlers for ftrace
 *
 * This file defines specific trace event handler classes for various
 * kernel subsystems. Each event type has its own handler that knows
 * how to format and display the event data appropriately.
 *
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

#ifndef EVENTS_DEFS_H_
#define EVENTS_DEFS_H_

#include "trace_event.h"
#include <linux/ioctl.h>

// DWC3 endpoint command definitions
#define DWC3_DEPCMD_DEPSTARTCFG        (0x09 << 0)
#define DWC3_DEPCMD_ENDTRANSFER        (0x08 << 0)
#define DWC3_DEPCMD_UPDATETRANSFER     (0x07 << 0)
#define DWC3_DEPCMD_STARTTRANSFER      (0x06 << 0)
#define DWC3_DEPCMD_CLEARSTALL         (0x05 << 0)
#define DWC3_DEPCMD_SETSTALL           (0x04 << 0)
#define DWC3_DEPCMD_GETEPSTATE         (0x03 << 0)
#define DWC3_DEPCMD_SETTRANSFRESOURCE  (0x02 << 0)
#define DWC3_DEPCMD_SETEPCONFIG        (0x01 << 0)

// DWC3 endpoint event definitions
#define DEPEVT_TRANSFER_NO_RESOURCE    1
#define DEPEVT_TRANSFER_BUS_EXPIRY     2

/**
 * @brief Macro to define a trace event handler class
 *
 * This macro creates a class that inherits from TraceEvent and
 * implements the handle() method for specific event formatting.
 * Uses the base class default print_log implementation.
 *
 * @param name The base name of the event (e.g., "sched_switch")
 */
#define DEFINE_EVENT(name)                                      \
class name##_event : public TraceEvent {                        \
    public:                                                     \
    void handle(ulong addr) override;                           \
};

/**
 * @brief Macro to define a trace event handler class with custom print_log
 *
 * This macro creates a class that inherits from TraceEvent and
 * implements both handle() and print_log() methods for events that
 * need custom formatting logic.
 *
 * @param name The base name of the event (e.g., "rwmmio_read")
 */
#define DEFINE_EVENT_WITH_CUSTOM_LOG(name)                      \
class name##_event : public TraceEvent {                        \
    public:                                                     \
    void handle(ulong addr) override;                           \
    void print_log(std::ostringstream& oss) override;           \
};

// Core ftrace events
DEFINE_EVENT(bprint)                        /**< Binary print event handler */
DEFINE_EVENT(print)                         /**< Print event handler */
DEFINE_EVENT(kernel_stack)                  /**< Kernel stack trace event handler */
DEFINE_EVENT(user_stack)                    /**< User stack trace event handler */
DEFINE_EVENT(bputs)                         /**< Binary puts event handler */

// Scheduler events
DEFINE_EVENT(sched_switch)                  /**< Scheduler context switch event handler */

// Interrupt events
DEFINE_EVENT(softirq_raise)                 /**< Soft IRQ raise event handler */
DEFINE_EVENT(softirq_entry)                 /**< Soft IRQ entry event handler */
DEFINE_EVENT(softirq_exit)                  /**< Soft IRQ exit event handler */
DEFINE_EVENT(irq_handler_exit)              /**< IRQ handler exit event handler */

// Binder IPC events
DEFINE_EVENT(binder_return)                 /**< Binder return event handler */
DEFINE_EVENT(binder_command)                /**< Binder command event handler */

// DWC3 USB controller events
DEFINE_EVENT(dwc3_ep_queue)                 /**< DWC3 endpoint queue event handler */
DEFINE_EVENT(dwc3_ep_dequeue)               /**< DWC3 endpoint dequeue event handler */
DEFINE_EVENT(dwc3_prepare_trb)              /**< DWC3 prepare TRB event handler */
DEFINE_EVENT(dwc3_gadget_giveback)          /**< DWC3 gadget giveback event handler */
DEFINE_EVENT(dwc3_gadget_ep_cmd)            /**< DWC3 gadget endpoint command event handler */
DEFINE_EVENT(dwc3_event)                    /**< DWC3 general event handler */
DEFINE_EVENT(dwc3_complete_trb)             /**< DWC3 complete TRB event handler */

// DWC3 request events
DEFINE_EVENT(dwc3_alloc_request)            /**< DWC3 alloc request event handler */
DEFINE_EVENT(dwc3_free_request)             /**< DWC3 free request event handler */

// DWC3 endpoint events
DEFINE_EVENT(dwc3_gadget_ep_enable)         /**< DWC3 gadget EP enable event handler */
DEFINE_EVENT(dwc3_gadget_ep_disable)        /**< DWC3 gadget EP disable event handler */

// Memory-mapped I/O events
DEFINE_EVENT_WITH_CUSTOM_LOG(rwmmio_read)   /**< Read MMIO event handler */
DEFINE_EVENT_WITH_CUSTOM_LOG(rwmmio_write)  /**< Write MMIO event handler */
DEFINE_EVENT_WITH_CUSTOM_LOG(rwmmio_post_read) /**< Post-read MMIO event handler */
DEFINE_EVENT_WITH_CUSTOM_LOG(rwmmio_post_write) /**< Post-write MMIO event handler */

// GPIO events
DEFINE_EVENT(gpio_value)                    /**< GPIO value change event handler */
DEFINE_EVENT(gpio_direction)                /**< GPIO direction change event handler */

// DMA events
DEFINE_EVENT(dma_map_page)                  /**< DMA page mapping event handler */
DEFINE_EVENT(dma_unmap_page)                /**< DMA page unmapping event handler */
DEFINE_EVENT(dma_map_resource)              /**< DMA resource mapping event handler */
DEFINE_EVENT(dma_unmap_resource)            /**< DMA resource unmapping event handler */
DEFINE_EVENT(dma_alloc)                     /**< DMA allocation event handler */
DEFINE_EVENT(dma_free)                      /**< DMA free event handler */
DEFINE_EVENT(dma_map_sg)                    /**< DMA scatter-gather mapping event handler */
DEFINE_EVENT(dma_unmap_sg)                  /**< DMA scatter-gather unmapping event handler */
DEFINE_EVENT(dma_sync_single_for_cpu)       /**< DMA sync single for CPU event handler */
DEFINE_EVENT(dma_sync_single_for_device)    /**< DMA sync single for device event handler */
DEFINE_EVENT(dma_sync_sg_for_cpu)           /**< DMA sync scatter-gather for CPU event handler */
DEFINE_EVENT(dma_sync_sg_for_device)        /**< DMA sync scatter-gather for device event handler */
DEFINE_EVENT(swiotlb_bounced)               /**< SWIOTLB bounce event handler */

DEFINE_EVENT(sys_enter)
DEFINE_EVENT(hrtimer_init)
DEFINE_EVENT(hrtimer_start)
DEFINE_EVENT(timer_start)
DEFINE_EVENT(tick_stop)

// Alarmtimer events
DEFINE_EVENT(alarmtimer_suspend)            /**< Alarmtimer suspend event handler */
DEFINE_EVENT(alarmtimer_fired)              /**< Alarmtimer fired event handler */
DEFINE_EVENT(alarmtimer_start)              /**< Alarmtimer start event handler */
DEFINE_EVENT(alarmtimer_cancel)             /**< Alarmtimer cancel event handler */

// CPU idle and power management events
DEFINE_EVENT(cpu_idle_miss)                 /**< CPU idle miss event handler */
DEFINE_EVENT(suspend_resume)                /**< Suspend/resume event handler */

// Memory management events
DEFINE_EVENT(mm_lru_insertion)              /**< Memory LRU insertion event handler */
DEFINE_EVENT(mm_vmscan_wakeup_kswapd)       /**< VM scan wakeup kswapd event handler */
DEFINE_EVENT(mm_vmscan_direct_reclaim_begin) /**< VM scan direct reclaim begin event handler */
DEFINE_EVENT(mm_vmscan_memcg_reclaim_begin) /**< VM scan memcg reclaim begin event handler */
DEFINE_EVENT(mm_vmscan_memcg_softlimit_reclaim_begin) /**< VM scan memcg softlimit reclaim begin event handler */
DEFINE_EVENT(mm_vmscan_node_reclaim_begin)  /**< VM scan node reclaim begin event handler */
DEFINE_EVENT(mm_shrink_slab_start)          /**< Shrink slab start event handler */
DEFINE_EVENT(kmalloc)                       /**< Kmalloc event handler */
DEFINE_EVENT(kmem_cache_alloc)              /**< Kmem cache alloc event handler */
DEFINE_EVENT(kmalloc_node)                  /**< Kmalloc node event handler */
DEFINE_EVENT(kmem_cache_alloc_node)         /**< Kmem cache alloc node event handler */
DEFINE_EVENT(mm_page_free)                  /**< Page free event handler */
DEFINE_EVENT(mm_page_free_batched)          /**< Page free batched event handler */
DEFINE_EVENT(mm_page_alloc)                 /**< Page alloc event handler */
DEFINE_EVENT(mm_page_alloc_zone_locked)     /**< Page alloc zone locked event handler */
DEFINE_EVENT(mm_page_pcpu_drain)            /**< Page per-CPU drain event handler */
DEFINE_EVENT(mm_page_alloc_extfrag)         /**< Page alloc external fragmentation event handler */
DEFINE_EVENT(rss_stat)                      /**< RSS stat event handler */

// Writeback events
DEFINE_EVENT(writeback_mark_inode_dirty)    /**< Writeback mark inode dirty event handler */
DEFINE_EVENT(writeback_dirty_inode_start)   /**< Writeback dirty inode start event handler */
DEFINE_EVENT(writeback_dirty_inode)         /**< Writeback dirty inode event handler */
DEFINE_EVENT(writeback_queue)               /**< Writeback queue event handler */
DEFINE_EVENT(writeback_exec)                /**< Writeback exec event handler */
DEFINE_EVENT(writeback_start)               /**< Writeback start event handler */
DEFINE_EVENT(writeback_written)             /**< Writeback written event handler */
DEFINE_EVENT(writeback_wait)                /**< Writeback wait event handler */
DEFINE_EVENT(writeback_queue_io)            /**< Writeback queue IO event handler */
DEFINE_EVENT(writeback_sb_inodes_requeue)   /**< Writeback sb inodes requeue event handler */
DEFINE_EVENT(writeback_single_inode_start)  /**< Writeback single inode start event handler */
DEFINE_EVENT(writeback_single_inode)        /**< Writeback single inode event handler */
DEFINE_EVENT(writeback_lazytime)            /**< Writeback lazytime event handler */
DEFINE_EVENT(writeback_lazytime_iput)       /**< Writeback lazytime iput event handler */
DEFINE_EVENT(writeback_dirty_inode_enqueue) /**< Writeback dirty inode enqueue event handler */
DEFINE_EVENT(sb_mark_inode_writeback)       /**< SB mark inode writeback event handler */
DEFINE_EVENT(sb_clear_inode_writeback)      /**< SB clear inode writeback event handler */

// SCSI events
DEFINE_EVENT(scsi_dispatch_cmd_start)       /**< SCSI dispatch command start event handler */
DEFINE_EVENT(scsi_dispatch_cmd_done)        /**< SCSI dispatch command done event handler */
DEFINE_EVENT(scsi_dispatch_cmd_error)       /**< SCSI dispatch command error event handler */
DEFINE_EVENT(scsi_dispatch_cmd_timeout)     /**< SCSI dispatch command timeout event handler */

// Memory events
DEFINE_EVENT(mem_connect)                   /**< Memory connect event handler */
DEFINE_EVENT(mem_disconnect)                /**< Memory disconnect event handler */
DEFINE_EVENT(mem_return_failed)             /**< Memory return failed event handler */
DEFINE_EVENT(mm_filemap_add_to_page_cache)  /**< Filemap add to page cache event handler */
DEFINE_EVENT(mm_filemap_delete_from_page_cache) /**< Filemap delete from page cache event handler */
DEFINE_EVENT(reclaim_retry_zone)            /**< Reclaim retry zone event handler */
DEFINE_EVENT(compact_retry)                 /**< Compact retry event handler */

// File lock events
DEFINE_EVENT(posix_lock_inode)              /**< POSIX lock inode event handler */
DEFINE_EVENT(fcntl_setlk)                   /**< Fcntl setlk event handler */
DEFINE_EVENT(locks_remove_posix)            /**< Locks remove POSIX event handler */
DEFINE_EVENT(flock_lock_inode)              /**< Flock lock inode event handler */
DEFINE_EVENT(break_lease_noblock)           /**< Break lease noblock event handler */
DEFINE_EVENT(break_lease_block)             /**< Break lease block event handler */
DEFINE_EVENT(break_lease_unblock)           /**< Break lease unblock event handler */
DEFINE_EVENT(generic_delete_lease)          /**< Generic delete lease event handler */
DEFINE_EVENT(time_out_leases)               /**< Time out leases event handler */
DEFINE_EVENT(generic_add_lease)             /**< Generic add lease event handler */
DEFINE_EVENT(leases_conflict)               /**< Leases conflict event handler */

// Filesystem events (key events only)
DEFINE_EVENT(iomap_iter)                    /**< Iomap iter event handler */
DEFINE_EVENT(ext4_allocate_blocks)          /**< Ext4 allocate blocks event handler */
DEFINE_EVENT(ext4_free_blocks)              /**< Ext4 free blocks event handler */
DEFINE_EVENT(ext4_ext_map_blocks_enter)     /**< Ext4 ext map blocks enter event handler */
DEFINE_EVENT(ext4_ext_map_blocks_exit)      /**< Ext4 ext map blocks exit event handler */

// Scheduler advanced events
DEFINE_EVENT(sched_switch_with_ctrs)        /**< Sched switch with counters event handler */
DEFINE_EVENT(sched_enq_deq_task)            /**< Sched enqueue/dequeue task event handler */

// Devfreq events
DEFINE_EVENT(devfreq_monitor)               /**< Devfreq monitor event handler */
DEFINE_EVENT(devfreq_frequency)             /**< Devfreq frequency event handler */

// UFS events
DEFINE_EVENT(ufshcd_command)                /**< UFS command event handler */
DEFINE_EVENT(ufshcd_clk_gating)             /**< UFS clock gating event handler */
DEFINE_EVENT(ufshcd_upiu)                   /**< UFS UPIU event handler */
DEFINE_EVENT(ufshcd_uic_command)            /**< UFS UIC command event handler */
DEFINE_EVENT(ufshcd_wl_runtime_resume)      /**< UFS WL runtime resume event handler */
DEFINE_EVENT(ufshcd_wl_runtime_suspend)     /**< UFS WL runtime suspend event handler */
DEFINE_EVENT(ufshcd_wl_resume)              /**< UFS WL resume event handler */
DEFINE_EVENT(ufshcd_wl_suspend)             /**< UFS WL suspend event handler */
DEFINE_EVENT(ufshcd_init)                   /**< UFS init event handler */
DEFINE_EVENT(ufshcd_runtime_resume)         /**< UFS runtime resume event handler */
DEFINE_EVENT(ufshcd_runtime_suspend)        /**< UFS runtime suspend event handler */
DEFINE_EVENT(ufshcd_system_resume)          /**< UFS system resume event handler */
DEFINE_EVENT(ufshcd_system_suspend)         /**< UFS system suspend event handler */

// Scheduler WALT events
DEFINE_EVENT(sched_update_task_ravg)        /**< Sched update task RAVG event handler */
DEFINE_EVENT(sched_update_history)          /**< Sched update history event handler */
DEFINE_EVENT(sched_update_pred_demand)      /**< Sched update pred demand event handler */

// Filesystem additional events
DEFINE_EVENT(locks_get_lock_context)        /**< Locks get lock context event handler */
DEFINE_EVENT(iomap_iter_dstmap)             /**< Iomap iter dstmap event handler */
DEFINE_EVENT(iomap_iter_srcmap)             /**< Iomap iter srcmap event handler */
DEFINE_EVENT(ext4_da_write_pages_extent)    /**< Ext4 da write pages extent event handler */
DEFINE_EVENT(ext4_request_blocks)           /**< Ext4 request blocks event handler */
DEFINE_EVENT(ext4_mballoc_alloc)            /**< Ext4 mballoc alloc event handler */
DEFINE_EVENT(ext4_fallocate_enter)          /**< Ext4 fallocate enter event handler */
DEFINE_EVENT(ext4_punch_hole)               /**< Ext4 punch hole event handler */
DEFINE_EVENT(ext4_zero_range)               /**< Ext4 zero range event handler */
DEFINE_EVENT(ext4_ind_map_blocks_enter)     /**< Ext4 ind map blocks enter event handler */
DEFINE_EVENT(ext4_ind_map_blocks_exit)      /**< Ext4 ind map blocks exit event handler */
DEFINE_EVENT(ext4_ext_handle_unwritten_extents) /**< Ext4 ext handle unwritten extents event handler */
DEFINE_EVENT(ext4_get_implied_cluster_alloc_exit) /**< Ext4 get implied cluster alloc exit event handler */
DEFINE_EVENT(ext4_es_insert_extent)         /**< Ext4 es insert extent event handler */
DEFINE_EVENT(ext4_es_cache_extent)          /**< Ext4 es cache extent event handler */
DEFINE_EVENT(ext4_es_find_extent_range_exit) /**< Ext4 es find extent range exit event handler */
DEFINE_EVENT(ext4_es_lookup_extent_exit)    /**< Ext4 es lookup extent exit event handler */
DEFINE_EVENT(ext4_es_insert_delayed_block)  /**< Ext4 es insert delayed block event handler */

// Memory management additional events
DEFINE_EVENT(mm_vmscan_lru_isolate)         /**< VM scan LRU isolate event handler */
DEFINE_EVENT(mm_vmscan_writepage)           /**< VM scan writepage event handler */
DEFINE_EVENT(mm_vmscan_lru_shrink_inactive) /**< VM scan LRU shrink inactive event handler */
DEFINE_EVENT(mm_vmscan_lru_shrink_active)   /**< VM scan LRU shrink active event handler */
DEFINE_EVENT(mm_compaction_begin)           /**< Memory compaction begin event handler */
DEFINE_EVENT(mm_compaction_end)             /**< Memory compaction end event handler */
DEFINE_EVENT(mm_compaction_try_to_compact_pages) /**< Memory compaction try event handler */
DEFINE_EVENT(mm_compaction_finished)        /**< Memory compaction finished event handler */
DEFINE_EVENT(mm_compaction_suitable)        /**< Memory compaction suitable event handler */
DEFINE_EVENT(mm_compaction_deferred)        /**< Memory compaction deferred event handler */
DEFINE_EVENT(mm_compaction_defer_compaction) /**< Memory compaction defer event handler */
DEFINE_EVENT(mm_compaction_defer_reset)     /**< Memory compaction defer reset event handler */
DEFINE_EVENT(mm_compaction_wakeup_kcompactd) /**< Memory compaction wakeup kcompactd event handler */
DEFINE_EVENT(mm_compaction_kcompactd_wake)  /**< Memory compaction kcompactd wake event handler */
DEFINE_EVENT(mmap_lock_start_locking)       /**< Mmap lock start locking event handler */
DEFINE_EVENT(mmap_lock_acquire_returned)    /**< Mmap lock acquire returned event handler */
DEFINE_EVENT(mmap_lock_released)            /**< Mmap lock released event handler */
DEFINE_EVENT(vm_unmapped_area)              /**< VM unmapped area event handler */
DEFINE_EVENT(mm_migrate_pages)              /**< Page migration event handler */
DEFINE_EVENT(mm_migrate_pages_start)        /**< Page migration start event handler */
DEFINE_EVENT(mm_khugepaged_scan_pmd)        /**< Khugepaged scan PMD event handler */
DEFINE_EVENT(mm_collapse_huge_page)         /**< Collapse huge page event handler */
DEFINE_EVENT(mm_collapse_huge_page_isolate) /**< Collapse huge page isolate event handler */

// SPMI events
DEFINE_EVENT(spmi_read_end)                 /**< SPMI read end event handler */
DEFINE_EVENT(spmi_write_begin)              /**< SPMI write begin event handler */

// SPI events
DEFINE_EVENT(spi_transfer_start)            /**< SPI transfer start event handler */
DEFINE_EVENT(spi_transfer_stop)             /**< SPI transfer stop event handler */
DEFINE_EVENT(spi_set_cs)                    /**< SPI set CS event handler */
DEFINE_EVENT(spi_setup)                     /**< SPI setup event handler */

// USB Gadget request events
DEFINE_EVENT(usb_gadget_giveback_request)   /**< USB gadget giveback request event handler */
DEFINE_EVENT(usb_ep_dequeue)                /**< USB EP dequeue event handler */
DEFINE_EVENT(usb_ep_queue)                  /**< USB EP queue event handler */
DEFINE_EVENT(usb_ep_free_request)           /**< USB EP free request event handler */
DEFINE_EVENT(usb_ep_alloc_request)          /**< USB EP alloc request event handler */

// USB Gadget endpoint events
DEFINE_EVENT(usb_ep_enable)                 /**< USB EP enable event handler */
DEFINE_EVENT(usb_ep_disable)                /**< USB EP disable event handler */
DEFINE_EVENT(usb_ep_set_halt)               /**< USB EP set halt event handler */
DEFINE_EVENT(usb_ep_clear_halt)             /**< USB EP clear halt event handler */
DEFINE_EVENT(usb_ep_set_wedge)              /**< USB EP set wedge event handler */
DEFINE_EVENT(usb_ep_fifo_status)            /**< USB EP fifo status event handler */
DEFINE_EVENT(usb_ep_fifo_flush)             /**< USB EP fifo flush event handler */
DEFINE_EVENT(usb_ep_set_maxpacket_limit)    /**< USB EP set maxpacket limit event handler */

// USB Gadget device events
DEFINE_EVENT(usb_gadget_frame_number)       /**< USB gadget frame number event handler */
DEFINE_EVENT(usb_gadget_wakeup)             /**< USB gadget wakeup event handler */
DEFINE_EVENT(usb_gadget_set_remote_wakeup)  /**< USB gadget set remote wakeup event handler */
DEFINE_EVENT(usb_gadget_set_selfpowered)    /**< USB gadget set selfpowered event handler */
DEFINE_EVENT(usb_gadget_clear_selfpowered)  /**< USB gadget clear selfpowered event handler */
DEFINE_EVENT(usb_gadget_vbus_connect)       /**< USB gadget vbus connect event handler */
DEFINE_EVENT(usb_gadget_vbus_draw)          /**< USB gadget vbus draw event handler */
DEFINE_EVENT(usb_gadget_vbus_disconnect)    /**< USB gadget vbus disconnect event handler */
DEFINE_EVENT(usb_gadget_connect)            /**< USB gadget connect event handler */
DEFINE_EVENT(usb_gadget_disconnect)         /**< USB gadget disconnect event handler */
DEFINE_EVENT(usb_gadget_deactivate)         /**< USB gadget deactivate event handler */
DEFINE_EVENT(usb_gadget_activate)           /**< USB gadget activate event handler */

// xHCI DBC events
DEFINE_EVENT(xhci_dbc_alloc_request)        /**< xHCI DBC alloc request event handler */
DEFINE_EVENT(xhci_dbc_free_request)         /**< xHCI DBC free request event handler */
DEFINE_EVENT(xhci_dbc_queue_request)        /**< xHCI DBC queue request event handler */
DEFINE_EVENT(xhci_dbc_giveback_request)     /**< xHCI DBC giveback request event handler */

// xHCI URB events
DEFINE_EVENT(xhci_urb_enqueue)              /**< xHCI URB enqueue event handler */
DEFINE_EVENT(xhci_urb_giveback)             /**< xHCI URB giveback event handler */
DEFINE_EVENT(xhci_urb_dequeue)              /**< xHCI URB dequeue event handler */

// F2FS compression events
DEFINE_EVENT(f2fs_compress_pages_start)     /**< F2FS compress pages start event handler */
DEFINE_EVENT(f2fs_decompress_pages_start)   /**< F2FS decompress pages start event handler */

// F2FS page operation events
DEFINE_EVENT(f2fs_writepage)                /**< F2FS writepage event handler */
DEFINE_EVENT(f2fs_do_write_data_page)       /**< F2FS do write data page event handler */
DEFINE_EVENT(f2fs_readpage)                 /**< F2FS readpage event handler */
DEFINE_EVENT(f2fs_set_page_dirty)           /**< F2FS set page dirty event handler */
DEFINE_EVENT(f2fs_vm_page_mkwrite)          /**< F2FS vm page mkwrite event handler */

// F2FS BIO events
DEFINE_EVENT(f2fs_submit_page_bio)          /**< F2FS submit page bio event handler */
DEFINE_EVENT(f2fs_submit_page_write)        /**< F2FS submit page write event handler */
DEFINE_EVENT(f2fs_prepare_write_bio)        /**< F2FS prepare write bio event handler */
DEFINE_EVENT(f2fs_prepare_read_bio)         /**< F2FS prepare read bio event handler */
DEFINE_EVENT(f2fs_submit_read_bio)          /**< F2FS submit read bio event handler */
DEFINE_EVENT(f2fs_submit_write_bio)         /**< F2FS submit write bio event handler */

// F2FS GC events
DEFINE_EVENT(f2fs_gc_begin)                 /**< F2FS GC begin event handler */
DEFINE_EVENT(f2fs_get_victim)               /**< F2FS get victim event handler */

// F2FS sync events
DEFINE_EVENT(f2fs_sync_file_exit)           /**< F2FS sync file exit event handler */
DEFINE_EVENT(f2fs_sync_fs)                  /**< F2FS sync fs event handler */
DEFINE_EVENT(f2fs_write_checkpoint)         /**< F2FS write checkpoint event handler */
DEFINE_EVENT(f2fs_issue_flush)              /**< F2FS issue flush event handler */
DEFINE_EVENT(f2fs_sync_dirty_inodes_enter)  /**< F2FS sync dirty inodes enter event handler */
DEFINE_EVENT(f2fs_sync_dirty_inodes_exit)   /**< F2FS sync dirty inodes exit event handler */

// F2FS extent tree events
DEFINE_EVENT(f2fs_lookup_extent_tree_start) /**< F2FS lookup extent tree start event handler */
DEFINE_EVENT(f2fs_shrink_extent_tree)       /**< F2FS shrink extent tree event handler */
DEFINE_EVENT(f2fs_destroy_extent_tree)      /**< F2FS destroy extent tree event handler */

// F2FS other events
DEFINE_EVENT(f2fs_truncate_partial_nodes)   /**< F2FS truncate partial nodes event handler */
DEFINE_EVENT(f2fs_writepages)               /**< F2FS writepages event handler */
DEFINE_EVENT(f2fs_shutdown)                 /**< F2FS shutdown event handler */

// V4L2 videobuf2 events
DEFINE_EVENT(vb2_v4l2_buf_done)             /**< VB2 V4L2 buf done event handler */
DEFINE_EVENT(vb2_v4l2_buf_queue)            /**< VB2 V4L2 buf queue event handler */
DEFINE_EVENT(vb2_v4l2_dqbuf)                /**< VB2 V4L2 dqbuf event handler */
DEFINE_EVENT(vb2_v4l2_qbuf)                 /**< VB2 V4L2 qbuf event handler */

// V4L2 events
DEFINE_EVENT(v4l2_dqbuf)                    /**< V4L2 dqbuf event handler */
DEFINE_EVENT(v4l2_qbuf)                     /**< V4L2 qbuf event handler */

// I2C events
DEFINE_EVENT(i2c_write)                     /**< I2C write event handler */
DEFINE_EVENT(i2c_reply)                     /**< I2C reply event handler */
DEFINE_EVENT(i2c_read)                      /**< I2C read event handler */
DEFINE_EVENT(i2c_result)                    /**< I2C result event handler */

// SMBus events
DEFINE_EVENT(smbus_write)                   /**< SMBus write event handler */
DEFINE_EVENT(smbus_read)                    /**< SMBus read event handler */
DEFINE_EVENT(smbus_reply)                   /**< SMBus reply event handler */
DEFINE_EVENT(smbus_result)                  /**< SMBus result event handler */

// Thermal events
DEFINE_EVENT(thermal_zone_trip)             /**< Thermal zone trip event handler */
DEFINE_EVENT(thermal_power_cpu_get_power)   /**< Thermal power CPU get power event handler */
DEFINE_EVENT(thermal_power_devfreq_get_power) /**< Thermal power devfreq get power event handler */
DEFINE_EVENT(thermal_power_allocator)       /**< Thermal power allocator event handler */

// RCU events
DEFINE_EVENT(rcu_batch_end)                 /**< RCU batch end event handler */
DEFINE_EVENT(rcu_segcb_stats)               /**< RCU segcb stats event handler */

// SMC Invoke events
DEFINE_EVENT(smcinvoke_ioctl)               /**< SMC invoke ioctl event handler */

DEFINE_EVENT(module_load)                   /**< module event handler */
#endif // EVENTS_DEFS_H_
