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

#include "lockdep.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(Lockdep)
#endif

Lockdep::Lockdep() {
    lock_classes_in_use_bitmap = 0;
    lock_classes = 0;
    sizeof_held_lock = 0;
    sizeof_lock_class = 0;
}

void Lockdep::cmd_main(void) {
    int c;
    bool summary_mode = false;
    bool detail_mode = false;
    ulong target_task_addr = 0;

    if (argcnt < 2) {
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }

    while ((c = getopt(argcnt, args, "cd:")) != EOF) {
        switch(c) {
            case 'c':
                summary_mode = true;
                break;
            case 'd':
                detail_mode = true;
                target_task_addr = htol(optarg, FAULT_ON_ERROR, NULL);
                break;
            default:
                argerrs++;
                break;
        }
    }

    if (argerrs) {
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }

    // Check if kernel has CONFIG_LOCKDEP enabled
    if (get_config_val("CONFIG_LOCKDEP") != "y") {
        PRINT("CONFIG_LOCKDEP not enabled in kernel\n");
        return;
    }

    if (summary_mode) {
        parse_tasks_summary();
    } else if (detail_mode) {
        parse_single_task_detail(target_task_addr);
    } else {
        cmd_usage(pc->curcmd, SYNOPSIS);
    }
}

void Lockdep::init_offset(void) {
    // Initialize task_struct related field offsets
    field_init(task_struct, held_locks);
    field_init(task_struct, lockdep_depth);
    field_init(task_struct, comm);
    field_init(task_struct, pid);

    // Initialize held_lock structure field offsets
    field_init(held_lock, prev_chain_key);
    field_init(held_lock, acquire_ip);
    field_init(held_lock, acquire_ip_caller);
    field_init(held_lock, instance);
    field_init(held_lock, nest_lock);
    field_init(held_lock, class_idx);
    field_init(held_lock, pin_count);
    field_init(held_lock, waittime_stamp);
    field_init(held_lock, holdtime_stamp);

    // Initialize lock_class structure field offsets
    field_init(lock_class, name);
    field_init(lock_class, key);
    field_init(lock_class, subclass);
    field_init(lock_class, dep_gen_id);

    // Initialize structure sizes
    struct_init(held_lock);
    struct_init(lock_class);

    // Cache structure sizes
    sizeof_held_lock = struct_size(held_lock);
    sizeof_lock_class = struct_size(lock_class);
}

void Lockdep::init_command(void) {
    cmd_name = "lockdep";
    help_str_list = {
        "lockdep",
        "Analyze Linux kernel lock dependency (lockdep) information",
        "-c | -d task_addr\n"
        "  -c: Display summary table of all tasks holding locks\n"
        "  -d task_addr: Display detailed lock information for specific task",
        "\n",
        "Examples:",
        "  Display summary of tasks holding locks:",
        "    %s> lockdep -c",
        "    PID    COMM           LOCKS  TASK_STRUCT",
        "    ------ -------------- ------ ----------------",
        "    1      systemd        1      0xffffff802726ca40",
        "    123    kworker/0:1    2      0xffffff8027123456",
        "    Total: 2 tasks holding locks out of 580 processed",
        "\n",
        "  Display detailed lock info for specific task:",
        "    %s> lockdep -d 0xffffff802726ca40",
        "    1 locks held by systemd (pid=1, task_struct=0xffffff802726ca40):",
        "    #0: 0xffffffe25436dd30 (tty_mutex) at: tty_open",
        "        ├─ Lock Mode : EXCLUSIVE, type=struct mutex",
        "        ├─ Timing    : holdtime=17.049303s, no race (fast path)",
        "        └─ Debug     : chain_key=0xffffffffffffffff (lockdep hash)",
        "\n",
        "Notes:",
        "  - Requires CONFIG_LOCKDEP enabled in kernel",
        "  - Use -c to get task addresses, then -d for details",
        "  - Timing info requires CONFIG_LOCK_STAT",
        "\n",
    };
}

/**
 * Test if a specific bit is set in a bitmap
 * Used to check if a lock class is currently in use by the lockdep system
 *
 * The Linux kernel uses bitmaps to track which lock classes are active.
 * Each bit represents one lock class, and if set, indicates the class is in use.
 *
 * Example: For lock class index 67 on a 64-bit system:
 *   - index = 67 / 64 = 1 (second ulong in bitmap)
 *   - bit_pos = 67 & 63 = 3 (bit 3 within that ulong)
 *
 * @param nr Bit number to test (lock class index)
 * @param addr Base address of the bitmap (lock_classes_in_use)
 * @return true if bit is set (lock class is active), false otherwise
 */
