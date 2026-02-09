/*
 * tests/host/test_heap.c - Host-side kernel heap allocator tests
 *
 * Tests heap algorithms (first-fit, splitting, coalescing, OOM handling)
 * with mocked PMM/VMM.
 *
 * _kernel_end is aliased to heap_buf via --defsym so the heap uses
 * accessible memory. Compiled with -fno-pie/-no-pie to keep addresses
 * in the lower 4GB (kernel code stores addresses as uint32_t).
 *
 * Build: make test_heap (from tests/ directory)
 */

#include "unity/unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>

/*
 * Heap backing buffer - _kernel_end points here via linker --defsym.
 * Must be large enough for all test allocations + heap metadata.
 */
char heap_buf[4 * 1024 * 1024] __attribute__((aligned(4096)));

/* ===== Mock State ===== */

static uint32_t mock_free_frames;
static int mock_pmm_fail_after;    /* -1 = never, 0 = now, N = after N */
static uint32_t mock_next_frame;
static jmp_buf panic_jmp;
static int panic_expected;
static int panic_triggered;

/* Mock VMM map failure control */
static int mock_vmm_fail_after;    /* -1 = never, 0 = now, N = after N */

/* Include the code under test (brings in kernel header declarations) */
#include "../../kernel/mm/heap.c"

/* ===== Mock Implementations ===== */

uint32_t pmm_alloc_frame(void)
{
    if (mock_pmm_fail_after == 0) return 0;
    if (mock_pmm_fail_after > 0) mock_pmm_fail_after--;
    if (mock_free_frames == 0) return 0;
    mock_free_frames--;
    mock_next_frame += 0x1000;
    return mock_next_frame;
}

void pmm_free_frame(uint32_t addr)
{
    (void)addr;
    mock_free_frames++;
}

uint32_t pmm_get_free_count(void)
{
    return mock_free_frames;
}

uint32_t pmm_get_total_count(void)
{
    return 1024;
}

int vmm_map_page(uint32_t virt, uint32_t phys, uint32_t flags)
{
    (void)virt; (void)phys; (void)flags;
    if (mock_vmm_fail_after == 0) return -ENOMEM;
    if (mock_vmm_fail_after > 0) mock_vmm_fail_after--;
    return 0;
}

void vmm_unmap_page(uint32_t virt)
{
    (void)virt;
}

uint32_t vmm_get_physaddr(uint32_t virt)
{
    return virt;
}

void vmm_init(void)
{
}

void printk(int level, const char *fmt, ...)
{
    (void)level; (void)fmt;
}

void panic(const char *msg)
{
    panic_triggered = 1;
    if (panic_expected) {
        longjmp(panic_jmp, 1);
    }
    fprintf(stderr, "PANIC: %s\n", msg);
    abort();
}

/* ===== Test Helpers ===== */

static void reset_mocks(void)
{
    memset(heap_buf, 0, sizeof(heap_buf));
    mock_free_frames = 1024;
    mock_pmm_fail_after = -1;
    mock_vmm_fail_after = -1;
    mock_next_frame = 0x100000;
    panic_expected = 0;
    panic_triggered = 0;
}

void setUp(void)
{
    reset_mocks();
    heap_init();
}

void tearDown(void)
{
}

/* ===== Core Functionality Tests ===== */

void test_basic_alloc_returns_non_null(void)
{
    void *p = kmalloc(64);
    TEST_ASSERT_NOT_NULL(p);
    kfree(p);
}

void test_alloc_alignment(void)
{
    void *p = kmalloc(1);
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQUAL_UINT32(0, (uint32_t)p % HEAP_ALIGNMENT);
    kfree(p);

    p = kmalloc(13);
    TEST_ASSERT_EQUAL_UINT32(0, (uint32_t)p % HEAP_ALIGNMENT);
    kfree(p);
}

