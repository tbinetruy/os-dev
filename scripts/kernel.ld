/*
 * kernel.ld - Linker Script for os-dev Kernel
 *
 * =============================================================================
 * WHAT IS A LINKER SCRIPT?
 * =============================================================================
 *
 * When you compile C code, the compiler produces "object files" (.o files).
 * Each object file contains machine code organized into "sections":
 *
 *   .text   - Executable code (your functions)
 *   .rodata - Read-only data (string literals, const arrays)
 *   .data   - Initialized global/static variables
 *   .bss    - Uninitialized global/static variables (zero-filled at runtime)
 *
 * The LINKER combines all object files into a single executable. But it needs
 * to know WHERE in memory to place each section. That's what this script does.
 *
 * For a normal program, the OS loader handles this. But we ARE the OS, so we
 * must tell the linker exactly where our kernel will live in memory.
 *
 * =============================================================================
 * MEMORY LAYOUT BACKGROUND
 * =============================================================================
 *
 * x86 PCs have a legacy memory map from the 1980s IBM PC:
 *
 *   0x00000000 - 0x000003FF : Interrupt Vector Table (IVT) - BIOS uses this
 *   0x00000400 - 0x000004FF : BIOS Data Area (BDA)
 *   0x00000500 - 0x00007BFF : Free conventional memory
 *   0x00007C00 - 0x00007DFF : Boot sector loaded here by BIOS
 *   0x00007E00 - 0x0009FFFF : Free conventional memory (~600KB)
 *   0x000A0000 - 0x000BFFFF : Video memory (VGA framebuffer)
 *   0x000C0000 - 0x000FFFFF : ROM area (BIOS, option ROMs)
 *   0x00100000 - onwards    : Extended memory (FREE for kernel!)
 *
 * The first 1MB (0x00000000 - 0x000FFFFF) is called "conventional memory"
 * and is cluttered with legacy stuff. We load our kernel at 0x100000 (1MB)
 * to avoid all that mess.
 *
 * =============================================================================
 * HIGHER-HALF KERNEL DESIGN
 * =============================================================================
 *
 * We use a "higher-half kernel" design. This means:
 *
 *   Physical address: Where the kernel actually sits in RAM (0x100000 = 1MB)
 *   Virtual address:  Where the kernel THINKS it is (0xC0100000 = 3GB + 1MB)
 *
 * Why do this? When we enable paging (virtual memory), we want:
 *
 *   - Kernel space: 0xC0000000 - 0xFFFFFFFF (upper 1GB)
 *   - User space:   0x00000000 - 0xBFFFFFFF (lower 3GB)
 *
 * This gives each user process a clean 3GB address space starting at 0,
 * while the kernel is always mapped at the same high addresses. Every
 * process shares the same kernel mapping, making syscalls fast.
 *
 * The bootloader sets up initial page tables that map:
 *   Virtual 0xC0100000 -> Physical 0x100000
 *
 * So when the kernel code references address 0xC0100000, the CPU's MMU
 * (Memory Management Unit) translates it to physical address 0x100000.
 *
 * =============================================================================
 * SECTION ALIGNMENT
 * =============================================================================
 *
 * We align each section to 4KB (0x1000 bytes) boundaries. Why?
 *
 * x86 paging works in 4KB "pages". Each page can have different permissions:
 *   - .text:   Execute + Read (code runs here)
 *   - .rodata: Read only (constants, can't be modified)
 *   - .data:   Read + Write (variables)
 *   - .bss:    Read + Write (uninitialized variables)
 *
 * If sections aren't page-aligned, we can't set different permissions for
 * each section. Aligning to page boundaries enables proper memory protection.
 *
 * =============================================================================
 * EXPORTED SYMBOLS
 * =============================================================================
 *
 * We export symbols like _kernel_start, _kernel_end, _bss_start, _bss_end.
 * These become global variables that C code can reference:
 *
 *   extern char _kernel_start;  // Address of kernel start
 *   extern char _kernel_end;    // Address of kernel end
 *   extern char _bss_start;     // BSS section start
 *   extern char _bss_end;       // BSS section end
 *
 * The kernel uses these to:
 *   - Know how much memory the kernel occupies (for memory manager)
 *   - Zero out the BSS section at startup (C expects uninitialized globals = 0)
 *   - Set up page tables with correct permissions
 *
 * =============================================================================
 */

