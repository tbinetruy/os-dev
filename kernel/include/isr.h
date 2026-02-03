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

#endif /* KERNEL_INCLUDE_ISR_H */
