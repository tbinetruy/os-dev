/*
 * heap.c - Kernel Heap Allocator
 *
 * Simple linked list allocator with first-fit algorithm.
 * Provides kmalloc()/kfree() for dynamic kernel memory allocation.
 *
 * The heap starts at its fixed VMM-owned region and
 * grows upward by requesting pages from PMM and mapping them via VMM.
 *
 * Block layout in memory:
 *   +-------------------+-------------------+-------------------+
 *   | block_header      | payload (usable)  | block_header      | ...
 *   | size, free, next  | (8-byte aligned)  | size, free, next  |
 *   +-------------------+-------------------+-------------------+
 *   ^                   ^                   ^
 *   block ptr           returned to caller  next block
 *
 * TODO: Add interrupt safety (cli/sti) when allocations may happen
 *       from interrupt context.
 * TODO: Add spinlock when threading is implemented (Epic 4).
 */

#include <heap.h>
#include <pmm.h>
#include <vmm.h>
#include <page.h>
#include <printk.h>
#include <panic.h>
#include <errno.h>
#include <string.h>

/*
 * Block header - metadata for each allocated or free block
 *
 * size:  Total block size in bytes (includes this header)
 * free:  1 if block is free, 0 if allocated
 * next:  Next block in memory order (NOT a free list - all blocks are linked)
 * _padding: Ensures sizeof(struct block_header) == 16 for 8-byte payload alignment
 */
struct block_header {
    uint32_t size;
    uint32_t free;
    struct block_header *next;
    uint32_t _padding;
};

/*
 * Initial heap size: 16 pages = 64KB
 */
#define HEAP_INITIAL_PAGES  16

/*
 * Minimum expansion: 4 pages = 16KB
 */
#define HEAP_MIN_EXPAND_PAGES  4

/*
 * Static variables tracking heap state
 */
static uint32_t heap_start;     /* Virtual address of heap start */
static uint32_t heap_end;       /* Current mapped end of heap */
static struct block_header *free_list;  /* First block in memory */

extern uint8_t __kernel_heap_start[];

static uint32_t heap_backing_address(void)
{
#ifdef HOST_TEST
    extern char heap_buf[];
    return (uint32_t)heap_buf;
#else
    return KERNEL_HEAP_START;
#endif
}

/*
 * heap_expand - Grow the heap by allocating and mapping new pages
 *
 * @needed: Minimum number of bytes needed (used to calculate page count)
 *
 * All-or-nothing: if any page allocation or mapping fails, all
 * previously mapped pages in this call are unmapped and freed,
 * heap_end is restored, and -ENOMEM is returned.
 *
 * Returns: 0 on success, -ENOMEM on failure (heap unchanged)
 */
