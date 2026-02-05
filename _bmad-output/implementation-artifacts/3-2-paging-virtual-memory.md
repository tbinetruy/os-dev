# Story 3.2: Paging & Virtual Memory

Status: done

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a developer,
I want paging enabled with a higher-half kernel mapping,
so that I understand virtual memory and have address space isolation foundation.

## Acceptance Criteria

1. **AC1: Page Directory Creation**
   - Given PMM is initialized
   - When vmm_init() is called
   - Then kernel page directory is created
   - And kernel is mapped at 0xC0000000+ (higher-half)
   - And identity mapping exists for low memory during transition

2. **AC2: Paging Enablement**
   - Given page directory is set up
   - When paging is enabled (CR0.PG = 1)
   - Then kernel continues executing at higher-half addresses
   - And identity mapping can be removed after transition

3. **AC3: Page Mapping**
   - Given paging is enabled
   - When vmm_map_page(virt, phys, flags) is called
   - Then page table entry is created/updated
   - And virtual address maps to specified physical address
   - And flags (present, writable, user) are set correctly

4. **AC4: Page Unmapping**
   - Given a mapping exists
   - When vmm_unmap_page(virt) is called
   - Then page table entry is cleared
   - And TLB is invalidated for that address (invlpg)

5. **AC5: VMM Code Structure**
   - Given kernel/mm/vmm.c source
   - When I examine the code
   - Then page directory entries point to page tables
   - And KERNEL_BASE is defined as 0xC0000000
   - And P2V/V2P macros convert between physical and virtual

6. **AC6: Page Table Entry Format**
   - Given kernel/mm/page.c source
   - When I examine the code
   - Then page table entry format matches Intel SDM
   - And PAGE_PRESENT, PAGE_WRITABLE, PAGE_USER flags defined

## Tasks / Subtasks

