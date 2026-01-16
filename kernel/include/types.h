#ifndef KERNEL_INCLUDE_TYPES_H
#define KERNEL_INCLUDE_TYPES_H

/*
 * When compiling for host-side tests (HOST_TEST defined), use standard headers.
 * When compiling freestanding (kernel), define types manually.
 */
#ifdef HOST_TEST

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#else /* Freestanding / kernel build */

/* Fixed-width integer types */
typedef signed char        int8_t;
typedef unsigned char      uint8_t;
typedef signed short       int16_t;
typedef unsigned short     uint16_t;
typedef signed int         int32_t;
typedef unsigned int       uint32_t;
typedef signed long long   int64_t;
typedef unsigned long long uint64_t;

/* Size types */
typedef uint32_t size_t;
typedef int32_t  ssize_t;
typedef int32_t  ptrdiff_t;

/* Pointer-sized integer types */
typedef uint32_t uintptr_t;
typedef int32_t  intptr_t;

/* Boolean type */
typedef _Bool bool;
#define true  1
#define false 0

/* NULL pointer */
#define NULL ((void *)0)

#endif /* HOST_TEST */

#endif /* KERNEL_INCLUDE_TYPES_H */
