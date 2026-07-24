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

#include "swapinfo.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

/**
 * @brief Main command handler (empty in base class)
 *
 * This is overridden by derived classes to implement command functionality.
 * Base class provides no implementation as it's meant to be extended.
 */
void Swapinfo::cmd_main(void) {

}

/**
 * @brief Constructor with ZRAM information pointer
 *
 * Initializes the Swapinfo parser with an existing ZRAM information object.
 * This allows integration with ZRAM devices for reading compressed swap pages.
 *
 * @param zram Shared pointer to Zraminfo object for ZRAM device access
 */
Swapinfo::Swapinfo(std::shared_ptr<Zraminfo> zram) : zram_ptr(zram) {
    init_offset();
}

/**
 * @brief Default constructor
 *
 * Initializes the Swapinfo parser with a new ZRAM information object.
 * Creates a default Zraminfo instance for ZRAM device management.
 */
Swapinfo::Swapinfo() {
    zram_ptr = std::make_shared<Zraminfo>();
    init_offset();
}

/**
 * @brief Check if ZRAM is enabled in the kernel
 *
 * Delegates to the ZRAM information handler to verify if ZRAM support
 * is available in the kernel dump.
 *
 * @return true if ZRAM is enabled and accessible, false otherwise
 */
bool Swapinfo::is_zram_enable() {
    bool enabled = zram_ptr->is_zram_enable();
    return enabled;
}

/**
 * @brief Initialize structure field offsets
 *
 * Initializes all field offsets for swap-related kernel structures.
 * This must be called before any parsing operations to ensure correct
 * memory access to kernel structures.
 *
 */
void Swapinfo::init_offset(void) {
    // Initialize swap_info_struct fields
    field_init(swap_info_struct, pages);
    field_init(swap_info_struct, inuse_pages);
    field_init(swap_info_struct, swap_file);
    field_init(swap_info_struct, swap_vfsmnt);
    field_init(swap_info_struct, old_block_size);
    field_init(swap_info_struct, bdev);
    field_init(swap_info_struct, swap_extent_root);
    struct_init(swap_info_struct);

    // Initialize block_device fields
    field_init(block_device, bd_disk);
    field_init(block_device, bd_start_sect);
    field_init(block_device, bd_part);

    // Initialize gendisk fields
    field_init(gendisk, private_data);

    // Initialize swap_extent fields
    field_init(swap_extent, rb_node);

    // Initialize hd_struct fields (for older kernels)
    field_init(hd_struct, start_sect);

    // Initialize address_space fields
    field_init(address_space, i_pages);
    field_init(address_space, page_tree);
    struct_init(address_space);
}

/**
 * @brief Initialize command information (empty in base class)
 *
 * This is overridden by derived classes to set up command metadata.
 * Base class provides no implementation as it's meant to be extended.
 */
void Swapinfo::init_command(void) {

}

/**
 * @brief Destructor
 *
 * Cleans up Swapinfo resources. The ZRAM pointer is automatically
 * managed by shared_ptr.
 */
Swapinfo::~Swapinfo() {

}

/**
 * @brief Convert PTE value to ZRAM page index
 *
 * This function performs the complex task of converting a page table entry
 * to a ZRAM page index by:
 * 1. Extracting the swap offset from the PTE
 * 2. Finding the corresponding swap extent in the red-black tree
 * 3. Calculating the disk block number
 * 4. Adding the partition start sector
 * 5. Computing the final ZRAM page index
 *
 * @param swap_ptr Pointer to swap_info structure
 * @param pte_val Page table entry value containing swap information
 * @return ZRAM page index for accessing the compressed page
 */