bool Lockdep::test_bit(uint32_t nr, ulong addr) {
    if (!is_kvaddr(addr)) {
        LOGD("Invalid bitmap address: 0x%lx\n", addr);
        return false;
    }

    uint32_t index = nr / BITS_PER_LONG;
    ulong bitmap_entry_addr = addr + index * sizeof(ulong);

    // Validate the specific bitmap entry address
    if (!is_kvaddr(bitmap_entry_addr)) {
        LOGD("Invalid bitmap entry address: 0x%lx for bit %u\n", bitmap_entry_addr, nr);
        return false;
    }

    ulong data = read_ulong(bitmap_entry_addr, "bitmap data");
    return (data >> (nr & (BITS_PER_LONG - 1))) & 1;
}

/**
 * Parse the various flag bits encoded in the class_idx field
 *
 * The class_idx field is a 32-bit value that contains multiple pieces of information
 * packed into different bit ranges. This is a space-efficient way for the kernel
 * to store lock metadata in the held_lock structure.
 *
 * Bit field layout (from kernel lockdep implementation):
 *   Bits  0-12: Lock class index (0x1FFF = 8191 max classes)
 *   Bits 13-14: IRQ context flags (0x6000)
 *   Bit    15:  Trylock flag (0x8000) - non-blocking acquisition
 *   Bits 16-17: Read lock mode (0x30000) - shared/exclusive access
 *   Bit    18:  Check flag (0x40000) - enable dependency checking
 *   Bit    19:  Hard IRQs disabled (0x80000) - interrupt state
 *   Bits 20-31: Reference count (0xFFF00000) - usage tracking
 *
 * Example: class_idx_full = 0x12345678
 *   class_idx = 0x678 (lock class 1656)
 *   irq_context = 0x1 (interrupt context)
 *   trylock = 0x1 (non-blocking)
 *   read_mode = 0x2 (shared recursive)
 *   check = 0x1 (checking enabled)
 *   hardirqs_off = 0x0 (interrupts enabled)
 *   references = 0x123 (291 references)
 *
 * @param class_idx_full Complete 32-bit class_idx value from held_lock
 * @param lock_info Output structure to store parsed bit field information
 */
void Lockdep::parse_class_idx_bits(uint32_t class_idx_full, HeldLockInfo& lock_info) {
    // Extract lock class index (bits 0-12): identifies the specific lock type
    // Range: 0-8191 (0x1FFF), each lock type gets a unique class index
    lock_info.class_idx = class_idx_full & 0x00001FFF;

    // Extract IRQ context flags (bits 13-14): indicates interrupt context state
    // 0=process context, 1=softirq, 2=hardirq, 3=NMI context
    lock_info.irq_context = (class_idx_full & 0x00006000) >> 13;

    // Extract trylock flag (bit 15): indicates acquisition method
    // 0=blocking lock (will wait), 1=trylock (non-blocking, may fail)
    lock_info.trylock = (class_idx_full & 0x00008000) >> 15;

    // Extract read lock mode (bits 16-17): indicates lock access pattern
    // 0=exclusive (write), 1=shared (read), 2=shared recursive
    lock_info.read_mode = (class_idx_full & 0x00030000) >> 16;

    // Extract check flag (bit 18): controls lockdep dependency validation
    // 0=checking disabled, 1=checking enabled (default for most locks)
    lock_info.check = (class_idx_full & 0x00040000) >> 18;

    // Extract hard IRQs disabled flag (bit 19): interrupt state when acquired
    // 0=hard interrupts enabled, 1=hard interrupts disabled
    lock_info.hardirqs_off = (class_idx_full & 0x00080000) >> 19;

    // Extract reference count (bits 20-31): tracks lock usage
    // Range: 0-4095 (0xFFF), used for lock statistics and debugging
    lock_info.references = (class_idx_full & 0xFFF00000) >> 20;
}

/**
 * Get the name of a lock class from the global lock_classes array
 *
 * The Linux kernel maintains a global array of lock classes, where each class
 * represents a unique lock type (e.g., "rcu_read_lock", "mutex", etc.).
 * Each lock class has a name pointer that points to a string in kernel memory.
 *
 * Process:
 * 1. Check if the lock class is active using the bitmap
 * 2. Calculate the address of the lock_class structure
 * 3. Read the name pointer from the structure
 * 4. Read the actual name string from kernel memory
 *
 * Example: For class_idx=123 with sizeof_lock_class=64:
 *   lock_class_addr = lock_classes + 64 * 123 = lock_classes + 7872
 *
 * @param class_idx Lock class index (0-8191, from bits 0-12 of class_idx field)
 * @return Lock name string, or "unknown" if not found/invalid
 */
