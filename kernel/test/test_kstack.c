#ifdef TEST_MODE

#include <kstack.h>

#include <page.h>
#include <pmm.h>
#include <test.h>
#include <vmm.h>

void test_kstack(void)
{
    struct kstack stack;
    struct kstack saved;
    uint32_t free_before;

    TEST_BEGIN("kstack");
    /* Warm the stack-region page table; page-table pages remain owned by VMM. */
    TEST_ASSERT_MSG(kstack_alloc(&stack) == 0, "warm stack page table");
    TEST_ASSERT_MSG(kstack_free(&stack) == 0, "release warm stack");
    free_before = pmm_get_free_count();
    TEST_ASSERT_MSG(kstack_alloc(&stack) == 0, "allocate guarded stack");
    TEST_ASSERT_MSG(stack.guard_base >= KERNEL_STACK_START,
                    "stack is in owned region");
    TEST_ASSERT_MSG(stack.top == stack.stack_base + PAGE_SIZE,
                    "top is exclusive");
    TEST_ASSERT_MSG(vmm_get_physaddr(stack.guard_base) == 0,
                    "guard page remains absent");
    TEST_ASSERT_MSG(vmm_get_physaddr(stack.stack_base) != 0,
                    "stack page is present");
    saved = stack;
    TEST_ASSERT_MSG(kstack_free(&stack) == 0, "free guarded stack");
    TEST_ASSERT_MSG(pmm_get_free_count() == free_before,
                    "stack free restores PMM count");
    TEST_ASSERT_MSG(kstack_free(&saved) < 0, "double free rejected");
    TEST_END();
}

#endif /* TEST_MODE */
