/*
 * entry.S - Kernel Entry Point
 *
 * =============================================================================
 * KERNEL ENTRY POINT (Story 1.3)
 * =============================================================================
 *
 * This is the first kernel code that runs after the bootloader transfers
 * control. The CPU is in 32-bit protected mode with paging disabled.
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
 *   2. Clears BSS section (C requires uninitialized globals = 0)
 *   3. Sets up stack (reinforces bootloader setup)
 *   4. Calls kmain() - the C entry point
 *   5. Halts if kmain returns (should never happen)
 *
 * =============================================================================
 */

.code32
.section .text.boot     /* Place this first in the binary (see linker script) */
.global _start
.extern kmain
.extern _bss_start
.extern _bss_end

/*
 * _start - Kernel entry point
 *
 * This symbol MUST be at the start of the kernel binary. The bootloader
 * jumps directly to address 0x100000, so this code must be there.
 *
 * Input:
 *   EBX = memory map entries pointer
 *   ECX = memory map entry count
 * Output:
 *   Never returns
 * Clobbers:
 *   All registers (we're starting fresh)
 */
_start:
    /*
     * Disable interrupts (should already be disabled, but be defensive)
     *
     * We won't enable them until we've set up an IDT in Story 2.1.
     * Any interrupt at this point would cause a triple fault.
     */
    cli

    /*
     * Save boot parameters from bootloader
     *
     * The bootloader passes memory map information in registers:
     *   EBX = pointer to E820 entries
     *   ECX = number of entries
     *
     * We save these to global variables so C code can access them.
     * These will be used by the physical memory manager (Story 3.1).
     */
    movl %ebx, boot_mmap_ptr
    movl %ecx, boot_mmap_count

    /*
     * Clear BSS section
     *
     * C language semantics require all uninitialized global and static
     * variables to be zero-initialized. These variables are placed in
     * the BSS section, which the linker just reserves space for (doesn't
     * store actual zeros in the binary to save space).
     *
     * We must zero this region before calling any C code.
     *
     * _bss_start and _bss_end are defined by the linker script.
     */
    movl $_bss_start, %edi      /* EDI = start of BSS */
    movl $_bss_end, %ecx        /* ECX = end of BSS */
    subl %edi, %ecx             /* ECX = size of BSS in bytes */
    shrl $2, %ecx               /* ECX = size in double-words (divide by 4) */
    xorl %eax, %eax             /* EAX = 0 (value to store) */
    cld                         /* Clear direction flag (increment addresses) */
    rep stosl                   /* Store EAX to [EDI], ECX times */

    /*
     * Set up stack pointer
     *
     * The bootloader already set ESP to 0x90000, but we reinforce it here.
     * The stack grows downward, so this gives us plenty of space.
     *
     * Stack region: 0x80000 - 0x90000 (64KB)
     *
     * Note: In Story 1.4, we'll set up a proper kernel stack in the
     * higher-half kernel area. For now, we use this temporary location.
     */
    movl $0x90000, %esp
    movl %esp, %ebp             /* Set frame pointer = stack pointer */

    /*
     * Call C entry point
     *
     * kmain() is the C entry point defined in main.c. It will:
     *   - Initialize kernel subsystems
     *   - Set up memory management
     *   - Eventually start user processes
     *
     * If kmain() ever returns, we halt below (should never happen).
     */
    call kmain

    /*
     * If kmain returns, halt the system
     *
     * kmain() should never return - it should loop forever or start
     * a scheduler. If we get here, something went very wrong.
     */
.Lhalt:
    cli                         /* Ensure interrupts stay disabled */
    hlt                         /* Halt the CPU */
    jmp .Lhalt                  /* Loop in case of NMI or SMI */


/*
 * =============================================================================
 * DATA SECTION
 * =============================================================================
 *
 * Boot parameters saved for C code access. These are global variables
 * that can be accessed from C code as:
 *
 *   extern uint32_t boot_mmap_ptr;
 *   extern uint32_t boot_mmap_count;
 */

.section .data

.global boot_mmap_ptr
.global boot_mmap_count

/*
 * boot_mmap_ptr - Pointer to E820 memory map entries
 *
 * Points to an array of E820 entries at physical address 0x504.
 * Each entry is 24 bytes: base(8) + length(8) + type(4) + extended(4)
 */
boot_mmap_ptr:
    .long 0

/*
 * boot_mmap_count - Number of E820 memory map entries
 *
 * The count of valid entries in the memory map array.
 */
boot_mmap_count:
    .long 0
