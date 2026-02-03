/*
 * kernel/include/pic.h - 8259 Programmable Interrupt Controller
 *
 * Defines constants and functions for controlling the 8259 PIC.
 * The i386 uses two cascaded 8259 PICs:
 *   - Master (PIC1): Handles IRQ 0-7
 *   - Slave (PIC2): Handles IRQ 8-15, connected to master's IRQ 2
 *
 * By default, BIOS maps:
 *   - IRQ 0-7  → INT 0x08-0x0F (conflicts with CPU exceptions!)
 *   - IRQ 8-15 → INT 0x70-0x77
 *
 * We remap to:
 *   - IRQ 0-7  → INT 0x20-0x27 (32-39)
 *   - IRQ 8-15 → INT 0x28-0x2F (40-47)
 *
 * References:
 *   - Intel 8259A Datasheet
 *   - OSDev Wiki: 8259 PIC
 */

#ifndef KERNEL_INCLUDE_PIC_H
#define KERNEL_INCLUDE_PIC_H

#include <types.h>

/*
 * =============================================================================
 * PIC I/O Ports
 * =============================================================================
 */

/* Master PIC (PIC1) ports */
#define PIC1_COMMAND    0x20    /* Command port (write ICW1, OCW2, OCW3) */
#define PIC1_DATA       0x21    /* Data port (write ICW2-4, read/write OCW1) */

/* Slave PIC (PIC2) ports */
#define PIC2_COMMAND    0xA0    /* Command port */
#define PIC2_DATA       0xA1    /* Data port */

/*
 * =============================================================================
 * Initialization Command Words (ICW1-ICW4)
 * =============================================================================
 *
 * PIC initialization requires sending ICW1-ICW4 in sequence.
 * ICW1 goes to command port; ICW2-4 go to data port.
 */

/*
 * ICW1 - Initialization Command Word 1
 *
 * Sent to command port to start initialization sequence.
 * Bits:
 *   Bit 4: 1 = ICW1 (required)
 *   Bit 3: 0 = edge triggered, 1 = level triggered
 *   Bit 2: 0 = 8-byte interrupt vectors, 1 = 4-byte (x86 uses 0)
 *   Bit 1: 0 = cascade mode, 1 = single PIC
 *   Bit 0: 1 = ICW4 needed
 */
#define ICW1_INIT       0x10    /* Initialization bit (required) */
#define ICW1_ICW4       0x01    /* ICW4 needed */

/*
 * ICW2 - Initialization Command Word 2 (Vector Offset)
 *
 * Specifies the base interrupt vector number for IRQs.
 * The vector offset must be divisible by 8.
 */
#define PIC1_OFFSET     0x20    /* IRQ 0-7  → INT 32-39 */
#define PIC2_OFFSET     0x28    /* IRQ 8-15 → INT 40-47 */

/*
 * ICW3 - Initialization Command Word 3 (Cascade Configuration)
 *
 * For master: bitmap indicating which IRQ lines have slaves
 * For slave: slave ID (which IRQ line it's connected to on master)
 */
#define ICW3_MASTER     0x04    /* Slave on IRQ 2 (bit 2 set) */
#define ICW3_SLAVE      0x02    /* Slave ID = 2 */

/*
 * ICW4 - Initialization Command Word 4 (Mode Configuration)
 *
 * Bits:
 *   Bit 4: 0 = not special fully nested, 1 = special fully nested
 *   Bit 3: 0 = not buffered, 1 = buffered
 *   Bit 2: (buffered mode) 0 = slave, 1 = master
 *   Bit 1: 0 = normal EOI, 1 = auto EOI
 *   Bit 0: 0 = MCS-80/85 mode, 1 = 8086/88 mode
 */
#define ICW4_8086       0x01    /* 8086/88 mode (required for x86) */

/*
 * =============================================================================
 * Operation Command Words (OCW)
 * =============================================================================
 */

/*
 * OCW2 - End of Interrupt (EOI) command
 *
 * Sent to command port after handling an IRQ.
 * Non-specific EOI: 0x20
 */
#define PIC_EOI         0x20    /* End of interrupt command */

/*
 * =============================================================================
 * IRQ Numbers
 * =============================================================================
 *
 * Standard PC IRQ assignments:
 *   IRQ 0:  PIT (timer)
 *   IRQ 1:  Keyboard
 *   IRQ 2:  Cascade (slave PIC)
 *   IRQ 3:  COM2
 *   IRQ 4:  COM1
 *   IRQ 5:  LPT2 / Sound card
 *   IRQ 6:  Floppy
 *   IRQ 7:  LPT1 / Spurious
 *   IRQ 8:  RTC
 *   IRQ 9:  ACPI / legacy IRQ 2
 *   IRQ 10: Available
 *   IRQ 11: Available
 *   IRQ 12: PS/2 Mouse
 *   IRQ 13: FPU
 *   IRQ 14: Primary ATA
 *   IRQ 15: Secondary ATA
 */
#define IRQ_TIMER       0
#define IRQ_KEYBOARD    1
#define IRQ_CASCADE     2
#define IRQ_COM2        3
#define IRQ_COM1        4
#define IRQ_LPT2        5
#define IRQ_FLOPPY      6
#define IRQ_LPT1        7
#define IRQ_RTC         8
#define IRQ_ACPI        9
#define IRQ_AVAILABLE1  10
#define IRQ_AVAILABLE2  11
#define IRQ_PS2_MOUSE   12
#define IRQ_FPU         13
#define IRQ_ATA_PRIMARY 14
#define IRQ_ATA_SECONDARY 15

/*
 * =============================================================================
 * Function Declarations
 * =============================================================================
 */

/*
 * pic_init - Initialize both PICs with remapped IRQ vectors
 *
 * Performs the ICW1-ICW4 initialization sequence on both PICs:
 *   1. Send ICW1 (init + ICW4 needed)
 *   2. Send ICW2 (vector offsets: 0x20 for master, 0x28 for slave)
 *   3. Send ICW3 (cascade configuration)
 *   4. Send ICW4 (8086 mode)
 *   5. Mask all IRQs initially
 *
 * After this call:
 *   - IRQ 0-7 map to INT 32-39
 *   - IRQ 8-15 map to INT 40-47
 *   - All IRQs are masked (disabled)
 *
 * Call this after IDT is initialized but before enabling interrupts.
 */
void pic_init(void);

/*
 * pic_send_eoi - Send End-of-Interrupt to PIC
 *
 * Must be called after handling any IRQ to acknowledge the interrupt.
 * For IRQs 8-15 (slave PIC), EOI must be sent to both slave and master.
 *
 * @irq: IRQ number (0-15)
 */
void pic_send_eoi(uint8_t irq);

/*
 * pic_set_mask - Mask (disable) an IRQ
 *
 * Sets the corresponding bit in the PIC's IMR (Interrupt Mask Register)
 * to prevent the IRQ from generating interrupts.
 *
 * @irq: IRQ number (0-15)
 */
void pic_set_mask(uint8_t irq);

/*
 * pic_clear_mask - Unmask (enable) an IRQ
 *
 * Clears the corresponding bit in the PIC's IMR to allow the IRQ
 * to generate interrupts.
 *
 * @irq: IRQ number (0-15)
 */
void pic_clear_mask(uint8_t irq);

#endif /* KERNEL_INCLUDE_PIC_H */
