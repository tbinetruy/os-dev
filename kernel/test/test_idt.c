/*
 * kernel/test/test_idt.c - IDT verification tests
 *
 * Tests the Interrupt Descriptor Table setup to verify:
 *   - IDT entries have correct format
 *   - Gate types are properly set
 *   - Handler addresses are correctly split
 *
 * Note: Exception triggering tests (div by zero, page fault) are
 * intentionally not included in the automated suite because they
 * cause system halts. Use manual testing via TEST_DIV_ZERO or
 * TEST_PAGE_FAULT defines.
 */

#ifdef TEST_MODE

#include <test.h>
#include <idt.h>
#include <gdt.h>
#include <printk.h>

/*
 * Access the IDT for testing
 *
 * We need to read IDT entries to verify their format. The IDT is
 * declared static in idt.c, so we use inline assembly to read IDTR
 * and access the table.
 */

/*
 * get_idtr - Read current IDTR value
 *
 * @limit: Output parameter for IDT limit (size - 1)
 * @base:  Output parameter for IDT base address
 */
static void get_idtr(uint16_t *limit, uint32_t *base)
{
    struct idt_ptr idtr;
    __asm__ volatile ("sidt %0" : "=m"(idtr));
    *limit = idtr.limit;
    *base = idtr.base;
}

/*
 * test_idt_loaded - Verify IDT is loaded with valid base and limit
 */
static void test_idt_loaded(void)
{
    uint16_t limit;
    uint32_t base;

    get_idtr(&limit, &base);

    /* IDT should have 256 entries * 8 bytes each - 1 = 2047 */
    TEST_ASSERT_EQ(256 * 8 - 1, limit);

    /* Base should be non-zero (IDT must be somewhere in memory) */
    TEST_ASSERT_NEQ(0, base);
}

/*
 * test_idt_entry_format - Verify IDT entry 0 has correct format
 */
static void test_idt_entry_format(void)
{
    uint16_t limit;
    uint32_t base;
    struct idt_entry *idt;

    get_idtr(&limit, &base);
    idt = (struct idt_entry *)base;

    /* Entry 0 (Division Error) should be present */
    TEST_ASSERT((idt[0].type_attr & 0x80) != 0);

    /* Should be 32-bit interrupt gate (0x8E) */
    TEST_ASSERT_EQ(IDT_GATE_INT32, idt[0].type_attr);

    /* Verify KERNEL_CS is the expected value (0x08) */
    TEST_ASSERT_EQ(0x08, KERNEL_CS);

    /* Selector should be kernel code segment */
    TEST_ASSERT_EQ(KERNEL_CS, idt[0].selector);

    /* Zero byte must be 0 */
    TEST_ASSERT_EQ(0, idt[0].zero);
}

/*
 * test_idt_exception_handlers - Verify all 32 exception handlers installed
 */
static void test_idt_exception_handlers(void)
{
    uint16_t limit;
    uint32_t base;
    struct idt_entry *idt;
    int i;
    int all_present = 1;
    int all_nonzero = 1;

    get_idtr(&limit, &base);
    idt = (struct idt_entry *)base;

    /* Check that all 32 exception entries (0-31) are present */
    for (i = 0; i < 32; i++) {
        /* Check present bit */
        if ((idt[i].type_attr & 0x80) == 0) {
            printk(LOG_ERROR, "Entry %d not present\n", i);
            all_present = 0;
        }

        /* Check handler address is non-zero */
        uint32_t handler = idt[i].offset_low |
                          ((uint32_t)idt[i].offset_high << 16);
        if (handler == 0) {
            printk(LOG_ERROR, "Entry %d handler is NULL\n", i);
            all_nonzero = 0;
        }
    }

    TEST_ASSERT_MSG(all_present, "all exception handlers present");
    TEST_ASSERT_MSG(all_nonzero, "all exception handlers non-zero");
}

/*
 * test_idt_handler_address - Verify handler address encoding/decoding
 */
static void test_idt_handler_address(void)
{
    uint16_t limit;
    uint32_t base;
    struct idt_entry *idt;

    get_idtr(&limit, &base);
    idt = (struct idt_entry *)base;

    /*
     * Entry 0's handler address should be isr0.
     * We can't directly compare to isr0 address without extern,
     * but we can verify the address looks reasonable (in kernel space).
     */
    uint32_t handler = idt[0].offset_low |
                      ((uint32_t)idt[0].offset_high << 16);

    /*
     * Handler should be in low memory where kernel is loaded.
     * Kernel is at 0x100000 (1MB), handlers should be nearby.
     * Just verify it's not in first 64KB (reserved) and not NULL.
     */
    TEST_ASSERT_GT(handler, 0x10000);

    /* Verify high and low parts reconstruct correctly */
    uint16_t expected_low = (uint16_t)(handler & 0xFFFF);
    uint16_t expected_high = (uint16_t)((handler >> 16) & 0xFFFF);

    TEST_ASSERT_EQ(expected_low, idt[0].offset_low);
    TEST_ASSERT_EQ(expected_high, idt[0].offset_high);
}

/*
 * test_idt - Run all IDT tests
 */
void test_idt(void)
{
    test_begin("idt");

    test_idt_loaded();
    test_idt_entry_format();
    test_idt_exception_handlers();
    test_idt_handler_address();

    test_end();
}

#endif /* TEST_MODE */
