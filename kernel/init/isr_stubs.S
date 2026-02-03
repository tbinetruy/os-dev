/*
 * kernel/init/isr.S - Interrupt Service Routine stubs
 *
 * Contains assembly stubs for CPU exceptions (0-31). Each stub:
 *   1. Pushes a dummy error code (if CPU doesn't push one)
 *   2. Pushes the interrupt number
 *   3. Jumps to isr_common
 *
 * The common handler saves all registers and calls the C handler
 * (isr_handler), then restores registers and returns from interrupt.
 *
 * Exceptions that push error codes: 8, 10, 11, 12, 13, 14, 17
 * All others need a dummy error code pushed for stack alignment.
 *
 * References:
 *   - Intel SDM Vol 3, Section 6.13 (Error Codes)
 *   - Intel SDM Vol 3, Section 6.12 (Exception and Interrupt Handling)
 */

.code32
.section .text

/*
 * Macro for exceptions WITHOUT error code
 *
 * CPU does not push an error code for these exceptions, so we push
 * a dummy value (0) to maintain consistent stack layout.
 */
.macro ISR_NOERRCODE num
.global isr\num
.type isr\num, @function
isr\num:
    pushl $0            /* Dummy error code */
    pushl $\num         /* Interrupt number */
    jmp isr_common
.size isr\num, . - isr\num
.endm

/*
 * Macro for exceptions WITH error code
 *
 * CPU already pushed error code, so we just push the interrupt number.
 */
.macro ISR_ERRCODE num
.global isr\num
.type isr\num, @function
isr\num:
    /* Error code already on stack from CPU */
    pushl $\num         /* Interrupt number */
    jmp isr_common
.size isr\num, . - isr\num
.endm

/*
 * Exception stubs (0-31)
 *
 * Exceptions with error codes: 8, 10, 11, 12, 13, 14, 17
 * (Double Fault, Invalid TSS, Segment Not Present, Stack Fault,
 *  General Protection, Page Fault, Alignment Check)
 */

/* 0: Division Error (#DE) - no error code */
ISR_NOERRCODE 0

/* 1: Debug (#DB) - no error code */
ISR_NOERRCODE 1

/* 2: NMI - no error code */
ISR_NOERRCODE 2

/* 3: Breakpoint (#BP) - no error code */
ISR_NOERRCODE 3

/* 4: Overflow (#OF) - no error code */
ISR_NOERRCODE 4

/* 5: Bound Range Exceeded (#BR) - no error code */
ISR_NOERRCODE 5

/* 6: Invalid Opcode (#UD) - no error code */
ISR_NOERRCODE 6

/* 7: Device Not Available (#NM) - no error code */
ISR_NOERRCODE 7

/* 8: Double Fault (#DF) - HAS error code (always 0) */
ISR_ERRCODE 8

/* 9: Coprocessor Segment Overrun (legacy) - no error code */
ISR_NOERRCODE 9

/* 10: Invalid TSS (#TS) - HAS error code */
ISR_ERRCODE 10

/* 11: Segment Not Present (#NP) - HAS error code */
ISR_ERRCODE 11

/* 12: Stack-Segment Fault (#SS) - HAS error code */
ISR_ERRCODE 12

/* 13: General Protection (#GP) - HAS error code */
ISR_ERRCODE 13

/* 14: Page Fault (#PF) - HAS error code */
ISR_ERRCODE 14

/* 15: Reserved - no error code */
ISR_NOERRCODE 15

/* 16: x87 FPU Error (#MF) - no error code */
ISR_NOERRCODE 16

/* 17: Alignment Check (#AC) - HAS error code */
ISR_ERRCODE 17

/* 18: Machine Check (#MC) - no error code */
ISR_NOERRCODE 18

/* 19: SIMD Exception (#XM) - no error code */
ISR_NOERRCODE 19

/* 20: Virtualization Exception (#VE) - no error code */
ISR_NOERRCODE 20

/* 21-31: Reserved - no error code */
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

/*
 * isr_common - Common interrupt handler stub
 *
 * Purpose:
 *   Save all CPU state, call C handler, restore state, and return.
 *
 * Stack on entry (from high to low address):
 *   [SS, ESP]     (only on privilege change)
 *   EFLAGS
 *   CS
 *   EIP
 *   Error Code    (real or dummy)
 *   Interrupt #   (pushed by ISR stub)
 *
 * We then push:
 *   General regs  (via pushal)
 *   Segment regs  (DS, ES, FS, GS)
 *
 * After all pushes, ESP points to a struct registers.
 *
 * Clobbers:
 *   None (all registers restored)
 */
