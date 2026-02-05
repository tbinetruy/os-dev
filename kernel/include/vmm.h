/*
 * vmm.h - Virtual Memory Manager
 *
 * Manages virtual address space using x86 paging. Features:
 *   - Higher-half kernel at 0xC0000000
 *   - Two-level page tables (Page Directory + Page Tables)
 *   - P2V/V2P macros for physical-virtual address conversion
 *
 * After vmm_init(), all kernel code runs at virtual addresses
 * >= KERNEL_BASE. Physical addresses must be converted using
 * P2V() before dereferencing.
 */

#ifndef KERNEL_INCLUDE_VMM_H
#define KERNEL_INCLUDE_VMM_H

#include <types.h>

/*
 * =============================================================================
 * Kernel Virtual Address Space
 * =============================================================================
 *
 * Memory layout after paging enabled:
 *
 *   0xFFFFFFFF ┌──────────────────┐
 *              │   Kernel Heap    │  (grows down, future)
 *              ├──────────────────┤
 *              │  Kernel Stack    │
 *              ├──────────────────┤
 *              │   Kernel BSS     │
 *              ├──────────────────┤
 *              │   Kernel Data    │
 *              ├──────────────────┤
 *              │   Kernel Code    │
 *   0xC0100000 ├──────────────────┤
 *              │   Reserved       │  (first 1MB virtual)
 *   0xC0000000 ├──────────────────┤  KERNEL_BASE
 *              │                  │
 *              │   User Space     │  (future - Epic 5+)
 *              │   (unmapped)     │
 *              │                  │
 *   0x00000000 └──────────────────┘
 */

/*
 * KERNEL_BASE - Virtual address where kernel is mapped
 *
 * The kernel occupies the upper 1GB of virtual address space.
 * User processes get the lower 3GB (0x00000000 - 0xBFFFFFFF).
 */
#define KERNEL_BASE         0xC0000000

/*
 * KERNEL_PAGE_DIR_IDX - Page directory index for KERNEL_BASE
 *
 * PDE index = virtual address bits 22-31
 * KERNEL_BASE (0xC0000000) >> 22 = 768
 */
#define KERNEL_PAGE_DIR_IDX 768

/*
 * =============================================================================
 * Address Conversion Macros
 * =============================================================================
 *
 * After paging is enabled, physical addresses cannot be accessed directly.
 * Use P2V() to convert physical to virtual, V2P() for the reverse.
 *
 * IMPORTANT: These only work for addresses in the kernel's direct-mapped
 * region (physical 0x00000000 maps to virtual 0xC0000000).
 */

/*
 * P2V - Convert physical address to virtual address
 *
 * @phys: Physical address
 *
 * Returns: Corresponding virtual address in kernel space
 */
#define P2V(phys)   ((uint32_t)(phys) + KERNEL_BASE)

/*
 * V2P - Convert virtual address to physical address
 *
 * @virt: Virtual address (must be >= KERNEL_BASE)
 *
 * Returns: Corresponding physical address
 */
#define V2P(virt)   ((uint32_t)(virt) - KERNEL_BASE)

/*
 * =============================================================================
 * Page Constants
 * =============================================================================
 *
 * Note: PAGE_SIZE and PAGE_SHIFT are also defined in pmm.h.
 * We use #ifndef guards to avoid redefinition errors.
 */

#ifndef PAGE_SIZE
#define PAGE_SIZE       4096            /* 4KB per page */
#endif

#ifndef PAGE_SHIFT
#define PAGE_SHIFT      12              /* log2(PAGE_SIZE) */
#endif

/*
 * PAGE_MASK - Mask to extract page-aligned address
 *
 * Clears the low 12 bits (offset within page).
 * Note: Also defined in pmm.h - use guards to avoid redefinition.
 */
#ifndef PAGE_MASK
#define PAGE_MASK       (~(PAGE_SIZE - 1))
#endif

/*
 * =============================================================================
 * VMM Functions
 * =============================================================================
 */

/*
 * vmm_init - Initialize the virtual memory manager
 *
 * Called after pmm_init(). At this point, paging has already been enabled
 * by entry.S, and the kernel is running at higher-half addresses.
 *
 * This function:
 *   1. Verifies paging is enabled (CR0.PG = 1)
 *   2. Stores reference to kernel page directory
 *   3. Initializes VMM state
 */
void vmm_init(void);

/*
 * vmm_map_page - Map a virtual address to a physical address
 *
 * Creates or updates the page table entry for the given virtual address.
 * Allocates a new page table from PMM if necessary.
 *
 * @virt:  Virtual address to map (will be page-aligned)
 * @phys:  Physical address to map to (will be page-aligned)
 * @flags: Page flags (PAGE_PRESENT, PAGE_WRITABLE, PAGE_USER)
 *
 * Returns: 0 on success, -ENOMEM if page table allocation fails
 */
int vmm_map_page(uint32_t virt, uint32_t phys, uint32_t flags);

/*
 * vmm_unmap_page - Unmap a virtual address
 *
 * Clears the page table entry and invalidates the TLB.
 * Does NOT free the physical frame - caller's responsibility.
 *
 * @virt: Virtual address to unmap (will be page-aligned)
 */
void vmm_unmap_page(uint32_t virt);

/*
 * vmm_get_physaddr - Get physical address for a virtual address
 *
 * Walks the page tables to find the physical address mapped
 * to the given virtual address.
 *
 * @virt: Virtual address to translate
 *
 * Returns: Physical address, or 0 if not mapped
 *
 * Note: Physical address 0x00000000 is indistinguishable from
 * "not mapped". This is acceptable because the PMM reserves the
 * first 1MB and never allocates frame 0.
 */
uint32_t vmm_get_physaddr(uint32_t virt);

#endif /* KERNEL_INCLUDE_VMM_H */
