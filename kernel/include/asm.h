/*
 * kernel/include/asm.h - Low-level assembly helpers
 *
 * Inline assembly wrappers for common x86 instructions that cannot be
 * expressed in pure C. These are used throughout the kernel for:
 *   - I/O port access (inb, outb, etc.)
 *   - CPU control (halt, interrupt enable/disable)
 *   - Memory barriers
 *
 * All functions are static inline to avoid function call overhead.
 */

#ifndef KERNEL_INCLUDE_ASM_H
#define KERNEL_INCLUDE_ASM_H

#include <types.h>

/*
 * =============================================================================
 * I/O Port Access
 * =============================================================================
 *
 * x86 uses a separate I/O address space accessed via IN/OUT instructions.
 * Common devices and their port ranges:
 *   - 0x20-0x21: Master PIC
 *   - 0xA0-0xA1: Slave PIC
 *   - 0x40-0x43: PIT (timer)
 *   - 0x60:      Keyboard data
 *   - 0x64:      Keyboard status/command
 *   - 0x3D4-0x3D5: VGA CRTC (cursor control)
 *   - 0x3F8:     COM1 serial
 */

/*
 * outb - Write a byte to an I/O port
 *
 * @port: I/O port address (0-65535)
 * @value: Byte value to write
 */
static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

/*
 * inb - Read a byte from an I/O port
 *
 * @port: I/O port address (0-65535)
 *
 * Returns: Byte value read from port
 */
static inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

/*
 * outw - Write a 16-bit word to an I/O port
 *
 * @port: I/O port address (0-65535)
 * @value: Word value to write
 */
static inline void outw(uint16_t port, uint16_t value)
{
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

/*
 * inw - Read a 16-bit word from an I/O port
 *
 * @port: I/O port address (0-65535)
 *
 * Returns: Word value read from port
 */
static inline uint16_t inw(uint16_t port)
{
    uint16_t value;
    __asm__ volatile ("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

/*
 * outl - Write a 32-bit dword to an I/O port
 *
 * @port: I/O port address (0-65535)
 * @value: Dword value to write
 */
static inline void outl(uint16_t port, uint32_t value)
{
    __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

/*
 * inl - Read a 32-bit dword from an I/O port
 *
 * @port: I/O port address (0-65535)
 *
 * Returns: Dword value read from port
 */
static inline uint32_t inl(uint16_t port)
{
    uint32_t value;
    __asm__ volatile ("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

/*
 * io_wait - Brief I/O delay
 *
 * Some hardware requires a small delay between I/O operations.
 * Writing to port 0x80 (POST diagnostic port) is a common technique
 * as it takes a known amount of time (~1 microsecond).
 */
static inline void io_wait(void)
{
    outb(0x80, 0);
}

/*
 * =============================================================================
 * CPU Control
 * =============================================================================
 */

/*
 * cli - Clear interrupt flag (disable interrupts)
 */
static inline void cli(void)
{
    __asm__ volatile ("cli");
}

/*
 * sti - Set interrupt flag (enable interrupts)
 */
static inline void sti(void)
{
    __asm__ volatile ("sti");
}

/*
 * hlt - Halt the CPU until next interrupt
 */
static inline void hlt(void)
{
    __asm__ volatile ("hlt");
}

#endif /* KERNEL_INCLUDE_ASM_H */