std::string Lockdep::get_lock_name(uint32_t class_idx) {
    // Validate global lock_classes array address
    if (!is_kvaddr(lock_classes)) {
        LOGD("Invalid lock_classes address: 0x%lx\n", lock_classes);
        return "unknown";
    }

    // Check if the lock class is currently in use
    if (!test_bit(class_idx, lock_classes_in_use_bitmap)) {
        LOGD("Lock class %u is not in use\n", class_idx);
        return "unknown";
    }

    // Calculate the address of the specific lock_class structure
    ulong lock_class_addr = lock_classes + sizeof_lock_class * class_idx;

    // Validate the calculated lock class address
    if (!is_kvaddr(lock_class_addr)) {
        LOGD("Invalid lock_class address: 0x%lx for class_idx %u\n", lock_class_addr, class_idx);
        return "unknown";
    }

    // Calculate the address of the name field within the lock_class structure
    ulong name_field_addr = lock_class_addr + field_offset(lock_class, name);

    // Validate the name field address
    if (!is_kvaddr(name_field_addr)) {
        LOGD("Invalid name field address: 0x%lx\n", name_field_addr);
        return "unknown";
    }

    // Read the name pointer from the lock_class structure
    ulong name_ptr = read_pointer(name_field_addr, "lock class name");
    if (name_ptr == 0) {
        LOGD("Null name pointer for lock class %u\n", class_idx);
        return "unknown";
    }

    // Validate the name string address
    if (!is_kvaddr(name_ptr)) {
        LOGD("Invalid name string address: 0x%lx\n", name_ptr);
        return "unknown";
    }

    // Read the actual lock name string from kernel memory
    return read_cstring(name_ptr, 256, "lock name");
}

/**
 * Convert lock mode value to descriptive text
 *
 * The read_mode field encodes the lock's access pattern, which determines
 * how the lock can be shared among multiple tasks. This is crucial for
 * understanding lock behavior and potential contention.
 *
 * Lock modes in Linux kernel:
 * - EXCLUSIVE (0): Write lock, only one task can hold it (mutual exclusion)
 * - SHARED (1): Read lock, multiple readers can hold simultaneously
 * - SHARED_RECURSIVE (2): Read lock that can be acquired recursively by same task
 *
 * Example: For a rwsem (read-write semaphore):
 *   - Mode 0: Writer has exclusive access, blocks all readers/writers
 *   - Mode 1: Reader has shared access, allows other readers but blocks writers
 *   - Mode 2: Recursive reader, same task can acquire multiple times
 *
 * @param hl_read Lock mode value from held_lock structure
 * @return Human-readable description of the lock mode
 */
std::string Lockdep::get_lock_mode_description(uint32_t hl_read) {
    switch (hl_read) {
        case 0:
            return "EXCLUSIVE (write lock, mutual exclusion)";
        case 1:
            return "SHARED (read lock, multiple readers allowed)";
        case 2:
            return "SHARED_RECURSIVE (read lock, can be acquired recursively)";
        default:
            return "UNKNOWN (" + std::to_string(hl_read) + ")";
    }
}

/**
 * Convert acquisition method flag to descriptive text
 *
 * The trylock flag indicates whether the lock was acquired using a blocking
 * or non-blocking method. This affects system behavior and performance.
 *
 * Acquisition methods:
 * - BLOCKING (0): Will wait indefinitely until lock becomes available
 *   Examples: mutex_lock(), down_read(), spin_lock()
 * - TRYLOCK (1): Non-blocking attempt, fails immediately if unavailable
 *   Examples: mutex_trylock(), down_read_trylock(), spin_trylock()
 *
 * Trylock is often used in scenarios where:
 * - Deadlock avoidance is needed
 * - Performance-critical paths cannot afford to block
 * - Alternative actions can be taken if lock is unavailable
 *
 * @param hl_trylock Trylock flag value from held_lock structure
 * @return Human-readable description of the acquisition method
 */
std::string Lockdep::get_acquire_method_description(uint32_t hl_trylock) {
    if (hl_trylock == 1) {
        return "TRYLOCK (non-blocking attempt, may fail immediately)";
    } else {
        return "BLOCKING (will wait until lock is available)";
    }
}

