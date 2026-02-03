/*
 * kernel/include/timer.h - PIT Timer driver
 *
 * Defines constants and functions for the 8253/8254 Programmable
 * Interval Timer (PIT). The PIT provides the system timer interrupt
 * (IRQ 0) used for timekeeping and preemptive scheduling.
 *
 * PIT Channels:
 *   - Channel 0: System timer (IRQ 0) - used by this driver
 *   - Channel 1: DRAM refresh (historical, not used)
 *   - Channel 2: PC speaker
 *
 * The PIT operates at a base frequency of 1,193,182 Hz (derived from
 * the 14.31818 MHz PC crystal / 12). We configure it to generate
 * 100 interrupts per second (10ms interval) for the system tick.
 *
 * References:
 *   - Intel 8253/8254 Datasheet
 *   - OSDev Wiki: Programmable Interval Timer
 */

#ifndef KERNEL_INCLUDE_TIMER_H
#define KERNEL_INCLUDE_TIMER_H

#include <types.h>

/*
 * =============================================================================
 * PIT I/O Ports
 * =============================================================================
 */

#define PIT_CHANNEL0    0x40    /* Channel 0 data port */
#define PIT_CHANNEL1    0x41    /* Channel 1 data port (unused) */
#define PIT_CHANNEL2    0x42    /* Channel 2 data port (speaker) */
#define PIT_CMD         0x43    /* Mode/Command register */

/*
 * =============================================================================
 * PIT Configuration Constants
 * =============================================================================
 */

/*
 * PIT base frequency: 1,193,182 Hz
 *
 * The PIT's oscillator runs at 14.31818 MHz / 12 = 1,193,181.8... Hz.
 * Conventionally rounded to 1,193,182 Hz.
 */
#define PIT_FREQUENCY   1193182

/*
 * Target tick rate: 100 Hz (10ms per tick)
 *
 * This provides a good balance between timer resolution and overhead.
 * 100Hz is standard for many Unix-like systems.
 */
#define TARGET_HZ       100

/*
 * PIT divisor for 100 Hz
 *
 * Divisor = PIT_FREQUENCY / TARGET_HZ = 1,193,182 / 100 = 11,931.82
 * Integer division gives 11,931, actual frequency = 1,193,182 / 11,931 ≈ 100.01 Hz
 */
#define PIT_DIVISOR     (PIT_FREQUENCY / TARGET_HZ)

/*
 * =============================================================================
 * PIT Command Register Bits (Port 0x43)
 * =============================================================================
 *
 * Format: [Channel(2)][Access(2)][Mode(3)][BCD(1)]
 */

/*
 * Channel select (bits 7-6)
 */
#define PIT_CMD_CHANNEL0    0x00    /* Select channel 0 */
#define PIT_CMD_CHANNEL1    0x40    /* Select channel 1 */
#define PIT_CMD_CHANNEL2    0x80    /* Select channel 2 */

/*
 * Access mode (bits 5-4)
 */
#define PIT_CMD_LATCH       0x00    /* Latch count value */
#define PIT_CMD_LOBYTE      0x10    /* Access low byte only */
#define PIT_CMD_HIBYTE      0x20    /* Access high byte only */
#define PIT_CMD_LOHI        0x30    /* Access low byte then high byte */

/*
 * Operating mode (bits 3-1)
 */
#define PIT_CMD_MODE0       0x00    /* Interrupt on terminal count */
#define PIT_CMD_MODE1       0x02    /* Hardware retriggerable one-shot */
#define PIT_CMD_MODE2       0x04    /* Rate generator */
#define PIT_CMD_MODE3       0x06    /* Square wave generator */
#define PIT_CMD_MODE4       0x08    /* Software triggered strobe */
#define PIT_CMD_MODE5       0x0A    /* Hardware triggered strobe */

/*
 * BCD/Binary mode (bit 0)
 */
#define PIT_CMD_BINARY      0x00    /* 16-bit binary counter */
#define PIT_CMD_BCD         0x01    /* 4-digit BCD counter */

/*
 * Combined command for timer initialization:
 * Channel 0 + Lo/Hi access + Mode 3 (square wave) + Binary = 0x36
 */
#define PIT_CMD_TIMER       (PIT_CMD_CHANNEL0 | PIT_CMD_LOHI | PIT_CMD_MODE3 | PIT_CMD_BINARY)

/*
 * =============================================================================
 * Function Declarations
 * =============================================================================
 */

/*
 * timer_init - Initialize the PIT timer
 *
 * Configures PIT channel 0 for 100 Hz operation:
 *   1. Write command byte (0x36) to mode/command register
 *   2. Write divisor low byte to channel 0
 *   3. Write divisor high byte to channel 0
 *   4. Register timer interrupt handler for IRQ 0
 *   5. Enable IRQ 0 (unmask in PIC)
 *
 * Must be called after pic_init() and idt_init().
 */
void timer_init(void);

/*
 * timer_get_ticks - Get current tick count
 *
 * Returns the number of timer interrupts since boot.
 * At 100 Hz, each tick represents 10ms.
 *
 * Returns: Current tick count (monotonically increasing)
 */
uint32_t timer_get_ticks(void);

#endif /* KERNEL_INCLUDE_TIMER_H */
