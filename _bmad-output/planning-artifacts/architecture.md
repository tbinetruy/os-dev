---
stepsCompleted: [1, 2, 3, 4, 5, 6, 7, 8]
status: 'complete'
completedAt: '2026-01-12'
inputDocuments:
  - '_bmad-output/planning-artifacts/prd.md'
  - '_bmad-output/planning-artifacts/product-brief-os-dev-2026-01-11.md'
workflowType: 'architecture'
project_name: 'os-dev'
user_name: 'Thomas'
date: '2026-01-12'
---

# Architecture Decision Document

_This document builds collaboratively through step-by-step discovery. Sections are appended as we work through each architectural decision together._

## Project Context Analysis

### Requirements Overview

**Functional Requirements:**
68 requirements across 8 subsystems, each building on the previous:

| Category | Count | Architectural Scope |
|----------|-------|---------------------|
| Boot & Initialization | 8 | Bootloader, GDT/IDT, paging setup, VGA output |
| Memory Management | 7 | Physical allocator, page tables, virtual address spaces, heap |
| Process Management | 10 | task_struct, context switch, scheduler, fork/exec/exit/wait |
| System Call Interface | 5 | int 0x80 dispatch, argument passing, error returns |
| Device I/O | 6 | Timer, keyboard, VGA, ATA PIO, serial |
| File System | 10 | VFS layer, inodes, directories, file descriptors, persistence |
| Shell & Userspace | 15 | Shell commands, text editor, minimal libc |
| Development & Debugging | 7 | Build system, QEMU integration, GDB, panic handlers |

**Non-Functional Requirements:**
25 requirements establishing quality gates:

- **Code Quality (6):** Function size limits, naming conventions, minimal assembly, no magic numbers
- **Debuggability (5):** Panic handlers, serial output, GDB support, page fault reporting
- **Build & Iteration (5):** Sub-30s builds, single-command workflows, Linux host compatibility
- **Correctness (5):** Reliable boot, no memory leaks, correct context switches, data integrity
- **Documentation (4):** Per-milestone docs, design rationale, Linux comparison notes

**Scale & Complexity:**

- Primary domain: System Software / Educational OS
- Complexity level: Medium
- Estimated architectural components: 12 (matching milestone structure)
- Target platform: i386 (32-bit), QEMU only

### Technical Constraints & Dependencies

| Constraint | Impact |
|------------|--------|
| i386 architecture | 2-level paging, 32-bit addresses, specific register conventions |
| Custom bootloader only | No Multiboot, must implement real→protected mode transition |
| Higher-half kernel (0xC0000000) | Identity mapping required during boot, kernel mapped into all address spaces |
| QEMU only | No real hardware quirks, but must work with QEMU's device emulation |
| No external runtime | Everything from scratch — no libc, no existing allocators |
| Cross-compiler required | i686-elf-gcc toolchain, separate from host compiler |

### Cross-Cutting Concerns Identified

1. **Debug Infrastructure** — Serial logging, panic handlers, GDB integration must be available from earliest boot and work across all subsystems

2. **Memory Management** — Physical allocator, page tables, and heap are foundational; most subsystems depend on memory allocation

3. **Error Handling Strategy** — Need consistent approach: when to panic vs return error codes, how to propagate errors through layers

4. **Linux Naming Alignment** — Consistent use of Linux conventions (task_struct, fork, inode, etc.) across entire codebase

5. **Documentation Integration** — Per-milestone docs require clear module boundaries and stable interfaces to document against

## Starter Template Evaluation

### Primary Technology Domain

Bare-metal system software (i386 educational OS) — no framework starters applicable.

### Approach Selected: xv6-Inspired Structure

**Rationale:**
- Proven organization for educational OS projects
- Linux-aligned naming (mm/, proc/, fs/) supports learning transfer goal
- Clear subsystem boundaries map to 12 milestones
- Non-recursive Make for fast, correct builds

### Project Structure