ulonglong Swapinfo::pte_handle_index(std::shared_ptr<swap_info> swap_ptr, ulonglong pte_val) {
    LOGD("Converting PTE 0x%llx to ZRAM index\n", pte_val);
    // Extract swap offset from PTE
    ulong swp_offset = 0;
    if (THIS_KERNEL_VERSION >= LINUX(2, 6, 0)) {
        swp_offset = (ulong)__swp_offset(pte_val);
    } else {
        swp_offset = (ulong)SWP_OFFSET(pte_val);
    }
    // Read swap extent root from swap_info_struct
    ulong swap_extent_root = read_pointer(swap_ptr->addr +
                                         field_offset(swap_info_struct, swap_extent_root),
                                         "rb_root");
    // Traverse red-black tree to find matching swap extent
    std::vector<ulong> swap_extent_list = for_each_rbtree(swap_extent_root, field_offset(swap_extent, rb_node));
    struct swap_extent extent;
    BZERO(&extent, sizeof(struct swap_extent));

    // Find the extent containing our swap offset
    for (const auto& addr : swap_extent_list) {
        BZERO(&extent, sizeof(struct swap_extent));
        if (!read_struct(addr, &extent, sizeof(extent), "swap_extent")) {
            continue;
        }
        // Check if offset falls within this extent's range
        if (extent.start_page <= swp_offset &&
            swp_offset < (extent.start_page + extent.nr_pages)) {
            LOGD("Found matching swap_extent at 0x%lx: start_block=%lld, start_page=%lu, nr_pages=%lu\n",
                 addr, extent.start_block, extent.start_page, extent.nr_pages);
            break;
        }
    }

    // Get partition start sector (kernel version dependent)
    ulonglong start_sect = 0;
    if (THIS_KERNEL_VERSION >= LINUX(5, 10, 0)) {
        start_sect = read_ulonglong(swap_ptr->bdev + field_offset(block_device, bd_start_sect),"block_device bd_start_sect");
    } else {
        ulong bd_part = read_ulong(swap_ptr->bdev + field_offset(block_device, bd_part),"block_device bd_part");
        start_sect = read_ulong(bd_part + field_offset(hd_struct, start_sect),"hd_struct start_sect");
    }

    // Calculate final ZRAM page index
    ulonglong index = start_sect + (extent.start_block + (swp_offset - extent.start_page));

    LOGD("Calculated ZRAM index: %lld\n", index);
    return index;
}

/**
 * @brief Look up page in swap cache
 *
 * Searches the swap cache (swapper_spaces address_space) for a page
 * corresponding to the given PTE value. The swap cache stores recently
 * accessed swap pages in memory to avoid repeated disk/ZRAM reads.
 *
 * Process:
 * 1. Extract swap type and offset from PTE
 * 2. Locate the appropriate address_space in swapper_spaces array
 * 3. Search the radix tree or xarray for the page
 * 4. Validate the page is not an exceptional entry
 *
 * @param pte_val Page table entry value
 * @return Page structure address if found in cache, 0 otherwise
 */
ulong Swapinfo::lookup_swap_cache(ulonglong pte_val) {
    LOGD("Looking up swap cache for PTE 0x%llx\n", pte_val);
    struct list_pair lp;
    bool is_xarray = false;

    // Extract swap type and offset
    ulong swp_type = SWP_TYPE(pte_val);
    ulonglong swp_offset = (ulonglong)__swp_offset(pte_val);
    LOGD("swp_type: %lu, swp_offset: %lld\n", swp_type, swp_offset);
    // Check if swapper_spaces exists
    if (!csymbol_exists("swapper_spaces")) {
        LOGE("swapper_spaces doesn't exist in this kernel!\n");
        return 0;
    }

    // Calculate address_space location
    ulong swp_space = csymbol_value("swapper_spaces");
    swp_space += swp_type * sizeof(void *);
    swp_space = read_pointer(swp_space, "address_space addr");
    swp_space += (swp_offset >> SWAP_ADDRESS_SPACE_SHIFT) * struct_size(address_space);
    LOGD("swap address_space: 0x%lx\n", swp_space);
    if (!is_kvaddr(swp_space)) {
        LOGE("address_space address is invalid!\n");
        return 0;
    }
    // Determine if using xarray or radix tree
    std::string i_pages_type = MEMBER_TYPE_NAME(TO_CONST_STRING("address_space"),
                                                TO_CONST_STRING("i_pages"));
    is_xarray = (i_pages_type == "xarray");
    LOGD("Using %s for page cache lookup\n",is_xarray ? "xarray" : "radix tree");
    // Get i_pages offset (try i_pages first, fall back to page_tree)
    int i_pages_offset = field_offset(address_space, i_pages);
    if (i_pages_offset == -1) {
        i_pages_offset = field_offset(address_space, page_tree);
    }
    // Search for the page
    ulong page = 0;
    lp.index = swp_offset;
    if (is_xarray) {
        if (do_xarray(swp_space + i_pages_offset, XARRAY_SEARCH, &lp)) {
            if ((ulong)lp.value & 1) {
                page = 0;
            }else{
                page = (ulong)lp.value;
            }
        }
    } else {
        if (do_radix_tree(swp_space + i_pages_offset, RADIX_TREE_SEARCH, &lp)) {
            if ((ulong)lp.value & RADIX_TREE_EXCEPTIONAL_ENTRY) {
                page = 0;
            }else{
                page = (ulong)lp.value;
            }
        }
    }
    if (page) {
        LOGD("Found page in swap cache: 0x%lx\n", page);
    } else {
        LOGD("Page not found in swap cache\n");
    }
    return page;
}

