/*
 * kernel/init/pic.c - 8259 PIC driver implementation
 *
 * Initializes the dual 8259 Programmable Interrupt Controllers and
 * remaps IRQs to avoid conflicts with CPU exceptions.
 *
 * The 8259 PIC requires a specific initialization sequence (ICW1-4)
 * to configure interrupt vector offsets, cascade mode, and operating mode.
 *
 * After initialization:
 *   - IRQ 0-7  (master) → INT 32-39
 *   - IRQ 8-15 (slave)  → INT 40-47
 *   - All IRQs masked (disabled) until explicitly enabled
 *
 * References:
 *   - Intel 8259A Datasheet
 *   - Intel SDM Vol 3, Section 10.8 (APIC/8259 interactions)
 *   - OSDev Wiki: 8259 PIC
 */

#include <pic.h>
#include <asm.h>
#include <printk.h>

/*
 * pic_init - Initialize both PICs with remapped vectors
 *
 * Performs the complete ICW1-ICW4 initialization sequence on both
 * master and slave PICs. The io_wait() calls provide the small delay
 * required between consecutive I/O operations to the PIC.
 *
 * Initialization sequence for each PIC:
 *   1. ICW1 to command port: Initialize, expect ICW4
 *   2. ICW2 to data port: Vector offset (0x20 or 0x28)
 *   3. ICW3 to data port: Cascade configuration
 *   4. ICW4 to data port: 8086 mode
 *   5. Mask all IRQs via data port
 */
void pic_init(void)
{
    /*
     * ICW1: Start initialization sequence
     *
     * Bit 4 (0x10): ICW1 indicator (required)
     * Bit 0 (0x01): ICW4 will be sent
     *
     * Writing to command port with bit 4 set begins initialization.
     */
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    /*
     * ICW2: Set interrupt vector offsets
     *
     * Master PIC: IRQ 0-7 → INT 0x20-0x27 (32-39)
     * Slave PIC:  IRQ 8-15 → INT 0x28-0x2F (40-47)
     *
     * These offsets avoid conflicts with CPU exceptions (INT 0-31).
     */
    outb(PIC1_DATA, PIC1_OFFSET);
    io_wait();
    outb(PIC2_DATA, PIC2_OFFSET);
    io_wait();

    /*
     * ICW3: Configure cascade mode
     *
     * Master: Bit mask indicating which IRQ lines have slaves.
     *         We set bit 2 (0x04) because slave is on IRQ 2.
     *
     * Slave:  Slave identification number (which IRQ line on master).
     *         We set 2 (0x02) for IRQ 2.
     */
    outb(PIC1_DATA, ICW3_MASTER);
    io_wait();
    outb(PIC2_DATA, ICW3_SLAVE);
    io_wait();

    /*
     * ICW4: Set operating mode
     *
     * Bit 0 (0x01): 8086/88 mode (required for x86)
     *
     * Other bits (auto-EOI, buffered mode, etc.) left at 0.
     */
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    /*
     * Mask all IRQs initially
     *
     * Write 0xFF to data port (OCW1) to disable all IRQs.
     * Individual IRQs are enabled later as drivers initialize.
     */
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);

    printk(LOG_DEBUG, "PIC: remapped IRQ 0-7→INT 32-39, IRQ 8-15→INT 40-47\n");
}

/*
 * pic_send_eoi - Send End-of-Interrupt command
 *
 * After handling an IRQ, we must acknowledge it by sending EOI.
 * For slave PIC IRQs (8-15), we must send EOI to both PICs because
 * the slave is cascaded through the master's IRQ 2.
 *
 * @irq: IRQ number (0-15)
 */
void pic_send_eoi(uint8_t irq)
{
    /*
     * If IRQ came from slave PIC (IRQ 8-15), send EOI to slave first.
     * Then always send EOI to master (since slave cascades through master).
     */
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

/*
 * pic_set_mask - Mask (disable) a specific IRQ
 *
 * Sets the corresponding bit in the Interrupt Mask Register (IMR)
 * to prevent the IRQ from generating interrupts.
 *
 * The IMR is accessed via the data port (PIC1_DATA or PIC2_DATA).
 * Setting a bit masks (disables) that IRQ.
 *
 * @irq: IRQ number (0-15)
 */
void pic_set_mask(uint8_t irq)
{
    uint16_t port;
    uint8_t value;

    if (irq >= 16) {
        printk(LOG_WARN, "PIC: invalid IRQ %d for set_mask\n", irq);
        return;
    }

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    value = inb(port) | (1 << irq);
    outb(port, value);
}

/*
 * pic_clear_mask - Unmask (enable) a specific IRQ
 *
 * Clears the corresponding bit in the Interrupt Mask Register (IMR)
 * to allow the IRQ to generate interrupts.
 *
 * For slave PIC IRQs (8-15), also ensures the cascade line (IRQ 2)
 * on the master PIC is unmasked, since slave interrupts route through it.
 *
 * @irq: IRQ number (0-15)
 */
void pic_clear_mask(uint8_t irq)
{
    uint16_t port;
    uint8_t value;

    if (irq >= 16) {
        printk(LOG_WARN, "PIC: invalid IRQ %d for clear_mask\n", irq);
        return;
    }

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        /*
         * For slave PIC IRQs, also unmask the cascade line (IRQ 2)
         * on the master PIC to allow slave interrupts through.
         */
        value = inb(PIC1_DATA) & ~(1 << IRQ_CASCADE);
        outb(PIC1_DATA, value);

        port = PIC2_DATA;
        irq -= 8;
    }

    value = inb(port) & ~(1 << irq);
    outb(port, value);
}