void test_write_readback(void)
{
    uint8_t *p = (uint8_t *)kmalloc(256);
    TEST_ASSERT_NOT_NULL(p);

    for (int i = 0; i < 256; i++) {
        p[i] = (uint8_t)(i & 0xFF);
    }
    for (int i = 0; i < 256; i++) {
        TEST_ASSERT_EQUAL_UINT8((uint8_t)(i & 0xFF), p[i]);
    }
    kfree(p);
}

void test_multiple_allocs_different_pointers(void)
{
    void *a = kmalloc(32);
    void *b = kmalloc(32);
    void *c = kmalloc(32);

    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_NOT_NULL(c);
    TEST_ASSERT_NOT_EQUAL((uint32_t)a, (uint32_t)b);
    TEST_ASSERT_NOT_EQUAL((uint32_t)b, (uint32_t)c);

    kfree(a);
    kfree(b);
    kfree(c);
}

void test_reuse_after_free(void)
{
    void *a = kmalloc(64);
    uint32_t addr_a = (uint32_t)a;
    kfree(a);

    void *b = kmalloc(64);
    TEST_ASSERT_EQUAL_UINT32(addr_a, (uint32_t)b);
    kfree(b);
}

void test_kmalloc_zero_returns_null(void)
{
    TEST_ASSERT_NULL(kmalloc(0));
}

void test_kfree_null_is_safe(void)
{
    kfree(NULL);
    /* No crash = pass */
}

/* ===== Coalescing Tests ===== */

void test_coalesce_forward(void)
{
    void *a = kmalloc(64);
    void *b = kmalloc(64);
    void *c = kmalloc(64);

    kfree(b);
    kfree(c);

    /* B and C should have coalesced; 128-byte alloc should fit at B */
    void *big = kmalloc(128);
    TEST_ASSERT_NOT_NULL(big);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)b, (uint32_t)big);

    kfree(a);
    kfree(big);
}

void test_coalesce_backward(void)
{
    void *a = kmalloc(64);
    void *b = kmalloc(64);
    void *c = kmalloc(64);

    kfree(a);
    kfree(b);

    /* A and B should have coalesced; 128-byte alloc should fit at A */
    void *big = kmalloc(128);
    TEST_ASSERT_NOT_NULL(big);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)a, (uint32_t)big);

    kfree(big);
    kfree(c);
}

void test_coalesce_both_neighbors(void)
{
    void *a = kmalloc(64);
    void *b = kmalloc(64);
    void *c = kmalloc(64);
    void *d = kmalloc(64);  /* Prevent tail coalesce */

    kfree(a);
    kfree(c);
    /* Now A is free, B allocated, C free */

    kfree(b);
    /* B should coalesce forward with C AND backward with A */

    void *big = kmalloc(192);
    TEST_ASSERT_NOT_NULL(big);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)a, (uint32_t)big);

    kfree(big);
    kfree(d);
}

void test_no_coalesce_nonadjacent(void)
{
    void *a = kmalloc(64);
    void *b = kmalloc(64);
    void *c = kmalloc(64);

    kfree(a);
    kfree(c);
    /* A and C are free but B separates them */

    /* A large alloc should NOT fit in either A or C alone */
    void *big = kmalloc(128);
    TEST_ASSERT_NOT_NULL(big);
    /* big should NOT be at A's address (A is only 64 bytes payload) */
    TEST_ASSERT_NOT_EQUAL((uint32_t)a, (uint32_t)big);

    kfree(b);
    kfree(big);
}

/* ===== Splitting Tests ===== */

void test_split_creates_remainder_block(void)
{
    /* Initial heap has one big free block. Small alloc should split it. */
    uint32_t free_before = heap_get_free();
    void *p = kmalloc(32);
    uint32_t free_after = heap_get_free();
    uint32_t used = heap_get_used();

    TEST_ASSERT_NOT_NULL(p);
    /* Free space should have decreased by allocated block size */
    TEST_ASSERT_TRUE(free_after < free_before);
    TEST_ASSERT_TRUE(used > 0);

    kfree(p);
}

