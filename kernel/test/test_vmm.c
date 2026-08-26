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
#include <errno.h>

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

    /* Force the new dynamic-region page table above the direct-map limit. */
    TEST_ASSERT_MSG(pmm_test_force_next_frame(0x01000000) == 0,
                    "real free high frame reserved for page table");

    /*
     * Test 4: Map a page in an unused virtual address region
     *
     * We use 0xD0000000 which is above the kernel but below 0xFFFFFFFF.
     * This region is not mapped by the boot page tables.
     */
    uint32_t test_virt = KERNEL_DYNAMIC_START;
    int ret = vmm_map_page(test_virt, test_phys, PAGE_PRESENT | PAGE_WRITABLE);
    TEST_ASSERT_MSG(ret == 0, "vmm_map_page should succeed");
    TEST_ASSERT_MSG((((uint32_t *)RECURSIVE_PD_VADDR)
                     [PDE_INDEX(test_virt)] & PAGE_FRAME_MASK) >=
                    DIRECT_MAP_PHYS_LIMIT,
                    "page table frame is above direct-map limit");
    TEST_ASSERT_MSG(vmm_map_page(test_virt, test_phys + PAGE_SIZE,
                                 PAGE_KERNEL) == -EEXIST,
                    "occupied mapping is preserved");
    TEST_ASSERT_MSG(vmm_get_physaddr(test_virt) == test_phys,
                    "collision leaves original frame");
    TEST_ASSERT_MSG(vmm_map_page(test_virt + 1, test_phys, PAGE_KERNEL) ==
                    -EINVAL, "unaligned virtual address rejected");
    TEST_ASSERT_MSG(vmm_map_page(KERNEL_RESERVED_START, test_phys,
                                 PAGE_KERNEL) == -EINVAL,
                    "reserved region rejected");
    TEST_ASSERT_MSG(vmm_map_page(RECURSIVE_START, test_phys,
                                 PAGE_KERNEL) == -EINVAL,
                    "recursive region rejected");
    TEST_ASSERT_MSG(vmm_unmap_page(KERNEL_RESERVED_START) == -EINVAL,
                    "reserved unmap rejected");
    TEST_ASSERT_MSG(vmm_unmap_page(RECURSIVE_PD_VADDR) == -EINVAL,
                    "page-directory alias unmap rejected");
    TEST_ASSERT_MSG((((uint32_t *)RECURSIVE_PD_VADDR)
                     [RECURSIVE_PDE_INDEX] & PAGE_FRAME_MASK) ==
                    (read_cr3() & PAGE_FRAME_MASK),
                    "recursive PDE points to CR3");
    TEST_ASSERT_MSG((((uint32_t *)RECURSIVE_PD_VADDR)
                     [RECURSIVE_PDE_INDEX] & PAGE_USER) == 0,
                    "recursive PDE is supervisor-only");

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
     * After higher-half setup, VGA is in the checked direct-map window.
     */
    uint32_t vga_virt;
    vmm_direct_phys_to_virt(0xB8000, &vga_virt);
    volatile uint16_t *vga = (volatile uint16_t *)vga_virt;
    /* Verify we can read without faulting and VGA has content */
    uint16_t vga_val = *vga;
    TEST_ASSERT_MSG(vga_val != 0, "VGA memory accessible through direct map");

    /*
     * Test 11: P2V/V2P macro consistency
     */
    uint32_t test_addr = 0x00200000;
    uint32_t converted;
    uint32_t round_trip;
    TEST_ASSERT_MSG(vmm_direct_phys_to_virt(test_addr, &converted) == 0,
                    "bounded physical conversion succeeds");
    TEST_ASSERT_MSG(vmm_direct_virt_to_phys(converted, &round_trip) == 0 &&
                    round_trip == test_addr, "bounded conversion round trip");

    /*
     * Cleanup: Free the test frame
     */
    pmm_free_frame(test_phys);

    TEST_END();
}

#endif /* TEST_MODE */
