#include <kstack.h>

#include <errno.h>
#include <page.h>
#include <pmm.h>
#include <vmm.h>

#define KSTACK_BITMAP_BYTES ((KSTACK_SLOT_COUNT + 7U) / 8U)

static uint8_t slot_bitmap[KSTACK_BITMAP_BYTES];
static uint32_t allocated_slots;

static int slot_used(uint32_t index)
{
    return (slot_bitmap[index / 8U] & (1U << (index % 8U))) != 0;
}

static void set_slot(uint32_t index, int used)
{
    uint8_t mask = (uint8_t)(1U << (index % 8U));
    if (used) {
        slot_bitmap[index / 8U] |= mask;
    } else {
        slot_bitmap[index / 8U] &= (uint8_t)~mask;
    }
}

static void clear_stack(struct kstack *stack)
{
    stack->guard_base = 0;
    stack->stack_base = 0;
    stack->top = 0;
}

int kstack_alloc(struct kstack *stack)
{
    uint32_t index;
    uint32_t guard;
    uint32_t mapped;
    uint32_t phys;
    int ret;

    if (stack == NULL) {
        return -EINVAL;
    }
    clear_stack(stack);

    for (index = 0; index < KSTACK_SLOT_COUNT; index++) {
        if (!slot_used(index)) {
            break;
        }
    }
    if (index == KSTACK_SLOT_COUNT) {
        return -ENOMEM;
    }

    guard = KERNEL_STACK_START + index * KSTACK_SLOT_SIZE;
    mapped = guard + KSTACK_GUARD_PAGES * PAGE_SIZE;
    if (vmm_get_physaddr(guard) != 0 || vmm_get_physaddr(mapped) != 0) {
        return -EEXIST;
    }

    phys = pmm_alloc_frame();
    if (phys == 0) {
        return -ENOMEM;
    }
    ret = vmm_map_page(mapped, phys, PAGE_KERNEL);
    if (ret != 0) {
        pmm_free_frame(phys);
        return ret;
    }

    set_slot(index, 1);
    allocated_slots++;
    stack->guard_base = guard;
    stack->stack_base = mapped;
    stack->top = mapped + KSTACK_PAGES * PAGE_SIZE;
    return 0;
}

int kstack_free(struct kstack *stack)
{
    uint32_t offset;
    uint32_t index;
    uint32_t phys;

    if (stack == NULL || stack->guard_base < KERNEL_STACK_START ||
        stack->guard_base >= KERNEL_STACK_END_EXCLUSIVE) {
        return -EINVAL;
    }
    offset = stack->guard_base - KERNEL_STACK_START;
    if (offset % KSTACK_SLOT_SIZE != 0) {
        return -EINVAL;
    }
    index = offset / KSTACK_SLOT_SIZE;
    if (!slot_used(index) ||
        stack->stack_base != stack->guard_base + PAGE_SIZE ||
        stack->top != stack->stack_base + PAGE_SIZE) {
        return -EINVAL;
    }

    phys = vmm_get_physaddr(stack->stack_base);
    if (phys == 0 || vmm_unmap_page(stack->stack_base) != 0) {
        return -EINVAL;
    }
    pmm_free_frame(phys & PAGE_FRAME_MASK);
    set_slot(index, 0);
    allocated_slots--;
    clear_stack(stack);
    return 0;
}

uint32_t kstack_free_count(void)
{
    return KSTACK_SLOT_COUNT - allocated_slots;
}