```
os-dev/
├── boot/                    # Bootloader (Milestone 1)
│   ├── stage1.S             # Stage 1: MBR, 512 bytes, loads stage2
│   ├── stage2.S             # Stage 2: A20, protected mode, load kernel
│   ├── boot.h               # Shared boot constants
│   └── boot.ld              # Bootloader linker script
│
├── kernel/                  # Kernel code (Milestones 1-10)
│   ├── init/                # Kernel entry and setup
│   │   ├── main.c           # kmain() - kernel entry point
│   │   ├── gdt.c            # Global Descriptor Table
│   │   ├── idt.c            # Interrupt Descriptor Table
│   │   └── pic.c            # Programmable Interrupt Controller
│   │
│   ├── mm/                  # Memory management (Milestones 3-4)
│   │   ├── pmm.c            # Physical memory manager (bitmap)
│   │   ├── vmm.c            # Virtual memory / paging
│   │   ├── heap.c           # Kernel heap allocator
│   │   └── page.c           # Page table operations
│   │
│   ├── proc/                # Process management (Milestones 5-9)
│   │   ├── proc.c           # Process creation, fork, exec
│   │   ├── sched.c          # Scheduler (round-robin)
│   │   ├── switch.S         # Context switch (assembly)
│   │   └── task.h           # task_struct definition
│   │
│   ├── syscall/             # System calls (Milestone 8)
│   │   ├── syscall.c        # int 0x80 dispatch
│   │   ├── sys_proc.c       # fork, exec, exit, wait, getpid
│   │   └── sys_fs.c         # read, write, open, close, lseek
│   │
│   ├── drivers/             # Device drivers (Milestones 1-2, 10)
│   │   ├── vga.c            # VGA text mode
│   │   ├── keyboard.c       # PS/2 keyboard
│   │   ├── serial.c         # Serial port (debug)
│   │   ├── timer.c          # PIT timer
│   │   └── ata.c            # ATA PIO disk driver
│   │
│   ├── fs/                  # File system (Milestone 10)
│   │   ├── vfs.c            # Virtual filesystem layer
│   │   ├── inode.c          # Inode operations
│   │   ├── file.c           # File descriptor table
│   │   └── osfs.c           # Custom minimal filesystem
│   │
│   ├── lib/                 # Kernel utility functions
│   │   ├── string.c         # memcpy, memset, strlen, etc.
│   │   ├── printf.c         # Kernel printk
│   │   └── panic.c          # Panic handler
│   │
│   └── include/             # Kernel headers
│       ├── kernel.h         # Common kernel definitions
│       ├── types.h          # uint32_t, size_t, etc.
│       └── asm.h            # Assembly helpers, port I/O
│
├── libc/                    # Minimal C library (Milestone 11)
│   ├── syscall.S            # Syscall stubs (int 0x80 wrappers)
│   ├── string.c             # User-space string functions
│   ├── stdio.c              # printf, puts
│   ├── stdlib.c             # malloc (via brk), exit
│   ├── crt0.S               # C runtime startup
│   └── include/             # Libc headers
│       ├── stdio.h
│       ├── stdlib.h
│       ├── string.h
│       └── unistd.h         # POSIX-like syscall declarations
│
├── user/                    # User programs (Milestones 11-12)
│   ├── shell/               # Shell implementation
│   │   └── shell.c
│   ├── edit/                # Text editor
│   │   └── edit.c
│   └── bin/                 # Simple utilities
│       ├── ls.c
│       ├── cat.c
│       └── echo.c
│
├── tools/                   # Host-side tools
│   └── mkfs.c               # Filesystem image creator
│
├── scripts/                 # Build support
│   ├── kernel.ld            # Kernel linker script
│   ├── user.ld              # User program linker script
│   └── gdbinit              # GDB initialization script
│
├── docs/                    # Per-milestone documentation
│   ├── milestone-01-boot.md
│   ├── milestone-02-interrupts.md
│   └── ...
│
├── build/                   # Build output (generated, gitignored)
│   ├── boot/
│   ├── kernel/
│   ├── libc/
│   ├── user/
│   └── os-dev.img           # Final bootable image
│
├── Makefile                 # Top-level Makefile
├── config.mk                # Build configuration (CC, flags, etc.)
└── README.md
```

