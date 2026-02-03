/*
 * kernel/include/isr.h - Interrupt Service Routine definitions
 *
 * Defines the interrupt stack frame structure and ISR declarations
 * for handling CPU exceptions and hardware interrupts.
 *
 * When an interrupt fires, the CPU pushes state onto the stack.
 * Our ISR stubs add additional context (exception number, error code)
 * and then the common handler saves all registers before calling
 * the C handler.
 *
 * References:
 *   - Intel SDM Vol 3, Section 6.12 (Exception and Interrupt Handling)
 *   - Intel SDM Vol 3, Section 6.13 (Error Codes)
 */

#ifndef KERNEL_INCLUDE_ISR_H
#define KERNEL_INCLUDE_ISR_H

#include <types.h>

/*
 * struct registers - Complete CPU state during interrupt
 *
 * This structure captures the full CPU register state when an
 * interrupt or exception occurs. The order matches exactly how
 * values are pushed onto the stack by the ISR stub and CPU.
 *
 * Stack layout (from high to low address):
 *
 * [SS]          \
 * [ESP]          } Pushed by CPU only on privilege change (ring 3 -> ring 0)
 * EFLAGS        \
 * CS             } Always pushed by CPU
 * EIP           /
 * [Error Code]  } Pushed by CPU for some exceptions, dummy for others
 * int_no        } Pushed by ISR stub
 * EAX           \
 * ECX            \
 * EDX             \
 * EBX              } Pushed by pushal in isr_common
 * ESP (original)   /
 * EBP             /
 * ESI            /
 * EDI           /
 * DS            \
 * ES             } Pushed by isr_common
 * FS             /
 * GS            /
 *
 * Note: The ESP in pushal is the value before pushal, not useful.
 */
struct registers {
    /* Segment registers (pushed by isr_common) */
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    /* General-purpose registers (from pushal, reverse order) */
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;       /* ESP before pushal (not useful) */
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    /* Pushed by ISR stub */
    uint32_t int_no;    /* Interrupt number */
    uint32_t err_code;  /* Error code (real or dummy 0) */

    /* Pushed by CPU on interrupt */
    uint32_t eip;       /* Instruction pointer at interrupt */
    uint32_t cs;        /* Code segment */
    uint32_t eflags;    /* Flags register */

    /* Pushed by CPU only on privilege change (user -> kernel) */
    uint32_t useresp;   /* User stack pointer */
    uint32_t ss;        /* User stack segment */
};

/*
 * ISR stub declarations (defined in isr.S)
 *
 * These assembly stubs are the actual handlers registered in the IDT.
 * Each stub:
 *   1. Pushes a dummy error code (for exceptions that don't have one)
 *   2. Pushes the interrupt number
 *   3. Jumps to isr_common
 */
extern void isr0(void);     /* #DE - Division Error */
extern void isr1(void);     /* #DB - Debug */
extern void isr2(void);     /* NMI */
extern void isr3(void);     /* #BP - Breakpoint */
extern void isr4(void);     /* #OF - Overflow */
extern void isr5(void);     /* #BR - Bound Range Exceeded */
extern void isr6(void);     /* #UD - Invalid Opcode */
extern void isr7(void);     /* #NM - Device Not Available */
extern void isr8(void);     /* #DF - Double Fault (error code) */
extern void isr9(void);     /* Coprocessor Segment Overrun */
extern void isr10(void);    /* #TS - Invalid TSS (error code) */
extern void isr11(void);    /* #NP - Segment Not Present (error code) */
extern void isr12(void);    /* #SS - Stack-Segment Fault (error code) */
extern void isr13(void);    /* #GP - General Protection (error code) */
extern void isr14(void);    /* #PF - Page Fault (error code) */
extern void isr15(void);    /* Reserved */
extern void isr16(void);    /* #MF - x87 FPU Error */
extern void isr17(void);    /* #AC - Alignment Check (error code) */
extern void isr18(void);    /* #MC - Machine Check */
extern void isr19(void);    /* #XM - SIMD Exception */
extern void isr20(void);    /* #VE - Virtualization Exception */
extern void isr21(void);    /* Reserved */
extern void isr22(void);    /* Reserved */
extern void isr23(void);    /* Reserved */
extern void isr24(void);    /* Reserved */
extern void isr25(void);    /* Reserved */
extern void isr26(void);    /* Reserved */
extern void isr27(void);    /* Reserved */
extern void isr28(void);    /* Reserved */
extern void isr29(void);    /* Reserved */
extern void isr30(void);    /* Reserved */
extern void isr31(void);    /* Reserved */

/*
 * isr_handler - Common C handler for all exceptions
 *
 * Called by isr_common after registers are saved. Dispatches to
 * appropriate exception handler based on interrupt number.
 *
 * @regs: Pointer to saved register state on stack
 */
void isr_handler(struct registers *regs);

/*
 * =============================================================================
 * IRQ Handler Support (Story 2.2)
 * =============================================================================
 */

/*
 * IRQ stub declarations (defined in isr_stubs.S)
 *
 * These assembly stubs handle hardware interrupts from devices.
 * After PIC remapping, IRQs map to INT 32-47.
 */
extern void irq0(void);     /* IRQ 0:  Timer (INT 32) */
extern void irq1(void);     /* IRQ 1:  Keyboard (INT 33) */
extern void irq2(void);     /* IRQ 2:  Cascade (INT 34) */
extern void irq3(void);     /* IRQ 3:  COM2 (INT 35) */
extern void irq4(void);     /* IRQ 4:  COM1 (INT 36) */
extern void irq5(void);     /* IRQ 5:  LPT2 (INT 37) */
extern void irq6(void);     /* IRQ 6:  Floppy (INT 38) */
extern void irq7(void);     /* IRQ 7:  LPT1 (INT 39) */
extern void irq8(void);     /* IRQ 8:  RTC (INT 40) */
extern void irq9(void);     /* IRQ 9:  ACPI (INT 41) */
extern void irq10(void);    /* IRQ 10: Available (INT 42) */
extern void irq11(void);    /* IRQ 11: Available (INT 43) */
extern void irq12(void);    /* IRQ 12: PS/2 Mouse (INT 44) */
extern void irq13(void);    /* IRQ 13: FPU (INT 45) */
extern void irq14(void);    /* IRQ 14: Primary ATA (INT 46) */
extern void irq15(void);    /* IRQ 15: Secondary ATA (INT 47) */

/*
 * IRQ handler function pointer type
 *
 * Device drivers register handlers of this type to handle their IRQs.
 */
typedef void (*irq_handler_t)(struct registers *);

/*
 * irq_register_handler - Register a handler for an IRQ
 *
 * Device drivers call this to register their interrupt handler.
 * When the specified IRQ fires, the handler will be called with
 * the saved register state.
 *
 * @irq:     IRQ number (0-15)
 * @handler: Function to call when IRQ fires
 */
void irq_register_handler(uint8_t irq, irq_handler_t handler);

/*
 * irq_handler - Common C handler for all IRQs
 *
 * Called by irq_common (assembly) after registers are saved.
 * Dispatches to the registered handler for the IRQ, then
 * sends EOI to the PIC.
 *
 * @regs: Pointer to saved register state on stack
 */
void irq_handler(struct registers *regs);

#endif /* KERNEL_INCLUDE_ISR_H */