.global isr_common
.type isr_common, @function
isr_common:
    /*
     * Save general-purpose registers
     *
     * pushal pushes in order: EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
     * The ESP value pushed is the value BEFORE pushal.
     */
    pushal

    /*
     * Save segment registers
     *
     * We push DS, ES, FS, GS. The stack now contains struct registers.
     */
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs

    /*
     * Load kernel data segment into all data segment registers
     *
     * We need kernel data segment (0x10) to access kernel data.
     * This is important if interrupt occurred in user mode.
     */
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    /*
     * Call C handler
     *
     * Pass pointer to struct registers as argument.
     * The struct starts at current ESP.
     */
    pushl %esp          /* Push pointer to registers struct */
    call isr_handler    /* Call C handler */
    addl $4, %esp       /* Clean up argument */

    /*
     * Restore segment registers
     */
    popl %gs
    popl %fs
    popl %es
    popl %ds

    /*
     * Restore general-purpose registers
     */
    popal

    /*
     * Remove interrupt number and error code from stack
     *
     * These were pushed by the ISR stub (and CPU for error code).
     * Each is 4 bytes, so we add 8 to ESP.
     */
    addl $8, %esp

    /*
     * Return from interrupt
     *
     * iret pops EIP, CS, EFLAGS (and ESP, SS if privilege change).
     */
    iret

.size isr_common, . - isr_common

/*
 * =============================================================================
 * IRQ Stubs (Hardware Interrupts)
 * =============================================================================
 *
 * IRQs are hardware interrupts from external devices (timer, keyboard, etc.).
 * After PIC remapping (Story 2.2):
 *   IRQ 0-7  → INT 32-39 (master PIC)
 *   IRQ 8-15 → INT 40-47 (slave PIC)
 *
 * IRQs never have error codes, but we push a dummy 0 for consistent
 * stack layout with exceptions.
 */

/*
 * Macro for IRQ stubs
 *
 * @num:     IRQ number (0-15)
 * @int_num: Interrupt number after remapping (32-47)
 */
.macro IRQ num, int_num
.global irq\num
.type irq\num, @function
irq\num:
    pushl $0            /* Dummy error code */
    pushl $\int_num     /* Interrupt number */
    jmp irq_common
.size irq\num, . - irq\num
.endm

/* IRQ 0-7: Master PIC (mapped to INT 32-39) */
IRQ 0, 32       /* Timer */
IRQ 1, 33       /* Keyboard */
IRQ 2, 34       /* Cascade (slave PIC) */
IRQ 3, 35       /* COM2 */
IRQ 4, 36       /* COM1 */
IRQ 5, 37       /* LPT2 / Sound */
IRQ 6, 38       /* Floppy */
IRQ 7, 39       /* LPT1 / Spurious */

/* IRQ 8-15: Slave PIC (mapped to INT 40-47) */
IRQ 8, 40       /* RTC */
IRQ 9, 41       /* ACPI / Legacy IRQ2 */
IRQ 10, 42      /* Available */
IRQ 11, 43      /* Available */
IRQ 12, 44      /* PS/2 Mouse */
IRQ 13, 45      /* FPU */
IRQ 14, 46      /* Primary ATA */
IRQ 15, 47      /* Secondary ATA */

/*
 * irq_common - Common IRQ handler stub
 *
 * Purpose:
 *   Save all CPU state, call C handler, restore state, and return.
 *   Similar to isr_common but calls irq_handler instead.
 *
 * The stack layout is identical to exceptions:
 *   [SS, ESP]     (only on privilege change)
 *   EFLAGS
 *   CS
 *   EIP
 *   Error Code    (dummy 0)
 *   Interrupt #   (32-47)
 *
 * After our pushes:
 *   General regs  (via pushal)
 *   Segment regs  (DS, ES, FS, GS)
 *
 * Clobbers:
 *   None (all registers restored)
 */
.global irq_common
.type irq_common, @function
irq_common:
    /*
     * Save general-purpose registers
     */
    pushal

    /*
     * Save segment registers
     */
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs

    /*
     * Load kernel data segment
     */
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    /*
     * Call C handler
     *
     * Pass pointer to struct registers.
     */
    pushl %esp
    call irq_handler
    addl $4, %esp

    /*
     * Restore segment registers
     */
    popl %gs
    popl %fs
    popl %es
    popl %ds

    /*
     * Restore general-purpose registers
     */
    popal

    /*
     * Remove interrupt number and error code from stack
     */
    addl $8, %esp

    /*
     * Return from interrupt
     */
    iret

.size irq_common, . - irq_common
