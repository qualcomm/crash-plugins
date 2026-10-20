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
#include <unordered_set>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(Lockdep)
#endif

static std::string trim_spaces(const std::string &s) {
    const char *ws = " \t\r\n";
    size_t b = s.find_first_not_of(ws);
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}

Lockdep::Lockdep() {
    lock_classes_in_use_bitmap = 0;
    lock_classes = 0;
    sizeof_held_lock = 0;
    sizeof_lock_class = 0;
}

void Lockdep::cmd_main(void) {
    int c;
    enum class Mode {
        NONE,
        D,
        T,
    };
    Mode mode = Mode::NONE;
    ulong target_addr = 0;
    bool has_target = false;

    if (argcnt < 2) {
        /* Explicit option is required: lockdep -t/-d */
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }

    auto parse_pid_or_addr = [&](const char *arg) -> ulong {
        char *endp;
        ulong val = strtoull(arg, &endp, 10);
        if (*endp == '\0' && !(arg[0]=='0' && (arg[1]=='x'||arg[1]=='X'))) {
            ulong t = pid_to_task(val);
            if (!t) { LOGE("No task for PID %lu", val); argerrs++; }
            return t;
        }
        return htol(const_cast<char*>(arg), FAULT_ON_ERROR, NULL);
    };

    auto set_mode = [&](Mode m) {
        if (mode != Mode::NONE && mode != m) {
            argerrs++;
            return;
        }
        mode = m;
    };

    auto parse_optional_target = [&]() {
        if (optarg) {
            target_addr = parse_pid_or_addr(optarg);
            has_target = true;
        } else if (optind < argcnt && args[optind] && args[optind][0] != '-') {
            target_addr = parse_pid_or_addr(args[optind]);
            has_target = true;
            optind++;
        }
    };

    while ((c = getopt(argcnt, args, "d::t::")) != EOF) {
        switch(c) {
            case 'd':
                set_mode(Mode::D);
                parse_optional_target();
                break;
            case 't':
                set_mode(Mode::T);
                parse_optional_target();
                break;
            default:
                argerrs++;
                break;
        }
    }

    /* No positional smart-dispatch: require explicit option-based invocation. */
    if (optind < argcnt) {
        argerrs++;
    }

    if (argerrs || mode == Mode::NONE) {
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }

    switch (mode) {
        case Mode::D:
            if (get_config_val("CONFIG_LOCKDEP") != "y") {
                PRINT("CONFIG_LOCKDEP not enabled in kernel\n");
                return;
            }
            if (has_target)
                print_task_nesting_detail(target_addr);
            else
                detect_lock_nesting();
            return;
        case Mode::T:
            if (has_target) {
                show_lock_smart(target_addr);
            } else
                show_lock_analysis();
            return;
        default:
            break;
    }

    cmd_usage(pc->curcmd, SYNOPSIS);
}

