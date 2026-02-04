/*
 * bitmap.h - Bitmap Operations
 *
 * Generic bitmap manipulation functions for tracking allocation state.
 * Used by the physical memory manager and potentially other subsystems.
 *
 * Each bit represents one item: 0 = free, 1 = allocated.
 */

#ifndef KERNEL_INCLUDE_BITMAP_H
#define KERNEL_INCLUDE_BITMAP_H

#include <types.h>

/*
 * bitmap_set - Set a bit (mark as allocated)
 *
 * @bitmap: Pointer to bitmap array
 * @bit: Bit index to set
 */
void bitmap_set(uint8_t *bitmap, uint32_t bit);

/*
 * bitmap_clear - Clear a bit (mark as free)
 *
 * @bitmap: Pointer to bitmap array
 * @bit: Bit index to clear
 */
void bitmap_clear(uint8_t *bitmap, uint32_t bit);

/*
 * bitmap_test - Test if a bit is set
 *
 * @bitmap: Pointer to bitmap array
 * @bit: Bit index to test
 *
 * Returns: 1 if set (allocated), 0 if clear (free)
 */
int bitmap_test(uint8_t *bitmap, uint32_t bit);

#endif /* KERNEL_INCLUDE_BITMAP_H */
