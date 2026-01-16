/*
 * kernel/include/panic.h - Kernel Panic Handler Interface
 *
 * Provides panic() for unrecoverable kernel errors. When called:
 *   1. Captures all CPU registers
 *   2. Displays "KERNEL PANIC" message in red on VGA
 *   3. Dumps register state to both VGA and serial
 *   4. Halts the system in an infinite loop
 *
 * Usage:
 *   panic("Out of memory");
 *   panic("Invalid page table entry");
 *
 * This should only be called for truly unrecoverable situations
 * where the kernel cannot continue safely.
 */

#ifndef KERNEL_INCLUDE_PANIC_H
#define KERNEL_INCLUDE_PANIC_H

/*
 * panic - Halt the kernel with error message and register dump
 *
 * This function never returns. It:
 *   1. Immediately captures all general-purpose registers
 *   2. Disables interrupts
 *   3. Displays "KERNEL PANIC: <message>" in red
 *   4. Prints complete register dump
 *   5. Halts in cli; hlt loop
 *
 * @msg: Description of the panic condition (null-terminated string)
 */
void panic(const char *msg) __attribute__((noreturn));

#endif /* KERNEL_INCLUDE_PANIC_H */
