/*
 * main.c - Kernel Main Entry Point
 *
 * =============================================================================
 * KERNEL MAIN (Story 1.5)
 * =============================================================================
 *
 * This is the C entry point for the kernel. It's called by entry.S after
 * the assembly startup code has:
 *   - Saved boot parameters
 *   - Cleared BSS
 *   - Set up the stack
 *
 * At this point:
 *   - 32-bit protected mode
 *   - Interrupts disabled
 *   - Paging disabled (physical == virtual)
 *   - Running at physical 0x100000
 *
 * Initialization order:
 *   1. GDT setup (Story 1.4)
 *   2. VGA driver (Story 1.5)
 *   3. Serial debug, printk, panic (Story 1.6)
 *   4. IDT, interrupts (Story 2.x)
 *   5. Memory management (Story 3.x)
 *
 * =============================================================================
 */

#include <types.h>
#include <gdt.h>
#include <vga.h>
#include <asm.h>

#ifdef TEST_MODE
#include <test.h>
#endif

/*
 * Boot parameters from entry.S
 *
 * These are set by the assembly startup code from values passed by
 * the bootloader in registers.
 */
extern uint32_t boot_mmap_ptr;
extern uint32_t boot_mmap_count;

/*
 * print_num - Print a number (0-999) using VGA driver
 *
 * Helper function to display memory map count.
 *
 * Note: Duplicated in test_runner.c as test_print_num(). Both are
 * temporary helpers until printk is implemented in Story 1.6.
 * Intentionally kept separate to avoid cross-dependencies between
 * boot code and test infrastructure.
 */
static void print_num(uint32_t num)
{
    if (num >= 100) {
        vga_putchar('0' + (num / 100));
        num %= 100;
        vga_putchar('0' + (num / 10));
        vga_putchar('0' + (num % 10));
    } else if (num >= 10) {
        vga_putchar('0' + (num / 10));
        vga_putchar('0' + (num % 10));
    } else {
        vga_putchar('0' + num);
    }
}

/*
 * kmain - Kernel main entry point
 *
 * This is the C entry point called by entry.S. It performs basic
 * kernel initialization and then halts.
 *
 * Initialization sequence:
 *   1. Initialize GDT (segment descriptors)
 *   2. Initialize VGA driver (text output)
 *   3. Display boot messages
 *   4. Run tests if TEST_MODE enabled
 *   5. Halt
 */
void kmain(void)
{
    /*
     * Initialize GDT (must be first - we need proper segments)
     *
     * This replaces the minimal bootloader GDT with the kernel's
     * complete GDT including user mode and TSS placeholders.
     */
    gdt_init();

    /*
     * Initialize VGA driver
     *
     * Clears screen to black, resets cursor to (0,0), and
     * enables hardware cursor tracking.
     */
    vga_init();

    /*
     * Display boot message
     *
     * Shows we successfully:
     *   - Loaded through bootloader stages
     *   - Reached C code in protected mode
     *   - Initialized GDT and VGA
     */
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("Hello from os-dev!\n");

    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts("GDT OK\n");

    /*
     * Display memory map count
     *
     * Shows how many E820 memory map entries we got from BIOS.
     */
    vga_puts("Memory map entries: ");
    print_num(boot_mmap_count);
    vga_putchar('\n');

    /*
     * Run tests if TEST_MODE is enabled
     *
     * When compiled with -DTEST_MODE, run the kernel test suite
     * instead of normal operation. Tests output via serial/VGA.
     */
#ifdef TEST_MODE
    test_run_all();
#endif

    /*
     * Halt
     *
     * We're done for this story. The HLT instruction stops the CPU
     * until an interrupt occurs. Since interrupts are disabled,
     * this effectively stops execution.
     *
     * In later stories, we'll have a proper scheduler loop here.
     */
    for (;;) {
        hlt();
    }
}
