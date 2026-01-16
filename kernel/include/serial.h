/*
 * kernel/include/serial.h - Serial Port (UART) Driver Interface
 *
 * Provides serial port communication for debug output. Uses COM1 (0x3F8)
 * as the primary debug serial port with polling-based I/O.
 *
 * The serial driver is essential for:
 *   - Debug output visible in QEMU's -serial stdio
 *   - Output when VGA fails or isn't initialized
 *   - Logging that can be captured to files
 *   - GDB debugging coordination
 *
 * Hardware: 16550A UART compatible
 * Configuration: 38400 baud, 8 data bits, no parity, 1 stop bit (8N1)
 */

#ifndef KERNEL_INCLUDE_SERIAL_H
#define KERNEL_INCLUDE_SERIAL_H

#include <types.h>

/*
 * =============================================================================
 * COM Port Base Addresses
 * =============================================================================
 */
#define COM1_PORT 0x3F8
#define COM2_PORT 0x2F8
#define COM3_PORT 0x3E8
#define COM4_PORT 0x2E8

/*
 * =============================================================================
 * UART Register Offsets (from base port)
 * =============================================================================
 *
 * Register layout depends on DLAB (Divisor Latch Access Bit) in LCR:
 *
 * Offset | DLAB=0 Read             | DLAB=0 Write      | DLAB=1
 * -------|-------------------------|-------------------|------------------
 *   +0   | RX Buffer               | TX Buffer         | Divisor Latch LSB
 *   +1   | Interrupt Enable        | Int Enable        | Divisor Latch MSB
 *   +2   | Interrupt Identification| FIFO Control      | (same)
 *   +3   | Line Control            | Line Control      | (same)
 *   +4   | Modem Control           | Modem Control     | (same)
 *   +5   | Line Status             | (factory test)    | (same)
 *   +6   | Modem Status            | (not used)        | (same)
 *   +7   | Scratch                 | Scratch           | (same)
 */
#define SERIAL_DATA         0   /* Data register (RX/TX) */
#define SERIAL_INT_ENABLE   1   /* Interrupt enable register */
#define SERIAL_DIV_LSB      0   /* Divisor latch LSB (DLAB=1) */
#define SERIAL_DIV_MSB      1   /* Divisor latch MSB (DLAB=1) */
#define SERIAL_FIFO_CTRL    2   /* FIFO control register */
#define SERIAL_LINE_CTRL    3   /* Line control register */
#define SERIAL_MODEM_CTRL   4   /* Modem control register */
#define SERIAL_LINE_STATUS  5   /* Line status register */
#define SERIAL_MODEM_STATUS 6   /* Modem status register */
#define SERIAL_SCRATCH      7   /* Scratch register */

/*
 * =============================================================================
 * Line Control Register (LCR) Bits
 * =============================================================================
 */
#define SERIAL_LCR_DLAB     0x80    /* Divisor Latch Access Bit */
#define SERIAL_LCR_8N1      0x03    /* 8 data bits, no parity, 1 stop bit */

/*
 * =============================================================================
 * Line Status Register (LSR) Bits
 * =============================================================================
 */
#define SERIAL_LSR_DATA_READY   0x01    /* Data available in RX buffer */
#define SERIAL_LSR_TX_EMPTY     0x20    /* TX holding register empty */

/*
 * =============================================================================
 * FIFO Control Register (FCR) Values
 * =============================================================================
 */
#define SERIAL_FCR_ENABLE       0x01    /* Enable FIFOs */
#define SERIAL_FCR_CLEAR_RX     0x02    /* Clear receive FIFO */
#define SERIAL_FCR_CLEAR_TX     0x04    /* Clear transmit FIFO */
#define SERIAL_FCR_TRIGGER_14   0xC0    /* 14-byte trigger level */

/*
 * =============================================================================
 * Modem Control Register (MCR) Bits
 * =============================================================================
 */
#define SERIAL_MCR_DTR          0x01    /* Data Terminal Ready */
#define SERIAL_MCR_RTS          0x02    /* Request To Send */
#define SERIAL_MCR_OUT2         0x08    /* Auxiliary output 2 (IRQ enable) */

/*
 * =============================================================================
 * Baud Rate Divisors
 * =============================================================================
 *
 * Divisor = 115200 / baud_rate
 */
#define SERIAL_BAUD_115200      1
#define SERIAL_BAUD_57600       2
#define SERIAL_BAUD_38400       3
#define SERIAL_BAUD_19200       6
#define SERIAL_BAUD_9600        12

/* Default baud rate for debug output */
#define SERIAL_DEFAULT_BAUD     SERIAL_BAUD_38400

/*
 * =============================================================================
 * Public Functions
 * =============================================================================
 */

/*
 * serial_init - Initialize the serial port driver
 *
 * Configures COM1 for serial communication:
 *   - Disables interrupts
 *   - Sets baud rate to 38400
 *   - Configures 8N1 (8 data bits, no parity, 1 stop bit)
 *   - Enables and clears FIFOs
 *   - Sets DTR, RTS, and OUT2
 *
 * Must be called before any other serial functions.
 */
void serial_init(void);

/*
 * serial_putchar - Write a single character to serial port
 *
 * Waits for the transmit buffer to be empty (polling), then sends
 * the character. This is a blocking call.
 *
 * @c: Character to transmit
 */
void serial_putchar(char c);

/*
 * serial_puts - Write a null-terminated string to serial port
 *
 * Sends each character of the string using serial_putchar().
 * Automatically converts '\n' to '\r\n' for proper terminal display.
 *
 * @str: Null-terminated string to transmit
 */
void serial_puts(const char *str);

/*
 * serial_write - Write a buffer of bytes to serial port
 *
 * Sends exactly 'len' bytes from the buffer. Does not interpret
 * the data (no newline conversion). Useful for binary data.
 *
 * @buf: Pointer to data buffer
 * @len: Number of bytes to send
 */
void serial_write(const void *buf, size_t len);

#endif /* KERNEL_INCLUDE_SERIAL_H */
