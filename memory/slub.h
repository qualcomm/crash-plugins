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

#ifndef SLUB_DEFS_H_
#define SLUB_DEFS_H_

#include "plugin.h"
#include <unordered_set>

/**
 * @file slub.h
 * @brief SLUB (Simple List of Unused Blocks) allocator analysis plugin
 *
 * This plugin provides comprehensive analysis of the Linux kernel's SLUB allocator,
 * including memory corruption detection, allocation tracking, and performance analysis.
 */

/* ============================================================================
 * SLUB Debug Constants
 * ============================================================================ */

/** @brief Poison values used for memory corruption detection */
#define SLUB_RED_INACTIVE   0xbb    ///< Red zone pattern for freed objects
#define SLUB_RED_ACTIVE     0xcc    ///< Red zone pattern for allocated objects
#define POISON_INUSE        0x5a    ///< Pattern for allocated object data
#define POISON_FREE         0x6b    ///< Pattern for freed object data
#define POISON_END          0xa5    ///< Pattern for object end marker

/** @brief Track types for allocation/deallocation tracing */
#define TRACK_ALLOC         0       ///< Track allocation operations
#define TRACK_FREE          1       ///< Track deallocation operations

/* ============================================================================
 * Forward Declarations
 * ============================================================================ */

struct kmem_cache;
struct slab;
struct obj;

/* ============================================================================
 * Core Data Structures
 * ============================================================================ */

/**
 * @brief Stack trace information for allocation/deallocation tracking
 *
 * This structure stores the call stack and metadata for memory operations,
 * enabling detailed analysis of allocation patterns and memory leaks.
 * Corresponds to the kernel's track structure in slub.c (__kmem_obj_info).
 */
struct track {
    unsigned long track_addr;                   ///< Address of the track structure in kernel memory
    unsigned long obj_addr;                     ///< Address of the tracked object
    std::string frame;                          ///< Formatted call stack string
    int cpu;                                    ///< CPU ID where operation occurred
    int pid;                                    ///< Process ID that performed the operation
    unsigned long when;                         ///< Timestamp of the operation (in jiffies)
    std::shared_ptr<kmem_cache> kmem_cache_ptr; ///< Pointer to parent cache structure
    std::shared_ptr<obj> obj_ptr;
};

/**
 * @brief Individual SLUB object representation
 *
 * Represents a single allocated object within a slab, including its memory
 * layout, allocation status, and relationships to parent structures.
 *
 * Memory layout:
 * +--------------+------------+
 * | red left zone|  obj data  |
 * +--------------+------------+
 * ^              < -obj size- >
 * |
 * start
 */
struct obj {
    int index;                                  ///< Object index within the slab (1-based)
    ulong start;                                ///< Start address (including red zone if present)
    ulong end;                                  ///< End address of the object
    bool is_free;                               ///< True if object is currently free
    std::shared_ptr<slab> slab_ptr;             ///< Pointer to parent slab structure
    std::shared_ptr<kmem_cache> cache_ptr;      ///< Pointer to parent cache structure
    std::shared_ptr<track> track_ptr;
};

/**
 * @brief SLUB slab structure representation
 *
 * A slab is a contiguous block of memory containing multiple objects of the same size.
 * This structure tracks the slab's metadata and all objects within it.
 */
struct slab {
    ulong first_page;                           ///< Address of the first page in the slab
    unsigned int order;                         ///< Page order (slab size = 2^order * PAGE_SIZE)
    unsigned int inuse;                         ///< Number of allocated objects
    unsigned int totalobj;                      ///< Total number of objects in the slab
    unsigned int freeobj;                       ///< Number of free objects
    std::vector<std::shared_ptr<obj>> obj_list; ///< List of all objects in the slab
};

/**
 * @brief Per-CPU SLUB cache data
 *
 * Each CPU maintains its own set of slabs to minimize lock contention
 * and improve allocation performance in SMP systems.
 */
struct kmem_cache_cpu {
    ulong addr;                                 ///< Address of the kmem_cache_cpu structure
    ulong freeobj;                              ///< Address of the first free object
    unsigned long tid;                          ///< Transaction ID for lockless operations
    std::shared_ptr<slab> cur_slab;             ///< Currently active slab for allocations
    std::vector<std::shared_ptr<slab>> partial; ///< List of partially filled slabs
};

