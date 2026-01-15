/*
 * main.c - Kernel Main Entry Point
 *
 * =============================================================================
 * KERNEL MAIN (Story 1.3)
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
 *   - No console output (VGA driver not yet implemented)
 *
 * For now, we just write 'K' to the VGA buffer to show we reached the kernel,
 * then halt. Later stories will add:
 *   - Story 1.4: Proper GDT setup
 *   - Story 1.5: VGA text mode driver
 *   - Story 1.6: Serial debug, printk, panic
 *   - Story 2.x: IDT, interrupts
 *   - Story 3.x: Memory management
 *
 * =============================================================================
 */

#include <types.h>

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
 * VGA text mode memory address
 *
 * The VGA text buffer starts at physical address 0xB8000. In text mode,
 * each character cell is 2 bytes:
 *   - Byte 0: ASCII character code
 *   - Byte 1: Attribute (foreground + background colors)
 *
 * Screen is 80 columns x 25 rows = 2000 characters = 4000 bytes
 *
 * Attribute byte format:
 *   Bits 0-3: Foreground color (0-15)
 *   Bits 4-6: Background color (0-7)
 *   Bit 7:    Blink (or bright background on some systems)
 *
 * Common colors:
 *   0x07 = Light gray on black (default)
 *   0x0F = White on black
 *   0x0A = Light green on black
 *   0x0C = Light red on black
 */
#define VGA_BUFFER ((volatile uint16_t *)0xB8000)
#define VGA_WIDTH  80
#define VGA_HEIGHT 25

/* VGA colors */
#define VGA_COLOR_BLACK         0
#define VGA_COLOR_LIGHT_GREY    7
#define VGA_COLOR_WHITE         15
#define VGA_COLOR_LIGHT_GREEN   10

/*
 * vga_entry - Create a VGA character entry
 *
 * Combines a character and color attribute into a 16-bit VGA entry.
 *
 * @c:  ASCII character
 * @fg: Foreground color (0-15)
 * @bg: Background color (0-7)
 *
 * Returns: 16-bit VGA entry (character | (attribute << 8))
 */
static inline uint16_t vga_entry(char c, uint8_t fg, uint8_t bg)
{
    uint8_t color = fg | (bg << 4);
    return (uint16_t)c | ((uint16_t)color << 8);
}

/*
 * kmain - Kernel main entry point
 *
 * This is the C entry point called by entry.S. It performs basic
 * kernel initialization and then halts.
 *
 * For Story 1.3, we just:
 *   1. Clear the screen
 *   2. Display "K" to show we reached the kernel
 *   3. Halt
 *
 * Later stories will add proper initialization and a main loop.
 */
void kmain(void)
{
    /*
     * Clear the VGA screen
     *
     * Fill the entire screen with spaces (blank characters) with
     * white-on-black attribute. This gives us a clean slate.
     */
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_BUFFER[i] = vga_entry(' ', VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    }

    /*
     * Display "OK" at the top-left corner
     *
     * This shows that:
     *   - Stage 1 loaded stage 2
     *   - Stage 2 enabled A20, loaded kernel, switched to protected mode
     *   - Kernel entry point was reached
     *   - C runtime is working (BSS cleared, stack set up)
     *
     * We use green on black for visibility.
     */
    VGA_BUFFER[0] = vga_entry('O', VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    VGA_BUFFER[1] = vga_entry('K', VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);

    /*
     * Display memory map count (for debugging)
     *
     * Show how many memory map entries we received from BIOS.
     * This helps verify E820 worked. Display at position (0, 1).
     */
    VGA_BUFFER[VGA_WIDTH] = vga_entry('M', VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    VGA_BUFFER[VGA_WIDTH + 1] = vga_entry(':', VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    /* Convert count to ASCII digits (handles 0-999) */
    uint32_t count = boot_mmap_count;
    int pos = VGA_WIDTH + 2;
    if (count >= 100) {
        VGA_BUFFER[pos++] = vga_entry('0' + (count / 100), VGA_COLOR_WHITE,
                                       VGA_COLOR_BLACK);
        count %= 100;
        VGA_BUFFER[pos++] = vga_entry('0' + (count / 10), VGA_COLOR_WHITE,
                                       VGA_COLOR_BLACK);
        VGA_BUFFER[pos++] = vga_entry('0' + (count % 10), VGA_COLOR_WHITE,
                                       VGA_COLOR_BLACK);
    } else if (count >= 10) {
        VGA_BUFFER[pos++] = vga_entry('0' + (count / 10), VGA_COLOR_WHITE,
                                       VGA_COLOR_BLACK);
        VGA_BUFFER[pos++] = vga_entry('0' + (count % 10), VGA_COLOR_WHITE,
                                       VGA_COLOR_BLACK);
    } else {
        VGA_BUFFER[pos++] = vga_entry('0' + count, VGA_COLOR_WHITE,
                                       VGA_COLOR_BLACK);
    }

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
        __asm__ volatile ("hlt");
    }
}