/**
 * @brief Read C-string from user space
 *
 * Reads a null-terminated string from user space memory.
 *
 * @param task_addr Task structure address
 * @param uvaddr User virtual address
 * @param len Maximum length to read
 * @param note Description for debugging
 * @return String read from user space
 */
std::string Swapinfo::uread_cstring(ulonglong task_addr, ulonglong uvaddr, int len,
                                   const std::string& note) {
    std::string res;
    std::vector<char> buf = uread_memory(task_addr, uvaddr, len, note);
    if (buf.size() > 0) {
        res.assign(buf.begin(), buf.end());
    }
    return res;
}

/**
 * @brief Read boolean value from user space
 * @param task_addr Task structure address
 * @param uvaddr User virtual address
 * @param note Description for debugging
 * @return Boolean value read from user space
 */
bool Swapinfo::uread_bool(ulonglong task_addr, ulonglong uvaddr, const std::string& note) {
    std::vector<char> buf = uread_memory(task_addr, uvaddr, sizeof(bool), note);
    if (buf.size() == 0) {
        return false;
    }
    bool res = BOOL(buf.data());
    return res;
}

/**
 * @brief Read integer value from user space
 * @param task_addr Task structure address
 * @param uvaddr User virtual address
 * @param note Description for debugging
 * @return Integer value read from user space
 */
int Swapinfo::uread_int(ulonglong task_addr, ulonglong uvaddr, const std::string& note) {
    std::vector<char> buf = uread_memory(task_addr, uvaddr, sizeof(int), note);
    if (buf.size() == 0) {
        return 0;
    }
    int res = INT(buf.data());
    return res;
}

/**
 * @brief Read unsigned integer value from user space
 * @param task_addr Task structure address
 * @param uvaddr User virtual address
 * @param note Description for debugging
 * @return Unsigned integer value read from user space
 */
uint Swapinfo::uread_uint(ulonglong task_addr, ulonglong uvaddr, const std::string& note) {
    std::vector<char> buf = uread_memory(task_addr, uvaddr, sizeof(uint), note);
    if (buf.size() == 0) {
        return 0;
    }
    uint res = UINT(buf.data());
    return res;
}

/**
 * @brief Read long value from user space
 * @param task_addr Task structure address
 * @param uvaddr User virtual address
 * @param note Description for debugging
 * @return Long value read from user space
 */
long Swapinfo::uread_long(ulonglong task_addr, ulonglong uvaddr, const std::string& note) {
    std::vector<char> buf = uread_memory(task_addr, uvaddr, sizeof(long), note);
    if (buf.size() == 0) {
        return 0;
    }
    long res = LONG(buf.data());
    return res;
}

/**
 * @brief Read unsigned long value from user space
 * @param task_addr Task structure address
 * @param uvaddr User virtual address
 * @param note Description for debugging
 * @return Unsigned long value read from user space
 */
ulong Swapinfo::uread_ulong(ulonglong task_addr, ulonglong uvaddr, const std::string& note) {
    std::vector<char> buf = uread_memory(task_addr, uvaddr, sizeof(ulong), note);
    if (buf.size() == 0) {
        return 0;
    }
    ulong res = ULONG(buf.data());
    return res;
}

/**
 * @brief Read unsigned long long value from user space
 * @param task_addr Task structure address
 * @param uvaddr User virtual address
 * @param note Description for debugging
 * @return Unsigned long long value read from user space
 */