static int heap_expand(uint32_t needed)
{
    uint32_t pages_needed;
    uint32_t pages_mapped;
    uint32_t i;
    uint32_t new_start;
    uint32_t phys;
    int failed;
    struct block_header *new_block;
    struct block_header *last;

    /* Calculate pages needed (minimum HEAP_MIN_EXPAND_PAGES) */
    if (needed > 0xFFFFFFFFU - (PAGE_SIZE - 1U)) {
        return -ENOMEM;
    }
    pages_needed = (needed + PAGE_SIZE - 1U) / PAGE_SIZE;
    if (pages_needed < HEAP_MIN_EXPAND_PAGES) {
        pages_needed = HEAP_MIN_EXPAND_PAGES;
    }

    new_start = heap_end;
    if (pages_needed >
        (KERNEL_HEAP_END_EXCLUSIVE - heap_end) / PAGE_SIZE) {
        return -ENOMEM;
    }
    pages_mapped = 0;
    failed = 0;

    /* Allocate and map each page */
    for (i = 0; i < pages_needed; i++) {
        phys = pmm_alloc_frame();
        if (phys == 0) {
            printk(LOG_ERROR,
                   "HEAP: expand failed - out of frames after %d pages\n",
                   pages_mapped);
            failed = 1;
            break;
        }

        if (vmm_map_page(heap_end, phys, PAGE_KERNEL) != 0) {
            pmm_free_frame(phys);
            printk(LOG_ERROR,
                   "HEAP: expand failed - vmm_map_page after %d pages\n",
                   pages_mapped);
            failed = 1;
            break;
        }

        heap_end += PAGE_SIZE;
        pages_mapped++;
    }

    /* Roll back on failure: unmap and free all pages from this call */
    if (failed) {
        for (i = 0; i < pages_mapped; i++) {
            uint32_t vaddr = new_start + i * PAGE_SIZE;
            uint32_t paddr = vmm_get_physaddr(vaddr);

            vmm_unmap_page(vaddr);
            if (paddr != 0) {
                pmm_free_frame(paddr);
            }
        }
        heap_end = new_start;
        return -ENOMEM;
    }

    /* Zero new pages */
    memset((void *)new_start, 0, pages_needed * PAGE_SIZE);

    /* Create a new free block spanning the new pages */
    new_block = (struct block_header *)new_start;
    new_block->size = pages_needed * PAGE_SIZE;
    new_block->free = 1;
    new_block->next = NULL;
    new_block->_padding = 0;

    /*
     * Find the last block in the list and link the new block.
     * If the last block is free, coalesce (absorb new into last).
     */
    last = free_list;
    while (last->next != NULL) {
        last = last->next;
    }

    if (last->free) {
        /* Coalesce: absorb new block into last existing free block */
        last->size += new_block->size;
    } else {
        /* Link new block after the last block */
        last->next = new_block;
    }

    printk(LOG_DEBUG, "HEAP: expanded by %d pages (%d KB)\n",
           pages_needed, (pages_needed * PAGE_SIZE) / 1024);

    return 0;
}

/*
 * heap_init - Initialize the kernel heap
 *
 * Allocates initial pages from PMM, maps them into kernel virtual
 * address space, and creates the initial free block.
 */
void heap_init(void)
{
    uint32_t i;
    uint32_t phys;

    if ((uint32_t)__kernel_heap_start != KERNEL_HEAP_START) {
#ifndef HOST_TEST
        panic("HEAP: linker/C heap start mismatch");
#endif
    }
    heap_start = heap_backing_address();
    heap_end = heap_start;

    /* Allocate and map initial heap pages */
    for (i = 0; i < HEAP_INITIAL_PAGES; i++) {
        phys = pmm_alloc_frame();
        if (phys == 0) {
            panic("HEAP: cannot allocate initial frames");
        }

        if (vmm_map_page(heap_end, phys, PAGE_KERNEL) != 0) {
            panic("HEAP: cannot map initial pages");
        }

        heap_end += PAGE_SIZE;
    }

    /* Zero the initial heap memory */
    memset((void *)heap_start, 0, HEAP_INITIAL_PAGES * PAGE_SIZE);

    /* Create initial free block spanning entire heap region */
    {
        struct block_header *initial = (struct block_header *)heap_start;
        initial->size = HEAP_INITIAL_PAGES * PAGE_SIZE;
        initial->free = 1;
        initial->next = NULL;
        initial->_padding = 0;
        free_list = initial;
    }

    printk(LOG_INFO, "HEAP: initialized %d KB at 0x%x-0x%x\n",
           (HEAP_INITIAL_PAGES * PAGE_SIZE) / 1024, heap_start, heap_end);
}

/*
 * try_alloc - Search free list for a suitable block
 *
 * First-fit search. Splits block if remainder is large enough.
 *
 * @total_needed: Total bytes needed (header + aligned payload)
 *
 * Returns: Pointer to payload, or NULL if no suitable block found
 */
