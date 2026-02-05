/*
 * pmm.h - Physical Memory Manager
 *
 * Manages physical page frame allocation using a bitmap allocator.
 * Each bit represents one 4KB physical frame:
 *   - bit = 0: frame is free
 *   - bit = 1: frame is allocated
 *
 * The PMM must be initialized before any frame allocations. During
 * initialization, it parses the E820 memory map from the bootloader
 * and marks available regions as free while reserving:
 *   - The first 1MB (BIOS, VGA, ROM areas)
 *   - The kernel itself (code, data, BSS)
 */

#ifndef KERNEL_INCLUDE_PMM_H
#define KERNEL_INCLUDE_PMM_H

#include <types.h>

/*
 * Page Frame Constants
 */
#define PAGE_SIZE       4096            /* 4KB per page frame */
#define PAGE_SHIFT      12              /* log2(PAGE_SIZE) */
#ifndef PAGE_MASK
#define PAGE_MASK       (~(PAGE_SIZE - 1))
#endif

/*
 * Physical Memory Layout
 *
 * KERNEL_PHYS_START: Where the kernel is loaded (1MB mark)
 * KERNEL_PHYS_END:   End of kernel image (from linker symbol)
 */
#define KERNEL_PHYS_START   0x100000

/*
 * Linker-provided symbol marking end of kernel image (VIRTUAL address)
 *
 * After Story 3.2, _kernel_end is a virtual address (0xC0XXXXXX).
 * Use V2P() to convert to physical address for frame calculations.
 */
extern char _kernel_end;

/*
 * Get kernel physical end address
 *
 * This macro converts the virtual _kernel_end to physical.
 * Note: KERNEL_VIRT_BASE is 0xC0000000 (defined in vmm.h).
 */
#define KERNEL_VIRT_BASE_CONST  0xC0000000
#define KERNEL_PHYS_END     ((uint32_t)&_kernel_end - KERNEL_VIRT_BASE_CONST)

/*
 * Frame Number Conversion Macros
 *
 * PHYS_TO_FRAME: Convert physical address to frame number
 * FRAME_TO_PHYS: Convert frame number to physical address
 * PAGE_ALIGN_UP: Round up to next page boundary
 * PAGE_ALIGN_DOWN: Round down to page boundary
 */
#define PHYS_TO_FRAME(addr)     ((addr) >> PAGE_SHIFT)
#define FRAME_TO_PHYS(frame)    ((frame) << PAGE_SHIFT)
#define PAGE_ALIGN_UP(addr)     (((addr) + PAGE_SIZE - 1) & PAGE_MASK)
#define PAGE_ALIGN_DOWN(addr)   ((addr) & PAGE_MASK)

/*
 * pmm_init - Initialize the physical memory manager
 *
 * Parses the E820 memory map from the bootloader and initializes
 * the frame bitmap. After this call, pmm_alloc_frame() can be used.
 *
 * Initialization steps:
 *   1. Mark all frames as allocated initially
 *   2. Parse memory map, mark available regions as free
 *   3. Re-mark first 1MB as reserved (always)
 *   4. Re-mark kernel frames as allocated
 */
void pmm_init(void);

/*
 * pmm_alloc_frame - Allocate a physical page frame
 *
 * Scans the bitmap for the first free frame (starting after 1MB),
 * marks it as allocated, and returns its physical address.
 *
 * Returns: Physical address of allocated frame (4KB-aligned)
 *          0 if no frames are available (out of memory)
 */
uint32_t pmm_alloc_frame(void);

/*
 * pmm_free_frame - Free a physical page frame
 *
 * Marks the specified frame as free in the bitmap. The frame
 * can then be allocated again by pmm_alloc_frame().
 *
 * @phys_addr: Physical address of frame to free (must be 4KB-aligned)
 *
 * Warnings are logged for:
 *   - Unaligned addresses
 *   - Addresses outside valid range
 *   - Double-free attempts
 */
void pmm_free_frame(uint32_t phys_addr);

/*
 * pmm_get_free_count - Get number of free frames
 *
 * Returns: Number of frames available for allocation
 */
uint32_t pmm_get_free_count(void);

/*
 * pmm_get_total_count - Get total number of usable frames
 *
 * Returns: Total number of frames that can potentially be used
 *          (excludes reserved regions like first 1MB and kernel)
 */
uint32_t pmm_get_total_count(void);

#endif /* KERNEL_INCLUDE_PMM_H */