/**
 * Convert IRQ state flags to descriptive text
 *
 * The IRQ state information is critical for understanding the context in which
 * a lock was acquired and potential deadlock scenarios involving interrupts.
 *
 * Hard IRQ state:
 * - hardirqs_enabled (0): Interrupts were enabled when lock was acquired
 * - hardirqs_disabled (1): Interrupts were disabled (e.g., in spin_lock_irqsave)
 *
 * IRQ context values:
 * - 0: Process context (normal task execution)
 * - 1: Soft IRQ context (softirq, tasklet, timer)
 * - 2: Hard IRQ context (hardware interrupt handler)
 * - 3: NMI context (non-maskable interrupt)
 *
 * Example scenarios:
 * - Lock acquired in process context with IRQs enabled: Normal case
 * - Lock acquired in IRQ context: May cause deadlock if same lock used in process context
 * - Lock acquired with IRQs disabled: Prevents interrupt-related deadlocks
 *
 * @param hl_hardirqs_off Hard IRQ disabled flag
 * @param hl_irq_context IRQ context value
 * @return Human-readable description of the IRQ state
 */
std::string Lockdep::get_irq_state_description(uint32_t hl_hardirqs_off, uint32_t hl_irq_context) {
    std::string irq_state = (hl_hardirqs_off == 0) ? "hardirqs_enabled" : "hardirqs_disabled";
    std::string context_desc = (hl_irq_context == 0) ? "process context" : "interrupt context";
    return irq_state + ", irq_context=" + std::to_string(hl_irq_context) + " (" + context_desc + ")";
}

/**
 * Convert lock validation flag to descriptive text
 *
 * The check flag controls whether lockdep performs dependency validation
 * for this particular lock acquisition. This is part of the lockdep
 * deadlock detection mechanism.
 *
 * Validation states:
 * - check=enabled (1): Lockdep will track this lock for dependency analysis
 *   - Records lock ordering relationships
 *   - Detects potential deadlock scenarios
 *   - Updates dependency graphs
 *
 * - check=disabled (0): Lockdep skips validation for this lock
 *   - Used for locks that are known to be safe
 *   - Reduces overhead for performance-critical locks
 *   - May be disabled for debugging purposes
 *
 * Example: Some low-level locks (like scheduler locks) may have checking
 * disabled to avoid recursion in the lockdep system itself.
 *
 * @param hl_check Lock validation flag value
 * @return Human-readable description of the validation state
 */
std::string Lockdep::get_lock_validation_description(uint32_t hl_check) {
    return (hl_check == 1) ? "check=enabled" : "check=disabled";
}

/**
 * Determine lock status based on timestamps
 *
 * This function analyzes timing information to detect lock race conditions
 * and determine the current state of the lock. Lock race occurs when multiple
 * tasks compete for the same lock simultaneously.
 *
 * Status determination logic:
 * - If CONFIG_LOCK_STAT is disabled: Always return "OK ACQUIRED"
 * - Compare waittime vs holdtime:
 *   - waittime > holdtime: Indicates race condition (task had to wait)
 *   - waittime <= holdtime: Fast path acquisition (no waiting)
 *
 * Lock race indicators:
 * - High waittime: Task spent significant time waiting for lock
 * - Multiple tasks with same lock: Potential bottleneck
 * - Frequent races: May indicate need for lock optimization
 *
 * Example scenarios:
 * - "OK ACQUIRED": Lock obtained without race condition
 * - "WAITING/RACING": Lock is experiencing race condition
 *
 * @param waittime Timestamp when task started waiting for lock
 * @param holdtime Timestamp when lock was actually acquired
 * @return Human-readable description of the lock status
 */
std::string Lockdep::get_lock_status_description(uint64_t waittime, uint64_t holdtime) {
    // Check if lock statistics are available
    if (get_config_val("CONFIG_LOCK_STAT") != "y") {
        return "OK ACQUIRED (timing info not available)";
    }

    // Check for race conditions based on timing
    if (waittime > holdtime) {
        return "WAITING/RACING";
    }

    return "OK ACQUIRED";
}

/**
 * Calculate and format timing information
 *
 * This function processes lock timing data to provide insights into
 * lock performance and race conditions. The timing information helps identify
 * performance bottlenecks and lock race issues.
 *
 * Timing metrics:
 * - holdtime: How long the lock has been held by current task
 * - waittime: How long the task waited before acquiring the lock
 *
 * Race condition analysis:
 * - waittime > holdtime: Indicates lock race condition
 *   - Task had to wait longer than it has held the lock
 *   - Suggests other tasks were competing for the same lock
 *
 * - waittime <= holdtime: Fast path acquisition
 *   - Lock was available immediately or with minimal wait
 *   - Indicates good lock performance
 *
 * Performance implications:
 * - High holdtime: Lock held for long periods (potential bottleneck)
 * - High waittime: Significant race condition (may need lock optimization)
 * - Low values: Good lock performance
 *
 * Example outputs:
 * - "holdtime=0.001234s, no race (fast path)"
 * - "holdtime=0.050000s, race detected (waittime=0.100000s)"
 *
 * @param waittime Wait timestamp in nanoseconds
 * @param holdtime Hold timestamp in nanoseconds
 * @return Human-readable timing description with race condition analysis
 */
