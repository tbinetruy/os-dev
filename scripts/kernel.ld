/*
 * kernel.ld - Linker Script for os-dev Higher-Half Kernel
 *
 * =============================================================================
 * HIGHER-HALF KERNEL LINKER SCRIPT (Story 3.2)
 * =============================================================================
 *
 * This script arranges kernel sections for a higher-half kernel:
 *   - Virtual addresses start at 0xC0100000 (3GB + 1MB)
 *   - Physical addresses start at 0x00100000 (1MB)
 *   - Paging maps virtual -> physical in entry.S
 *
 * The key insight: virtual - KERNEL_VIRT_BASE = physical - KERNEL_PHYS_BASE
 * So for any virtual address V, physical = V - 0xC0000000
 *
 * =============================================================================
 */

ENTRY(_start)

/*
 * Address constants
 */
KERNEL_VIRT_BASE = 0xC0000000;      /* Virtual base (3GB) */
KERNEL_PHYS_BASE = 0x00100000;      /* Physical base (1MB) */
KERNEL_VIRT_START = 0xC0100000;     /* Virtual load address */

SECTIONS
{
    /*
     * Set virtual address to 3GB + 1MB
     * All symbol addresses will be virtual from here on
     */
    . = KERNEL_VIRT_START;

    _kernel_virt_start = .;
    _kernel_start = .;

    /*
     * .text - Code Section
     *
     * AT(addr - KERNEL_VIRT_BASE) computes the physical address
     * from the virtual address.
     */
    .text ALIGN(0x1000) : AT(ADDR(.text) - KERNEL_VIRT_BASE)
    {
        *(.text.boot)       /* Boot code runs before paging */
        *(.text)            /* All other code */
        *(.text.*)          /* Compiler-generated sections */
    }

    /*
     * .rodata - Read-Only Data Section
     */
    .rodata ALIGN(0x1000) : AT(ADDR(.rodata) - KERNEL_VIRT_BASE)
    {
        _rodata_start = .;
        *(.rodata)
        *(.rodata.*)
        _rodata_end = .;
    }

    /*
     * .data - Initialized Data Section
     */
    .data ALIGN(0x1000) : AT(ADDR(.data) - KERNEL_VIRT_BASE)
    {
        _data_start = .;
        *(.data)
        *(.data.*)
        _data_end = .;
    }

    /*
     * .bss - Uninitialized Data Section
     *
     * Note: BSS doesn't need AT() since it's not loaded from file.
     * The physical address is computed from the virtual address.
     */
    .bss ALIGN(0x1000) : AT(ADDR(.bss) - KERNEL_VIRT_BASE)
    {
        _bss_start = .;

        /*
         * Boot page tables - MUST be 4KB aligned
         */
        . = ALIGN(0x1000);
        *(.bss.boot)

        /*
         * Regular BSS
         */
        *(.bss)
        *(.bss.*)
        *(COMMON)

        _bss_end = .;
    }

    . = ALIGN(0x1000);

    _kernel_virt_end = .;
    _kernel_end = .;

    /*
     * Physical end address (for memory manager)
     */
    _kernel_phys_end = _kernel_virt_end - KERNEL_VIRT_BASE;

    /DISCARD/ :
    {
        *(.comment)
        *(.note.*)
        *(.eh_frame*)
    }
}
