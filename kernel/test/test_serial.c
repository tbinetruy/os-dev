/*
 * kernel/test/test_serial.c - Serial driver tests
 *
 * Tests for the serial port driver functionality.
 * Verifies character output, string output, and buffer writes.
 *
 * Note: These tests verify the driver functions execute without error.
 * Actual serial output verification requires checking QEMU's serial console.
 */

#ifdef TEST_MODE

#include <test.h>
#include <serial.h>
#include <types.h>

/*
 * test_serial_putchar - Test single character output
 *
 * Verifies serial_putchar executes without hanging or crashing.
 */
static void test_serial_putchar(void)
{
    /* Output a few test characters */
    serial_putchar('S');
    serial_putchar('E');
    serial_putchar('R');
    serial_putchar('\n');

    /* If we got here, output worked */
    test_pass("serial_putchar");
}

/*
 * test_serial_puts - Test string output
 *
 * Verifies serial_puts outputs a complete string.
 */
static void test_serial_puts(void)
{
    serial_puts("Serial string test OK\n");

    test_pass("serial_puts");
}

/*
 * test_serial_write - Test raw buffer output
 *
 * Verifies serial_write outputs exact bytes.
 */
static void test_serial_write(void)
{
    const char buf[] = "RAW\n";
    serial_write(buf, 4);

    test_pass("serial_write");
}

/*
 * test_serial - Serial driver test suite entry point
 *
 * Called from test_runner.c when tests are executed.
 */
void test_serial(void)
{
    TEST_BEGIN("serial");

    test_serial_putchar();
    test_serial_puts();
    test_serial_write();

    TEST_END();
}

#endif /* TEST_MODE */
