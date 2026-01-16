/*
 * kernel/include/gdt.h - Global Descriptor Table definitions
 *
 * Defines GDT structures and segment selectors per Intel SDM Vol 3,
 * Chapter 3 (Protected-Mode Memory Management), Section 3.4.5.
 *
 * The GDT contains segment descriptors that define memory segments
 * for protected mode operation. Each descriptor is 8 bytes with a
 * complex layout for historical x86 compatibility reasons.
 *
 * Our GDT layout (6 entries):
 *   Index 0 (0x00): Null descriptor (required)
 *   Index 1 (0x08): Kernel code segment (ring 0)
 *   Index 2 (0x10): Kernel data segment (ring 0)
 *   Index 3 (0x18): User code segment (ring 3) - placeholder
 *   Index 4 (0x20): User data segment (ring 3) - placeholder
 *   Index 5 (0x28): TSS descriptor - placeholder
 */

#ifndef KERNEL_INCLUDE_GDT_H
#define KERNEL_INCLUDE_GDT_H

#include <types.h>

/*
 * Segment Selectors
 *
 * Selector format: [Index (13 bits)][TI (1 bit)][RPL (2 bits)]
 * - Index: GDT entry index (entry number * 8)
 * - TI: Table Indicator (0 = GDT, 1 = LDT)
 * - RPL: Requested Privilege Level (0-3)
 *
 * For GDT selectors, TI=0, so selector = index * 8 + RPL
 * Kernel selectors use RPL=0, user selectors use RPL=3.
 */
#define KERNEL_CS   0x08    /* Kernel code: index 1, RPL 0 */
#define KERNEL_DS   0x10    /* Kernel data: index 2, RPL 0 */
#define USER_CS     0x1B    /* User code:   index 3, RPL 3 (0x18 | 3) */
#define USER_DS     0x23    /* User data:   index 4, RPL 3 (0x20 | 3) */
#define TSS_SEG     0x28    /* TSS:         index 5, RPL 0 */

/*
 * struct gdt_entry - GDT segment descriptor (8 bytes)
 *
 * Intel SDM Vol 3, Figure 3-8 (Segment Descriptor).
 * The structure is packed to prevent compiler padding.
 *
 * Memory layout (little-endian):
 *   Bytes 0-1: Limit[15:0]
 *   Bytes 2-3: Base[15:0]
 *   Byte 4:    Base[23:16]
 *   Byte 5:    Access byte (P, DPL, S, Type)
 *   Byte 6:    Flags[7:4] | Limit[19:16]
 *   Byte 7:    Base[31:24]
 *
 * Access byte format:
 *   Bit 7:    P   - Present (1 = valid segment)
 *   Bits 6-5: DPL - Descriptor Privilege Level (0 = kernel, 3 = user)
 *   Bit 4:    S   - Descriptor type (1 = code/data, 0 = system)
 *   Bit 3:    E   - Executable (1 = code, 0 = data)
 *   Bit 2:    DC  - Direction/Conforming
 *   Bit 1:    RW  - Readable (code) / Writable (data)
 *   Bit 0:    A   - Accessed (set by CPU)
 *
 * Flags (upper nibble of byte 6):
 *   Bit 7 (G):  Granularity (0 = byte, 1 = 4KB pages)
 *   Bit 6 (DB): Size (0 = 16-bit, 1 = 32-bit)
 *   Bit 5 (L):  Long mode (0 for 32-bit protected mode)
 *   Bit 4:     Available for OS use
 */
struct gdt_entry {
    uint16_t limit_low;     /* Segment limit bits 0-15 */
    uint16_t base_low;      /* Base address bits 0-15 */
    uint8_t  base_middle;   /* Base address bits 16-23 */
    uint8_t  access;        /* Access flags (P, DPL, S, Type) */
    uint8_t  granularity;   /* Flags (G, DB, L) | Limit bits 16-19 */
    uint8_t  base_high;     /* Base address bits 24-31 */
} __attribute__((packed));

/*
 * struct gdt_ptr - GDT pointer for LGDT instruction (6 bytes)
 *
 * This structure is loaded into GDTR register via LGDT instruction.
 * Must be packed to ensure 6-byte layout.
 */
struct gdt_ptr {
    uint16_t limit;         /* Size of GDT in bytes minus 1 */
    uint32_t base;          /* Linear address of GDT */
} __attribute__((packed));

/*
 * gdt_init - Initialize the Global Descriptor Table
 *
 * Sets up the kernel's GDT with all required entries and loads it
 * into the GDTR. After this call, all segment registers will be
 * reloaded with the new selectors.
 *
 * Must be called early in kernel initialization, before any code
 * that depends on proper segment setup (e.g., interrupt handlers).
 */
void gdt_init(void);

/*
 * gdt_set_gate - Set a GDT descriptor entry
 *
 * Fills in a GDT entry with the given parameters. Exposed for
 * host-side testing of the encoding logic.
 *
 * @entry:  Pointer to GDT entry to fill
 * @base:   Segment base address (32 bits)
 * @limit:  Segment limit (20 bits)
 * @access: Access byte (P, DPL, S, type fields)
 * @flags:  Upper 4 bits of granularity (G, D/B, L, AVL)
 */
void gdt_set_gate(struct gdt_entry *entry, uint32_t base, uint32_t limit,
                  uint8_t access, uint8_t flags);

#endif /* KERNEL_INCLUDE_GDT_H */
