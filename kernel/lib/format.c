/*
 * kernel/lib/format.c - Pure String Formatting Implementation
 *
 * Implements buffer-based string formatting functions. These are pure
 * functions with no kernel dependencies - they only use types.h.
 *
 * This allows the formatting logic to be:
 *   - Unit tested on the host with standard tools
 *   - Used by printk for kernel output
 *   - Reused anywhere string formatting is needed
 */

#include <format.h>

/*
 * format_unsigned - Format unsigned integer to buffer
 */
int format_unsigned(char *buf, size_t size, uint32_t num, int base, int uppercase)
{
    char tmp[12];  /* Enough for 32-bit decimal (10 digits) or hex (8 digits) */
    char *p = tmp + sizeof(tmp) - 1;
    const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    int len = 0;

    if (size == 0) {
        return 0;
    }

    *p = '\0';

    /* Special case: zero */
    if (num == 0) {
        if (size > 1) {
            buf[0] = '0';
            buf[1] = '\0';
            return 1;
        }
        buf[0] = '\0';
        return 0;
    }

    /* Build string in reverse */
    while (num > 0) {
        *--p = digits[num % base];
        num /= base;
    }

    /* Copy to output buffer */
    while (*p && (size_t)(len + 1) < size) {
        buf[len++] = *p++;
    }
    buf[len] = '\0';

    return len;
}

/*
 * format_signed - Format signed integer to buffer
 */
int format_signed(char *buf, size_t size, int32_t num)
{
    int len = 0;

    if (size == 0) {
        return 0;
    }

    if (num < 0) {
        if (size > 1) {
            buf[len++] = '-';
        }

        /* Handle INT32_MIN specially to avoid overflow on negation */
        if (num == (int32_t)0x80000000) {
            const char *int32_min = "2147483648";
            const char *p = int32_min;
            while (*p && (size_t)(len + 1) < size) {
                buf[len++] = *p++;
            }
            buf[len] = '\0';
            return len;
        }
        num = -num;
    }

    /* Format the absolute value */
    int ulen = format_unsigned(buf + len, size - len, (uint32_t)num, 10, 0);
    return len + ulen;
}

/*
 * format_pointer - Format pointer to buffer
 */
int format_pointer(char *buf, size_t size, uint32_t ptr)
{
    const char *digits = "0123456789abcdef";
    int i;
    int len = 0;

    if (size == 0) {
        return 0;
    }

    /* Write "0x" prefix */
    if (size > 1) {
        buf[len++] = '0';
    }
    if ((size_t)(len + 1) < size) {
        buf[len++] = 'x';
    }

    /* Write 8 hex digits with leading zeros */
    for (i = 7; i >= 0 && (size_t)(len + 1) < size; i--) {
        buf[len++] = digits[(ptr >> (i * 4)) & 0xF];
    }

    buf[len] = '\0';
    return len;
}
