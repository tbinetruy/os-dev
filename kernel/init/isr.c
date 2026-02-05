/*
 * kernel/init/isr.c - Interrupt Service Routine C handlers
 *
 * Implements the C-side exception handling. Called by isr_common (isr.S)
 * after registers are saved. Prints diagnostic information and halts
 * for unrecoverable exceptions.
 *
 * Special handling:
 *   - Page Fault (#PF, 14): Reads CR2 for faulting address
 *   - General Protection (#GP, 13): Often indicates privilege violations
 *
 * References:
 *   - Intel SDM Vol 3, Section 6.15 (Exception and Interrupt Reference)
 *   - Intel SDM Vol 3, Section 6.13 (Error Codes)
 */

#include <isr.h>
#include <idt.h>
#include <pic.h>
#include <asm.h>
#include <printk.h>
#include <panic.h>

/*
 * Exception names for diagnostic output
 *
 * Index corresponds to exception number (0-31).
 * Names follow Intel SDM conventions.
 */
static const char *exception_names[32] = {
    "Division Error",           /* 0: #DE */
    "Debug",                    /* 1: #DB */
    "NMI Interrupt",            /* 2: NMI */
    "Breakpoint",               /* 3: #BP */
    "Overflow",                 /* 4: #OF */
    "Bound Range Exceeded",     /* 5: #BR */
    "Invalid Opcode",           /* 6: #UD */
    "Device Not Available",     /* 7: #NM */
    "Double Fault",             /* 8: #DF */
    "Coprocessor Segment Overrun", /* 9: legacy */
    "Invalid TSS",              /* 10: #TS */
    "Segment Not Present",      /* 11: #NP */
    "Stack-Segment Fault",      /* 12: #SS */
    "General Protection Fault", /* 13: #GP */
    "Page Fault",               /* 14: #PF */
    "Reserved",                 /* 15 */
    "x87 FPU Error",            /* 16: #MF */
    "Alignment Check",          /* 17: #AC */
    "Machine Check",            /* 18: #MC */
    "SIMD Exception",           /* 19: #XM */
    "Virtualization Exception", /* 20: #VE */
    "Reserved",                 /* 21 */
    "Reserved",                 /* 22 */
    "Reserved",                 /* 23 */
    "Reserved",                 /* 24 */
    "Reserved",                 /* 25 */
    "Reserved",                 /* 26 */
    "Reserved",                 /* 27 */
    "Reserved",                 /* 28 */
    "Reserved",                 /* 29 */
    "Reserved",                 /* 30 */
    "Reserved",                 /* 31 */
};

/*
 * page_fault_handler - Handle page fault exception
 *
 * Reads CR2 (faulting address) and parses the error code to provide
 * detailed diagnostic information. Must read CR2 FIRST before any
 * other memory access that could overwrite it.
 *
 * Error code bits (Intel SDM Vol 3, Section 6.15):
 *   Bit 0 (P):  0 = not-present page, 1 = protection violation
 *   Bit 1 (W):  0 = read access, 1 = write access
 *   Bit 2 (U):  0 = supervisor mode, 1 = user mode
 *   Bit 3 (R):  1 = reserved bit violation
 *   Bit 4 (I):  1 = instruction fetch (NX violation)
 *
 * @regs: Pointer to saved register state
 */
