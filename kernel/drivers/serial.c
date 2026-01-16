/*
 * kernel/drivers/serial.c - Serial Port (UART) Driver Implementation
 *
 * Implements polling-based serial I/O on COM1 for kernel debug output.
 * Uses the 16550A UART interface available on standard PC hardware.
 *
 * This driver is intentionally simple:
 *   - Polling-based (no interrupts) for reliability during early boot
 *   - Output only (no input handling yet)
 *   - Single port (COM1) hardcoded
 *
 * The polling approach ensures output works even before interrupts are
 * enabled, making it suitable for debug output during all boot stages.
 */

#include <serial.h>
#include <asm.h>

/*
 * serial_init - Initialize COM1 for serial communication
 *
 * Initialization sequence:
 *   1. Disable all UART interrupts
 *   2. Enable DLAB to set baud rate divisor
 *   3. Set divisor for 38400 baud
 *   4. Clear DLAB and configure 8N1
 *   5. Enable and clear FIFOs
 *   6. Set modem control lines (DTR, RTS, OUT2)
 */
void serial_init(void)
{
    /* Disable all interrupts */
    outb(COM1_PORT + SERIAL_INT_ENABLE, 0x00);

    /* Enable DLAB (Divisor Latch Access Bit) to set baud rate */
    outb(COM1_PORT + SERIAL_LINE_CTRL, SERIAL_LCR_DLAB);

    /* Set divisor to 3 (38400 baud) */
    outb(COM1_PORT + SERIAL_DIV_LSB, SERIAL_DEFAULT_BAUD);
    outb(COM1_PORT + SERIAL_DIV_MSB, 0x00);

    /* Clear DLAB and set 8N1 (8 data bits, no parity, 1 stop bit) */
    outb(COM1_PORT + SERIAL_LINE_CTRL, SERIAL_LCR_8N1);

    /* Enable FIFO, clear both FIFOs, set 14-byte threshold */
    outb(COM1_PORT + SERIAL_FIFO_CTRL,
         SERIAL_FCR_ENABLE | SERIAL_FCR_CLEAR_RX |
         SERIAL_FCR_CLEAR_TX | SERIAL_FCR_TRIGGER_14);

    /* Set DTR, RTS, and OUT2 (OUT2 enables IRQs if we want them later) */
    outb(COM1_PORT + SERIAL_MODEM_CTRL,
         SERIAL_MCR_DTR | SERIAL_MCR_RTS | SERIAL_MCR_OUT2);
}

/*
 * serial_is_transmit_empty - Check if transmit buffer is empty
 *
 * Reads the Line Status Register and checks the TX empty bit.
 *
 * Returns: non-zero if transmit buffer is empty, 0 if busy
 */
static int serial_is_transmit_empty(void)
{
    return inb(COM1_PORT + SERIAL_LINE_STATUS) & SERIAL_LSR_TX_EMPTY;
}

/*
 * serial_putchar - Write a single character to serial port
 *
 * Spins waiting for the transmit holding register to be empty,
 * then writes the character. This is a blocking operation.
 */
void serial_putchar(char c)
{
    /* Wait for transmit buffer to be empty */
    while (!serial_is_transmit_empty()) {
        /* spin */
    }

    /* Send the character */
    outb(COM1_PORT + SERIAL_DATA, c);
}

/*
 * serial_puts - Write a null-terminated string to serial port
 *
 * Sends each character, converting lone '\n' to '\r\n' for proper
 * terminal behavior (CR+LF line endings).
 */
void serial_puts(const char *str)
{
    while (*str) {
        if (*str == '\n') {
            serial_putchar('\r');
        }
        serial_putchar(*str);
        str++;
    }
}

/*
 * serial_write - Write a buffer of bytes to serial port
 *
 * Sends exactly 'len' bytes without any interpretation or conversion.
 * Useful for binary data or when explicit control of output is needed.
 */
void serial_write(const void *buf, size_t len)
{
    const uint8_t *p = (const uint8_t *)buf;

    while (len--) {
        serial_putchar(*p++);
    }
}