/* ===== OOM Tests (main value-add over in-kernel tests) ===== */

void test_oom_pmm_exhausted_returns_null(void)
{
    /* Exhaust mock PMM before allocating */
    mock_free_frames = 0;

    /* Allocation that needs expansion should fail */
    void *p = kmalloc(128 * 1024);  /* Bigger than initial 64KB heap */
    TEST_ASSERT_NULL(p);
}

void test_oom_pmm_fails_immediately_in_expand(void)
{
    mock_pmm_fail_after = 0;

    void *p = kmalloc(128 * 1024);
    TEST_ASSERT_NULL(p);
}

void test_oom_partial_expand_rollback(void)
{
    /* Let PMM succeed for 2 frames then fail */
    uint32_t free_before = mock_free_frames;
    mock_pmm_fail_after = 2;

    /* Needs more than 2 pages = needs > 8KB expansion */
    void *p = kmalloc(64 * 1024);
    TEST_ASSERT_NULL(p);

    /* Frames should be rolled back (freed) */
    TEST_ASSERT_EQUAL_UINT32(free_before, mock_free_frames);
}

void test_oom_vmm_map_fails_rollback(void)
{
    /* VMM fails after mapping 3 pages */
    uint32_t free_before = mock_free_frames;
    mock_vmm_fail_after = 3;

    void *p = kmalloc(64 * 1024);
    TEST_ASSERT_NULL(p);

    /* All allocated frames should have been freed */
    TEST_ASSERT_EQUAL_UINT32(free_before, mock_free_frames);
}

void test_heap_init_panics_on_pmm_failure(void)
{
    reset_mocks();
    mock_pmm_fail_after = 0;  /* Fail immediately */
    panic_expected = 1;

    if (setjmp(panic_jmp) == 0) {
        heap_init();
        TEST_FAIL_MESSAGE("heap_init should have panicked");
    } else {
        /* Panic was triggered - expected */
        TEST_ASSERT_TRUE(panic_triggered);
    }

    /* Restore for tearDown */
    reset_mocks();
    heap_init();
}

void test_oom_small_alloc_in_full_heap(void)
{
    /* Fill the initial heap with small allocations.
     * 64KB heap / (128 + header) per alloc ≈ 430 blocks on 64-bit,
     * so 512 entries is enough to guarantee exhaustion.
     */
    void *ptrs[512];
    int count = 0;

    /* Prevent expansion */
    mock_free_frames = 0;

    for (int i = 0; i < 512; i++) {
        ptrs[i] = kmalloc(128);
        if (ptrs[i] == NULL) break;
        count++;
    }

    /* Should have allocated some, then hit NULL */
    TEST_ASSERT_TRUE(count > 0);
    TEST_ASSERT_TRUE(count < 512);

    for (int i = 0; i < count; i++) {
        kfree(ptrs[i]);
    }
}

/* ===== Safety Tests ===== */

void test_kfree_invalid_pointer(void)
{
    /* Pointer outside heap region should be safely rejected */
    int stack_var;
    kfree(&stack_var);
    /* No crash = pass */
}

void test_double_free_detection(void)
{
    void *p = kmalloc(64);
    kfree(p);
    kfree(p);  /* Double free - should warn but not crash */
}

/* ===== Stress Tests ===== */

void test_stress_alloc_free_cycle(void)
{
    uint32_t free_before = heap_get_free();

    for (int i = 0; i < 500; i++) {
        void *p = kmalloc(64);
        TEST_ASSERT_NOT_NULL(p);
        ((uint8_t *)p)[0] = 0xAA;
        ((uint8_t *)p)[63] = 0xBB;
        kfree(p);
    }

    /* Free space should be restored after all alloc/free cycles */
    TEST_ASSERT_EQUAL_UINT32(free_before, heap_get_free());
}