### Build System

**Approach:** Non-recursive GNU Make with per-directory includes

```makefile
# Top-level Makefile structure
TOP := $(shell pwd)
BUILD := $(TOP)/build

include config.mk           # Cross-compiler, flags
include boot/build.mk       # Boot targets
include kernel/build.mk     # Kernel targets
include libc/build.mk       # Libc targets
include user/build.mk       # User program targets

all: $(BUILD)/os-dev.img

qemu: all
    qemu-system-i386 -drive file=$(BUILD)/os-dev.img,format=raw

debug: all
    qemu-system-i386 -s -S -drive file=$(BUILD)/os-dev.img,format=raw
```

**Key Properties:**
- Single dependency graph for correct parallel builds (`make -j`)
- Each `build.mk` adds to shared source variables
- No recursive make invocations
- Debug build by default (symbols, -O0)
- Targets: `make`, `make qemu`, `make debug`, `make image`, `make clean`

### Milestone-to-Directory Mapping

| Milestone | Primary Directories |
|-----------|---------------------|
| 1. Boot to Hello | boot/, kernel/init/, kernel/drivers/vga.c |
| 2. Keyboard input | kernel/init/idt.c, kernel/drivers/keyboard.c |
| 3. Physical memory | kernel/mm/pmm.c |
| 4. Paging | kernel/mm/vmm.c, kernel/mm/page.c |
| 5. First thread | kernel/proc/proc.c, kernel/proc/task.h |
| 6. Scheduler | kernel/proc/sched.c, kernel/proc/switch.S |
| 7. User mode | kernel/proc/, kernel/init/gdt.c (TSS) |
| 8. System calls | kernel/syscall/ |
| 9. fork() | kernel/proc/proc.c, kernel/mm/ |
| 10. File system | kernel/fs/, kernel/drivers/ata.c |
| 11. Shell | libc/, user/shell/ |
| 12. Editor | user/edit/ |

## Core Architectural Decisions

### Decision Summary

| Category | Decision | Rationale |
|----------|----------|-----------|
| Kernel Heap | Simple linked list allocator | Fully understandable, upgradeable interface |
| Physical Memory | Bitmap allocator | Simple to debug, visualize, matches PRD |
| Custom Filesystem | Superblock → bitmaps → inode table → data | Mirrors early Unix/ext2 concepts |
| Error Handling | Linux-style negative errno | Aligns with learning goal, compact |
| Logging | printk with 4 levels (ERROR/WARN/INFO/DEBUG) | Visibility without flooding |
| Testing | In-kernel tests + host-side unit tests | Fast feedback, proof of understanding |

### Memory Architecture

**Kernel Heap Allocator:**
- Algorithm: Simple linked list with first-fit allocation
- Interface: `kmalloc(size)` / `kfree(ptr)`
- Location: `kernel/mm/heap.c`
- Future: Interface supports upgrade to buddy/slab if needed

**Physical Memory Manager:**
- Algorithm: Bitmap — 1 bit per 4KB page frame
- Interface: `pmm_alloc_frame()` / `pmm_free_frame(addr)`
- Location: `kernel/mm/pmm.c`
- Debug: Bitmap can be dumped to visualize allocation state

### Filesystem Architecture

**On-Disk Layout:**
```
Block 0:        Superblock (magic, counts, pointers)
Blocks 1-N:     Inode bitmap + Block bitmap
Blocks N+1-M:   Inode table (fixed-size entries)
Blocks M+1-end: Data blocks
```

**Inode Structure:**
- Type: file or directory
- Size: file length in bytes
- Pointers: 12 direct + 1 indirect (~16MB max file)
- Timestamps: optional for MVP

**Directory Format:**
- Fixed-size entries: filename (max 28 chars) + inode number (4 bytes)
- Simple sequential scan for lookup

### Error Handling Strategy

**Panic Conditions (unrecoverable):**
- Kernel invariant violated (null pointers in critical structures)
- Corrupted page tables or GDT/IDT
- Double fault or unexpected CPU exception in kernel mode