ulonglong Swapinfo::uread_ulonglong(ulonglong task_addr, ulonglong uvaddr, const std::string& note) {
    std::vector<char> buf = uread_memory(task_addr, uvaddr, sizeof(ulonglong), note);
    if (buf.size() == 0) {
        return 0;
    }
    ulonglong res = ULONGLONG(buf.data());
    return res;
}

/**
 * @brief Read unsigned short value from user space
 * @param task_addr Task structure address
 * @param uvaddr User virtual address
 * @param note Description for debugging
 * @return Unsigned short value read from user space
 */
ushort Swapinfo::uread_ushort(ulonglong task_addr, ulonglong uvaddr, const std::string& note) {
    std::vector<char> buf = uread_memory(task_addr, uvaddr, sizeof(ushort), note);
    if (buf.size() == 0) {
        return 0;
    }
    ushort res = USHORT(buf.data());
    return res;
}

/**
 * @brief Read short value from user space
 * @param task_addr Task structure address
 * @param uvaddr User virtual address
 * @param note Description for debugging
 * @return Short value read from user space
 */
short Swapinfo::uread_short(ulonglong task_addr, ulonglong uvaddr, const std::string& note) {
    std::vector<char> buf = uread_memory(task_addr, uvaddr, sizeof(short), note);
    if (buf.size() == 0) {
        return 0;
    }
    short res = SHORT(buf.data());
    return res;
}

/**
 * @brief Read pointer value from user space
 * @param task_addr Task structure address
 * @param uvaddr User virtual address
 * @param note Description for debugging
 * @return Pointer value read from user space
 */
ulong Swapinfo::uread_pointer(ulonglong task_addr, ulonglong uvaddr, const std::string& note) {
    std::vector<char> buf = uread_memory(task_addr, uvaddr, sizeof(void *), note);
    if (buf.size() == 0) {
        return 0;
    }
    ulong res = (ulong)VOID_PTR(buf.data());
    return res;
}

/**
 * @brief Read byte value from user space
 * @param task_addr Task structure address
 * @param uvaddr User virtual address
 * @param note Description for debugging
 * @return Byte value read from user space
 */
unsigned char Swapinfo::uread_byte(ulonglong task_addr, ulonglong uvaddr, const std::string& note) {
    std::vector<char> buf = uread_memory(task_addr, uvaddr, 1, note);
    if (buf.size() == 0) {
        return 0;
    }
    unsigned char res = UCHAR(buf.data());
    return res;
}

/**
 * @brief Read memory buffer from user space
 *
 * This is the core function for reading arbitrary data from user space.
 * It handles the complex task of reading data that may span multiple pages,
 * where each page might be:
 * - In physical memory
 * - In swap cache
 * - Swapped to ZRAM
 * - Swapped to disk
 *
 * The function reads one page at a time, handling page boundaries correctly.
 *
 * Memory layout example:
 * -----------------------------------------------------------------
 * |         Page 1          |         Page 2          |  Page 3  |
 * -----------------------------------------------------------------
 *                  ^                              ^
 *                  |<-------- read length ------->|
 *                start                           end
 *
 * @param task_addr Task structure address
 * @param uvaddr User virtual address to start reading from
 * @param len Number of bytes to read
 * @param note Description for debugging
 * @return Vector containing read data
 */
std::vector<char> Swapinfo::uread_memory(ulonglong task_addr, ulonglong uvaddr, int len,
                                        const std::string& note) {
    LOGD("uread_memory: task=0x%llx, vaddr=0x%llx, len=%d\n",task_addr, uvaddr, len);
    int remain = len;
    std::vector<char> result(len);
    // Read data page by page
    while (remain > 0) {
        // Read one page
        char* buf_page = do_swap_page(task_addr, uvaddr);
        int offset_in_page = (uvaddr & ~page_mask);
        int read_len = std::min(remain, static_cast<int>(page_size) - offset_in_page);
        if (buf_page != nullptr) {
            memcpy(result.data() + (len - remain), buf_page + offset_in_page, read_len);
            FREEBUF(buf_page);
        } else {
            LOGD("Failed to read page at 0x%llx\n", uvaddr);
        }
        remain -= read_len;
        uvaddr += read_len;
    }
    return result;
}

