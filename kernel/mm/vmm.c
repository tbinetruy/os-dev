/*
 * vmm.c - Virtual Memory Manager Implementation
 *
 * Manages virtual address space using x86 two-level paging.
 *
 * After vmm_init(), the kernel is running at higher-half virtual addresses
 * (>= 0xC0000000). Physical addresses must be converted using P2V() before
 * dereferencing.
 *
 * Page Table Management:
 *   - Page Directory: 1024 entries, each pointing to a Page Table
 *   - Page Table: 1024 entries, each mapping a 4KB page
 *   - Total: 1024 × 1024 × 4KB = 4GB addressable
 *
 * TODO(multicore): vmm_map_page() and vmm_unmap_page() are not
 * thread-safe. When adding SMP/multi-threading support, protect
 * page table modifications with a spinlock and disable interrupts
 * to prevent races between threads and interrupt handlers.
 *
 * Higher-Half Kernel:
 *   - Kernel code/data at virtual 0xC0100000+
 *   - Physical 0x00000000-0x003FFFFF mapped to virtual 0xC0000000-0xC03FFFFF
 *   - User space (future): 0x00000000-0xBFFFFFFF
 */

#include <vmm.h>
#include <page.h>
#include <pmm.h>
#include <errno.h>
#include <asm.h>
#include <string.h>
#include <printk.h>
#include <panic.h>

/*
 * =============================================================================
 * Private State
 * =============================================================================
 */

/*
 * Kernel page directory (set up by entry.S)
 *
 * This is the virtual address of the boot page directory.
 * We can access it via higher-half address after paging is enabled.
 */
static uint32_t *kernel_page_directory;

/*
 * Boot page directory from entry.S
 * Declared as extern array so we get its address, not contents
 */
extern uint32_t boot_page_directory[];

/*
 * =============================================================================
 * Private Helper Functions
 * =============================================================================
 */

/*
 * get_or_create_page_table - Get page table for a virtual address
 *
 * If the page table doesn't exist, allocates one from PMM.
 *
 * @virt: Virtual address
 * @create: If true, create page table if it doesn't exist
 *
 * Returns: Virtual address of page table, or NULL if not present and !create
 */
static uint32_t *get_or_create_page_table(uint32_t virt, bool create)
{
    uint32_t pde_idx = PDE_INDEX(virt);
    uint32_t pde = kernel_page_directory[pde_idx];

    if (pde & PAGE_PRESENT) {
        /* Page table exists - return its virtual address */
        uint32_t pt_phys = pde & PAGE_FRAME_MASK;
        return (uint32_t *)P2V(pt_phys);
    }

    if (!create) {
        return NULL;
    }

    /* Allocate new page table */
    uint32_t pt_phys = pmm_alloc_frame();
    if (pt_phys == 0) {
        return NULL;    /* Out of memory */
    }

    /* Zero the new page table */
    uint32_t *pt_virt = (uint32_t *)P2V(pt_phys);
    memset(pt_virt, 0, PAGE_SIZE);

    /* Add to page directory with kernel flags */
    kernel_page_directory[pde_idx] = pt_phys | PAGE_PRESENT | PAGE_WRITABLE;

    return pt_virt;
}

/*
 * =============================================================================
 * Public Functions
 * =============================================================================
 */

/*
 * vmm_init - Initialize the virtual memory manager
 *
 * Called after pmm_init() and after paging has been enabled by entry.S.
 * Validates that paging is working and stores references for later use.
 */
