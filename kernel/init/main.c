#include <types.h>

/*
 * kmain - Kernel entry point
 *
 * Called by the bootloader after setting up protected mode
 * and paging. This is the C entry point for the kernel.
 */
void kmain(void)
{
    /* Placeholder - will be expanded in later stories */
    for (;;) {
        /* Halt the CPU until next interrupt */
        __asm__ volatile ("hlt");
    }
}