/**
 * @brief Check if PTE represents a swap entry
 *
 * Determines if a page table entry indicates a swapped-out page by
 * checking architecture-specific present bits. A swap PTE has:
 * - Non-zero value (valid entry)
 * - Present bit clear (not in physical memory)
 * - May have PROT_NONE bit set (ARM64)
 *
 * @param pte Page table entry value
 * @return true if PTE is a swap entry, false otherwise
 */
bool Swapinfo::is_swap_pte(ulong pte) {
    int present = 0;

#if defined(ARM64)
    present = pte & (PTE_VALID | machdep->machspec->PTE_PROT_NONE);
    LOGD("ARM64 PTE check: pte=0x%lx, present: %s\n", pte, (present ? "true":"false"));
#endif

#if defined(ARM)
    #define L_PTE_PRESENT (1 << 0)
    present = pte & L_PTE_PRESENT;
    LOGD("ARM PTE check: pte=0x%lx, present=%d\n", pte, (present ? "true":"false"));
#endif
    bool is_swap = (pte && !present);
    LOGD("is_swap_pte(0x%lx)=%s\n", pte, is_swap ? "true" : "false");
    return is_swap;
}

/**
 * @brief Handle swap page fault and retrieve page data
 *
 * This is the main function for resolving a virtual address to its page data.
 * It handles multiple scenarios:
 *
 * 1. Page in physical memory: Read directly from physical address
 * 2. Page in swap cache: Read from cached page structure
 * 3. Page in ZRAM: Decompress from ZRAM device
 * 4. Page on disk: Would need disk I/O (not fully implemented)
 *
 * Process:
 * 1. Validate task context and address
 * 2. Try to translate virtual to physical address
 * 3. If translation fails, check if page is swapped
 * 4. Look up in swap cache
 * 5. If not in cache, read from ZRAM or disk
 *
 * @param task_addr Task structure address
 * @param uvaddr User virtual address
 * @return Pointer to page data buffer, or nullptr on failure
 */
char* Swapinfo::do_swap_page(ulonglong task_addr, ulonglong uvaddr) {
    LOGD("do_swap_page: task=0x%llx, vaddr=0x%llx\n", task_addr, uvaddr);
    // Ensure swap information is loaded
    nr_swap = read_int(csymbol_value("nr_swapfiles"), "nr_swapfiles");
    if (swap_list.size() == 0 && nr_swap > 0) {
        parser_swap_info();
    }
    physaddr_t paddr = 0;
    // Get task context
    struct task_context *tc = task_to_context(task_addr);
    if (!tc) {
        LOGE("Not found task 0x%llx in dump\n", task_addr);
        return nullptr;
    }
    ulonglong page_start = uvaddr & page_mask;
    LOGD("Task: pid=%lu, comm=%s, Page start: 0x%llx\n", tc->pid, tc->comm, page_start);

    // Validate address is in user space
    if (!IS_UVADDR(page_start, tc)) {
        LOGE("Address 0x%llx not in user space\n", page_start);
        return nullptr;
    }
    // Try to translate virtual to physical address
    int page_exist = uvtop(tc, page_start, &paddr, 0);
    LOGD("page 0x%llx %s\n", page_start, page_exist ? "exist":"not exist");
    if (page_exist) {
        // Page is in physical memory
        LOGD("read 0x%llx from page_vaddr:0x%llx, page_paddr:0x%llx\n\n",
             uvaddr, page_start, (ulonglong)paddr);
        char* buf = (char*)read_memory(paddr, page_size, "do_swap_page", false);
        if (buf == nullptr) {
            LOGE("read 0x%llx from memory failed\n", uvaddr);
            return nullptr;
        }
        return buf;
    } else {
        // Page is not in physical memory - check if swapped
        ulong pte = paddr;
#if defined(ARM)
        pte = get_arm_pte(tc->task, page_start);
#endif
        LOGD("Pid:%ld vaddr:0x%llx PTE:0x%lx\n", tc->pid, page_start, pte);
        if (is_swap_pte(pte)) {
            // Page is swapped out
            ulong swp_type = SWP_TYPE(pte);
            ulonglong swp_offset = (ulonglong)__swp_offset(pte);
            LOGD("PTE:0x%lx, type:%ld, offset:%lld\n", pte, swp_type, swp_offset);

            // Try to find page in swap cache
            ulong swap_page = lookup_swap_cache(pte);
            if (is_kvaddr(swap_page)) {
                // Page found in swap cache
                ulong page_paddr = page_to_phy(swap_page);
                LOGD("read 0x%llx from swapcache page_vaddr:0x%lx, page_paddr:0x%lx\n\n",
                     uvaddr, swap_page, page_paddr);
                char* buf = (char*)read_memory(page_paddr, page_size, "do_swap_page", false);
                if (buf == nullptr) {
                    LOGE("read swap page 0x%llx from memory failed\n", uvaddr);
                    return nullptr;
                }
                return buf;
            }
            // Page not in cache - read from ZRAM
            std::shared_ptr<swap_info> swap_ptr = get_swap_info(pte);
            if (swap_ptr == nullptr) {
                LOGE("can't found swap_info!\n");
                return nullptr;
            }
            ulong zram_addr = get_zram_addr(swap_ptr);
            if (!is_kvaddr(zram_addr)) {
                LOGE("can't found zram addr: 0x%lx!\n", zram_addr);
                return nullptr;
            }
            ulonglong index = pte_handle_index(swap_ptr, pte);
            LOGD("read 0x%llx from zram:0x%lx, index:%lld\n", page_start, zram_addr, index);
            return zram_ptr->read_zram_page(zram_addr, index);
        }
        LOGD("invalid PTE:0x%lx vaddr:0x%llx\n", pte, page_start);
        return nullptr;
    }
}

