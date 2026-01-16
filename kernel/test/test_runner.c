/*
 * kernel/test/test_runner.c - In-kernel test harness
 *
 * Implements the test framework core functions and orchestrates
 * running all registered subsystem tests.
 *
 * Output is sent via printk to both serial and VGA in the format:
 *   [suite] Running tests...
 *   [PASS] test_name
 *   [FAIL] test_name: reason (file:line)
 *   [suite] X passed, Y failed
 */

#ifdef TEST_MODE

#include <test.h>
#include <types.h>
#include <printk.h>
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

/* Story 1.6: Serial debug and printk */
extern void test_serial(void);
extern void test_printk(void);

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
 * test_begin - Start a test suite
 *
 * Resets per-suite counters and prints suite header.
 */
void test_begin(const char *suite_name)
{
    test_current_suite = suite_name;
    suite_passed = 0;
    suite_failed = 0;

    printk(LOG_INFO, "[%s] Running tests...\n", suite_name);
}

/*
 * test_end - End a test suite
 *
 * Prints suite summary and accumulates global counters.
 */
void test_end(void)
{
    printk(LOG_INFO, "[%s] %d passed, %d failed\n",
           test_current_suite, suite_passed, suite_failed);

    test_passed_count += suite_passed;
    test_failed_count += suite_failed;
}

/*
 * test_pass - Record a passing test
 */
void test_pass(const char *name)
{
    printk(LOG_INFO, "[PASS] %s\n", name);
    suite_passed++;
}

/*
 * test_fail - Record a failing test
 */
void test_fail(const char *name, const char *reason,
               const char *file, int line)
{
    printk(LOG_ERROR, "[FAIL] %s: %s (%s:%d)\n", name, reason, file, line);
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
    printk(LOG_INFO, "\n========================================\n");
    printk(LOG_INFO, "       OS-DEV KERNEL TEST SUITE        \n");
    printk(LOG_INFO, "========================================\n\n");

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

    /* Story 1.6: Serial debug and printk */
    test_serial();
    test_printk();

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
    printk(LOG_INFO, "\n========================================\n");
    printk(LOG_INFO, "  TOTAL: %d passed, %d failed\n",
           test_passed_count, test_failed_count);
    printk(LOG_INFO, "========================================\n\n");

    if (test_failed_count > 0) {
        printk(LOG_ERROR, "*** TESTS FAILED ***\n");

        /*
         * Halt on failure - don't let kernel continue with broken state
         */
        while (1) {
            hlt();
        }
    } else {
        printk(LOG_INFO, "*** ALL TESTS PASSED ***\n");
    }
}

#endif /* TEST_MODE */
