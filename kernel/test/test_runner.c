/*
 * kernel/test/test_runner.c - In-kernel test harness
 *
 * Implements the test framework core functions and orchestrates
 * running all registered subsystem tests.
 *
 * Output is sent via serial (and VGA if available) in the format:
 *   [suite] Running tests...
 *   [PASS] test_name
 *   [FAIL] test_name: reason (file:line)
 *   [suite] X passed, Y failed
 */

#ifdef TEST_MODE

#include <test.h>
#include <types.h>

/* TODO: Include these once implemented
 * #include <kernel.h>
 * #include <lib/printf.h>
 */

/*
 * VGA text mode constants for test result display
 */
#define VGA_BUFFER      ((volatile uint16_t *)0xB8000)
#define VGA_WIDTH       80
#define VGA_COLOR_GREEN 0x0A    /* Light green on black */
#define VGA_COLOR_RED   0x0C    /* Light red on black */
#define VGA_COLOR_WHITE 0x0F    /* White on black */

/*
 * vga_put - Write a character to VGA at position
 */
static void vga_put(int pos, char c, uint8_t color)
{
    VGA_BUFFER[pos] = (uint16_t)c | ((uint16_t)color << 8);
}

/*
 * vga_puts - Write a string to VGA starting at position
 */
static void vga_puts(int pos, const char *s, uint8_t color)
{
    while (*s) {
        vga_put(pos++, *s++, color);
    }
}

/*
 * vga_put_num - Write a number to VGA (0-999)
 */
static int vga_put_num(int pos, int num, uint8_t color)
{
    if (num >= 100) {
        vga_put(pos++, '0' + (num / 100), color);
        num %= 100;
        vga_put(pos++, '0' + (num / 10), color);
        vga_put(pos++, '0' + (num % 10), color);
    } else if (num >= 10) {
        vga_put(pos++, '0' + (num / 10), color);
        vga_put(pos++, '0' + (num % 10), color);
    } else {
        vga_put(pos++, '0' + num, color);
    }
    return pos;
}

/*
 * Global test statistics
 */
int test_passed_count = 0;
int test_failed_count = 0;
const char *test_current_suite = "unknown";

/* Per-suite statistics */
static int suite_passed = 0;
static int suite_failed = 0;

/*
 * Forward declarations for subsystem test functions
 * Add new test functions here as subsystems are implemented
 */

/* Story 1.3: Boot verification */
extern void test_boot(void);

/* Milestone 3: Memory Management */
/* extern void test_pmm(void); */
/* extern void test_bitmap(void); */

/* Milestone 4: Paging */
/* extern void test_vmm(void); */

/* Milestone 5-6: Process Management */
/* extern void test_sched(void); */

/* Kernel library functions */
/* extern void test_string(void); */

/*
 * Stub printk until kernel/lib/printf.c is implemented
 * Replace with actual printk once available
 */
static void test_printk(const char *fmt, ...)
{
    /*
     * TODO: Replace with actual printk call
     * For now, this is a placeholder that will be replaced
     * once serial output is available.
     *
     * When serial driver is ready:
     *   #include <lib/printf.h>
     *   printk(LOG_INFO, fmt, ...);
     */
    (void)fmt;
}

/*
 * test_begin - Start a test suite
 *
 * Resets per-suite counters and prints suite header.
 */
void test_begin(const char *suite_name)
{
    test_current_suite = suite_name;
    suite_passed = 0;
    suite_failed = 0;
    test_printk("[%s] Running tests...\n", suite_name);
}

/*
 * test_end - End a test suite
 *
 * Prints suite summary and accumulates global counters.
 */
void test_end(void)
{
    test_printk("[%s] %d passed, %d failed\n\n",
                test_current_suite, suite_passed, suite_failed);
    test_passed_count += suite_passed;
    test_failed_count += suite_failed;
}

/*
 * test_pass - Record a passing test
 */
void test_pass(const char *name)
{
    test_printk("[PASS] %s\n", name);
    suite_passed++;
}

/*
 * test_fail - Record a failing test
 */
void test_fail(const char *name, const char *reason,
               const char *file, int line)
{
    test_printk("[FAIL] %s: %s (%s:%d)\n", name, reason, file, line);
    suite_failed++;
}

/*
 * test_run_all - Execute all registered test suites
 *
 * Called from kmain() when TEST_MODE is enabled.
 * After all tests complete, the kernel halts.
 */
void test_run_all(void)
{
    test_printk("\n");
    test_printk("========================================\n");
    test_printk("       OS-DEV KERNEL TEST SUITE        \n");
    test_printk("========================================\n\n");

    test_passed_count = 0;
    test_failed_count = 0;

    /*
     * Run all subsystem tests
     * Uncomment as subsystems are implemented
     */

    /* Story 1.3: Boot verification */
    test_boot();

    /* Milestone 3: Memory */
    /* test_pmm(); */
    /* test_bitmap(); */

    /* Milestone 4: Paging */
    /* test_vmm(); */

    /* Milestone 5-6: Processes */
    /* test_sched(); */

    /* Kernel library */
    /* test_string(); */

    /* Print final summary */
    test_printk("========================================\n");
    test_printk("  TOTAL: %d passed, %d failed\n",
                test_passed_count, test_failed_count);
    test_printk("========================================\n");

    /*
     * Display test results on VGA (visible until serial driver ready)
     *
     * Format on line 2 (below "OK" and "M:X"):
     *   TEST: X passed, Y failed [PASS] or [FAIL]
     */
    int pos = VGA_WIDTH * 2;  /* Start at line 2 */
    vga_puts(pos, "TEST: ", VGA_COLOR_WHITE);
    pos += 6;
    pos = vga_put_num(pos, test_passed_count, VGA_COLOR_GREEN);
    vga_puts(pos, " passed, ", VGA_COLOR_WHITE);
    pos += 9;
    pos = vga_put_num(pos, test_failed_count,
                      test_failed_count > 0 ? VGA_COLOR_RED : VGA_COLOR_WHITE);
    vga_puts(pos, " failed ", VGA_COLOR_WHITE);
    pos += 8;

    if (test_failed_count > 0) {
        test_printk("\n*** TESTS FAILED ***\n");
        /* Display [FAIL] in red */
        vga_puts(pos, "[FAIL]", VGA_COLOR_RED);

        /*
         * Halt on failure - don't let kernel continue with broken state
         */
        test_printk("\nTest failure. Halting.\n");
        while (1) {
            __asm__ volatile ("hlt");
        }
    } else {
        test_printk("\n*** ALL TESTS PASSED ***\n");
        /* Display [PASS] in green */
        vga_puts(pos, "[PASS]", VGA_COLOR_GREEN);
    }

    /*
     * Tests passed - return to kmain for normal operation
     */
    test_printk("\nTest run complete.\n");
}

#endif /* TEST_MODE */
