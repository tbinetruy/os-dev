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

/*
 * read_eflags - Read the EFLAGS register
 *
 * Returns: Current value of EFLAGS
 */
static inline uint32_t read_eflags(void)
{
    uint32_t eflags;
    __asm__ volatile ("pushfl; popl %0" : "=r"(eflags));
    return eflags;
}

/*
 * write_eflags - Write to the EFLAGS register
 *
 * @eflags: Value to write to EFLAGS
 */
static inline void write_eflags(uint32_t eflags)
{
    __asm__ volatile ("pushl %0; popfl" : : "r"(eflags) : "cc");
}

/*
 * interrupts_enabled - Check if interrupts are enabled
 *
 * Returns: true if IF flag is set, false otherwise
 */
static inline bool interrupts_enabled(void)
{
    return (read_eflags() & 0x200) != 0;
}

/*
 * =============================================================================
 * Control Registers (Paging - Story 3.2)
 * =============================================================================
 */

/*
 * read_cr0 - Read CR0 control register
 *
 * CR0 contains system control flags:
 *   - Bit 0 (PE): Protection Enable
 *   - Bit 31 (PG): Paging Enable
 *
 * Returns: Current value of CR0
 */
static inline uint32_t read_cr0(void)
{
    uint32_t val;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(val));
    return val;
}

/*
 * write_cr0 - Write CR0 control register
 *
 * @val: Value to write to CR0
 *
 * WARNING: Changing CR0 can cause triple faults if done incorrectly.
 */
static inline void write_cr0(uint32_t val)
{
    __asm__ volatile ("mov %0, %%cr0" : : "r"(val) : "memory");
}

/*
 * read_cr2 - Read CR2 (page fault linear address)
 *
 * CR2 holds the linear address that caused the most recent page fault.
 * Used by the page fault handler (Story 3.3).
 *
 * Returns: Faulting address from last page fault
 */
static inline uint32_t read_cr2(void)
{
    uint32_t val;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(val));
    return val;
}

/*
 * read_cr3 - Read CR3 (page directory base register)
 *
 * CR3 holds the physical address of the current page directory.
 *
 * Returns: Physical address of page directory
 */
static inline uint32_t read_cr3(void)
{
    uint32_t val;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(val));
    return val;
}

/*
 * write_cr3 - Write CR3 (page directory base register)
 *
 * Loading CR3 also flushes the entire TLB (except global pages).
 * Used to switch address spaces.
 *
 * @val: Physical address of page directory (must be 4KB aligned)
 */
static inline void write_cr3(uint32_t val)
{
    __asm__ volatile ("mov %0, %%cr3" : : "r"(val) : "memory");
}

/*
 * invlpg - Invalidate TLB entry for a specific address
 *
 * Invalidates the TLB entry for the page containing the given address.
 * More efficient than reloading CR3 when only one page changed.
 *
 * @addr: Virtual address whose TLB entry should be invalidated
 */
static inline void invlpg(uint32_t addr)
{
    __asm__ volatile ("invlpg (%0)" : : "r"(addr) : "memory");
}

#endif /* KERNEL_INCLUDE_ASM_H */
