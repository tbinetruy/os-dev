/*
 * tests/host/test_bitmap.c - Bitmap allocator unit tests
 *
 * Tests the bitmap operations used by the physical memory manager.
 * These functions can be tested on the host without kernel dependencies.
 *
 * Build: make (in tests/ directory)
 * Run: ./test_bitmap
 */

#include "unity/unity.h"
#include <stdint.h>
#include <string.h>

/*
 * Include the actual kernel bitmap implementation.
 * HOST_TEST is defined by the Makefile so types.h uses standard headers.
 */
#include "../../kernel/lib/bitmap.c"

/* Test fixtures */
static uint8_t test_bitmap[16];  /* 128 bits */

void setUp(void)
{
    memset(test_bitmap, 0, sizeof(test_bitmap));
}

void tearDown(void)
{
    /* Nothing to clean up */
}

/*
 * Test cases
 */

void test_bitmap_initially_clear(void)
{
    /* All bits should be clear after setUp */
    for (int i = 0; i < 128; i++) {
        TEST_ASSERT_FALSE(bitmap_test(test_bitmap, i));
    }
}

void test_bitmap_set_single_bit(void)
{
    bitmap_set(test_bitmap, 0);
    TEST_ASSERT_TRUE(bitmap_test(test_bitmap, 0));
    TEST_ASSERT_FALSE(bitmap_test(test_bitmap, 1));
}

void test_bitmap_set_bit_in_different_bytes(void)
{
    bitmap_set(test_bitmap, 0);   /* Byte 0, bit 0 */
    bitmap_set(test_bitmap, 7);   /* Byte 0, bit 7 */
    bitmap_set(test_bitmap, 8);   /* Byte 1, bit 0 */
    bitmap_set(test_bitmap, 63);  /* Byte 7, bit 7 */

    TEST_ASSERT_TRUE(bitmap_test(test_bitmap, 0));
    TEST_ASSERT_TRUE(bitmap_test(test_bitmap, 7));
    TEST_ASSERT_TRUE(bitmap_test(test_bitmap, 8));
    TEST_ASSERT_TRUE(bitmap_test(test_bitmap, 63));

    /* Check that adjacent bits are not affected */
    TEST_ASSERT_FALSE(bitmap_test(test_bitmap, 1));
    TEST_ASSERT_FALSE(bitmap_test(test_bitmap, 6));
    TEST_ASSERT_FALSE(bitmap_test(test_bitmap, 9));
}

void test_bitmap_clear_bit(void)
{
    /* Set all bits in first byte */
    test_bitmap[0] = 0xFF;

    /* Clear bit 5 */
    bitmap_clear(test_bitmap, 5);

    TEST_ASSERT_FALSE(bitmap_test(test_bitmap, 5));
    TEST_ASSERT_TRUE(bitmap_test(test_bitmap, 4));
    TEST_ASSERT_TRUE(bitmap_test(test_bitmap, 6));
    TEST_ASSERT_EQUAL_HEX8(0xDF, test_bitmap[0]);
}

void test_bitmap_set_and_clear_sequence(void)
{
    /* Set some bits */
    bitmap_set(test_bitmap, 10);
    bitmap_set(test_bitmap, 20);
    bitmap_set(test_bitmap, 30);

    TEST_ASSERT_TRUE(bitmap_test(test_bitmap, 10));
    TEST_ASSERT_TRUE(bitmap_test(test_bitmap, 20));
    TEST_ASSERT_TRUE(bitmap_test(test_bitmap, 30));

    /* Clear middle one */
    bitmap_clear(test_bitmap, 20);

    TEST_ASSERT_TRUE(bitmap_test(test_bitmap, 10));
    TEST_ASSERT_FALSE(bitmap_test(test_bitmap, 20));
    TEST_ASSERT_TRUE(bitmap_test(test_bitmap, 30));
}

void test_bitmap_boundary_bits(void)
{
    /* Test first and last bits */
    bitmap_set(test_bitmap, 0);
    bitmap_set(test_bitmap, 127);

    TEST_ASSERT_TRUE(bitmap_test(test_bitmap, 0));
    TEST_ASSERT_TRUE(bitmap_test(test_bitmap, 127));
    TEST_ASSERT_FALSE(bitmap_test(test_bitmap, 1));
    TEST_ASSERT_FALSE(bitmap_test(test_bitmap, 126));
}

void test_bitmap_all_bits_in_byte(void)
{
    /* Set all 8 bits in byte 1 */
    for (int i = 8; i < 16; i++) {
        bitmap_set(test_bitmap, i);
    }

    TEST_ASSERT_EQUAL_HEX8(0xFF, test_bitmap[1]);

    /* Verify all are set */
    for (int i = 8; i < 16; i++) {
        TEST_ASSERT_TRUE(bitmap_test(test_bitmap, i));
    }

    /* Verify adjacent bytes untouched */
    TEST_ASSERT_EQUAL_HEX8(0x00, test_bitmap[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, test_bitmap[2]);
}

void test_bitmap_idempotent_set(void)
{
    /* Setting same bit twice should be idempotent */
    bitmap_set(test_bitmap, 42);
    bitmap_set(test_bitmap, 42);

    TEST_ASSERT_TRUE(bitmap_test(test_bitmap, 42));
}

void test_bitmap_idempotent_clear(void)
{
    /* Clearing same bit twice should be idempotent */
    bitmap_set(test_bitmap, 42);
    bitmap_clear(test_bitmap, 42);
    bitmap_clear(test_bitmap, 42);

    TEST_ASSERT_FALSE(bitmap_test(test_bitmap, 42));
}

/*
 * Main test runner
 */
int main(void)
{
    UNITY_BEGIN();

    /* Basic operations */
    RUN_TEST(test_bitmap_initially_clear);
    RUN_TEST(test_bitmap_set_single_bit);
    RUN_TEST(test_bitmap_set_bit_in_different_bytes);
    RUN_TEST(test_bitmap_clear_bit);
    RUN_TEST(test_bitmap_set_and_clear_sequence);

    /* Edge cases */
    RUN_TEST(test_bitmap_boundary_bits);
    RUN_TEST(test_bitmap_all_bits_in_byte);
    RUN_TEST(test_bitmap_idempotent_set);
    RUN_TEST(test_bitmap_idempotent_clear);

    return UNITY_END();
}
