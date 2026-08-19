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
 *
 * lock_decoder.h — Human-readable decode of kernel lock state fields.
 *
 * Covers the low-bit flag encodings that are embedded in lock owner/count
 * fields and that would otherwise require looking up kernel source.
 */

#ifndef LOCK_DECODER_H_
#define LOCK_DECODER_H_

#include <string>
#include "plugin.h"

/* ---- mutex.owner (atomic_long_t) ----------------------------------------
 *   bits [63:3]  task_struct pointer
 *   bit  [2]     MUTEX_FLAG_PICKUP   — top waiter is picking up the lock
 *   bit  [1]     MUTEX_FLAG_HANDOFF  — unlock will hand off to top waiter
 *   bit  [0]     MUTEX_FLAG_WAITERS  — wait_list is non-empty
 */
#define MUTEX_FLAG_WAITERS  0x01UL
#define MUTEX_FLAG_HANDOFF  0x02UL
#define MUTEX_FLAG_PICKUP   0x04UL
#define MUTEX_FLAG_MASK     0x07UL

/* ---- rw_semaphore.count (atomic_long_t) ----------------------------------
 *   bit  [0]     RWSEM_WRITER_LOCKED — write-locked
 *   bit  [1]     RWSEM_FLAG_WAITERS  — wait_list is non-empty
 *   bit  [2]     RWSEM_FLAG_HANDOFF  — hand-off to top waiter pending
 *   bits [63:8]  active reader count (each reader adds RWSEM_READER_BIAS)
 */
#define RWSEM_WRITER_LOCKED (1UL << 0)
#define RWSEM_FLAG_WAITERS  (1UL << 1)
#define RWSEM_FLAG_HANDOFF  (1UL << 2)
#define RWSEM_READER_SHIFT  8
#define RWSEM_READER_BIAS   (1UL << RWSEM_READER_SHIFT)

/* ---- rw_semaphore.owner (atomic_long_t) ----------------------------------
 *   bit 0  RWSEM_READER_OWNED — lock is held by a reader (hint only; may be
 *                               any one of several concurrent readers)
 *   bit 1  RWSEM_NONSPINNABLE — readers cannot optimistically spin on this sem
 *   bits[63:2]  task_struct pointer of the writer (valid when write-locked;
 *               set to a reader as a hint when read-locked)
 */
#define RWSEM_READER_OWNED   (1UL << 0)
#define RWSEM_NONSPINNABLE   (1UL << 1)
#define RWSEM_OWNER_FLAGS    (RWSEM_READER_OWNED | RWSEM_NONSPINNABLE)

static inline ulong rwsem_owner_task(ulong owner_raw) {
    return owner_raw & ~RWSEM_OWNER_FLAGS;
}

std::string decode_rwsem_owner(ulong raw);
#define RT_MUTEX_HAS_WAITERS 1UL
#define RT_MUTEX_OWNER_MASK  (~RT_MUTEX_HAS_WAITERS)

static inline ulong mutex_owner_task(ulong owner_raw) {
    return owner_raw & ~MUTEX_FLAG_MASK;
}
static inline ulong rt_mutex_owner_task(ulong owner_raw) {
    return (ulong)(owner_raw & RT_MUTEX_OWNER_MASK);
}

/* Decode helpers — return a compact human-readable state string. */
std::string decode_mutex_owner(ulong raw);
std::string decode_rwsem_count(ulong raw);
std::string decode_rt_mutex_owner(ulong raw);

#endif /* LOCK_DECODER_H_ */
