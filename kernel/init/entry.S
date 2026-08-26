/*
 * entry.S - Kernel Entry Point with Higher-Half Paging Setup
 *
 * =============================================================================
 * KERNEL ENTRY POINT (Story 3.2 - Higher-Half Kernel)
 * =============================================================================
 *
 * This is the first kernel code that runs after the bootloader transfers
 * control. The CPU is in 32-bit protected mode with paging DISABLED.
 *
 * Entry conditions (from stage 2 bootloader):
 *   - 32-bit protected mode
 *   - Interrupts disabled
 *   - Paging disabled (physical == virtual addresses)
 *   - EBX = pointer to E820 memory map entries
 *   - ECX = number of memory map entries
 *   - ESP = valid stack at 0x90000
 *   - Running at physical address 0x100000 (1MB)
 *
 * This code:
 *   1. Saves boot parameters for C code access
 *   2. Clears BSS section (including boot page tables)
 *   3. Sets up page tables for higher-half mapping
 *   4. Enables paging
 *   5. Jumps to higher-half kernel code
 *   6. Updates stack to higher-half address
 *   7. Calls kmain() - the C entry point
 *
 * =============================================================================
 * HIGHER-HALF KERNEL MEMORY MAP
 * =============================================================================
 *
 * Before paging (physical addresses):
 *   0x00100000 - Kernel loaded here by bootloader
 *
 * After paging (virtual addresses):
 *   0xC0100000 - Kernel mapped here (higher-half)
 *   0x00100000 - Identity map (temporary, for transition)
 *
 * =============================================================================
 */

.code32

/*
 * Virtual/Physical address constants
 * Must match kernel.ld
 */
.set KERNEL_VIRT_BASE,  0xC0000000
.set KERNEL_PHYS_BASE,  0x00100000

/*
 * Page table flags
 */
.set PAGE_PRESENT,      0x001
.set PAGE_WRITABLE,     0x002
.set PAGE_FLAGS,        (PAGE_PRESENT | PAGE_WRITABLE)

/*
 * Number of pages per page table (each table covers 4MB)
 */
.set PAGE_TABLE_ENTRIES, 1024

/*
 * Number of page tables to create (4 tables = 16MB coverage)
 * This provides enough mapped memory for initial page table allocations.
 */
.set NUM_PAGE_TABLES, 4

/*
 * =============================================================================
 * BOOT CODE SECTION
 * =============================================================================
 *
 * This section MUST be first in the binary - bootloader jumps to 0x100000.
 * All code here runs BEFORE paging is enabled, using physical addresses.
 */
.section .text.boot
.global _start

/*
 * External symbols from linker script (these are VIRTUAL addresses)
 */
.extern _bss_start
.extern _bss_end
.extern kmain

/*
 * _start - Kernel entry point
 *
 * Input:
 *   EBX = memory map entries pointer
 *   ECX = memory map entry count
 * Output:
 *   Never returns
 * Clobbers:
 *   All registers
 */
_start:
    /*
     * Disable interrupts (should already be disabled, but be defensive)
     */
    cli

    /*
     * Save boot parameters from bootloader
     * Store to physical addresses (subtract KERNEL_VIRT_BASE from virtual)
     */
    movl %ebx, (boot_mmap_ptr - KERNEL_VIRT_BASE)
    movl %ecx, (boot_mmap_count - KERNEL_VIRT_BASE)

    /*
     * Clear BSS section
     *
     * All linker symbols are virtual addresses, so we convert to physical.
     * BSS includes the boot page tables which must be zeroed.
     */
    movl $(_bss_start - KERNEL_VIRT_BASE), %edi
    movl $(_bss_end - KERNEL_VIRT_BASE), %ecx
    subl %edi, %ecx             /* ECX = size of BSS in bytes */
    shrl $2, %ecx               /* ECX = size in dwords */
    xorl %eax, %eax             /* EAX = 0 */
    cld
    rep stosl

    /*
     * ==========================================================================
     * SET UP PAGING
     * ==========================================================================
     *
     * We set up a simple two-level paging structure:
     *   - Page Directory with 8 entries:
     *     - Entries 0-3: Identity map first 16MB (for transition)
     *     - Entries 768-771: Higher-half map first 16MB at 0xC0000000
     *   - 4 Page Tables mapping 4 × 1024 × 4KB = 16MB
     *     - Identity and higher-half PD entries share the same tables
     *
     * This allows us to:
     *   1. Continue running at physical 0x100000 after enabling paging
     *   2. Jump to virtual 0xC0100000
     *   3. Remove identity mapping after transition
     */

    /*
     * Fill boot page tables
     *
     * Map 16MB (4 page tables × 4MB each) starting at physical 0x00000000.
     * Each entry: physical_address | PAGE_PRESENT | PAGE_WRITABLE
     */
    movl $(boot_page_tables - KERNEL_VIRT_BASE), %edi
    movl $0, %esi                           /* Physical address starts at 0 */
    movl $(PAGE_TABLE_ENTRIES * NUM_PAGE_TABLES), %ecx  /* 4096 entries total */