static void page_fault_handler(struct registers *regs)
{
    /*
     * CRITICAL: Read CR2 FIRST
     *
     * CR2 contains the linear address that caused the page fault.
     * Any memory access (including function calls) could potentially
     * cause another page fault and overwrite CR2.
     */
    uint32_t faulting_addr = read_cr2();

    /* Parse error code bits using named constants */
    bool present = (regs->err_code & PF_ERR_PRESENT) != 0;
    bool write   = (regs->err_code & PF_ERR_WRITE) != 0;
    bool user    = (regs->err_code & PF_ERR_USER) != 0;

    printk(LOG_ERROR, "PAGE FAULT at 0x%x\n", faulting_addr);
    printk(LOG_ERROR, "Error: %s %s %s\n",
           write ? "write" : "read",
           user ? "user" : "kernel",
           present ? "present" : "not-present");
    printk(LOG_ERROR, "Faulting EIP: 0x%x\n", regs->eip);

    if (regs->err_code & PF_ERR_RSVD)
        printk(LOG_ERROR, "Reserved bit violation in PTE\n");
    if (regs->err_code & PF_ERR_IFETCH)
        printk(LOG_ERROR, "Instruction fetch fault\n");

    /* Interrupt frame register dump for debugging */
    printk(LOG_ERROR, "CS: 0x%x  EFLAGS: 0x%x\n",
           regs->cs, regs->eflags);
    printk(LOG_ERROR, "EAX: 0x%x  EBX: 0x%x  ECX: 0x%x  EDX: 0x%x\n",
           regs->eax, regs->ebx, regs->ecx, regs->edx);
    printk(LOG_ERROR, "ESP: 0x%x  EBP: 0x%x  ESI: 0x%x  EDI: 0x%x\n",
           regs->esp, regs->ebp, regs->esi, regs->edi);
    printk(LOG_ERROR, "DS: 0x%x  ES: 0x%x  FS: 0x%x  GS: 0x%x\n",
           regs->ds, regs->es, regs->fs, regs->gs);

    if (!user) {
        /* Kernel-mode fault: unrecoverable, panic with context */
        panic("Page fault in kernel");
    } else {
        /*
         * TODO(Epic 5+): User-mode fault handling.
         * For now, user-mode faults also panic since we have
         * no user processes yet.
         */
        panic("Page fault in user mode");
    }
}

#ifdef TEST_MODE

/*
 * Page fault test hook
 *
 * When non-NULL, isr_handler calls this instead of page_fault_handler
 * for page faults. Tests set this to intercept faults without panicking.
 */
static void (*pf_test_hook)(struct registers *) = NULL;

void pf_set_test_hook(void (*hook)(struct registers *))
{
    pf_test_hook = hook;
}

#endif /* TEST_MODE */

/*
 * isr_handler - Common C handler for all exceptions
 *
 * Called by isr_common (isr.S) after all registers are saved.
 * Dispatches to specific handlers or prints generic exception info.
 *
 * @regs: Pointer to struct registers on stack
 */
void isr_handler(struct registers *regs)
{
    /* Only handle CPU exceptions (0-31) */
    if (regs->int_no >= 32) {
        /* Hardware interrupts will be handled in Story 2.2 */
        printk(LOG_WARN, "Unhandled interrupt: %d\n", regs->int_no);
        return;
    }

    /* Page fault gets special handling */
    if (regs->int_no == EXC_PAGE_FAULT) {
#ifdef TEST_MODE
        if (pf_test_hook) {
            pf_test_hook(regs);
            return;
        }
#endif
        page_fault_handler(regs);
        return;
    }

    /* Generic exception handler for all other exceptions */
    const char *name = exception_names[regs->int_no];

    printk(LOG_ERROR, "\n");
    printk(LOG_ERROR, "========================================\n");
    printk(LOG_ERROR, "EXCEPTION: %s (#%d)\n", name, regs->int_no);
    printk(LOG_ERROR, "========================================\n");
    printk(LOG_ERROR, "Error code: 0x%x\n", regs->err_code);
    printk(LOG_ERROR, "\n");
    printk(LOG_ERROR, "EIP: 0x%x  CS: 0x%x\n", regs->eip, regs->cs);
    printk(LOG_ERROR, "EFLAGS: 0x%x\n", regs->eflags);
    printk(LOG_ERROR, "EAX: 0x%x  EBX: 0x%x  ECX: 0x%x  EDX: 0x%x\n",
           regs->eax, regs->ebx, regs->ecx, regs->edx);
    printk(LOG_ERROR, "ESP: 0x%x  EBP: 0x%x  ESI: 0x%x  EDI: 0x%x\n",
           regs->esp, regs->ebp, regs->esi, regs->edi);
    printk(LOG_ERROR, "DS: 0x%x  ES: 0x%x  FS: 0x%x  GS: 0x%x\n",
           regs->ds, regs->es, regs->fs, regs->gs);
    printk(LOG_ERROR, "========================================\n");

    panic("Unhandled CPU exception");
}

/*
 * =============================================================================
 * IRQ Handler Support (Story 2.2)
 * =============================================================================
 */

