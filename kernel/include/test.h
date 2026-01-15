/*
 * kernel/include/test.h - In-kernel test framework
 *
 * Provides assertion macros and test harness for kernel subsystem testing.
 * All test code is conditionally compiled with TEST_MODE.
 *
 * Usage:
 *   void test_pmm(void)
 *   {
 *       TEST_BEGIN("pmm");
 *       TEST_ASSERT(pmm_alloc_frame() != 0);
 *       TEST_ASSERT_EQ(expected, actual);
 *       TEST_END();
 *   }
 *
 * Output format:
 *   [PASS] test_name
 *   [FAIL] test_name: reason (file:line)
 */

#ifndef KERNEL_INCLUDE_TEST_H
#define KERNEL_INCLUDE_TEST_H

#ifdef TEST_MODE

#include <types.h>

/*
 * Test statistics - tracked globally during test run
 */
extern int test_passed_count;
extern int test_failed_count;
extern const char *test_current_suite;

/*
 * Test framework core functions
 * Implemented in kernel/test/test_runner.c
 */
void test_begin(const char *suite_name);
void test_end(void);
void test_pass(const char *name);
void test_fail(const char *name, const char *reason, const char *file, int line);
void test_run_all(void);

/*
 * TEST_BEGIN - Start a test suite
 * Call at the beginning of each test_<subsystem>() function
 */
#define TEST_BEGIN(suite) test_begin(suite)

/*
 * TEST_END - End a test suite
 * Call at the end of each test_<subsystem>() function
 */
#define TEST_END() test_end()

/*
 * TEST_ASSERT - Assert a condition is true
 */
#define TEST_ASSERT(cond) \
    do { \
        if (cond) { \
            test_pass(#cond); \
        } else { \
            test_fail(#cond, "condition false", __FILE__, __LINE__); \
        } \
    } while (0)

/*
 * TEST_ASSERT_MSG - Assert with custom failure message
 */
#define TEST_ASSERT_MSG(cond, msg) \
    do { \
        if (cond) { \
            test_pass(#cond); \
        } else { \
            test_fail(#cond, msg, __FILE__, __LINE__); \
        } \
    } while (0)

/*
 * TEST_ASSERT_EQ - Assert two values are equal
 */
#define TEST_ASSERT_EQ(expected, actual) \
    do { \
        if ((expected) == (actual)) { \
            test_pass(#actual " == " #expected); \
        } else { \
            test_fail(#actual " == " #expected, "values not equal", \
                      __FILE__, __LINE__); \
        } \
    } while (0)

/*
 * TEST_ASSERT_NEQ - Assert two values are not equal
 */
#define TEST_ASSERT_NEQ(a, b) \
    do { \
        if ((a) != (b)) { \
            test_pass(#a " != " #b); \
        } else { \
            test_fail(#a " != " #b, "values are equal", __FILE__, __LINE__); \
        } \
    } while (0)

/*
 * TEST_ASSERT_NULL - Assert pointer is NULL
 */
#define TEST_ASSERT_NULL(ptr) \
    do { \
        if ((ptr) == ((void *)0)) { \
            test_pass(#ptr " is NULL"); \
        } else { \
            test_fail(#ptr " is NULL", "pointer not null", __FILE__, __LINE__); \
        } \
    } while (0)

/*
 * TEST_ASSERT_NOT_NULL - Assert pointer is not NULL
 */
#define TEST_ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) != ((void *)0)) { \
            test_pass(#ptr " not NULL"); \
        } else { \
            test_fail(#ptr " not NULL", "pointer is null", __FILE__, __LINE__); \
        } \
    } while (0)

/*
 * TEST_ASSERT_GT - Assert a > b
 */
#define TEST_ASSERT_GT(a, b) \
    do { \
        if ((a) > (b)) { \
            test_pass(#a " > " #b); \
        } else { \
            test_fail(#a " > " #b, "not greater than", __FILE__, __LINE__); \
        } \
    } while (0)

/*
 * TEST_ASSERT_GTE - Assert a >= b
 */
#define TEST_ASSERT_GTE(a, b) \
    do { \
        if ((a) >= (b)) { \
            test_pass(#a " >= " #b); \
        } else { \
            test_fail(#a " >= " #b, "not greater or equal", __FILE__, __LINE__); \
        } \
    } while (0)

/*
 * TEST_ASSERT_LT - Assert a < b
 */
#define TEST_ASSERT_LT(a, b) \
    do { \
        if ((a) < (b)) { \
            test_pass(#a " < " #b); \
        } else { \
            test_fail(#a " < " #b, "not less than", __FILE__, __LINE__); \
        } \
    } while (0)

/*
 * TEST_ASSERT_LTE - Assert a <= b
 */
#define TEST_ASSERT_LTE(a, b) \
    do { \
        if ((a) <= (b)) { \
            test_pass(#a " <= " #b); \
        } else { \
            test_fail(#a " <= " #b, "not less or equal", __FILE__, __LINE__); \
        } \
    } while (0)

/*
 * TEST_FAIL - Unconditional failure
 */
#define TEST_FAIL(msg) \
    test_fail("explicit fail", msg, __FILE__, __LINE__)

/*
 * TEST_SKIP - Skip a test (counts as pass with note)
 */
#define TEST_SKIP(reason) \
    test_pass("SKIP: " reason)

#else /* !TEST_MODE */

/*
 * When TEST_MODE is not defined, all test macros compile to nothing
 */
#define TEST_BEGIN(suite)           ((void)0)
#define TEST_END()                  ((void)0)
#define TEST_ASSERT(cond)           ((void)0)
#define TEST_ASSERT_MSG(cond, msg)  ((void)0)
#define TEST_ASSERT_EQ(exp, act)    ((void)0)
#define TEST_ASSERT_NEQ(a, b)       ((void)0)
#define TEST_ASSERT_NULL(ptr)       ((void)0)
#define TEST_ASSERT_NOT_NULL(ptr)   ((void)0)
#define TEST_ASSERT_GT(a, b)        ((void)0)
#define TEST_ASSERT_GTE(a, b)       ((void)0)
#define TEST_ASSERT_LT(a, b)        ((void)0)
#define TEST_ASSERT_LTE(a, b)       ((void)0)
#define TEST_FAIL(msg)              ((void)0)
#define TEST_SKIP(reason)           ((void)0)

static inline void test_run_all(void) { }

#endif /* TEST_MODE */

#endif /* KERNEL_INCLUDE_TEST_H */
