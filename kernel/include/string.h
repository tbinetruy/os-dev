/*
 * string.h - Kernel String and Memory Functions
 *
 * Minimal implementations of standard library functions needed
 * by the kernel. These operate in kernel space only.
 */

#ifndef KERNEL_INCLUDE_STRING_H
#define KERNEL_INCLUDE_STRING_H

#include <types.h>

/*
 * memset - Fill memory with a constant byte
 *
 * @dest: Pointer to the memory area to fill
 * @val:  Byte value to fill with (only low 8 bits used)
 * @len:  Number of bytes to fill
 *
 * Returns: Pointer to dest
 */
void *memset(void *dest, int val, size_t len);

#endif /* KERNEL_INCLUDE_STRING_H */