- [x] **Task 1: Create VMM Header** (AC: #3, #5)
  - [x] 1.1 Create `kernel/include/vmm.h`
  - [x] 1.2 Define KERNEL_BASE (0xC0000000), KERNEL_PAGE_DIR_IDX (768)
  - [x] 1.3 Define P2V(phys) macro: (phys) + KERNEL_BASE
  - [x] 1.4 Define V2P(virt) macro: (virt) - KERNEL_BASE
  - [x] 1.5 Declare vmm_init(), vmm_map_page(), vmm_unmap_page()
  - [x] 1.6 Declare vmm_get_physaddr() for virtual-to-physical lookup
  - [x] 1.7 Add PAGE_SIZE, PAGE_MASK (0xFFFFF000) definitions

- [x] **Task 2: Create Page Table Header** (AC: #6)
  - [x] 2.1 Create `kernel/include/page.h`
  - [x] 2.2 Define PAGE_PRESENT (0x001), PAGE_WRITABLE (0x002), PAGE_USER (0x004)
  - [x] 2.3 Define PAGE_ACCESSED (0x020), PAGE_DIRTY (0x040)
  - [x] 2.4 Define PAGE_FRAME_MASK (0xFFFFF000) to extract frame address
  - [x] 2.5 Define PDE_INDEX(addr), PTE_INDEX(addr) macros
  - [x] 2.6 Document i386 page table entry format in comments

- [x] **Task 3: Implement Boot Page Tables (Assembly)** (AC: #1, #2)
  - [x] 3.1 Modify `kernel/init/entry.S` for early paging setup
  - [x] 3.2 Allocate boot page directory (4KB aligned) in BSS
  - [x] 3.3 Allocate boot page tables for identity map (0-16MB)
  - [x] 3.4 Allocate boot page tables for higher-half (0xC0000000-0xC0FFFFFF)
  - [x] 3.5 Fill page tables: identity map first 16MB
  - [x] 3.6 Fill page tables: map first 16MB at 0xC0000000
  - [x] 3.7 Load page directory address into CR3
  - [x] 3.8 Enable paging by setting CR0.PG bit
  - [x] 3.9 Jump to higher-half kernel code
  - [x] 3.10 Remove identity mapping after jump

- [x] **Task 4: Implement VMM Core** (AC: #1, #3, #4, #5)
  - [x] 4.1 Create `kernel/mm/vmm.c`
  - [x] 4.2 Declare kernel_page_directory as extern (from entry.S)
  - [x] 4.3 Implement vmm_init() - validate paging is enabled, setup state
  - [x] 4.4 Implement get_or_create_page_table() - get/create page table for address
  - [x] 4.5 Implement vmm_map_page(virt, phys, flags) - map single page
  - [x] 4.6 Implement vmm_unmap_page(virt) - unmap single page
  - [x] 4.7 Implement vmm_get_physaddr(virt) - translate virtual to physical
  - [x] 4.8 Add TLB invalidation using invlpg instruction

- [x] **Task 5: Implement Page Operations** (AC: #6)
  - [x] 5.1 Page operations included in vmm.c (simple enough)
  - [x] 5.2 Helper to allocate page-aligned memory via PMM in get_or_create_page_table()
  - [x] 5.3 Page zeroing uses memset in get_or_create_page_table()
  - [x] 5.4 Page table entry bit layout documented in page.h

- [x] **Task 6: Update Assembly Helpers** (AC: #2, #4)
  - [x] 6.1 Add invlpg(addr) inline assembly in `kernel/include/asm.h`
  - [x] 6.2 Add read_cr0(), write_cr0()
  - [x] 6.3 Add read_cr3(), write_cr3() for page directory manipulation
  - [x] 6.4 Add read_cr2() for page fault address (prepare for story 3.3)

- [x] **Task 7: Update Kernel Linker Script** (AC: #1, #2)
  - [x] 7.1 Modify `scripts/kernel.ld` for higher-half kernel
  - [x] 7.2 Set kernel virtual base address to 0xC0100000
  - [x] 7.3 Set kernel physical load address to 0x00100000 (1MB)
  - [x] 7.4 Use AT() directive for physical placement
  - [x] 7.5 Export _kernel_virt_start, _kernel_virt_end symbols

- [x] **Task 8: Integrate with main.c** (AC: #1, #2)
  - [x] 8.1 Call vmm_init() in kmain() after pmm_init()
  - [x] 8.2 Verify kernel is running at 0xC0XXXXXX addresses
  - [x] 8.3 Add printk messages showing virtual addresses
  - [x] 8.4 Update physical address usage to use P2V/V2P

- [x] **Task 9: Testing and Verification** (AC: #1-6)
  - [x] 9.1 Create `kernel/test/test_vmm.c`
  - [x] 9.2 Test: vmm_map_page() maps address correctly
  - [x] 9.3 Test: vmm_get_physaddr() returns correct physical address
  - [x] 9.4 Test: vmm_unmap_page() makes address invalid
  - [x] 9.5 Test: Kernel code runs at higher-half addresses
  - [x] 9.6 Test: VGA memory (0xB8000) still accessible via P2V mapping
  - [x] 9.7 Add test_vmm() to test_runner.c

---

## Dev Notes

### What This Story Accomplishes

This is **Story 3.2 in Epic 3** (Memory Management). After this:
- Paging is enabled with higher-half kernel
- Virtual memory infrastructure is ready
- Address space isolation foundation exists
- Story 3.3 (Page Fault Handler) can add fault handling
- Story 3.4 (Kernel Heap) can use vmm_map_page() for heap growth

### Higher-Half Kernel Concept

**Why Higher-Half?**
- Kernel occupies upper 1GB (0xC0000000 - 0xFFFFFFFF)
- User processes get lower 3GB (0x00000000 - 0xBFFFFFFF)
- Kernel mappings are shared across all processes
- Simple address calculation: virtual = physical + 0xC0000000

**Memory Layout After Paging:**
```
0xFFFFFFFF ┌──────────────────┐
           │   Kernel Heap    │  (grows down, story 3.4)
           ├──────────────────┤
           │  Kernel Stack    │
           ├──────────────────┤
           │   Kernel BSS     │
           ├──────────────────┤
           │   Kernel Data    │
           ├──────────────────┤
           │   Kernel Code    │
0xC0100000 ├──────────────────┤
           │   Reserved       │  (first 1MB virtual)
0xC0000000 ├──────────────────┤  KERNEL_BASE
           │                  │
           │                  │
           │   User Space     │  (future - stories 4+5)
           │   (unmapped)     │
           │                  │
0x00000000 └──────────────────┘
```

### i386 Paging Architecture

**Two-Level Paging:**
- Page Directory: 1024 entries × 4 bytes = 4KB
- Page Table: 1024 entries × 4 bytes = 4KB
- Each page: 4KB
- Total addressable: 1024 × 1024 × 4KB = 4GB

**Virtual Address Breakdown (32-bit):**
```
31        22 21        12 11          0
┌──────────┬────────────┬─────────────┐
│ PD Index │  PT Index  │   Offset    │
│ (10 bits)│ (10 bits)  │  (12 bits)  │
└──────────┴────────────┴─────────────┘
```

**Page Directory Entry / Page Table Entry Format:**
```
31                      12 11  9 8 7 6 5 4 3 2 1 0
┌─────────────────────────┬────┬─┬─┬─┬─┬─┬─┬─┬─┬─┐
│   Frame Address (20b)   │AVL │G│0│D│A│C│W│U│W│P│
└─────────────────────────┴────┴─┴─┴─┴─┴─┴─┴─┴─┴─┘
P  = Present (1 = valid entry)
W  = Writable (1 = read/write, 0 = read-only)
U  = User (1 = user accessible, 0 = kernel only)
W  = Write-through
C  = Cache disable
A  = Accessed (set by CPU on access)
D  = Dirty (set by CPU on write, PTE only)
G  = Global (TLB not flushed on CR3 load)
```

### Index Calculation Macros

```c
/* Extract page directory index (bits 22-31) */
#define PDE_INDEX(addr)  (((uint32_t)(addr) >> 22) & 0x3FF)

/* Extract page table index (bits 12-21) */
#define PTE_INDEX(addr)  (((uint32_t)(addr) >> 12) & 0x3FF)

/* Extract page offset (bits 0-11) */
#define PAGE_OFFSET(addr) ((uint32_t)(addr) & 0xFFF)

/* KERNEL_BASE = 0xC0000000, so PDE_INDEX(KERNEL_BASE) = 768 */
#define KERNEL_PAGE_DIR_IDX  768
```

### Boot Paging Setup (entry.S)

The tricky part is that we need paging enabled BEFORE jumping to C code at higher-half addresses. This requires assembly setup.

**Strategy:**
1. Bootloader loads kernel at physical 0x100000
2. entry.S sets up page directory and page tables
3. Identity maps first 4MB (so current code keeps working)
4. Also maps first 4MB at 0xC0000000 (for higher-half)
5. Enable paging
6. Jump to higher-half address of kmain

**Boot Page Tables Layout:**
```
boot_page_directory (4KB, page-aligned):
  Entry 0:   → boot_page_table_0 (identity map 0-4MB)
  Entry 768: → boot_page_table_0 (same table! maps 0xC0000000-0xC0400000)
  Entry 769-1023: not present

boot_page_table_0 (4KB, page-aligned):
  Entry 0:   0x00000000 | PAGE_PRESENT | PAGE_WRITABLE
  Entry 1:   0x00001000 | PAGE_PRESENT | PAGE_WRITABLE
  ...
  Entry 1023: 0x003FF000 | PAGE_PRESENT | PAGE_WRITABLE
```

**Assembly Snippet (entry.S):**
```asm
.section .bss
.align 4096
boot_page_directory:
    .skip 4096
boot_page_table_0:
    .skip 4096

.section .text
_start:
    /* Set up boot page table - identity map first 4MB */
    movl $boot_page_table_0, %edi
    movl $0, %esi                    /* Physical address starts at 0 */
    movl $1024, %ecx                 /* 1024 entries */
1:
    movl %esi, %eax
    orl $(PAGE_PRESENT | PAGE_WRITABLE), %eax
    movl %eax, (%edi)
    addl $4096, %esi                 /* Next physical page */
    addl $4, %edi                    /* Next PTE */
    loop 1b

    /* Set up page directory */
    movl $boot_page_directory, %edi

    /* Entry 0: Identity map (PDE index 0) */
    movl $boot_page_table_0, %eax
    orl $(PAGE_PRESENT | PAGE_WRITABLE), %eax
    movl %eax, (%edi)

    /* Entry 768: Higher-half map (PDE index 768 = 0xC0000000) */
    movl %eax, 768*4(%edi)

    /* Load page directory into CR3 */
    movl $boot_page_directory, %eax
    movl %eax, %cr3

    /* Enable paging (set CR0.PG) */
    movl %cr0, %eax
    orl $0x80000000, %eax
    movl %eax, %cr0

    /* Jump to higher-half kernel */
    lea higher_half, %eax
    jmp *%eax

higher_half:
    /* Now running at 0xC0XXXXXX addresses */
    /* Update stack pointer to higher-half */
    addl $KERNEL_BASE, %esp

    /* Call kmain */
    call kmain
```

### Linker Script Modifications

**Current kernel.ld (physical addresses):**
```ld
ENTRY(_start)
SECTIONS {
    . = 0x100000;    /* Kernel loads at 1MB */
    /* ... */
}
```

**Higher-Half kernel.ld:**
```ld
ENTRY(_start)

KERNEL_PHYS_BASE = 0x00100000;
KERNEL_VIRT_BASE = 0xC0100000;

SECTIONS {
    . = KERNEL_VIRT_BASE;

    _kernel_virt_start = .;

    .text : AT(KERNEL_PHYS_BASE) {
        *(.multiboot)
        *(.text)
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(COMMON)
        *(.bss)
    }

    . = ALIGN(4096);
    _kernel_virt_end = .;
    _kernel_phys_end = . - KERNEL_VIRT_BASE + KERNEL_PHYS_BASE;
}
```

### VMM Implementation

**vmm_init():**
```c
/* Global kernel page directory */
uint32_t *kernel_page_directory;

void vmm_init(void)
{
    /* Page directory was set up in entry.S, get its virtual address */
    extern uint32_t boot_page_directory[];
    kernel_page_directory = (uint32_t *)P2V((uint32_t)boot_page_directory);

    /* Verify paging is enabled */
    uint32_t cr0 = read_cr0();
    if (!(cr0 & 0x80000000)) {
        panic("vmm_init: paging not enabled");
    }

    printk(LOG_INFO, "VMM: Paging enabled, kernel at 0x%x\n",
           (uint32_t)&vmm_init);
}
```

**vmm_map_page():**
```c
/*
 * vmm_map_page - Map a virtual address to a physical address
 *
 * Creates or updates the page table entry for the given virtual address.
 * Allocates a new page table if necessary.
 *
 * Returns 0 on success, -ENOMEM if page table allocation fails.
 */
int vmm_map_page(uint32_t virt, uint32_t phys, uint32_t flags)
{
    uint32_t pde_idx = PDE_INDEX(virt);
    uint32_t pte_idx = PTE_INDEX(virt);

    /* Get or create page table */
    if (!(kernel_page_directory[pde_idx] & PAGE_PRESENT)) {
        /* Allocate new page table */
        uint32_t pt_phys = pmm_alloc_frame();
        if (pt_phys == 0) {
            return -ENOMEM;
        }

        /* Zero the new page table */
        memset((void *)P2V(pt_phys), 0, PAGE_SIZE);

        /* Add to page directory */
        kernel_page_directory[pde_idx] = pt_phys | PAGE_PRESENT | PAGE_WRITABLE;
    }

    /* Get page table virtual address */
    uint32_t pt_phys = kernel_page_directory[pde_idx] & PAGE_FRAME_MASK;
    uint32_t *page_table = (uint32_t *)P2V(pt_phys);

    /* Set page table entry */
    page_table[pte_idx] = (phys & PAGE_FRAME_MASK) | flags | PAGE_PRESENT;

    /* Invalidate TLB for this address */
    invlpg(virt);

    return 0;
}
```

**vmm_unmap_page():**
```c
/*
 * vmm_unmap_page - Unmap a virtual address
 *
 * Clears the page table entry and invalidates the TLB.
 * Does not free the physical frame (caller's responsibility).
 */
void vmm_unmap_page(uint32_t virt)
{
    uint32_t pde_idx = PDE_INDEX(virt);
    uint32_t pte_idx = PTE_INDEX(virt);

    /* Check if page table exists */
    if (!(kernel_page_directory[pde_idx] & PAGE_PRESENT)) {
        return;  /* Already unmapped */
    }

    /* Get page table */
    uint32_t pt_phys = kernel_page_directory[pde_idx] & PAGE_FRAME_MASK;
    uint32_t *page_table = (uint32_t *)P2V(pt_phys);

    /* Clear entry */
    page_table[pte_idx] = 0;

    /* Invalidate TLB */
    invlpg(virt);
}
```

### Inline Assembly Helpers (asm.h)

```c
/* Invalidate TLB entry for a specific address */
static inline void invlpg(uint32_t addr)
{
    __asm__ __volatile__("invlpg (%0)" : : "r"(addr) : "memory");
}

/* Read CR0 register */
static inline uint32_t read_cr0(void)
{
    uint32_t val;
    __asm__ __volatile__("mov %%cr0, %0" : "=r"(val));
    return val;
}

/* Write CR0 register */
static inline void write_cr0(uint32_t val)
{
    __asm__ __volatile__("mov %0, %%cr0" : : "r"(val));
}

/* Read CR3 register (page directory physical address) */
static inline uint32_t read_cr3(void)
{
    uint32_t val;
    __asm__ __volatile__("mov %%cr3, %0" : "=r"(val));
    return val;
}

/* Write CR3 register (also flushes TLB) */
static inline void write_cr3(uint32_t val)
{
    __asm__ __volatile__("mov %0, %%cr3" : : "r"(val) : "memory");
}

/* Read CR2 register (page fault address) */
static inline uint32_t read_cr2(void)
{
    uint32_t val;
    __asm__ __volatile__("mov %%cr2, %0" : "=r"(val));
    return val;
}
```

### P2V/V2P Macro Usage

**Important:** After paging is enabled, physical addresses cannot be accessed directly. Must use P2V() to convert to virtual addresses.

```c
/* Before paging (in entry.S): addresses are physical */
uint32_t *ptr = (uint32_t *)0x100000;  /* OK - physical */

/* After paging: must use virtual addresses */
uint32_t *ptr = (uint32_t *)P2V(0x100000);  /* Correct */
uint32_t *ptr = (uint32_t *)0xC0100000;     /* Also correct */

/* Converting back for hardware operations */
uint32_t phys = V2P((uint32_t)ptr);  /* Get physical for PMM, etc. */
```

### VGA Memory Mapping

VGA text buffer is at physical 0xB8000. This is below 1MB, so it's part of the identity-mapped region. However, after removing identity mapping, we need to access it via higher-half.

**Options:**
1. **Keep identity map** for I/O region (0x0-0x100000) - simplest
2. **Map VGA separately** at 0xC00B8000 - cleaner but more work

For this story, recommend keeping VGA at identity-mapped address or updating vga.c to use P2V(0xB8000).

**In vga.c:**
```c
/* Option 1: Use P2V macro (requires higher-half map of low memory) */
#define VGA_MEMORY  ((uint16_t *)P2V(0xB8000))

/* Option 2: Map VGA to specific virtual address */
#define VGA_MEMORY  ((uint16_t *)0xC00B8000)
vmm_map_page(0xC00B8000, 0xB8000, PAGE_PRESENT | PAGE_WRITABLE);
```

### Previous Story Intelligence (Story 3.1)

**From Story 3.1 (PMM):**
- PMM provides `pmm_alloc_frame()` returning physical addresses
- PMM provides `pmm_free_frame(phys)` for deallocation
- `memset()` available in kernel/lib/string.c
- Test pattern: `test_<subsystem>()` in kernel/test/
- Header guards: `KERNEL_INCLUDE_<NAME>_H` or `KERNEL_MM_<NAME>_H`

**Key learnings from previous stories:**
- Use printk(LOG_INFO, ...) for initialization messages
- Use printk(LOG_WARN, ...) for warnings
- Static for internal functions
- Extern for linker symbols

### Git Intelligence

**Recent commit pattern:**
```
feat[story 3.1]: physical memory manager.
```

**Files typically modified in mm/ stories:**
- kernel/include/*.h - New headers
- kernel/mm/*.c - Implementation
- kernel/init/main.c - Integration
- kernel/test/test_*.c - Tests
- kernel/test/test_runner.c - Test registration
- scripts/kernel.ld - Linker script changes

### Critical Constraints

**From Architecture Document:**
- Higher-half kernel at 0xC0000000
- PAGE_SIZE = 4096
- Interface: vmm_map_page(), vmm_unmap_page()
- P2V/V2P macros for address conversion
- PAGE_PRESENT, PAGE_WRITABLE, PAGE_USER flags

**From Project Context:**
- No floating point in kernel
- Use volatile for hardware registers
- Validate all pointers
- Save/restore interrupt state around critical sections

### Testing Strategy

1. **Paging enabled test:** CR0.PG bit is set
2. **Higher-half test:** Kernel code address > 0xC0000000
3. **Map page test:** vmm_map_page() allows access to mapped address
4. **Physical translation test:** vmm_get_physaddr() returns correct address
5. **Unmap test:** Access to unmapped address fails (prepare for story 3.3)
6. **VGA still works:** Can print after paging enabled

### Test Implementation

```c
/* test_vmm.c */
void test_vmm(void)
{
    /* Test 1: Paging is enabled */
    uint32_t cr0 = read_cr0();
    TEST_ASSERT((cr0 & 0x80000000) != 0, "paging_enabled");

    /* Test 2: Kernel running at higher-half */
    uint32_t code_addr = (uint32_t)&test_vmm;
    TEST_ASSERT(code_addr >= KERNEL_BASE, "kernel_higher_half");

    /* Test 3: Map a new page */
    uint32_t test_virt = 0xD0000000;  /* Unmapped region */
    uint32_t test_phys = pmm_alloc_frame();
    TEST_ASSERT(test_phys != 0, "pmm_alloc_for_test");

    int ret = vmm_map_page(test_virt, test_phys, PAGE_PRESENT | PAGE_WRITABLE);
    TEST_ASSERT(ret == 0, "vmm_map_page_success");

    /* Test 4: Can write to mapped page */
    uint32_t *ptr = (uint32_t *)test_virt;
    *ptr = 0xDEADBEEF;
    TEST_ASSERT(*ptr == 0xDEADBEEF, "vmm_write_read");

    /* Test 5: Physical address translation */
    uint32_t phys = vmm_get_physaddr(test_virt);
    TEST_ASSERT(phys == test_phys, "vmm_get_physaddr");

    /* Test 6: Unmap page */
    vmm_unmap_page(test_virt);
    /* Note: Accessing test_virt now would cause page fault */

    /* Cleanup */
    pmm_free_frame(test_phys);

    printk(LOG_INFO, "[PASS] test_vmm\n");
}
```

### Common Pitfalls

1. **Forgetting identity map** - Need identity map during transition or crash
2. **Wrong CR3 value** - CR3 takes PHYSICAL address of page directory
3. **Not invalidating TLB** - Changes not visible without invlpg
4. **P2V before paging enabled** - Can't use P2V macro before paging
5. **Stack pointer not updated** - Stack must move to higher-half
6. **VGA breakage** - VGA at 0xB8000 may need remapping
7. **Bootloader compatibility** - stage2 must load kernel correctly
8. **Page table alignment** - PD and PT must be 4KB aligned
9. **Self-mapping page directory** - Optional but useful technique
10. **Recursive mapping** - Alternative to P2V, not used here

### File Locations

| File | Purpose |
|------|---------|
| `kernel/include/vmm.h` | VMM interface (KERNEL_BASE, P2V/V2P, function declarations) |
| `kernel/include/page.h` | Page table flags and macros (PAGE_PRESENT, PDE_INDEX, etc.) |
| `kernel/include/asm.h` | CR register access, invlpg (add to existing) |
| `kernel/mm/vmm.c` | Virtual memory manager implementation |
| `kernel/init/entry.S` | Boot paging setup (modify existing) |
| `scripts/kernel.ld` | Higher-half linker script (modify existing) |
| `kernel/test/test_vmm.c` | VMM unit tests |

### Relationship to Other Stories

- **Depends on:** Story 3.1 (PMM for page table allocation)
- **Enables:** Story 3.3 (Page Fault Handler), Story 3.4 (Kernel Heap), Epic 5 (User Address Spaces)
- **Affects:** All existing code (addresses change to higher-half)

### Project Structure Notes

**Modified Files:**
- `kernel/init/entry.S` - Add boot paging setup
- `scripts/kernel.ld` - Convert to higher-half
- `kernel/include/asm.h` - Add CR register functions
- `kernel/init/main.c` - Add vmm_init() call
- `kernel/drivers/vga.c` - May need P2V for VGA memory
- `kernel/test/test_runner.c` - Add test_vmm()

**New Files:**
- `kernel/include/vmm.h`
- `kernel/include/page.h`
- `kernel/mm/vmm.c`
- `kernel/test/test_vmm.c`

### References

- [Source: _bmad-output/planning-artifacts/architecture.md#Memory-Architecture]
- [Source: _bmad-output/planning-artifacts/architecture.md#Project-Structure]
- [Source: _bmad-output/planning-artifacts/epics.md#Story-3.2]
- [Source: _bmad-output/project-context.md#Memory-Rules]
- [Source: _bmad-output/project-context.md#Critical-Assembly-Rules]
- [Source: Intel SDM Vol 3 Chapter 4 - Paging]
- [Source: OSDev Wiki - Paging]
- [Source: OSDev Wiki - Higher Half Kernel]

---

## Dev Agent Record

### Agent Model Used

Claude Opus 4.5 (claude-opus-4-5-20251101)

### Debug Log References

None required - all tests pass.

### Completion Notes List

1. **Extended boot page tables from 4MB to 16MB mapping** - The original 4MB mapping was insufficient for VMM to allocate page tables and access them via P2V(). Extended to 4 page tables (16MB) to allow PMM-allocated frames to be accessible.

2. **VGA driver updated to use P2V()** - `vga.c` updated to access VGA buffer at `P2V(0xB8000)` instead of raw physical address.

3. **PMM updated to use P2V() for memory map** - `pmm.c` now accesses `boot_mmap_ptr` via `P2V()` since the memory map is stored at physical address 0x504.

4. **KERNEL_PHYS_END fixed** - The `_kernel_end` symbol is now a virtual address; updated pmm.h to subtract `KERNEL_VIRT_BASE_CONST` to get the physical address.

5. **Test files updated for higher-half** - `test_boot.c` and `test_vga.c` updated to use P2V() for physical address access and verify kernel is at 0xC0100000.

6. **PAGE_MASK conflict resolved** - Both vmm.h and pmm.h defined PAGE_MASK; added `#ifndef PAGE_MASK` guards to prevent redefinition.

7. **All 394 tests pass** - Full test suite validates boot, GDT, VGA, serial, IDT, PIC, timer, printk, keyboard, PMM, and VMM functionality.

### File List

**New Files:**
- `kernel/include/vmm.h` - VMM interface (KERNEL_BASE, P2V/V2P macros, function declarations)
- `kernel/include/page.h` - Page table entry flags and address decomposition macros
- `kernel/include/errno.h` - Shared kernel error numbers (ENOMEM, EINVAL, EFAULT)
- `kernel/mm/vmm.c` - Virtual memory manager implementation
- `kernel/test/test_vmm.c` - VMM unit tests (11 tests)

**Modified Files:**
- `kernel/init/entry.S` - Complete rewrite for higher-half paging setup with 16MB mapping
- `scripts/kernel.ld` - Higher-half linker script with virtual addresses and AT() directives
- `kernel/include/asm.h` - Added CR register functions and invlpg
- `kernel/init/main.c` - Added vmm_init() call after pmm_init()
- `kernel/drivers/vga.c` - VGA buffer access via P2V(0xB8000)
- `kernel/mm/pmm.c` - Memory map access via P2V(boot_mmap_ptr)
- `kernel/include/pmm.h` - Fixed KERNEL_PHYS_END calculation, PAGE_MASK guard
- `kernel/test/test_boot.c` - Updated for higher-half kernel verification
- `kernel/test/test_vga.c` - Updated VGA buffer access to use P2V()
- `kernel/test/test_runner.c` - Added test_vmm() call

### Change Log

| File | Change Type | Description |
|------|-------------|-------------|
| `kernel/include/vmm.h` | Created | VMM interface with KERNEL_BASE, P2V/V2P macros |
| `kernel/include/page.h` | Created | Page table flags (PAGE_PRESENT, etc.) and index macros |
| `kernel/mm/vmm.c` | Created | VMM implementation: vmm_init, map_page, unmap_page, get_physaddr |
| `kernel/test/test_vmm.c` | Created | 11 VMM tests verifying paging, mapping, translation |
| `kernel/init/entry.S` | Modified | Higher-half boot with 16MB paging, identity map removal |
| `scripts/kernel.ld` | Modified | Virtual addresses at 0xC0100000, AT() for physical placement |
| `kernel/include/asm.h` | Modified | Added read_cr0/2/3, write_cr0/3, invlpg inline functions |
| `kernel/init/main.c` | Modified | Added vmm_init() call, #include <vmm.h> |
| `kernel/drivers/vga.c` | Modified | VGA_BUFFER now uses P2V(0xB8000) |
| `kernel/mm/pmm.c` | Modified | Memory map access via P2V(boot_mmap_ptr) |
| `kernel/include/pmm.h` | Modified | Fixed KERNEL_PHYS_END, added PAGE_MASK guard |
| `kernel/test/test_boot.c` | Modified | P2V for A20 test, check kernel at 0xC0100000 |
| `kernel/test/test_vga.c` | Modified | TEST_VGA_BUFFER uses P2V(0xB8000) |
| `kernel/test/test_runner.c` | Modified | Added extern and call to test_vmm() |
| `kernel/include/errno.h` | Created | Shared kernel error numbers (ENOMEM, EINVAL, EFAULT) |