**Error Return Conditions:**
- Resource exhaustion → `-ENOMEM`, `-EAGAIN`
- Invalid user input → `-EINVAL`, `-EFAULT`
- File not found → `-ENOENT`
- Permission denied → `-EACCES`

**Convention:** Functions return `int`, negative values are `-errno`, zero or positive is success/result.

### Logging Infrastructure

**Levels:**

| Level | Value | Usage |
|-------|-------|-------|
| LOG_ERROR | 0 | Failures requiring attention |
| LOG_WARN | 1 | Unexpected but handled |
| LOG_INFO | 2 | Significant events |
| LOG_DEBUG | 3 | Detailed tracing |

**Interface:**
```c
#define LOG_LEVEL LOG_DEBUG  // Compile-time filter
printk(LOG_INFO, "PMM: %d pages free\n", free_count);
```

**Output:** Serial (COM1) primary, VGA console secondary

### Testing Strategy

**In-Kernel Tests:**
- Each subsystem provides `test_<subsystem>()` function
- Called during boot when `make test` target used
- Output via serial: `[PASS] test_pmm_alloc` or `[FAIL] test_pmm_alloc: expected X got Y`

**Host-Side Unit Tests:**
- Pure C algorithms extracted to testable modules
- Compiled with host gcc, run natively
- Covers: bitmap operations, string functions, data structures

**Integration Tests (post-shell):**
- Shell scripts exercising syscalls
- Validates end-to-end behavior

## Implementation Patterns & Consistency Rules

### Pattern Summary

These patterns ensure consistent code across all milestones and prevent conflicts when working in different sessions.

| Area | Pattern |
|------|---------|
| Functions/variables | `snake_case` |
| Structs | `struct snake_case` |
| Constants/macros | `UPPER_SNAKE` |
| Header guards | `PATH_FILENAME_H` |
| Pointer style | `int *p` |
| Braces | K&R (function defs on new line) |
| Comments | `/* */` for docs, `//` for inline |
| Indentation | 4 spaces |
| Assembly | AT&T syntax |

### Naming Conventions

**C Code:**
```c
/* Functions and variables: snake_case */
uint32_t alloc_page_frame(void);
int page_count;

/* Structs: struct snake_case */
struct task_struct {
    pid_t pid;
    uint32_t *page_dir;
};

/* Constants and macros: UPPER_SNAKE */
#define PAGE_SIZE 4096
#define KERNEL_BASE 0xC0000000
```

**Assembly:**
```asm
/* Global labels: snake_case */
context_switch:
    pushl %ebp
    /* Local labels: .prefixed */
.save_registers:
    pushl %ebx
```

### Header Guard Pattern

Use full path to avoid collisions:

```c
/* kernel/mm/pmm.h */
#ifndef KERNEL_MM_PMM_H
#define KERNEL_MM_PMM_H
...
#endif /* KERNEL_MM_PMM_H */

/* kernel/proc/task.h */
#ifndef KERNEL_PROC_TASK_H
#define KERNEL_PROC_TASK_H
...
#endif /* KERNEL_PROC_TASK_H */
```

### Code Style

**Brace Placement (K&R with Linux exception):**
```c
/* Function definitions: brace on new line */
int pmm_alloc_frame(void)
{
    ...
}

/* Control structures: brace on same line */
if (frame == NULL) {
    return -ENOMEM;
}

for (int i = 0; i < count; i++) {
    process_frame(i);
}
```

**Pointer Declarations:**
```c
/* Correct: asterisk with variable */
int *ptr;
char *src, *dst;

/* Wrong: asterisk with type */
int* ptr;      /* Misleading with multiple declarations */
```

**Indentation:**
- 4 spaces (no tabs)
- Continuation lines indented 4 additional spaces

### Comment Conventions

**Function Documentation (Linux kernel style):**
```c
/*
 * Allocate a physical page frame from the free pool.
 *
 * Scans the bitmap for the first free frame, marks it
 * as allocated, and returns its physical address.
 *
 * Returns physical address on success, 0 if no frames available.
 * Note: Linux uses alloc_page() which returns struct page*.
 */
uint32_t pmm_alloc_frame(void)
{
    ...
}
```

