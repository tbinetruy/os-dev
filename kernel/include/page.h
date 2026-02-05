/*
 * page.h - Page Table Entry Definitions
 *
 * Defines the x86 paging structures and flags per Intel SDM Vol 3 Chapter 4.
 *
 * x86 Two-Level Paging:
 *   - Page Directory: 1024 entries × 4 bytes = 4KB
 *   - Page Table: 1024 entries × 4 bytes = 4KB
 *   - Each page: 4KB
 *   - Total addressable: 1024 × 1024 × 4KB = 4GB
 */

#ifndef KERNEL_INCLUDE_PAGE_H
#define KERNEL_INCLUDE_PAGE_H

#include <types.h>

/*
 * =============================================================================
 * Page Table Entry Format (i386)
 * =============================================================================
 *
 * Both Page Directory Entries (PDEs) and Page Table Entries (PTEs) share
 * the same format. The Frame Address field points to:
 *   - PDE: Physical address of a Page Table
 *   - PTE: Physical address of a 4KB page frame
 *
 * 31                      12 11  9 8 7 6 5 4 3 2 1 0
 * ┌─────────────────────────┬────┬─┬─┬─┬─┬─┬─┬─┬─┬─┐
 * │   Frame Address (20b)   │AVL │G│0│D│A│C│W│U│W│P│
 * └─────────────────────────┴────┴─┴─┴─┴─┴─┴─┴─┴─┴─┘
 *
 * Bit  Name  Description
 * ---  ----  -----------
 *  0   P     Present: 1 = entry is valid, 0 = not present (triggers #PF)
 *  1   R/W   Read/Write: 1 = writable, 0 = read-only
 *  2   U/S   User/Supervisor: 1 = user accessible, 0 = kernel only
 *  3   PWT   Page Write-Through: 1 = write-through caching
 *  4   PCD   Page Cache Disable: 1 = disable caching for this page
 *  5   A     Accessed: Set by CPU when page is read or written
 *  6   D     Dirty: Set by CPU when page is written (PTE only)
 *  7   PAT   Page Attribute Table (or PS for 4MB pages in PDE)
 *  8   G     Global: TLB entry not flushed on CR3 write
 * 9-11 AVL   Available for OS use
 * 12-31      Frame address (physical address >> 12)
 */

/*
 * =============================================================================
 * Page Entry Flags
 * =============================================================================
 */

/*
 * PAGE_PRESENT - Page is present in memory
 *
 * If clear, access to this page triggers a page fault (#PF).
 * All other bits are available for OS use when P=0.
 */
#define PAGE_PRESENT        0x001

/*
 * PAGE_WRITABLE - Page is writable
 *
 * If clear, writes to this page trigger a page fault.
 * Note: In ring 0 (kernel), this is ignored unless CR0.WP=1.
 */
#define PAGE_WRITABLE       0x002

/*
 * PAGE_USER - Page is accessible from user mode (ring 3)
 *
 * If clear, only kernel (ring 0) can access this page.
 * User access triggers a page fault with error code bit 2 set.
 */
#define PAGE_USER           0x004

/*
 * PAGE_WRITE_THROUGH - Enable write-through caching
 *
 * If set, writes go directly to memory (not cached).
 * Useful for memory-mapped I/O regions.
 */
#define PAGE_WRITE_THROUGH  0x008

/*
 * PAGE_CACHE_DISABLE - Disable caching for this page
 *
 * If set, page is not cached. Required for memory-mapped I/O.
 */
#define PAGE_CACHE_DISABLE  0x010

/*
 * PAGE_ACCESSED - Page has been accessed
 *
 * Set by CPU on any access (read or write). Must be cleared
 * by software. Used for page replacement algorithms.
 */
#define PAGE_ACCESSED       0x020

/*
 * PAGE_DIRTY - Page has been written
 *
 * Set by CPU on write access. Must be cleared by software.
 * Only valid in PTEs (not PDEs). Used for page replacement.
 */
#define PAGE_DIRTY          0x040

/*
 * PAGE_GLOBAL - Page is global (TLB not flushed on CR3 write)
 *
 * Requires CR4.PGE=1. Used for kernel pages that are shared
 * across all processes to avoid TLB flushes.
 */
#define PAGE_GLOBAL         0x100

/*
 * =============================================================================
 * Page Entry Masks
 * =============================================================================
 */

/*
 * PAGE_FRAME_MASK - Extract physical frame address from entry
 *
 * Masks off the low 12 bits (flags) to get the page-aligned
 * physical address.
 */
#define PAGE_FRAME_MASK     0xFFFFF000

/*
 * PAGE_FLAGS_MASK - Extract flags from entry
 *
 * Masks off the high 20 bits (frame address) to get just the flags.
 */
#define PAGE_FLAGS_MASK     0x00000FFF

/*
 * =============================================================================
 * Address Decomposition Macros
 * =============================================================================
 *
 * Virtual address breakdown (32-bit):
 *
 *   31        22 21        12 11          0
 *   ┌──────────┬────────────┬─────────────┐
 *   │ PD Index │  PT Index  │   Offset    │
 *   │ (10 bits)│ (10 bits)  │  (12 bits)  │
 *   └──────────┴────────────┴─────────────┘
 */

/*
 * PDE_INDEX - Extract page directory index from virtual address
 *
 * @addr: Virtual address
 *
 * Returns: Page directory index (0-1023)
 */
#define PDE_INDEX(addr)     (((uint32_t)(addr) >> 22) & 0x3FF)

/*
 * PTE_INDEX - Extract page table index from virtual address
 *
 * @addr: Virtual address
 *
 * Returns: Page table index (0-1023)
 */
#define PTE_INDEX(addr)     (((uint32_t)(addr) >> 12) & 0x3FF)

/*
 * PAGE_OFFSET - Extract offset within page from virtual address
 *
 * @addr: Virtual address
 *
 * Returns: Offset within 4KB page (0-4095)
 */
#define PAGE_OFFSET(addr)   ((uint32_t)(addr) & 0xFFF)

/*
 * =============================================================================
 * Common Flag Combinations
 * =============================================================================
 */

/*
 * PAGE_KERNEL - Kernel page (present, writable, supervisor)
 */
#define PAGE_KERNEL         (PAGE_PRESENT | PAGE_WRITABLE)

/*
 * PAGE_USER_RO - User read-only page
 */
#define PAGE_USER_RO        (PAGE_PRESENT | PAGE_USER)

/*
 * PAGE_USER_RW - User read-write page
 */
#define PAGE_USER_RW        (PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER)

#endif /* KERNEL_INCLUDE_PAGE_H */