void vmm_init(void)
{
    /*
     * Verify paging is enabled
     */
    uint32_t cr0 = read_cr0();
    if (!(cr0 & 0x80000000)) {
        printk(LOG_ERROR, "VMM: CR0=0x%x\n", cr0);
        panic("vmm_init: paging not enabled");
    }

    /*
     * Get kernel page directory virtual address
     *
     * boot_page_directory is declared as an extern symbol.
     * Its address is already a virtual address (from linker).
     */
    kernel_page_directory = boot_page_directory;

    /*
     * Verify we're running at higher-half address
     */
    uint32_t vmm_addr = (uint32_t)&vmm_init;
    if (vmm_addr < KERNEL_BASE) {
        printk(LOG_ERROR, "VMM: addr=0x%x\n", vmm_addr);
        panic("vmm_init: not running at higher-half");
    }

    /*
     * Verify page directory address in CR3 matches our expectation
     */
    uint32_t cr3 = read_cr3();
    uint32_t pd_phys = V2P((uint32_t)boot_page_directory);
    if (cr3 != pd_phys) {
        printk(LOG_WARN, "VMM: CR3 mismatch (CR3=0x%x, expected=0x%x)\n",
               cr3, pd_phys);
    }

    printk(LOG_INFO, "VMM: initialized, kernel at 0x%x, PD at 0x%x\n",
           vmm_addr, (uint32_t)kernel_page_directory);
}

/*
 * vmm_map_page - Map a virtual address to a physical address
 *
 * Creates or updates the page table entry for the given virtual address.
 * Allocates a new page table from PMM if necessary.
 *
 * @virt:  Virtual address to map (will be page-aligned)
 * @phys:  Physical address to map to (will be page-aligned)
 * @flags: Page flags (PAGE_PRESENT, PAGE_WRITABLE, PAGE_USER)
 *
 * Returns: 0 on success, -ENOMEM if page table allocation fails
 */
int vmm_map_page(uint32_t virt, uint32_t phys, uint32_t flags)
{
    /* Page-align addresses */
    virt &= PAGE_MASK;
    phys &= PAGE_MASK;

    /* Get or create page table */
    uint32_t *page_table = get_or_create_page_table(virt, true);
    if (page_table == NULL) {
        return -ENOMEM;
    }

    /* Set page table entry */
    uint32_t pte_idx = PTE_INDEX(virt);
    page_table[pte_idx] = phys | (flags & PAGE_FLAGS_MASK) | PAGE_PRESENT;

    /* Invalidate TLB for this address */
    invlpg(virt);

    return 0;
}

/*
 * vmm_unmap_page - Unmap a virtual address
 *
 * Clears the page table entry and invalidates the TLB.
 * Does NOT free the physical frame - caller's responsibility.
 *
 * @virt: Virtual address to unmap (will be page-aligned)
 */
void vmm_unmap_page(uint32_t virt)
{
    /* Page-align address */
    virt &= PAGE_MASK;

    /* Get page table (don't create if missing) */
    uint32_t *page_table = get_or_create_page_table(virt, false);
    if (page_table == NULL) {
        return;     /* Already unmapped (no page table) */
    }

    /* Clear page table entry */
    uint32_t pte_idx = PTE_INDEX(virt);
    page_table[pte_idx] = 0;

    /* Invalidate TLB */
    invlpg(virt);
}

/*
 * vmm_get_physaddr - Get physical address for a virtual address
 *
 * Walks the page tables to find the physical address mapped
 * to the given virtual address.
 *
 * @virt: Virtual address to translate
 *
 * Returns: Physical address, or 0 if not mapped
 *
 * Note: Returns 0 for unmapped pages. Physical address 0 is reserved
 * by PMM (first 1MB), so 0 is safe as an error sentinel.
 */
uint32_t vmm_get_physaddr(uint32_t virt)
{
    /* Get page table (don't create if missing) */
    uint32_t *page_table = get_or_create_page_table(virt, false);
    if (page_table == NULL) {
        return 0;   /* Not mapped (no page table) */
    }

    /* Get page table entry */
    uint32_t pte_idx = PTE_INDEX(virt);
    uint32_t pte = page_table[pte_idx];

    if (!(pte & PAGE_PRESENT)) {
        return 0;   /* Not mapped */
    }

    /* Combine frame address with page offset */
    uint32_t frame = pte & PAGE_FRAME_MASK;
    uint32_t offset = PAGE_OFFSET(virt);

    return frame | offset;
}
