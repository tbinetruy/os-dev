# Story 1.3: Stage 2 Bootloader & Protected Mode

Status: review

## Story

As a developer,
I want stage 2 to enable A20, switch to protected mode, and load the kernel,
so that I understand CPU mode transitions and memory access beyond 1MB.

## Acceptance Criteria

1. **AC1: A20 Line Enablement**
   - Given stage 2 is executing in real mode
   - When it initializes
   - Then it enables the A20 line for full memory access
   - And A20 enablement is verified before proceeding

2. **AC2: Protected Mode Transition**
   - Given A20 is enabled
   - When stage 2 prepares for protected mode
   - Then it sets up a minimal GDT for the transition
   - And disables interrupts
   - And sets CR0.PE bit to enter protected mode
   - And performs far jump to flush prefetch queue

3. **AC3: Kernel Loading**
   - Given CPU is in protected mode
   - When stage 2 loads the kernel
   - Then kernel is loaded from disk to physical address 0x100000 (1MB)
   - And kernel size is determined from disk or header

4. **AC4: Kernel Entry**
   - Given kernel is loaded
   - When stage 2 transfers control
   - Then it jumps to kernel entry point with defined register state
   - And memory map information is passed to kernel (if available from BIOS)

## Tasks / Subtasks

- [x] **Task 1: A20 Line Enablement** (AC: #1)
  - [x] 1.1 Implement keyboard controller method (port 0x64/0x60)
  - [x] 1.2 Implement Fast A20 method (port 0x92) as fallback
  - [x] 1.3 Implement A20 verification routine (test wrap-around at 1MB boundary)
  - [x] 1.4 Retry A20 enablement if verification fails
  - [x] 1.5 Print error and halt if A20 cannot be enabled

- [x] **Task 2: GDT Setup** (AC: #2)
  - [x] 2.1 Define null descriptor (entry 0)
  - [x] 2.2 Define kernel code segment descriptor (entry 1): base=0, limit=4GB, ring 0, executable
  - [x] 2.3 Define kernel data segment descriptor (entry 2): base=0, limit=4GB, ring 0, read/write
  - [x] 2.4 Define GDT pointer structure (limit + base address)
  - [x] 2.5 Document segment selector values (0x08 for code, 0x10 for data)

- [x] **Task 3: Protected Mode Switch** (AC: #2)
  - [x] 3.1 Disable interrupts (cli)
  - [x] 3.2 Load GDT with lgdt instruction
  - [x] 3.3 Set CR0.PE bit (bit 0) to enable protected mode
  - [x] 3.4 Far jump to 32-bit code segment to flush prefetch queue
  - [x] 3.5 Reload segment registers (DS, ES, FS, GS, SS) with data selector

- [x] **Task 4: Kernel Loading (in Protected Mode)** (AC: #3)
  - [x] 4.1 Implement disk read routine for protected mode (can't use BIOS)
  - [x] 4.2 **Alternative:** Load kernel in real mode BEFORE switching to protected mode
  - [x] 4.3 Define kernel load address constant (0x100000 = 1MB)
  - [x] 4.4 Define kernel sector start and size constants
  - [x] 4.5 Copy kernel from low memory to 0x100000 if loaded in real mode

- [x] **Task 5: Memory Map (Optional but Recommended)** (AC: #4)
  - [x] 5.1 Query BIOS INT 0x15, EAX=0xE820 for memory map (before protected mode)
  - [x] 5.2 Store memory map entries at known location
  - [x] 5.3 Pass memory map pointer/count to kernel

- [x] **Task 6: Kernel Entry** (AC: #4)
  - [x] 6.1 Set up initial stack for kernel (e.g., at 0x90000)
  - [x] 6.2 Clear EAX, EBX, etc. or set to known values
  - [x] 6.3 Jump to kernel entry point at 0x100000
  - [x] 6.4 Document register state contract between stage 2 and kernel

- [x] **Task 7: Create Kernel Stub** (AC: #3, #4)
  - [x] 7.1 Create minimal kernel entry in `kernel/init/entry.S`
  - [x] 7.2 Entry sets up stack and calls kmain()
  - [x] 7.3 Create minimal kmain() in `kernel/init/main.c` that halts
  - [x] 7.4 Update Makefile to build and link kernel

- [x] **Task 8: Update Disk Image Layout** (AC: #3)
  - [x] 8.1 Update Makefile to place kernel at correct disk offset
  - [x] 8.2 Define sector layout: stage1 (1 sector), stage2 (N sectors), kernel (M sectors)
  - [x] 8.3 Update KERNEL_SECTORS constant based on actual kernel size

- [x] **Task 9: Testing and Verification** (AC: #1, #2, #3, #4)
  - [x] 9.1 Verify A20 is enabled (test memory access above 1MB)
  - [x] 9.2 Verify protected mode entry (check CR0.PE bit)
  - [x] 9.3 Verify kernel loads to correct address
  - [x] 9.4 Verify kernel entry executes (print character or halt indicator)

---

## Dev Notes

## Part 1: Understanding the Big Picture

### What This Story Accomplishes

This story transforms our bootloader from a simple "load and jump" program into something that fundamentally changes how the CPU operates. When we're done:

1. **A20 Line Enabled** - We can access memory above 1MB (the full 4GB address space)
2. **Protected Mode Active** - The CPU runs in 32-bit mode with memory protection
3. **Kernel Loaded** - Our actual OS kernel sits at 1MB, ready to run
4. **Control Transferred** - The kernel takes over from the bootloader

### Why Can't We Just Load the Kernel Directly?

When the computer starts, the CPU is in **real mode** - a backwards-compatible mode that works like the original 8086 processor from 1978. This mode has severe limitations:

| Limitation | Real Mode | Protected Mode |
|------------|-----------|----------------|
| Address Space | 1 MB max | 4 GB |
| Registers | 16-bit | 32-bit |
| Memory Protection | None | Full (rings 0-3) |
| BIOS Access | Yes | No |

Our kernel needs to run in protected mode (32-bit, full memory access), but we can only load it while we still have BIOS access (real mode). This creates a chicken-and-egg problem that stage 2 solves.

### The Boot Journey So Far

```
Power On
    │
    ▼
┌─────────────────────────────────────────────────────────────┐
│ BIOS (firmware on motherboard)                              │
│ - POST (Power-On Self Test)                                 │
│ - Finds bootable disk                                       │
│ - Loads sector 0 (MBR) to 0x7C00                           │
│ - Jumps to 0x7C00                                          │
└───────────────────────────────┬─────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────┐
│ Stage 1 (MBR) - Story 1.2 ✓ DONE                           │
│ - Exactly 512 bytes                                         │
│ - Sets up segments and stack                                │
│ - Loads stage 2 to 0x7E00                                  │
│ - Jumps to stage 2                                         │
└───────────────────────────────┬─────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────┐
│ Stage 2 - THIS STORY                                        │
│ - Enable A20 line                                           │
│ - Load kernel to temporary location                         │
│ - Set up GDT (segment descriptors)                         │
│ - Switch to protected mode                                  │
│ - Copy kernel to 1MB                                        │
│ - Jump to kernel                                            │
└───────────────────────────────┬─────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────┐
│ Kernel - Story 1.4+                                         │
│ - Running in 32-bit protected mode                         │
│ - Full 4GB address space available                         │
│ - Sets up its own GDT, IDT, paging, etc.                   │
└─────────────────────────────────────────────────────────────┘
```

---

## Part 2: The A20 Line - A Historical Quirk

### What Is the A20 Line?

The A20 line is the 21st address line of the CPU (A0-A19 = 20 lines, A20 is the 21st). In binary, it represents bit 20 of any memory address.

**Why Does It Matter?**

The original IBM PC (8086 processor) had only 20 address lines, giving it a 1MB address space:
- 2^20 = 1,048,576 bytes = 1 MB

When the 80286 came out with 24 address lines (16MB), IBM had a problem: some old software relied on a quirk where memory addresses "wrapped around" at the 1MB boundary. If you tried to access address 0x100000 (1MB), you'd actually get address 0x000000 (0).

IBM's solution was the **A20 gate** - a physical gate that could disable the A20 address line, forcing it to always be 0. This preserved the wrap-around behavior for old software.

### Why We Must Enable It

With A20 disabled, addresses work like this:

```
Address Requested    Actual Address (A20 forced to 0)
0x000000            0x000000    ← Works normally
0x0FFFFF            0x0FFFFF    ← Works normally (last byte under 1MB)
0x100000            0x000000    ← WRONG! Wraps to 0!
0x100001            0x000001    ← WRONG! Wraps to 1!
0x1FFFFF            0x0FFFFF    ← WRONG! Wraps to under 1MB!
```

Our kernel loads at 0x100000 (1MB). Without A20, we'd be writing to address 0x000000 - overwriting the interrupt vector table and crashing immediately!

### How to Enable A20

The A20 line was historically controlled by the keyboard controller (yes, really - IBM reused spare pins). There are multiple methods:

**Method 1: Keyboard Controller (Most Compatible)**

The keyboard controller has command port 0x64 and data port 0x60. We send a "write output port" command and set bit 1:

```asm
/*
 * enable_a20_keyboard - Enable A20 via keyboard controller
 *
 * The keyboard controller (8042) has an output port with bit 1
 * controlling the A20 line. We must:
 * 1. Wait for controller to be ready (input buffer empty)
 * 2. Send "write output port" command (0xD1) to command port
 * 3. Wait for controller to be ready again
 * 4. Write the new value (0xDF = all bits set including A20)
 *
 * Port 0x64: Command/Status port
 *   - Read: Status register (bit 1 = input buffer full)
 *   - Write: Command byte
 *
 * Port 0x60: Data port
 *   - Read/Write: Data for commands
 */
enable_a20_keyboard:
    /* Wait for input buffer to be empty (bit 1 of status = 0) */
.wait_input1:
    inb $0x64, %al              /* Read status register */
    testb $0x02, %al            /* Test bit 1 (input buffer status) */
    jnz .wait_input1            /* Loop if buffer not empty */

    /* Send command: "Write to output port" (0xD1) */
    movb $0xD1, %al
    outb %al, $0x64

    /* Wait for input buffer again */
.wait_input2:
    inb $0x64, %al
    testb $0x02, %al
    jnz .wait_input2

    /* Write new output port value with A20 enabled (bit 1 = 1) */
    /* 0xDF = 11011111 binary - all bits set except bit 5 */
    movb $0xDF, %al
    outb %al, $0x60

    ret
```

**Method 2: Fast A20 (Fallback)**

Modern systems support a faster method using system control port 0x92:

```asm
/*
 * enable_a20_fast - Enable A20 via System Control Port A
 *
 * Port 0x92 bit 1 controls A20 on many systems.
 * WARNING: Bit 0 is system reset - must NOT set it!
 */
enable_a20_fast:
    inb $0x92, %al              /* Read current value */
    orb $0x02, %al              /* Set bit 1 (A20 enable) */
    andb $0xFE, %al             /* Clear bit 0 (system reset) - CRITICAL! */
    outb %al, $0x92
    ret
```

**Verifying A20 Is Enabled**

We MUST verify A20 is actually enabled - some systems need multiple attempts:

```asm
/*
 * check_a20 - Test if A20 line is enabled
 *
 * Strategy: Write a unique value to address 0x100000 (1MB), then
 * write a DIFFERENT value to address 0x000000 (0). If A20 is
 * disabled, writing to 0x100000 actually writes to 0x000000
 * (wrap-around), so both addresses will have the same value.
 *
 * Returns: AL = 1 if A20 enabled, 0 if disabled
 *
 * Note: We use addresses 0x000500 and 0x100500 to avoid
 * the interrupt vector table at 0x000000.
 */
check_a20:
    pushw %ds
    pushw %es
    pushw %di
    pushw %si

    xorw %ax, %ax
    movw %ax, %es               /* ES = 0x0000 */
    movw $0xFFFF, %ax
    movw %ax, %ds               /* DS = 0xFFFF */

    /* ES:DI = 0x0000:0x0500 = physical 0x000500 */
    movw $0x0500, %di

    /* DS:SI = 0xFFFF:0x0510 = physical 0x100500 (0xFFFF0 + 0x510 = 0x100500) */
    movw $0x0510, %si

    /* Save original values */
    movb %es:(%di), %al
    pushw %ax
    movb %ds:(%si), %al
    pushw %ax

    /* Write different values to each location */
    movb $0x00, %es:(%di)       /* Write 0x00 to 0x000500 */
    movb $0xFF, %ds:(%si)       /* Write 0xFF to 0x100500 */

    /* If A20 disabled, 0x100500 wrapped to 0x000500, so both are 0xFF */
    cmpb $0xFF, %es:(%di)       /* Check if 0x000500 was overwritten */

    /* Restore original values */
    popw %ax
    movb %al, %ds:(%si)
    popw %ax
    movb %al, %es:(%di)

    popw %si
    popw %di
    popw %es
    popw %ds

    /* Set return value based on comparison */
    jne .a20_enabled
    xorb %al, %al               /* AL = 0: A20 disabled */
    ret
.a20_enabled:
    movb $0x01, %al             /* AL = 1: A20 enabled */
    ret
```

---

## Part 3: The Global Descriptor Table (GDT)

### What Is Memory Segmentation?

In protected mode, the CPU doesn't use memory addresses directly. Instead, every memory access uses a **segment selector** that points to a **segment descriptor** in the GDT.

**Real Mode Segmentation (Simple):**
```
Physical Address = (Segment Register × 16) + Offset
Example: CS=0x1000, IP=0x0200 → Address = 0x10000 + 0x0200 = 0x10200
```

**Protected Mode Segmentation (Complex):**
```
Segment Register contains a SELECTOR, not an address
Selector points to a DESCRIPTOR in the GDT
Descriptor defines: base address, limit, permissions, type
Physical Address = Descriptor.Base + Offset (if offset < limit)
```

### Why Do We Need a GDT?

The GDT serves two purposes:

1. **Memory Protection**: Each segment has access permissions (read, write, execute) and a privilege level (ring 0-3). Code can't access memory outside its segments without permission.

2. **Mode Requirement**: The CPU simply won't enter protected mode without a valid GDT. The first thing it does after `mov cr0, eax` (with PE bit set) is validate the current code segment.

### GDT Structure Explained

The GDT is an array of 8-byte **segment descriptors**. Each descriptor defines a memory segment:

```
Descriptor Entry (8 bytes = 64 bits):

Byte:   7        6        5        4        3        2        1        0
      ┌────────┬────────┬────────┬────────┬────────┬────────┬────────┬────────┐
      │Base    │Flags + │Access  │Base    │Base    │Base    │Limit   │Limit   │
      │(31-24) │Lim(19-16)│Byte   │(23-16) │(15-8)  │(7-0)   │(15-8)  │(7-0)   │
      └────────┴────────┴────────┴────────┴────────┴────────┴────────┴────────┘

Why is it so weird? Historical reasons! The 80286 had a different format,
and Intel had to maintain backwards compatibility when the 80386 came out.
```

**Access Byte (Byte 5) Breakdown:**

```
  Bit 7    Bit 6-5    Bit 4    Bit 3    Bit 2    Bit 1    Bit 0
┌────────┬──────────┬────────┬────────┬────────┬────────┬────────┐
│Present │Privilege │  Type  │Execut- │Direct- │ R/W    │Accessed│
│  (P)   │  (DPL)   │  (S)   │  able  │  ion   │        │  (A)   │
└────────┴──────────┴────────┴────────┴────────┴────────┴────────┘

Present (P):     1 = segment is valid, 0 = segment not in memory
Privilege (DPL): 0 = highest (kernel), 3 = lowest (user)
Type (S):        1 = code/data segment, 0 = system segment (TSS, etc.)
Executable:      1 = code segment, 0 = data segment
Direction:       For data: 0 = grows up, 1 = grows down
                 For code: 0 = non-conforming, 1 = conforming
R/W:             For code: 1 = readable (can read code as data)
                 For data: 1 = writable
Accessed (A):    CPU sets this to 1 when segment is accessed
```

**Flags + Upper Limit (Byte 6) Breakdown:**

```
  Bit 7    Bit 6    Bit 5    Bit 4    Bit 3-0
┌────────┬────────┬────────┬────────┬────────────┐
│Granul- │ Size   │64-bit  │  AVL   │Limit(19-16)│
│arity   │(D/B)   │  (L)   │        │            │
└────────┴────────┴────────┴────────┴────────────┘

Granularity (G): 0 = limit in bytes, 1 = limit in 4KB pages
Size (D/B):      0 = 16-bit segment, 1 = 32-bit segment
64-bit (L):      1 = 64-bit code segment (x86-64 only), must be 0 for 32-bit
AVL:             Available for OS use (we set to 0)
```

### Our Minimal GDT

For entering protected mode, we need exactly 3 entries:

```
Entry 0 (Null Descriptor):
┌─────────────────────────────────────────────────────────────────────────────┐
│ 0x0000000000000000                                                          │
│ Required by CPU - accessing this segment causes a fault (useful for errors) │
└─────────────────────────────────────────────────────────────────────────────┘

Entry 1 (Code Segment - selector 0x08):
┌─────────────────────────────────────────────────────────────────────────────┐
│ Base = 0x00000000 (starts at address 0)                                     │
│ Limit = 0xFFFFF (with 4KB granularity = 4GB)                               │
│ Access = 0x9A: Present, Ring 0, Code, Executable, Readable                 │
│ Flags = 0xC: 4KB granularity, 32-bit                                       │
└─────────────────────────────────────────────────────────────────────────────┘

Entry 2 (Data Segment - selector 0x10):
┌─────────────────────────────────────────────────────────────────────────────┐
│ Base = 0x00000000 (starts at address 0)                                     │
│ Limit = 0xFFFFF (with 4KB granularity = 4GB)                               │
│ Access = 0x92: Present, Ring 0, Data, Writable                             │
│ Flags = 0xC: 4KB granularity, 32-bit                                       │
└─────────────────────────────────────────────────────────────────────────────┘
```

### GDT in Assembly

```asm
/*
 * Global Descriptor Table (GDT)
 *
 * This is the most important data structure for protected mode.
 * The CPU uses it to translate segment selectors to memory addresses
 * and enforce protection.
 *
 * We use a "flat" memory model where both code and data segments
 * span the entire 4GB address space (base=0, limit=4GB). This is
 * standard for modern OSes - we'll use paging for real protection.
 */

    .align 8                    /* GDT must be aligned for performance */
gdt_start:
    /*
     * Entry 0: Null Descriptor (required)
     * The CPU requires the first GDT entry to be null.
     * Any segment selector pointing here causes a fault.
     */
    .quad 0x0000000000000000

    /*
     * Entry 1: Kernel Code Segment (selector = 0x08)
     *
     * This segment allows the CPU to EXECUTE code.
     *
     * Raw bytes: 0x00CF9A000000FFFF
     * Breakdown:
     *   Limit (0-15):  0xFFFF
     *   Base (0-15):   0x0000
     *   Base (16-23):  0x00
     *   Access:        0x9A = 10011010b
     *                    Present=1, DPL=0, S=1, Exec=1, DC=0, RW=1, A=0
     *   Flags+Lim:     0xCF = 11001111b
     *                    G=1 (4KB pages), D=1 (32-bit), L=0, AVL=0, Lim=F
     *   Base (24-31):  0x00
     */
    .word 0xFFFF                /* Limit bits 0-15 */
    .word 0x0000                /* Base bits 0-15 */
    .byte 0x00                  /* Base bits 16-23 */
    .byte 0x9A                  /* Access byte: present, ring 0, code, readable */
    .byte 0xCF                  /* Flags (4KB gran, 32-bit) + limit bits 16-19 */
    .byte 0x00                  /* Base bits 24-31 */

    /*
     * Entry 2: Kernel Data Segment (selector = 0x10)
     *
     * This segment allows the CPU to READ and WRITE data.
     *
     * Access: 0x92 = 10010010b
     *   Same as code except: Exec=0 (data), RW=1 (writable)
     */
    .word 0xFFFF                /* Limit bits 0-15 */
    .word 0x0000                /* Base bits 0-15 */
    .byte 0x00                  /* Base bits 16-23 */
    .byte 0x92                  /* Access byte: present, ring 0, data, writable */
    .byte 0xCF                  /* Flags (4KB gran, 32-bit) + limit bits 16-19 */
    .byte 0x00                  /* Base bits 24-31 */

gdt_end:

/*
 * GDT Pointer Structure
 *
 * The LGDT instruction expects a 6-byte structure:
 *   - 2 bytes: Limit (size of GDT minus 1)
 *   - 4 bytes: Base address of GDT
 */
gdt_ptr:
    .word gdt_end - gdt_start - 1   /* Limit: size - 1 */
    .long gdt_start                  /* Base: address of GDT */

/*
 * Segment Selectors
 *
 * When we load a segment register (CS, DS, etc.), we load a SELECTOR,
 * not a descriptor. The selector format:
 *
 *   Bits 15-3: Index into GDT (which entry)
 *   Bit 2:     Table indicator (0 = GDT, 1 = LDT)
 *   Bits 1-0:  Requested Privilege Level (RPL)
 *
 * For entry 1 (code): 1 << 3 = 0x08
 * For entry 2 (data): 2 << 3 = 0x10
 */
.equ CODE_SEG, 0x08             /* Selector for code segment */
.equ DATA_SEG, 0x10             /* Selector for data segment */
```

---

## Part 4: Switching to Protected Mode

### The Mode Switch Process

Switching from real mode to protected mode is a delicate operation that must be done in a specific order:

```
┌──────────────────────────────────────────────────────────────────────────────┐
│ STEP 1: Disable Interrupts (CLI)                                             │
│                                                                              │
│ WHY: Once we switch modes, the interrupt handlers (which are real-mode      │
│ BIOS routines) will crash. We need to disable interrupts until we set up    │
│ our own protected-mode IDT (which happens in a later story).                │
└──────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌──────────────────────────────────────────────────────────────────────────────┐
│ STEP 2: Load GDT (LGDT)                                                      │
│                                                                              │
│ WHY: The CPU needs the GDT before it can enter protected mode. The LGDT     │
│ instruction loads the GDT pointer into the CPU's GDTR register.             │
└──────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌──────────────────────────────────────────────────────────────────────────────┐
│ STEP 3: Set CR0.PE Bit                                                       │
│                                                                              │
│ CR0 is a Control Register. Bit 0 (PE = Protection Enable) controls          │
│ whether the CPU is in real mode (0) or protected mode (1).                  │
│                                                                              │
│ After this instruction, WE ARE IN PROTECTED MODE. But there's a catch...    │
└──────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌──────────────────────────────────────────────────────────────────────────────┐
│ STEP 4: Far Jump to Flush Pipeline (LJMP)                                    │
│                                                                              │
│ WHY: The CPU has a prefetch queue and instruction pipeline that may still   │
│ contain 16-bit real-mode instructions. A far jump forces the CPU to:        │
│   1. Flush the pipeline (discard prefetched instructions)                   │
│   2. Load a new Code Segment selector into CS                               │
│   3. Start fetching 32-bit protected-mode instructions                      │
│                                                                              │
│ The jump target must be 32-bit code (we switch assembler mode with .code32) │
└──────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌──────────────────────────────────────────────────────────────────────────────┐
│ STEP 5: Reload Segment Registers                                             │
│                                                                              │
│ WHY: DS, ES, SS, FS, GS still contain real-mode values. We must reload      │
│ them with proper protected-mode selectors. The far jump already loaded CS.  │
└──────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌──────────────────────────────────────────────────────────────────────────────┐
│ STEP 6: Set Up Stack                                                         │
│                                                                              │
│ WHY: We need a valid stack pointer in protected mode. We use a location     │
│ that's safe (not overlapping with kernel or boot code).                     │
└──────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
                    NOW IN 32-BIT PROTECTED MODE!
```

### The Code

```asm
/*
 * enter_protected_mode - Switch CPU from real mode to protected mode
 *
 * This is the critical transition. After this function, the CPU
 * operates in 32-bit mode with segment-based memory protection.
 *
 * IMPORTANT: This function NEVER returns! After the far jump,
 * execution continues in 32-bit code at protected_mode_entry.
 *
 * Prerequisites:
 *   - A20 line must be enabled
 *   - Kernel should be loaded to low memory
 *   - GDT must be defined
 */
enter_protected_mode:
    /*
     * Step 1: Disable interrupts
     *
     * CRITICAL: We MUST disable interrupts before switching modes.
     * BIOS interrupt handlers don't work in protected mode, and
     * we haven't set up an IDT yet. Any interrupt would crash.
     */
    cli

    /*
     * Step 2: Load the Global Descriptor Table
     *
     * The LGDT instruction loads a 6-byte pseudo-descriptor:
     *   - Bytes 0-1: GDT limit (size - 1)
     *   - Bytes 2-5: GDT base address
     *
     * After this, the CPU knows where to find segment descriptors.
     */
    lgdt (gdt_ptr)

    /*
     * Step 3: Enable protected mode
     *
     * CR0 (Control Register 0) bit 0 is the Protection Enable (PE) bit.
     * Setting it to 1 switches the CPU to protected mode.
     *
     * We must read CR0, set the bit, then write it back. We can't
     * write directly because other bits in CR0 are important.
     */
    movl %cr0, %eax             /* Read current CR0 value */
    orl $0x00000001, %eax       /* Set bit 0 (PE) */
    movl %eax, %cr0             /* Write back - NOW IN PROTECTED MODE! */

    /*
     * Step 4: Far jump to flush pipeline and load CS
     *
     * After setting CR0.PE, the CPU is technically in protected mode,
     * but it's still executing 16-bit real-mode code! The instruction
     * pipeline may contain prefetched real-mode instructions.
     *
     * A far jump does two critical things:
     *   1. Flushes the instruction pipeline
     *   2. Loads CS with our code segment selector (0x08)
     *
     * The syntax is: ljmp $selector, $offset
     * We jump to the label 'protected_mode_entry' with CS=CODE_SEG
     */
    ljmp $CODE_SEG, $protected_mode_entry


/*
 * 32-bit protected mode code starts here
 *
 * The .code32 directive tells the assembler to generate 32-bit
 * instructions from this point on. Without this, we'd still get
 * 16-bit code even though the CPU is in 32-bit mode.
 */
.code32

/*
 * protected_mode_entry - First 32-bit code after mode switch
 *
 * At this point:
 *   - CPU is in 32-bit protected mode
 *   - CS contains CODE_SEG (0x08) from the far jump
 *   - Other segment registers still have real-mode values (INVALID!)
 *   - Stack pointer is invalid
 *   - Interrupts are disabled
 */
protected_mode_entry:
    /*
     * Step 5: Reload all data segment registers
     *
     * We must load DS, ES, FS, GS, and SS with valid protected-mode
     * selectors. We use DATA_SEG (0x10) for all of them.
     *
     * Note: We can't use MOV with a segment register directly,
     * we must go through a general-purpose register.
     */
    movw $DATA_SEG, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    /*
     * Step 6: Set up stack
     *
     * We need a valid stack before we can call functions.
     * 0x90000 is a safe location above where we've loaded data.
     */
    movl $0x90000, %esp

    /*
     * Now in full 32-bit protected mode!
     * We can:
     *   - Use 32-bit registers (EAX, EBX, etc.)
     *   - Access the full 4GB address space
     *   - Call C functions
     *
     * We CANNOT:
     *   - Use BIOS interrupts (INT xx)
     *   - Handle hardware interrupts (no IDT yet)
     */
```

---

## Part 5: Loading the Kernel

### The Problem

We want to load the kernel at physical address 0x100000 (1MB). But there's a problem:

- **BIOS disk routines only work in real mode**
- **Protected mode can't use BIOS**
- **Real mode can only directly address up to 1MB** (without A20 tricks)

### The Solution: Two-Step Loading

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ PHASE 1 (Real Mode - BIOS Available)                                        │
│                                                                             │
│ 1. Enable A20 line (so we can access memory above 1MB)                     │
│ 2. Query memory map from BIOS (E820)                                       │
│ 3. Load kernel from disk to LOW memory (e.g., 0x10000 = 64KB)             │
│ 4. Switch to protected mode                                                 │
│                                                                             │
│ Memory Layout:                                                              │
│   0x00000 - 0x07BFF : Free (below stage 1)                                 │
│   0x07C00 - 0x07DFF : Stage 1 (can be overwritten now)                     │
│   0x07E00 - 0x0FFFF : Stage 2                                              │
│   0x10000 - 0x8FFFF : Kernel TEMPORARY location                            │
│   0x90000 - 0x9FFFF : Stack (in protected mode)                            │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ PHASE 2 (Protected Mode - Full Memory Access)                               │
│                                                                             │
│ 1. Copy kernel from 0x10000 to 0x100000 (1MB)                              │
│ 2. Set up registers for kernel                                              │
│ 3. Jump to kernel entry point                                               │
│                                                                             │
│ Memory Layout (final):                                                      │
│   0x000000 - 0x0FFFFF : Low memory (BIOS, VGA, boot code - can reclaim)   │
│   0x100000 - 0x??????  : KERNEL (our OS!)                                   │
│   ...                                                                       │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Disk Reading in Real Mode

We use BIOS INT 0x13 to read disk sectors (same as stage 1, but reading more sectors):

```asm
/*
 * load_kernel - Load kernel from disk to low memory
 *
 * This must be called BEFORE switching to protected mode because
 * it uses BIOS INT 0x13.
 *
 * We load the kernel to 0x10000 (64KB mark). This is:
 *   - Above stage 1 and stage 2
 *   - Below 1MB so we can address it in real mode
 *   - Easy to address with segment:offset (0x1000:0x0000)
 *
 * Input: boot_drive contains drive number from BIOS
 * Output: Kernel loaded to 0x10000
 */

/* Constants for disk layout */
.equ KERNEL_START_SECTOR, 5     /* Kernel starts at sector 5 */
                                 /* Sectors 1-4 are stage 2 */
.equ KERNEL_LOAD_SEG, 0x1000    /* Segment for 0x10000 */

load_kernel:
    /*
     * Set up ES:BX for BIOS disk read
     * ES:BX = destination address
     * 0x1000:0x0000 = physical address 0x10000
     */
    movw $KERNEL_LOAD_SEG, %ax
    movw %ax, %es
    xorw %bx, %bx               /* ES:BX = 0x1000:0x0000 = 0x10000 */

    /* Set up disk read parameters */
    movb (boot_drive), %dl      /* Drive number */
    movb $0x02, %ah             /* Function: read sectors */
    movb $KERNEL_SECTORS, %al   /* Number of sectors to read */
    movb $0, %ch                /* Cylinder 0 */
    movb $KERNEL_START_SECTOR, %cl  /* Starting sector */
    movb $0, %dh                /* Head 0 */

    /* Read with retry logic */
    movb $3, %si                /* 3 attempts */
.read_retry:
    pusha                       /* Save registers */
    int $0x13                   /* BIOS disk read */
    jnc .read_success           /* Jump if no error (carry clear) */
    popa
    decb %si
    jz .read_failed             /* All attempts failed */

    /* Reset disk controller and retry */
    xorb %ah, %ah
    int $0x13
    jmp .read_retry

.read_success:
    popa
    ret

.read_failed:
    movb $'K', %al              /* 'K' = Kernel load error */
    call print_char
    cli
    hlt
```

### Copying Kernel to 1MB (Protected Mode)

After switching to protected mode, we copy the kernel from low memory to its final destination:

```asm
/*
 * copy_kernel_to_1mb - Copy kernel from 0x10000 to 0x100000
 *
 * Called in 32-bit protected mode.
 *
 * Uses REP MOVSL for efficient copying:
 *   - ESI = source address
 *   - EDI = destination address
 *   - ECX = count (in double-words for MOVSL)
 *   - Direction flag must be clear (CLD)
 *
 * Note: KERNEL_SIZE must be defined based on actual kernel size.
 * For now we use a constant, later we can read it from the kernel header.
 */
.code32
copy_kernel_to_1mb:
    cld                         /* Clear direction flag (copy forward) */
    movl $0x10000, %esi         /* Source: low memory */
    movl $0x100000, %edi        /* Destination: 1MB */
    movl $KERNEL_SIZE_DWORDS, %ecx  /* Count in double-words */
    rep movsl                   /* Copy ECX double-words from ESI to EDI */
    ret

/* KERNEL_SIZE_DWORDS = number of bytes / 4 (rounded up) */
/* For a 64KB kernel: 64 * 1024 / 4 = 16384 */
.equ KERNEL_SIZE_DWORDS, 16384
```

---

## Part 6: Memory Map (E820)

### Why Get a Memory Map?

The kernel needs to know which memory regions are usable and which are reserved (for BIOS, ACPI, devices, etc.). The BIOS provides this information through INT 0x15, EAX=0xE820.

**Without a memory map:**
- Kernel might try to use reserved memory → crash
- Kernel won't know how much RAM is available

### E820 Interface

```asm
/*
 * get_memory_map - Query BIOS for memory map using E820
 *
 * Must be called in real mode before switching to protected mode.
 *
 * The E820 interface returns memory map entries one at a time.
 * We call it in a loop until EBX=0 (no more entries) or carry is set.
 *
 * Each entry describes a region of physical memory:
 *   - Base address (8 bytes)
 *   - Length (8 bytes)
 *   - Type (4 bytes): 1=usable, 2=reserved, 3=ACPI reclaimable, etc.
 *   - Extended attributes (4 bytes, optional)
 *
 * We store entries at MMAP_ADDR and count at MMAP_COUNT_ADDR.
 */

.equ MMAP_ADDR, 0x500           /* Store memory map here */
.equ MMAP_COUNT_ADDR, 0x4FC     /* Store entry count here */
.equ E820_MAGIC, 0x534D4150     /* 'SMAP' in ASCII */

get_memory_map:
    movw $0, (MMAP_COUNT_ADDR)  /* Initialize count to 0 */
    movw $MMAP_ADDR, %di        /* ES:DI = buffer for entries */
    xorl %ebx, %ebx             /* EBX = 0 for first call */

.e820_loop:
    movl $0xE820, %eax          /* E820 function */
    movl $E820_MAGIC, %edx      /* 'SMAP' signature */
    movl $24, %ecx              /* Buffer size (24 bytes per entry) */
    int $0x15                   /* Call BIOS */

    jc .e820_done               /* Carry set = error or done */
    cmpl $E820_MAGIC, %eax      /* BIOS should return 'SMAP' in EAX */
    jne .e820_done

    /* Valid entry received */
    incw (MMAP_COUNT_ADDR)      /* Increment entry count */
    addw $24, %di               /* Advance buffer pointer */

    testl %ebx, %ebx            /* EBX = 0 means last entry */
    jnz .e820_loop

.e820_done:
    ret
```

### Memory Map Entry Structure

```c
/*
 * E820 Memory Map Entry
 *
 * Each entry returned by BIOS INT 0x15 EAX=E820 has this format.
 * The kernel will use this to build its physical memory map.
 */
struct e820_entry {
    uint64_t base_addr;         /* Physical start address of region */
    uint64_t length;            /* Size of region in bytes */
    uint32_t type;              /* Type of memory region (see below) */
    uint32_t extended_attr;     /* ACPI 3.0 extended attributes */
};

/*
 * Memory Region Types:
 *   1 = AddressRangeMemory    - Usable RAM
 *   2 = AddressRangeReserved  - Reserved (BIOS, motherboard, etc.)
 *   3 = AddressRangeACPI      - ACPI Reclaimable (usable after parsing)
 *   4 = AddressRangeNVS       - ACPI Non-Volatile Storage
 *   5 = AddressRangeUnusable  - Bad memory
 */
```

---

## Part 7: Kernel Entry

### The Handoff

When stage 2 jumps to the kernel, it must set up a well-defined state:

```asm
/*
 * jump_to_kernel - Transfer control to the kernel
 *
 * This is the final step of the bootloader. We set up registers
 * with information the kernel needs, then jump to its entry point.
 *
 * Register convention (our choice, documented here):
 *   EAX = 0 (reserved for future use)
 *   EBX = pointer to memory map (MMAP_ADDR) or 0 if unavailable
 *   ECX = number of memory map entries or 0
 *   EDX = 0 (reserved)
 *   ESP = stack pointer (0x90000)
 *   EBP = 0 (frame pointer cleared)
 *
 * The kernel entry point is at 0x100000 (1MB) per our linker script.
 */
.equ KERNEL_ENTRY, 0x100000

jump_to_kernel:
    /* Set up registers for kernel */
    xorl %eax, %eax             /* EAX = 0 */
    movl $MMAP_ADDR, %ebx       /* EBX = memory map pointer */
    movzwl (MMAP_COUNT_ADDR), %ecx  /* ECX = memory map count */
    xorl %edx, %edx             /* EDX = 0 */
    xorl %ebp, %ebp             /* Clear frame pointer */

    /* Stack should already be set up at 0x90000 */

    /* Jump to kernel! */
    jmp *$KERNEL_ENTRY
    /* Never returns */
```

### Kernel Entry Code

The kernel needs a small assembly stub to receive control from the bootloader:

```asm
/* kernel/init/entry.S */

/*
 * Kernel Entry Point
 *
 * This is the first kernel code that runs after the bootloader.
 * It receives control in 32-bit protected mode with:
 *   - EBX = pointer to E820 memory map
 *   - ECX = number of memory map entries
 *   - ESP = valid stack (0x90000)
 *   - Interrupts disabled
 *   - Paging disabled
 *   - Running at physical address 0x100000
 *
 * Our job is to:
 *   1. Save boot information
 *   2. Set up C runtime (clear BSS)
 *   3. Call kmain()
 */

.section .text.boot             /* Put this first in binary */
.global _start
.extern kmain
.extern __bss_start
.extern __bss_end

_start:
    /* Save boot parameters */
    movl %ebx, boot_mmap_addr
    movl %ecx, boot_mmap_count

    /* Clear BSS section (uninitialized globals must be zero) */
    movl $__bss_start, %edi
    movl $__bss_end, %ecx
    subl %edi, %ecx             /* ECX = size of BSS */
    shrl $2, %ecx               /* Convert to double-words */
    xorl %eax, %eax             /* Value to store (0) */
    cld
    rep stosl                   /* Fill BSS with zeros */

    /* Set up stack (bootloader already did this, but be safe) */
    movl $0x90000, %esp

    /* Call C main function */
    call kmain

    /* If kmain returns, halt */
    cli
.halt_loop:
    hlt
    jmp .halt_loop

/* Boot parameters saved for kernel use */
.section .data
.global boot_mmap_addr
.global boot_mmap_count
boot_mmap_addr: .long 0
boot_mmap_count: .long 0
```

### Minimal kmain()

```c
/* kernel/init/main.c */

/*
 * kmain - Kernel Main Entry Point
 *
 * This is the first C code that runs. At this point:
 *   - We're in 32-bit protected mode
 *   - Interrupts are disabled
 *   - Paging is NOT enabled yet (Story 3.2)
 *   - We have no drivers, no console output yet
 *
 * For now, we just halt. Later stories add VGA, serial, etc.
 */

/* These are set by entry.S */
extern unsigned long boot_mmap_addr;
extern unsigned long boot_mmap_count;

void kmain(void)
{
    /*
     * TODO (Story 1.5): Initialize VGA driver, print message
     * TODO (Story 1.6): Initialize serial port, printk
     * TODO (Story 1.4): Set up proper GDT
     *
     * For now, reaching here means boot was successful!
     * The system will halt in entry.S after we return.
     */

    /* Indicate we got here (temporary - no output yet) */
    volatile unsigned char *vga = (volatile unsigned char *)0xB8000;
    vga[0] = 'K';  /* Write 'K' to top-left of screen */
    vga[1] = 0x0F; /* White on black */

    /* Halt - we're done for this story */
    for (;;) {
        __asm__ volatile ("hlt");
    }
}
```

---

## Part 8: Complete Stage 2 Code Structure

Here's the complete flow of stage 2:

```asm
/* boot/stage2.S */

/*
 * Stage 2 Bootloader
 *
 * Entry: Real mode, 16-bit
 * Exit: Jumps to kernel in protected mode, 32-bit
 *
 * Tasks:
 *   1. Enable A20 line
 *   2. Get memory map from BIOS
 *   3. Load kernel to low memory
 *   4. Switch to protected mode
 *   5. Copy kernel to 1MB
 *   6. Jump to kernel
 */

.code16
.section .text
.global _start

_start:
    /* Set direction flag for string operations */
    cld

    /* Print '2' to show stage 2 started */
    movb $'2', %al
    call print_char

    /* ======================================== */
    /* PHASE 1: Real Mode Operations            */
    /* (Must happen before protected mode)      */
    /* ======================================== */

    /* Step 1: Enable A20 line */
    call enable_a20
    call check_a20
    testb %al, %al
    jz .a20_failed

    /* Print 'A' to show A20 enabled */
    movb $'A', %al
    call print_char

    /* Step 2: Get memory map (optional but recommended) */
    call get_memory_map

    /* Step 3: Load kernel from disk to 0x10000 */
    call load_kernel

    /* Print 'L' to show kernel loaded */
    movb $'L', %al
    call print_char

    /* ======================================== */
    /* PHASE 2: Switch to Protected Mode        */
    /* ======================================== */

    /* Enter protected mode (never returns!) */
    call enter_protected_mode

.a20_failed:
    /* A20 enable failed */
    movb $'!', %al
    call print_char
    cli
    hlt
    jmp .a20_failed


/* ============================================ */
/* Real Mode Functions (16-bit)                 */
/* ============================================ */

/*
 * print_char - Print character in AL using BIOS
 */
print_char:
    movb $0x0E, %ah
    xorb %bh, %bh
    int $0x10
    ret

/* Include other real-mode functions here:
 *   enable_a20_keyboard
 *   enable_a20_fast
 *   enable_a20
 *   check_a20
 *   get_memory_map
 *   load_kernel
 *   enter_protected_mode (starts as 16-bit, switches)
 */

/* ... (functions defined earlier in this document) ... */


/* ============================================ */
/* Protected Mode Code (32-bit)                 */
/* ============================================ */

.code32

/*
 * protected_mode_entry - 32-bit code after mode switch
 *
 * At this point CS is loaded with CODE_SEG from the far jump.
 * We need to reload other segment registers and continue.
 */
protected_mode_entry:
    /* Reload segment registers */
    movw $DATA_SEG, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    /* Set up stack */
    movl $0x90000, %esp

    /* Copy kernel from 0x10000 to 0x100000 */
    call copy_kernel_to_1mb

    /* Jump to kernel */
    jmp jump_to_kernel

/* Include 32-bit functions:
 *   copy_kernel_to_1mb
 *   jump_to_kernel
 */

/* ... (functions defined earlier) ... */


/* ============================================ */
/* Data Section                                 */
/* ============================================ */

.section .data

/* Boot drive number (saved by stage 1, passed in DL) */
boot_drive: .byte 0

/* GDT and GDT pointer defined earlier */


/* ============================================ */
/* Constants                                    */
/* ============================================ */

.equ CODE_SEG, 0x08
.equ DATA_SEG, 0x10
.equ KERNEL_START_SECTOR, 5
.equ KERNEL_LOAD_SEG, 0x1000
.equ KERNEL_SECTORS, 128        /* 64KB = 128 sectors */
.equ KERNEL_SIZE_DWORDS, 16384  /* 64KB / 4 */
.equ KERNEL_ENTRY, 0x100000
.equ MMAP_ADDR, 0x500
.equ MMAP_COUNT_ADDR, 0x4FC
.equ E820_MAGIC, 0x534D4150
```

---

## Part 9: Disk Image Layout and Makefile

### Disk Layout

```
Offset (sectors)    Content                 Size
────────────────────────────────────────────────────────
0                   Stage 1 (MBR)           512 bytes (1 sector)
1-4                 Stage 2                 2KB (4 sectors)
5+                  Kernel                  Variable

Sector size = 512 bytes
```

### Makefile Updates

```makefile
# Disk image constants
STAGE2_SECTORS = 4
KERNEL_START_SECTOR = 5

# Build stage 2
$(BUILD)/boot/stage2.bin: boot/stage2.S
    $(AS) -o $(BUILD)/boot/stage2.o boot/stage2.S
    $(LD) -Ttext 0x7E00 --oformat binary -o $@ $(BUILD)/boot/stage2.o
    @# Verify size (should be <= 2048 bytes for 4 sectors)
    @size=$$(stat -c%s $@); \
    if [ $$size -gt 2048 ]; then \
        echo "ERROR: stage2.bin too large ($$size > 2048)"; exit 1; \
    fi

# Build kernel
$(BUILD)/kernel/kernel.bin: $(KERNEL_OBJS) scripts/kernel.ld
    $(LD) -T scripts/kernel.ld -o $(BUILD)/kernel/kernel.elf $(KERNEL_OBJS)
    $(OBJCOPY) -O binary $(BUILD)/kernel/kernel.elf $@

# Assemble disk image
$(BUILD)/os-dev.img: $(BUILD)/boot/stage1.bin $(BUILD)/boot/stage2.bin $(BUILD)/kernel/kernel.bin
    # Create empty image
    dd if=/dev/zero of=$@ bs=512 count=2880
    # Write stage 1 to sector 0
    dd if=$(BUILD)/boot/stage1.bin of=$@ bs=512 conv=notrunc
    # Write stage 2 to sectors 1-4
    dd if=$(BUILD)/boot/stage2.bin of=$@ bs=512 seek=1 conv=notrunc
    # Write kernel starting at sector 5
    dd if=$(BUILD)/kernel/kernel.bin of=$@ bs=512 seek=$(KERNEL_START_SECTOR) conv=notrunc
```

### Kernel Linker Script

```ld
/* scripts/kernel.ld */

/*
 * Kernel Linker Script
 *
 * This tells the linker how to arrange kernel sections in memory.
 * The kernel loads at physical address 0x100000 (1MB).
 *
 * Note: In Story 3.2, we'll add a virtual address at 0xC0100000
 * for the higher-half kernel mapping.
 */

ENTRY(_start)

SECTIONS
{
    /* Physical load address */
    . = 0x100000;

    /* Code section - entry point first */
    .text : {
        *(.text.boot)   /* Entry point - must be first! */
        *(.text)        /* All other code */
    }

    /* Read-only data */
    .rodata : {
        *(.rodata)
    }

    /* Initialized data */
    .data : {
        *(.data)
    }

    /* Uninitialized data (BSS) */
    .bss : {
        __bss_start = .;
        *(.bss)
        *(COMMON)
        __bss_end = .;
    }

    /* End of kernel */
    _kernel_end = .;
}
```

---

## Part 10: Testing and Verification

### Test Checklist

| Test | How to Verify | Expected Result |
|------|---------------|-----------------|
| Stage 2 starts | Watch QEMU output | '2' printed |
| A20 enabled | Watch QEMU output | 'A' printed after '2' |
| Kernel loaded | Watch QEMU output | 'L' printed after 'A' |
| Protected mode works | Kernel runs | 'K' appears at top-left of screen |
| Memory map | Debug with GDB | Values at 0x500 (MMAP_ADDR) |

### Verification Commands

```bash
# Build and run
make qemu

# Expected output: "2AL" then 'K' on screen

# Check binary sizes
ls -la build/boot/stage1.bin build/boot/stage2.bin build/kernel/kernel.bin

# Examine disk image layout
hexdump -C build/os-dev.img | head -50   # Stage 1
hexdump -C build/os-dev.img -s 512 | head -20   # Stage 2 start
hexdump -C build/os-dev.img -s 2560 | head -20  # Kernel start (sector 5)

# Debug with GDB
make debug
# In another terminal:
gdb -x scripts/gdbinit
(gdb) target remote :1234
(gdb) break *0x100000      # Break at kernel entry
(gdb) continue
```

### Common Issues and Solutions

| Issue | Symptom | Solution |
|-------|---------|----------|
| Triple fault after PM switch | Immediate reboot | Check GDT alignment, selector values |
| A20 not enabled | Crash accessing 1MB+ | Try both A20 methods, verify with check_a20 |
| Kernel not loading | No 'K' on screen | Check disk read, KERNEL_SECTORS constant |
| Far jump fails | Freeze after CR0 write | Verify ljmp syntax, CODE_SEG value |

---

## Project Structure Notes

### Files Created/Modified

**New Files:**
- `kernel/init/entry.S` - Kernel entry point
- `kernel/init/main.c` - kmain() function
- `scripts/kernel.ld` - Kernel linker script

**Modified Files:**
- `boot/stage2.S` - Full implementation (replace placeholder)
- `Makefile` - Add kernel build rules, update disk image

### Directory Structure After This Story

```
os-dev/
├── boot/
│   ├── stage1.S        (from Story 1.2)
│   └── stage2.S        (THIS STORY)
├── kernel/
│   ├── init/
│   │   ├── entry.S     (THIS STORY)
│   │   └── main.c      (THIS STORY)
│   └── include/
├── scripts/
│   └── kernel.ld       (THIS STORY)
├── Makefile            (modified)
└── config.mk           (from Story 1.1)
```

---

## References

- [Source: _bmad-output/planning-artifacts/architecture.md#Boot-→-Kernel]
- [Source: _bmad-output/planning-artifacts/epics.md#Story-1.3]
- [Source: _bmad-output/planning-artifacts/prd.md#FR3-FR4]
- [Source: _bmad-output/project-context.md#Critical-Assembly-Rules]
- [Source: Intel SDM Vol 3, Chapter 9 - Processor Management and Initialization]
- [Source: Intel SDM Vol 3, Chapter 3 - Protected-Mode Memory Management]
- [Source: OSDev Wiki - A20 Line]
- [Source: OSDev Wiki - Protected Mode]
- [Source: OSDev Wiki - GDT Tutorial]
- [Source: OSDev Wiki - Memory Map (x86)]

### Previous Story Intelligence

**From Story 1.2:**
- Stage 1 loads stage 2 to 0x7E00 and jumps there
- Boot drive number passed in DL register - must preserve!
- AT&T/GAS syntax (not NASM)
- Used retry logic with disk reset for reliability
- Comprehensive comments expected

**Git History:**
```
388efd3 feat[story 1.2]: stage 1 bootloader and MBR.
cce8748 feat[story 1.1]: project structure build system.
```

---

## Dev Agent Record

### Agent Model Used

Claude Opus 4.5 (claude-opus-4-5-20251101)

### Debug Log References

- QEMU execution trace verified boot sequence through protected mode transition
- CR0 register showed PE bit (0x01) being set during protected mode switch
- Kernel execution verified at address 0x100000 with VGA output and HLT

### Completion Notes List

1. **A20 Line Enablement**: Implemented both keyboard controller (port 0x64/0x60) and Fast A20 (port 0x92) methods with retry logic and verification routine
2. **GDT Setup**: Created 3-entry GDT (null, code, data) with flat 4GB segments at ring 0
3. **Protected Mode Switch**: Implemented CLI, LGDT, CR0.PE set, far jump to 32-bit code, segment register reload sequence
4. **Kernel Loading**: Loads kernel via BIOS INT 0x13 to 0x10000, copies to 0x100000 after PM switch (two-phase approach)
5. **Memory Map (E820)**: Queries BIOS for memory regions, stores at 0x504, passes count and pointer to kernel
6. **Kernel Entry**: Passes memory map info via EBX/ECX registers, sets up stack at 0x90000
7. **Kernel Stub**: Created entry.S with BSS clearing and kmain() call; main.c displays "OK" and memory map count on VGA
8. **Disk Image**: Updated Makefile with kernel at sector 5 (LBA), proper linking with entry.S first
9. **Testing**: Verified via QEMU execution trace - kernel runs at 0x100000, writes to VGA, halts successfully

### File List

**New Files:**
- `kernel/init/entry.S` - Kernel entry point assembly (BSS clearing, calls kmain)
- `kernel/test/test_boot.c` - In-kernel boot verification tests (A20, PM, GDT, segments, memory map)
- `kernel/include/test.h` - In-kernel test framework macros
- `kernel/test/test_runner.c` - Test harness orchestrating all kernel tests
- `kernel/test/test_example.c` - Example test demonstrating framework usage

**Modified Files:**
- `boot/stage1.S` - Updated STAGE2_SECTORS from 4 to 12 (stage2 grew larger)
- `boot/stage2.S` - Complete stage 2 implementation (A20, GDT, PM switch, kernel load, memory map); moved GDT to .rodata; updated disk layout constants for larger stage2
- `kernel/init/main.c` - Updated with VGA output showing "OK" and memory map count (handles 0-999); added TEST_MODE integration
- `scripts/kernel.ld` - Changed to physical address 0x100000, entry point _start, .text.boot section first
- `Makefile` - Added kernel assembly build, test build support (TEST_MODE=1), updated disk layout (kernel at sector 13)

### Change Log

- 2026-01-15: Code review fixes - GDT moved to .rodata, memory map display handles 3 digits, disk layout updated for larger stage2 (12 sectors), added in-kernel test framework with boot verification tests
- 2026-01-14: Story 1.3 implemented - Stage 2 bootloader with A20, GDT, protected mode switch, kernel loading, memory map, and kernel entry
