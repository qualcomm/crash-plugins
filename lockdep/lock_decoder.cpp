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

#include "lock_decoder.h"
#include <sstream>
#include <iomanip>

/*
 * mutex.owner  (atomic_long_t)
 *   bits[63:3]  task_struct pointer
 *   bit 0  MUTEX_FLAG_WAITERS
 *   bit 1  MUTEX_FLAG_HANDOFF
 *   bit 2  MUTEX_FLAG_PICKUP
 */
std::string decode_mutex_owner(ulong raw) {
    std::ostringstream oss;
    ulong task   = mutex_owner_task(raw);
    bool waiters = (raw & MUTEX_FLAG_WAITERS) != 0;
    bool handoff = (raw & MUTEX_FLAG_HANDOFF) != 0;
    bool pickup  = (raw & MUTEX_FLAG_PICKUP)  != 0;

    if (!task) {
        oss << "UNLOCKED";
        if (waiters || handoff || pickup)
            oss << "  [raw=0x" << std::hex << raw << " — no owner but flags set, suspicious]";
        return oss.str();
    }

    oss << "LOCKED  owner=0x" << std::hex << task;
    if (waiters || handoff || pickup) {
        oss << "\n    flags:";
        if (waiters) oss << "\n      WAITERS  — other tasks are queued waiting for this lock";
        if (handoff) oss << "\n      HANDOFF  — when unlocked, the lock goes directly to the"
                            " longest-waiting task (kernel anti-starvation, not a normal wakeup)";
        if (pickup)  oss << "\n      PICKUP   — the next waiter has been chosen and is"
                            " in the process of taking ownership";
    }
    return oss.str();
}

/*
 * rw_semaphore.count  (atomic_long_t)
 *   bit 0  RWSEM_WRITER_LOCKED
 *   bit 1  RWSEM_FLAG_WAITERS
 *   bit 2  RWSEM_FLAG_HANDOFF
 *   bits[63:8]  active reader count
 */
std::string decode_rwsem_count(ulong raw) {
    std::ostringstream oss;
    if (raw == 0) {
        oss << "UNLOCKED";
        return oss.str();
    }
    bool writer  = (raw & RWSEM_WRITER_LOCKED) != 0;
    bool waiters = (raw & RWSEM_FLAG_WAITERS)  != 0;
    bool handoff = (raw & RWSEM_FLAG_HANDOFF)  != 0;
    long readers = (long)(raw >> RWSEM_READER_SHIFT);

    if (writer) {
        oss << "WRITE-LOCKED";
    } else if (readers > 0) {
        oss << "READ-LOCKED  active-readers=" << std::dec << readers;
    } else {
        oss << "unknown state  raw=0x" << std::hex << raw;
    }
    if (waiters || handoff) {
        oss << "\n    flags:";
        if (waiters) oss << "\n      WAITERS  — tasks are queued waiting for this lock";
        if (handoff) oss << "\n      HANDOFF  — a pending writer is next in line"
                            " (kernel ensures the writer gets it before new readers)";
    }
    return oss.str();
}

/*
 * rw_semaphore.owner  (atomic_long_t)
 *   bit 0  RWSEM_READER_OWNED — set when any reader holds it
 *   bit 1  RWSEM_NONSPINNABLE — readers can't spin optimistically
 *   bits[63:2]  task_struct pointer (writer, or a hint-reader when read-locked)
 *
 * States:
 *   0             no owner recorded (unlocked or just released)
 *   T | flags=0   write-locked by T
 *   * | READER_OWNED  read-locked; T is one reader as a hint (may not be unique)
 */
std::string decode_rwsem_owner(ulong raw) {
    std::ostringstream oss;
    bool reader_owned  = (raw & RWSEM_READER_OWNED)  != 0;
    bool nonspinnable  = (raw & RWSEM_NONSPINNABLE)   != 0;
    ulong task         = rwsem_owner_task(raw);

    if (!raw) {
        oss << "no owner recorded";
        return oss.str();
    }
    if (reader_owned) {
        oss << "READ  (one reader at 0x" << std::hex << task
            << " — hint only, there may be more readers)";
    } else if (task) {
        oss << "WRITE  owner=0x" << std::hex << task;
    } else {
        oss << "raw=0x" << std::hex << raw << " (flags only, no task ptr)";
    }
    if (nonspinnable)
        oss << "\n    NONSPINNABLE — readers will not spin on this lock";
    return oss.str();
}

/*
 *   rt_mutex_base.owner  (task_struct *)
 *   bit 0  RT_MUTEX_HAS_WAITERS
 *   bits[63:1]  task_struct pointer
 *
 * States:
 *   0x0       no owner, no waiters
 *   0x1       no owner, but waiters exist — lock was just released and is
 *             being handed to the top waiter right now (very brief window)
 *   T         owned by T, no other waiters
 *   T | 0x1   owned by T, other tasks are queued
 *
 * On PREEMPT_RT, spinlock_t is backed by rt_mutex_base, same decoding applies.
 */
std::string decode_rt_mutex_owner(ulong raw) {
    std::ostringstream oss;
    ulong task        = rt_mutex_owner_task(raw);
    bool  has_waiters = (raw & RT_MUTEX_HAS_WAITERS) != 0;

    if (!task && !has_waiters) {
        oss << "UNLOCKED  (nobody waiting)";
    } else if (!task && has_waiters) {
        oss << "UNLOCKED — handoff in progress\n"
               "    The previous owner just released the lock. The kernel is waking\n"
               "    up the highest-priority waiter (rb_leftmost) to take it.\n"
               "    This is a very short-lived state; a new owner will appear shortly.";
    } else {
        oss << "LOCKED  owner=0x" << std::hex << task;
        if (has_waiters)
            oss << "\n    flags: HAS_WAITERS — other tasks are queued in the waiters tree";
    }
    return oss.str();
}