static void *try_alloc(uint32_t total_needed)
{
    struct block_header *current;
    struct block_header *new_block;
    uint32_t remaining;

    current = free_list;
    while (current != NULL) {
        if (current->free && current->size >= total_needed) {
            remaining = current->size - total_needed;

            if (remaining >= HEAP_MIN_BLOCK_SIZE + sizeof(struct block_header)) {
                /*
                 * Split: carve out exactly total_needed, create new free
                 * block for the remainder.
                 */
                new_block = (struct block_header *)
                    ((uint8_t *)current + total_needed);
                new_block->size = remaining;
                new_block->free = 1;
                new_block->next = current->next;
                new_block->_padding = 0;

                current->size = total_needed;
                current->next = new_block;
            }

            current->free = 0;
            return (void *)((uint8_t *)current + sizeof(struct block_header));
        }
        current = current->next;
    }

    return NULL;
}

/*
 * kmalloc - Allocate kernel memory
 *
 * First-fit search through linked block list. Splits blocks if
 * remainder is large enough. Expands heap if no suitable block found.
 */
void *kmalloc(size_t size)
{
    uint32_t aligned_size;
    uint32_t total_needed;
    void *result;

    if (size == 0) {
        return NULL;
    }

    if (size > 0xFFFFFFFFU - (HEAP_ALIGNMENT - 1U) -
               sizeof(struct block_header)) {
        return NULL;
    }

    /* Round up to alignment boundary */
    aligned_size = (size + HEAP_ALIGNMENT - 1) & ~(HEAP_ALIGNMENT - 1);

    /* Total block size needed = header + aligned payload */
    total_needed = sizeof(struct block_header) + aligned_size;

    /* First attempt */
    result = try_alloc(total_needed);
    if (result != NULL) {
        return result;
    }

    /* No suitable block found - expand the heap */
    if (heap_expand(total_needed) != 0) {
        return NULL;
    }

    /* Retry allocation (one retry only) */
    return try_alloc(total_needed);
}

/*
 * kfree - Free kernel memory
 *
 * Marks the block as free and coalesces with adjacent free blocks
 * (both forward and backward) to reduce fragmentation.
 */
void kfree(void *ptr)
{
    struct block_header *block;
    struct block_header *prev;

    if (ptr == NULL) {
        return;
    }

    /* Validate pointer is within heap region */
    if ((uint32_t)ptr < heap_start || (uint32_t)ptr >= heap_end) {
        printk(LOG_WARN, "HEAP: kfree invalid pointer 0x%x\n",
               (uint32_t)ptr);
        return;
    }

    /* Get block header from pointer */
    block = (struct block_header *)((uint8_t *)ptr - sizeof(struct block_header));

    /* Check for double-free */
    if (block->free) {
        printk(LOG_WARN, "HEAP: double-free detected at 0x%x\n",
               (uint32_t)ptr);
        return;
    }

    /* Mark block as free */
    block->free = 1;

    /* Coalesce forward: merge with next block if it's free */
    if (block->next != NULL && block->next->free) {
        block->size += block->next->size;
        block->next = block->next->next;
    }

    /* Coalesce backward: find previous block and merge if it's free */
    if (block != free_list) {
        prev = free_list;
        while (prev != NULL && prev->next != block) {
            prev = prev->next;
        }

        if (prev != NULL && prev->free) {
            prev->size += block->size;
            prev->next = block->next;
        }
    }
}

/*
 * heap_get_free - Get total free bytes in heap
 *
 * Walks all blocks, summing sizes of free blocks.
 */
uint32_t heap_get_free(void)
{
    struct block_header *current;
    uint32_t total = 0;

    current = free_list;
    while (current != NULL) {
        if (current->free) {
            total += current->size;
        }
        current = current->next;
    }

    return total;
}

/*
 * heap_get_used - Get total used bytes in heap
 *
 * Walks all blocks, summing sizes of allocated blocks.
 */
uint32_t heap_get_used(void)
{
    struct block_header *current;
    uint32_t total = 0;

    current = free_list;
    while (current != NULL) {
        if (!current->free) {
            total += current->size;
        }
        current = current->next;
    }

    return total;
}
