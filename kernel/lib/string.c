/*
 * string.c - Kernel String and Memory Functions
 *
 * Minimal implementations of memory manipulation functions needed
 * by the kernel. These are freestanding implementations that don't
 * depend on any external libraries.
 */

#include <string.h>

/*
 * memset - Fill memory with a constant byte
 *
 * Simple byte-by-byte implementation. For larger fills, this could
 * be optimized to use word-sized operations, but this version is
 * sufficient for kernel initialization.
 */
void *memset(void *dest, int val, size_t len)
{
    uint8_t *ptr = (uint8_t *)dest;
    uint8_t byte = (uint8_t)val;

    while (len--) {
        *ptr++ = byte;
    }

    return dest;
}