std::string Lockdep::get_timing_description(uint64_t waittime, uint64_t holdtime) {
    // Check if lock statistics are available
    if (get_config_val("CONFIG_LOCK_STAT") != "y") {
        return "timing info not available";
    }

    // Convert nanoseconds to seconds for human readability
    double holdtime_sec = holdtime / 1000000000.0;
    double waittime_sec = waittime / 1000000000.0;

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);

    // Analyze race conditions based on timing relationship
    if (waittime_sec > holdtime_sec) {
        oss << "holdtime=" << holdtime_sec << "s, race detected (waittime=" << waittime_sec << "s)";
    } else {
        oss << "holdtime=" << holdtime_sec << "s, no race (fast path)";
    }

    return oss.str();
}

/**
 * Format individual lock information into tree structure
 *
 * This function creates a visually appealing tree-structured representation
 * of lock information using Unicode box-drawing characters. The tree format
 * makes it easy to understand the hierarchical relationship of lock data.
 *
 * Tree structure format:
 * #N: 0xADDRESS (lock_name) at: function_name
 *     ├─ Lock Mode: [mode description]
 *     ├─ Acquire Method: [method description]
 *     ├─ IRQ State: [IRQ state description]
 *     ├─ Lock Validation: [validation state]
 *     ├─ Reference Count: [reference info]
 *     ├─ Timing: [timing analysis]
 *     ├─ Lock Status: [status description]
 *     └─ Debug: [raw debug values]
 *
 * Unicode characters used:
 * - ├─ : Branch connector (middle items)
 * - └─ : Final branch connector (last item)
 *
 * Information hierarchy:
 * 1. Header: Lock index, address, name, acquisition function
 * 2. Mode: How the lock can be shared (exclusive/shared)
 * 3. Method: How it was acquired (blocking/trylock)
 * 4. IRQ: Interrupt context and state
 * 5. Validation: Whether lockdep checking is enabled
 * 6. Counters: Reference and pin counts
 * 7. Timing: Performance and contention analysis
 * 8. Status: Current lock state
 * 9. Debug: Raw values for troubleshooting
 *
 * @param lock_index Index of lock in the held_locks array
 * @param instance Address of the actual lock instance
 * @param lock_info Parsed lock information structure
 * @return Formatted tree-structure string ready for display
 */
std::string Lockdep::format_lock_info_tree(int lock_index, ulong instance, const HeldLockInfo& lock_info) {
    std::ostringstream output;

    // Lock header with index, address, name, and acquisition function
    output << "#" << lock_index << ": 0x" << std::hex << instance
           << " (" << lock_info.lock_name << ") at: " << lock_info.acquire_function << "\n";

    // Create tree structure using Unicode box-drawing characters with aligned colons
    output << "    ├─ Lock Mode        : " << get_lock_mode_description(lock_info.read_mode) << "\n";

    output << "    ├─ Acquire Method   : " << get_acquire_method_description(lock_info.trylock) << "\n";
    output << "    ├─ IRQ State        : " << get_irq_state_description(lock_info.hardirqs_off, lock_info.irq_context) << "\n";
    output << "    ├─ Lock Validation  : " << get_lock_validation_description(lock_info.check) << "\n";
    output << "    ├─ Reference Count  : references=" << lock_info.references << " (lockdep usage tracking), pin_count=" << lock_info.pin_count << " (active holders)\n";
    output << "    ├─ Timing           : " << get_timing_description(lock_info.waittime_stamp, lock_info.holdtime_stamp) << "\n";
    output << "    ├─ Lock Status      : " << get_lock_status_description(lock_info.waittime_stamp, lock_info.holdtime_stamp) << "\n";

    // Debug information (last item uses └─ to close the tree)
    output << "    └─ Debug            : class_idx=0x" << std::hex << lock_info.class_idx
           << ", acquire_ip=0x" << lock_info.acquire_ip
           << ", chain_key=0x" << lock_info.prev_chain_key << " (lockdep hash for deadlock detection)\n";

    return output.str();
}