void Lockdep::init_command(void) {
    cmd_name = "lockdep";
    help_str_list = {
        "lockdep",
        "Analyze kernel lock state (explicit option required)",
        "-d [pid|task_addr] | -t [pid|task_addr|lock_addr]\n"
        "  NOTE: lockdep requires an explicit option; no default action.\n"
        "  -d              Scan ALL tasks for lock nesting anomalies (CONFIG_LOCKDEP)\n"
        "  -d pid|addr     Detailed held-lock breakdown + nesting pattern for one task\n"
        "  -t              System-wide lock analysis + quick summary table\n"
        "  -t pid|addr     Target analysis (task chain/graph or lock object auto-detected)\n"
        "                    task/pid: blocked chain and lock relations\n"
        "                    lock addr: holder/waiters/flags and dependency context",
        "\n",
        "Examples:",
        "  %s> lockdep -d             (scan all: find lock nesting bugs)",
        "  %s> lockdep -d 1234        (detail for task PID 1234)",
        "  %s> lockdep -d 0xffff...   (detail for specific task addr)",
        "  %s> lockdep -t             (system-wide lock health report + summary)",
        "  %s> lockdep -t 1234        (chain: why is PID 1234 blocked?)",
        "  %s> lockdep -t 0xffff...   (task or lock object auto-analysis)",
        "\n",
        "Notes:",
        "  -d requires CONFIG_LOCKDEP (held_locks[] must be available).",
        "  -t works without CONFIG_LOCKDEP via pi_blocked_on + stack scan",
        "  and may provide partial held-lock info tagged [pi_chain] / [inferred].",
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

    ulong name_field_addr = lock_class_addr + field_offset(lock_class, name);
    if (!is_kvaddr(name_field_addr)) return "unknown";

    ulong name_ptr = read_pointer(name_field_addr, "lock class name");
    if (!is_kvaddr(name_ptr)) return "unknown";

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
 * task_struct -> held_locks[i] -> held_lock fields -> parsed lock info
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

        // Read the address of the actual lock instance (mutex, spinlock, etc.)
        lock_info.instance = read_ulong(held_lock_addr + field_offset(held_lock, instance), "instance");

        // Read the class_idx field containing packed lock metadata
        lock_info.class_idx_full = read_uint(held_lock_addr + field_offset(held_lock, class_idx), "class_idx");

        // Read the pin count (reference counting for lock usage)
        lock_info.pin_count = read_uint(held_lock_addr + field_offset(held_lock, pin_count), "pin_count");

        // Read timing information if CONFIG_LOCK_STAT is enabled
        // These timestamps are used for lock contention analysis
        if (get_config_val("CONFIG_LOCK_STAT") == "y") {
            if (field_offset(held_lock, waittime_stamp) != -1)
                lock_info.waittime_stamp = read_ulonglong(held_lock_addr + field_offset(held_lock, waittime_stamp), "waittime_stamp");
            if (field_offset(held_lock, holdtime_stamp) != -1)
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


bool Lockdep::ensure_lockdep_globals() {
    if (!csymbol_exists("lock_classes_in_use") || !csymbol_exists("lock_classes"))
        return false;
    lock_classes_in_use_bitmap = csymbol_value("lock_classes_in_use");
    lock_classes               = csymbol_value("lock_classes");
    if (!sizeof_held_lock)  sizeof_held_lock  = struct_size(held_lock);
    if (!sizeof_lock_class) sizeof_lock_class = struct_size(lock_class);
    return is_kvaddr(lock_classes_in_use_bitmap) && is_kvaddr(lock_classes);
}

/* =========================================================================
 * Shared lock helpers
 * =========================================================================
 */

Lockdep::LockType Lockdep::classify_lock_type(const std::string &name, ulong instance) {
    // Fast-path: class name keyword matching
    if (!name.empty()) {
        auto contains = [&](const char *s) {
            return name.find(s) != std::string::npos;
        };
        if (contains("rt_mutex"))                       return LockType::RT_MUTEX;
        if (contains("percpu_rwsem") || contains("cpu_hotplug")) return LockType::PERCPU_RWSEM;
        if (contains("rwsem") || contains("rw_sem") ||
            contains("i_rwsem") || contains("mmap_lock")) return LockType::RWSEM;
        if (contains("mutex"))                          return LockType::MUTEX;
    }

    // Scoring-based heuristic on instance address
    if (!is_kvaddr(instance)) return LockType::NONE;

    struct Candidate { LockType type; int score; } best = {LockType::NONE, 0};
    auto try_update = [&](LockType t, int s) {
        if (s > best.score) { best.type = t; best.score = s; }
    };

    /* ---- rt_mutex / PREEMPT_RT spinlock ---------------------------------- */
    if (field_offset(rt_mutex_base, owner) != -1) {
        int score = 0;
        ulong raw  = read_pointer(instance + field_offset(rt_mutex_base, owner), "rt?owner");
        ulong task = rt_mutex_owner_task(raw);
        if (task && is_kvaddr(task) && task_to_context(task)) score += 5;
        else if (!task && (raw & RT_MUTEX_HAS_WAITERS))        score += 2; // handoff
        if (field_offset(rt_mutex_base, waiters) != -1) {
            ulong rb_node = read_pointer(
                instance + field_offset(rt_mutex_base, waiters), "rt?rb_node");
            ulong leftmost = read_pointer(
                instance + field_offset(rt_mutex_base, waiters) + sizeof(ulong), "rt?leftmost");
            if (rb_node   && is_kvaddr(rb_node))   score += 3;
            if (leftmost  && is_kvaddr(leftmost))  score += 2;
        }
        try_update(LockType::RT_MUTEX, score);
    }

    /* ---- rw_semaphore ---------------------------------------------------- */
    if (field_offset(rw_semaphore, count)     != -1 &&
        field_offset(rw_semaphore, wait_list) != -1) {
        int   score = 0;
        ulong cnt   = read_ulong(instance + field_offset(rw_semaphore, count), "rwsem?count");
        if (cnt & RWSEM_WRITER_LOCKED)                      score += 4;
        if (cnt & (RWSEM_FLAG_WAITERS | RWSEM_FLAG_HANDOFF)) score += 3;
        ulong readers = cnt >> RWSEM_READER_SHIFT;
        if (readers > 0 && readers < 512)                   score += 3;
        ulong wl   = instance + field_offset(rw_semaphore, wait_list);
        ulong next = read_pointer(wl, "rwsem?wl.next");
        ulong prev = read_pointer(wl + sizeof(ulong), "rwsem?wl.prev");
        if (is_kvaddr(next) && is_kvaddr(prev))             score += 2;
        if (field_offset(rw_semaphore, owner) != -1) {
            ulong ow = read_ulong(instance + field_offset(rw_semaphore, owner), "rwsem?owner");
            ulong wt = rwsem_owner_task(ow);
            if (wt && is_kvaddr(wt) && task_to_context(wt) &&
                !(ow & RWSEM_READER_OWNED))                 score += 3;
        }
        try_update(LockType::RWSEM, score);
    }

    /* ---- percpu_rwsem --------------------------------------------- */
    if (field_offset(percpu_rw_semaphore, waiters) != -1) {
        int score = 0;
        if (field_offset(percpu_rw_semaphore, block) != -1) {
            int blk = read_int(instance + field_offset(percpu_rw_semaphore, block),
                               "percpu_rwsem?block");
            if (blk != 0) score += 2;
        }
        if (field_offset(percpu_rw_semaphore, writer) != -1 &&
            field_offset(rcuwait, task) != -1) {
            ulong wt = read_pointer(
                instance + field_offset(percpu_rw_semaphore, writer) +
                    field_offset(rcuwait, task),
                "percpu_rwsem?writer.task");
            wt = is_kvaddr(wt);
            if (wt && task_to_context(wt)) score += 2;
        }
        try_update(LockType::PERCPU_RWSEM, score);
    }

    /* ---- mutex ----------------------------------------------------------- */
    if (field_offset(mutex, owner)     != -1 &&
        field_offset(mutex, wait_list) != -1) {
        int   score = 0;
        ulong raw   = read_pointer(instance + field_offset(mutex, owner), "mutex?owner");
        ulong task  = mutex_owner_task(raw);
        if (task && is_kvaddr(task) && task_to_context(task))  score += 5;
        if (raw & MUTEX_FLAG_WAITERS)                          score += 3;
        if (raw & MUTEX_FLAG_HANDOFF)                          score += 2;
        ulong wl   = instance + field_offset(mutex, wait_list);
        ulong next = read_pointer(wl, "mutex?wl.next");
        ulong prev = read_pointer(wl + sizeof(ulong), "mutex?wl.prev");
        if (is_kvaddr(next) && is_kvaddr(prev))                score += 2;
        try_update(LockType::MUTEX, score);
    }

    /* ---- semaphore --------------------------------------------------- */
    if (field_offset(semaphore, count)     != -1 &&
        field_offset(semaphore, wait_list) != -1) {
        int score = 0;
        ulong cnt_addr = instance + field_offset(semaphore, count);
        uint32_t cnt = read_uint(cnt_addr, "sem?count");
        if (cnt <= 1024) score += 3;
        ulong wl   = instance + field_offset(semaphore, wait_list);
        ulong next = read_pointer(wl, "sem?wl.next");
        ulong prev = read_pointer(wl + sizeof(ulong), "sem?wl.prev");
        if (is_kvaddr(next) && is_kvaddr(prev)) score += 2;
        if (cnt == 0 && next != wl) score += 2;  /* locked with waiters */
        try_update(LockType::SEMAPHORE, score);
    }

    /* ---- completion -------------------------------------------------- */
    if (field_offset(completion, done) != -1) {
        int score = 0;
        uint32_t done = read_uint(instance + field_offset(completion, done),
                                  "comp?done");
        if (done == 0) score += 3;  /* pending completion */
        if (done <= 1024) score += 1;
        try_update(LockType::COMPLETION, score);
    }

    /* ---- raw_spinlock (qspinlock) ------------------------------------- */
    {
        int score = 0;
        uint32_t val = read_uint(instance, "spin?val");
        uint32_t locked_f  = val & 0xff;
        uint32_t pending_f = (val >> 8) & 0x1;
        uint32_t tail_f    = val >> 9;
        if (locked_f <= 1 && pending_f <= 1 && tail_f < 0x400000) score += 2;
        if (locked_f == 1) score += 2;
        if (pending_f == 1) score += 1;
        if (val < 0x80000000U) score += 1;
        try_update(LockType::SPINLOCK, score);
    }

    return (best.score >= 3) ? best.type : LockType::NONE;
}

/* Collect all task addresses waiting on a given lock, excluding skip_task. */
std::vector<ulong> Lockdep::collect_lock_waiters(ulong lock_addr, LockType type,
                                                  ulong skip_task) {
    std::vector<ulong> result;
    std::unordered_set<ulong> seen;

    if (type == LockType::RT_MUTEX) {
        /* Skip rbtree walk entirely — use brute-force pi_blocked_on scan only.
         * Crash dumps taken during rt_mutex operations may have partially-corrupted
         * rbtree nodes (HANDOFF, partial rebalance, or false-positive locks from
         * stack scan).  On PREEMPT_RT kernels pi_blocked_on is always maintained,
         * so brute-force yields the same complete result without any noise. */

        // Brute-force: scan all tasks' pi_blocked_on
        if (field_offset(task_struct, pi_blocked_on) != -1 &&
            field_offset(rt_mutex_waiter, lock) != -1) {
            for (ulong task : for_each_threads()) {
                if (task == skip_task) continue;
                ulong pw = read_pointer(task + field_offset(task_struct, pi_blocked_on), "pi_bo");
                if (!pw || !is_kvaddr(pw)) continue;
                ulong lk = read_pointer(pw + field_offset(rt_mutex_waiter, lock), "rt_waiter.lock");
                if (lk != lock_addr) continue;
                ulong t = read_pointer(pw + field_offset(rt_mutex_waiter, task), "rt_waiter.task");
                if (t && seen.insert(t).second)
                    result.push_back(t);
            }
        }
    } else if (type == LockType::MUTEX) {
        int off_wl = field_offset(mutex, wait_list),
            off_ml = field_offset(mutex_waiter, list),
            off_mt = field_offset(mutex_waiter, task);
        if (off_wl == -1 || off_ml == -1 || off_mt == -1) return result;
        for (ulong w : for_each_list(lock_addr + off_wl, off_ml)) {
            ulong t = read_pointer(w + off_mt, "mwaiter.task");
            if (t && t != skip_task && seen.insert(t).second)
                result.push_back(t);
        }
    } else if (type == LockType::RWSEM) {
        int off_wl = field_offset(rw_semaphore, wait_list),
            off_rl = field_offset(rwsem_waiter, list),
            off_rt = field_offset(rwsem_waiter, task);
        if (off_wl == -1 || off_rl == -1 || off_rt == -1) return result;
        for (ulong w : for_each_list(lock_addr + off_wl, off_rl)) {
            ulong t = read_pointer(w + off_rt, "rswaiter.task");
            if (t && t != skip_task && seen.insert(t).second)
                result.push_back(t);
        }
    } else if (type == LockType::PERCPU_RWSEM) {
        int off_wts = field_offset(percpu_rw_semaphore, waiters),
            off_hd  = field_offset(wait_queue_head, head),
            off_ent = field_offset(wait_queue_entry, entry),
            off_prv = field_offset(wait_queue_entry, private),
            off_nxt = field_offset(list_head, next);
        if (off_wts == -1 || off_hd == -1 || off_ent == -1 || off_prv == -1 || off_nxt == -1) return result;
        ulong head = lock_addr + off_wts + off_hd;
        ulong cur  = read_pointer(head + off_nxt, "wq.head.next");
        for (int s = 0; cur && is_kvaddr(cur) && cur != head && s < 128; s++) {
            ulong t = is_kvaddr(
                read_pointer(cur - off_ent + off_prv, "wq.private"));
            if (t && t != skip_task && seen.insert(t).second)
                result.push_back(t);
            cur = read_pointer(cur + off_nxt, "wq.next");
        }
    } else if (type == LockType::SEMAPHORE) {
        result = collect_semaphore_waiters(lock_addr, skip_task);
    } else if (type == LockType::COMPLETION) {
        int wait_off = field_offset(completion, wait);
        if (wait_off >= 0) {
            ulong wh = lock_addr + wait_off;
            ulong next = read_pointer(wh, "comp.wait.next");
            std::unordered_set<ulong> seen2;
            int steps = 0;
            while (is_kvaddr(next) && next != wh && steps++ < 64) {
                /* swait_queue_entry: first field is task_struct * */
                ulong t = read_pointer(next, "swq.task");
                if (t && is_kvaddr(t) && t != skip_task &&
                    task_to_context(t) && seen2.insert(t).second)
                    result.push_back(t);
                next = read_pointer(next + sizeof(ulong), "swq.next");
            }
        }
    }
    return result;
}

std::string Lockdep::format_task_line(ulong task_addr) {
    struct task_context *tc = task_to_context(task_addr);
    std::string state_txt = task_state_display(task_addr);
    std::string prio_str = format_prio_str(task_addr);

    char buf[320];
    if (tc) {
        snprintf(buf, sizeof(buf), "PID %-6d  %-22s  %s  [%s]  CPU=%d",
                 (int)tc->pid, tc->comm, prio_str.c_str(), state_txt.c_str(), (int)tc->processor);
    } else {
        snprintf(buf, sizeof(buf), "task=0x%lx  [%s]", task_addr, state_txt.c_str());
    }
    return buf;
}

/* -------------------------------------------------------------------------
 * find_any_blocking_lock — unified finder: pi_blocked_on (RT) first,
 * then mutex stack scan (regular mutex without CONFIG_DEBUG_MUTEXES).
 * -------------------------------------------------------------------------
 */
ulong Lockdep::find_any_blocking_lock(ulong task_addr, LockType &lock_type,
                                      ulong &owner_raw_out) {
    // Path 1: pi_blocked_on -> rt_mutex or PREEMPT_RT spinlock
    if (field_offset(task_struct, pi_blocked_on) != -1 &&
        field_offset(rt_mutex_waiter, lock) != -1 &&
        field_offset(rt_mutex_base, owner) != -1) {
        ulong waiter = read_pointer(
            task_addr + field_offset(task_struct, pi_blocked_on), "pi_blocked_on");
        if (waiter && is_kvaddr(waiter)) {
            ulong lock = read_pointer(
                waiter + field_offset(rt_mutex_waiter, lock), "rt_mutex lock");
            if (lock && is_kvaddr(lock)) {
                ulong owner_raw = read_pointer(
                    lock + field_offset(rt_mutex_base, owner), "rt_mutex owner");
                ulong owner_task = rt_mutex_owner_task(owner_raw);
                bool  handoff    = !owner_task && (owner_raw & RT_MUTEX_HAS_WAITERS);
                if ((owner_task && owner_task != task_addr) || handoff) {
                    lock_type     = LockType::RT_MUTEX;
                    owner_raw_out = owner_raw;
                    return lock;
                }
            }
        }
    }

    // Path 2+3: single-pass stack scan for mutex and rwsem
    ulong s = find_blocking_lock_on_stack(task_addr, lock_type, owner_raw_out);
    if (s) return s;

    lock_type = LockType::NONE;
    return 0;
}

/* -------------------------------------------------------------------------
 * find_blocking_lock_on_stack — single-pass stack scan for mutex or rwsem.
 *
 * Scans the task's kernel stack once and tries each candidate address as
 * both a mutex and an rwsem, using a two-way verification for each:
 *   mutex:  owner points to another task  AND  wait_list contains task
 *   rwsem:  count != 0                    AND  wait_list contains task
 * -------------------------------------------------------------------------
 */
ulong Lockdep::find_blocking_lock_on_stack(ulong task_addr, LockType &type_out,
                                            ulong &raw_out) {
    const bool has_mutex = (field_offset(mutex, owner)          != -1 &&
                            field_offset(mutex, wait_list)       != -1 &&
                            field_offset(mutex_waiter, list)     != -1 &&
                            field_offset(mutex_waiter, task)     != -1);
    const bool has_rwsem = (field_offset(rw_semaphore, count)    != -1 &&
                            field_offset(rw_semaphore, wait_list) != -1 &&
                            field_offset(rwsem_waiter, list)     != -1 &&
                            field_offset(rwsem_waiter, task)     != -1);
    const bool has_percpu_rwsem = (field_offset(percpu_rw_semaphore, waiters) != -1 &&
                                   field_offset(wait_queue_head, head)        != -1 &&
                                   field_offset(wait_queue_entry, entry)      != -1 &&
                                   field_offset(wait_queue_entry, private)     != -1 &&
                                   field_offset(list_head, next)              != -1);
    if (!has_mutex && !has_rwsem && !has_percpu_rwsem) return 0;

    ulong stackbase   = GET_STACKBASE(task_addr);
    ulong stacktop    = GET_STACKTOP(task_addr);
    size_t stack_bytes = stacktop - stackbase;
    std::vector<ulong> stack_words(stack_bytes / sizeof(ulong));
    if (!read_struct(stackbase, stack_words.data(), (int)stack_bytes, "stack scan"))
        return 0;

    for (ulong v : stack_words) {
        if (!is_kvaddr(v) || is_kernel_text(v)) continue;
        if (v >= stackbase && v < stacktop) continue;

        /* --- try mutex --------------------------------------------------- */
        if (has_mutex) {
            ulong owner_raw  = read_pointer(v + field_offset(mutex, owner), "mutex?owner");
            ulong owner_task = mutex_owner_task(owner_raw);
            if (owner_task && owner_task != task_addr) {
                ulong head = v + field_offset(mutex, wait_list);
                ulong cur  = read_pointer(head, "wl.next");
                for (int s = 0; s < 16 && is_kvaddr(cur) && cur != head; s++) {
                    ulong w = cur - field_offset(mutex_waiter, list);
                    if (read_pointer(w + field_offset(mutex_waiter, task),
                                     "waiter.task") == task_addr) {
                        type_out = LockType::MUTEX;
                        raw_out  = owner_raw;
                        return v;
                    }
                    cur = read_pointer(cur, "list.next");
                }
            }
        }

        /* --- try rwsem --------------------------------------------------- */
        if (has_rwsem) {
            ulong count_raw = read_ulong(v + field_offset(rw_semaphore, count),
                                         "rwsem?count");
            if (!count_raw) continue;
            ulong head = v + field_offset(rw_semaphore, wait_list);
            ulong cur  = read_pointer(head, "wl.next");
            if (!is_kvaddr(cur) || cur == head) continue;
            for (int s = 0; s < 16 && is_kvaddr(cur) && cur != head; s++) {
                ulong w = cur - field_offset(rwsem_waiter, list);
                if (read_pointer(w + field_offset(rwsem_waiter, task),
                                 "rswaiter.task") == task_addr) {
                    type_out = LockType::RWSEM;
                    raw_out  = count_raw;
                    return v;
                }
                cur = read_pointer(cur, "list.next");
            }
        }

        /* --- try percpu_rwsem -------------------------------------------- */
        if (has_percpu_rwsem) {
            bool pass = true;
            if (field_offset(percpu_rw_semaphore, block) != -1) {
                int blk = read_int(v + field_offset(percpu_rw_semaphore, block),
                                   "percpu_rwsem?block");
                if (blk == 0) pass = false;
            }
            if (!pass) continue;
            bool found = false;
            ulong wq = v + field_offset(percpu_rw_semaphore, waiters);
            ulong head = wq + field_offset(wait_queue_head, head);
            ulong cur = read_pointer(head + field_offset(list_head, next), "wq.head.next");
            for (int s = 0; cur && is_kvaddr(cur) && cur != head && s < 128; s++) {
                ulong e = cur - field_offset(wait_queue_entry, entry);
                ulong t = read_pointer(e + field_offset(wait_queue_entry, private),
                                       "wq.private");
                t = is_kvaddr(t);
                if (t == task_addr) { found = true; break; }
                cur = read_pointer(cur + field_offset(list_head, next), "wq.next");
            }
            if (!found) continue;
            ulong writer_task = 0;
            if (field_offset(percpu_rw_semaphore, writer) != -1 &&
                field_offset(rcuwait, task) != -1) {
                writer_task = read_pointer(
                    v + field_offset(percpu_rw_semaphore, writer) +
                        field_offset(rcuwait, task),
                    "percpu_rwsem.writer.task");
                writer_task = is_kvaddr(writer_task);
            }
            type_out = LockType::PERCPU_RWSEM;
            raw_out  = writer_task;
            return v;
        }
    }
    return 0;
}

/* =========================================================================
 * Lock object analysis helpers (used by -t <lock_addr>)
 * =========================================================================
 */

/* -------------------------------------------------------------------------
 * find_held_locks_pi_waiters — walk task.pi_waiters rbtree to find all
 * rt_mutex locks this task holds that currently have waiters.
 *
 * On PREEMPT_RT, when task T holds lock L and task W waits on L, W's
 * rt_mutex_waiter.pi_tree node is inserted into T.pi_waiters.  Walking
 * this tree gives us every lock T holds that has at least one waiter —
 * no stack dependency, no LOCKDEP required.
 * -------------------------------------------------------------------------
 */
std::vector<ulong> Lockdep::find_held_locks_pi_waiters(ulong task_addr) {
    std::vector<ulong> result;
    std::unordered_set<ulong> seen;

    if (field_offset(task_struct, pi_waiters)  == -1 ||
        field_offset(rt_mutex_waiter, pi_tree) == -1 ||
        field_offset(rt_mutex_waiter, lock)    == -1)
        return result;

    ulong root       = task_addr + field_offset(task_struct, pi_waiters);
    int   pi_tree_off = field_offset(rt_mutex_waiter, pi_tree);
    int   lock_off    = field_offset(rt_mutex_waiter, lock);

    /* Safe manual DFS — validates alignment at each step before pushing children.
     * Replaces for_each_rbtree to avoid its noise on partially-corrupted trees. */
    auto aligned_kvaddr = [this](ulong n) -> bool {
        return !n || (is_kvaddr(n) && !(n & 0x7));
    };

    ulong rb_root_node = read_pointer(root, "pi_waiters.root");
    if (!rb_root_node || !is_kvaddr(rb_root_node) || (rb_root_node & 0x7))
        return result;

    std::vector<ulong> stk;
    std::unordered_set<ulong> vis;
    stk.push_back(rb_root_node);

    while (!stk.empty()) {
        ulong n = stk.back(); stk.pop_back();
        if (!n || !is_kvaddr(n) || (n & 0x7)) continue;
        if (!vis.insert(n).second) continue;          /* cycle guard */

        /* n is the rb_node inside rt_mutex_waiter.pi_tree; compute waiter ptr */
        ulong waiter = n - pi_tree_off;
        if (is_kvaddr(waiter)) {
            ulong la = read_pointer(waiter + lock_off, "pi_w.lock");
            if (la && is_kvaddr(la) && seen.insert(la).second)
                result.push_back(la);
        }

        /* Push children only after validating their alignment */
        ulong right = read_pointer(n + sizeof(ulong),      "rbt.right");
        ulong left  = read_pointer(n + 2 * sizeof(ulong),  "rbt.left");
        if (aligned_kvaddr(right) && right) stk.push_back(right);
        if (aligned_kvaddr(left)  && left)  stk.push_back(left);
    }
    return result;
}

/* -------------------------------------------------------------------------
 * try_name_percpu_lock — attempt to identify a lock as a per-CPU variable.
 *
 * Per-CPU BH locks (softirq_ctrl.lock) and similar per-CPU rt_mutex
 * instances live at computable addresses: base_sym + __per_cpu_offset[N].
 * We scan known per-CPU lock symbols to find a CPU-N match.
 * -------------------------------------------------------------------------
 */
std::string Lockdep::try_name_percpu_lock(ulong lock_addr) {
    /* candidate per-CPU lock symbols to probe */
    static const char *candidates[] = {
        "softirq_ctrl",   /* PREEMPT_RT BH lock */
        "bh_lock_key",    /* older BH lock variant */
        nullptr
    };

    for (int i = 0; candidates[i]; i++) {
        if (!csymbol_exists(candidates[i])) continue;
        ulong base = csymbol_value(candidates[i]);
        std::vector<ulong> percpu_addrs = for_each_percpu(base);

        for (size_t cpu = 0; cpu < percpu_addrs.size(); cpu++) {
            ulong candidate_addr = percpu_addrs[cpu];
            if (!candidate_addr) continue;
            /* check a small range around the per-CPU base in case the
             * rt_mutex_base is at a small offset within the structure */
            if (lock_addr >= candidate_addr &&
                lock_addr <  candidate_addr + 512) {
                char buf[64];
                snprintf(buf, sizeof(buf), "%s (CPU %zu)", candidates[i], cpu);
                return buf;
            }
        }
    }
    return "";
}

/* -------------------------------------------------------------------------
 * find_held_locks_all — unified held-lock discovery combining three sources:
 *   1. pi_waiters traversal  (authoritative, finds locks WITH waiters)
 *   2. Stack scan            (heuristic, finds locks still in active frames)
 *   3. Per-CPU identification (names BH/IRQ locks)
 *
 * Returns parallel vectors: lock_addrs, names, sources.
 * Deduplicates across all three sources.
 * -------------------------------------------------------------------------
 */
std::vector<ulong> Lockdep::find_held_locks_all(
        ulong task_addr,
        std::vector<std::string> &names,
        std::vector<std::string> &sources)
{
    std::vector<ulong>       result;
    std::unordered_set<ulong> seen;

    auto add = [&](ulong la, const std::string &src) {
        if (la && is_kvaddr(la) && seen.insert(la).second) {
            result.push_back(la);
            /* name: try symbol, then per-CPU identification */
            std::string sym = to_symbol(la);
            bool has_sym = !sym.empty() && sym[0] != '[';
            if (!has_sym) sym = try_name_percpu_lock(la);
            names.push_back(sym.empty() ? "?" : sym);
            sources.push_back(src);
        }
    };

    /* Source 1: pi_waiters (most reliable for PREEMPT_RT, finds locks with waiters) */
    for (ulong la : find_held_locks_pi_waiters(task_addr))
        add(la, "pi_chain");

    /* Source 2: stack scan (finds locks still in active call frames) */
    for (auto &h : find_held_locks_on_stack(task_addr))
        add(h.first, "inferred");

    return result;
}

/* -------------------------------------------------------------------------
 * find_held_locks_on_stack — scan task stack for held mutex/rt_mutex/rwsem.
 * Used as fallback when CONFIG_LOCKDEP is not available.
 * -------------------------------------------------------------------------
 */
std::vector<std::pair<ulong, Lockdep::LockType>>
Lockdep::find_held_locks_on_stack(ulong task_addr) {
    using P = std::pair<ulong, LockType>;
    std::vector<P> result;
    std::unordered_set<ulong> seen;

    ulong base  = GET_STACKBASE(task_addr);
    ulong top   = GET_STACKTOP(task_addr);
    size_t bytes = top - base;
    std::vector<ulong> words(bytes / sizeof(ulong));
    if (!read_struct(base, words.data(), (int)bytes, "held-scan"))
        return result;

    for (ulong v : words) {
        if (!is_kvaddr(v) || is_kernel_text(v)) continue;
        if (v >= base && v < top) continue;
        if (!seen.insert(v).second) continue;

        /* rt_mutex held */
        if (field_offset(rt_mutex_base, owner) != -1) {
            ulong raw  = read_pointer(v + field_offset(rt_mutex_base, owner), "rth.o");
            ulong own  = rt_mutex_owner_task(raw);
            if (own == task_addr &&
                !(raw & ~(RT_MUTEX_OWNER_MASK | RT_MUTEX_HAS_WAITERS))) {
                result.push_back(P(v, LockType::RT_MUTEX));
                continue;
            }
        }
        /* mutex held */
        if (field_offset(mutex, owner) != -1) {
            ulong raw  = read_pointer(v + field_offset(mutex, owner), "mxh.o");
            ulong own  = mutex_owner_task(raw);
            if (own == task_addr &&
                (raw & ~MUTEX_FLAG_MASK) == own) {
                result.push_back(P(v, LockType::MUTEX));
                continue;
            }
        }
        /* rwsem held (exclusive writer only) */
        if (field_offset(rw_semaphore, owner) != -1 &&
            field_offset(rw_semaphore, count) != -1) {
            ulong ow  = read_ulong(v + field_offset(rw_semaphore, owner), "rsh.o");
            if (!(ow & RWSEM_READER_OWNED)) {
                ulong own = rwsem_owner_task(ow);
                if (own == task_addr) {
                    ulong cnt = read_ulong(v + field_offset(rw_semaphore, count),
                                           "rsh.cnt");
                    if (cnt & RWSEM_WRITER_LOCKED) {
                        result.push_back(P(v, LockType::RWSEM));
                        continue;
                    }
                }
            }
        }
    }
    return result;
}

/* -------------------------------------------------------------------------
 * collect_semaphore_waiters — walk semaphore.wait_list to get waiter tasks.
 * -------------------------------------------------------------------------
 */
std::vector<ulong> Lockdep::collect_semaphore_waiters(ulong sem_addr, ulong skip_task) {
    std::vector<ulong> result;
    if (field_offset(semaphore, wait_list)   == -1 ||
        field_offset(semaphore_waiter, list) == -1 ||
        field_offset(semaphore_waiter, task) == -1)
        return result;

    ulong head = sem_addr + field_offset(semaphore, wait_list);
    ulong cur  = read_pointer(head, "sem.wl.next");
    std::unordered_set<ulong> seen;
    for (int s = 0; is_kvaddr(cur) && cur != head && s < 64; s++) {
        ulong w = cur - field_offset(semaphore_waiter, list);
        ulong t = read_pointer(w + field_offset(semaphore_waiter, task),
                               "sem_w.task");
        if (t && is_kvaddr(t) && t != skip_task && seen.insert(t).second)
            result.push_back(t);
        cur = read_pointer(cur, "list.next");
    }
    return result;
}

/* -------------------------------------------------------------------------
 * describe_spinlock — decode qspinlock.val and print state.
 * Returns 0 if unlocked, 1 if locked, -1 if invalid.
 * -------------------------------------------------------------------------
 */
int Lockdep::describe_spinlock(ulong lock_addr) {
    if (!is_kvaddr(lock_addr)) return -1;
    uint32_t val = read_uint(lock_addr, "spin.val");
    int locked  = (int)(val & 0xff);
    int pending = (int)((val >> 8) & 0x1);
    int tail    = (int)(val >> 9);
    PRINT("  [qspinlock]  val=0x%08x\n", val);
    PRINT("    locked=%d  pending=%d  tail_queue=%d\n", locked, pending, tail);
    if (!locked && !pending && !tail) {
        PRINT("  state: UNLOCKED\n");
        return 0;
    }
    if (locked) PRINT("  state: LOCKED (holder CPU not encodeable in qspinlock)\n");
    if (pending) PRINT("  pending: 1 waiter spinning in pending slot\n");
    if (tail)    PRINT("  MCS queue: %d entry/entries waiting\n", tail);
    PRINT("  -> use 'bt -a' or 'bt -c N' to find the holder\n");
    return 1;
}

/* -------------------------------------------------------------------------
 * show_lock_smart — auto-detect whether addr is a task or lock, dispatch.
 * -------------------------------------------------------------------------
 */
void Lockdep::show_lock_smart(ulong addr) {
    if (!addr || !is_kvaddr(addr)) {
        LOGE("Invalid address: 0x%lx", addr);
        return;
    }

    /* Try as task_struct */
    struct task_context *tc = task_to_context(addr);
    if (tc) {
        show_task_lock_snapshot(addr);
        return;
    }

    /* Try as lock (any known type) */
    LockType lt = classify_lock_type("", addr);
    if (lt != LockType::NONE) {
        show_lock_object(addr);
        return;
    }

    LOGE("0x%lx: not recognized as task_struct or lock\n"
         "  Try: bt to find a task addr, or lockdep -t for system summary", addr);
}

int Lockdep::task_prio(ulong task_addr) {
    if (field_offset(task_struct, prio) == -1) return -1;
    return read_int(task_addr + field_offset(task_struct, prio), "prio");
}

std::string Lockdep::format_prio_str(ulong task_addr) {
    int prio = task_prio(task_addr);
    if (prio < 0) return "prio=?";

    int nprio = -1;
    if (field_offset(task_struct, normal_prio) != -1)
        nprio = read_int(task_addr + field_offset(task_struct, normal_prio), "nprio");

    if (nprio < 0 || prio == nprio) {
        char buf[32];
        snprintf(buf, sizeof(buf), "prio=%d", prio);
        return buf;
    }

    /* Boosted: find source via pi_waiters.rb_leftmost */
    std::string src;
    int off_piw = field_offset(task_struct,    pi_waiters),
        off_pit = field_offset(rt_mutex_waiter, pi_tree),
        off_tsk = field_offset(rt_mutex_waiter, task);
    if (off_piw != -1 && off_pit != -1 && off_tsk != -1) {
        ulong lm = is_kvaddr(
            read_pointer(task_addr + off_piw + sizeof(ulong), "pi_waiters.lm"));
        if (is_kvaddr(lm)) {
            ulong st = is_kvaddr(
                read_pointer(lm - off_pit + off_tsk, "pi_top.task"));
            if (is_kvaddr(st)) {
                struct task_context *tc = task_to_context(st);
                if (tc) {
                    char tmp[80];
                    snprintf(tmp, sizeof(tmp), " by PID %d %s", (int)tc->pid, tc->comm);
                    src = tmp;
                }
            }
        }
    }

    char buf[192];
    snprintf(buf, sizeof(buf), "prio=%d [normal=%d, PI\xe2\x86\x91%s]", prio, nprio, src.c_str());
    return buf;
}

void Lockdep::show_lock_object(ulong lock_addr) {
    if (!is_kvaddr(lock_addr)) {
        LOGE("Invalid lock address: 0x%lx", lock_addr);
        return;
    }

    /* ---- Classify ---- */
    LockType ltype = classify_lock_type("", lock_addr);
    if (ltype == LockType::NONE) {
        LOGE("0x%lx: cannot identify as mutex/rwsem/rt_mutex\n"
             "  (raw_spinlock/arch_spinlock have no waiter list and are not analysable)",
             lock_addr);
        return;
    }

    if (ltype == LockType::SPINLOCK) {
        PRINT("raw_spinlock:%s\n", to_hex_addr(lock_addr).c_str());
        describe_spinlock(lock_addr);
        return;
    }
    if (ltype == LockType::SEMAPHORE) {
        uint32_t cnt = (field_offset(semaphore, count) != -1)
            ? read_uint(lock_addr + field_offset(semaphore, count), "sem.count") : 0;
        std::string sym_s = to_symbol(lock_addr);
        bool hs_s = !sym_s.empty() && sym_s[0] != '[';
        PRINT("semaphore:%s%s%s\n",
              to_hex_addr(lock_addr).c_str(),
              hs_s ? " (" : "",
              hs_s ? sym_s.c_str() : "");
        if (hs_s) PRINT(")\n");
        else PRINT("\n");
        PRINT("├─ count=%u%s\n", cnt, cnt == 0 ? "  [LOCKED]" : "");
        auto waiters_s = collect_semaphore_waiters(lock_addr, 0);
        if (waiters_s.empty()) {
            PRINT("└─ waiters(0)\n\n");
        } else {
            PRINT("└─ waiters(%zu):\n", waiters_s.size());
            for (size_t i = 0; i < waiters_s.size(); i++) {
                struct task_context *wtc = task_to_context(waiters_s[i]);
                if (wtc)
                    PRINT("   %s #%zu  PID %-6d  %-22s  prio=%d  [%s]\n",
                          (i + 1 == waiters_s.size()) ? "└─" : "├─",
                          i, (int)wtc->pid, wtc->comm, task_prio(waiters_s[i]),
                          task_state_display(waiters_s[i]).c_str());
            }
            PRINT("\n");
        }
        return;
    }
    if (ltype == LockType::COMPLETION) {
        uint32_t done_v = (field_offset(completion, done) != -1)
            ? read_uint(lock_addr + field_offset(completion, done), "comp.done") : 0;
        std::string sym_c = to_symbol(lock_addr);
        bool hs_c = !sym_c.empty() && sym_c[0] != '[';
        PRINT("completion:%s%s%s\n",
              to_hex_addr(lock_addr).c_str(),
              hs_c ? " (" : "",
              hs_c ? sym_c.c_str() : "");
        if (hs_c) PRINT(")\n");
        else PRINT("\n");
        PRINT("├─ done=%u%s\n", done_v, done_v == 0 ? "  [PENDING]" : "  [COMPLETE]");
        auto waiters_c = collect_lock_waiters(lock_addr, LockType::COMPLETION, 0);
        if (waiters_c.empty()) {
            PRINT("└─ waiters(0)\n\n");
        } else {
            PRINT("└─ waiters(%zu):\n", waiters_c.size());
            for (size_t i = 0; i < waiters_c.size(); i++) {
                struct task_context *wtc = task_to_context(waiters_c[i]);
                if (wtc)
                    PRINT("   %s #%zu  PID %-6d  %-22s\n",
                          (i + 1 == waiters_c.size()) ? "└─" : "├─",
                          i, (int)wtc->pid, wtc->comm);
            }
            PRINT("\n");
        }
        return;
    }

    const char *type_str = (ltype == LockType::RT_MUTEX) ? "rt_mutex"
                         : (ltype == LockType::RWSEM)    ? "rwsem"
                         :                                  "mutex";

    /* ---- Symbol and state flags ---- */
    std::string sym    = to_symbol(lock_addr);
    bool        has_sym = !sym.empty() && sym[0] != '[';

    std::string flags_str;
    ulong       holder_task = 0;
    int         holder_prio = 140;
    ulong       readers     = 0;

    if (ltype == LockType::MUTEX && field_offset(mutex, owner) != -1) {
        ulong raw = read_pointer(lock_addr + field_offset(mutex, owner), "mutex.owner");
        flags_str   = decode_mutex_owner(raw);
        holder_task = mutex_owner_task(raw);
    } else if (ltype == LockType::RT_MUTEX && field_offset(rt_mutex_base, owner) != -1) {
        ulong raw = read_pointer(lock_addr + field_offset(rt_mutex_base, owner), "rt.owner");
        flags_str   = decode_rt_mutex_owner(raw);
        holder_task = rt_mutex_owner_task(raw);
    } else if (ltype == LockType::RWSEM) {
        if (field_offset(rw_semaphore, count) != -1) {
            ulong cnt = read_ulong(lock_addr + field_offset(rw_semaphore, count), "rwsem.count");
            flags_str = decode_rwsem_count(cnt);
            readers   = cnt >> RWSEM_READER_SHIFT;
        }
        if (field_offset(rw_semaphore, owner) != -1) {
            ulong ow = read_ulong(lock_addr + field_offset(rw_semaphore, owner),
                                  "rwsem.owner");
            if (!(ow & RWSEM_READER_OWNED)) holder_task = rwsem_owner_task(ow);
        }
    } else if (ltype == LockType::PERCPU_RWSEM) {
        if (field_offset(percpu_rw_semaphore, block) != -1) {
            int blk = read_int(lock_addr + field_offset(percpu_rw_semaphore, block),
                               "percpu_rwsem.block");
            flags_str = blk ? "write-locked" : "unlocked";
        }
        if (field_offset(percpu_rw_semaphore, writer) != -1 &&
            field_offset(rcuwait, task) != -1) {
            ulong wt = read_pointer(
                lock_addr + field_offset(percpu_rw_semaphore, writer) +
                    field_offset(rcuwait, task),
                "percpu_rwsem.writer.task");
            wt = is_kvaddr(wt);
            if (wt && is_kvaddr(wt)) holder_task = wt;
        }
    }

    std::string lock_head = std::string(type_str) + ":" + to_hex_addr(lock_addr);
    if (has_sym) lock_head += " (" + sym + ")";
    PRINT("%s\n", lock_head.c_str());
    PRINT("state: %s\n", flags_str.c_str());
    if (ltype == LockType::RWSEM && readers > 0)
        PRINT("readers: %lu concurrent\n", readers);
    PRINT("\n");

    /* ---- Holder ---- */
    PRINT("holder:\n");

    /* LOCKDEP init (needed for acquire-site lookup) */
    bool lockdep_ok = (get_config_val("CONFIG_LOCKDEP") == "y" && ensure_lockdep_globals());

    if (holder_task && is_kvaddr(holder_task)) {
        struct task_context *htc = task_to_context(holder_task);
        if (htc) {
            holder_prio = task_prio(holder_task);
            PRINT("  %s\n", format_task_line(holder_task).c_str());

            /* Acquire site from held_locks[] */
            if (lockdep_ok) {
                ulong da = holder_task + field_offset(task_struct, lockdep_depth);
                uint32_t depth = is_kvaddr(da) ? read_uint(da, "lockdep_depth") : 0;
                if (depth && depth <= 64) {
                    TaskLockdepInfo hi;
                    hi.tc = htc;
                    hi.lockdep_depth = depth;
                    parse_task_held_locks(holder_task, hi);
                    for (const auto &hl : hi.held_locks) {
                        if (hl.instance == lock_addr && hl.acquire_ip) {
                            char *fn = closest_symbol(hl.acquire_ip);
                            if (fn) PRINT("  acquired: %s\n", fn);
                            break;
                        }
                    }
                }
            }

            /* Is holder itself waiting on another lock? */
            LockType bt  = LockType::NONE;
            ulong    braw = 0;
            ulong    blk  = find_any_blocking_lock(holder_task, bt, braw);
            if (blk) {
                const char *btype = (bt == LockType::RT_MUTEX) ? "rt_mutex"
                                  : (bt == LockType::RWSEM)    ? "rwsem" : "mutex";
                std::string bsym  = to_symbol(blk);
                bool        bhs   = !bsym.empty() && bsym[0] != '[';
                if (bhs)
                    PRINT("  blocked on: %s @ 0x%lx  (%s)\n", btype, blk, bsym.c_str());
                else
                    PRINT("  blocked on: %s @ 0x%lx\n", btype, blk);
            }
        }
    } else if (ltype == LockType::RT_MUTEX &&
               field_offset(rt_mutex_base, owner) != -1) {
        ulong raw = read_pointer(lock_addr + field_offset(rt_mutex_base, owner), "rt.owner");
        if (!rt_mutex_owner_task(raw) && (raw & RT_MUTEX_HAS_WAITERS)) {
            PRINT("  (HANDOFF pending — no owner, lock is being handed to top waiter)\n");
            /* show next acquirer from rb_leftmost */
            if (field_offset(rt_mutex_base, waiters) != -1 &&
                field_offset(rt_mutex_waiter, task)  != -1) {
                ulong lm = read_pointer(
                    lock_addr + field_offset(rt_mutex_base, waiters) + sizeof(ulong),
                    "rb_leftmost");
                if (lm && is_kvaddr(lm)) {
                    ulong nt = read_pointer(lm + field_offset(rt_mutex_waiter, task),
                                            "lm.task");
                    if (nt && is_kvaddr(nt))
                        PRINT("  next acquirer: %s\n", format_task_line(nt).c_str());
                }
            }
        } else {
            PRINT("  (unlocked)\n");
        }
    } else if (ltype == LockType::RWSEM && readers > 0) {
        PRINT("  (read-locked — no single owner; %lu reader%s active)\n",
              readers, readers == 1 ? "" : "s");
        if (field_offset(rw_semaphore, owner) != -1) {
            ulong ow = read_ulong(lock_addr + field_offset(rw_semaphore, owner),
                                  "rwsem.owner");
            if (ow & RWSEM_READER_OWNED) {
                ulong hint = rwsem_owner_task(ow);
                if (hint && is_kvaddr(hint)) {
                    struct task_context *htc = task_to_context(hint);
                    if (htc) PRINT("  reader hint: PID %d  %s\n", (int)htc->pid, htc->comm);
                }
            }
        }
    } else {
        PRINT("  (unlocked or owner undetermined)\n");
    }
    PRINT("\n");

    /* ---- Waiters ---- */
    auto waiters = collect_lock_waiters(lock_addr, ltype, 0);
    if (waiters.empty()) {
        PRINT("waiters: none\n\n");
    } else {
        PRINT("waiters(%zu):\n", waiters.size());

        /* Pre-collect rwsem waiter types */
        std::unordered_map<ulong, int> rwsem_wtype;
        if (ltype == LockType::RWSEM &&
            field_offset(rw_semaphore, wait_list) != -1 &&
            field_offset(rwsem_waiter, list)      != -1 &&
            field_offset(rwsem_waiter, task)      != -1 &&
            field_offset(rwsem_waiter, type)      != -1) {
            ulong head = lock_addr + field_offset(rw_semaphore, wait_list);
            ulong cur  = read_pointer(head, "wl.next");
            for (int s = 0; is_kvaddr(cur) && cur != head && s < 64; s++) {
                ulong w = cur - field_offset(rwsem_waiter, list);
                ulong t = read_pointer(w + field_offset(rwsem_waiter, task), "wt");
                int   k = read_int(w + field_offset(rwsem_waiter, type), "wtype");
                if (t) rwsem_wtype[t] = k;
                cur = read_pointer(cur, "list.next");
            }
        }

        /* Pre-collect rt_mutex waiter nodes (for PI boosted prio) */
        std::unordered_map<ulong, int> rt_boosted_prio;
        if (ltype == LockType::RT_MUTEX &&
            field_offset(rt_mutex_base, waiters)   != -1 &&
            field_offset(rt_mutex_waiter, task)    != -1 &&
            field_offset(rt_waiter_node, prio)     != -1) {
            ulong rb = lock_addr + field_offset(rt_mutex_base, waiters);
            for (ulong wnode : for_each_rbtree(rb, 0)) {
                if (!is_kvaddr(wnode)) continue;
                ulong t = read_pointer(wnode + field_offset(rt_mutex_waiter, task), "wn.task");
                int   p = read_int(wnode + field_offset(rt_waiter_node, prio), "wn.prio");
                if (t) rt_boosted_prio[t] = p;
            }
        }

        int max_waiter_prio = 140;
        for (size_t i = 0; i < waiters.size(); i++) {
            ulong wt = waiters[i];
            if (!is_kvaddr(wt)) continue;

            struct task_context *wtc = task_to_context(wt);
            int  prio  = task_prio(wt);
            std::string state = task_state_display(wt);
            if (prio < max_waiter_prio) max_waiter_prio = prio;

            /* rwsem: READ/WRITE marker */
            const char *rw_tag = "";
            if (ltype == LockType::RWSEM) {
                auto it = rwsem_wtype.find(wt);
                rw_tag  = (it != rwsem_wtype.end())
                          ? (it->second == 0 ? "  WRITE" : "  READ ")
                          : "  ?    ";
            }

            /* rt_mutex: boosted prio annotation */
            char pi_buf[32] = "";
            if (ltype == LockType::RT_MUTEX) {
                auto it = rt_boosted_prio.find(wt);
                if (it != rt_boosted_prio.end() && it->second != prio)
                    snprintf(pi_buf, sizeof(pi_buf), "  [boosted->%d]", it->second);
            }

            /* held-lock count */
            char holds_buf[16] = "holds:?";
            if (lockdep_ok) {
                ulong da = wt + field_offset(task_struct, lockdep_depth);
                if (is_kvaddr(da)) {
                    uint32_t d = read_uint(da, "lockdep_depth");
                    if (d <= 64) snprintf(holds_buf, sizeof(holds_buf), "holds:%u", d);
                }
            }

            /* rt_mutex: first waiter = next acquirer (rbtree leftmost) */
            const char *next_tag = "";
            if (ltype == LockType::RT_MUTEX && i == 0) next_tag = "  <- next";

            if (wtc)
                PRINT("  #%zu  PID %-6d  %-22s  prio=%-3d  [%s]%s  %s%s%s\n",
                      i, (int)wtc->pid, wtc->comm, prio, state.c_str(),
                      rw_tag, holds_buf, pi_buf, next_tag);
            else
                PRINT("  #%zu  task=0x%lx  [%s]%s  %s%s%s\n",
                      i, wt, state.c_str(), rw_tag, holds_buf, pi_buf, next_tag);
        }

        if (holder_task && holder_prio > max_waiter_prio)
            PRINT("\n  PRIORITY INVERSION: top waiter prio=%d, holder prio=%d\n",
                  max_waiter_prio, holder_prio);
        PRINT("\n");
    }

    /* ---- Chain from holder ---- */
    if (holder_task && is_kvaddr(holder_task)) {
        PRINT("holder-chain:\n");
        std::unordered_set<ulong> visited = {holder_task};
        print_lock_chain(holder_task, "", true, 0, true, visited);
        PRINT("\n");
    }
}

/* =========================================================================
 * Phase A: detect_lock_nesting / print_task_nesting_detail  (-d)
 * =========================================================================
 */

void Lockdep::detect_lock_nesting() {
    if (!ensure_lockdep_globals()) {
        PRINT("CONFIG_LOCKDEP not enabled — lock nesting detection unavailable\n");
        return;
    }

    struct NestInfo {
        ulong task_addr;
        uint32_t depth;
        bool suspicious;
        std::vector<HeldLockInfo> locks;
    };
    std::vector<NestInfo> all;

    for (ulong task : for_each_threads()) {
        if (!is_kvaddr(task)) continue;
        ulong da = task + field_offset(task_struct, lockdep_depth);
        if (!is_kvaddr(da)) continue;
        uint32_t depth = read_uint(da, "lockdep_depth");
        if (!depth || depth > 64) continue;

        NestInfo ni;
        ni.task_addr = task;
        ni.depth = depth;
        ni.suspicious = false;

        TaskLockdepInfo tli;
        tli.tc = task_to_context(task);
        tli.lockdep_depth = depth;
        parse_task_held_locks(task, tli);
        ni.locks = tli.held_locks;

        /* Pattern: repeated class_idx */
        std::unordered_map<uint32_t, int> class_cnt;
        std::unordered_map<ulong, int>    ip_cnt;
        for (const auto &hl : tli.held_locks) {
            class_cnt[hl.class_idx]++;
            if (hl.acquire_ip) ip_cnt[hl.acquire_ip]++;
        }

        if (depth >= 48) {
            ni.suspicious = true;
        } else if (depth >= 8) {
            ni.suspicious = true;
        }

        for (auto it = class_cnt.begin(); it != class_cnt.end(); ++it) {
            if (it->second >= 3) {
                ni.suspicious = true;
            }
        }
        for (auto it = ip_cnt.begin(); it != ip_cnt.end(); ++it) {
            if (it->second >= 3) {
                ni.suspicious = true;
            }
        }
        all.push_back(ni);
    }

    int susp = 0;
    for (const auto &n : all) if (n.suspicious) susp++;

    PRINT("Lock Nesting Analysis:\n");
    PRINT("  tasks with locks=%zu  suspicious=%d%s\n",
          all.size(), susp, susp > 0 ? " ***" : "");
    PRINT("\n");

    for (const auto &ni : all) {
        if (!ni.suspicious) continue;
        struct task_context *tc = task_to_context(ni.task_addr);
        PRINT("*** PID %-6d  %-22s  depth=%u%s\n",
              tc ? (int)tc->pid : -1, tc ? tc->comm : "?",
              ni.depth, ni.depth >= 48 ? "  [CRITICAL]" : "");

        std::unordered_map<uint32_t, int> class_cnt;
        std::unordered_map<ulong, int>    ip_cnt;
        for (const auto &hl : ni.locks) {
            class_cnt[hl.class_idx]++;
            if (hl.acquire_ip) ip_cnt[hl.acquire_ip]++;
        }

        if (ni.depth >= 48)
            PRINT("   - depth high: %u (MAX_LOCK_DEPTH=48)\n", ni.depth);
        else if (ni.depth >= 8)
            PRINT("   - depth high: %u (threshold=8)\n", ni.depth);

        for (auto it = class_cnt.begin(); it != class_cnt.end(); ++it) {
            if (it->second < 3) continue;
            std::string nm = trim_spaces(get_lock_name(it->first));
            PRINT("   - repeated class: '%s' x%d\n",
                  nm.empty() ? "?" : nm.c_str(), it->second);
        }

        for (auto it = ip_cnt.begin(); it != ip_cnt.end(); ++it) {
            if (it->second < 3) continue;
            const char *fn = closest_symbol(it->first);
            PRINT("   - repeated callsite: '%s' x%d\n",
                  fn ? fn : "?", it->second);
        }
        PRINT("\n");
    }

    if (susp == 0) PRINT("No suspicious nesting patterns detected.\n\n");

    /* normal summary */
    if (!all.empty()) {
        PRINT("All tasks with held locks (%zu):\n", all.size());
        int shown = 0;
        for (const auto &ni : all) {
            if (ni.suspicious) continue;
            if (shown++ >= 10) {
                PRINT("  ... (%zu more)\n", all.size() - (size_t)susp - (size_t)shown + 1);
                break;
            }
            struct task_context *tc = task_to_context(ni.task_addr);
            PRINT("  PID %-6d  %-22s  depth=%u\n",
                  tc ? (int)tc->pid : -1, tc ? tc->comm : "?", ni.depth);
        }
        PRINT("\n");
    }
}

void Lockdep::print_task_nesting_detail(ulong task_addr) {
    if (!is_kvaddr(task_addr)) { LOGE("Invalid addr: 0x%lx", task_addr); return; }
    if (!ensure_lockdep_globals()) {
        PRINT("CONFIG_LOCKDEP not enabled\n"); return;
    }

    struct task_context *tc = task_to_context(task_addr);
    if (!tc) { LOGE("No task context for 0x%lx", task_addr); return; }

    ulong da = task_addr + field_offset(task_struct, lockdep_depth);
    uint32_t depth = is_kvaddr(da) ? read_uint(da, "lockdep_depth") : 0;
    if (!depth) { PRINT("PID %d  %s: no held locks\n", tc->pid, tc->comm); return; }
    if (depth > 64) { LOGE("Suspicious depth %u for %s", depth, tc->comm); return; }

    TaskLockdepInfo tli;
    tli.tc = tc; tli.lockdep_depth = depth;
    parse_task_held_locks(task_addr, tli);

    /* pattern analysis */
    std::unordered_map<uint32_t, int> class_cnt;
    std::unordered_map<ulong,    int> ip_cnt;
    for (const auto &hl : tli.held_locks) {
        class_cnt[hl.class_idx]++;
        if (hl.acquire_ip) ip_cnt[hl.acquire_ip]++;
    }

    bool nesting_warn = false;
    for (auto it = class_cnt.begin(); it != class_cnt.end(); ++it)
        if (it->second >= 3) { nesting_warn = true; break; }
    if (!nesting_warn) {
        for (auto it = ip_cnt.begin(); it != ip_cnt.end(); ++it)
            if (it->second >= 3) { nesting_warn = true; break; }
    }

    PRINT("%u locks held by %s (pid=%d, task=0x%lx)%s:\n",
          depth, tc->comm, tc->pid, task_addr,
          nesting_warn ? " [heuristic flags]" : "");

    for (size_t i = 0; i < tli.held_locks.size(); i++) {
        std::string out = format_lock_info_tree((int)i, tli.held_locks[i].instance, tli.held_locks[i]);
        PRINT("%s", out.c_str());
    }

    if (nesting_warn) {
        PRINT("\nNesting pattern summary (heuristic):\n");
        PRINT("  repeated classes:\n");
        for (auto it = class_cnt.begin(); it != class_cnt.end(); ++it) {
            if (it->second < 3) continue;
            std::string nm = trim_spaces(get_lock_name(it->first));
            PRINT("   - class '%s' x%d\n", nm.empty() ? "?" : nm.c_str(), it->second);
        }

        PRINT("  repeated callsites:\n");
        for (auto it = ip_cnt.begin(); it != ip_cnt.end(); ++it) {
            if (it->second < 3) continue;
            const char *fn = closest_symbol(it->first);
            PRINT("   - callsite '%s' x%d\n", fn ? fn : "?", it->second);
        }
    }
}

/* =========================================================================
 * Chain rendering helpers for -t task snapshots
 * ========================================================================= */

std::string Lockdep::format_task_entry(ulong taddr, bool detailed) {
    struct task_context *tc = task_to_context(taddr);
    if (!tc) {
        std::string s = "[Core?:?]task=" + to_hex_addr(taddr);
        if (detailed) s += "  state:?  prio:?";
        return s;
    }
    int cpu = (int)tc->processor;
    if (!detailed) {
        std::ostringstream oss;
        oss << "[Core" << cpu << ":" << tc->pid << "]" << tc->comm;
        return oss.str();
    }
    char buf[320];
    std::string state_txt = task_state_display(taddr);
    snprintf(buf, sizeof(buf), "[Core%d:%-6lu%s] %-20.20s  state:%s  %s",
             cpu, (unsigned long)tc->pid, (cpu >= 0 && cpu < 10) ? " " : "",
             tc->comm, state_txt.c_str(), format_prio_str(taddr).c_str());
    return buf;
}

std::string Lockdep::chain_lock_label(ulong lock_addr, LockType ltype) {
    const char *tn = (ltype == LockType::RT_MUTEX)    ? "rt_mutex"
                   : (ltype == LockType::RWSEM)        ? "rwsem"
                   : (ltype == LockType::PERCPU_RWSEM) ? "percpu_rwsem"
                   : (ltype == LockType::MUTEX)        ? "mutex"
                   :                                     "unknown";
    std::ostringstream oss;
    oss << tn << ":" << to_hex_addr(lock_addr);
    std::string sym = to_symbol(lock_addr);
    if (!sym.empty() && sym[0] != '[') oss << " (" << sym << ")";
    return oss.str();
}

ulong Lockdep::chain_follow_task(ulong lock_addr, LockType ltype, ulong owner_raw) {
    ulong t = 0;
    if (ltype == LockType::RT_MUTEX)  t = rt_mutex_owner_task(owner_raw);
    else if (ltype == LockType::MUTEX) t = mutex_owner_task(owner_raw);
    else if (ltype == LockType::RWSEM && field_offset(rw_semaphore, owner) != -1) {
        ulong raw = read_ulong(lock_addr + field_offset(rw_semaphore, owner), "rwsem.owner");
        if (!(raw & RWSEM_READER_OWNED)) t = rwsem_owner_task(raw);
    }
    /* PERCPU_RWSEM: no direct owner field; owner_raw is not a task pointer */
    if (t && is_kvaddr(t) && task_to_context(t)) return t;
    /* RT_MUTEX with no direct owner: try handoff (next waiter) */
    if (ltype == LockType::RT_MUTEX &&
        field_offset(rt_mutex_base,   waiters) != -1 &&
        field_offset(rt_mutex_waiter, task)    != -1) {
        ulong lm = is_kvaddr(read_pointer(
            lock_addr + field_offset(rt_mutex_base, waiters) + sizeof(ulong), "rb_lm"));
        if (lm && is_kvaddr(lm)) {
            t = is_kvaddr(
                read_pointer(lm + field_offset(rt_mutex_waiter, task), "lm.task"));
            if (t && is_kvaddr(t) && task_to_context(t)) return t;
        }
    }
    return 0;
}

/* Returns true and prints STUCK annotation if this rt_mutex lock is an rwlock with active readers */
bool Lockdep::try_print_rwlock_stuck(ulong rt_mutex_addr, const std::string& prefix) {
    ulong raddr = rt_mutex_addr - 8;   /* rwbase_rt.readers sits 8 bytes before rt_mutex_base */
    if (!is_kvaddr(raddr)) return false;
    int readers = read_int(raddr, "rwb.readers");
    if (readers <= 0 || readers >= 200) return false;
    PRINT("%s\xe2\x94\x94\xe2\x94\x80 [STUCK: rwlock write-lock pending, %d active reader(s) not yet released]\n",
          prefix.c_str(), readers);
    PRINT("%s   Active reader holds lock via atomic fast-path \xe2\x80\x94 not visible to PI chain\n",
          prefix.c_str());
    PRINT("%s   -> use foreach_bt + diff rt_mutex waiter-list to locate reader\n",
          prefix.c_str());
    return true;
}

void Lockdep::print_lock_chain(ulong cur_task, const std::string& prefix,
                                bool is_last, int depth, bool print_self,
                                std::unordered_set<ulong>& visited) {
    const int max_depth = 10;

    LockType ltype = LockType::NONE;
    ulong owner_raw = 0;
    ulong lock_addr = find_any_blocking_lock(cur_task, ltype, owner_raw);

    std::string line = format_task_entry(cur_task) + " blocked on ";
    line += lock_addr ? chain_lock_label(lock_addr, ltype) : "<none>";

    if (print_self) {
        if (depth == 0)
            PRINT("%s\n", line.c_str());
        else
            PRINT("%s%s%s\n", prefix.c_str(), is_last ? "\xe2\x94\x94\xe2\x94\x80 " : "\xe2\x94\x9c\xe2\x94\x80 ", line.c_str());
    }

    if (depth >= max_depth) return;

    if (!lock_addr) {
        /* Held-only: expand rt_mutex locks held by this task (no blocked lock) */
        auto held = find_held_locks_pi_waiters(cur_task);
        std::string cp = (depth == 0 && print_self)
            ? "" : prefix + (is_last ? "   " : "\xe2\x94\x82  ");
        for (size_t hi = 0; hi < held.size(); hi++) {
            ulong la = held[hi];
            bool last_held = (hi + 1 == held.size());
            PRINT("%s%s holds %s\n", cp.c_str(),
                  last_held ? "\xe2\x94\x94\xe2\x94\x80" : "\xe2\x94\x9c\xe2\x94\x80",
                  chain_lock_label(la, LockType::RT_MUTEX).c_str());
            std::string hp = cp + (last_held ? "   " : "\xe2\x94\x82  ");
            if (try_print_rwlock_stuck(la, hp)) continue;
            auto wlist = collect_lock_waiters(la, LockType::RT_MUTEX, cur_task);
            PRINT("%s\xe2\x94\x94\xe2\x94\x80 waiters(%zu)\n", hp.c_str(), wlist.size());
            std::string wp = hp + "   ";
            for (size_t wi = 0; wi < wlist.size(); wi++) {
                PRINT("%s%s%s\n", wp.c_str(),
                      (wi + 1 == wlist.size()) ? "\xe2\x94\x94\xe2\x94\x80 " : "\xe2\x94\x9c\xe2\x94\x80 ",
                      format_task_entry(wlist[wi], true).c_str());
            }
        }
        return;
    }

    std::vector<ulong> waiters = collect_lock_waiters(lock_addr, ltype, cur_task);
    std::string child_prefix = (depth == 0 && print_self)
        ? "" : prefix + (is_last ? "   " : "\xe2\x94\x82  ");

    ulong follow_task = chain_follow_task(lock_addr, ltype, owner_raw);
    bool  has_follow  = follow_task && is_kvaddr(follow_task);

    std::string owner_desc = "held by <unknown>";
    if (has_follow) {
        owner_desc = "held by " + format_task_entry(follow_task);
    } else if (ltype == LockType::RWSEM && field_offset(rw_semaphore, owner) != -1) {
        ulong raw = read_ulong(lock_addr + field_offset(rw_semaphore, owner), "rwsem.owner");
        if (raw & RWSEM_READER_OWNED) owner_desc = "held by <reader-owned>";
    } else if (ltype == LockType::RT_MUTEX && (owner_raw & RT_MUTEX_HAS_WAITERS)) {
        owner_desc = "held by <handoff>";
    }
    PRINT("%s\xe2\x94\x9c\xe2\x94\x80 %s\n", child_prefix.c_str(), owner_desc.c_str());

    PRINT("%s%s waiters(%zu)\n",
          child_prefix.c_str(), has_follow ? "\xe2\x94\x9c\xe2\x94\x80" : "\xe2\x94\x94\xe2\x94\x80",
          waiters.size());
    std::string wp = child_prefix + (has_follow ? "\xe2\x94\x82  " : "   ");
    for (size_t i = 0; i < waiters.size(); i++) {
        PRINT("%s%s%s\n", wp.c_str(),
              (i + 1 == waiters.size()) ? "\xe2\x94\x94\xe2\x94\x80 " : "\xe2\x94\x9c\xe2\x94\x80 ",
              format_task_entry(waiters[i], true).c_str());
    }

    if (!has_follow) return;

    if (visited.count(follow_task)) {
        PRINT("%s\xe2\x94\x94\xe2\x94\x80 %s blocked on [cycle]\n",
              child_prefix.c_str(), format_task_entry(follow_task).c_str());
        return;
    }
    if (depth + 1 >= max_depth) {
        PRINT("%s\xe2\x94\x94\xe2\x94\x80 %s blocked on [max-depth]\n",
              child_prefix.c_str(), format_task_entry(follow_task).c_str());
        return;
    }

    LockType nt = LockType::NONE; ulong nr = 0;
    ulong next_lock = find_any_blocking_lock(follow_task, nt, nr);
    if (!next_lock || nt == LockType::NONE) {
        bool annotated = false;
        for (ulong la : find_held_locks_pi_waiters(follow_task))
            if (try_print_rwlock_stuck(la, child_prefix)) { annotated = true; break; }
        if (!annotated)
            PRINT("%s\xe2\x94\x94\xe2\x94\x80 %s blocked on <none>\n",
                  child_prefix.c_str(), format_task_entry(follow_task).c_str());
        return;
    }
    PRINT("%s\xe2\x94\x94\xe2\x94\x80 %s blocked on %s\n",
          child_prefix.c_str(), format_task_entry(follow_task).c_str(),
          chain_lock_label(next_lock, nt).c_str());
    visited.insert(follow_task);
    print_lock_chain(follow_task, child_prefix, true, depth + 1, false, visited);
    visited.erase(follow_task);
}

/* =========================================================================
 * Tree-chain Investigation  (-t)
 * ========================================================================= */

void Lockdep::show_task_lock_snapshot(ulong task_addr) {
    if (!is_kvaddr(task_addr)) {
        LOGE("Invalid task address: 0x%lx", task_addr);
        return;
    }
    struct task_context *tc = task_to_context(task_addr);
    if (!tc) {
        LOGE("No task_context for 0x%lx", task_addr);
        return;
    }

    std::unordered_set<ulong> visited = {task_addr};
    print_lock_chain(task_addr, "", true, 0, true, visited);
}

/* =========================================================================
 * Comprehensive lock analysis (used by -t without target)
 * =========================================================================
 */

/* scan_stack_refs — collect candidate lock addresses found on a task's stack. */
void Lockdep::scan_stack_refs(ulong task_addr,
                               std::unordered_map<ulong, int> &refs) {
    ulong base  = GET_STACKBASE(task_addr);
    ulong top   = GET_STACKTOP(task_addr);
    size_t bytes = top - base;
    std::vector<ulong> words(bytes / sizeof(ulong));
    if (!read_struct(base, words.data(), (int)bytes, "stack-ref"))
        return;
    for (ulong v : words) {
        if (!is_kvaddr(v) || is_kernel_text(v)) continue;
        if (v >= base && v < top) continue;
        refs[v]++;
    }
}

/* find_deadlock_cycles — standard iterative Tarjan, returns all SCCs. */
void Lockdep::find_deadlock_cycles(
    const std::unordered_map<ulong, std::vector<ulong>> &g,
    const std::vector<ulong> &nodes,
    std::vector<std::vector<ulong>> &sccs)
{
    std::unordered_map<ulong,int>  idx, low;
    std::unordered_map<ulong,bool> on_stk;
    std::vector<ulong> stk;
    int counter = 0;

    std::function<void(ulong)> sc = [&](ulong v) {
        idx[v] = low[v] = counter++;
        stk.push_back(v);
        on_stk[v] = true;
        auto it = g.find(v);
        if (it != g.end()) {
            for (ulong w : it->second) {
                if (!idx.count(w))       { sc(w); low[v] = std::min(low[v], low[w]); }
                else if (on_stk.count(w) && on_stk[w])
                                         { low[v] = std::min(low[v], idx[w]); }
            }
        }
        if (low[v] == idx[v]) {
            std::vector<ulong> scc;
            ulong w;
            do { w = stk.back(); stk.pop_back(); on_stk[w] = false; scc.push_back(w); }
            while (w != v);
            sccs.push_back(std::move(scc));
        }
    };
    for (ulong v : nodes) if (!idx.count(v)) sc(v);
}

void Lockdep::collect_lock_analysis(std::vector<TaskScanInfo> &tasks,
    std::unordered_map<ulong, LockScanInfo> &lmap)
{
    bool ldep = (get_config_val("CONFIG_LOCKDEP") == "y" && ensure_lockdep_globals());

    /* Phase 1 — enumerate tasks */
    for (ulong task : for_each_threads()) {
        if (!is_kvaddr(task)) continue;

        TaskScanInfo info;
        info.task_addr = task;

        /* held locks from held_locks[] */
        if (ldep) {
            ulong da = task + field_offset(task_struct, lockdep_depth);
            if (is_kvaddr(da)) {
                uint32_t depth = read_uint(da, "depth");
                if (depth > 0 && depth <= 64) {
                    TaskLockdepInfo tli;
                    tli.tc = task_to_context(task);
                    tli.lockdep_depth = depth;
                    parse_task_held_locks(task, tli);
                    for (auto &hl : tli.held_locks) {
                        if (!hl.instance || !is_kvaddr(hl.instance)) continue;
                        info.held_addrs.push_back(hl.instance);
                        info.held_names.push_back(hl.lock_name);
                        info.held_ips.push_back(hl.acquire_ip);
                        if (!lmap.count(hl.instance)) {
                            LockScanInfo li;
                            li.type   = classify_lock_type(hl.lock_name, hl.instance);
                            li.name   = hl.lock_name;
                            li.holder = task;
                            lmap[hl.instance] = li;
                        } else {
                            lmap[hl.instance].holder = task;
                        }
                    }
                }
            }
        }

        if (!ldep) {
            std::vector<std::string> names, srcs;
            auto lv = find_held_locks_all(task, names, srcs);
            for (size_t i = 0; i < lv.size(); i++) {
                ulong la = lv[i];
                info.held_addrs.push_back(la);
                info.held_names.push_back(names[i]);
                info.held_ips.push_back(0);
                if (!lmap.count(la)) {
                    LockScanInfo li;
                    li.type   = classify_lock_type(names[i], la);
                    li.name   = names[i];
                    li.holder = task;
                    lmap[la]  = li;
                } else {
                    lmap[la].holder = task;
                }
            }
        }

        /* blocked lock */
        LockType bt = LockType::NONE; ulong braw = 0;
        ulong blk = find_any_blocking_lock(task, bt, braw);
        if (blk) {
            info.blocked_lock = blk;
            info.blocked_type = bt;
            if      (bt == LockType::RT_MUTEX)
                info.blocked_owner = rt_mutex_owner_task(braw);
            else if (bt == LockType::RWSEM && field_offset(rw_semaphore, owner) != -1) {
                ulong ow = read_ulong(blk + field_offset(rw_semaphore, owner), "rso");
                if (!(ow & RWSEM_READER_OWNED)) info.blocked_owner = rwsem_owner_task(ow);
            } else if (bt == LockType::MUTEX)
                info.blocked_owner = mutex_owner_task(braw);

            if (!lmap.count(blk)) {
                LockScanInfo li;
                li.type   = bt;
                std::string s = to_symbol(blk);
                li.name   = (!s.empty() && s[0] != '[') ? s : "";
                li.holder = info.blocked_owner;
                lmap[blk] = li;
            }
        } else {
            std::string st = task_state_display(task);
            info.blocked_is_io = (st.rfind("UN", 0) == 0 || st.rfind("ID", 0) == 0);
        }

        tasks.push_back(std::move(info));
    }

    /* Phase 2 — waiters for all known locks.
     * For RT_MUTEX use brute-force pi_blocked_on scan only — the rbtree may be
     * partially corrupted at crash time (HANDOFF state, partial rebalance), and
     * for_each_rbtree will print noise for every bad node it visits. */
    for (auto &_p : lmap) {
        auto &la = _p.first; auto &li = _p.second;
        if (li.type == LockType::NONE) continue;

        if (li.type == LockType::RT_MUTEX) {
            /* brute-force: scan all tasks' pi_blocked_on */
            if (field_offset(task_struct, pi_blocked_on) != -1 &&
                field_offset(rt_mutex_waiter, lock)      != -1 &&
                field_offset(rt_mutex_waiter, task)      != -1) {
                std::unordered_set<ulong> seen;
                for (ulong t2 : for_each_threads()) {
                    ulong pw = read_pointer(
                        t2 + field_offset(task_struct, pi_blocked_on), "pi_bo");
                    if (!pw || !is_kvaddr(pw)) continue;
                    ulong lk = read_pointer(
                        pw + field_offset(rt_mutex_waiter, lock), "pw.lock");
                    if (lk != la) continue;
                    ulong wt = read_pointer(
                        pw + field_offset(rt_mutex_waiter, task), "pw.task");
                    if (wt && is_kvaddr(wt) && seen.insert(wt).second)
                        li.waiters.push_back(wt);
                }
            }
        } else {
            li.waiters = collect_lock_waiters(la, li.type, 0);
        }
    }

    /* Phase 3 — stack hotspot: count refs only for already-confirmed locks.
     * Stack addresses are not used to create new lock entries — pure heuristic
     * classification on raw stack words generates too many false positives
     * (global variables, data structures with pointer-like fields). */
    std::unordered_map<ulong, int> refs;
    for (const auto &t : tasks) {
        std::string st = task_state_display(t.task_addr);
        if (st.rfind("UN", 0) == 0 || st.rfind("ID", 0) == 0)
            scan_stack_refs(t.task_addr, refs);
    }
    for (auto &_r : refs) {
        auto la = _r.first; auto cnt = _r.second;
        if (lmap.count(la))
            lmap[la].stack_refs += cnt;
    }

    /* Phase 4 — mark external locks */
    std::unordered_set<ulong> tset;
    for (const auto &t : tasks) tset.insert(t.task_addr);
    for (auto &_p : lmap) {
        auto la = _p.first; auto &li = _p.second;
        (void)la;
        if (li.holder && !tset.count(li.holder)) li.is_external = true;
    }
}

void Lockdep::print_lock_analysis(
    std::vector<TaskScanInfo> &tasks,
    std::unordered_map<ulong, LockScanInfo> &lmap)
{
    if (tasks.empty()) { PRINT("No tasks found.\n"); return; }

    std::unordered_map<ulong, const TaskScanInfo *> tmap;
    for (const auto &t : tasks) tmap[t.task_addr] = &t;

    auto lts = [](LockType t) -> const char * {
        if (t == LockType::RT_MUTEX)   return "rt_mutex";
        if (t == LockType::RWSEM)      return "rwsem  ";
        if (t == LockType::MUTEX)      return "mutex  ";
        if (t == LockType::SEMAPHORE)  return "sema   ";
        if (t == LockType::COMPLETION) return "compl  ";
        if (t == LockType::SPINLOCK)   return "spinlk ";
        return "?      ";
    };

    int d_cnt = 0, h_cnt = 0;
    for (const auto &t : tasks) {
        std::string st = task_state_display(t.task_addr);
        if (st.rfind("UN", 0) == 0 || st.rfind("ID", 0) == 0) d_cnt++;
        if (!t.held_addrs.empty()) h_cnt++;
    }

    /* hot locks sorted by stack_refs */
    std::vector<std::pair<int,ulong>> hot;
    for (auto &_p : lmap) {
        auto la = _p.first; auto &li = _p.second;
        if (li.stack_refs > 0) hot.push_back({li.stack_refs, la});
    }
    std::sort(hot.rbegin(), hot.rend());

    /* directed graph: A->B  =  A waits for a lock held by B */
    std::unordered_map<ulong, std::vector<ulong>> dg;
    std::vector<ulong> nodes;
    for (const auto &t : tasks) {
        nodes.push_back(t.task_addr);
        if (t.blocked_owner && tmap.count(t.blocked_owner))
            dg[t.task_addr].push_back(t.blocked_owner);
    }

    std::vector<std::vector<ulong>> sccs;
    find_deadlock_cycles(dg, nodes, sccs);

    std::unordered_set<ulong> in_deadlock;
    std::vector<const std::vector<ulong>*> deadlocks;
    for (const auto &s : sccs)
        if (s.size() > 1) { deadlocks.push_back(&s); for (ulong t : s) in_deadlock.insert(t); }

    /* ── Header ── */
    PRINT("Lock Analysis:");
    PRINT("  [system-wide]");
    PRINT("  tasks=%zu  D=%d  holding=%d  hot=%zu",
          tasks.size(), d_cnt, h_cnt, hot.size());
    if (!deadlocks.empty()) PRINT("  %zu deadlock(s)", deadlocks.size());
    PRINT("\n\n");

    /* ── Contended Locks (hot locks with waiters, fully expanded) ── */
    {
        bool any_contended = false;
        for (auto &_h : hot)
            if (lmap.count(_h.second) && !lmap[_h.second].waiters.empty())
                { any_contended = true; break; }

        if (any_contended) {
            PRINT("Contended Locks:\n\n");
            int n = 0;
            for (auto &_h : hot) { auto refs = _h.first; auto la = _h.second;
                if (!lmap.count(la)) continue;
                auto &li = lmap[la];
                if (li.waiters.empty()) continue;
                if (++n > 8) break;

                PRINT("  [%s]  0x%lx  %-30s  refs=%d\n",
                      lts(li.type), la,
                      li.name.empty() ? "?" : li.name.c_str(), refs);

                /* holder + holder's own blocked_on */
                if (li.holder) {
                    PRINT("  HOLDER:  %s\n", format_task_line(li.holder).c_str());
                    const TaskScanInfo *hti = tmap.count(li.holder) ? tmap[li.holder] : nullptr;
                    if (hti && hti->blocked_lock) {
                        const char *btype = "lock";
                        std::string bsym  = to_symbol(hti->blocked_lock);
                        bool bhs = !bsym.empty() && bsym[0] != '[';
                        struct task_context *botc = hti->blocked_owner
                            ? task_to_context(hti->blocked_owner) : nullptr;
                        if (bhs)
                            PRINT("           blocked on: %s @ 0x%lx  (%s)",
                                  btype, hti->blocked_lock, bsym.c_str());
                        else
                            PRINT("           blocked on: %s @ 0x%lx",
                                  btype, hti->blocked_lock);
                        if (botc)
                            PRINT("  -> PID %d  %s%s\n",
                                  (int)botc->pid, botc->comm,
                                  li.is_external ? "  [EXTERNAL]" : "");
                        else
                            PRINT("  (owner unknown / HANDOFF)\n");
                    }
                }

                /* waiter list */
                PRINT("  WAITERS (%zu):\n", li.waiters.size());
                for (size_t i = 0; i < li.waiters.size(); i++) {
                    ulong wt = li.waiters[i];
                    if (!is_kvaddr(wt)) {
                        PRINT("    #%zu  (invalid addr 0x%lx — corrupted node)\n", i, wt);
                        continue;
                    }
                    struct task_context *wtc = task_to_context(wt);
                    int prio = wtc ? task_prio(wt) : -1;
                    if (wtc)
                        PRINT("    #%zu  PID %-6d  %-22s  prio=%-3d  [%s]\n",
                              i, (int)wtc->pid, wtc->comm, prio,
                              task_state_display(wt).c_str());
                    else
                        PRINT("    #%zu  task=0x%lx  prio=%-3d  [%s]\n",
                              i, wt, prio, task_state_display(wt).c_str());
                }
                PRINT("\n");
            }
        } else if (!hot.empty()) {
            /* non-contended hot locks: show compact */
            PRINT("Hot Locks (no active waiters):\n");
            int n = 0;
            for (auto &_h : hot) { auto refs = _h.first; auto la = _h.second;
                if (!lmap.count(la)) continue;
                auto &li = lmap[la];
                if (++n > 5) break;
                PRINT("  [%s]  0x%lx  %-28s  refs=%d\n",
                      lts(li.type), la,
                      li.name.empty() ? "?" : li.name.c_str(), refs);
            }
            PRINT("\n");
        }
    }

    /* ── Deadlock components ── */
    int comp = 0;
    for (const auto *cyc : deadlocks) {
        std::unordered_set<ulong> clocks;
        for (ulong ta : *cyc) {
            const TaskScanInfo *ti = tmap.count(ta) ? tmap[ta] : nullptr;
            if (!ti) continue;
            for (ulong la : ti->held_addrs)  clocks.insert(la);
            if (ti->blocked_lock)            clocks.insert(ti->blocked_lock);
        }
        PRINT("── Component #%d  [DEADLOCK]  %zu tasks / %zu locks ",
              ++comp, cyc->size(), clocks.size());
        for (int i = 0; i < 16; i++) PRINT("─");
        PRINT("\n\n");

        for (ulong ta : *cyc) {
            const TaskScanInfo *ti = tmap.count(ta) ? tmap[ta] : nullptr;
            PRINT("  %s\n", format_task_line(ta).c_str());
            if (!ti) continue;
            for (size_t i = 0; i < ti->held_addrs.size(); i++) {
                ulong la = ti->held_addrs[i];
                const char *fn = ti->held_ips[i] ? closest_symbol(ti->held_ips[i]) : nullptr;
                PRINT("    holds  [%s]  0x%lx  %-28s",
                      lts(lmap.count(la) ? lmap[la].type : LockType::NONE), la,
                      ti->held_names[i].empty() ? "?" : ti->held_names[i].c_str());
                if (fn) PRINT("  @ %s", fn);
                PRINT("  (%zu waiters)\n",
                      lmap.count(la) ? lmap[la].waiters.size() : 0);
            }
            if (ti->blocked_lock) {
                auto &bli = lmap[ti->blocked_lock];
                struct task_context *otc = ti->blocked_owner
                    ? task_to_context(ti->blocked_owner) : nullptr;
                PRINT("    waits  [%s]  0x%lx  %-28s",
                      lts(bli.type), ti->blocked_lock,
                      bli.name.empty() ? "?" : bli.name.c_str());
                if (otc) {
                    PRINT("  -> PID %d  %s", (int)otc->pid, otc->comm);
                    if (in_deadlock.count(ti->blocked_owner)) PRINT("  <- CYCLE");
                } else { PRINT("  (owner unknown)"); }
                PRINT("\n");
            }
            PRINT("\n");
        }
    }

    /* ── Blocked chains (non-deadlock) ── */
    std::unordered_set<ulong> shown;
    for (ulong td : in_deadlock) shown.insert(td);

    /* precompute reverse map: blocker -> [tasks waiting on its locks] */
    std::unordered_map<ulong, std::vector<ulong>> blocked_by;
    for (const auto &t : tasks)
        if (t.blocked_owner && tmap.count(t.blocked_owner))
            blocked_by[t.blocked_owner].push_back(t.task_addr);

    /* Collect trivial IO-wait tasks (single task, no locks) for summary fold */
    std::vector<ulong> io_only;

    for (const auto &root : tasks) {
        if (shown.count(root.task_addr)) continue;
        /* root = task whose blocker is external/IO, or holds locks with waiters */
        bool is_root = (root.blocked_lock &&
                        (!root.blocked_owner || !tmap.count(root.blocked_owner)))
                    || (!root.blocked_lock &&
                        !root.held_addrs.empty() &&
                        blocked_by.count(root.task_addr));
        if (!is_root && !root.blocked_is_io) continue;

        /* Trivial IO-wait: single D-state task, no held locks, no blockers_of it */
        if (root.blocked_is_io && !root.blocked_lock &&
            root.held_addrs.empty() &&
            !blocked_by.count(root.task_addr)) {
            shown.insert(root.task_addr);
            io_only.push_back(root.task_addr);
            continue;
        }

        /* BFS: expand all tasks waiting on locks held by this root chain */
        std::vector<ulong> chain;
        std::vector<ulong> q = {root.task_addr};
        while (!q.empty()) {
            std::vector<ulong> next_q;
            for (ulong cur : q) {
                if (shown.count(cur)) continue;
                shown.insert(cur);
                chain.push_back(cur);
                const TaskScanInfo *ci = tmap.count(cur) ? tmap[cur] : nullptr;
                if (!ci) continue;
                for (ulong la : ci->held_addrs) {
                    if (!lmap.count(la)) continue;
                    for (ulong wt : lmap[la].waiters)
                        if (!shown.count(wt) && tmap.count(wt))
                            next_q.push_back(wt);
                }
                /* reverse direction: tasks blocked because cur holds their lock */
                auto bit = blocked_by.find(cur);
                if (bit != blocked_by.end())
                    for (ulong wt : bit->second)
                        if (!shown.count(wt)) next_q.push_back(wt);
            }
            q = std::move(next_q);
            if (chain.size() > 32) break;
        }
        if (chain.empty()) continue;

        const char *cause = root.blocked_is_io ? "IO wait"
                          : (root.blocked_owner && !tmap.count(root.blocked_owner)) ? "external dep"
                          : (!root.blocked_lock && !root.held_addrs.empty()) ? "lock holder"
                          : "blocked";
        std::unordered_set<ulong> clocks;
        for (ulong ta : chain) {
            const TaskScanInfo *ci = tmap.count(ta) ? tmap[ta] : nullptr;
            if (!ci) continue;
            for (ulong la : ci->held_addrs)  clocks.insert(la);
            if (ci->blocked_lock)            clocks.insert(ci->blocked_lock);
        }

        PRINT("── Component #%d  [%s]  %zu tasks / %zu locks ",
              ++comp, cause, chain.size(), clocks.size());
        for (int i = 0; i < 20; i++) PRINT("─");
        PRINT("\n\n");

        /* Tree display: start from root task, let print_lock_chain expand the chain */
        std::unordered_set<ulong> vis;
        print_lock_chain(root.task_addr, "  ", true, 0, true, vis);
        PRINT("\n");
    }

    /* ── Holding / no contention ── */
    bool hdr = false;
    for (const auto &t : tasks) {
        if (shown.count(t.task_addr) || t.held_addrs.empty()) continue;
        bool any_waiters = false;
        for (ulong la : t.held_addrs)
            if (lmap.count(la) && !lmap[la].waiters.empty()) { any_waiters = true; break; }
        if (any_waiters) continue;
        if (!hdr) {
            PRINT("── Holding / no contention ");
            for (int i = 0; i < 37; i++) PRINT("─");
            PRINT("\n\n");
            hdr = true;
        }
        shown.insert(t.task_addr);
        PRINT("  %s\n", format_task_line(t.task_addr).c_str());
        for (size_t i = 0; i < t.held_addrs.size(); i++) {
            ulong la = t.held_addrs[i];
            PRINT("    holds  [%s]  0x%lx  %-28s  (0 waiters)\n",
                  lts(lmap.count(la) ? lmap[la].type : LockType::NONE), la,
                  t.held_names[i].empty() ? "?" : t.held_names[i].c_str());
        }
        PRINT("\n");
    }

    /* ── Summary ── */
    int ext = 0;
    for (auto &_p : lmap) {
        auto &li = _p.second;
        if (li.is_external) ext++;
    }
    int blast = 0;
    for (const auto &t : tasks) {
        std::string st = task_state_display(t.task_addr);
        if ((st.rfind("UN", 0) == 0 || st.rfind("ID", 0) == 0) || !t.held_addrs.empty())
            blast++;
    }

    /* ── IO-wait background tasks (folded) ── */
    if (!io_only.empty()) {
        PRINT("── D-state / IO wait  (%zu tasks, no lock involvement) ",
              io_only.size());
        for (int i = 0; i < 16; i++) PRINT("─");
        PRINT("\n");
        int shown_n = 0;
        for (ulong ta : io_only) {
            if (shown_n++ >= 6) { PRINT("  ... (%zu more)\n", io_only.size() - 6); break; }
            struct task_context *tc = task_to_context(ta);
            if (tc) PRINT("  PID %-6d  %s\n", (int)tc->pid, tc->comm);
        }
        PRINT("\n");
    }

    PRINT("Summary:\n");
    PRINT("  deadlocks=%zu  components=%d  ext-deps=%d  blast-radius=%d\n",
          deadlocks.size(), comp, ext, blast);
    PRINT("\n");
}

void Lockdep::show_lock_analysis() {
    std::vector<TaskScanInfo> tasks;
    std::unordered_map<ulong, LockScanInfo> lmap;
    collect_lock_analysis(tasks, lmap);
    print_lock_analysis(tasks, lmap);
}

void Lockdep::init_offset(void) {}
#pragma GCC diagnostic pop
