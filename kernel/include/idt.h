/*
 * kernel/include/idt.h - Interrupt Descriptor Table definitions
 *
 * Defines IDT structures and constants per Intel SDM Vol 3,
 * Chapter 6 (Interrupt and Exception Handling).
 *
 * The IDT contains gate descriptors that define handlers for:
 *   - CPU exceptions (0-31)
 *   - Hardware interrupts via PIC (32-47)
 *   - Software interrupts (48-255)
 *
 * Our IDT has 256 entries:
 *   0-31:   CPU exceptions (Division Error, Page Fault, etc.)
 *   32-47:  Hardware IRQs (remapped PIC, Story 2.2)
 *   48-255: Software interrupts (syscalls, etc.)
 */

#ifndef KERNEL_INCLUDE_IDT_H
#define KERNEL_INCLUDE_IDT_H

#include <types.h>

/*
 * IDT Gate Types
 *
 * Intel SDM Vol 3, Section 6.11 (IDT Descriptors).
 * For 32-bit protected mode, we use interrupt gates and trap gates.
 *
 * Gate type field (bits 40-43 of descriptor):
 *   0x5: 32-bit task gate
 *   0xE: 32-bit interrupt gate (clears IF)
 *   0xF: 32-bit trap gate (does not clear IF)
 *
 * Full type_attr byte: P(1) | DPL(2) | 0(1) | Type(4)
 *   - P=1 (present)
 *   - DPL=00 (ring 0)
 *   - 0=0 (storage segment bit, always 0 for gates)
 *   - Type=0xE or 0xF
 *
 * 0x8E = 10001110b = Present, Ring 0, Interrupt Gate
 * 0x8F = 10001111b = Present, Ring 0, Trap Gate
 */
#define IDT_GATE_INT32  0x8E    /* 32-bit interrupt gate, ring 0 */
#define IDT_GATE_TRAP32 0x8F    /* 32-bit trap gate, ring 0 */

/*
 * Exception Numbers (Intel defined, 0-31)
 *
 * Intel SDM Vol 3, Section 6.3.1 (External Interrupts).
 * Exceptions 0-31 are reserved by Intel for CPU exceptions.
 */
#define EXC_DIVIDE_ERROR         0   /* #DE - Division by zero */
#define EXC_DEBUG                1   /* #DB - Debug exception */
#define EXC_NMI                  2   /* NMI - Non-maskable interrupt */
#define EXC_BREAKPOINT           3   /* #BP - Breakpoint (INT3) */
#define EXC_OVERFLOW             4   /* #OF - Overflow (INTO) */
#define EXC_BOUND_RANGE          5   /* #BR - BOUND range exceeded */
#define EXC_INVALID_OPCODE       6   /* #UD - Invalid opcode */
#define EXC_DEVICE_NOT_AVAIL     7   /* #NM - Device not available (FPU) */
#define EXC_DOUBLE_FAULT         8   /* #DF - Double fault */
#define EXC_COPROC_OVERRUN       9   /* Coprocessor segment overrun */
#define EXC_INVALID_TSS         10   /* #TS - Invalid TSS */
#define EXC_SEGMENT_NOT_PRESENT 11   /* #NP - Segment not present */
#define EXC_STACK_FAULT         12   /* #SS - Stack-segment fault */
#define EXC_GENERAL_PROTECTION  13   /* #GP - General protection fault */
#define EXC_PAGE_FAULT          14   /* #PF - Page fault */
#define EXC_RESERVED_15         15   /* Reserved */
#define EXC_FPU_ERROR           16   /* #MF - x87 FPU error */
#define EXC_ALIGNMENT_CHECK     17   /* #AC - Alignment check */
#define EXC_MACHINE_CHECK       18   /* #MC - Machine check */
#define EXC_SIMD_ERROR          19   /* #XM - SIMD floating-point */
#define EXC_VIRTUALIZATION      20   /* #VE - Virtualization exception */
/* 21-31 are reserved */

#define IDT_ENTRIES 256         /* Total IDT entries */

/*
 * struct idt_entry - IDT gate descriptor (8 bytes)
 *
 * Intel SDM Vol 3, Figure 6-2 (IDT Gate Descriptors).
 * The structure is packed to prevent compiler padding.
 *
 * Memory layout (little-endian):
 *   Bytes 0-1: Offset[15:0] (low 16 bits of handler address)
 *   Bytes 2-3: Segment Selector (kernel code segment, 0x08)
 *   Byte 4:    Reserved (must be 0)
 *   Byte 5:    Type and attributes (P, DPL, type)
 *   Bytes 6-7: Offset[31:16] (high 16 bits of handler address)
 *
 * Type/Attr byte format:
 *   Bit 7:    P   - Present (1 = valid gate)
 *   Bits 6-5: DPL - Descriptor Privilege Level (0 = kernel, 3 = user)
 *   Bit 4:    0   - Storage segment (always 0 for gates)
 *   Bits 3-0: Type (0xE = interrupt gate, 0xF = trap gate)
 */
struct idt_entry {
    uint16_t offset_low;    /* Offset bits 0-15 */
    uint16_t selector;      /* Code segment selector */
    uint8_t  zero;          /* Reserved, must be 0 */
    uint8_t  type_attr;     /* Type and attributes */
    uint16_t offset_high;   /* Offset bits 16-31 */
} __attribute__((packed));

/*
 * struct idt_ptr - IDT pointer for LIDT instruction (6 bytes)
 *
 * This structure is loaded into IDTR register via LIDT instruction.
 * Must be packed to ensure 6-byte layout.
 */
struct idt_ptr {
    uint16_t limit;         /* Size of IDT in bytes minus 1 */
    uint32_t base;          /* Linear address of IDT */
} __attribute__((packed));

/*
 * idt_init - Initialize the Interrupt Descriptor Table
 *
 * Sets up the kernel's IDT with all exception handlers (0-31)
 * and loads it into the IDTR. Interrupts should remain disabled
 * until the PIC is configured (Story 2.2).
 *
 * Must be called early in kernel initialization, after GDT setup.
 */
void idt_init(void);

/*
 * idt_set_gate - Set an IDT gate descriptor
 *
 * Fills in an IDT entry with the given parameters.
 *
 * @num:       IDT entry number (0-255)
 * @handler:   Linear address of interrupt handler
 * @selector:  Code segment selector (typically 0x08 for kernel CS)
 * @type_attr: Gate type and attributes (e.g., IDT_GATE_INT32)
 */
void idt_set_gate(uint8_t num, uint32_t handler, uint16_t selector,
                  uint8_t type_attr);

#endif /* KERNEL_INCLUDE_IDT_H */
