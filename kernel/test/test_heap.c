/*
 * test_heap.c - Kernel heap allocator tests
 *
 * Tests kmalloc/kfree functionality including allocation, freeing,
 * coalescing, heap growth, alignment, and leak detection.
 */

#ifdef TEST_MODE

#include <test.h>
#include <heap.h>
#include <pmm.h>
#include <types.h>

/*
 * test_heap - Run all heap allocator tests
 */
void test_heap(void)
{
    void *ptr;
    void *ptr_a;
    void *ptr_b;
    void *ptr_c;
    void *ptr_new;
    uint32_t free_before;
    uint32_t free_after;
    uint32_t i;
    uint8_t *bytes;

    TEST_BEGIN("heap");

    /*
     * Test: kmalloc returns non-NULL, 8-byte aligned, within heap region
     * AC: #2
     */
    ptr = kmalloc(64);
    TEST_ASSERT_NOT_NULL(ptr);
    TEST_ASSERT_EQ(0, (uint32_t)ptr % HEAP_ALIGNMENT);
    TEST_ASSERT_GT((uint32_t)ptr, (uint32_t)&_kernel_end);
    kfree(ptr);

    /*
     * Test: write to allocated memory doesn't fault, data preserved
     * AC: #2
     */
    bytes = (uint8_t *)kmalloc(128);
    TEST_ASSERT_NOT_NULL(bytes);
    for (i = 0; i < 128; i++) {
        bytes[i] = (uint8_t)(i & 0xFF);
    }
    for (i = 0; i < 128; i++) {
        TEST_ASSERT_EQ((uint8_t)(i & 0xFF), bytes[i]);
    }
    kfree(bytes);

    /*
     * Test: kfree then kmalloc reuses freed memory
     * AC: #3
     */
    ptr_a = kmalloc(64);
    kfree(ptr_a);
    ptr_b = kmalloc(64);
    /* After free+alloc of same size, should get same (or nearby) address */
    TEST_ASSERT_NOT_NULL(ptr_b);
    TEST_ASSERT_EQ((uint32_t)ptr_a, (uint32_t)ptr_b);
    kfree(ptr_b);

    /*
     * Test: multiple allocations return different pointers
     * AC: #2
     */
    ptr_a = kmalloc(32);
    ptr_b = kmalloc(32);
    ptr_c = kmalloc(32);
    TEST_ASSERT_NOT_NULL(ptr_a);
    TEST_ASSERT_NOT_NULL(ptr_b);
    TEST_ASSERT_NOT_NULL(ptr_c);
    TEST_ASSERT_NEQ((uint32_t)ptr_a, (uint32_t)ptr_b);
    TEST_ASSERT_NEQ((uint32_t)ptr_b, (uint32_t)ptr_c);
    TEST_ASSERT_NEQ((uint32_t)ptr_a, (uint32_t)ptr_c);
    kfree(ptr_a);
    kfree(ptr_b);
    kfree(ptr_c);

    /*
     * Test: coalescing - allocate A, B, C; free B then C;
     *       new allocation of size B+C fits in merged block
     * AC: #6
     */
    ptr_a = kmalloc(64);
    ptr_b = kmalloc(64);
    ptr_c = kmalloc(64);
    kfree(ptr_b);
    kfree(ptr_c);
    /* Freeing B then C should coalesce into one block >= 128 bytes */
    ptr_new = kmalloc(128);
    TEST_ASSERT_NOT_NULL(ptr_new);
    TEST_ASSERT_EQ((uint32_t)ptr_new, (uint32_t)ptr_b);
    kfree(ptr_a);
    kfree(ptr_new);

    /*
     * Test: large allocation triggers heap growth
     * AC: #4
     */
    ptr = kmalloc(128 * 1024);  /* 128KB - exceeds initial 64KB heap */
    TEST_ASSERT_NOT_NULL(ptr);
    bytes = (uint8_t *)ptr;
    /* Write first and last bytes to verify mapping */
    bytes[0] = 0xAA;
    bytes[128 * 1024 - 1] = 0xBB;
    TEST_ASSERT_EQ(0xAA, bytes[0]);
    TEST_ASSERT_EQ(0xBB, bytes[128 * 1024 - 1]);
    kfree(ptr);

    /*
     * Test: allocate-free cycle returns free frame count to original
     * AC: #7
     */
    free_before = pmm_get_free_count();
    ptr = kmalloc(256);
    TEST_ASSERT_NOT_NULL(ptr);
    kfree(ptr);
    free_after = pmm_get_free_count();
    /* Free count should be same - no frames leaked by alloc/free */
    TEST_ASSERT_EQ(free_before, free_after);

    /*
     * Test: kmalloc(0) returns NULL
     * AC: #5 (defined behavior: return NULL for zero-size)
     */
    ptr = kmalloc(0);
    TEST_ASSERT_NULL(ptr);

    /*
     * Test: kfree(NULL) does not crash
     * AC: #3
     */
    kfree(NULL);
    TEST_ASSERT_MSG(1, "kfree(NULL) did not crash");

    /*
     * Test: Large-scale heap expansion and cleanup doesn't crash
     * AC: #4, #5
     *
     * Boot page tables cover ~14MB of kernel VA space above heap_start.
     * With 128MB QEMU RAM, PMM won't exhaust before VA limit.
     * This test validates multi-expansion behavior within safe VA range.
     */
    {
        void *oom_ptrs[12];
        uint32_t oom_count = 0;
        uint32_t j;

        for (j = 0; j < 12; j++) {
            oom_ptrs[j] = kmalloc(1024 * 1024);
            if (oom_ptrs[j] == NULL) {
                break;
            }
            oom_count++;
        }
        TEST_ASSERT_GT(oom_count, (uint32_t)0);
        TEST_ASSERT_MSG(1, "no crash during multi-expansion");

        for (j = 0; j < oom_count; j++) {
            kfree(oom_ptrs[j]);
        }
    }

    TEST_END();
}

#endif /* TEST_MODE */
