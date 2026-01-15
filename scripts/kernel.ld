/*
 * kernel.ld - Linker Script for os-dev Kernel
 *
 * =============================================================================
 * KERNEL LINKER SCRIPT (Story 1.3)
 * =============================================================================
 *
 * This script tells the linker how to arrange kernel sections in memory.
 *
 * IMPORTANT: For Story 1.3, we run at PHYSICAL address 0x100000 (1MB).
 * Paging is NOT enabled yet - that comes in Story 3.2.
 *
 * In Story 3.2, this will change to support the higher-half kernel:
 *   - Virtual address: 0xC0100000 (3GB + 1MB)
 *   - Physical address: 0x100000 (1MB)
 *   - Paging maps virtual -> physical
 *
 * For now, virtual == physical == 0x100000.
 *
 * =============================================================================
 * MEMORY LAYOUT
 * =============================================================================
 *
 * x86 PC memory map (simplified):
 *
 *   0x00000000 - 0x000003FF : Interrupt Vector Table (IVT)
 *   0x00000400 - 0x000004FF : BIOS Data Area
 *   0x00000500 - 0x00007BFF : Free / Boot data (memory map stored here)
 *   0x00007C00 - 0x00007DFF : Stage 1 bootloader (MBR)
 *   0x00007E00 - 0x000087FF : Stage 2 bootloader
 *   0x00010000 - 0x0008FFFF : Kernel temporary load location
 *   0x00090000 - 0x0009FFFF : Stack (protected mode)
 *   0x000A0000 - 0x000BFFFF : Video memory (VGA)
 *   0x000C0000 - 0x000FFFFF : ROM area (BIOS, option ROMs)
 *   0x00100000 - onwards    : Extended memory - KERNEL LIVES HERE
 *
 * We load the kernel at 0x100000 (1MB) to avoid all the legacy clutter
 * in the first 1MB of memory.
 *
 * =============================================================================
 * SECTION ALIGNMENT
 * =============================================================================
 *
 * Sections are aligned to 4KB (0x1000) page boundaries. This enables:
 *   - Different memory protection per section (when paging is enabled)
 *   - .text:   Execute + Read
 *   - .rodata: Read only
 *   - .data:   Read + Write
 *   - .bss:    Read + Write
 *
 * =============================================================================
 * EXPORTED SYMBOLS
 * =============================================================================
 *
 * The linker exports symbols that C code can reference:
 *   extern char _kernel_start;  - Start of kernel image
 *   extern char _kernel_end;    - End of kernel image
 *   extern char _bss_start;     - Start of BSS section
 *   extern char _bss_end;       - End of BSS section
 *
 * These are used by:
 *   - entry.S: Zero BSS section at startup
 *   - Memory manager: Know kernel memory footprint
 *   - Paging: Set correct permissions
 *
 * =============================================================================
 */

/*
 * ENTRY(_start)
 *
 * The entry point is _start (in entry.S), which sets up the C runtime
 * and then calls kmain().
 *
 * The bootloader jumps to 0x100000, so _start must be at that address.
 * We ensure this by placing .text.boot first in the .text section.
 */
ENTRY(_start)

SECTIONS
{
    /*
     * . = 0x100000
     *
     * Set the location counter to 1MB. This is where the kernel
     * is loaded by the bootloader and where it will execute.
     *
     * In Story 3.2, this will change to 0xC0100000 for the higher-half
     * kernel, with AT(0x100000) specifying the physical load address.
     */
    . = 0x100000;

    /*
     * _kernel_start
     *
     * Mark the beginning of the kernel image. Used by the memory
     * manager to know where the kernel starts in physical memory.
     */
    _kernel_start = .;

    /*
     * .text - Code Section
     *
     * Contains all executable code. The .text.boot section is placed
     * first to ensure _start is at address 0x100000.
     *
     * Aligned to 4KB for page-level execute permission (future).
     */
    .text ALIGN(0x1000) :
    {
        *(.text.boot)   /* Entry point MUST be first */
        *(.text)        /* All other code */
        *(.text.*)      /* Compiler-generated sections */
    }

    /*
     * .rodata - Read-Only Data Section
     *
     * Contains:
     *   - String literals
     *   - const arrays
     *   - Switch jump tables
     *
     * Aligned to 4KB for page-level read-only permission (future).
     */
    .rodata ALIGN(0x1000) :
    {
        *(.rodata)
        *(.rodata.*)
    }

    /*
     * .data - Initialized Data Section
     *
     * Contains global/static variables with initial values.
     * These values are stored in the binary.
     *
     * Aligned to 4KB for page-level permissions (future).
     */
    .data ALIGN(0x1000) :
    {
        *(.data)
        *(.data.*)
    }

    /*
     * .bss - Uninitialized Data Section
     *
     * Contains global/static variables without initial values.
     * C requires these to be zero-initialized at runtime.
     *
     * _bss_start and _bss_end mark the region to zero in entry.S.
     *
     * Aligned to 4KB for page-level permissions (future).
     */
    .bss ALIGN(0x1000) :
    {
        _bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)       /* Old-style common symbols */
        _bss_end = .;
    }

    /*
     * _kernel_end
     *
     * Mark the end of the kernel image. Everything after this address
     * is free for dynamic allocation (once we have a memory manager).
     */
    _kernel_end = .;

    /*
     * /DISCARD/ - Sections to throw away
     *
     * These sections add no value at runtime and waste space:
     *   .comment  - Compiler version strings
     *   .note.*   - ELF notes (build-id, etc.)
     *   .eh_frame - C++ exception handling (we don't use exceptions)
     */
    /DISCARD/ :
    {
        *(.comment)
        *(.note.*)
        *(.eh_frame*)
    }
}