/**
 * @brief Get swap_info structure for a given PTE value
 *
 * Extracts the swap type from the PTE and locates the corresponding
 * swap_info_struct in the kernel's swap_info array.
 *
 * Handles two kernel versions:
 * - SWAPINFO_V2: Array of pointers to swap_info_struct
 * - Older: Array of swap_info_struct directly
 *
 * @param pte_val Page table entry value
 * @return Shared pointer to swap_info structure, or nullptr if not found
 */
std::shared_ptr<swap_info> Swapinfo::get_swap_info(ulonglong pte_val) {
    LOGD("Getting swap_info for PTE 0x%llx\n", pte_val);
    if (!csymbol_exists("swap_info")) {
        LOGE("swap_info doesn't exist in this kernel!\n");
        return nullptr;
    }
    swap_info_init();
    ulong swap_addr = csymbol_value("swap_info");
    ulong swp_type = SWP_TYPE(pte_val);
    // Calculate swap_info_struct address based on kernel version
    if (vt->flags & SWAPINFO_V2) {
        swap_addr += (swp_type * sizeof(void *));
        swap_addr = read_pointer(swap_addr, "swap_info_struct addr");
    } else {
        swap_addr += (struct_size(swap_info_struct) * swp_type);
    }
    LOGD("swap_info_struct at 0x%lx\n", swap_addr);
    if (!is_kvaddr(swap_addr)) {
        LOGE("swap_info address is invalid!\n");
        return nullptr;
    }
    // Search in cached swap list
    for (const auto& swap_ptr : swap_list) {
        if (swap_ptr->addr == swap_addr) {
            LOGD("Found swap_info\n");
            return swap_ptr;
        }
    }
    LOGD("Not found swap_info\n");
    return nullptr;
}

/**
 * @brief Get ZRAM device address from swap_info
 *
 * Retrieves the ZRAM device address by following the chain:
 * swap_info -> block_device -> gendisk -> private_data (ZRAM device)
 *
 * Only works for ZRAM-backed swap devices (file path contains "zram").
 *
 * @param swap_ptr Pointer to swap_info structure
 * @return ZRAM device address, or 0 if not a ZRAM device
 */
ulong Swapinfo::get_zram_addr(std::shared_ptr<swap_info> swap_ptr) {
    LOGD("Getting ZRAM address for swap device: %s\n",swap_ptr->swap_file.c_str());
    // Check if this is a ZRAM device
    if (swap_ptr->swap_file.rfind("zram") == std::string::npos) {
        LOGE("Not a ZRAM device\n");
        return 0;
    }
    if (!is_kvaddr(swap_ptr->bdev)) {
        LOGE("Invalid block device address\n");
        return 0;
    }
    // Follow: bdev -> bd_disk -> private_data
    ulong bd_disk = read_pointer(swap_ptr->bdev + field_offset(block_device, bd_disk),
                                 "block_device bd_disk");
    if (!is_kvaddr(bd_disk)) {
        LOGE("Invalid gendisk address\n");
        return 0;
    }
    ulong data = read_pointer(bd_disk + field_offset(gendisk, private_data),
                             "gendisk private_data");

    LOGD("ZRAM device address: 0x%lx\n", data);
    return data;
}

