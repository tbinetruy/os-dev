/*
 * kernel/include/printk.h - Kernel Logging Interface
 *
 * Provides printf-like formatted output for kernel messages with
 * configurable log levels. Output goes to both serial (primary)
 * and VGA (secondary) for maximum visibility.
 *
 * Log Levels:
 *   LOG_ERROR (0) - Failures requiring attention
 *   LOG_WARN  (1) - Unexpected but handled conditions
 *   LOG_INFO  (2) - Significant events (boot progress, etc.)
 *   LOG_DEBUG (3) - Detailed tracing for development
 *
 * Usage:
 *   printk(LOG_INFO, "PMM: %d pages free\n", free_count);
 *   printk(LOG_ERROR, "Failed to allocate page\n");
 *
 * Supported format specifiers:
 *   %s  - string (char *)
 *   %d  - signed decimal (int32_t)
 *   %u  - unsigned decimal (uint32_t)
 *   %x  - lowercase hexadecimal (uint32_t)
 *   %X  - uppercase hexadecimal (uint32_t)
 *   %c  - character (char)
 *   %p  - pointer (void *) - printed as 0xXXXXXXXX
 *   %%  - literal percent sign
 */

#ifndef KERNEL_INCLUDE_PRINTK_H
#define KERNEL_INCLUDE_PRINTK_H

#include <types.h>

/*
 * =============================================================================
 * Log Levels
 * =============================================================================
 *
 * Messages with level > LOG_LEVEL are filtered at compile time.
 * Set LOG_LEVEL in Makefile or here for default.
 */
#define LOG_ERROR   0
#define LOG_WARN    1
#define LOG_INFO    2
#define LOG_DEBUG   3

/* Default compile-time log level filter */
#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_DEBUG
#endif

/*
 * =============================================================================
 * Public Functions
 * =============================================================================
 */

/*
 * printk - Print formatted kernel message
 *
 * Outputs a formatted message to both serial and VGA console.
 * Messages are prefixed with the log level (e.g., "[INFO] ").
 *
 * If level > LOG_LEVEL, the message is silently discarded.
 *
 * @level: Log level (LOG_ERROR, LOG_WARN, LOG_INFO, LOG_DEBUG)
 * @fmt: printf-style format string
 * @...: Arguments for format specifiers
 */
void printk(int level, const char *fmt, ...);

#endif /* KERNEL_INCLUDE_PRINTK_H */