/**
 * @brief NUMA node-specific SLUB cache data
 *
 * In NUMA systems, each memory node maintains separate slab lists
 * to optimize memory locality and reduce cross-node memory access.
 */
struct kmem_cache_node {
    ulong addr;                                 ///< Address of the kmem_cache_node structure
    unsigned long nr_partial;                  ///< Number of partial slabs
    std::vector<std::shared_ptr<slab>> partial; ///< List of partial slabs
    ulong nr_slabs;                             ///< Total number of slabs
    ulong total_objects;                        ///< Total number of objects across all slabs
    std::vector<std::shared_ptr<slab>> full;    ///< List of fully allocated slabs
};

/**
 * @brief Main SLUB cache structure
 *
 * The central structure representing a SLUB cache, which manages objects
 * of a specific size and type. Contains all metadata, configuration,
 * and references to per-CPU and per-node data structures.
 */
struct kmem_cache {
    // Basic cache information
    ulong addr;                                 ///< Address of the kmem_cache structure
    std::string name;                           ///< Cache name (e.g., "kmalloc-64", "inode_cache")

    // Per-CPU and per-node data
    std::vector<std::shared_ptr<kmem_cache_cpu>> cpu_slabs;  ///< Per-CPU slab data
    std::vector<std::shared_ptr<kmem_cache_node>> node_list; ///< Per-NUMA-node data

    // Allocation tracking (requires SLAB_STORE_USER flag)
    std::vector<std::shared_ptr<track>> alloc_list;          ///< Allocation traces
    std::vector<std::shared_ptr<track>> free_list;           ///< Deallocation traces

    // Cache configuration and flags
    unsigned int flags;                         ///< SLAB flags (RED_ZONE, POISON, STORE_USER, etc.)
    unsigned long min_partial;                 ///< Minimum number of partial slabs to keep
    unsigned int size;                          ///< Total size per object (including metadata)
    unsigned int object_size;                   ///< Actual object size requested by user
    unsigned int offset;                        ///< Offset to free pointer within object
    unsigned int cpu_partial;                   ///< Number of partial slabs per CPU
    unsigned int oo;                            ///< Encoded object count and page order
    int per_slab_obj;                          ///< Number of objects per slab (decoded from oo)
    int page_order;                            ///< Page order for slab allocation (decoded from oo)

    // Size and alignment configuration
    unsigned int max;                           ///< Maximum objects per slab
    unsigned int min;                           ///< Minimum objects per slab
    unsigned int allocflags;                    ///< GFP allocation flags
    int refcount;                              ///< Reference count
    unsigned int inuse;                         ///< Size of the used portion of object
    unsigned int align;                         ///< Object alignment requirement
    unsigned int red_left_pad;                  ///< Left red zone padding size
    unsigned long random;                       ///< Random value for freelist pointer obfuscation

    // User-accessible region (for SLAB_USERCOPY)
    unsigned int useroffset;                    ///< Offset of user-accessible region
    unsigned int usersize;                      ///< Size of user-accessible region

    // Aggregate statistics (calculated during parsing)
    size_t total_size;                          ///< Total memory used by this cache
    size_t total_nr_slabs;                      ///< Total number of slabs across all nodes/CPUs
    size_t total_nr_objs;                       ///< Total number of objects across all slabs
};

/* ============================================================================
 * Memory Corruption Detection Structures
 * ============================================================================ */

/**
 * @brief Results of memory corruption check for a single cache
 *
 * Contains comprehensive information about corruption detection results,
 * including error counts by type and detailed error descriptions.
 */
struct SlubCheckResult {
    std::string cache_name;                     ///< Name of the checked cache
    ulong cache_addr;                           ///< Address of the cache structure
    int total_objects = 0;                      ///< Total objects in the cache
    int checked_objects = 0;                    ///< Number of objects actually checked
    int corrupted_objects = 0;                  ///< Number of corrupted objects found
    std::vector<std::string> errors;            ///< Detailed error descriptions
    bool overall_result = true;                 ///< True if no corruption detected

    // Error type counters
    int redzone_errors = 0;                     ///< Red zone violations
    int poison_errors = 0;                      ///< Poison pattern mismatches
    int freeptr_errors = 0;                     ///< Free pointer corruptions
    int padding_errors = 0;                     ///< Padding area violations
};

/**
 * @brief Detailed corruption check results for a single object
 *
 * Provides fine-grained information about different types of corruption
 * detected within a single SLUB object.
 */
