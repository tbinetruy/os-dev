/*
 * kernel/drivers/timer.c - PIT timer driver implementation
 *
 * Configures the 8253/8254 Programmable Interval Timer to generate
 * interrupts at 100 Hz (10ms interval) for system timekeeping.
 *
 * The timer interrupt (IRQ 0 → INT 32) increments a global tick
 * counter that can be used for:
 *   - System uptime tracking
 *   - Sleep/delay functions
 *   - Preemptive scheduler time slicing
 *
 * PIT Configuration:
 *   - Channel 0: System timer
 *   - Mode 3: Square wave generator (for continuous interrupts)
 *   - Divisor: 11,932 (gives ~100.007 Hz)
 *
 * References:
 *   - Intel 8253/8254 Datasheet
 *   - OSDev Wiki: Programmable Interval Timer
 */

#include <timer.h>
#include <pic.h>
#include <isr.h>
#include <asm.h>
#include <printk.h>

/*
 * Global tick counter
 *
 * Incremented by timer interrupt handler. Declared volatile because
 * it's modified in interrupt context and read from main code.
 *
 * At 100 Hz, this will overflow after ~497 days. For a hobby OS,
 * this is acceptable. Production systems would use 64-bit counters.
 */
static volatile uint32_t ticks = 0;

/*
 * timer_handler - Timer interrupt handler
 *
 * Called by the IRQ dispatch code when IRQ 0 fires.
 * Increments the global tick counter.
 *
 * Future enhancements:
 *   - Call scheduler for preemptive multitasking
 *   - Wake sleeping processes
 *   - Update system time
 *
 * @regs: Pointer to saved register state (unused for now)
 */
static void timer_handler(struct registers *regs)
{
    (void)regs;  /* Suppress unused parameter warning */
    ticks++;

    /*
     * Future: scheduler tick processing
     * sched_tick();
     */
}

/*
 * timer_init - Initialize the PIT for 100 Hz operation
 *
 * Configuration sequence:
 *   1. Write command byte to select channel 0, lo/hi access, mode 3
 *   2. Write low byte of divisor
 *   3. Write high byte of divisor
 *   4. Register our handler for IRQ 0
 *   5. Unmask IRQ 0 to enable timer interrupts
 *
 * The PIT immediately begins counting after the divisor is loaded.
 * Interrupts won't actually fire until:
 *   a) IRQ 0 is unmasked in the PIC (done here)
 *   b) CPU interrupts are enabled with STI (done in kmain)
 */
void timer_init(void)
{
    /*
     * Calculate divisor for target frequency
     *
     * divisor = base_frequency / target_frequency
     *         = 1,193,182 / 100
     *         = 11,931.82 → 11,931 (truncated)
     *
     * Actual frequency = 1,193,182 / 11,931 ≈ 100.01 Hz
     *
     * Note: We use PIT_DIVISOR constant which equals PIT_FREQUENCY/TARGET_HZ
     */
    uint16_t divisor = PIT_DIVISOR;

    printk(LOG_DEBUG, "Timer: configuring PIT for %d Hz (divisor=%d)\n",
           TARGET_HZ, divisor);

    /*
     * Write command byte to mode/command register (port 0x43)
     *
     * Command: 0x36
     *   Bits 7-6: 00 = Channel 0
     *   Bits 5-4: 11 = Access lo/hi byte
     *   Bits 3-1: 011 = Mode 3 (square wave)
     *   Bit 0:    0 = Binary mode
     */
    outb(PIT_CMD, PIT_CMD_TIMER);

    /*
     * Write divisor to channel 0 data port (port 0x40)
     *
     * Low byte first, then high byte (as specified by access mode 11).
     */
    outb(PIT_CHANNEL0, divisor & 0xFF);         /* Low byte */
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);  /* High byte */

    /*
     * Register our handler for IRQ 0
     *
     * IRQ 0 is mapped to INT 32 after PIC remapping.
     */
    irq_register_handler(IRQ_TIMER, timer_handler);

    /*
     * Enable IRQ 0 by clearing its mask bit in the PIC
     */
    pic_clear_mask(IRQ_TIMER);

    printk(LOG_DEBUG, "Timer: initialized at %d Hz\n", TARGET_HZ);
}

/*
 * timer_get_ticks - Return current tick count
 *
 * Returns the number of timer interrupts since boot.
 * At 100 Hz, divide by 100 to get seconds.
 *
 * Note: This is a simple read of a volatile variable. No locking
 * is needed because the read is atomic on 32-bit x86.
 *
 * Returns: Current tick count
 */
uint32_t timer_get_ticks(void)
{
    return ticks;
}