/**
 * Parse the locks held by a single task from its task_struct
 *
 * Each task in the Linux kernel has a held_locks array in its task_struct that
 * contains information about all locks currently held by that task. This function
 * extracts and parses this information to provide detailed lock analysis.
 *
 * The held_locks array is a fixed-size array (MAX_LOCK_DEPTH entries) where
 * each entry is a struct held_lock containing:
 * - Lock chain key for deadlock detection
 * - Instruction pointer where lock was acquired
 * - Lock instance address (the actual lock object)
 * - Class index with embedded flags
 * - Timing information (if CONFIG_LOCK_STAT enabled)
 *
 * Data flow:
 * task_struct → held_locks[i] → held_lock fields → parsed lock info
 *
 * @param task_addr Address of the task_struct in kernel memory
 * @param task_info Output structure to store parsed lock information
 */
void Lockdep::parse_task_held_locks(ulong task_addr, TaskLockdepInfo& task_info) {
    // Validate the task structure address
    if (!is_kvaddr(task_addr)) {
        LOGD("Invalid task_struct address: 0x%lx\n", task_addr);
        return;
    }

    // Calculate the address of the held_locks array within the task_struct
    ulong held_locks_addr = task_addr + field_offset(task_struct, held_locks);

    // Validate the held_locks array address
    if (!is_kvaddr(held_locks_addr)) {
        LOGD("Invalid held_locks address: 0x%lx\n", held_locks_addr);
        return;
    }

    // Iterate through all held locks (up to lockdep_depth)
    for (uint32_t i = 0; i < task_info.lockdep_depth; i++) {
        HeldLockInfo lock_info = {};

        // Calculate the address of the current held_lock structure
        ulong held_lock_addr = held_locks_addr + (i * sizeof_held_lock);

        // Validate the held_lock structure address
        if (!is_kvaddr(held_lock_addr)) {
            LOGD("Invalid held_lock address: 0x%lx for index %u\n", held_lock_addr, i);
            break;
        }

        // Read the lock chain key - used for deadlock detection algorithms
        // If this is 0, it indicates an invalid/unused lock entry
        lock_info.prev_chain_key = read_ulong(held_lock_addr + field_offset(held_lock, prev_chain_key), "prev_chain_key");
        if (lock_info.prev_chain_key == 0) {
            LOGD("Invalid lock entry at index %u (zero chain key)\n", i);
            break; // Invalid lock entry, stop processing
        }

        // Read instruction pointer where the lock was acquired
        lock_info.acquire_ip = read_ulong(held_lock_addr + field_offset(held_lock, acquire_ip), "acquire_ip");

        // Read caller's instruction pointer (function that called the lock function)
        lock_info.acquire_ip_caller = read_ulong(held_lock_addr + field_offset(held_lock, acquire_ip_caller), "acquire_ip_caller");

        // Read the address of the actual lock instance (mutex, spinlock, etc.)
        lock_info.instance = read_ulong(held_lock_addr + field_offset(held_lock, instance), "instance");

        // Read nested lock information (for lock hierarchies)
        lock_info.nest_lock = read_ulong(held_lock_addr + field_offset(held_lock, nest_lock), "nest_lock");

        // Read the class_idx field containing packed lock metadata
        lock_info.class_idx_full = read_uint(held_lock_addr + field_offset(held_lock, class_idx), "class_idx");

        // Read the pin count (reference counting for lock usage)
        lock_info.pin_count = read_uint(held_lock_addr + field_offset(held_lock, pin_count), "pin_count");

        // Read timing information if CONFIG_LOCK_STAT is enabled
        // These timestamps are used for lock contention analysis
        if (get_config_val("CONFIG_LOCK_STAT") == "y") {
            lock_info.waittime_stamp = read_ulonglong(held_lock_addr + field_offset(held_lock, waittime_stamp), "waittime_stamp");
            lock_info.holdtime_stamp = read_ulonglong(held_lock_addr + field_offset(held_lock, holdtime_stamp), "holdtime_stamp");
        }

        // Parse the bit fields encoded in class_idx
        parse_class_idx_bits(lock_info.class_idx_full, lock_info);

        // Get the human-readable lock name from the lock class
        lock_info.lock_name = get_lock_name(lock_info.class_idx);

        // Resolve the function name where the lock was acquired
        lock_info.acquire_function = "unknown";
        if (lock_info.acquire_ip != 0 && is_kvaddr(lock_info.acquire_ip)) {
            char *func_name = closest_symbol(lock_info.acquire_ip);
            if (func_name) {
                lock_info.acquire_function = func_name;
            }
        }

        // Add the parsed lock information to the task's lock list
        task_info.held_locks.push_back(lock_info);
    }
}


/**
 * Parse tasks summary - display table format of tasks holding locks
 */
