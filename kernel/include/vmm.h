/*
 * vmm.h - Virtual memory layout and mapping interface
 *
 * Kernel virtual space is partitioned into explicit, half-open ownership
 * regions. The recursive region ends at 4 GiB, so its representable bound is
 * expressed by RECURSIVE_LAST rather than a wrapping 32-bit exclusive end.
 */

#ifndef KERNEL_INCLUDE_VMM_H
#define KERNEL_INCLUDE_VMM_H

#include <types.h>

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096U
#endif
#ifndef PAGE_SHIFT
#define PAGE_SHIFT 12U
#endif
#ifndef PAGE_MASK
#define PAGE_MASK (~(PAGE_SIZE - 1U))
#endif

/* Canonical region ownership, in ascending virtual-address order. */
#define USER_SPACE_START              0x00000000U
#define USER_SPACE_END_EXCLUSIVE      0xC0000000U
#define USER_SPACE_LAST               0xBFFFFFFFU
#define DIRECT_MAP_START              0xC0000000U
#define DIRECT_MAP_END_EXCLUSIVE      0xC1000000U
#define DIRECT_MAP_LAST               0xC0FFFFFFU
#define DIRECT_MAP_PHYS_LIMIT         0x01000000U
#define KERNEL_HEAP_START             0xC1000000U
#define KERNEL_HEAP_END_EXCLUSIVE     0xE0000000U
#define KERNEL_HEAP_LAST              0xDFFFFFFFU
#define KERNEL_DYNAMIC_START          0xE0000000U
#define KERNEL_DYNAMIC_END_EXCLUSIVE  0xF0000000U
#define KERNEL_DYNAMIC_LAST           0xEFFFFFFFU
#define KERNEL_RESERVED_START         0xF0000000U
#define KERNEL_RESERVED_END_EXCLUSIVE 0xFF000000U
#define KERNEL_RESERVED_LAST          0xFEFFFFFFU
#define KERNEL_STACK_START            0xFF000000U
#define KERNEL_STACK_END_EXCLUSIVE    0xFFC00000U
#define KERNEL_STACK_LAST             0xFFBFFFFFU
#define RECURSIVE_START               0xFFC00000U
#define RECURSIVE_LAST                0xFFFFFFFFU

#define KERNEL_BASE                   DIRECT_MAP_START
#define KERNEL_PAGE_DIR_IDX           768U
#define DIRECT_MAP_FIRST_PDE          768U
#define DIRECT_MAP_LAST_PDE           771U
#define KERNEL_HEAP_FIRST_PDE         772U
#define KERNEL_HEAP_LAST_PDE          895U
#define KERNEL_DYNAMIC_FIRST_PDE      896U
#define KERNEL_DYNAMIC_LAST_PDE       959U
#define KERNEL_RESERVED_FIRST_PDE     960U
#define KERNEL_RESERVED_LAST_PDE      1019U
#define KERNEL_STACK_FIRST_PDE        1020U
#define KERNEL_STACK_LAST_PDE         1022U
#define RECURSIVE_PDE_INDEX           1023U

#define RECURSIVE_PD_VADDR            0xFFFFF000U
#define VMM_PAGE_TABLE_VADDR(index) \
    (RECURSIVE_START + ((uint32_t)(index) * PAGE_SIZE))

#define KSTACK_GUARD_PAGES            1U
#define KSTACK_PAGES                  1U
#define KSTACK_SLOT_PAGES             (KSTACK_GUARD_PAGES + KSTACK_PAGES)
#define KSTACK_SLOT_SIZE              (KSTACK_SLOT_PAGES * PAGE_SIZE)
#define KSTACK_SLOT_COUNT \
    ((KERNEL_STACK_END_EXCLUSIVE - KERNEL_STACK_START) / KSTACK_SLOT_SIZE)

/* Checked conversions apply only to the fixed low-physical direct map. */
int vmm_direct_phys_to_virt(uint32_t phys, uint32_t *virt_out);
int vmm_direct_virt_to_phys(uint32_t virt, uint32_t *phys_out);

void vmm_init(void);
int vmm_map_page(uint32_t virt, uint32_t phys, uint32_t flags);
int vmm_unmap_page(uint32_t virt);
uint32_t vmm_get_physaddr(uint32_t virt);

#endif /* KERNEL_INCLUDE_VMM_H */
