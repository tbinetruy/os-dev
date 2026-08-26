#ifndef KERNEL_INCLUDE_KSTACK_H
#define KERNEL_INCLUDE_KSTACK_H

#include <types.h>

struct kstack {
    uint32_t guard_base;
    uint32_t stack_base;
    uint32_t top;
};

int kstack_alloc(struct kstack *stack);
int kstack_free(struct kstack *stack);
uint32_t kstack_free_count(void);

#endif /* KERNEL_INCLUDE_KSTACK_H */