/**
 * @brief Parse all swap device information
 *
 * Reads swap device information from kernel structures and populates
 * the swap_list with details about each swap device.
 *
 * For each swap device, extracts:
 * - Total and used page counts
 * - File path
 * - Block device pointer
 * - Address space pointer
 *
 * Handles different kernel versions for file path extraction.
 */
void Swapinfo::parser_swap_info() {
    if (!csymbol_exists("nr_swapfiles")) {
        LOGE("nr_swapfiles doesn't exist in this kernel!\n");
        return;
    }
    if (!csymbol_exists("swap_info")) {
        LOGE("swap_info doesn't exist in this kernel!\n");
        return;
    }
    char buf[BUFSIZE];
    swap_info_init();

    ulong swap_info_addr = csymbol_value("swap_info");
    nr_swap = read_int(csymbol_value("nr_swapfiles"), "nr_swapfiles");
    ulong swp_space = csymbol_value("swapper_spaces");
    LOGD("Number of swap files: %d\n", nr_swap);
    LOGD("swap_info address: 0x%lx\n", swap_info_addr);
    LOGD("swapper_spaces address: 0x%lx\n", swp_space);
    // Parse each swap device
    for (int i = 0; i < nr_swap; i++) {
        LOGD("Parsing swap device#%d\n", i);
        ulong addr = read_pointer(swap_info_addr + i * sizeof(void *), "swap_info_struct addr");
        if (!is_kvaddr(addr)) {
            LOGE("Invalid swap_info_struct address, skipping\n");
            continue;
        }
        void *swap_info_buf = read_struct(addr, "swap_info_struct");
        if (swap_info_buf == nullptr) {
            LOGE("Failed to read swap_info_struct, skipping\n");
            continue;
        }
        std::shared_ptr<swap_info> swap_ptr = std::make_shared<swap_info>();
        swap_ptr->addr = addr;
        swap_ptr->swap_space = read_pointer(swp_space + i * sizeof(void *), "swapper_space addr");
        swap_ptr->pages = UINT(swap_info_buf + field_offset(swap_info_struct, pages));
        swap_ptr->inuse_pages = UINT(swap_info_buf + field_offset(swap_info_struct, inuse_pages));

        // Extract file path (kernel version dependent)
        ulong swap_file = ULONG(swap_info_buf + field_offset(swap_info_struct, swap_file));
        if (is_kvaddr(swap_file)) {
            if (field_offset(swap_info_struct, swap_vfsmnt) != -1) {
                ulong vfsmnt = ULONG(swap_info_buf + field_offset(swap_info_struct, swap_vfsmnt));
                get_pathname(swap_file, buf, BUFSIZE, 1, vfsmnt);
            } else if (field_offset(swap_info_struct, old_block_size) != -1) {
                get_pathname(file_to_dentry(swap_file), buf, BUFSIZE, 1, file_to_vfsmnt(swap_file));
            } else {
                get_pathname(file_to_dentry(swap_file), buf, BUFSIZE, 1, 0);
            }
        }
        swap_ptr->swap_file = buf;
        swap_ptr->bdev = ULONG(swap_info_buf + field_offset(swap_info_struct, bdev));

        FREEBUF(swap_info_buf);
        LOGD("Swap device#%d parsed:\n", i);
        LOGD("  addr=0x%lx\n", swap_ptr->addr);
        LOGD("  file=%s\n", swap_ptr->swap_file.c_str());
        LOGD("  pages=%u\n", swap_ptr->pages);
        LOGD("  inuse_pages=%u\n", swap_ptr->inuse_pages);
        LOGD("  bdev=0x%lx\n", swap_ptr->bdev);
        swap_list.push_back(swap_ptr);
    }
}

#pragma GCC diagnostic pop