.Lfill_page_tables:
    movl %esi, %eax
    orl  $PAGE_FLAGS, %eax                  /* Add flags */
    movl %eax, (%edi)                       /* Store PTE */
    addl $4096, %esi                        /* Next physical page */
    addl $4, %edi                           /* Next PTE */
    loop .Lfill_page_tables

    /*
     * Set up page directory entries
     *
     * PD entry format: page_table_phys_addr | flags
     * Set up both identity mapping (PDE 0-3) and higher-half (PDE 768-771)
     */
    movl $(boot_page_directory - KERNEL_VIRT_BASE), %edi
    movl $(boot_page_tables - KERNEL_VIRT_BASE), %esi
    movl $NUM_PAGE_TABLES, %ecx
    xorl %edx, %edx                         /* PDE index counter */

.Lsetup_pde:
    movl %esi, %eax
    orl  $PAGE_FLAGS, %eax

    /* Identity map entry (PDE 0, 1, 2, 3) */
    movl %eax, (%edi, %edx, 4)

    /* Higher-half entry (PDE 768, 769, 770, 771) */
    movl %eax, (768 * 4)(%edi, %edx, 4)

    addl $4096, %esi                        /* Next page table */
    incl %edx                               /* Next PDE index */
    loop .Lsetup_pde

    /*
     * Load page directory into CR3
     *
     * CR3 holds the PHYSICAL address of the page directory.
     */
    movl $(boot_page_directory - KERNEL_VIRT_BASE), %eax
    movl %eax, %cr3

    /*
     * Enable paging
     *
     * Set CR0.PG (bit 31) to enable paging.
     * After this instruction, all memory accesses go through paging.
     * We continue executing because of the identity mapping.
     */
    movl %cr0, %eax
    orl  $0x80000000, %eax
    movl %eax, %cr0

    /*
     * Jump to higher-half kernel code
     *
     * This jump uses an absolute address in the higher-half.
     * The identity mapping allows the fetch of this instruction to work,
     * and after the jump we're running at virtual addresses.
     */
    lea  .Lhigher_half, %eax
    jmp  *%eax

/*
 * =============================================================================
 * HIGHER-HALF CODE
 * =============================================================================
 *
 * From here on, we're running at virtual addresses (0xC0XXXXXX).
 * The identity mapping is still present but no longer needed.
 */

.section .text

.Lhigher_half:
    /*
     * Remove identity mapping
     *
     * Clear PDE entries 0-3 to remove the full 16MB identity map.
     * This helps catch bugs where code incorrectly uses physical addresses.
     */
    movl $boot_page_directory, %edi
    movl $0, 0(%edi)            /* PDE 0: 0-4MB */
    movl $0, 4(%edi)            /* PDE 1: 4-8MB */
    movl $0, 8(%edi)            /* PDE 2: 8-12MB */
    movl $0, 12(%edi)           /* PDE 3: 12-16MB */

    /*
     * Invalidate TLB for the identity-mapped region
     * (Reloading CR3 flushes the entire TLB)
     */
    movl %cr3, %eax
    movl %eax, %cr3

    /*
     * Update stack pointer to higher-half address
     *
     * The bootloader set ESP to 0x90000 (physical), the exclusive top of
     * PID 0's downward-growing bootstrap stack. It has no enforced lower
     * bound or guard and is never a normal kstack allocation.
     * We add KERNEL_VIRT_BASE to get the virtual address.
     * Note: The stack is in the first 4MB, so it's mapped.
     */
    addl $KERNEL_VIRT_BASE, %esp
    movl %esp, %ebp

    /*
     * Call C entry point
     *
     * kmain() is now at a higher-half virtual address.
     */
    call kmain

    /*
     * If kmain returns, halt the system
     */
.Lhalt:
    cli
    hlt
    jmp .Lhalt


/*
 * =============================================================================
 * BOOT DATA SECTION (BSS)
 * =============================================================================
 *
 * Page tables must be 4KB aligned. The .bss.boot section is placed
 * at a 4KB boundary by the linker script.
 */

.section .bss.boot, "aw", @nobits

/*
 * boot_page_directory - Page Directory (4KB, 1024 entries)
 *
 * Each entry points to a page table.
 * We only use entry 0 (identity) and entry 768 (higher-half).
 */
.global boot_page_directory
.align 4096
boot_page_directory:
    .skip 4096

/*
 * boot_page_tables - Page Tables for first 16MB (4 × 4KB = 16KB)
 *
 * Maps physical 0x00000000-0x00FFFFFF.
 * Used by both identity mapping (PDE 0-3) and higher-half (PDE 768-771).
 *
 * This provides enough mapped memory for vmm_map_page() to allocate
 * early low-memory objects before VMM installs recursive page-table access.
 */
.global boot_page_tables
.align 4096
boot_page_tables:
    .skip (4096 * 4)  /* 4 page tables */

/* Alias for backwards compatibility */
.global boot_page_table_0
.set boot_page_table_0, boot_page_tables


/*
 * =============================================================================
 * DATA SECTION
 * =============================================================================
 *
 * Boot parameters saved for C code access.
 */

.section .data

.global boot_mmap_ptr
.global boot_mmap_count

/*
 * boot_mmap_ptr - Pointer to E820 memory map entries
 */
boot_mmap_ptr:
    .long 0

/*
 * boot_mmap_count - Number of E820 memory map entries
 */
boot_mmap_count:
    .long 0
