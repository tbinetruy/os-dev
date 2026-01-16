# Story 1.4: Kernel Entry & GDT Setup

Status: done

## Story

As a developer,
I want the kernel to initialize properly with a correct GDT,
so that I have proper segment descriptors for kernel and future user mode.

## Acceptance Criteria

1. **AC1: Kernel Runs at Linked Address**
   - Given the bootloader jumps to kernel entry
   - When kmain() begins executing
   - Then the kernel runs at 0x100000 physical (identity-mapped; virtual mapping to 0xC0100000 occurs after paging in Milestone 2)

2. **AC2: GDT Creation**
   - Given kernel is initializing
   - When GDT setup is called
   - Then a GDT is created with: null descriptor, kernel code (ring 0), kernel data (ring 0)
   - And placeholder entries exist for user code/data (ring 3) and TSS

3. **AC3: GDT Loading**
   - Given GDT is loaded
   - When segment registers are reloaded
   - Then CS points to kernel code segment
   - And DS, ES, SS point to kernel data segment

4. **AC4: Code Documentation**
   - Given kernel/init/gdt.c source
   - When I examine the code
   - Then GDT structure matches Intel SDM format
   - And comments explain each descriptor field

## Tasks / Subtasks

- [x] **Task 1: Create GDT Header File** (AC: #2, #4)
  - [x] 1.1 Create `kernel/include/gdt.h` with segment selector constants
  - [x] 1.2 Define GDT entry structure (8 bytes per descriptor)
  - [x] 1.3 Define GDT pointer structure (6 bytes: limit + base)
  - [x] 1.4 Define segment selector macros: KERNEL_CS (0x08), KERNEL_DS (0x10), USER_CS (0x1B), USER_DS (0x23), TSS_SEG (0x28)
  - [x] 1.5 Declare gdt_init() function prototype

- [x] **Task 2: Implement GDT in C** (AC: #2, #4)
  - [x] 2.1 Create `kernel/init/gdt.c`
  - [x] 2.2 Define static GDT array with 6 entries (null, kcode, kdata, ucode, udata, tss)
  - [x] 2.3 Implement gdt_set_gate() helper function to fill descriptor entries
  - [x] 2.4 Implement gdt_init() to populate all GDT entries
  - [x] 2.5 Document each descriptor field with Intel SDM references

- [x] **Task 3: Create GDT Flush Assembly** (AC: #3)
  - [x] 3.1 Create `kernel/init/gdt_flush.S` (or add to existing entry.S)
  - [x] 3.2 Implement gdt_flush(gdt_ptr) to load GDTR with lgdt
  - [x] 3.3 Perform far jump to reload CS with new kernel code selector
  - [x] 3.4 Reload DS, ES, FS, GS, SS with kernel data selector
  - [x] 3.5 Document register state before/after

- [x] **Task 4: Integrate GDT Init into Kernel Boot** (AC: #1, #3)
  - [x] 4.1 Call gdt_init() from kmain() as first initialization step
  - [x] 4.2 Verify kernel continues running after GDT switch
  - [x] 4.3 Add VGA output to confirm GDT initialization ("GDT OK")

- [x] **Task 5: Update Build System** (AC: #1)
  - [x] 5.1 Add gdt.c to kernel source list in Makefile
  - [x] 5.2 Add gdt_flush.S to kernel assembly sources
  - [x] 5.3 Ensure build completes without errors

- [x] **Task 6: Testing and Verification** (AC: #1, #2, #3, #4)
  - [x] 6.1 Run `make qemu` and verify system boots
  - [x] 6.2 Verify "GDT OK" or similar appears on screen
  - [x] 6.3 Use GDB to inspect GDTR and segment registers after init
  - [x] 6.4 Add in-kernel test for GDT (test_gdt.c) to verify segment values

---

## Dev Notes

### What This Story Accomplishes

Story 1.3 set up a minimal GDT in stage 2 bootloader for the real-to-protected mode transition. That GDT only has 3 entries (null, code, data) and lives in bootloader memory.

This story creates the **kernel's own GDT** with:
- All entries the kernel will ever need (including user mode and TSS placeholders)
- Proper location in kernel memory
- Full documentation following Intel SDM

### GDT Descriptor Format (Intel SDM Vol 3, Section 3.4.5)

Each GDT entry is 8 bytes with a complex layout for historical reasons:

```
Byte 7:   Base[31:24]
Byte 6:   Flags (4 bits) | Limit[19:16] (4 bits)
Byte 5:   Access byte
Byte 4:   Base[23:16]
Bytes 3-2: Base[15:0]
Bytes 1-0: Limit[15:0]
```

**Access Byte (Byte 5):**
```
Bit 7:    Present (P) - must be 1 for valid segment
Bits 6-5: DPL - Descriptor Privilege Level (0=kernel, 3=user)
Bit 4:    Descriptor Type (S) - 1 for code/data, 0 for system
Bit 3:    Executable (E) - 1 for code, 0 for data
Bit 2:    Direction/Conforming (DC)
Bit 1:    Read/Write (RW) - readable for code, writable for data
Bit 0:    Accessed (A) - CPU sets when segment accessed
```

**Flags (upper 4 bits of Byte 6):**
```
Bit 7 (G):  Granularity - 0=byte, 1=4KB pages
Bit 6 (DB): Size - 0=16-bit, 1=32-bit
Bit 5 (L):  Long mode - 0 for 32-bit (must be 0)
Bit 4:      Available for OS use
```

### GDT Entries Required

| Index | Selector | Description | Access | Flags |
|-------|----------|-------------|--------|-------|
| 0 | 0x00 | Null descriptor | 0x00 | 0x0 |
| 1 | 0x08 | Kernel code (ring 0) | 0x9A | 0xC |
| 2 | 0x10 | Kernel data (ring 0) | 0x92 | 0xC |
| 3 | 0x18 | User code (ring 3) | 0xFA | 0xC |
| 4 | 0x20 | User data (ring 3) | 0xF2 | 0xC |
| 5 | 0x28 | TSS (placeholder) | 0x89 | 0x0 |

**Selector calculation:** Index * 8 + RPL
- Kernel selectors: RPL=0, so selector = index * 8
- User selectors: RPL=3, so selector = index * 8 + 3 = 0x1B (code), 0x23 (data)

### Access Byte Values Explained

**0x9A (Kernel Code):** 10011010b
- P=1 (present), DPL=00 (ring 0), S=1 (code/data), E=1 (executable), DC=0, RW=1 (readable), A=0

**0x92 (Kernel Data):** 10010010b
- P=1, DPL=00, S=1, E=0 (data), DC=0, RW=1 (writable), A=0

**0xFA (User Code):** 11111010b
- P=1, DPL=11 (ring 3), S=1, E=1, DC=0, RW=1, A=0

**0xF2 (User Data):** 11110010b
- P=1, DPL=11, S=1, E=0, DC=0, RW=1, A=0

**0x89 (TSS):** 10001001b
- P=1, DPL=00, S=0 (system), Type=1001 (32-bit TSS available)

### Implementation Pattern

```c
/* kernel/include/gdt.h */

#ifndef KERNEL_INIT_GDT_H
#define KERNEL_INIT_GDT_H

#include <types.h>

/* Segment selectors */
#define KERNEL_CS   0x08    /* Kernel code segment */
#define KERNEL_DS   0x10    /* Kernel data segment */
#define USER_CS     0x1B    /* User code segment (0x18 | RPL=3) */
#define USER_DS     0x23    /* User data segment (0x20 | RPL=3) */
#define TSS_SEG     0x28    /* TSS segment */

/*
 * GDT entry structure (8 bytes)
 *
 * Packed to prevent compiler padding. Order matches
 * Intel SDM Vol 3, Figure 3-8 (Segment Descriptor).
 */
struct gdt_entry {
    uint16_t limit_low;     /* Limit bits 0-15 */
    uint16_t base_low;      /* Base bits 0-15 */
    uint8_t  base_middle;   /* Base bits 16-23 */
    uint8_t  access;        /* Access byte */
    uint8_t  granularity;   /* Flags + limit bits 16-19 */
    uint8_t  base_high;     /* Base bits 24-31 */
} __attribute__((packed));

/*
 * GDT pointer structure (6 bytes)
 *
 * Used by LGDT instruction.
 */
struct gdt_ptr {
    uint16_t limit;         /* Size of GDT - 1 */
    uint32_t base;          /* Linear address of GDT */
} __attribute__((packed));

/* Initialize the GDT */
void gdt_init(void);

#endif /* KERNEL_INIT_GDT_H */
```

```c
/* kernel/init/gdt.c */

#include "gdt.h"

/* GDT with 6 entries */
static struct gdt_entry gdt[6];
static struct gdt_ptr gdt_pointer;

/* Assembly function to load GDT and reload segments */
extern void gdt_flush(uint32_t);

/*
 * gdt_set_gate - Set a GDT descriptor entry
 *
 * @num:    Entry index (0-5)
 * @base:   Segment base address
 * @limit:  Segment limit (size - 1)
 * @access: Access byte (P, DPL, S, type bits)
 * @flags:  Flags (G, D/B, L, AVL)
 */
static void gdt_set_gate(int num, uint32_t base, uint32_t limit,
                         uint8_t access, uint8_t flags)
{
    gdt[num].base_low    = base & 0xFFFF;
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;

    gdt[num].limit_low   = limit & 0xFFFF;
    gdt[num].granularity = (limit >> 16) & 0x0F;
    gdt[num].granularity |= (flags << 4) & 0xF0;

    gdt[num].access = access;
}

void gdt_init(void)
{
    gdt_pointer.limit = sizeof(gdt) - 1;
    gdt_pointer.base  = (uint32_t)&gdt;

    /* Entry 0: Null descriptor (required) */
    gdt_set_gate(0, 0, 0, 0, 0);

    /* Entry 1: Kernel code (0x08) */
    gdt_set_gate(1, 0, 0xFFFFF, 0x9A, 0xC);

    /* Entry 2: Kernel data (0x10) */
    gdt_set_gate(2, 0, 0xFFFFF, 0x92, 0xC);

    /* Entry 3: User code (0x18) - placeholder for Story 5.1 */
    gdt_set_gate(3, 0, 0xFFFFF, 0xFA, 0xC);

    /* Entry 4: User data (0x20) - placeholder for Story 5.1 */
    gdt_set_gate(4, 0, 0xFFFFF, 0xF2, 0xC);

    /* Entry 5: TSS (0x28) - placeholder for Story 5.1 */
    gdt_set_gate(5, 0, 0, 0, 0);  /* Will be filled by tss_init() */

    gdt_flush((uint32_t)&gdt_pointer);
}
```

```asm
/* kernel/init/gdt_flush.S */

/*
 * gdt_flush - Load new GDT and reload segment registers
 *
 * Input:  4(%esp) = pointer to gdt_ptr structure
 * Output: None
 * Clobbers: EAX
 *
 * After loading GDTR, we must reload all segment registers.
 * CS requires a far jump; others use regular MOV.
 */
.global gdt_flush
gdt_flush:
    movl 4(%esp), %eax      /* Get pointer to gdt_ptr */
    lgdt (%eax)             /* Load GDT register */

    /* Reload CS via far jump */
    ljmp $0x08, $.reload_segments

.reload_segments:
    /* Reload data segment registers */
    movw $0x10, %ax         /* Kernel data selector */
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    ret
```

### File Locations (per Architecture)

| File | Purpose |
|------|---------|
| `kernel/include/gdt.h` | GDT structures and constants |
| `kernel/init/gdt.c` | GDT initialization |
| `kernel/init/gdt_flush.S` | Assembly to load GDT and reload segments |

### Integration with main.c

```c
/* In kmain(), add as first initialization step: */
void kmain(void)
{
    /* Initialize GDT (must be first - we need proper segments) */
    gdt_init();

    /* ... rest of existing code ... */
}
```

### Makefile Changes

Add to kernel sources:
```makefile
KERNEL_C_SOURCES += kernel/init/gdt.c
KERNEL_ASM_SOURCES += kernel/init/gdt_flush.S
```

### Testing Strategy

1. **Boot test:** System boots without triple fault
2. **VGA output:** Display "GDT" after init to confirm success
3. **GDB inspection:**
   ```
   (gdb) info registers
   # Check CS=0x08, DS=ES=SS=0x10

   (gdb) x/6gx &gdt
   # Inspect GDT entries
   ```
4. **In-kernel test (test_gdt.c):**
   - Verify CS register contains 0x08
   - Verify DS register contains 0x10
   - Verify GDT entries have expected values

### Relationship to Other Stories

- **Depends on:** Story 1.3 (kernel entry exists, basic protected mode)
- **Required by:** Story 5.1 (TSS setup for user mode)
- **Related:** Story 2.1 (IDT setup follows similar pattern)

### Common Pitfalls

1. **Forgetting packed attribute** - Compiler may add padding, breaking structure
2. **Wrong segment selector after far jump** - Must use 0x08 (not 1) for CS
3. **Not reloading all segment registers** - FS and GS often forgotten
4. **TSS entry format differs** - It's a system descriptor, not code/data

### Project Structure Notes

**New Files:**
- `kernel/include/gdt.h`
- `kernel/init/gdt.c`
- `kernel/init/gdt_flush.S`

**Modified Files:**
- `kernel/init/main.c` - Call gdt_init()
- `Makefile` - Add new source files

### References

- [Source: _bmad-output/planning-artifacts/architecture.md#Implementation-Patterns]
- [Source: _bmad-output/planning-artifacts/epics.md#Story-1.4]
- [Source: _bmad-output/project-context.md#Critical-C-Rules]
- [Source: Intel SDM Vol 3, Chapter 3 - Protected-Mode Memory Management]
- [Source: Intel SDM Vol 3, Section 3.4.5 - Segment Descriptors]
- [Source: OSDev Wiki - GDT Tutorial]

### Previous Story Intelligence

**From Story 1.3:**
- Bootloader's GDT at boot/stage2.S - 3 entries only (null, code, data)
- GDT in .rodata section, aligned to 8 bytes
- Entry point at kernel/init/entry.S calls kmain()
- VGA output works - display success message after GDT init
- AT&T assembly syntax used throughout

---

## Dev Agent Record

### Agent Model Used

Claude Opus 4.5 (claude-opus-4-5-20251101)

### Debug Log References

- QEMU segment register dump confirmed CS=0x08, DS/SS/ES/FS/GS=0x10

### Completion Notes List

- Created kernel GDT with 6 entries: null, kernel code (0x08), kernel data (0x10), user code (0x18), user data (0x20), TSS placeholder (0x28)
- GDT structures packed to 8 bytes (entry) and 6 bytes (pointer) per Intel SDM spec
- Assembly routine gdt_flush loads GDTR via lgdt and reloads all segment registers
- Far jump (ljmp $0x08) used to reload CS; MOV used for data segments
- VGA output changed from "OK" to "GDT OK" to indicate successful initialization
- Unit tests added to verify selector constants and segment register values
- Build system automatically picks up new files via Makefile wildcard patterns
- Kernel boots successfully without triple fault, confirming GDT is valid

### File List

**New Files:**
- kernel/include/gdt.h
- kernel/init/gdt.c
- kernel/init/gdt_flush.S
- kernel/test/test_gdt.c
- tests/ (host-side test infrastructure)
  - tests/host/test_gdt.c (GDT encoding tests)
  - tests/host/unity/ (Unity test framework)
  - tests/Makefile
  - tests/README.md

**Modified Files:**
- kernel/include/types.h (added HOST_TEST conditional for host builds)
- kernel/init/main.c
- kernel/test/test_runner.c
- Makefile (added host-test target)

### Change Log

- 2026-01-15: Story 1.4 implementation complete - GDT initialized with kernel/user segments and TSS placeholder

