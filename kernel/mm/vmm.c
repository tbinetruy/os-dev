/* vmm.c - Recursive page-table based virtual memory manager. */

#include <vmm.h>

#include <asm.h>
#include <errno.h>
#include <page.h>
#include <panic.h>
#include <pmm.h>
#include <printk.h>
#include <string.h>

extern uint32_t boot_page_directory[];

static uint32_t *page_directory;

static int ordinary_address_allowed(uint32_t virt)
{
    return !((virt >= KERNEL_RESERVED_START &&
              virt < KERNEL_RESERVED_END_EXCLUSIVE) ||
             virt >= RECURSIVE_START);
}

static uint32_t *page_table_alias(uint32_t pde_idx)
{
    return (uint32_t *)VMM_PAGE_TABLE_VADDR(pde_idx);
}

static uint32_t *get_or_create_page_table(uint32_t virt, bool create)
{
    uint32_t pde_idx = PDE_INDEX(virt);
    uint32_t prior_pde = page_directory[pde_idx];
    uint32_t pt_phys;
    uint32_t *table;

    if (prior_pde & PAGE_PRESENT) {
        return page_table_alias(pde_idx);
    }
    if (!create) {
        return NULL;
    }

    pt_phys = pmm_alloc_frame();
    if (pt_phys == 0) {
        return NULL;
    }

    page_directory[pde_idx] = pt_phys | PAGE_PRESENT | PAGE_WRITABLE;
    invlpg(VMM_PAGE_TABLE_VADDR(pde_idx));
    table = page_table_alias(pde_idx);
    memset(table, 0, PAGE_SIZE);
    return table;
}

void vmm_init(void)
{
    uint32_t cr0 = read_cr0();
    uint32_t cr3;

    if (!(cr0 & 0x80000000U)) {
        panic("vmm_init: paging not enabled");
    }

    cr3 = read_cr3() & PAGE_FRAME_MASK;
    boot_page_directory[RECURSIVE_PDE_INDEX] =
        cr3 | PAGE_PRESENT | PAGE_WRITABLE;
    write_cr3(cr3);
    page_directory = (uint32_t *)RECURSIVE_PD_VADDR;

    if ((page_directory[RECURSIVE_PDE_INDEX] & PAGE_FRAME_MASK) != cr3 ||
        (page_directory[RECURSIVE_PDE_INDEX] & PAGE_USER) != 0) {
        panic("vmm_init: invalid recursive page directory mapping");
    }

    printk(LOG_INFO, "VMM: recursive PD at 0x%x (CR3=0x%x)\n",
           RECURSIVE_PD_VADDR, cr3);
}

int vmm_map_page(uint32_t virt, uint32_t phys, uint32_t flags)
{
    uint32_t *table;
    uint32_t pte_idx;

    if ((virt & (PAGE_SIZE - 1U)) != 0 ||
        (phys & (PAGE_SIZE - 1U)) != 0 ||
        !ordinary_address_allowed(virt)) {
        return -EINVAL;
    }

    table = get_or_create_page_table(virt, true);
    if (table == NULL) {
        return -ENOMEM;
    }
    pte_idx = PTE_INDEX(virt);
    if (table[pte_idx] & PAGE_PRESENT) {
        return -EEXIST;
    }

    table[pte_idx] = phys | (flags & PAGE_FLAGS_MASK) | PAGE_PRESENT;
    invlpg(virt);
    return 0;
}

int vmm_unmap_page(uint32_t virt)
{
    uint32_t *table;

    if ((virt & (PAGE_SIZE - 1U)) != 0 || !ordinary_address_allowed(virt)) {
        return -EINVAL;
    }

    table = get_or_create_page_table(virt, false);
    if (table == NULL) {
        return 0;
    }
    table[PTE_INDEX(virt)] = 0;
    invlpg(virt);
    return 0;
}

uint32_t vmm_get_physaddr(uint32_t virt)
{
    uint32_t *table = get_or_create_page_table(virt, false);
    uint32_t pte;

    if (table == NULL) {
        return 0;
    }
    pte = table[PTE_INDEX(virt)];
    if (!(pte & PAGE_PRESENT)) {
        return 0;
    }
    return (pte & PAGE_FRAME_MASK) | PAGE_OFFSET(virt);
}
