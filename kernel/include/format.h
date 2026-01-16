/*
 * kernel/include/format.h - Pure String Formatting Functions
 *
 * Provides buffer-based string formatting functions with no kernel
 * dependencies. These can be tested on the host and used by printk.
 *
 * All functions write to a provided buffer and return the number of
 * characters written (excluding null terminator).
 */

#ifndef KERNEL_INCLUDE_FORMAT_H
#define KERNEL_INCLUDE_FORMAT_H

#include <types.h>

/*
 * format_unsigned - Format unsigned integer to buffer
 *
 * Converts an unsigned integer to a string representation in the
 * specified base (10 or 16).
 *
 * @buf: Destination buffer (must be at least 12 bytes for base 10)
 * @size: Buffer size
 * @num: Number to format
 * @base: Number base (10 or 16)
 * @uppercase: Use uppercase hex digits (A-F) if true
 *
 * Returns: Number of characters written (excluding null terminator)
 */
int format_unsigned(char *buf, size_t size, uint32_t num, int base, int uppercase);

/*
 * format_signed - Format signed integer to buffer
 *
 * Converts a signed integer to a decimal string representation.
 * Handles INT32_MIN correctly without overflow.
 *
 * @buf: Destination buffer (must be at least 12 bytes)
 * @size: Buffer size
 * @num: Number to format
 *
 * Returns: Number of characters written (excluding null terminator)
 */
int format_signed(char *buf, size_t size, int32_t num);

/*
 * format_pointer - Format pointer to buffer
 *
 * Formats a pointer as "0x" followed by 8 lowercase hex digits.
 *
 * @buf: Destination buffer (must be at least 11 bytes)
 * @size: Buffer size
 * @ptr: Pointer value to format
 *
 * Returns: Number of characters written (excluding null terminator)
 */
int format_pointer(char *buf, size_t size, uint32_t ptr);

#endif /* KERNEL_INCLUDE_FORMAT_H */