**Inline Comments:**
```c
uint32_t flags = read_eflags();  // Save interrupt state
cli();                           // Disable interrupts
// Critical section
write_eflags(flags);             // Restore interrupt state
```

### Header File Structure

Standard order for all headers:

```c
#ifndef KERNEL_MM_PMM_H
#define KERNEL_MM_PMM_H

/* 1. Includes */
#include <types.h>

/* 2. Constants */
#define PAGE_SIZE       4096
#define PAGE_PRESENT    0x1
#define PAGE_WRITABLE   0x2

/* 3. Type definitions */
struct page_frame {
    uint32_t phys_addr;
    uint32_t flags;
};

/* 4. Function declarations */
void pmm_init(uint32_t mem_size);
uint32_t pmm_alloc_frame(void);
void pmm_free_frame(uint32_t phys_addr);

/* 5. Inline functions (if any) */
static inline int is_page_aligned(uint32_t addr)
{
    return (addr & (PAGE_SIZE - 1)) == 0;
}

#endif /* KERNEL_MM_PMM_H */
```

### Include Order

In `.c` files, order includes as:

```c
/* 1. Own header first (catches missing deps) */
#include "pmm.h"

/* 2. Kernel-wide headers */
#include <kernel.h>
#include <types.h>

/* 3. Subsystem headers */
#include <mm/page.h>
#include <lib/string.h>
```

### Assembly Conventions

**Syntax:** AT&T (gcc default)

**Documentation:** Every assembly function must document:
- Purpose
- Input registers
- Output registers
- Clobbered registers

```asm
/*
 * context_switch - Switch from current task to next task
 *
 * Input:  4(%esp) = pointer to current task_struct
 *         8(%esp) = pointer to next task_struct
 * Output: None (returns to new task's context)
 * Clobbers: All registers (full context switch)
 */
.global context_switch
context_switch:
    ...
```

### Enforcement Checklist

**All code MUST:**
- [ ] Use `snake_case` for functions, variables, struct names
- [ ] Use `UPPER_SNAKE` for constants and macros
- [ ] Use `PATH_FILENAME_H` header guards
- [ ] Place function opening braces on new line
- [ ] Use 4-space indentation (no tabs)
- [ ] Document functions with block comments
- [ ] Follow include ordering convention

## Project Structure & Boundaries

### Architectural Boundaries

| Boundary | Interface Location | Protocol |
|----------|-------------------|----------|
| Boot → Kernel | `kmain()` in `kernel/init/main.c` | Direct jump, defined register state |
| User → Kernel | `int 0x80` handler | Syscall ABI (eax=num, ebx/ecx/edx=args) |
| VFS → Filesystem | `struct inode_ops` | Function pointer vtable |
| Scheduler → Process | `struct task_struct` | State machine transitions |
| Kernel → Hardware | Driver functions | Port I/O or MMIO |

### Key Interface Headers

| Header | Purpose | Used By |
|--------|---------|---------|
| `kernel/include/types.h` | Base types | All kernel code |
| `kernel/include/errno.h` | Error codes | All subsystems |
| `kernel/proc/task.h` | Process structures | proc/, syscall/, sched/ |
| `kernel/fs/vfs.h` | VFS abstractions | fs/, syscall/ |
| `kernel/mm/mm.h` | Memory interfaces | mm/, proc/ |
| `kernel/include/syscall.h` | Syscall definitions | syscall/, libc/ |

### Subsystem Dependencies

```
                    ┌─────────┐
                    │  init   │
                    └────┬────┘
         ┌───────────────┼───────────────┐
         ↓               ↓               ↓
    ┌─────────┐    ┌─────────┐    ┌─────────┐
    │   mm    │←───│  proc   │───→│   fs    │
    └─────────┘    └─────────┘    └─────────┘
         ↑               ↑               ↑
         └───────────────┼───────────────┘
                         ↓
                   ┌─────────┐
                   │ drivers │
                   └─────────┘
```

