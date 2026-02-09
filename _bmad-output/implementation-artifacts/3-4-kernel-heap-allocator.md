# Story 3.4: Kernel Heap Allocator

Status: done

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a developer,
I want kmalloc/kfree for dynamic kernel memory allocation,
so that I can allocate variable-sized objects without manual frame management.

## Acceptance Criteria

1. **AC1: Heap Initialization**
   - Given paging is enabled (vmm_init() complete)
   - When heap_init() is called
   - Then kernel heap region is established starting after kernel BSS
   - And initial heap pages are mapped into virtual address space
   - And heap is ready for allocations

2. **AC2: kmalloc Basic Allocation**
   - Given heap is initialized
   - When kmalloc(size) is called
   - Then block of at least 'size' bytes is returned
   - And returned pointer is aligned to 8 bytes
   - And memory is from kernel heap region (above _kernel_end)
   - And writing to allocated memory does not cause page faults

3. **AC3: kfree and Reuse**
   - Given memory was allocated with kmalloc
   - When kfree(ptr) is called
   - Then memory is returned to free pool
   - And memory can be reused by future kmalloc calls

4. **AC4: Heap Growth**
   - Given heap runs out of space in current mapped pages
   - When kmalloc needs more memory
   - Then new pages are allocated via pmm_alloc_frame()
   - And pages are mapped into heap region via vmm_map_page()
   - And allocation succeeds transparently

5. **AC5: Out-of-Memory Handling**
   - Given all physical frames are exhausted
   - When kmalloc cannot grow the heap
   - Then NULL is returned
   - And no crash occurs

6. **AC6: Coalescing**
   - Given adjacent free blocks exist after kfree
   - When kfree merges adjacent blocks
   - Then contiguous free space is available for larger allocations
   - And fragmentation is reduced

7. **AC7: No Memory Leaks**
   - Given I allocate and free memory repeatedly
   - When checking for leaks
   - Then free frame count returns to original value after all frees
   - And NFR18 (no memory leaks) is verifiable

8. **AC8: Code Quality**
   - Given kernel/mm/heap.c source
   - When I examine the code
   - Then free list block header structure is documented
   - And first-fit algorithm is used (per Architecture)
   - And coalescing of adjacent free blocks is implemented
   - And code follows project conventions (snake_case, K&R, 4-space indent)

## Tasks / Subtasks