void Lockdep::parse_tasks_summary() {
    // Initialize global lockdep data
    if (!csymbol_exists("lock_classes_in_use") || !csymbol_exists("lock_classes")) {
        LOGE("lockdep symbols not found\n");
        return;
    }

    lock_classes_in_use_bitmap = csymbol_value("lock_classes_in_use");
    lock_classes = csymbol_value("lock_classes");

    if (!is_kvaddr(lock_classes_in_use_bitmap) || !is_kvaddr(lock_classes)) {
        LOGE("Invalid lockdep addresses\n");
        return;
    }

    // Print table header
    PRINT("%-6s %-14s %6s %s\n", "PID", "COMM", "LOCKS", "TASK_STRUCT");
    PRINT("------ -------------- ------ ----------------\n");

    int total_tasks_with_locks = 0;
    int total_tasks_processed = 0;

    // Iterate through all tasks
    for (ulong task_addr : for_each_threads()) {
        total_tasks_processed++;

        if (!is_kvaddr(task_addr)) continue;

        struct task_context *tc = task_to_context(task_addr);
        if (!tc) continue;

        ulong lockdep_depth_addr = task_addr + field_offset(task_struct, lockdep_depth);
        if (!is_kvaddr(lockdep_depth_addr)) continue;

        uint32_t lockdep_depth = read_uint(lockdep_depth_addr, "lockdep_depth");

        if (lockdep_depth > 0 && lockdep_depth <= 64) {
            // Truncate comm if too long
            char comm_truncated[15];
            strncpy(comm_truncated, tc->comm, 14);
            comm_truncated[14] = '\0';

            PRINT("%-6lu %-14s %6u 0x%lx\n", tc->pid, comm_truncated, lockdep_depth, task_addr);
            total_tasks_with_locks++;
        }
    }

    PRINT("------ -------------- ------ ----------------\n");
    PRINT("Total: %d tasks holding locks out of %d processed\n",
          total_tasks_with_locks, total_tasks_processed);
}

/**
 * Parse single task detail - display detailed lock information for specific task
 */
void Lockdep::parse_single_task_detail(ulong task_addr) {
    // Validate task address
    if (!is_kvaddr(task_addr)) {
        LOGE("Invalid task address: 0x%lx\n", task_addr);
        return;
    }

    // Initialize global lockdep data
    if (!csymbol_exists("lock_classes_in_use") || !csymbol_exists("lock_classes")) {
        LOGE("lockdep symbols not found\n");
        return;
    }

    lock_classes_in_use_bitmap = csymbol_value("lock_classes_in_use");
    lock_classes = csymbol_value("lock_classes");

    if (!is_kvaddr(lock_classes_in_use_bitmap) || !is_kvaddr(lock_classes)) {
        LOGE("Invalid lockdep addresses\n");
        return;
    }

    // Get task context
    struct task_context *tc = task_to_context(task_addr);
    if (!tc) {
        LOGE("Failed to get task context for 0x%lx\n", task_addr);
        return;
    }

    // Read lockdep_depth
    ulong lockdep_depth_addr = task_addr + field_offset(task_struct, lockdep_depth);
    if (!is_kvaddr(lockdep_depth_addr)) {
        LOGE("Invalid lockdep_depth address\n");
        return;
    }

    uint32_t lockdep_depth = read_uint(lockdep_depth_addr, "lockdep_depth");

    if (lockdep_depth == 0) {
        PRINT("Task %s (pid=%lu) is not holding any locks\n", tc->comm, tc->pid);
        return;
    }

    if (lockdep_depth > 64) {
        LOGE("Suspicious lockdep_depth %u for task %s\n", lockdep_depth, tc->comm);
        return;
    }

    // Parse task locks
    TaskLockdepInfo task_info;
    task_info.tc = tc;
    task_info.lockdep_depth = lockdep_depth;

    parse_task_held_locks(task_addr, task_info);

    if (task_info.held_locks.empty()) {
        PRINT("No valid locks found for task %s despite lockdep_depth=%u\n", tc->comm, lockdep_depth);
        return;
    }

    // Display detailed lock information
    PRINT("%u locks held by %s (pid=%lu, task_struct=0x%lx):\n",
          lockdep_depth, tc->comm, tc->pid, task_addr);

    for (size_t i = 0; i < task_info.held_locks.size(); i++) {
        std::string formatted_output = format_lock_info_tree(i,
            task_info.held_locks[i].instance, task_info.held_locks[i]);
        PRINT("%s", formatted_output.c_str());
    }
}

/**
 * Parse lockdep information for all tasks in the system (legacy function)
 */