**Dependency Rules:**
- `init/` depends on all subsystems (initialization order)
- `proc/` depends on `mm/` (address spaces)
- `fs/` depends on `drivers/` (storage)
- `syscall/` depends on `proc/`, `fs/`, `mm/`
- `drivers/` has no kernel dependencies (hardware interface)

### Milestone-to-File Mapping

| Milestone | Primary Files | New Interfaces |
|-----------|---------------|----------------|
| 1. Boot | `boot/*.S`, `kernel/init/main.c`, `kernel/drivers/vga.c` | `vga_putchar()` |
| 2. Interrupts | `kernel/init/idt.c`, `kernel/drivers/keyboard.c` | `idt_register()` |
| 3. PMM | `kernel/mm/pmm.c` | `pmm_alloc_frame()`, `pmm_free_frame()` |
| 4. Paging | `kernel/mm/vmm.c`, `kernel/mm/page.c` | `vmm_map_page()`, `vmm_create_address_space()` |
| 5. Threads | `kernel/proc/proc.c`, `kernel/proc/task.h` | `struct task_struct`, `thread_create()` |
| 6. Scheduler | `kernel/proc/sched.c`, `kernel/proc/switch.S` | `schedule()`, `context_switch()` |
| 7. User Mode | `kernel/init/gdt.c` (TSS), `kernel/proc/proc.c` | `enter_usermode()` |
| 8. Syscalls | `kernel/syscall/*.c` | `sys_read()`, `sys_write()`, etc. |
| 9. fork() | `kernel/proc/proc.c`, `kernel/mm/vmm.c` | `do_fork()`, `copy_address_space()` |
| 10. Filesystem | `kernel/fs/*.c`, `kernel/drivers/ata.c` | VFS ops, `ata_read_sector()` |
| 11. Shell | `libc/*.c`, `user/shell/shell.c` | libc functions |
| 12. Editor | `user/edit/edit.c` | (uses existing interfaces) |

### Data Flow Overview

```
┌─────────────────────────────────────────────────────────────┐
│                     User Space                               │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐                     │
│  │  shell  │  │  edit   │  │  ls/cat │                     │
│  └────┬────┘  └────┬────┘  └────┬────┘                     │
│       └────────────┼────────────┘                          │
│                    ↓                                        │
│              ┌─────────┐                                    │
│              │  libc   │                                    │
│              └────┬────┘                                    │
└───────────────────┼─────────────────────────────────────────┘
                    │ int 0x80
┌───────────────────┼─────────────────────────────────────────┐
│                   ↓           Kernel Space                   │
│            ┌──────────────┐                                 │
│            │   syscall    │                                 │
│            └──────┬───────┘                                 │
│       ┌───────────┼───────────┐                             │
│       ↓           ↓           ↓                             │
│  ┌────────┐  ┌────────┐  ┌────────┐                        │
│  │  proc  │  │   fs   │  │   mm   │                        │
│  └────┬───┘  └────┬───┘  └────┬───┘                        │
│       │           │           │                             │
│       ↓           ↓           ↓                             │
│  ┌────────────────────────────────────┐                    │
│  │            drivers                  │                    │
│  │  timer │ keyboard │ vga │ ata │ serial                  │
│  └────────────────────────────────────┘                    │
└─────────────────────────────────────────────────────────────┘
```

## Architecture Validation Results

### Coherence Validation ✅

**Decision Compatibility:** All decisions target i386 bare-metal C development with Linux-aligned conventions. No conflicts between technology choices.

**Pattern Consistency:** Implementation patterns (snake_case, K&R braces, 4-space indent) apply uniformly across kernel and userspace.

**Structure Alignment:** Project structure directly maps to 12 milestones with clear subsystem boundaries.

### Requirements Coverage ✅

**Functional Requirements:** All 68 FRs mapped to specific subsystems and files.

**Non-Functional Requirements:** All 25 NFRs addressed through patterns (code quality), infrastructure (debugging), and process (testing).

### Implementation Readiness ✅

**Decision Completeness:** All architectural decisions documented with rationale and examples.

**Pattern Completeness:** 10 implementation patterns covering all potential conflict points.

**Structure Completeness:** Full directory tree with interface headers and dependency rules.

