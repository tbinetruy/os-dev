/*
 * tests/host/test_format.c - Host-side unit tests for formatting functions
 *
 * Tests the pure string formatting functions used by printk.
 * These run on the development host using Unity framework.
 *
 * Build: make (in tests/ directory)
 * Run: ./test_format
 */

#include "unity/unity.h"
#include <format.h>
#include <string.h>

void setUp(void)
{
}

void tearDown(void)
{
}

/*
 * =============================================================================
 * format_unsigned tests
 * =============================================================================
 */

void test_format_unsigned_zero(void)
{
    char buf[16];
    int len = format_unsigned(buf, sizeof(buf), 0, 10, 0);
    TEST_ASSERT_EQUAL_STRING("0", buf);
    TEST_ASSERT_EQUAL_INT(1, len);
}

void test_format_unsigned_decimal(void)
{
    char buf[16];
    int len = format_unsigned(buf, sizeof(buf), 12345, 10, 0);
    TEST_ASSERT_EQUAL_STRING("12345", buf);
    TEST_ASSERT_EQUAL_INT(5, len);
}

void test_format_unsigned_max(void)
{
    char buf[16];
    int len = format_unsigned(buf, sizeof(buf), 4294967295U, 10, 0);
    TEST_ASSERT_EQUAL_STRING("4294967295", buf);
    TEST_ASSERT_EQUAL_INT(10, len);
}

void test_format_unsigned_hex_lower(void)
{
    char buf[16];
    int len = format_unsigned(buf, sizeof(buf), 0xDEADBEEF, 16, 0);
    TEST_ASSERT_EQUAL_STRING("deadbeef", buf);
    TEST_ASSERT_EQUAL_INT(8, len);
}

void test_format_unsigned_hex_upper(void)
{
    char buf[16];
    int len = format_unsigned(buf, sizeof(buf), 0xDEADBEEF, 16, 1);
    TEST_ASSERT_EQUAL_STRING("DEADBEEF", buf);
    TEST_ASSERT_EQUAL_INT(8, len);
}

void test_format_unsigned_hex_zero(void)
{
    char buf[16];
    int len = format_unsigned(buf, sizeof(buf), 0, 16, 0);
    TEST_ASSERT_EQUAL_STRING("0", buf);
    TEST_ASSERT_EQUAL_INT(1, len);
}

void test_format_unsigned_small_buffer(void)
{
    char buf[4];
    int len = format_unsigned(buf, sizeof(buf), 12345, 10, 0);
    /* Should truncate to fit buffer (3 chars + null) */
    TEST_ASSERT_EQUAL_INT(3, len);
    TEST_ASSERT_EQUAL_STRING("123", buf);
}

void test_format_unsigned_zero_buffer(void)
{
    char buf[16] = "unchanged";
    int len = format_unsigned(buf, 0, 123, 10, 0);
    TEST_ASSERT_EQUAL_INT(0, len);
    TEST_ASSERT_EQUAL_STRING("unchanged", buf);
}

/*
 * =============================================================================
 * format_signed tests
 * =============================================================================
 */

void test_format_signed_positive(void)
{
    char buf[16];
    int len = format_signed(buf, sizeof(buf), 12345);
    TEST_ASSERT_EQUAL_STRING("12345", buf);
    TEST_ASSERT_EQUAL_INT(5, len);
}

void test_format_signed_negative(void)
{
    char buf[16];
    int len = format_signed(buf, sizeof(buf), -12345);
    TEST_ASSERT_EQUAL_STRING("-12345", buf);
    TEST_ASSERT_EQUAL_INT(6, len);
}

void test_format_signed_zero(void)
{
    char buf[16];
    int len = format_signed(buf, sizeof(buf), 0);
    TEST_ASSERT_EQUAL_STRING("0", buf);
    TEST_ASSERT_EQUAL_INT(1, len);
}

void test_format_signed_int32_max(void)
{
    char buf[16];
    int len = format_signed(buf, sizeof(buf), 2147483647);
    TEST_ASSERT_EQUAL_STRING("2147483647", buf);
    TEST_ASSERT_EQUAL_INT(10, len);
}

void test_format_signed_int32_min(void)
{
    char buf[16];
    /* INT32_MIN = -2147483648 = 0x80000000 */
    int len = format_signed(buf, sizeof(buf), (int32_t)0x80000000);
    TEST_ASSERT_EQUAL_STRING("-2147483648", buf);
    TEST_ASSERT_EQUAL_INT(11, len);
}

void test_format_signed_small_buffer(void)
{
    char buf[5];
    int len = format_signed(buf, sizeof(buf), -12345);
    /* Should truncate: "-123" (4 chars + null) */
    TEST_ASSERT_EQUAL_INT(4, len);
    TEST_ASSERT_EQUAL_STRING("-123", buf);
}

/*
 * =============================================================================
 * format_pointer tests
 * =============================================================================
 */

void test_format_pointer_regular(void)
{
    char buf[16];
    int len = format_pointer(buf, sizeof(buf), 0xC0100000);
    TEST_ASSERT_EQUAL_STRING("0xc0100000", buf);
    TEST_ASSERT_EQUAL_INT(10, len);
}

void test_format_pointer_null(void)
{
    char buf[16];
    int len = format_pointer(buf, sizeof(buf), 0);
    TEST_ASSERT_EQUAL_STRING("0x00000000", buf);
    TEST_ASSERT_EQUAL_INT(10, len);
}

void test_format_pointer_max(void)
{
    char buf[16];
    int len = format_pointer(buf, sizeof(buf), 0xFFFFFFFF);
    TEST_ASSERT_EQUAL_STRING("0xffffffff", buf);
    TEST_ASSERT_EQUAL_INT(10, len);
}

void test_format_pointer_small_buffer(void)
{
    char buf[6];
    int len = format_pointer(buf, sizeof(buf), 0xDEADBEEF);
    /* Should truncate: "0xdea" (5 chars + null) */
    TEST_ASSERT_EQUAL_INT(5, len);
    TEST_ASSERT_EQUAL_STRING("0xdea", buf);
}

/*
 * Main test runner
 */
int main(void)
{
    UNITY_BEGIN();

    /* format_unsigned tests */
    RUN_TEST(test_format_unsigned_zero);
    RUN_TEST(test_format_unsigned_decimal);
    RUN_TEST(test_format_unsigned_max);
    RUN_TEST(test_format_unsigned_hex_lower);
    RUN_TEST(test_format_unsigned_hex_upper);
    RUN_TEST(test_format_unsigned_hex_zero);
    RUN_TEST(test_format_unsigned_small_buffer);
    RUN_TEST(test_format_unsigned_zero_buffer);

    /* format_signed tests */
    RUN_TEST(test_format_signed_positive);
    RUN_TEST(test_format_signed_negative);
    RUN_TEST(test_format_signed_zero);
    RUN_TEST(test_format_signed_int32_max);
    RUN_TEST(test_format_signed_int32_min);
    RUN_TEST(test_format_signed_small_buffer);

    /* format_pointer tests */
    RUN_TEST(test_format_pointer_regular);
    RUN_TEST(test_format_pointer_null);
    RUN_TEST(test_format_pointer_max);
    RUN_TEST(test_format_pointer_small_buffer);

    return UNITY_END();
}
