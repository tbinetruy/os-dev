/*
 * tests/host/test_string.c - Kernel string function unit tests
 *
 * Tests the string/memory functions used by the kernel.
 * These functions can be tested on the host without kernel dependencies.
 *
 * Build: make (in tests/ directory)
 * Run: ./test_string
 */

#include "unity/unity.h"
#include <stdint.h>

/*
 * Include the actual kernel string implementation.
 * HOST_TEST is defined by the Makefile so types.h uses standard headers.
 *
 * Note: We rename our memset to avoid conflict with libc.
 */
#define memset kernel_memset
#include "../../kernel/lib/string.c"
#undef memset

/* Test buffer */
static uint8_t test_buf[256];

void setUp(void)
{
    /* Fill with known pattern */
    for (int i = 0; i < 256; i++) {
        test_buf[i] = 0xAA;
    }
}

void tearDown(void)
{
    /* Nothing to clean up */
}

/*
 * Test cases
 */

void test_memset_zero(void)
{
    kernel_memset(test_buf, 0, 16);

    /* First 16 bytes should be 0 */
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT_EQUAL_HEX8(0x00, test_buf[i]);
    }
    /* Rest should be unchanged */
    for (int i = 16; i < 256; i++) {
        TEST_ASSERT_EQUAL_HEX8(0xAA, test_buf[i]);
    }
}

void test_memset_nonzero(void)
{
    kernel_memset(test_buf, 0x55, 32);

    for (int i = 0; i < 32; i++) {
        TEST_ASSERT_EQUAL_HEX8(0x55, test_buf[i]);
    }
    for (int i = 32; i < 256; i++) {
        TEST_ASSERT_EQUAL_HEX8(0xAA, test_buf[i]);
    }
}

void test_memset_full_buffer(void)
{
    kernel_memset(test_buf, 0xBB, 256);

    for (int i = 0; i < 256; i++) {
        TEST_ASSERT_EQUAL_HEX8(0xBB, test_buf[i]);
    }
}

void test_memset_single_byte(void)
{
    kernel_memset(test_buf, 0xCC, 1);

    TEST_ASSERT_EQUAL_HEX8(0xCC, test_buf[0]);
    TEST_ASSERT_EQUAL_HEX8(0xAA, test_buf[1]);
}

void test_memset_zero_length(void)
{
    kernel_memset(test_buf, 0xFF, 0);

    /* Buffer should be unchanged */
    for (int i = 0; i < 256; i++) {
        TEST_ASSERT_EQUAL_HEX8(0xAA, test_buf[i]);
    }
}

void test_memset_returns_dest(void)
{
    void *result = kernel_memset(test_buf, 0, 16);
    TEST_ASSERT_EQUAL_PTR(test_buf, result);
}

void test_memset_middle_of_buffer(void)
{
    /* Set bytes 10-19 */
    kernel_memset(test_buf + 10, 0x77, 10);

    /* First 10 unchanged */
    for (int i = 0; i < 10; i++) {
        TEST_ASSERT_EQUAL_HEX8(0xAA, test_buf[i]);
    }
    /* Middle 10 set */
    for (int i = 10; i < 20; i++) {
        TEST_ASSERT_EQUAL_HEX8(0x77, test_buf[i]);
    }
    /* Rest unchanged */
    for (int i = 20; i < 256; i++) {
        TEST_ASSERT_EQUAL_HEX8(0xAA, test_buf[i]);
    }
}

void test_memset_0xff_pattern(void)
{
    kernel_memset(test_buf, 0xFF, 64);

    for (int i = 0; i < 64; i++) {
        TEST_ASSERT_EQUAL_HEX8(0xFF, test_buf[i]);
    }
}

void test_memset_value_truncation(void)
{
    /* memset should only use low 8 bits of value */
    kernel_memset(test_buf, 0x1234, 8);  /* Should use 0x34 */

    for (int i = 0; i < 8; i++) {
        TEST_ASSERT_EQUAL_HEX8(0x34, test_buf[i]);
    }
}

/*
 * Main test runner
 */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_memset_zero);
    RUN_TEST(test_memset_nonzero);
    RUN_TEST(test_memset_full_buffer);
    RUN_TEST(test_memset_single_byte);
    RUN_TEST(test_memset_zero_length);
    RUN_TEST(test_memset_returns_dest);
    RUN_TEST(test_memset_middle_of_buffer);
    RUN_TEST(test_memset_0xff_pattern);
    RUN_TEST(test_memset_value_truncation);

    return UNITY_END();
}
