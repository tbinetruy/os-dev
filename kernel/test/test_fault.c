/*
 * kernel/test/test_fault.c - Page fault handler tests
 *
 * Tests the page fault handler by triggering controlled page faults
 * and verifying the handler correctly captures CR2 and error code.
 *
 * Strategy: Use a test hook that intercepts page faults before the
 * real handler runs. The hook records fault info and remaps the
 * faulting page so the CPU can retry the access successfully.
 */

#ifdef TEST_MODE

#include <test.h>
#include <isr.h>
#include <asm.h>
#include <pmm.h>
#include <vmm.h>
#include <page.h>
#include <printk.h>

/*
 * Test address in unmapped region above the kernel's direct-mapped area.
 * Must not conflict with any existing mapping.
 */
#define TEST_FAULT_VADDR    0xE0001000

/* Recorded fault info from test hook */
static volatile uint32_t test_fault_addr;
static volatile uint32_t test_fault_err;
static volatile int test_fault_triggered;

/* Physical frame used for test mapping */
static uint32_t test_phys_frame;

/* pf_set_test_hook declared in isr.h under TEST_MODE */

/*
 * test_pf_hook - Test page fault handler
 *
 * Records the faulting address and error code, then remaps the page
 * so the CPU can retry the faulting instruction successfully.
 */
static void test_pf_hook(struct registers *regs)
{
    test_fault_addr = read_cr2();
    test_fault_err = regs->err_code;
    test_fault_triggered = 1;

    /* Remap the page so the retried access succeeds */
    vmm_map_page(TEST_FAULT_VADDR, test_phys_frame, PAGE_KERNEL);
}

/*
 * test_fault_captures_cr2 - Verify CR2 captures correct faulting address
 *
 * Maps a page, unmaps it, then accesses it to trigger a page fault.
 * Verifies the recorded CR2 matches the test address.
 */
static void test_fault_captures_cr2(void)
{
    test_phys_frame = pmm_alloc_frame();
    if (test_phys_frame == 0) {
        TEST_FAIL("could not allocate frame for fault test");
        return;
    }

    /* Map, then unmap to create a not-present page */
    vmm_map_page(TEST_FAULT_VADDR, test_phys_frame, PAGE_KERNEL);
    vmm_unmap_page(TEST_FAULT_VADDR);

    /* Reset test state and install hook */
    test_fault_triggered = 0;
    test_fault_addr = 0;
    test_fault_err = 0;
    pf_set_test_hook(test_pf_hook);

    /* Trigger page fault by reading unmapped address */
    volatile uint32_t *ptr = (volatile uint32_t *)TEST_FAULT_VADDR;
    (void)*ptr;

    /* Remove hook and clean up */
    pf_set_test_hook(0);
    vmm_unmap_page(TEST_FAULT_VADDR);
    pmm_free_frame(test_phys_frame);

    TEST_ASSERT_MSG(test_fault_triggered == 1,
                    "page fault handler was called");
    TEST_ASSERT_MSG(test_fault_addr == TEST_FAULT_VADDR,
                    "CR2 matches faulting address");
}

/*
 * test_fault_error_code_not_present - Verify error code for not-present read
 *
 * A read to a not-present page should produce error code 0x00:
 *   bit 0 = 0 (not present)
 *   bit 1 = 0 (read access)
 *   bit 2 = 0 (kernel mode)
 */
static void test_fault_error_code_not_present(void)
{
    test_phys_frame = pmm_alloc_frame();
    if (test_phys_frame == 0) {
        TEST_FAIL("could not allocate frame for fault test");
        return;
    }

    /* Map and unmap to ensure PTE exists but is not-present */
    vmm_map_page(TEST_FAULT_VADDR, test_phys_frame, PAGE_KERNEL);
    vmm_unmap_page(TEST_FAULT_VADDR);

    test_fault_triggered = 0;
    test_fault_addr = 0;
    test_fault_err = 0xFFFFFFFF;
    pf_set_test_hook(test_pf_hook);

    /* Read from unmapped address */
    volatile uint32_t *ptr = (volatile uint32_t *)TEST_FAULT_VADDR;
    (void)*ptr;

    pf_set_test_hook(0);
    vmm_unmap_page(TEST_FAULT_VADDR);
    pmm_free_frame(test_phys_frame);

    /* Not-present, read, kernel mode → error code bits 0,1,2 all clear */
    TEST_ASSERT_MSG((test_fault_err & PF_ERR_PRESENT) == 0,
                    "not-present fault bit correct");
    TEST_ASSERT_MSG((test_fault_err & PF_ERR_WRITE) == 0,
                    "read access bit correct");
    TEST_ASSERT_MSG((test_fault_err & PF_ERR_USER) == 0,
                    "kernel mode bit correct");
}

/*
 * test_fault_error_code_constants - Verify PF_ERR_* constant values
 *
 * Sanity check that the named constants match the Intel-specified
 * bit positions for page fault error codes.
 */
static void test_fault_error_code_constants(void)
{
    TEST_ASSERT_MSG(PF_ERR_PRESENT == 0x01,
                    "PF_ERR_PRESENT is bit 0");
    TEST_ASSERT_MSG(PF_ERR_WRITE == 0x02,
                    "PF_ERR_WRITE is bit 1");
    TEST_ASSERT_MSG(PF_ERR_USER == 0x04,
                    "PF_ERR_USER is bit 2");
    TEST_ASSERT_MSG(PF_ERR_RSVD == 0x08,
                    "PF_ERR_RSVD is bit 3");
    TEST_ASSERT_MSG(PF_ERR_IFETCH == 0x10,
                    "PF_ERR_IFETCH is bit 4");
}

/*
 * test_fault_error_code_write - Verify error code for write to unmapped page
 *
 * A write to a not-present page should produce an error code with
 * PF_ERR_WRITE set (bit 1 = 1) and PF_ERR_PRESENT clear (bit 0 = 0).
 */
static void test_fault_error_code_write(void)
{
    test_phys_frame = pmm_alloc_frame();
    if (test_phys_frame == 0) {
        TEST_FAIL("could not allocate frame for write fault test");
        return;
    }

    /* Map and unmap to create a not-present page */
    vmm_map_page(TEST_FAULT_VADDR, test_phys_frame, PAGE_KERNEL);
    vmm_unmap_page(TEST_FAULT_VADDR);

    test_fault_triggered = 0;
    test_fault_addr = 0;
    test_fault_err = 0;
    pf_set_test_hook(test_pf_hook);

    /* Write to unmapped address to trigger write fault */
    volatile uint32_t *ptr = (volatile uint32_t *)TEST_FAULT_VADDR;
    *ptr = 0xDEADBEEF;

    pf_set_test_hook(0);
    vmm_unmap_page(TEST_FAULT_VADDR);
    pmm_free_frame(test_phys_frame);

    TEST_ASSERT_MSG(test_fault_triggered == 1,
                    "write fault handler was called");
    TEST_ASSERT_MSG(test_fault_addr == TEST_FAULT_VADDR,
                    "write fault CR2 correct");
    TEST_ASSERT_MSG((test_fault_err & PF_ERR_WRITE) != 0,
                    "write fault has PF_ERR_WRITE set");
    TEST_ASSERT_MSG((test_fault_err & PF_ERR_PRESENT) == 0,
                    "write fault is not-present");
}

/*
 * test_fault - Run all page fault handler tests
 */
void test_fault(void)
{
    TEST_BEGIN("fault");

    test_fault_error_code_constants();
    test_fault_captures_cr2();
    test_fault_error_code_not_present();
    test_fault_error_code_write();

    TEST_END();
}

#endif /* TEST_MODE */