struct ObjectCheckResult {
    int left_redzone_errors = 0;                ///< Left red zone violations
    int right_redzone_errors = 0;               ///< Right red zone violations
    int redzone_errors = 0;                     ///< Total red zone violations
    int poison_errors = 0;                      ///< Poison pattern violations
    int freeptr_errors = 0;                     ///< Free pointer corruptions
    int padding_errors = 0;                     ///< Padding area violations
    std::vector<std::string> details;           ///< Detailed error descriptions
};

/* ============================================================================
 * Memory Usage Statistics Structures
 * ============================================================================ */

/**
 * @brief Per-process allocation statistics
 *
 * Tracks memory allocation patterns for individual processes,
 * including total allocations and per-frame breakdowns.
 */
struct PidStatistics {
    size_t allocation_count = 0;                ///< Total number of allocations by this PID
    size_t total_memory = 0;                    ///< Total memory allocated by this PID
    std::unordered_map<size_t, size_t> frame_memory; ///< Frame ID -> memory allocated by this frame
};

/**
 * @brief Per-call-stack allocation statistics
 *
 * Tracks memory allocation patterns for specific call stacks,
 * enabling identification of memory-intensive code paths.
 */
struct FrameStatistics {
    size_t total_allocations = 0;               ///< Total allocations from this call stack
    size_t total_memory = 0;                    ///< Total memory allocated from this call stack
    std::unordered_map<int, PidStatistics> pid_stats; ///< Per-PID statistics within this frame
};

/**
 * @brief SLUB allocator analysis plugin
 *
 * This class provides comprehensive analysis capabilities for the Linux kernel's
 * SLUB allocator, including cache inspection, corruption detection, and
 * allocation pattern analysis.
 */
class Slub : public ParserPlugin {
private:
    /// Cache database using unordered_map for O(1) lookup by name
    std::unordered_map<std::string, std::shared_ptr<kmem_cache>> cache_list;


    /* ========================================================================
     * Configuration and Constants
     * ======================================================================== */

    size_t max_name_len = 0;                    ///< Maximum cache name length for formatting

    /// SLAB debug flags (initialized from kernel configuration)
    uint SLAB_RED_ZONE;                         ///< Red zone debugging flag
    uint SLAB_POISON;                           ///< Poison pattern debugging flag
    uint SLAB_STORE_USER;                       ///< Store allocation traces flag
    uint OBJECT_POISON;                         ///< Object-level poison flag
    uint SLAB_KMALLOC;                          ///< General-purpose allocation flag

    /// Default number of top entries to display in statistics
    static constexpr size_t DEFAULT_TOP_COUNT = 20;

    /* ========================================================================
     * Utility Functions
     * ======================================================================== */

    /**
     * @brief Safe function call wrapper with exception handling
     * @param input_str Input string to process
     * @param param_name Parameter name for error reporting
     * @param func Function to execute
     */
    void func_call(const std::string& input_str, const std::string& param_name,
                   std::function<void(const std::string&)> func);

    /* ========================================================================
     * Core Parsing Functions
     * ======================================================================== */

    /**
     * @brief Parse NUMA node structures for a cache
     * @param cache_ptr Parent cache structure
     * @param node_addr Address of the node array
     * @return Vector of parsed node structures
     */
    std::vector<std::shared_ptr<kmem_cache_node>> parser_kmem_cache_node(
        std::shared_ptr<kmem_cache> cache_ptr, ulong node_addr);

    /**
     * @brief Parse per-CPU structures for a cache
     * @param cache_ptr Parent cache structure
     * @param cpu_addr Address of the per-CPU structure
     * @return Vector of parsed CPU structures
     */
    std::vector<std::shared_ptr<kmem_cache_cpu>> parser_kmem_cache_cpu(
        std::shared_ptr<kmem_cache> cache_ptr, ulong cpu_addr);

    /**
     * @brief Parse slabs from a linked list
     * @param cache_ptr Parent cache structure
     * @param head_addr Address of the list head
     * @return Vector of parsed slab structures
     */
    std::vector<std::shared_ptr<slab>> parser_slab_from_list(
        std::shared_ptr<kmem_cache> cache_ptr, ulong head_addr);

    /**
     * @brief Parse a single slab structure
     * @param cache_ptr Parent cache structure
     * @param slab_page_addr Address of the slab/page structure
     * @return Parsed slab structure or nullptr on failure
     */
    std::shared_ptr<slab> parser_slab(std::shared_ptr<kmem_cache> cache_ptr, ulong slab_page_addr);