/*
 * IRQ handler table
 *
 * Device drivers register their handlers here via irq_register_handler().
 * When an IRQ fires, irq_handler() looks up and calls the registered function.
 *
 * Index 0-15 corresponds to IRQ 0-15.
 */
static irq_handler_t irq_handlers[16] = {0};

/*
 * irq_register_handler - Register a handler for an IRQ
 *
 * @irq:     IRQ number (0-15)
 * @handler: Function to call when IRQ fires (NULL to unregister)
 */
void irq_register_handler(uint8_t irq, irq_handler_t handler)
{
    if (irq >= 16) {
        printk(LOG_WARN, "IRQ: invalid IRQ %d for register_handler\n", irq);
        return;
    }
    irq_handlers[irq] = handler;
}

/*
 * pic_read_isr - Read the In-Service Register from PIC
 *
 * Used to detect spurious IRQs. Returns bitmap of IRQs being serviced.
 *
 * @pic_port: PIC1_COMMAND (0x20) or PIC2_COMMAND (0xA0)
 * Returns: ISR bitmap
 */
static inline uint8_t pic_read_isr(uint16_t pic_port)
{
    /* OCW3: Read ISR (0x0B) */
    outb(pic_port, 0x0B);
    return inb(pic_port);
}

/*
 * irq_handler - Common C handler for hardware interrupts
 *
 * Called by irq_common (assembly) after registers are saved.
 * Looks up the registered handler for the IRQ and calls it,
 * then sends End-of-Interrupt to the PIC.
 *
 * Handles spurious IRQs on IRQ 7 and IRQ 15 by checking the ISR.
 *
 * @regs: Pointer to saved register state on stack
 */
void irq_handler(struct registers *regs)
{
    /*
     * Calculate IRQ number from interrupt number
     *
     * After PIC remapping: INT 32-47 = IRQ 0-15
     */
    uint8_t irq = regs->int_no - 32;

    /*
     * Check for spurious IRQs
     *
     * The 8259 PIC can generate spurious interrupts on IRQ 7 (master)
     * and IRQ 15 (slave). We detect these by reading the In-Service
     * Register - if the corresponding bit is not set, it's spurious.
     */
    if (irq == 7) {
        /* Check master PIC ISR for IRQ 7 */
        if ((pic_read_isr(PIC1_COMMAND) & 0x80) == 0) {
            /*
             * Spurious IRQ 7 - do NOT send EOI.
             *
             * Master PIC never actually serviced an interrupt, so there's
             * nothing to acknowledge. Sending EOI could incorrectly clear
             * a different pending interrupt.
             */
            return;
        }
    } else if (irq == 15) {
        /* Check slave PIC ISR for IRQ 15 */
        if ((pic_read_isr(PIC2_COMMAND) & 0x80) == 0) {
            /*
             * Spurious IRQ 15 - send EOI to master only.
             *
             * The slave PIC triggered the cascade line (IRQ 2) on master
             * before the spurious condition was detected. Master registered
             * a real interrupt on IRQ 2 that must be acknowledged, but slave
             * should not receive EOI since it didn't actually service IRQ 15.
             */
            outb(PIC1_COMMAND, PIC_EOI);
            return;
        }
    }

    /*
     * Call registered handler if one exists
     */
    if (irq < 16 && irq_handlers[irq] != NULL) {
        irq_handlers[irq](regs);
    }

    /*
     * Send End-of-Interrupt to PIC
     *
     * Must be done after handling to acknowledge the interrupt.
     * For slave PIC IRQs (8-15), EOI goes to both PICs.
     */
    pic_send_eoi(irq);
}

/*
 * =============================================================================
 * Test Support Functions
 * =============================================================================
 */

#ifdef TEST_MODE

/*
 * irq_has_handler - Check if an IRQ has a registered handler
 *
 * Test helper to verify device drivers correctly register their handlers.
 *
 * @irq: IRQ number (0-15)
 * Returns: true if a handler is registered, false otherwise
 */
bool irq_has_handler(uint8_t irq)
{
    if (irq >= 16) {
        return false;
    }
    return irq_handlers[irq] != NULL;
}

#endif /* TEST_MODE */