/*
 * ENTRY(kmain)
 *
 * Tells the linker that 'kmain' is the entry point. When the bootloader
 * jumps to the kernel, it jumps to this symbol's address.
 *
 * Note: The bootloader must know this address. It either:
 *   a) Reads it from the ELF header (if loading ELF format)
 *   b) Jumps to a known fixed address (if loading raw binary)
 *
 * Our bootloader will use approach (a) in later stories.
 */
ENTRY(kmain)

SECTIONS
{
    /*
     * . = 0xC0100000
     *
     * The '.' is the "location counter" - it tracks the current address
     * as we lay out sections. Setting it to 0xC0100000 means:
     *
     * "Start placing sections at virtual address 0xC0100000"
     *
     * This is 3GB + 1MB, leaving the lower 3GB for user programs.
     * The actual physical load address (1MB) is handled by the bootloader.
     */
    . = 0xC0100000;

    /*
     * _kernel_start = .
     *
     * Creates a symbol at the current address. C code can reference this
     * as 'extern char _kernel_start;' to know where the kernel begins.
     */
    _kernel_start = .;

    /*
     * .text ALIGN(0x1000) : { ... }
     *
     * CODE SECTION
     *
     * Contains all executable machine code. ALIGN(0x1000) ensures it starts
     * on a 4KB page boundary for proper memory protection (execute permission).
     *
     * *(.text) means: "include .text sections from ALL input object files"
     * *(.text.*) catches sections like .text.unlikely (compiler-generated)
     */
    .text ALIGN(0x1000) :
    {
        *(.text)
        *(.text.*)
    }

    /*
     * .rodata ALIGN(0x1000) : { ... }
     *
     * READ-ONLY DATA SECTION
     *
     * Contains:
     *   - String literals: "Hello, world!"
     *   - const arrays: const int table[] = {1, 2, 3};
     *   - Switch jump tables (compiler-generated)
     *
     * Aligned to page boundary so we can mark these pages read-only.
     * Any write attempt will cause a page fault (security feature).
     */
    .rodata ALIGN(0x1000) :
    {
        *(.rodata)
        *(.rodata.*)
    }

    /*
     * .data ALIGN(0x1000) : { ... }
     *
     * INITIALIZED DATA SECTION
     *
     * Contains global/static variables with initial values:
     *   int counter = 42;        // Goes in .data
     *   static char buf[10] = "hi";  // Goes in .data
     *
     * These values are stored in the kernel binary and copied to RAM.
     * The section is read-write (variables can be modified at runtime).
     */
    .data ALIGN(0x1000) :
    {
        *(.data)
        *(.data.*)
    }

    /*
     * .bss ALIGN(0x1000) : { ... }
     *
     * UNINITIALIZED DATA SECTION (Block Started by Symbol)
     *
     * Contains global/static variables WITHOUT initial values:
     *   int counter;             // Goes in .bss
     *   static char buffer[1024]; // Goes in .bss
     *
     * C guarantees these are zero-initialized. But we don't store zeros
     * in the binary (waste of space). Instead:
     *   1. Linker records the size of .bss
     *   2. Kernel startup code zeros this memory region
     *
     * _bss_start and _bss_end mark the region to zero.
     *
     * COMMON contains old-style "common" symbols (Fortran legacy).
     * Some compilers put uninitialized globals here instead of .bss.
     */
    .bss ALIGN(0x1000) :
    {
        _bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        _bss_end = .;
    }

    /*
     * _kernel_end = .
     *
     * Marks the end of all kernel sections. The memory manager uses this
     * to know where free memory begins (everything after _kernel_end is
     * available for allocation).
     */
    _kernel_end = .;

    /*
     * /DISCARD/ : { ... }
     *
     * Sections listed here are thrown away (not included in output).
     *
     * .comment  - Compiler version strings (not needed at runtime)
     * .note.*   - ELF notes (build-id, etc.)
     * .eh_frame - C++ exception handling tables (we don't use exceptions)
     *
     * Discarding these makes the kernel binary smaller.
     */
    /DISCARD/ :
    {
        *(.comment)
        *(.note.*)
        *(.eh_frame*)
    }
}