    /* ========================================================================
     * Cache Management Functions
     * ======================================================================== */

    /**
     * @brief Parse all SLUB caches from the kernel's slab_caches list
     */
    void parser_kmem_caches();

    std::shared_ptr<kmem_cache> parse_kmem_cache(std::string &name);

    /**
     * @brief Parse a single cache by address
     * @param cache_addr Address of the kmem_cache structure
     * @return Parsed cache structure or nullptr on failure
     */
    std::shared_ptr<kmem_cache> parse_kmem_cache(ulong cache_addr);

    /* ========================================================================
     * Display Functions
     * ======================================================================== */

    /**
     * @brief Print information for all caches
     */
    void print_kmem_caches();

    /**
     * @brief Print information for a specific cache by identifier
     * @param identifier Cache name or address
     */
    void print_kmem_cache(std::string& name);

    /**
     * @brief Print information for a specific cache
     * @param cache_ptr Cache structure to display
     */
    void print_kmem_cache(std::shared_ptr<kmem_cache> cache_ptr);

    /**
     * @brief Print summary information for all caches
     */
    void print_slab_summary_info();

    /**
     * @brief Print detailed information for a single slab
     * @param slab_ptr Slab structure to display
     */
    void print_slab_info(std::shared_ptr<slab> slab_ptr);

    /**
     * @brief Print object information with optional cache filtering
     * @param cache_filter Cache name or address filter (empty = all caches)
     */
    void print_slab_obj_info(const std::string& cache_filter = "");

    /**
     * @brief Print all objects information with detailed trace
     * @param cache_filter Cache name or address filter (empty = all caches)
     */
    void print_all_objects_info(const std::string& cache_filter = "");

    /**
     * @brief Print single object detail with trace information
     * @param obj_ptr Object to print details for
     */
    void print_single_object_detail(std::shared_ptr<obj> obj_ptr);

    /* ========================================================================
     * Memory Corruption Detection Functions
     * ======================================================================== */

    /**
     * @brief Check memory corruption for all or specific cache
     * @param kmem_cache_addr Cache address (0 = all caches)
     */
    void print_slub_poison(ulong kmem_cache_addr = 0);

    /**
     * @brief Check memory corruption with cache filtering
     * @param cache_filter Cache name or address filter (empty = all caches)
     */
    void print_slub_poison(const std::string& cache_filter = "");

    /**
     * @brief Check corruption for a specific cache
     * @param cache_ptr Cache to check
     * @param result Result structure to populate
     */
    void check_cache_corruption(std::shared_ptr<kmem_cache> cache_ptr, SlubCheckResult& result);

    /**
     * @brief Check corruption in a list of slabs
     * @param cache_ptr Parent cache
     * @param slab_list List of slabs to check
     * @param result Result structure to update
     */
    void check_slab_list_corruption(std::shared_ptr<kmem_cache> cache_ptr,
                                   const std::vector<std::shared_ptr<slab>>& slab_list,
                                   SlubCheckResult& result);

    /**
     * @brief Check corruption in a single slab
     * @param cache_ptr Parent cache
     * @param slab_ptr Slab to check
     * @param result Result structure to update
     */
    void check_single_slab_corruption(std::shared_ptr<kmem_cache> cache_ptr,
                                     std::shared_ptr<slab> slab_ptr,
                                     SlubCheckResult& result);

    /**
     * @brief Perform detailed corruption check on a single object
     * @param cache_ptr Parent cache
     * @param page_addr Page address
     * @param object_start_addr Object start address
     * @param val Expected poison value
     * @param obj_result Result structure to populate
     * @return True if object is clean, false if corrupted
     */
    bool check_object_detailed(std::shared_ptr<kmem_cache> cache_ptr, ulong page_addr,
                              ulong object_start_addr, uint8_t val, ObjectCheckResult& obj_result);

    /**
     * @brief Check and report byte pattern violations
     * @param cache_ptr Parent cache
     * @param page_addr Page address
     * @param obj_start Object start address
     * @param what Description of the region being checked
     * @param start Start address of region to check
     * @param value Expected byte value
     * @param bytes Number of bytes to check
     * @param obj_result Result structure to update
     * @return True if region is clean, false if corrupted
     */
    bool check_bytes_and_report_detailed(std::shared_ptr<kmem_cache> cache_ptr, ulong page_addr,
                                        ulong obj_start, std::string what, ulong start,
                                        uint8_t value, size_t bytes, ObjectCheckResult& obj_result);