void Lockdep::parse_all_tasks_lockdep() {
    // Validate and initialize global lockdep data structures
    // These symbols must exist for lockdep analysis to work
    if (!csymbol_exists("lock_classes_in_use")) {
        PRINT("lock_classes_in_use symbol not found\n");
        return;
    }
    if (!csymbol_exists("lock_classes")) {
        PRINT("lock_classes symbol not found\n");
        return;
    }

    // Get addresses of global lockdep data structures
    lock_classes_in_use_bitmap = csymbol_value("lock_classes_in_use");
    lock_classes = csymbol_value("lock_classes");

    // Validate the global data structure addresses
    if (!is_kvaddr(lock_classes_in_use_bitmap)) {
        PRINT("Invalid lock_classes_in_use address: 0x%lx\n", lock_classes_in_use_bitmap);
        return;
    }
    if (!is_kvaddr(lock_classes)) {
        PRINT("Invalid lock_classes address: 0x%lx\n", lock_classes);
        return;
    }

    LOGD("Global lockdep data initialized: lock_classes_in_use=0x%lx, lock_classes=0x%lx\n",
         lock_classes_in_use_bitmap, lock_classes);

    int total_tasks_with_locks = 0;
    int total_tasks_processed = 0;

    // Iterate through all tasks (processes and threads) in the system
    // The for_each_threads() function provides all schedulable entities
    for (ulong task_addr : for_each_threads()) {
        total_tasks_processed++;

        // Validate the task structure address before processing
        if (!is_kvaddr(task_addr)) {
            LOGD("Invalid task_struct address: 0x%lx, skipping\n", task_addr);
            continue;
        }

        // Get the task context structure for this task
        // This provides basic task information (comm, pid, etc.)
        struct task_context *tc = task_to_context(task_addr);
        if (!tc) {
            LOGD("Failed to get task context for task_struct 0x%lx\n", task_addr);
            continue;
        }

        // Calculate the address of the lockdep_depth field
        ulong lockdep_depth_addr = task_addr + field_offset(task_struct, lockdep_depth);
        if (!is_kvaddr(lockdep_depth_addr)) {
            LOGD("Invalid lockdep_depth address: 0x%lx for task %s\n", lockdep_depth_addr, tc->comm);
            continue;
        }

        // Read the number of locks currently held by this task
        // lockdep_depth indicates how many entries in held_locks[] are valid
        uint32_t lockdep_depth = read_uint(lockdep_depth_addr, "lockdep_depth");

        // Only process tasks that are currently holding locks
        if (lockdep_depth > 0) {
            // Sanity check: lockdep_depth should not exceed MAX_LOCK_DEPTH
            if (lockdep_depth > 64) { // MAX_LOCK_DEPTH is typically 48-64
                LOGD("Suspicious lockdep_depth %u for task %s, skipping\n", lockdep_depth, tc->comm);
                continue;
            }

            TaskLockdepInfo task_info;
            task_info.tc = tc;
            task_info.lockdep_depth = lockdep_depth;

            LOGD("Processing task %s (pid=%lu) with %u locks\n", tc->comm, tc->pid, lockdep_depth);

            // Parse the held_locks array for this task
            // This extracts detailed information about each lock
            parse_task_held_locks(task_addr, task_info);

            // Only display tasks that have successfully parsed locks
            if (!task_info.held_locks.empty()) {
                // Display task header with lock count and basic info
                PRINT("%u locks held by %s (pid=%lu, task_struct=0x%lx):\n",
                      lockdep_depth, tc->comm, tc->pid, task_addr);

                // Display each lock in a tree-structured format
                for (size_t i = 0; i < task_info.held_locks.size(); i++) {
                    std::string formatted_output = format_lock_info_tree(i,
                        task_info.held_locks[i].instance, task_info.held_locks[i]);
                    PRINT("%s", formatted_output.c_str());
                }
                PRINT("\n");

                total_tasks_with_locks++;
            } else {
                LOGD("No valid locks found for task %s despite lockdep_depth=%u\n", tc->comm, lockdep_depth);
            }
        }
    }

    // Provide summary information about the analysis
    if (total_tasks_with_locks == 0) {
        PRINT("No tasks found holding locks.\n");
        PRINT("This could indicate:\n");
        PRINT("  1. System is currently idle with no lock contention\n");
        PRINT("  2. CONFIG_LOCKDEP is not properly configured\n");
        PRINT("  3. Lock information has been cleared or corrupted\n");
        PRINT("  4. Analysis was performed at a moment with minimal lock usage\n\n");
    } else {
        PRINT("Summary: Found %d tasks holding locks out of %d total tasks processed.\n\n",
              total_tasks_with_locks, total_tasks_processed);
    }

    PRINT("============================================\n");
}

#pragma GCC diagnostic pop
