/*
 * test_vmm.c - Virtual Memory Manager Tests
 *
 * Tests for the VMM implementation including:
 *   - Paging enablement verification
 *   - Higher-half kernel execution
 *   - Page mapping/unmapping
 *   - Virtual-to-physical address translation
 *   - VGA memory accessibility
 */

#ifdef TEST_MODE

#include <test.h>
#include <vmm.h>
#include <page.h>
#include <pmm.h>
#include <asm.h>
#include <printk.h>

/*
 * test_vmm - Run all VMM tests
 *
 * Tests:
 *   1. Paging is enabled (CR0.PG = 1)
 *   2. Kernel is running at higher-half addresses
 *   3. vmm_map_page() correctly maps a page
 *   4. Mapped page is readable/writable
 *   5. vmm_get_physaddr() returns correct physical address
 *   6. vmm_unmap_page() removes the mapping
 *   7. VGA memory is still accessible
 */
void test_vmm(void)
{
    TEST_BEGIN("vmm");

    /*
     * Test 1: Paging is enabled
     *
     * CR0.PG (bit 31) must be set after vmm_init().
     */
    uint32_t cr0 = read_cr0();
    TEST_ASSERT_MSG((cr0 & 0x80000000) != 0, "CR0.PG should be set");

    /*
     * Test 2: Kernel running at higher-half address
     *
     * Function addresses should be >= KERNEL_BASE (0xC0000000).
     */
    uint32_t code_addr = (uint32_t)&test_vmm;
    TEST_ASSERT_MSG(code_addr >= KERNEL_BASE, "kernel should be at higher-half");

    /*
     * Test 3: Allocate a physical frame for testing
     */
    uint32_t test_phys = pmm_alloc_frame();
    TEST_ASSERT_MSG(test_phys != 0, "pmm_alloc_frame should succeed");

    if (test_phys == 0) {
        /* Can't continue without a frame */
        TEST_END();
        return;
    }

    /*
     * Test 4: Map a page in an unused virtual address region
     *
     * We use 0xD0000000 which is above the kernel but below 0xFFFFFFFF.
     * This region is not mapped by the boot page tables.
     */
    uint32_t test_virt = 0xD0000000;
    int ret = vmm_map_page(test_virt, test_phys, PAGE_PRESENT | PAGE_WRITABLE);
    TEST_ASSERT_MSG(ret == 0, "vmm_map_page should succeed");

    if (ret != 0) {
        /* Can't continue if mapping failed */
        pmm_free_frame(test_phys);
        TEST_END();
        return;
    }

    /*
     * Test 5: Write to mapped page and read back
     *
     * If the mapping is correct, we can access the page.
     */
    volatile uint32_t *ptr = (volatile uint32_t *)test_virt;
    *ptr = 0xDEADBEEF;
    TEST_ASSERT_MSG(*ptr == 0xDEADBEEF, "write/read should work on mapped page");

    /*
     * Test 6: Verify physical address translation
     *
     * vmm_get_physaddr() should return the physical address we mapped.
     */
    uint32_t phys = vmm_get_physaddr(test_virt);
    TEST_ASSERT_MSG(phys == test_phys, "vmm_get_physaddr should return mapped phys");

    /*
     * Test 7: Test address with offset
     *
     * vmm_get_physaddr should correctly handle page offsets.
     */
    uint32_t offset_virt = test_virt + 0x100;
    uint32_t offset_phys = vmm_get_physaddr(offset_virt);
    TEST_ASSERT_MSG(offset_phys == test_phys + 0x100,
                    "vmm_get_physaddr should handle offsets");

    /*
     * Test 8: Unmap the page
     */
    vmm_unmap_page(test_virt);

    /*
     * Test 9: Verify unmapped address returns 0
     *
     * After unmapping, vmm_get_physaddr should return 0.
     */
    phys = vmm_get_physaddr(test_virt);
    TEST_ASSERT_MSG(phys == 0, "unmapped address should return 0");

    /*
     * Note: We don't test accessing the unmapped page as it would
     * cause a page fault (which we handle in Story 3.3).
     */

    /*
     * Test 10: VGA memory is accessible
     *
     * VGA buffer at 0xB8000 should be mapped (either identity mapped
     * or through P2V). We test by reading a known pattern.
     *
     * After higher-half setup, VGA is at P2V(0xB8000) = 0xC00B8000.
     */
    volatile uint16_t *vga = (volatile uint16_t *)P2V(0xB8000);
    /* Verify we can read without faulting and VGA has content */
    uint16_t vga_val = *vga;
    TEST_ASSERT_MSG(vga_val != 0, "VGA memory accessible at P2V(0xB8000)");

    /*
     * Test 11: P2V/V2P macro consistency
     */
    uint32_t test_addr = 0x00200000;
    TEST_ASSERT_MSG(V2P(P2V(test_addr)) == test_addr,
                    "V2P(P2V(x)) should equal x");
    TEST_ASSERT_MSG(P2V(V2P(KERNEL_BASE + test_addr)) == KERNEL_BASE + test_addr,
                    "P2V(V2P(x)) should equal x for kernel addresses");

    /*
     * Cleanup: Free the test frame
     */
    pmm_free_frame(test_phys);

    TEST_END();
}

#endif /* TEST_MODE */
