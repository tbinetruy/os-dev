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
#include <idt.h>
#include <pic.h>
#include <timer.h>
#include <keyboard.h>
#include <vga.h>
#include <asm.h>
#include <serial.h>
#include <printk.h>
#include <panic.h>

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
 * kmain - Kernel main entry point
 *
 * This is the C entry point called by entry.S. It performs basic
 * kernel initialization and then halts.
 *
 * Initialization sequence:
 *   1. Initialize GDT (segment descriptors)
 *   2. Initialize VGA driver (text output)
 *   3. Initialize serial driver (debug output)
 *   4. Display boot messages via printk
 *   5. Run tests if TEST_MODE enabled
 *   6. Halt
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
     * Initialize serial driver
     *
     * Configures COM1 for 38400 baud 8N1 output. This enables
     * printk output to both serial and VGA from this point on.
     */
    serial_init();

    /*
     * Initialize IDT (exception handlers)
     *
     * Sets up CPU exception handlers (0-31). Interrupts remain
     * disabled until PIC is configured (Story 2.2).
     */
    idt_init();

    /*
     * Initialize PIC (Story 2.2)
     *
     * Remaps IRQs to avoid conflicts with CPU exceptions:
     *   IRQ 0-7  → INT 32-39
     *   IRQ 8-15 → INT 40-47
     * All IRQs are masked (disabled) after initialization.
     */
    pic_init();

    /*
     * Initialize Timer (Story 2.2)
     *
     * Configures PIT for 100 Hz (10ms interval) and enables IRQ 0.
     * Timer interrupts won't fire until STI enables interrupts.
     */
    timer_init();

    /*
     * Initialize Keyboard (Story 2.3)
     *
     * Registers IRQ 1 handler and enables keyboard interrupts.
     * Keystrokes will be buffered for retrieval via keyboard_getchar().
     */
    keyboard_init();

    /*
     * Display boot progress via printk
     *
     * All kernel messages now go through printk which outputs to
     * both serial (visible in QEMU -serial stdio) and VGA.
     */
    printk(LOG_INFO, "os-dev kernel starting\n");
    printk(LOG_INFO, "GDT initialized\n");
    printk(LOG_INFO, "VGA initialized\n");
    printk(LOG_INFO, "Serial initialized\n");
    printk(LOG_INFO, "IDT initialized\n");
    printk(LOG_INFO, "PIC initialized\n");
    printk(LOG_INFO, "Timer initialized (%d Hz)\n", TARGET_HZ);
    printk(LOG_INFO, "Memory map entries: %d\n", boot_mmap_count);

    /*
     * Enable interrupts (Story 2.2)
     *
     * Now that PIC and timer are configured, enable CPU interrupts.
     * Timer interrupt (IRQ 0) will start firing at 100 Hz.
     */
    sti();
    printk(LOG_INFO, "Interrupts enabled\n");

    /*
     * Run tests if TEST_MODE is enabled
     *
     * When compiled with -DTEST_MODE, run the kernel test suite
     * instead of normal operation. Tests output via serial/VGA.
     */
#ifdef TEST_MODE
    test_run_all();
#endif

    printk(LOG_INFO, "Boot complete\n");

    /*
     * Main loop with keyboard echo
     *
     * Echo keyboard input to VGA for manual testing.
     * The HLT instruction puts the CPU into a low-power state
     * until an interrupt occurs. With timer running at 100 Hz,
     * HLT will return approximately every 10ms.
     *
     * In later stories, the scheduler will run here.
     */
    for (;;) {
        int c = keyboard_getchar();
        if (c != -1) {
            vga_putchar((char)c);
        }
        hlt();
    }
}
