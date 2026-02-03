/*
 * kernel/init/idt.c - Interrupt Descriptor Table implementation
 *
 * Initializes the kernel's IDT with gate descriptors for:
 *   - CPU exceptions (0-31)
 *   - Hardware IRQs via PIC (32-47) - configured in Story 2.2
 *   - Software interrupts (48-255) - for future syscalls
 *
 * The IDT replaces the default BIOS interrupt vectors with kernel
 * handlers that can properly diagnose faults and manage interrupts.
 *
 * References:
 *   - Intel SDM Vol 3, Chapter 6: Interrupt and Exception Handling
 *   - Intel SDM Vol 3, Section 6.10: IDT
 */

#include <idt.h>
#include <isr.h>
#include <gdt.h>

/*
 * IDT_ENTRIES is defined in idt.h as 256.
 * We declare the full IDT even though we only fill exceptions initially.
 */
static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr idt_pointer;

/*
 * idt_flush - Load IDT into IDTR register (assembly)
 *
 * Defined inline since it's a single instruction.
 *
 * @idt_ptr: Linear address of idt_ptr structure
 */
static inline void idt_flush(uint32_t idt_ptr)
{
    __asm__ volatile ("lidt (%0)" : : "r"(idt_ptr) : "memory");
}

/*
 * idt_set_gate - Set an IDT gate descriptor
 *
 * Fills in an IDT entry with the given parameters. The descriptor
 * format is documented in Intel SDM Vol 3, Figure 6-2.
 *
 * @num:       IDT entry number (0-255)
 * @handler:   Linear address of interrupt handler
 * @selector:  Code segment selector (typically KERNEL_CS = 0x08)
 * @type_attr: Gate type and attributes (e.g., IDT_GATE_INT32 = 0x8E)
 */
void idt_set_gate(uint8_t num, uint32_t handler, uint16_t selector,
                  uint8_t type_attr)
{
    /* Handler address: split across two 16-bit fields */
    idt[num].offset_low  = (uint16_t)(handler & 0xFFFF);
    idt[num].offset_high = (uint16_t)((handler >> 16) & 0xFFFF);

    /* Segment selector: code segment where handler resides */
    idt[num].selector = selector;

    /* Reserved byte: must be zero */
    idt[num].zero = 0;

    /* Type and attributes: P, DPL, gate type */
    idt[num].type_attr = type_attr;
}

/*
 * idt_init - Initialize the Interrupt Descriptor Table
 *
 * Sets up all exception handlers (0-31) and loads the IDT.
 * Hardware IRQ handlers will be added in Story 2.2 after PIC setup.
 *
 * Exception handlers use interrupt gates (0x8E) which automatically
 * clear the IF flag to prevent nested interrupts during exception
 * handling.
 *
 * Note: Interrupts should remain disabled (IF=0) until the PIC
 * is properly configured. The sti instruction is not called here.
 */