### Architecture Completeness Checklist

**✅ Requirements Analysis**
- [x] Project context thoroughly analyzed
- [x] Scale and complexity assessed (Medium)
- [x] Technical constraints identified (i386, QEMU, no external runtime)
- [x] Cross-cutting concerns mapped (debug, memory, error handling)

**✅ Architectural Decisions**
- [x] Memory architecture (bitmap PMM, linked-list heap)
- [x] Filesystem architecture (superblock → bitmaps → inodes → data)
- [x] Error handling (Linux errno convention)
- [x] Logging infrastructure (printk with 4 levels)
- [x] Testing strategy (in-kernel + host-side)

**✅ Implementation Patterns**
- [x] Naming conventions (Linux kernel style)
- [x] Code style (K&R, 4 spaces, pointer style)
- [x] Header structure and guards
- [x] Comment and documentation conventions
- [x] Assembly conventions (AT&T syntax)

**✅ Project Structure**
- [x] Complete directory structure defined
- [x] Subsystem boundaries established
- [x] Interface headers identified
- [x] Milestone-to-file mapping complete

### Architecture Readiness Assessment

**Overall Status:** READY FOR IMPLEMENTATION

**Confidence Level:** High

**Key Strengths:**
- Linux-aligned patterns maximize learning transfer
- Clear milestone progression with defined deliverables
- Comprehensive debugging infrastructure from day one
- Simple algorithms (bitmap, linked-list) prioritize understanding

**First Implementation Step:**
Create project skeleton: directories, Makefile, config.mk, linker scripts — then Milestone 1 (boot to "Hello World")

## Architecture Completion Summary

### Workflow Completion

**Architecture Decision Workflow:** COMPLETED
**Total Steps Completed:** 8
**Date Completed:** 2026-01-12
**Document Location:** `_bmad-output/planning-artifacts/architecture.md`

### Final Architecture Deliverables

**Complete Architecture Document**
- All architectural decisions documented with rationale
- 10 implementation patterns ensuring code consistency
- Complete project structure with all files and directories
- Requirements to architecture mapping (68 FRs, 25 NFRs)
- Validation confirming coherence and completeness

**Implementation Ready Foundation**
- 6 core architectural decisions made (heap, PMM, filesystem, errors, logging, testing)
- 10 implementation patterns defined (naming, style, headers, comments, assembly)
- 12 milestone components specified with file mappings
- All 93 requirements fully supported architecturally

### Implementation Handoff

**For AI Agents:**
This architecture document is your complete guide for implementing os-dev. Follow all decisions, patterns, and structures exactly as documented.

**First Implementation Priority:**
1. Create project directory skeleton matching the defined structure
2. Set up `Makefile` and `config.mk` with cross-compiler configuration
3. Create `scripts/kernel.ld` linker script for higher-half kernel
4. Begin Milestone 1: Boot to "Hello World"

**Development Sequence:**
1. Initialize project structure per architecture document
2. Implement boot/ (stage1.S, stage2.S) — Milestone 1
3. Add kernel/init/ and kernel/drivers/vga.c — complete Milestone 1
4. Progress through milestones 2-12 following the defined mapping
5. Maintain consistency with documented patterns throughout

### Quality Assurance Checklist

**Architecture Coherence**
- [x] All decisions work together (C/i386/Linux-aligned)
- [x] Technology choices are compatible
- [x] Patterns support the architectural decisions
- [x] Structure aligns with all choices

**Requirements Coverage**
- [x] All 68 functional requirements are supported
- [x] All 25 non-functional requirements are addressed
- [x] Cross-cutting concerns are handled (debug, memory, errors)
- [x] Integration points are defined (boot→kernel, user→kernel, VFS→FS)

**Implementation Readiness**
- [x] Decisions are specific and actionable
- [x] Patterns prevent implementation conflicts
- [x] Structure is complete and unambiguous
- [x] Examples are provided for clarity

---

**Architecture Status:** READY FOR IMPLEMENTATION

**Next Phase:** Begin implementation using the architectural decisions and patterns documented herein.

