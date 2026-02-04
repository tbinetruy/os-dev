/*
 * mmap.h - Memory Map Definitions
 *
 * Defines structures and constants for parsing the BIOS E820 memory map
 * passed from the bootloader. Used by the physical memory manager to
 * determine which physical memory regions are available for allocation.
 *
 * The bootloader (stage2.S) queries BIOS INT 15h, E820h and stores
 * the results at a known location, passing the pointer and count
 * to the kernel via boot_mmap_ptr and boot_mmap_count.
 */

#ifndef KERNEL_INCLUDE_MMAP_H
#define KERNEL_INCLUDE_MMAP_H

#include <types.h>

/*
 * E820 Memory Map Entry Types
 *
 * Type 1 (Available): Normal usable RAM that can be used for allocation.
 * Type 2 (Reserved): Memory reserved by the system (firmware, hardware).
 * Type 3 (ACPI Reclaimable): Can be reclaimed after ACPI tables are parsed.
 * Type 4 (ACPI NVS): ACPI Non-Volatile Storage, must not be used.
 * Type 5 (Bad): Known bad memory, do not use.
 *
 * For this kernel, only Type 1 regions are considered free for allocation.
 */
#define MMAP_TYPE_AVAILABLE     1
#define MMAP_TYPE_RESERVED      2
#define MMAP_TYPE_ACPI_RECLAIM  3
#define MMAP_TYPE_ACPI_NVS      4
#define MMAP_TYPE_BAD           5

/*
 * struct mmap_entry - BIOS E820 memory map entry
 *
 * This structure matches the format returned by BIOS INT 15h, E820h.
 * Each entry describes a contiguous region of physical memory.
 *
 * Note: base and length are 64-bit, but this kernel only supports
 * the first 4GB of physical memory. Higher regions are ignored.
 *
 * Must be packed to ensure layout matches BIOS output exactly.
 */
struct mmap_entry {
    uint64_t base;      /* Physical start address of region */
    uint64_t length;    /* Length of region in bytes */
    uint32_t type;      /* Region type (see MMAP_TYPE_* constants) */
    uint32_t acpi_ext;  /* ACPI 3.0 extended attributes (ignored) */
} __attribute__((packed));

/*
 * Boot memory map variables (defined in entry.S)
 *
 * boot_mmap_ptr:   Physical address of the mmap_entry array
 * boot_mmap_count: Number of entries in the array
 *
 * These are populated by the bootloader before jumping to kmain().
 */
extern uint32_t boot_mmap_ptr;
extern uint32_t boot_mmap_count;

#endif /* KERNEL_INCLUDE_MMAP_H */
