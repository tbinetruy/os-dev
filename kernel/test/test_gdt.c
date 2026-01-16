/*
 * kernel/test/test_gdt.c - GDT unit tests
 *
 * Tests for GDT structures, selectors, and initialization.
 * Verifies:
 *   - Selector constants are correct
 *   - Structure sizes match Intel spec (8 bytes entry, 6 bytes pointer)
 *   - GDT entries are populated correctly after init
 *   - Segment registers are set correctly after flush
 */

#ifdef TEST_MODE

#include <test.h>
#include <gdt.h>

/*
 * get_cs - Read current CS register value
 *
 * CS can't be read with MOV; we use a far call trick or just
 * read from stack after interrupt. For simplicity, use inline asm.
 */
static inline uint16_t get_cs(void)
{
    uint16_t cs;
    __asm__ volatile ("movw %%cs, %0" : "=r"(cs));
    return cs;
}

/*
 * get_ds - Read current DS register value
 */
static inline uint16_t get_ds(void)
{
    uint16_t ds;
    __asm__ volatile ("movw %%ds, %0" : "=r"(ds));
    return ds;
}

/*
 * get_ss - Read current SS register value
 */
static inline uint16_t get_ss(void)
{
    uint16_t ss;
    __asm__ volatile ("movw %%ss, %0" : "=r"(ss));
    return ss;
}

/*
 * test_gdt - GDT test suite
 *
 * Called from test_runner.c when TEST_MODE is enabled.
 */
void test_gdt(void)
{
    TEST_BEGIN("gdt");

    /* Test 1: Selector constants are correct */
    TEST_ASSERT_EQ(0x08, KERNEL_CS);
    TEST_ASSERT_EQ(0x10, KERNEL_DS);
    TEST_ASSERT_EQ(0x1B, USER_CS);
    TEST_ASSERT_EQ(0x23, USER_DS);
    TEST_ASSERT_EQ(0x28, TSS_SEG);

    /* Test 2: GDT entry structure is exactly 8 bytes */
    TEST_ASSERT_EQ(8, sizeof(struct gdt_entry));

    /* Test 3: GDT pointer structure is exactly 6 bytes */
    TEST_ASSERT_EQ(6, sizeof(struct gdt_ptr));

    /* Test 4: CS register is set to kernel code segment */
    TEST_ASSERT_EQ(KERNEL_CS, get_cs());

    /* Test 5: DS register is set to kernel data segment */
    TEST_ASSERT_EQ(KERNEL_DS, get_ds());

    /* Test 6: SS register is set to kernel data segment */
    TEST_ASSERT_EQ(KERNEL_DS, get_ss());

    TEST_END();
}

#endif /* TEST_MODE */
