#include "unity/unity.h"

#include <errno.h>
#include <kstack.h>
#include <page.h>
#include <vmm.h>
#include <string.h>

static uint32_t mappings[KSTACK_SLOT_COUNT];
static uint32_t next_frame;
static uint32_t free_frames;
static int fail_pmm;
static int fail_map;

#include "../../kernel/mm/kstack.c"

uint32_t pmm_alloc_frame(void)
{
    if (fail_pmm || free_frames == 0) {
        return 0;
    }
    free_frames--;
    next_frame += PAGE_SIZE;
    return next_frame;
}

void pmm_free_frame(uint32_t phys)
{
    (void)phys;
    free_frames++;
}

static int mapping_index(uint32_t virt)
{
    if (virt < KERNEL_STACK_START || virt >= KERNEL_STACK_END_EXCLUSIVE) {
        return -1;
    }
    return (int)((virt - KERNEL_STACK_START) / KSTACK_SLOT_SIZE);
}

int vmm_map_page(uint32_t virt, uint32_t phys, uint32_t flags)
{
    int index = mapping_index(virt);
    (void)flags;
    if (fail_map) {
        return -ENOMEM;
    }
    if (index < 0 || mappings[index] != 0) {
        return -EEXIST;
    }
    mappings[index] = phys;
    return 0;
}

int vmm_unmap_page(uint32_t virt)
{
    int index = mapping_index(virt);
    if (index < 0) {
        return -EINVAL;
    }
    mappings[index] = 0;
    return 0;
}

uint32_t vmm_get_physaddr(uint32_t virt)
{
    int index = mapping_index(virt);
    if (index < 0 || virt % KSTACK_SLOT_SIZE == 0) {
        return 0;
    }
    return mappings[index];
}

void setUp(void)
{
    memset(slot_bitmap, 0, sizeof(slot_bitmap));
    memset(mappings, 0, sizeof(mappings));
    allocated_slots = 0;
    next_frame = 0x01000000;
    free_frames = KSTACK_SLOT_COUNT + 4;
    fail_pmm = 0;
    fail_map = 0;
}

void tearDown(void)
{
}

void test_first_slot_guard_map_free_and_reuse(void)
{
    struct kstack stack;
    struct kstack reused;
    uint32_t frames = free_frames;

    TEST_ASSERT_EQUAL_INT(0, kstack_alloc(&stack));
    TEST_ASSERT_EQUAL_HEX32(KERNEL_STACK_START, stack.guard_base);
    TEST_ASSERT_EQUAL_HEX32(KERNEL_STACK_START + PAGE_SIZE,
                            stack.stack_base);
    TEST_ASSERT_EQUAL_HEX32(stack.stack_base + PAGE_SIZE, stack.top);
    TEST_ASSERT_EQUAL_HEX32(0, vmm_get_physaddr(stack.guard_base));
    TEST_ASSERT_NOT_EQUAL(0, vmm_get_physaddr(stack.stack_base));
    TEST_ASSERT_EQUAL_INT(0, kstack_free(&stack));
    TEST_ASSERT_EQUAL_UINT32(frames, free_frames);
    TEST_ASSERT_EQUAL_HEX32(0, stack.top);
    TEST_ASSERT_EQUAL_INT(0, kstack_alloc(&reused));
    TEST_ASSERT_EQUAL_HEX32(KERNEL_STACK_START, reused.guard_base);
}

void test_failures_rollback_and_zero_output(void)
{
    struct kstack stack = { 1, 2, 3 };
    uint32_t frames = free_frames;

    fail_pmm = 1;
    TEST_ASSERT_EQUAL_INT(-ENOMEM, kstack_alloc(&stack));
    TEST_ASSERT_EQUAL_HEX32(0, stack.guard_base);
    TEST_ASSERT_EQUAL_UINT32(frames, free_frames);
    fail_pmm = 0;
    fail_map = 1;
    TEST_ASSERT_EQUAL_INT(-ENOMEM, kstack_alloc(&stack));
    TEST_ASSERT_EQUAL_UINT32(frames, free_frames);
    TEST_ASSERT_EQUAL_UINT32(KSTACK_SLOT_COUNT, kstack_free_count());
}

void test_exhaustion_last_slot_and_invalid_double_free(void)
{
    struct kstack stacks[KSTACK_SLOT_COUNT];
    struct kstack extra;
    struct kstack saved;
    uint32_t i;

    for (i = 0; i < KSTACK_SLOT_COUNT; i++) {
        TEST_ASSERT_EQUAL_INT(0, kstack_alloc(&stacks[i]));
    }
    TEST_ASSERT_EQUAL_HEX32(KERNEL_STACK_END_EXCLUSIVE - KSTACK_SLOT_SIZE,
                            stacks[KSTACK_SLOT_COUNT - 1].guard_base);
    TEST_ASSERT_EQUAL_INT(-ENOMEM, kstack_alloc(&extra));
    saved = stacks[0];
    TEST_ASSERT_EQUAL_INT(0, kstack_free(&stacks[0]));
    TEST_ASSERT_EQUAL_INT(-EINVAL, kstack_free(&saved));
    TEST_ASSERT_EQUAL_INT(-EINVAL, kstack_free(NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_first_slot_guard_map_free_and_reuse);
    RUN_TEST(test_failures_rollback_and_zero_output);
    RUN_TEST(test_exhaustion_last_slot_and_invalid_double_free);
    return UNITY_END();
}
