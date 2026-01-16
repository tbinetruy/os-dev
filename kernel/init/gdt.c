/*
 * kernel/init/gdt.c - Global Descriptor Table implementation
 *
 * Initializes the kernel's GDT with segment descriptors for:
 *   - Kernel code/data segments (ring 0)
 *   - User code/data segments (ring 3) - placeholders for Story 5.1
 *   - TSS descriptor - placeholder for Story 5.1
 *
 * The GDT replaces the minimal bootloader GDT with a complete one
 * that includes all segments the kernel will ever need.
 *
 * References:
 *   - Intel SDM Vol 3, Chapter 3: Protected-Mode Memory Management
 *   - Intel SDM Vol 3, Section 3.4.5: Segment Descriptors
 */

#include <gdt.h>

/*
 * The following are only needed for the kernel build (gdt_init).
 * Host-side tests only need gdt_set_gate.
 */
#ifndef HOST_TEST

/*
 * GDT_ENTRIES - Number of GDT entries
 *
 * Our GDT has 6 entries:
 *   0: Null descriptor (required by CPU)
 *   1: Kernel code segment
 *   2: Kernel data segment
 *   3: User code segment (placeholder)
 *   4: User data segment (placeholder)
 *   5: TSS descriptor (placeholder)
 */
#define GDT_ENTRIES 6

/*
 * Static GDT array and pointer
 *
 * These are declared static to keep them in kernel memory and
 * prevent external code from directly modifying the GDT.
 */
static struct gdt_entry gdt[GDT_ENTRIES];
static struct gdt_ptr gdt_pointer;

/*
 * gdt_flush - Load GDT and reload segment registers (assembly)
 *
 * Defined in gdt_flush.S. Loads the GDT pointer into GDTR and
 * reloads all segment registers with the new selectors.
 *
 * @gdt_ptr: Linear address of gdt_ptr structure
 */
extern void gdt_flush(uint32_t gdt_ptr);

#endif /* !HOST_TEST */

/*
 * gdt_set_gate - Set a GDT descriptor entry
 *
 * Fills in a GDT entry with the given parameters. The descriptor
 * format is fragmented for historical x86 reasons (see Intel SDM
 * Vol 3, Figure 3-8).
 *
 * @entry:  Pointer to GDT entry to fill
 * @base:   Segment base address (32 bits)
 * @limit:  Segment limit (20 bits, in bytes or 4KB pages)
 * @access: Access byte containing P, DPL, S, and type fields
 * @flags:  Upper 4 bits of granularity byte (G, D/B, L, AVL)
 *
 * Descriptor byte layout:
 *   Byte 0-1: Limit[15:0]
 *   Byte 2-3: Base[15:0]
 *   Byte 4:   Base[23:16]
 *   Byte 5:   Access (P | DPL | S | Type)
 *   Byte 6:   Flags[3:0] | Limit[19:16]
 *   Byte 7:   Base[31:24]
 */
void gdt_set_gate(struct gdt_entry *entry, uint32_t base, uint32_t limit,
                  uint8_t access, uint8_t flags)
{
    /* Base address: split across 3 fields */
    entry->base_low    = (uint16_t)(base & 0xFFFF);
    entry->base_middle = (uint8_t)((base >> 16) & 0xFF);
    entry->base_high   = (uint8_t)((base >> 24) & 0xFF);

    /* Limit: lower 16 bits in limit_low, upper 4 bits in granularity */
    entry->limit_low   = (uint16_t)(limit & 0xFFFF);

    /* Granularity byte: flags in upper nibble, limit[19:16] in lower nibble */
    entry->granularity = (uint8_t)((limit >> 16) & 0x0F);
    entry->granularity |= (uint8_t)((flags << 4) & 0xF0);

    /* Access byte: written directly */
    entry->access = access;
}

/*
 * gdt_init is only compiled for the kernel, not for host-side tests.
 * Host tests only need gdt_set_gate for testing the encoding logic.
 */
#ifndef HOST_TEST

