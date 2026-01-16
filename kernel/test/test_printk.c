/*
 * kernel/test/test_printk.c - printk format string tests
 *
 * Tests for the printk logging infrastructure.
 * Verifies all supported format specifiers work correctly.
 *
 * Output is sent to both serial and VGA, so verification
 * can be done by checking QEMU's serial console.
 */

#ifdef TEST_MODE

#include <test.h>
#include <printk.h>
#include <types.h>

/*
 * test_printk_string - Test %s format specifier
 */
static void test_printk_string(void)
{
    printk(LOG_DEBUG, "String test: %s\n", "hello");
    printk(LOG_DEBUG, "NULL string: %s\n", (char *)NULL);

    test_pass("printk %s");
}

/*
 * test_printk_decimal - Test %d and %u format specifiers
 */
static void test_printk_decimal(void)
{
    printk(LOG_DEBUG, "Signed positive: %d\n", 12345);
    printk(LOG_DEBUG, "Signed negative: %d\n", -12345);
    printk(LOG_DEBUG, "Signed zero: %d\n", 0);
    printk(LOG_DEBUG, "Unsigned: %u\n", 4294967295U);
    /* INT32_MIN edge case - has special handling to avoid overflow */
    printk(LOG_DEBUG, "INT32_MIN: %d\n", (int32_t)0x80000000);

    test_pass("printk %d/%u");
}

/*
 * test_printk_hex - Test %x and %X format specifiers
 */
static void test_printk_hex(void)
{
    printk(LOG_DEBUG, "Hex lower: %x\n", 0xDEADBEEF);
    printk(LOG_DEBUG, "Hex upper: %X\n", 0xDEADBEEF);
    printk(LOG_DEBUG, "Hex zero: %x\n", 0);

    test_pass("printk %x/%X");
}

/*
 * test_printk_char - Test %c format specifier
 */
static void test_printk_char(void)
{
    printk(LOG_DEBUG, "Char: %c%c%c\n", 'A', 'B', 'C');

    test_pass("printk %c");
}

/*
 * test_printk_pointer - Test %p format specifier
 */
static void test_printk_pointer(void)
{
    void *ptr = (void *)0xC0100000;
    printk(LOG_DEBUG, "Pointer: %p\n", ptr);
    printk(LOG_DEBUG, "NULL pointer: %p\n", NULL);

    test_pass("printk %p");
}

/*
 * test_printk_percent - Test %% escape sequence
 */
static void test_printk_percent(void)
{
    printk(LOG_DEBUG, "Percent: 100%% complete\n");

    test_pass("printk %%");
}

/*
 * test_printk_levels - Test all log levels
 */
static void test_printk_levels(void)
{
    printk(LOG_ERROR, "Error level message\n");
    printk(LOG_WARN, "Warning level message\n");
    printk(LOG_INFO, "Info level message\n");
    printk(LOG_DEBUG, "Debug level message\n");

    test_pass("printk log levels");
}

/*
 * test_printk - printk test suite entry point
 *
 * Called from test_runner.c when tests are executed.
 */
void test_printk(void)
{
    TEST_BEGIN("printk");

    test_printk_string();
    test_printk_decimal();
    test_printk_hex();
    test_printk_char();
    test_printk_pointer();
    test_printk_percent();
    test_printk_levels();

    TEST_END();
}

#endif /* TEST_MODE */
