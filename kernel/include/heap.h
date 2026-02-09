/*
 * heap.h - Kernel Heap Allocator
 *
 * Provides kmalloc/kfree for dynamic kernel memory allocation using
 * a simple linked list with first-fit algorithm. The heap grows
 * upward from _kernel_end (page-aligned) by mapping new pages on demand.
 *
 * Block layout:
 *   [block_header | payload ... | block_header | payload ... ]
 *   ^             ^
 *   header        returned pointer (8-byte aligned)
 *
 * TODO: Add spinlock protection when threading is implemented (Epic 4).
 */

#ifndef KERNEL_INCLUDE_HEAP_H
#define KERNEL_INCLUDE_HEAP_H

#include <types.h>

/*
 * HEAP_ALIGNMENT - Required alignment for returned pointers
 *
 * All pointers returned by kmalloc are aligned to this boundary.
 */
#define HEAP_ALIGNMENT      8

/*
 * HEAP_MIN_BLOCK_SIZE - Minimum payload size for split remainder blocks
 *
 * When splitting a block, the remainder must have at least
 * sizeof(struct block_header) + HEAP_MIN_BLOCK_SIZE bytes to be
 * worth creating as a separate block. This prevents tiny fragments
 * with unusably small payloads.
 */
#define HEAP_MIN_BLOCK_SIZE 16

/*
 * heap_init - Initialize the kernel heap
 *
 * Must be called after vmm_init(). Allocates initial pages from PMM,
 * maps them into kernel virtual address space, and sets up the free list.
 *
 * Panics if initial allocation fails (heap is required for kernel operation).
 */
void heap_init(void);

/*
 * kmalloc - Allocate kernel memory
 *
 * Allocates at least 'size' bytes of memory from the kernel heap.
 * Uses first-fit algorithm on a linked list of blocks.
 *
 * @size: Number of bytes to allocate
 *
 * Returns: Pointer to allocated memory (8-byte aligned), or NULL on failure
 */
void *kmalloc(size_t size);

/*
 * kfree - Free kernel memory
 *
 * Returns memory previously allocated by kmalloc to the free pool.
 * Adjacent free blocks are coalesced to reduce fragmentation.
 *
 * @ptr: Pointer returned by kmalloc, or NULL (no-op)
 */
void kfree(void *ptr);

/*
 * heap_get_free - Get total free bytes in heap
 *
 * Walks all blocks and sums sizes of free blocks.
 * Used for debugging and leak detection in tests.
 *
 * Returns: Total bytes in free blocks (including headers)
 */
uint32_t heap_get_free(void);

/*
 * heap_get_used - Get total used bytes in heap
 *
 * Walks all blocks and sums sizes of allocated blocks.
 * Used for debugging and leak detection in tests.
 *
 * Returns: Total bytes in allocated blocks (including headers)
 */
uint32_t heap_get_used(void);

#endif /* KERNEL_INCLUDE_HEAP_H */
