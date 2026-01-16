/*
 * kernel/lib/printk.c - Kernel Logging Implementation
 *
 * Implements printk() with format string parsing. Output goes to
 * both serial (primary) and VGA (secondary) for maximum visibility.
 *
 * This is a minimal implementation suitable for kernel debugging:
 *   - No dynamic memory allocation
 *   - No floating point support
 *   - No width/precision modifiers (MVP)
 *   - Fixed internal buffer for number conversion
 */

#include <printk.h>
#include <serial.h>
#include <vga.h>
#include <format.h>

/*
 * GCC built-in variadic argument support
 * Available in freestanding mode without stdarg.h
 */
typedef __builtin_va_list va_list;
#define va_start(ap, last)  __builtin_va_start(ap, last)
#define va_end(ap)          __builtin_va_end(ap)
#define va_arg(ap, type)    __builtin_va_arg(ap, type)

/*
 * Log level prefixes - indexed by level value
 */
static const char *level_prefixes[] = {
    "[ERROR] ",
    "[WARN]  ",
    "[INFO]  ",
    "[DEBUG] "
};

/*
 * output_char - Send character to both serial and VGA
 *
 * Converts '\n' to '\r\n' on serial for proper terminal display,
 * matching the behavior of serial_puts().
 */
static void output_char(char c)
{
    if (c == '\n') {
        serial_putchar('\r');
    }
    serial_putchar(c);
    vga_putchar(c);
}

/*
 * output_string - Send string to both serial and VGA
 */
static void output_string(const char *s)
{
    serial_puts(s);
    vga_puts(s);
}

/*
 * print_unsigned - Print unsigned integer in specified base
 *
 * Uses format_unsigned() for the actual conversion, then outputs the result.
 *
 * @num: Number to print
 * @base: Number base (10 or 16)
 * @uppercase: Use uppercase hex digits if true
 */
static void print_unsigned(uint32_t num, int base, int uppercase)
{
    char buf[12];
    format_unsigned(buf, sizeof(buf), num, base, uppercase);
    output_string(buf);
}

/*
 * print_signed - Print signed integer
 *
 * Uses format_signed() for the actual conversion, then outputs the result.
 *
 * @num: Number to print
 */
static void print_signed(int32_t num)
{
    char buf[12];
    format_signed(buf, sizeof(buf), num);
    output_string(buf);
}

/*
 * print_pointer - Print 32-bit value as hex with 0x prefix
 *
 * Uses format_pointer() for the actual conversion, then outputs the result.
 *
 * @num: Value to print
 */
static void print_pointer(uint32_t num)
{
    char buf[12];
    format_pointer(buf, sizeof(buf), num);
    output_string(buf);
}

/*
 * vprintk - Print formatted string with va_list
 *
 * @fmt: Format string
 * @args: Argument list
 */
static void vprintk(const char *fmt, va_list args)
{
    char c;

    while ((c = *fmt++) != '\0') {
        if (c != '%') {
            output_char(c);
            continue;
        }

        /* Handle format specifier */
        c = *fmt++;
        if (c == '\0') {
            break;
        }

        switch (c) {
        case 's': {
            const char *s = va_arg(args, const char *);
            if (s == NULL) {
                s = "(null)";
            }
            while (*s) {
                output_char(*s++);
            }
            break;
        }

        case 'd': {
            int32_t n = va_arg(args, int32_t);
            print_signed(n);
            break;
        }

        case 'u': {
            uint32_t n = va_arg(args, uint32_t);
            print_unsigned(n, 10, 0);
            break;
        }

        case 'x': {
            uint32_t n = va_arg(args, uint32_t);
            print_unsigned(n, 16, 0);
            break;
        }

        case 'X': {
            uint32_t n = va_arg(args, uint32_t);
            print_unsigned(n, 16, 1);
            break;
        }

        case 'c': {
            /* char is promoted to int in variadic functions */
            char ch = (char)va_arg(args, int);
            output_char(ch);
            break;
        }

        case 'p': {
            void *ptr = va_arg(args, void *);
            print_pointer((uint32_t)ptr);
            break;
        }

        case '%':
            output_char('%');
            break;

        default:
            /* Unknown specifier - print literally */
            output_char('%');
            output_char(c);
            break;
        }
    }
}

/*
 * printk - Print formatted kernel message
 *
 * Entry point for all kernel logging. Prefixes message with log level
 * and outputs to both serial and VGA.
 */
void printk(int level, const char *fmt, ...)
{
    va_list args;

    /* Filter by compile-time log level */
    if (level > LOG_LEVEL) {
        return;
    }

    /* Validate level for prefix lookup */
    if (level >= 0 && level <= LOG_DEBUG) {
        output_string(level_prefixes[level]);
    }

    va_start(args, fmt);
    vprintk(fmt, args);
    va_end(args);
}
