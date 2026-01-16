/*
 * kernel/lib/panic.c - Kernel Panic Handler Implementation
 *
 * Implements the panic handler that captures register state, displays
 * an error message, and halts the system.
 *
 * CRITICAL: Register capture must happen FIRST before any other operations
 * including local variable initialization, as the function prologue will
 * modify ESP/EBP and other registers.
 */

#include <panic.h>
#include <printk.h>
#include <vga.h>
#include <asm.h>
#include <types.h>

/*
 * panic - Halt the kernel with error message and register dump
 *
 * This function captures all general-purpose registers, displays the
 * panic message in red, dumps register state, and halts.
 *
 * IMPORTANT: The register capture assembly MUST be the first thing in
 * this function to capture the state at the time of the call.
 */
void panic(const char *msg)
{
    /*
     * Register storage - must be first to ensure capture happens
     * before any stack frame manipulation
     */
    uint32_t eax, ebx, ecx, edx, esi, edi, ebp, esp, eip, eflags;

    /*
     * CRITICAL: Capture registers IMMEDIATELY
     *
     * This inline assembly captures all GP registers. We use "=m" constraints
     * to write directly to memory, avoiding any register clobbering issues.
     *
     * IMPORTANT: These values reflect the state AFTER the function prologue:
     *   - EBP/ESP are from panic()'s stack frame, not the caller's
     *   - EAX/ECX/EDX may be clobbered by the caller per cdecl convention
     *   - EBX/ESI/EDI are callee-saved, so they reflect the caller's values
     *   - EIP is recovered from the return address at [EBP+4]
     * This is still useful for debugging but not a perfect snapshot.
     */
    __asm__ volatile (
        "movl %%eax, %0\n"
        "movl %%ebx, %1\n"
        "movl %%ecx, %2\n"
        "movl %%edx, %3\n"
        "movl %%esi, %4\n"
        "movl %%edi, %5\n"
        "movl %%ebp, %6\n"
        "movl %%esp, %7\n"
        "pushfl\n"
        "popl %8\n"
        : "=m"(eax), "=m"(ebx), "=m"(ecx), "=m"(edx),
          "=m"(esi), "=m"(edi), "=m"(ebp), "=m"(esp), "=m"(eflags)
    );

    /*
     * Get caller's EIP from return address on stack
     * The return address is at [ebp + 4] in standard calling convention
     */
    __asm__ volatile (
        "movl 4(%%ebp), %0"
        : "=r"(eip)
    );

    /* Disable interrupts - we're not coming back */
    cli();

    /*
     * Display KERNEL PANIC header (red on VGA)
     */
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    printk(LOG_ERROR, "\n*** KERNEL PANIC: %s ***\n\n", msg);

    /*
     * Display register dump
     */
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    printk(LOG_ERROR, "Register dump:\n");
    printk(LOG_ERROR, "  EAX=0x%X  EBX=0x%X\n", eax, ebx);
    printk(LOG_ERROR, "  ECX=0x%X  EDX=0x%X\n", ecx, edx);
    printk(LOG_ERROR, "  ESI=0x%X  EDI=0x%X\n", esi, edi);
    printk(LOG_ERROR, "  EBP=0x%X  ESP=0x%X\n", ebp, esp);
    printk(LOG_ERROR, "  EIP=0x%X  EFLAGS=0x%X\n\n", eip, eflags);

    /* Final message */
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    printk(LOG_ERROR, "System halted.\n");

    /*
     * Halt forever
     *
     * Use cli; hlt in a loop. The HLT instruction will stop the CPU
     * until an interrupt, but since interrupts are disabled, we'll
     * only wake on NMI - then immediately halt again.
     */
    for (;;) {
        cli();
        hlt();
    }
}
