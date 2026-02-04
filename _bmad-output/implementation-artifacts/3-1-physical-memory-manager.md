# Story 3.1: Physical Memory Manager

Status: done

## Story

As a developer,
I want a physical memory manager that tracks and allocates page frames,
so that I understand physical memory management and have frames available for paging.

## Acceptance Criteria

1. **AC1: PMM Initialization**
   - Given kernel is initializing
   - When pmm_init(memory_size) is called
   - Then bitmap is created with 1 bit per 4KB frame
   - And frames used by kernel are marked as allocated
   - And frames below 1MB are marked as reserved (BIOS, VGA, etc.)

2. **AC2: Frame Allocation**
   - Given PMM is initialized
   - When pmm_alloc_frame() is called
   - Then first free frame is found via bitmap scan
   - And frame is marked as allocated in bitmap
   - And physical address of frame is returned

3. **AC3: Unique Frame Allocation**
   - Given multiple frames are allocated
   - When pmm_alloc_frame() is called repeatedly
   - Then different frames are returned each time
   - And no frame is returned twice

4. **AC4: Frame Deallocation**
   - Given a frame is in use
   - When pmm_free_frame(phys_addr) is called
   - Then frame is marked as free in bitmap
   - And frame can be allocated again

5. **AC5: Exhaustion Handling**
   - Given all frames are allocated
   - When pmm_alloc_frame() is called
   - Then 0 (or NULL) is returned indicating failure
   - And no crash or corruption occurs

6. **AC6: Code Quality**
   - Given kernel/mm/pmm.c source
   - When I examine the code
   - Then bitmap is stored in kernel BSS or allocated region
   - And PAGE_SIZE is defined as 4096
   - And pmm_get_free_count() returns number of free frames

## Tasks / Subtasks

