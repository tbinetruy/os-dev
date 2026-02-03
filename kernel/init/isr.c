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
    uint32_t faulting_addr;

    /*
     * CRITICAL: Read CR2 FIRST
     *
     * CR2 contains the linear address that caused the page fault.
     * Any memory access (including function calls) could potentially
     * cause another page fault and overwrite CR2.
     */
    __asm__ volatile ("movl %%cr2, %0" : "=r"(faulting_addr));

    /* Parse error code bits */
    bool present = (regs->err_code & 0x1) != 0;
    bool write   = (regs->err_code & 0x2) != 0;
    bool user    = (regs->err_code & 0x4) != 0;
    bool rsvd    = (regs->err_code & 0x8) != 0;
    bool ifetch  = (regs->err_code & 0x10) != 0;

    printk(LOG_ERROR, "\n");
    printk(LOG_ERROR, "========================================\n");
    printk(LOG_ERROR, "EXCEPTION: Page Fault (#PF)\n");
    printk(LOG_ERROR, "========================================\n");
    printk(LOG_ERROR, "Faulting address: 0x%x\n", faulting_addr);
    printk(LOG_ERROR, "Error code: 0x%x\n", regs->err_code);
    printk(LOG_ERROR, "  %s\n", present ? "Protection violation" : "Page not present");
    printk(LOG_ERROR, "  %s access\n", write ? "Write" : "Read");
    printk(LOG_ERROR, "  %s mode\n", user ? "User" : "Kernel");
    if (rsvd) {
        printk(LOG_ERROR, "  Reserved bit violation\n");
    }
    if (ifetch) {
        printk(LOG_ERROR, "  Instruction fetch (NX violation)\n");
    }
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

    panic("Page fault in kernel");
}

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