- [x] **Task 1: Create heap header** (AC: #1, #2, #3, #8)
  - [x] 1.1 Create `kernel/include/heap.h` with header guard `KERNEL_INCLUDE_HEAP_H`
  - [x] 1.2 Declare: `void heap_init(void);`
  - [x] 1.3 Declare: `void *kmalloc(size_t size);`
  - [x] 1.4 Declare: `void kfree(void *ptr);`
  - [x] 1.5 Declare optional debug helpers: `uint32_t heap_get_free(void);` and `uint32_t heap_get_used(void);`
  - [x] 1.6 Define `HEAP_ALIGNMENT` as 8 and `HEAP_MIN_BLOCK_SIZE` as 16 (header + minimum usable payload)
  - [x] 1.7 Include `<types.h>` for size_t

- [x] **Task 2: Implement heap data structures** (AC: #1, #8)
  - [x] 2.1 Create `kernel/mm/heap.c`
  - [x] 2.2 Define `struct block_header` with fields: `size` (uint32_t, includes header), `free` (uint32_t, 0 or 1), `next` (struct block_header pointer to next block in memory order)
  - [x] 2.3 Define static variables: `heap_start` (virtual address), `heap_end` (current mapped end), `free_list` (pointer to first block)
  - [x] 2.4 Document the block layout in a comment:
    ```
    [block_header | payload ... | block_header | payload ... ]
    ^             ^
    header        returned pointer (8-byte aligned)
    ```

- [x] **Task 3: Implement heap_init()** (AC: #1)
  - [x] 3.1 Calculate heap start: page-align `_kernel_end` upward to next page boundary
  - [x] 3.2 Allocate initial heap pages (e.g., 16 pages = 64KB) from PMM via `pmm_alloc_frame()`
  - [x] 3.3 Map each page into kernel virtual address space via `vmm_map_page(virt, phys, PAGE_KERNEL)`
  - [x] 3.4 Zero the initial heap memory with `memset()`
  - [x] 3.5 Create initial free block spanning entire heap region (minus header overhead)
  - [x] 3.6 Set `free_list = heap_start`
  - [x] 3.7 Log initialization: `printk(LOG_INFO, "HEAP: initialized %d KB at 0x%x-0x%x\n", ...)`
  - [x] 3.8 Handle failure: if pmm_alloc_frame() returns 0, panic (heap is required for kernel operation)

- [x] **Task 4: Implement kmalloc()** (AC: #2, #4, #5)
  - [x] 4.1 Round up requested size to `HEAP_ALIGNMENT` boundary (8 bytes)
  - [x] 4.2 Add `sizeof(struct block_header)` to get total block size needed
  - [x] 4.3 Walk free list using **first-fit**: find first free block with `block->size >= needed`
  - [x] 4.4 If found block is significantly larger than needed (remaining >= `HEAP_MIN_BLOCK_SIZE`), **split** the block: carve out exact needed size, create new free block for remainder
  - [x] 4.5 Mark found block as `free = 0`
  - [x] 4.6 Return pointer to payload (address after block_header)
  - [x] 4.7 If no suitable block found, call `heap_expand()` to grow the heap
  - [x] 4.8 After expansion, retry allocation (one retry only)
  - [x] 4.9 Return NULL if expansion fails or retry fails

- [x] **Task 5: Implement heap_expand()** (AC: #4, #5)
  - [x] 5.1 Determine how many pages to allocate (at least enough for the requested size, minimum 4 pages = 16KB)
  - [x] 5.2 For each page: `pmm_alloc_frame()` then `vmm_map_page(heap_end, phys, PAGE_KERNEL)`
  - [x] 5.3 Zero new pages with `memset()`
  - [x] 5.4 Create a new free block spanning the new pages
  - [x] 5.5 If the last existing block is free, coalesce it with the new block (merge into one larger free block)
  - [x] 5.6 Otherwise, link new block as `next` of the last block
  - [x] 5.7 Update `heap_end`
  - [x] 5.8 Return 0 on success, -ENOMEM if pmm_alloc_frame() fails

- [x] **Task 6: Implement kfree()** (AC: #3, #6)
  - [x] 6.1 Validate pointer: not NULL, within heap region (`heap_start` to `heap_end`)
  - [x] 6.2 Compute block_header address: `ptr - sizeof(struct block_header)`
  - [x] 6.3 Verify block is currently allocated (`free == 0`); warn and return on double-free
  - [x] 6.4 Mark block as `free = 1`
  - [x] 6.5 **Coalesce forward**: if `next` block exists and is free, merge current with next (add next's size to current, update next pointer to skip merged block)
  - [x] 6.6 **Coalesce backward**: walk from `free_list` to find previous block; if previous is free, merge previous with current
  - [x] 6.7 Note: backward coalescing requires a linear walk from the start. This is acceptable for a simple allocator. An optimization (prev pointer or doubly-linked) can be added in future if needed.

- [x] **Task 7: Implement debug helpers** (AC: #7)
  - [x] 7.1 `heap_get_free()`: walk all blocks, sum sizes of free blocks
  - [x] 7.2 `heap_get_used()`: walk all blocks, sum sizes of allocated blocks
  - [x] 7.3 These are used by tests to verify no leaks

- [x] **Task 8: Integrate into kernel initialization** (AC: #1)
  - [x] 8.1 In `kernel/init/main.c`, add `#include <heap.h>`
  - [x] 8.2 Add `heap_init();` call after `vmm_init();` and before `sti();`
  - [x] 8.3 Verify boot succeeds with heap initialization

- [x] **Task 9: Write tests** (AC: #1-8)
  - [x] 9.1 Create `kernel/test/test_heap.c`
  - [x] 9.2 Test: `kmalloc(64)` returns non-NULL, 8-byte aligned pointer within heap region
  - [x] 9.3 Test: write to allocated memory doesn't fault, data preserved on read-back
  - [x] 9.4 Test: `kfree()` then `kmalloc()` reuses freed memory (pointer within same region)
  - [x] 9.5 Test: multiple allocations return different pointers
  - [x] 9.6 Test: coalescing - allocate A, B, C; free B, free C; new allocation of size B+C fits in merged block
  - [x] 9.7 Test: large allocation triggers heap growth (allocate more than initial heap size)
  - [x] 9.8 Test: allocate-free cycle returns free frame count to original (no leaks) - compare `pmm_get_free_count()` before and after
  - [x] 9.9 Test: `kmalloc(0)` returns NULL or valid pointer (define behavior, document choice)
  - [x] 9.10 Test: `kfree(NULL)` does not crash
  - [x] 9.11 Add `test_heap()` to `kernel/test/test_runner.c`

- [x] **Task 10: Integration and verification** (AC: #1-8)
  - [x] 10.1 Run `make test` - all existing tests pass (no regressions from 408 existing tests)
  - [x] 10.2 Run `make qemu` - kernel boots normally with heap initialized
  - [x] 10.3 Verify heap log message appears in serial output
  - [x] 10.4 Verify no memory corruption or page faults during normal boot

---

## Dev Notes

### What This Story Accomplishes

This is **Story 3.4, the final story in Epic 3** (Memory Management). After this:
- `kmalloc()` / `kfree()` are available for all kernel subsystems
- Dynamic allocation replaces manual frame management for variable-sized objects
- Epic 4 (Kernel Threads) can allocate `task_struct` and kernel stacks dynamically
- Foundation for all future kernel data structures requiring dynamic memory

### Architecture Specification

**From Architecture Document:**
- **Algorithm:** Simple linked list with first-fit allocation
- **Interface:** `kmalloc(size)` / `kfree(ptr)`
- **Location:** `kernel/mm/heap.c`
- **Future:** Interface supports upgrade to buddy/slab if needed

### Existing Infrastructure to Use (DO NOT REIMPLEMENT)

**PMM (Physical Memory Manager) - `kernel/include/pmm.h`:**
```c
uint32_t pmm_alloc_frame(void);      /* Returns phys addr, 0 on failure */
void pmm_free_frame(uint32_t addr);  /* Free a page frame */
uint32_t pmm_get_free_count(void);   /* For leak testing */
```

**VMM (Virtual Memory Manager) - `kernel/include/vmm.h`:**
```c
int vmm_map_page(uint32_t virt, uint32_t phys, uint32_t flags);  /* 0 on success */
void vmm_unmap_page(uint32_t virt);
#define PAGE_KERNEL  (PAGE_PRESENT | PAGE_WRITABLE)  /* from page.h */
```

**Address Macros - `kernel/include/vmm.h`:**
```c
#define KERNEL_BASE  0xC0000000
#define P2V(phys)    ((uint32_t)(phys) + KERNEL_BASE)
#define V2P(virt)    ((uint32_t)(virt) - KERNEL_BASE)
```

**Linker Symbols - `scripts/kernel.ld`:**
```c
extern uint32_t _kernel_end;   /* End of kernel virtual address (0xC0XXXXXX) */
```
To use as an address: `(uint32_t)&_kernel_end`

**Utility Functions:**
```c
void *memset(void *s, int c, size_t n);   /* kernel/lib/string.c */
void printk(int level, const char *fmt, ...);  /* kernel/lib/printf.c */
```

**Logging Levels - `kernel/include/printk.h`:**
```c
#define LOG_ERROR  0
#define LOG_WARN   1
#define LOG_INFO   2
#define LOG_DEBUG  3
```

**Error Codes - `kernel/include/errno.h`:**
```c
#define ENOMEM  12   /* Out of memory */
```

**Types - `kernel/include/types.h`:**
```c
typedef uint32_t size_t;
#define NULL ((void *)0)
```

**Test Macro Pattern:**
```c
TEST_ASSERT(condition, "test_name");  /* From existing test framework */
```

### Block Header Design

```
Memory layout:
+-------------------+-------------------+-------------------+
| block_header      | payload (usable)  | block_header      | ...
| size, free, next  | (8-byte aligned)  | size, free, next  |
+-------------------+-------------------+-------------------+
^                   ^                   ^
block ptr           returned to caller  next block
```

The `struct block_header` must be sized so that the payload after it is 8-byte aligned. Since `block_header` contains `uint32_t size` + `uint32_t free` + pointer `next` = 12 bytes, pad to 16 bytes (or use a design where `sizeof(struct block_header)` is a multiple of 8).

**Recommended struct:**
```c
struct block_header {
    uint32_t size;              /* Total block size including header */
    uint32_t free;              /* 1 if free, 0 if allocated */
    struct block_header *next;  /* Next block in memory order */
    uint32_t _padding;          /* Align to 16 bytes */
};
```
This ensures payload starts at a 16-byte aligned offset (which is also 8-byte aligned).

### Heap Region Location

```
Virtual Address Space:
0xC0000000  ┌──────────────────┐  KERNEL_BASE
            │   Kernel Code    │
            │   Kernel Data    │
            │   Kernel BSS     │
_kernel_end ├──────────────────┤  <-- heap starts here (page-aligned up)
            │   Kernel Heap    │  grows upward (heap_end increases)
            │   (mapped on     │
            │    demand)       │
            ├──────────────────┤  heap_end (current mapped limit)
            │   Unmapped       │
            │                  │
0xFFFFFFFF  └──────────────────┘
```

Heap start = page-align-up(`(uint32_t)&_kernel_end`):
```c
#define PAGE_ALIGN_UP(addr)  (((addr) + PAGE_SIZE - 1) & PAGE_MASK)
heap_start = PAGE_ALIGN_UP((uint32_t)&_kernel_end);
```

### Heap Growth Strategy

When `kmalloc()` cannot find a suitable block:
1. Calculate pages needed: `max(requested_size / PAGE_SIZE + 1, 4)` (minimum 4 pages = 16KB per expansion)
2. For each page: allocate frame, map at `heap_end`, advance `heap_end`
3. Create free block spanning new pages, coalesce with last block if free
4. Retry allocation

### Coalescing Algorithm

On `kfree(ptr)`:
1. Mark block as free
2. **Forward coalesce**: check if `block->next` is free; if so, absorb it: `block->size += block->next->size; block->next = block->next->next;`
3. **Backward coalesce**: find previous block by walking from `free_list`; if previous is free, absorb current into previous

### Makefile - No Changes Needed

The Makefile uses wildcards to discover source files:
```makefile
KERNEL_C_SRCS := ... $(wildcard kernel/mm/*.c)
```
New `kernel/mm/heap.c` will be automatically compiled and linked.

### Previous Story Intelligence

**From Story 3.3 (Page Fault Handler):**
- 408 tests currently pass - do NOT regress
- Test hook mechanism exists for page fault testing (pf_set_test_hook)
- Page faults in kernel mode trigger panic - heap code must never access unmapped memory
- `printk` format: don't use width specifiers (`%08x`), use `%x`

**From Story 3.2 (Paging & Virtual Memory):**
- `vmm_map_page()` returns 0 on success, -ENOMEM on failure
- `vmm_unmap_page()` does NOT free the physical frame (caller's responsibility)
- Boot page tables map first 16MB for both identity and higher-half
- Page flags: `PAGE_KERNEL` = `PAGE_PRESENT | PAGE_WRITABLE`

**From Story 3.1 (PMM):**
- `pmm_alloc_frame()` returns physical address (4KB-aligned), 0 on failure
- `pmm_free_frame()` validates alignment and range, warns on double-free
- `FIRST_ALLOC_FRAME = 256` (first allocatable after 1MB)

### Critical Constraints

- **No web research needed**: This is a bare-metal allocator with no external library dependencies
- **Thread safety**: Not needed yet (single-threaded kernel). Add TODO comment for future spinlock protection
- **No floating point**: Never use float/double in kernel code
- **Stack size is small**: Don't allocate large arrays on stack in heap functions
- **Interrupt safety**: Consider disabling interrupts around free list manipulation (cli/sti) if timer interrupts could trigger allocations. For now, heap_init runs before sti(), and subsequent allocations happen with interrupts enabled. Add TODO for interrupt safety.
- **NULL dereference**: Always validate pointers before use

### Expected Commit

```
feat[story 3.4]: kernel heap allocator.
```

### Files to Create

- `kernel/include/heap.h` - Header with declarations
- `kernel/mm/heap.c` - Implementation (heap_init, kmalloc, kfree, heap_expand)
- `kernel/test/test_heap.c` - Tests

### Files to Modify

- `kernel/init/main.c` - Add `heap_init()` call and `#include <heap.h>`
- `kernel/test/test_runner.c` - Add `test_heap()` registration

### Files NOT to Change

- `kernel/mm/pmm.c` - PMM is complete
- `kernel/mm/vmm.c` - VMM is complete
- `kernel/include/pmm.h` - PMM interface is stable
- `kernel/include/vmm.h` - VMM interface is stable
- `scripts/kernel.ld` - Linker symbols already sufficient
- `Makefile` - Wildcard auto-discovers new .c files

### Project Structure Notes

- `kernel/mm/heap.c` aligns with architecture: "kernel/mm/heap.c - Kernel heap allocator"
- `kernel/include/heap.h` follows existing pattern (pmm.h, vmm.h are in kernel/include/)
- Tests follow `kernel/test/test_<subsystem>.c` pattern
- No new directories needed

### References

- [Source: _bmad-output/planning-artifacts/architecture.md#Memory-Architecture] - "Simple linked list allocator with first-fit"
- [Source: _bmad-output/planning-artifacts/architecture.md#Core-Architectural-Decisions] - kmalloc/kfree interface
- [Source: _bmad-output/planning-artifacts/epics.md#Story-3.4] - Full acceptance criteria
- [Source: _bmad-output/project-context.md#Memory-Rules] - No malloc until heap implemented
- [Source: _bmad-output/project-context.md#Critical-Donts] - No large stack arrays, no FPU
- [Source: _bmad-output/project-context.md#Testing-Rules] - test_<subsystem>() pattern
- [Source: scripts/kernel.ld] - _kernel_end linker symbol
- [Source: kernel/include/pmm.h] - PMM interface for frame allocation
- [Source: kernel/include/vmm.h] - VMM interface for page mapping
- [Source: kernel/include/page.h] - PAGE_KERNEL flag combination
- [Source: _bmad-output/implementation-artifacts/3-3-page-fault-handler.md] - Previous story learnings

---

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- Build: clean compile with -Werror, zero warnings
- Tests: 559 passed, 0 failed (151 heap tests + 408 existing)
- Boot: HEAP initialized 64 KB at 0xc012b000-0xc013b000, no faults

### Completion Notes List

- Implemented simple linked-list allocator with first-fit algorithm per architecture spec
- struct block_header: 16 bytes (size, free, next, _padding) for 8-byte payload alignment
- heap_init(): allocates 16 pages (64KB) initial heap from PMM, maps via VMM, panics on failure
- kmalloc(): first-fit search, block splitting, single-retry after heap_expand()
- kfree(): pointer validation, double-free detection, forward+backward coalescing
- heap_expand(): minimum 4-page (16KB) expansion, coalesces with last block if free
- heap_get_free()/heap_get_used(): debug walk helpers for leak detection
- kmalloc(0) returns NULL (defined behavior, documented in test)
- Added TODO comments for spinlock (Epic 4) and interrupt safety
- All 11 test cases pass covering AC1-AC8
- No regressions: 408 prior tests still pass

### Change Log

- 2026-02-05: Story 3.4 implementation complete - kernel heap allocator with kmalloc/kfree
- 2026-02-06: Code review fixes - heap_expand partial failure cleanup, extracted try_alloc helper, fixed HEAP_MIN_BLOCK_SIZE comment, added multi-expansion test, zeroed _padding on split

### File List

**Created:**
- kernel/include/heap.h
- kernel/mm/heap.c
- kernel/test/test_heap.c

**Modified:**
- kernel/init/main.c
- kernel/test/test_runner.c