- [x] **Task 1: Create Memory Map Header** (AC: #1)
  - [x] 1.1 Create `kernel/include/mmap.h` with memory map entry structure
  - [x] 1.2 Define MMAP_TYPE_AVAILABLE (1), MMAP_TYPE_RESERVED (2), etc.
  - [x] 1.3 Declare mmap_entry structure matching BIOS E820 format
  - [x] 1.4 Ensure compatibility with boot_mmap_ptr, boot_mmap_count from entry.S

- [x] **Task 2: Create PMM Header** (AC: #1, #2, #4, #6)
  - [x] 2.1 Create `kernel/include/pmm.h`
  - [x] 2.2 Define PAGE_SIZE (4096), PAGE_SHIFT (12)
  - [x] 2.3 Define KERNEL_PHYS_START (0x100000), KERNEL_PHYS_END (from linker)
  - [x] 2.4 Declare pmm_init(), pmm_alloc_frame(), pmm_free_frame()
  - [x] 2.5 Declare pmm_get_free_count(), pmm_get_total_count()
  - [x] 2.6 Add PHYS_TO_FRAME(), FRAME_TO_PHYS() macros

- [x] **Task 3: Implement Bitmap Operations** (AC: #2, #4)
  - [x] 3.1 Create `kernel/mm/pmm.c`
  - [x] 3.2 Implement bitmap_set(frame_num) - mark frame as allocated
  - [x] 3.3 Implement bitmap_clear(frame_num) - mark frame as free
  - [x] 3.4 Implement bitmap_test(frame_num) - check if frame is allocated
  - [x] 3.5 Store bitmap in static array in BSS (max 128KB for 4GB/4KB)

- [x] **Task 4: Implement PMM Initialization** (AC: #1, #6)
  - [x] 4.1 Implement pmm_init() function
  - [x] 4.2 Parse boot memory map (boot_mmap_ptr, boot_mmap_count)
  - [x] 4.3 Calculate total memory and number of frames
  - [x] 4.4 Initialize bitmap: mark all frames as allocated initially
  - [x] 4.5 Mark available frames as free based on memory map
  - [x] 4.6 Mark frames 0-256 (0-1MB) as reserved (unconditionally)
  - [x] 4.7 Mark kernel frames (KERNEL_PHYS_START to kernel_end) as allocated
  - [x] 4.8 Calculate and store free frame count

- [x] **Task 5: Implement Frame Allocation** (AC: #2, #3, #5)
  - [x] 5.1 Implement pmm_alloc_frame() function
  - [x] 5.2 Scan bitmap for first free frame (bit = 0)
  - [x] 5.3 Start search after 1MB (frame 256) to skip reserved memory
  - [x] 5.4 Mark found frame as allocated (bit = 1)
  - [x] 5.5 Decrement free frame counter
  - [x] 5.6 Return physical address (frame_num * PAGE_SIZE)
  - [x] 5.7 Return 0 if no frames available

- [x] **Task 6: Implement Frame Deallocation** (AC: #4)
  - [x] 6.1 Implement pmm_free_frame(phys_addr)
  - [x] 6.2 Validate address is page-aligned
  - [x] 6.3 Calculate frame number from physical address
  - [x] 6.4 Validate frame is within valid range
  - [x] 6.5 Mark frame as free in bitmap (bit = 0)
  - [x] 6.6 Increment free frame counter
  - [x] 6.7 Handle double-free gracefully (warn but don't crash)

- [x] **Task 7: Implement Statistics Functions** (AC: #6)
  - [x] 7.1 Implement pmm_get_free_count() - return free frame count
  - [x] 7.2 Implement pmm_get_total_count() - return total usable frames
  - [x] 7.3 Track statistics during init and alloc/free operations

- [x] **Task 8: Create mm Directory and Integrate** (AC: #1, #6)
  - [x] 8.1 Create `kernel/mm/` directory
  - [x] 8.2 Add pmm_init() call in kmain() after keyboard_init()
  - [x] 8.3 Add printk messages for PMM initialization status
  - [x] 8.4 Update Makefile/build.mk to include kernel/mm/*.c

- [x] **Task 9: Testing and Verification** (AC: #1-6)
  - [x] 9.1 Create `kernel/test/test_pmm.c` with verification tests
  - [x] 9.2 Test: pmm_alloc_frame() returns non-zero address
  - [x] 9.3 Test: pmm_alloc_frame() returns page-aligned address
  - [x] 9.4 Test: Multiple allocations return unique addresses
  - [x] 9.5 Test: pmm_free_frame() allows re-allocation
  - [x] 9.6 Test: Free count decreases on alloc, increases on free
  - [x] 9.7 Test: Addresses are above 1MB (>= 0x100000)
  - [x] 9.8 Add test_pmm() to test_runner.c

---

## Dev Notes

### What This Story Accomplishes

This is the **first story in Epic 3** - Memory Management. After this:
- Physical frame allocation works
- Bitmap tracks all physical memory
- Foundation for virtual memory (paging) is ready
- Story 3.2 (Paging) can build on this

### Memory Map from Bootloader

The bootloader (stage2.S) queries BIOS E820 and passes memory map to kernel:

```c
extern uint32_t boot_mmap_ptr;   /* Physical address of mmap array */
extern uint32_t boot_mmap_count; /* Number of entries */
```

**Memory Map Entry Structure (E820 format):**
```c
struct mmap_entry {
    uint64_t base;      /* Start address */
    uint64_t length;    /* Length in bytes */
    uint32_t type;      /* 1=available, 2=reserved, 3=ACPI, etc. */
    uint32_t acpi_ext;  /* ACPI 3.0 extended attributes (ignore) */
} __attribute__((packed));
```

**Type Values:**
| Type | Meaning |
|------|---------|
| 1 | Available (usable RAM) |
| 2 | Reserved (do not use) |
| 3 | ACPI reclaimable |
| 4 | ACPI NVS |
| 5 | Bad memory |

For this story, only type 1 (available) should be marked as free in bitmap.

### Physical Memory Layout

```
0x00000000 - 0x000FFFFF  (1MB)     Reserved (BIOS, VGA, ROM, etc.)
0x00100000 - KERNEL_END  (~X KB)   Kernel code, data, BSS
KERNEL_END - RAM_TOP               Available for allocation
```

**Key Addresses:**
- Kernel loads at physical 0x100000 (1MB)
- Kernel end is provided by linker symbol `kernel_end`
- Frames below 1MB (frames 0-255) are ALWAYS reserved
- First allocatable frame is typically frame 256 or higher

### Bitmap Allocator Design

**Key Design Decisions:**
- 1 bit per 4KB frame (compact representation)
- Bit = 1 means allocated, bit = 0 means free
- Bitmap stored in kernel BSS (static array)
- Maximum supported memory: 4GB (needs 128KB bitmap)

**Bitmap Size Calculation:**
```c
#define MAX_MEMORY     (4UL * 1024 * 1024 * 1024)  /* 4GB */
#define PAGE_SIZE      4096
#define MAX_FRAMES     (MAX_MEMORY / PAGE_SIZE)     /* 1M frames */
#define BITMAP_SIZE    (MAX_FRAMES / 8)             /* 128KB */

static uint8_t frame_bitmap[BITMAP_SIZE];
```

**Frame Number Conversion:**
```c
#define PHYS_TO_FRAME(addr)    ((addr) >> PAGE_SHIFT)
#define FRAME_TO_PHYS(frame)   ((frame) << PAGE_SHIFT)
#define PAGE_SHIFT             12
```

### Bitmap Operations Implementation

```c
/*
 * bitmap_set - Mark frame as allocated
 */
static void bitmap_set(uint32_t frame)
{
    frame_bitmap[frame / 8] |= (1 << (frame % 8));
}

/*
 * bitmap_clear - Mark frame as free
 */
static void bitmap_clear(uint32_t frame)
{
    frame_bitmap[frame / 8] &= ~(1 << (frame % 8));
}

/*
 * bitmap_test - Check if frame is allocated
 * Returns: 1 if allocated, 0 if free
 */
static int bitmap_test(uint32_t frame)
{
    return (frame_bitmap[frame / 8] >> (frame % 8)) & 1;
}
```

### PMM Initialization Flow

```c
void pmm_init(void)
{
    /* 1. Mark all frames as allocated initially */
    memset(frame_bitmap, 0xFF, sizeof(frame_bitmap));

    /* 2. Parse memory map from bootloader */
    struct mmap_entry *mmap = (struct mmap_entry *)boot_mmap_ptr;
    uint32_t count = boot_mmap_count;

    /* 3. For each available region, mark frames as free */
    for (uint32_t i = 0; i < count; i++) {
        if (mmap[i].type == MMAP_TYPE_AVAILABLE) {
            uint32_t start_frame = PHYS_TO_FRAME(mmap[i].base);
            uint32_t end_frame = PHYS_TO_FRAME(mmap[i].base + mmap[i].length);

            for (uint32_t f = start_frame; f < end_frame; f++) {
                bitmap_clear(f);
                free_frame_count++;
            }
        }
    }

    /* 4. Mark first 1MB as reserved (frames 0-255) */
    for (uint32_t f = 0; f < 256; f++) {
        if (!bitmap_test(f)) {
            bitmap_set(f);
            free_frame_count--;
        }
    }

    /* 5. Mark kernel frames as allocated */
    uint32_t kernel_start = PHYS_TO_FRAME(KERNEL_PHYS_START);
    uint32_t kernel_end = PHYS_TO_FRAME((uint32_t)&_kernel_end);

    for (uint32_t f = kernel_start; f <= kernel_end; f++) {
        if (!bitmap_test(f)) {
            bitmap_set(f);
            free_frame_count--;
        }
    }

    printk(LOG_INFO, "PMM: %u frames free (%u MB available)\n",
           free_frame_count, (free_frame_count * PAGE_SIZE) / (1024 * 1024));
}
```

### Frame Allocation Implementation

```c
/*
 * pmm_alloc_frame - Allocate a physical page frame
 *
 * Scans the bitmap for the first free frame, marks it as allocated,
 * and returns its physical address.
 *
 * Returns: Physical address on success, 0 if no frames available.
 */
uint32_t pmm_alloc_frame(void)
{
    /* Start search after 1MB to skip reserved memory */
    for (uint32_t f = 256; f < total_frame_count; f++) {
        if (!bitmap_test(f)) {
            bitmap_set(f);
            free_frame_count--;
            return FRAME_TO_PHYS(f);
        }
    }
    return 0;  /* No free frames */
}
```

### Frame Deallocation Implementation

```c
/*
 * pmm_free_frame - Free a physical page frame
 *
 * Marks the frame as free in the bitmap. The frame can then be
 * allocated again by pmm_alloc_frame().
 */
void pmm_free_frame(uint32_t phys_addr)
{
    /* Validate page alignment */
    if (phys_addr & (PAGE_SIZE - 1)) {
        printk(LOG_WARN, "PMM: free_frame called with unaligned addr 0x%x\n",
               phys_addr);
        return;
    }

    uint32_t frame = PHYS_TO_FRAME(phys_addr);

    /* Validate frame range */
    if (frame >= total_frame_count) {
        printk(LOG_WARN, "PMM: free_frame called with invalid addr 0x%x\n",
               phys_addr);
        return;
    }

    /* Check for double-free */
    if (!bitmap_test(frame)) {
        printk(LOG_WARN, "PMM: double-free of frame %u (0x%x)\n",
               frame, phys_addr);
        return;
    }

    bitmap_clear(frame);
    free_frame_count++;
}
```

### Linker Symbol for Kernel End

The linker script (scripts/kernel.ld) should define kernel_end:

```ld
SECTIONS
{
    . = 0x100000;  /* Kernel loads at 1MB */

    /* ... sections ... */

    .bss : {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }

    /* Align to page boundary and export end symbol */
    . = ALIGN(4096);
    _kernel_end = .;
}
```

**In C code:**
```c
extern char _kernel_end;  /* Linker-provided symbol */
#define KERNEL_PHYS_START  0x100000
#define KERNEL_PHYS_END    ((uint32_t)&_kernel_end)
```

### Integration with main.c

```c
#include <pmm.h>

void kmain(void)
{
    gdt_init();
    vga_init();
    serial_init();
    idt_init();
    pic_init();
    timer_init();
    keyboard_init();
    pmm_init();           /* NEW - Story 3.1 */

    sti();
    printk(LOG_INFO, "Interrupts enabled\n");

    /* ... */
}
```

### File Locations

| File | Purpose |
|------|---------|
| `kernel/include/mmap.h` | Memory map entry structure and types |
| `kernel/include/pmm.h` | PMM constants, macros, and function declarations |
| `kernel/mm/pmm.c` | Physical memory manager implementation |
| `kernel/test/test_pmm.c` | PMM verification tests |

### Previous Story Intelligence (Epic 2)

**From Story 2.3:**
- Test pattern: test_<subsystem>() functions in kernel/test/
- Use static for internal functions
- printk(LOG_INFO, ...) for initialization messages
- Wildcard in Makefile handles new .c files automatically

**Code patterns established:**
- Header guards: KERNEL_INCLUDE_<NAME>_H
- Include types.h for uint32_t, etc.
- Use asm.h for cli()/sti() if needed
- Extern declarations for linker symbols

**Git commit pattern:**
```
feat[story 3.1]: physical memory manager.
```

### Common Pitfalls

1. **Not marking kernel as allocated** - Kernel memory must be reserved in bitmap
2. **Off-by-one in frame calculations** - Be careful with inclusive/exclusive ranges
3. **Forgetting to update counters** - free_frame_count must track allocations
4. **Wrong memory map parsing** - E820 entries can overlap or be out of order
5. **Not aligning kernel_end** - Should be page-aligned in linker script
6. **Using uint32_t for 64-bit addresses** - E820 base/length are 64-bit, but we only support low 4GB
7. **Bitmap size calculation** - Ensure static array is large enough for max memory
8. **First 1MB always reserved** - Even if E820 says it's available

### Critical Architecture Constraints

**From Architecture Document:**
- Bitmap allocator, not free list
- Interface: `pmm_alloc_frame()`, `pmm_free_frame()` (exact names)
- PAGE_SIZE = 4096
- Store bitmap in kernel BSS
- Linux-style negative errno for errors (though this PMM returns 0 on failure)

**Testing Standards:**
- In-kernel tests: `test_<subsystem>()` function
- Output format: `[PASS] test_name` or `[FAIL] test_name: reason`
- Test via `make test` target

### Testing Strategy

1. **Initialization test:** PMM initializes without crash
2. **Basic allocation test:** pmm_alloc_frame() returns non-zero
3. **Page alignment test:** Allocated address is 4KB-aligned
4. **Unique allocation test:** Multiple allocations return different addresses
5. **Free and realloc test:** Freed frame can be allocated again
6. **Counter test:** Free count changes correctly
7. **Above 1MB test:** All addresses are >= 0x100000

### Test Implementation

```c
/* test_pmm.c */
void test_pmm(void)
{
    /* Test 1: Initial free count > 0 */
    uint32_t initial_free = pmm_get_free_count();
    TEST_ASSERT(initial_free > 0, "pmm_has_free_frames");

    /* Test 2: Allocate returns valid address */
    uint32_t addr1 = pmm_alloc_frame();
    TEST_ASSERT(addr1 != 0, "pmm_alloc_returns_nonzero");
    TEST_ASSERT(addr1 >= 0x100000, "pmm_alloc_above_1mb");
    TEST_ASSERT((addr1 & 0xFFF) == 0, "pmm_alloc_page_aligned");

    /* Test 3: Second allocation is different */
    uint32_t addr2 = pmm_alloc_frame();
    TEST_ASSERT(addr2 != addr1, "pmm_alloc_unique");

    /* Test 4: Free count decreased */
    uint32_t after_alloc = pmm_get_free_count();
    TEST_ASSERT(after_alloc == initial_free - 2, "pmm_alloc_decrements_free");

    /* Test 5: Free frame, count increases */
    pmm_free_frame(addr1);
    uint32_t after_free = pmm_get_free_count();
    TEST_ASSERT(after_free == initial_free - 1, "pmm_free_increments_free");

    /* Test 6: Can reallocate freed frame */
    uint32_t addr3 = pmm_alloc_frame();
    TEST_ASSERT(addr3 != 0, "pmm_realloc_after_free");

    /* Cleanup */
    pmm_free_frame(addr2);
    pmm_free_frame(addr3);

    printk(LOG_INFO, "[PASS] test_pmm\n");
}
```

### Relationship to Other Stories

- **Depends on:** Story 1.1 (build system), Story 1.4 (kernel entry), Story 1.6 (printk)
- **Enables:** Story 3.2 (Paging & Virtual Memory), Story 3.4 (Kernel Heap)
- **Uses:** boot_mmap_ptr, boot_mmap_count from bootloader (Story 1.3)

### Project Structure Notes

**New Directory:** `kernel/mm/` (first memory management code)

**New Files:**
- `kernel/include/mmap.h`
- `kernel/include/pmm.h`
- `kernel/mm/pmm.c`
- `kernel/test/test_pmm.c`

**Modified Files:**
- `kernel/init/main.c` - Add pmm_init() call
- `kernel/test/test_runner.c` - Add test_pmm() call
- `scripts/kernel.ld` - Ensure _kernel_end symbol exported

### References

- [Source: _bmad-output/planning-artifacts/architecture.md#Memory-Architecture]
- [Source: _bmad-output/planning-artifacts/architecture.md#Physical-Memory-Manager]
- [Source: _bmad-output/planning-artifacts/epics.md#Story-3.1]
- [Source: _bmad-output/project-context.md#Memory-Rules]
- [Source: _bmad-output/project-context.md#Critical-C-Rules]
- [Source: kernel/init/main.c#boot_mmap_ptr]
- [Source: OSDev Wiki - Memory Map (x86)]
- [Source: OSDev Wiki - Page Frame Allocation]

---

## Dev Agent Record

### Agent Model Used

Claude Opus 4.5 (claude-opus-4-5-20251101)

### Debug Log References

- Fixed MAX_MEMORY macro to use `4ULL` instead of `4UL` to prevent 32-bit overflow
- Fixed printk format specifiers from `%08x` to `%x` (width specifiers not supported)

### Completion Notes List

- Implemented bitmap-based physical memory manager (PMM)
- PMM parses E820 memory map from bootloader
- Bitmap tracks 1 bit per 4KB frame (supports up to 4GB)
- First 1MB always reserved (BIOS, VGA, ROM)
- Kernel frames marked as allocated
- pmm_alloc_frame() returns first free frame above 1MB
- pmm_free_frame() validates alignment, range, and double-free
- All 11 PMM tests pass; 382 total tests pass with no regressions
- Added memset to kernel/lib for bitmap initialization
- Refactored bitmap ops to kernel/lib/bitmap.c for reuse and host testing
- Host tests now test actual bitmap.c code (9 tests pass)

### Code Review Fixes (2026-02-04)

- Added test for pmm_get_total_count() (M3)
- Added test for reserved frame protection in pmm_free_frame() (M4)
- Added test for exhaustion handling (AC5, M2)
- Added tests/host/test_string.c to File List documentation (M1)

### File List

**New Files:**
- kernel/include/mmap.h - E820 memory map structure and type constants
- kernel/include/pmm.h - PMM interface (PAGE_SIZE, macros, function declarations)
- kernel/include/bitmap.h - Generic bitmap operations interface
- kernel/include/string.h - memset declaration
- kernel/mm/pmm.c - Physical memory manager implementation
- kernel/lib/bitmap.c - Bitmap set/clear/test operations (shared)
- kernel/lib/string.c - memset implementation
- kernel/test/test_pmm.c - PMM unit tests (8 tests)

**Modified Files:**
- kernel/init/main.c - Added pmm_init() call, pmm.h include
- kernel/test/test_runner.c - Added test_pmm() declaration and call
- Makefile - Added kernel/mm/*.c to sources, mm build directory
- tests/host/test_bitmap.c - Updated to test actual kernel/lib/bitmap.c (9 tests)
- tests/host/test_string.c - Host tests for kernel memset implementation (9 tests)
