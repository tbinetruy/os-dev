/*
 * errno.h - Kernel Error Numbers
 *
 * Standard error codes for kernel functions. Functions return
 * negative errno values on failure (e.g., -ENOMEM).
 *
 * Values match Linux/POSIX errno for familiarity.
 */

#ifndef KERNEL_INCLUDE_ERRNO_H
#define KERNEL_INCLUDE_ERRNO_H

#define ENOMEM  12      /* Out of memory */
#define EEXIST  17      /* File or mapping already exists */
#define EINVAL  22      /* Invalid argument */
#define EFAULT  14      /* Bad address */

#endif /* KERNEL_INCLUDE_ERRNO_H */
