/*
 * bitmap.c - Bitmap Operations
 *
 * Generic bitmap manipulation functions. These are pure functions
 * with no kernel dependencies beyond types.h, making them testable
 * on the host system.
 */

#include <bitmap.h>

/*
 * bitmap_set - Set a bit (mark as allocated)
 */
void bitmap_set(uint8_t *bitmap, uint32_t bit)
{
    bitmap[bit / 8] |= (1 << (bit % 8));
}

/*
 * bitmap_clear - Clear a bit (mark as free)
 */
void bitmap_clear(uint8_t *bitmap, uint32_t bit)
{
    bitmap[bit / 8] &= ~(1 << (bit % 8));
}

/*
 * bitmap_test - Test if a bit is set
 *
 * Returns: 1 if set (allocated), 0 if clear (free)
 */
int bitmap_test(uint8_t *bitmap, uint32_t bit)
{
    return (bitmap[bit / 8] >> (bit % 8)) & 1;
}