/*
 * gdt_init - Initialize the Global Descriptor Table
 *
 * Sets up all GDT entries and loads the new GDT. This replaces the
 * minimal bootloader GDT with a complete kernel GDT.
 *
 * GDT Entry Configuration:
 *
 * Entry 0 - Null Descriptor (required)
 *   All zeros. CPU requires first GDT entry to be null.
 *
 * Entry 1 - Kernel Code (selector 0x08)
 *   Access: 0x9A = 10011010b
 *     P=1 (present), DPL=00 (ring 0), S=1 (code/data)
 *     E=1 (executable), DC=0 (not conforming), RW=1 (readable), A=0
 *   Flags: 0xC = 1100b
 *     G=1 (4KB granularity), D=1 (32-bit), L=0 (not 64-bit), AVL=0
 *   Base=0, Limit=0xFFFFF (4GB with 4KB granularity)
 *
 * Entry 2 - Kernel Data (selector 0x10)
 *   Access: 0x92 = 10010010b
 *     P=1, DPL=00, S=1, E=0 (data), DC=0 (grows up), RW=1 (writable), A=0
 *   Flags: 0xC
 *   Base=0, Limit=0xFFFFF (flat 4GB)
 *
 * Entry 3 - User Code (selector 0x18, with RPL=3 -> 0x1B)
 *   Access: 0xFA = 11111010b
 *     P=1, DPL=11 (ring 3), S=1, E=1, DC=0, RW=1, A=0
 *   Flags: 0xC
 *   Placeholder for Story 5.1 (user mode).
 *
 * Entry 4 - User Data (selector 0x20, with RPL=3 -> 0x23)
 *   Access: 0xF2 = 11110010b
 *     P=1, DPL=11, S=1, E=0, DC=0, RW=1, A=0
 *   Flags: 0xC
 *   Placeholder for Story 5.1.
 *
 * Entry 5 - TSS (selector 0x28)
 *   Access: 0x89 = 10001001b
 *     P=1, DPL=00, S=0 (system), Type=1001 (32-bit TSS available)
 *   Flags: 0x0
 *   Placeholder for Story 5.1. Will be filled by tss_init().
 */
void gdt_init(void)
{
    /* Set up GDT pointer for LGDT instruction */
    gdt_pointer.limit = (uint16_t)(sizeof(gdt) - 1);
    gdt_pointer.base  = (uint32_t)&gdt;

    /*
     * Entry 0: Null descriptor
     * Required by x86 architecture. Any reference to selector 0
     * will cause a general protection fault.
     */
    gdt_set_gate(&gdt[0], 0, 0, 0, 0);

    /*
     * Entry 1: Kernel code segment (0x08)
     * Flat 4GB code segment at ring 0.
     * Access 0x9A: Present, Ring 0, Code segment, Executable, Readable
     * Flags 0xC: 4KB granularity, 32-bit protected mode
     */
    gdt_set_gate(&gdt[1], 0, 0xFFFFF, 0x9A, 0xC);

    /*
     * Entry 2: Kernel data segment (0x10)
     * Flat 4GB data segment at ring 0.
     * Access 0x92: Present, Ring 0, Data segment, Writable
     * Flags 0xC: 4KB granularity, 32-bit
     */
    gdt_set_gate(&gdt[2], 0, 0xFFFFF, 0x92, 0xC);

    /*
     * Entry 3: User code segment (0x18)
     * Flat 4GB code segment at ring 3.
     * Access 0xFA: Present, Ring 3, Code segment, Executable, Readable
     * Placeholder - will be used by Story 5.1 for user mode.
     */
    gdt_set_gate(&gdt[3], 0, 0xFFFFF, 0xFA, 0xC);

    /*
     * Entry 4: User data segment (0x20)
     * Flat 4GB data segment at ring 3.
     * Access 0xF2: Present, Ring 3, Data segment, Writable
     * Placeholder - will be used by Story 5.1 for user mode.
     */
    gdt_set_gate(&gdt[4], 0, 0xFFFFF, 0xF2, 0xC);

    /*
     * Entry 5: TSS descriptor (0x28)
     * Will be filled properly by tss_init() in Story 5.1.
     * For now, leave it empty (not present).
     */
    gdt_set_gate(&gdt[5], 0, 0, 0, 0);

    /* Load the GDT and reload segment registers */
    gdt_flush((uint32_t)&gdt_pointer);
}

#endif /* !HOST_TEST */
