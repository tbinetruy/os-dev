/*
 * kernel/test/test_runner.c - In-kernel test harness
 *
 * Implements the test framework core functions and orchestrates
 * running all registered subsystem tests.
 *
 * Output is sent via VGA (and serial once available) in the format:
 *   [suite] Running tests...
 *   [PASS] test_name
 *   [FAIL] test_name: reason (file:line)
 *   [suite] X passed, Y failed
 */

#ifdef TEST_MODE

#include <test.h>
#include <types.h>
#include <vga.h>
#include <asm.h>

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

/* Story 1.4: GDT setup */
extern void test_gdt(void);

/* Story 1.5: VGA text mode driver */
extern void test_vga(void);

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
 * test_print_num - Print a number (0-999) to VGA
 *
 * Helper until printk is available.
 *
 * Note: Similar to print_num() in main.c. Both are temporary helpers
 * until printk is implemented in Story 1.6. Kept separate to avoid
 * cross-dependencies between test infrastructure and boot code.
 */
static void test_print_num(int num)
{
    if (num >= 100) {
        vga_putchar('0' + (num / 100));
        num %= 100;
        vga_putchar('0' + (num / 10));
        vga_putchar('0' + (num % 10));
    } else if (num >= 10) {
        vga_putchar('0' + (num / 10));
        vga_putchar('0' + (num % 10));
    } else {
        vga_putchar('0' + num);
    }
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

    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_putchar('[');
    vga_puts(suite_name);
    vga_puts("] Running tests...\n");
}

/*
 * test_end - End a test suite
 *
 * Prints suite summary and accumulates global counters.
 */
void test_end(void)
{
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_putchar('[');
    vga_puts(test_current_suite);
    vga_puts("] ");
    test_print_num(suite_passed);
    vga_puts(" passed, ");
    test_print_num(suite_failed);
    vga_puts(" failed\n");

    test_passed_count += suite_passed;
    test_failed_count += suite_failed;
}

/*
 * test_pass - Record a passing test
 */
void test_pass(const char *name)
{
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("[PASS] ");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts(name);
    vga_putchar('\n');
    suite_passed++;
}

/*
 * test_fail - Record a failing test
 */
void test_fail(const char *name, const char *reason,
               const char *file, int line)
{
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_puts("[FAIL] ");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts(name);
    vga_puts(": ");
    vga_puts(reason);
    vga_puts(" (");
    vga_puts(file);
    vga_putchar(':');
    test_print_num(line);
    vga_puts(")\n");
    suite_failed++;
}

/*
 * test_run_all - Execute all registered test suites
 *
 * Called from kmain() when TEST_MODE is enabled.
 * After all tests complete, the kernel halts on failure.
 */
void test_run_all(void)
{
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts("\n========================================\n");
    vga_puts("       OS-DEV KERNEL TEST SUITE        \n");
    vga_puts("========================================\n\n");

    test_passed_count = 0;
    test_failed_count = 0;

    /*
     * Run all subsystem tests
     * Uncomment as subsystems are implemented
     */

    /* Story 1.3: Boot verification */
    test_boot();

    /* Story 1.4: GDT setup */
    test_gdt();

    /* Story 1.5: VGA text mode driver */
    test_vga();

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
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts("\n========================================\n");
    vga_puts("  TOTAL: ");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    test_print_num(test_passed_count);
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts(" passed, ");

    if (test_failed_count > 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    }
    test_print_num(test_failed_count);
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts(" failed\n");
    vga_puts("========================================\n\n");

    if (test_failed_count > 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("*** TESTS FAILED ***\n");

        /*
         * Halt on failure - don't let kernel continue with broken state
         */
        while (1) {
            hlt();
        }
    } else {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_puts("*** ALL TESTS PASSED ***\n");
    }

    /* Reset color for subsequent output */
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

#endif /* TEST_MODE */
