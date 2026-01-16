/*
 * tests/host/test_example.c - Example host-side unit test
 *
 * Demonstrates Unity test framework usage for testing pure algorithms
 * that can be extracted from the kernel and run on the host.
 *
 * Build: make (in tests/ directory)
 * Run: ./test_example
 */

#include "unity/unity.h"

/* Test setup/teardown - called before/after each test */
void setUp(void)
{
    /* Initialize test fixtures here */
}

void tearDown(void)
{
    /* Clean up test fixtures here */
}

/*
 * Example tests demonstrating Unity assertions
 */

void test_basic_assertion(void)
{
    TEST_ASSERT_TRUE(1 == 1);
    TEST_ASSERT_FALSE(1 == 0);
}

void test_integer_equality(void)
{
    int expected = 42;
    int actual = 42;
    TEST_ASSERT_EQUAL_INT(expected, actual);
}

void test_integer_comparison(void)
{
    TEST_ASSERT_GREATER_THAN(5, 10);
    TEST_ASSERT_LESS_THAN(10, 5);
    TEST_ASSERT_INT_WITHIN(2, 10, 11);  /* 11 is within 2 of 10 */
}

void test_pointer_checks(void)
{
    int value = 100;
    int *ptr = &value;
    int *null_ptr = NULL;

    TEST_ASSERT_NOT_NULL(ptr);
    TEST_ASSERT_NULL(null_ptr);
}

void test_hex_values(void)
{
    uint32_t flags = 0xDEADBEEF;
    TEST_ASSERT_EQUAL_HEX32(0xDEADBEEF, flags);
}

void test_memory_comparison(void)
{
    uint8_t expected[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t actual[] = {0x01, 0x02, 0x03, 0x04};
    TEST_ASSERT_EQUAL_MEMORY(expected, actual, sizeof(expected));
}

void test_string_comparison(void)
{
    const char *expected = "hello";
    const char *actual = "hello";
    TEST_ASSERT_EQUAL_STRING(expected, actual);
}

/*
 * Main test runner
 */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_basic_assertion);
    RUN_TEST(test_integer_equality);
    RUN_TEST(test_integer_comparison);
    RUN_TEST(test_pointer_checks);
    RUN_TEST(test_hex_values);
    RUN_TEST(test_memory_comparison);
    RUN_TEST(test_string_comparison);

    return UNITY_END();
}