    /**
     * @brief Check padding bytes for corruption
     * @param cache_ptr Parent cache
     * @param page_addr Page address
     * @param obj_start Object start address
     * @param obj_result Result structure to update
     * @return True if padding is clean, false if corrupted
     */
    bool check_pad_bytes_detailed(std::shared_ptr<kmem_cache> cache_ptr, ulong page_addr,
                                 ulong obj_start, ObjectCheckResult& obj_result);

    /**
     * @brief Print corruption check results for a cache
     * @param result Check results
     * @param show_all_errors Whether to show all errors or limit output
     */
    void print_cache_check_result(const SlubCheckResult& result, bool show_all_errors = false);

    /**
     * @brief Print summary of corruption check results
     * @param results Vector of check results
     */
    void print_corruption_summary(const std::vector<SlubCheckResult>& results);

    /**
     * @brief Generate memory layout visualization for a corrupted object
     * @param cache_ptr Parent cache
     * @param obj_ptr Object structure
     * @param obj_result Corruption check results
     * @return Formatted memory layout string
     */
    std::string print_object_layout(std::shared_ptr<kmem_cache> cache_ptr,
                                   std::shared_ptr<obj> obj_ptr,
                                   const ObjectCheckResult& obj_result);

    /* ========================================================================
     * Memory Layout and Poison Helper Functions
     * ======================================================================== */

    /**
     * @brief Get the end offset of object info area
     * @param cache_ptr Cache structure
     * @return End offset of info area
     */
    unsigned int get_info_end(std::shared_ptr<kmem_cache> cache_ptr);

    /**
     * @brief Check if free pointer is stored outside object data
     * @param cache_ptr Cache structure
     * @return True if free pointer is outside object
     */
    bool freeptr_outside_object(std::shared_ptr<kmem_cache> cache_ptr);

    /**
     * @brief Adjust object address to account for left red zone
     * @param cache_ptr Cache structure
     * @param object_start_addr Object start address
     * @return Adjusted address pointing to actual object data
     */
    ulong fixup_red_left(std::shared_ptr<kmem_cache> cache_ptr, ulong object_start_addr);

    /**
     * @brief Find first byte not matching the specified value
     * @param start_addr Start address to search
     * @param c Byte value to compare against
     * @param bytes Number of bytes to check
     * @return Address of first non-matching byte, or 0 if all match
     */
    ulong memchr_inv(ulong start_addr, uint8_t c, size_t bytes);

    /**
     * @brief Check bytes using 8-byte aligned access
     * @param start Start address
     * @param value Expected byte value
     * @param bytes Number of bytes to check
     * @return Address of first non-matching byte, or 0 if all match
     */
    ulong check_bytes8(ulong start, uint8_t value, size_t bytes);

    /**
     * @brief Check if cache supports original size debugging
     * @param cache_ptr Cache structure
     * @return True if original size debugging is enabled
     */
    bool slub_debug_orig_size(std::shared_ptr<kmem_cache> cache_ptr);

    /**
     * @brief Get original size for kmalloc caches
     * @param cache_ptr Cache structure
     * @return Original requested size
     */
    unsigned int get_orig_size(std::shared_ptr<kmem_cache> cache_ptr);

    /**
     * @brief Restore object address by removing left red zone offset
     * @param cache_ptr Cache structure
     * @param object_start Object start address
     * @return Address without red zone offset
     */
    ulong restore_red_left(std::shared_ptr<kmem_cache> cache_ptr, ulong object_start);

    /**
     * @brief Validate that a pointer points to a valid object
     * @param cache_ptr Cache structure
     * @param page_addr Page address
     * @param object_start Object address to validate
     * @return True if pointer is valid
     */
    bool check_valid_pointer(std::shared_ptr<kmem_cache> cache_ptr, ulong page_addr, ulong object_start);

    /**
     * @brief Calculate object size excluding red zone
     * @param cache_ptr Cache structure
     * @return Object size without red zone
     */
    unsigned int size_from_object(std::shared_ptr<kmem_cache> cache_ptr);