void idt_init(void)
{
    /* Set up IDT pointer for LIDT instruction */
    idt_pointer.limit = (uint16_t)(sizeof(idt) - 1);
    idt_pointer.base  = (uint32_t)&idt;

    /*
     * Register all exception handlers (0-31)
     *
     * All exceptions use:
     *   - KERNEL_CS (0x08) as the code segment selector
     *   - IDT_GATE_INT32 (0x8E) as interrupt gate (clears IF)
     *
     * The isr0-isr31 functions are defined in isr.S
     */

    /* Exception 0: Division Error (#DE) */
    idt_set_gate(0, (uint32_t)isr0, KERNEL_CS, IDT_GATE_INT32);

    /* Exception 1: Debug (#DB) */
    idt_set_gate(1, (uint32_t)isr1, KERNEL_CS, IDT_GATE_INT32);

    /* Exception 2: Non-Maskable Interrupt (NMI) */
    idt_set_gate(2, (uint32_t)isr2, KERNEL_CS, IDT_GATE_INT32);

    /* Exception 3: Breakpoint (#BP) - trap gate to allow single-stepping */
    idt_set_gate(3, (uint32_t)isr3, KERNEL_CS, IDT_GATE_TRAP32);

    /* Exception 4: Overflow (#OF) */
    idt_set_gate(4, (uint32_t)isr4, KERNEL_CS, IDT_GATE_INT32);

    /* Exception 5: Bound Range Exceeded (#BR) */
    idt_set_gate(5, (uint32_t)isr5, KERNEL_CS, IDT_GATE_INT32);

    /* Exception 6: Invalid Opcode (#UD) */
    idt_set_gate(6, (uint32_t)isr6, KERNEL_CS, IDT_GATE_INT32);

    /* Exception 7: Device Not Available (#NM) */
    idt_set_gate(7, (uint32_t)isr7, KERNEL_CS, IDT_GATE_INT32);

    /* Exception 8: Double Fault (#DF) - pushes error code */
    idt_set_gate(8, (uint32_t)isr8, KERNEL_CS, IDT_GATE_INT32);

    /* Exception 9: Coprocessor Segment Overrun (legacy) */
    idt_set_gate(9, (uint32_t)isr9, KERNEL_CS, IDT_GATE_INT32);

    /* Exception 10: Invalid TSS (#TS) - pushes error code */
    idt_set_gate(10, (uint32_t)isr10, KERNEL_CS, IDT_GATE_INT32);

    /* Exception 11: Segment Not Present (#NP) - pushes error code */
    idt_set_gate(11, (uint32_t)isr11, KERNEL_CS, IDT_GATE_INT32);

    /* Exception 12: Stack-Segment Fault (#SS) - pushes error code */
    idt_set_gate(12, (uint32_t)isr12, KERNEL_CS, IDT_GATE_INT32);

    /* Exception 13: General Protection (#GP) - pushes error code */
    idt_set_gate(13, (uint32_t)isr13, KERNEL_CS, IDT_GATE_INT32);

    /* Exception 14: Page Fault (#PF) - pushes error code */
    idt_set_gate(14, (uint32_t)isr14, KERNEL_CS, IDT_GATE_INT32);

    /* Exception 15: Reserved */
    idt_set_gate(15, (uint32_t)isr15, KERNEL_CS, IDT_GATE_INT32);

    /* Exception 16: x87 FPU Error (#MF) */
    idt_set_gate(16, (uint32_t)isr16, KERNEL_CS, IDT_GATE_INT32);

    /* Exception 17: Alignment Check (#AC) - pushes error code */
    idt_set_gate(17, (uint32_t)isr17, KERNEL_CS, IDT_GATE_INT32);

    /* Exception 18: Machine Check (#MC) */
    idt_set_gate(18, (uint32_t)isr18, KERNEL_CS, IDT_GATE_INT32);

    /* Exception 19: SIMD Floating-Point (#XM) */
    idt_set_gate(19, (uint32_t)isr19, KERNEL_CS, IDT_GATE_INT32);

    /* Exception 20: Virtualization Exception (#VE) */
    idt_set_gate(20, (uint32_t)isr20, KERNEL_CS, IDT_GATE_INT32);

    /* Exceptions 21-31: Reserved */
    idt_set_gate(21, (uint32_t)isr21, KERNEL_CS, IDT_GATE_INT32);
    idt_set_gate(22, (uint32_t)isr22, KERNEL_CS, IDT_GATE_INT32);
    idt_set_gate(23, (uint32_t)isr23, KERNEL_CS, IDT_GATE_INT32);
    idt_set_gate(24, (uint32_t)isr24, KERNEL_CS, IDT_GATE_INT32);
    idt_set_gate(25, (uint32_t)isr25, KERNEL_CS, IDT_GATE_INT32);
    idt_set_gate(26, (uint32_t)isr26, KERNEL_CS, IDT_GATE_INT32);
    idt_set_gate(27, (uint32_t)isr27, KERNEL_CS, IDT_GATE_INT32);
    idt_set_gate(28, (uint32_t)isr28, KERNEL_CS, IDT_GATE_INT32);
    idt_set_gate(29, (uint32_t)isr29, KERNEL_CS, IDT_GATE_INT32);
    idt_set_gate(30, (uint32_t)isr30, KERNEL_CS, IDT_GATE_INT32);
    idt_set_gate(31, (uint32_t)isr31, KERNEL_CS, IDT_GATE_INT32);

    /*
     * Register IRQ handlers (32-47)
     *
     * After PIC remapping (Story 2.2):
     *   IRQ 0-7  → INT 32-39 (master PIC)
     *   IRQ 8-15 → INT 40-47 (slave PIC)
     *
     * All use interrupt gates (0x8E) which disable further
     * interrupts while handling.
     */

    /* IRQ 0-7: Master PIC (INT 32-39) */
    idt_set_gate(32, (uint32_t)irq0, KERNEL_CS, IDT_GATE_INT32);   /* Timer */
    idt_set_gate(33, (uint32_t)irq1, KERNEL_CS, IDT_GATE_INT32);   /* Keyboard */
    idt_set_gate(34, (uint32_t)irq2, KERNEL_CS, IDT_GATE_INT32);   /* Cascade */
    idt_set_gate(35, (uint32_t)irq3, KERNEL_CS, IDT_GATE_INT32);   /* COM2 */
    idt_set_gate(36, (uint32_t)irq4, KERNEL_CS, IDT_GATE_INT32);   /* COM1 */
    idt_set_gate(37, (uint32_t)irq5, KERNEL_CS, IDT_GATE_INT32);   /* LPT2 */
    idt_set_gate(38, (uint32_t)irq6, KERNEL_CS, IDT_GATE_INT32);   /* Floppy */
    idt_set_gate(39, (uint32_t)irq7, KERNEL_CS, IDT_GATE_INT32);   /* LPT1 */

    /* IRQ 8-15: Slave PIC (INT 40-47) */
    idt_set_gate(40, (uint32_t)irq8, KERNEL_CS, IDT_GATE_INT32);   /* RTC */
    idt_set_gate(41, (uint32_t)irq9, KERNEL_CS, IDT_GATE_INT32);   /* ACPI */
    idt_set_gate(42, (uint32_t)irq10, KERNEL_CS, IDT_GATE_INT32);  /* Available */
    idt_set_gate(43, (uint32_t)irq11, KERNEL_CS, IDT_GATE_INT32);  /* Available */
    idt_set_gate(44, (uint32_t)irq12, KERNEL_CS, IDT_GATE_INT32);  /* PS/2 Mouse */
    idt_set_gate(45, (uint32_t)irq13, KERNEL_CS, IDT_GATE_INT32);  /* FPU */
    idt_set_gate(46, (uint32_t)irq14, KERNEL_CS, IDT_GATE_INT32);  /* Primary ATA */
    idt_set_gate(47, (uint32_t)irq15, KERNEL_CS, IDT_GATE_INT32);  /* Secondary ATA */

    /* Load the IDT */
    idt_flush((uint32_t)&idt_pointer);
}
