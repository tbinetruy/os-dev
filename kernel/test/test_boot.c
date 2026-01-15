/*
 * kernel/test/test_boot.c - Story 1.3 Boot Tests
 *
 * Tests for verifying the boot process completed successfully:
 *   - A20 line is enabled
 *   - Protected mode is active
 *   - Kernel is at correct address
 *   - Memory map was retrieved
 *   - GDT is loaded
 *
 * These tests run in-kernel after boot to verify the bootloader
 * set everything up correctly.
 */

#ifdef TEST_MODE

#include <test.h>
#include <types.h>

/* Boot parameters from entry.S */
extern uint32_t boot_mmap_ptr;
extern uint32_t boot_mmap_count;

/*
 * get_cr0 - Read CR0 control register
 *
 * CR0 bit 0 (PE) indicates protected mode is active.
 */
static inline uint32_t get_cr0(void)
{
    uint32_t cr0;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    return cr0;
}

/*
 * get_gdtr - Read GDTR (GDT Register)
 *
 * Returns the GDT limit and base address.
 */
static void get_gdtr(uint16_t *limit, uint32_t *base)
{
    struct {
        uint16_t limit;
        uint32_t base;
    } __attribute__((packed)) gdtr;

    __asm__ volatile ("sgdt %0" : "=m"(gdtr));
    *limit = gdtr.limit;
    *base = gdtr.base;
}

/*
 * test_a20_enabled - Verify A20 line is enabled
 *
 * If A20 is disabled, addresses above 1MB wrap around.
 * We test by checking if we can access distinct memory above 1MB.
 */
static void test_a20_enabled(void)
{
    /*
     * Write a value to address 0x100000 (1MB) and verify it doesn't
     * appear at address 0x000000 (which would indicate wrapping).
     *
     * Use volatile to prevent compiler optimization.
     */
    volatile uint32_t *addr_low = (volatile uint32_t *)0x000500;
    volatile uint32_t *addr_high = (volatile uint32_t *)0x100500;

    uint32_t saved_low = *addr_low;
    uint32_t saved_high = *addr_high;

    /* Write different values */
    *addr_low = 0xDEADBEEF;
    *addr_high = 0xCAFEBABE;

    /* If A20 disabled, addr_high would have overwritten addr_low */
    int a20_enabled = (*addr_low == 0xDEADBEEF);

    /* Restore original values */
    *addr_low = saved_low;
    *addr_high = saved_high;

    TEST_ASSERT_MSG(a20_enabled, "A20 line not enabled - memory wrapping detected");
}

/*
 * test_protected_mode - Verify CPU is in protected mode
 *
 * Check CR0.PE (Protection Enable) bit is set.
 */
static void test_protected_mode(void)
{
    uint32_t cr0 = get_cr0();

    /* Bit 0 is PE (Protection Enable) */
    TEST_ASSERT_MSG((cr0 & 0x1) != 0, "CR0.PE bit not set - not in protected mode");
}

/*
 * test_kernel_address - Verify kernel is at correct address
 *
 * The _start symbol should be at 0x100000 (1MB).
 */
extern void _start(void);  /* From entry.S */

static void test_kernel_address(void)
{
    uint32_t start_addr = (uint32_t)&_start;

    TEST_ASSERT_MSG(start_addr == 0x100000,
                    "Kernel _start not at 0x100000");
}

/*
 * test_memory_map - Verify BIOS memory map was retrieved
 *
 * The bootloader should have queried E820 and stored entries.
 */
static void test_memory_map(void)
{
    /* Memory map pointer should point to valid location */
    TEST_ASSERT_MSG(boot_mmap_ptr == 0x504,
                    "Memory map pointer not at expected address 0x504");

    /* Should have at least 1 memory map entry */
    TEST_ASSERT_MSG(boot_mmap_count > 0,
                    "No memory map entries - E820 failed");

    /* Typical systems have 3-20 entries, sanity check upper bound */
    TEST_ASSERT_MSG(boot_mmap_count < 100,
                    "Suspiciously high memory map count");
}

/*
 * test_gdt_loaded - Verify GDT is loaded
 *
 * Check that GDTR contains a valid GDT pointer.
 */
static void test_gdt_loaded(void)
{
    uint16_t limit;
    uint32_t base;

    get_gdtr(&limit, &base);

    /* GDT should have at least 3 entries (null + code + data) = 24 bytes */
    /* Limit is size - 1, so minimum is 23 */
    TEST_ASSERT_MSG(limit >= 23, "GDT too small - less than 3 entries");

    /* Base should be in reasonable memory range (stage2 area or kernel) */
    TEST_ASSERT_MSG(base != 0, "GDT base address is NULL");
}

/*
 * test_segments - Verify segment registers are set correctly
 *
 * All data segments should have selector 0x10 (DATA_SEG).
 */
static void test_segments(void)
{
    uint16_t ds, es, fs, gs, ss;

    __asm__ volatile (
        "mov %%ds, %0\n"
        "mov %%es, %1\n"
        "mov %%fs, %2\n"
        "mov %%gs, %3\n"
        "mov %%ss, %4\n"
        : "=r"(ds), "=r"(es), "=r"(fs), "=r"(gs), "=r"(ss)
    );

    /* All should be DATA_SEG (0x10) */
    TEST_ASSERT_EQ(0x10, ds);
    TEST_ASSERT_EQ(0x10, es);
    TEST_ASSERT_EQ(0x10, fs);
    TEST_ASSERT_EQ(0x10, gs);
    TEST_ASSERT_EQ(0x10, ss);
}

/*
 * test_boot - Run all boot verification tests
 *
 * This is the main entry point called by test_run_all().
 */
void test_boot(void)
{
    TEST_BEGIN("boot");

    test_protected_mode();
    test_a20_enabled();
    test_kernel_address();
    test_gdt_loaded();
    test_segments();
    test_memory_map();

    TEST_END();
}

#endif /* TEST_MODE */