    /**
     * @brief Extract free pointer from object (handling obfuscation)
     * @param cache_ptr Cache structure
     * @param object_start_addr Object start address
     * @return Free pointer value (deobfuscated if necessary)
     */
    ulong get_free_pointer(std::shared_ptr<kmem_cache> cache_ptr, ulong object_start_addr);

    /* ========================================================================
     * Stack Trace Analysis Functions
     * ======================================================================== */

    /**
     * @brief Extract and format call stack from kernel address
     * @param frames_addr Kernel address of instruction pointer
     * @return Formatted call stack entry
     */
    std::string extract_callstack(ulong frames_addr);

    /**
     * @brief Parse stack trace from track structure
     * @param track_addr Address of track structure
     * @param track_ptr Track structure to populate
     */
    void parser_obj_trace(ulong track_addr, std::shared_ptr<track>& track_ptr);

    /**
     * @brief Get address of track structure for an object
     * @param cache_ptr Cache structure
     * @param object_start_addr Object start address
     * @param track_type Track type (TRACK_ALLOC or TRACK_FREE)
     * @return Address of track structure
     */
    ulong get_obj_track_addr(std::shared_ptr<kmem_cache> cache_ptr,
                            ulong object_start_addr, uint8_t track_type);

    /* ========================================================================
     * Object Search Functions
     * ======================================================================== */

    /**
     * @brief Find object containing a specific virtual address
     * @param target_addr Virtual address to search for
     * @return Object structure or nullptr if not found
     */
    std::shared_ptr<obj> find_object(ulong target_addr);

    /**
     * @brief Find object within a specific slab
     * @param slab_ptr Slab to search
     * @param target_addr Virtual address to search for
     * @return Object structure or nullptr if not found
     */
    std::shared_ptr<obj> find_object_in_slab(std::shared_ptr<slab> slab_ptr, ulong target_addr);

    /**
     * @brief Print detailed object information and stack trace
     * @param addr_str Address string to analyze
     */
    void print_object_trace(std::string addr_str);

    /**
     * @brief Print stack trace for a specific object
     * @param obj_ptr Object to analyze
     */
    void print_object_stack_trace(std::shared_ptr<obj> obj_ptr);

    /* ========================================================================
     * Statistics Collection and Analysis Functions
     * ======================================================================== */

    void print_frame_statistics_info(const std::vector<std::pair<std::string, FrameStatistics>> &sorted_frames, size_t total_allocations, size_t total_memory);

    /**
     * @brief Print allocation statistics by call stack frame
     */
    void print_alloc_mem_by_stack();

    /**
     * @brief Print top allocation traces with optional cache filtering
     * @param cache_filter Cache name or address filter (empty = all caches)
     */
    void print_top_alloc_trace(const std::string& cache_filter = "");

    /**
     * @brief Print top free traces with optional cache filtering
     * @param cache_filter Cache name or address filter (empty = all caches)
     */
    void print_top_free_trace(const std::string& cache_filter = "");

    /**
     * @brief Print free statistics by call stack frame
     */
    void print_free_mem_by_stack();

    /**
     * @brief Print free statistics for a specific cache
     * @param cache_name Cache name or address to analyze
     */
    void print_cache_free_mem_by_stack(const std::string& cache_name);

    /**
     * @brief Print detailed statistics for a specific frame
     * @param frame_id Hash ID of the frame to analyze
     */
    void print_frame_details(size_t frame_id);

    /**
     * @brief Print allocation statistics for a specific cache
     * @param cache_name Cache name or address to analyze
     */
    void print_cache_alloc_mem_by_stack(const std::string& cache_name);

    /**
     * @brief Print global allocation statistics by process ID
     */
    void print_alloc_mem_by_pid();

    /**
     * @brief Print detailed statistics for a specific process
     * @param pid Process ID to analyze
     */
    void print_pid_details(int pid);

    /**
     * @brief Print detailed stack information for a specific frame
     * @param frame_id Hash ID of the frame to display
     */
    void print_frame_stack_info(size_t frame_id);


public:
    /* ========================================================================
     * Public Interface
     * ======================================================================== */

    /**
     * @brief Constructor
     */
    Slub();

    /**
     * @brief Main command entry point
     */
    void cmd_main(void) override;

    /**
     * @brief Initialize structure field offsets
     */
    void init_offset(void) override;

    /**
     * @brief Initialize command help information
     */
    void init_command(void) override;

    DEFINE_PLUGIN_INSTANCE(Slub)
};

#endif // SLUB_DEFS_H_