void test_stress_many_small_allocs(void)
{
    void *ptrs[100];

    for (int i = 0; i < 100; i++) {
        ptrs[i] = kmalloc(16);
        TEST_ASSERT_NOT_NULL(ptrs[i]);
    }

    /* Write to each to verify no overlap */
    for (int i = 0; i < 100; i++) {
        memset(ptrs[i], (uint8_t)i, 16);
    }

    /* Verify data intact (no overlap corruption) */
    for (int i = 0; i < 100; i++) {
        uint8_t *p = (uint8_t *)ptrs[i];
        for (int j = 0; j < 16; j++) {
            TEST_ASSERT_EQUAL_UINT8((uint8_t)i, p[j]);
        }
    }

    for (int i = 0; i < 100; i++) {
        kfree(ptrs[i]);
    }
}

void test_stress_mixed_sizes(void)
{
    void *ptrs[50];
    int sizes[] = {8, 256, 16, 1024, 32, 512, 64, 128, 48, 96};
    uint32_t free_before = heap_get_free();

    for (int i = 0; i < 50; i++) {
        ptrs[i] = kmalloc(sizes[i % 10]);
        TEST_ASSERT_NOT_NULL(ptrs[i]);
    }

    /* Free in reverse order */
    for (int i = 49; i >= 0; i--) {
        kfree(ptrs[i]);
    }

    TEST_ASSERT_EQUAL_UINT32(free_before, heap_get_free());
}

void test_stress_fragmentation_recovery(void)
{
    void *ptrs[20];

    /* Allocate 20 blocks */
    for (int i = 0; i < 20; i++) {
        ptrs[i] = kmalloc(128);
        TEST_ASSERT_NOT_NULL(ptrs[i]);
    }

    /* Free every other block (create fragmentation) */
    for (int i = 0; i < 20; i += 2) {
        kfree(ptrs[i]);
    }

    /* Free remaining blocks */
    for (int i = 1; i < 20; i += 2) {
        kfree(ptrs[i]);
    }

    /* After all frees, should be able to allocate a large block */
    void *big = kmalloc(2048);
    TEST_ASSERT_NOT_NULL(big);
    kfree(big);
}

/* ===== Main ===== */

int main(void)
{
    UNITY_BEGIN();

    /* Core functionality */
    RUN_TEST(test_basic_alloc_returns_non_null);
    RUN_TEST(test_alloc_alignment);
    RUN_TEST(test_write_readback);
    RUN_TEST(test_multiple_allocs_different_pointers);
    RUN_TEST(test_reuse_after_free);
    RUN_TEST(test_kmalloc_zero_returns_null);
    RUN_TEST(test_kfree_null_is_safe);

    /* Coalescing */
    RUN_TEST(test_coalesce_forward);
    RUN_TEST(test_coalesce_backward);
    RUN_TEST(test_coalesce_both_neighbors);
    RUN_TEST(test_no_coalesce_nonadjacent);

    /* Splitting */
    RUN_TEST(test_split_creates_remainder_block);

    /* OOM (unique to host tests) */
    RUN_TEST(test_oom_pmm_exhausted_returns_null);
    RUN_TEST(test_oom_pmm_fails_immediately_in_expand);
    RUN_TEST(test_oom_partial_expand_rollback);
    RUN_TEST(test_oom_vmm_map_fails_rollback);
    RUN_TEST(test_heap_init_panics_on_pmm_failure);
    RUN_TEST(test_oom_small_alloc_in_full_heap);

    /* Safety */
    RUN_TEST(test_kfree_invalid_pointer);
    RUN_TEST(test_double_free_detection);

    /* Stress */
    RUN_TEST(test_stress_alloc_free_cycle);
    RUN_TEST(test_stress_many_small_allocs);
    RUN_TEST(test_stress_mixed_sizes);
    RUN_TEST(test_stress_fragmentation_recovery);

    return UNITY_END();
}
