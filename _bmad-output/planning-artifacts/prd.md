---
stepsCompleted: ['step-01-init', 'step-02-discovery', 'step-03-success', 'step-04-journeys', 'step-05-domain', 'step-06-innovation', 'step-07-project-type', 'step-08-scoping', 'step-09-functional', 'step-10-nonfunctional', 'step-11-polish']
inputDocuments:
  - '_bmad-output/planning-artifacts/product-brief-os-dev-2026-01-11.md'
documentCounts:
  briefs: 1
  research: 0
  brainstorming: 0
  projectDocs: 0
classification:
  projectType: 'System Software / Educational OS'
  domain: 'Educational/Scientific'
  complexity: 'Medium'
  projectContext: 'Greenfield'
workflowType: 'prd'
projectName: 'os-dev'
author: 'Thomas'
date: '2026-01-12'
---

# Product Requirements Document - os-dev

**Author:** Thomas
**Date:** 2026-01-12

## Executive Summary

**os-dev** is an educational operating system built from scratch in C, targeting i386 architecture. The goal is not to build a production OS, but to build deep understanding of OS internals — specifically, to develop sufficient comprehension to read and understand the Linux kernel source code.

**Key Characteristics:**
- 12-milestone incremental build from boot to shell + text editor
- Linux-aligned naming conventions and architecture patterns
- Comprehensive debugging infrastructure (serial, GDB, panic handlers)
- Per-milestone documentation explaining "why", not just "how"

**Success Metric:** Can read Linux kernel mailing list discussions and follow the technical arguments.

## Success Criteria

### User Success

The primary user (Thomas) achieves success when:

1. **Comprehension Transfer** — Can read Linux kernel mailing list discussions and follow the technical arguments
2. **Pattern Recognition** — Opens Linux source (e.g., `fork()`, scheduler, page fault handler) and recognizes the same patterns implemented in os-dev
3. **Explanation Capability** — Can explain each subsystem well enough to re-implement it from scratch without reference

The learning milestones serve as checkpoints:
- Boot & Initialization: BIOS → bootloader → kernel handoff, GDT, real/protected mode
- Memory: Physical vs virtual, page tables, kernel mapping, page faults
- Processes: PCB structures, context switch, fork/exec
- System Calls: User/kernel boundary, syscall dispatch
- Interrupts & Drivers: IDT, interrupt routing, I/O methods
- File Systems: Inodes, directories, block allocation, read path

### Technical Success

1. **Functional Completeness** — All 12 project milestones achieved (boot → shell + editor)
2. **Code Quality** — Functions ≤30 lines, high test coverage, per-milestone documentation
3. **Architectural Integrity** — Clean separation of subsystems, Linux-aligned naming where sensible

### Measurable Outcomes

| Outcome | Validation |
|---------|------------|
| System boots to shell | QEMU cold start → prompt |
| Multi-process support | Shell + background task running simultaneously |
| Memory isolation | Processes have separate address spaces; page faults handled correctly |
| File persistence | Create file, reboot, file still exists |
| Linux comprehension | Can follow LKML thread on scheduler/memory topic |

## Product Scope

### MVP - Minimum Viable Product

- Custom bootloader (stage 1 + stage 2, protected mode transition)
- Kernel core (GDT/IDT, interrupts, physical memory manager, paging, heap)
- Process management (PCB, context switching, round-robin scheduler, user mode, fork/exec)
- System calls (int 0x80 dispatch, core syscalls: fork, exec, exit, wait, read, write, open, close, getpid)
- Drivers (VGA text, PS/2 keyboard, ATA PIO storage)
- File system (VFS layer, custom minimal FS with inodes/directories)
- Userspace (minimal libc, shell with cd/pwd/ls/echo/touch/mkdir, basic text editor)

### Growth Features (Post-MVP)

- Networking stack (simple TCP/IP)
- Pipes and signals
- FAT12 or ext2-lite filesystem implementation

### Vision (Future)

- 64-bit port (x86_64)
- SMP / multi-core support
- More complex scheduler (CFS-like)
- Shared libraries, graphics

## User Journeys

### Journey 1: Thomas — First Boot

**Opening Scene:**
Thomas stares at a black QEMU window. He's written a bootloader in assembly — 512 bytes that should load from the first sector of a virtual disk. The code looks right. The OSDev wiki says this should work. But the screen stays black.

**Rising Action:**
He adds a debug character write to VGA memory — nothing. He checks the BIOS boot sequence — correct. He hexdumps the disk image — the boot signature is there. He re-reads the x86 manual on real mode addressing. Then he spots it: a segment register he assumed was zero, but isn't.

**Climax:**
One line fix. He rebuilds, runs QEMU. A single 'H' appears in the top-left corner. Then 'e', 'l', 'l', 'o'. The machine did exactly what he told it to.

**Resolution:**
For the first time, he understands what "bare metal" actually means. There's no runtime, no OS, no safety net. Just instructions and hardware. The next milestone — protected mode — feels achievable now.

**Requirements Revealed:**
- Build system that produces bootable disk images
- QEMU integration for rapid iteration
- Clear milestone structure (each builds on previous)

---

### Journey 2: Thomas — The Debugging Spiral

**Opening Scene:**
Page faults. Everywhere. Thomas has just enabled paging, and the kernel triple-faults before printing anything useful. The screen flashes and QEMU resets. No stack trace, no error message — just silence.

**Rising Action:**
He enables QEMU's debug logging. He sees the faulting address: 0xC0100000. That's the kernel's virtual address — but the page tables aren't mapping it yet. Classic chicken-and-egg: the code that sets up paging is itself at a virtual address that requires paging.

He reads about identity mapping. He reads about higher-half kernels. He sketches page table layouts on paper. He rewrites the boot sequence to identity-map the first 4MB, enable paging, then jump to higher-half addresses.

**Climax:**
The kernel boots. It prints "Paging enabled." He manually walks the page tables in the debugger, translating a virtual address to physical, and it matches. He *understands* what the MMU is doing.

**Resolution:**
The next time he sees a page fault, he knows exactly where to look. More importantly, he now has a mental model for how Linux's `mm/` subsystem must work. The mystery is becoming mechanics.

**Requirements Revealed:**
- Robust debugging infrastructure (serial output, QEMU monitor)
- Per-milestone documentation explaining "why it works"
- Code structured to make debugging tractable (small functions, clear control flow)

---

### Journey 3: Thomas — The Linux Moment

**Opening Scene:**
Thomas opens `kernel/fork.c` in the Linux source. A year ago, this was hieroglyphics. Today, he's looking for how Linux handles copy-on-write for forked processes.

**Rising Action:**
He sees `copy_mm()`, `dup_mmap()`, `copy_page_range()`. The function names make sense. He traces into `copy_page_range()` and sees it manipulating page table entries, marking pages read-only, setting up the copy-on-write fault handler.

He thinks: "I did this. Not this exact code, but this exact pattern. I wrote a simpler version of this."

**Climax:**
He finds the page fault handler path that triggers the actual copy. He reads an LWN article about a proposed optimization to this code path. He follows the argument. He has an opinion about the tradeoff.

**Resolution:**
The goal wasn't to build a production OS. The goal was to build a mental model strong enough to read the real thing. Mission accomplished. He bookmarks the kernel mailing list.

**Requirements Revealed:**
- Linux-aligned naming conventions (where sensible)
- Architecture documentation mapping os-dev subsystems to Linux equivalents
- Code comments explaining "this is simplified; Linux does X instead because Y"

---

### Journey 4: Future Learner — Following the Path

**Opening Scene:**
A developer finds os-dev on GitHub. They've been frustrated by the same scattered resources Thomas once faced. The README promises "understand every line, from boot to shell."

**Rising Action:**
They clone the repo, run `make qemu`, and see a shell prompt. They open milestone 1's documentation: "Boot to Hello World." It explains exactly what happens from BIOS to kernel entry. The code is short enough to read in one sitting.

They follow milestone 2, then 3. Each one builds clearly on the last. When they hit a wall, the documentation explains the "why" not just the "how."

**Climax:**
By milestone 6, they're modifying the scheduler. They add a simple priority mechanism. It works. They understand *why* it works.

**Resolution:**
They open the Linux scheduler code. It's complex, but not alien. They recognize the patterns. They're ready to go deeper.

**Requirements Revealed:**
- Per-milestone documentation as first-class artifact
- Clean, readable code optimized for learning
- Progressive complexity (each milestone achievable given previous)
- Self-contained — can build and run with minimal setup

---

### Journey Requirements Summary

| Capability | Revealed By |
|------------|-------------|
| Bootable disk image build system | First Boot |
| QEMU integration with debug output | First Boot, Debugging Spiral |
| Serial/debug logging infrastructure | Debugging Spiral |
| Per-milestone documentation | Debugging Spiral, Future Learner |
| Linux-aligned naming conventions | Linux Moment |
| Architecture docs mapping to Linux | Linux Moment |
| Clean, small functions | Debugging Spiral, Future Learner |
| Progressive milestone structure | All journeys |

## Domain-Specific Requirements

### Architecture Conventions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Architecture | i386 (32-bit) | Simpler paging, abundant resources, concepts transfer to 64-bit |
| Intel compliance | Pragmatic | Follow conventions, take liberties where it aids learning |
| Bootloader | Custom only | No Multiboot/GRUB — understand boot process from scratch |
| Memory layout | Higher-half kernel (0xC0000000) | Most common pattern, maps to Linux approach |

### Toolchain & Environment

| Component | Choice |
|-----------|--------|
| Cross-compiler | i686-elf-gcc (standard) |
| Emulator | QEMU only |
| Real hardware | Not supported (keeps scope manageable) |

### Debugging Infrastructure

Comprehensive debugging is a first-class requirement:

- **Serial output** — Primary debug channel, persists across crashes
- **VGA text output** — Visual feedback during boot and runtime
- **GDB stub** — Full source-level debugging via QEMU's GDB server
- **QEMU monitor** — Memory inspection, register dumps, CPU state
- **Panic handler** — Capture state on fatal errors before halt

*Rationale: Poor debugging infrastructure kills learning momentum. When something breaks, you need visibility.*

### Code Conventions

| Convention | Approach |
|------------|----------|
| Naming | Linux-aligned as much as possible (`task_struct`, `fork()`, `mm_struct`, etc.) |
| Assembly | Minimize — use C wherever feasible, assembly only where unavoidable |
| Comments | Explain "why", note where Linux differs and why |

### Reference Standards

| Source | Usage |
|--------|-------|
| Intel SDM | Authoritative for CPU behavior, instructions, paging, interrupts |
| OSDev Wiki | Guidance, practical examples, common pitfalls |
| xv6 | Comparison point — similar educational goals, simpler design |
| Minix | Comparison point — microkernel perspective, Tanenbaum's approach |
| Linux | Reference implementation — "this is how the real thing does it" |

*When implementing a subsystem, document how os-dev's approach compares to these references.*

## System Software Specific Requirements

### Build System

| Aspect | Choice | Rationale |
|--------|--------|-----------|
| Build tool | GNU Make | Universal, well-understood, matches Linux kernel |
| Structure | Top-level Makefile + per-directory includes | `kernel/`, `boot/`, `libc/`, `user/` directories |
| Default build | Debug (symbols, -O0) | Learning requires debuggability |
| Release build | Optional (`make release`) | -O2, stripped symbols |
| Output | `build/` directory | Keep source tree clean |
| Disk image | `make image` → `os-dev.img` | Bootable raw disk image |
| Run target | `make qemu` | One command to build and run |
| Debug target | `make debug` | QEMU with GDB stub, waits for connection |

### Memory Model

**Kernel Space (0xC0000000 - 0xFFFFFFFF):**
```
0xFFFFFFFF ┌─────────────────────┐
           │ Reserved/MMIO       │
0xFFC00000 ├─────────────────────┤
           │ Kernel heap         │ (grows up)
           ├─────────────────────┤
           │ Kernel BSS/data     │
           ├─────────────────────┤
           │ Kernel code         │
0xC0100000 ├─────────────────────┤
           │ Kernel page tables  │
0xC0000000 └─────────────────────┘
```

**User Space (0x00000000 - 0xBFFFFFFF):**
```
0xBFFFFFFF ┌─────────────────────┐
           │ User stack          │ (grows down)
           ├─────────────────────┤
           │ (unmapped gap)      │
           ├─────────────────────┤
           │ Heap                │ (grows up via sbrk)
           ├─────────────────────┤
           │ BSS (uninitialized) │
           ├─────────────────────┤
           │ Data (initialized)  │
           ├─────────────────────┤
           │ Text (code)         │
0x08048000 ├─────────────────────┤
           │ (unmapped - null    │
           │  pointer trap)      │
0x00000000 └─────────────────────┘
```

*0x08048000 is traditional Linux ELF base address — keeps alignment with Linux conventions.*

### System Call Interface

| Aspect | Choice | Rationale |
|--------|--------|-----------|
| Mechanism | `int 0x80` | Simpler than sysenter, matches classic Linux |
| Registers | eax=syscall#, ebx/ecx/edx/esi/edi=args | Linux i386 convention |
| Return | eax=result or -errno | Linux convention |
| Numbering | Sequential (0, 1, 2...) | Simple, but documented by category |

**Initial syscall table:**
```
0: read       4: close      8: fork       12: getpid
1: write      5: stat       9: exec       13: getppid
2: open       6: fstat     10: exit       14: wait
3: lseek      7: brk       11: waitpid    15: ...
```

### Process Model

| Aspect | Choice | Rationale |
|--------|--------|-----------|
| Structure | `task_struct` | Linux-aligned naming |
| State | RUNNING, READY, BLOCKED, ZOMBIE | Standard states |
| Kernel stack | 4KB per process | Separate from user stack |
| Scheduler | Round-robin ready queue | Simple, correct, extensible interface |
| PID | Sequential allocation | Start at 1 (init), 0 reserved for idle |
| Context switch | Save/restore via `task_struct` | Full register state + page directory |

### Driver Model

| Aspect | Choice | Rationale |
|--------|--------|-----------|
| Abstraction | Function pointer tables | Simple VTable-style interface |
| Registration | Static initialization | No dynamic module loading for MVP |
| I/O model | Blocking only | Async/interrupt-driven internally, blocking API |

**Driver interface (example):**
```c
struct device_ops {
    int (*read)(struct device *dev, void *buf, size_t len);
    int (*write)(struct device *dev, const void *buf, size_t len);
    int (*ioctl)(struct device *dev, int cmd, void *arg);
};
```

### File System Model

| Aspect | Choice | Rationale |
|--------|--------|-----------|
| Abstraction | VFS with inode-based design | Matches Linux, allows future FS implementations |
| File descriptors | Per-process FD table → global file table → inodes | Standard Unix model |
| Initial FS | Custom minimal FS | Simple enough to understand completely |
| Block size | 1024 bytes | Balance between overhead and fragmentation |
| Max file size | ~16MB (direct + indirect blocks) | Sufficient for MVP |

**VFS layer:**
```c
struct inode_ops {
    int (*lookup)(struct inode *dir, const char *name, struct inode **result);
    int (*create)(struct inode *dir, const char *name, int mode);
    int (*read)(struct inode *inode, void *buf, size_t len, off_t offset);
    int (*write)(struct inode *inode, const void *buf, size_t len, off_t offset);
};
```

### Implementation Considerations

- **Simplicity over optimization** — Correct and clear first, optimize only if needed
- **One way to do things** — Avoid multiple code paths for the same operation
- **Fail loudly** — Panic on unexpected states during development
- **Linux mapping** — Document which Linux files implement equivalent functionality

## Project Scoping & Phased Development

### MVP Strategy & Philosophy

**MVP Approach:** Learning Validation MVP

For os-dev, "MVP" means the minimum implementation that proves end-to-end OS understanding — from bare metal boot to running a shell that can create files. This isn't market validation; it's **learning validation**: "Do I understand enough to build a working system?"

**MVP Completion Criteria:**
- All 12 project milestones achieved
- Shell runs with cd, pwd, ls, echo, touch, mkdir
- Basic text editor functional
- Can explain each subsystem well enough to re-implement

### Milestone-Based Development

The 12 milestones provide natural phase gates:

| # | Milestone | Subsystem Proven |
|---|-----------|------------------|
| 1 | Boot to "Hello World" | Bootloader, kernel entry, VGA |
| 2 | Keyboard input echoed | IDT, interrupt handling, drivers |
| 3 | Physical memory manager | Memory layout, allocation |
| 4 | Paging enabled | Page tables, virtual memory |
| 5 | First kernel thread | Context structures, stack setup |
| 6 | Multiple threads + scheduler | Context switching, scheduling |
| 7 | User mode process | Ring transitions, address spaces |
| 8 | System calls working | User/kernel boundary, dispatch |
| 9 | fork() and multiple processes | Process creation, memory copy |
| 10 | Basic file system | Storage driver, inodes, directories |
| 11 | Shell (cd/pwd/ls/echo/touch/mkdir) | End-to-end integration |
| 12 | Basic text editor | File I/O, complex interaction |

### Risk Mitigation Strategy

| Risk | Category | Mitigation |
|------|----------|------------|
| Paging/VM complexity | Technical | Comprehensive debugging; identity-map first, then higher-half |
| Context switch correctness | Technical | Single-step with GDB; compare with xv6 implementation |
| Stuck on hard problem | Motivation | Milestone structure ensures regular wins |
| Scope creep | Scope | Strict MVP boundary — shell + editor is done, everything else is Phase 2+ |
| Losing context between sessions | Process | Per-milestone documentation captures understanding |

### Phase Boundaries

**Phase 1 (MVP):** Milestones 1-12 — Boot to shell + editor
**Phase 2 (Growth):** Pipes, signals, networking, FAT12/ext2
**Phase 3 (Vision):** 64-bit port, SMP, CFS scheduler, shared libraries

*Each phase boundary is a natural stopping point where the system works and learning is validated.*

## Functional Requirements

### Boot & Initialization

- **FR1:** System can boot from a raw disk image in QEMU
- **FR2:** Bootloader can load kernel from disk into memory
- **FR3:** Bootloader can transition CPU from real mode to protected mode
- **FR4:** Bootloader can enable the A20 line for full memory access
- **FR5:** Kernel can initialize the Global Descriptor Table (GDT)
- **FR6:** Kernel can initialize the Interrupt Descriptor Table (IDT)
- **FR7:** Kernel can set up initial page tables and enable paging
- **FR8:** Kernel can display text output to VGA during boot

### Memory Management

- **FR9:** Kernel can track physical memory allocation via bitmap or free list
- **FR10:** Kernel can allocate and free physical page frames
- **FR11:** Kernel can create and manage page directories and page tables
- **FR12:** Kernel can map virtual addresses to physical addresses
- **FR13:** Kernel can handle page faults and report faulting address
- **FR14:** Kernel can allocate kernel heap memory dynamically
- **FR15:** Kernel can provide separate virtual address spaces per process

### Process Management

- **FR16:** Kernel can create kernel threads with separate stacks
- **FR17:** Kernel can maintain process state in task_struct
- **FR18:** Kernel can perform context switches between processes
- **FR19:** Kernel can schedule processes using round-robin algorithm
- **FR20:** Kernel can transition processes to user mode (ring 3)
- **FR21:** Kernel can create new processes via fork()
- **FR22:** Kernel can replace process image via exec()
- **FR23:** Kernel can terminate processes via exit()
- **FR24:** Kernel can wait for child process termination via wait/waitpid()
- **FR25:** Kernel can track process hierarchy (parent/child relationships)

### System Call Interface

- **FR26:** User programs can invoke kernel services via int 0x80
- **FR27:** Kernel can dispatch system calls based on syscall number
- **FR28:** Kernel can pass arguments from user registers to syscall handlers
- **FR29:** Kernel can return results and error codes to user programs
- **FR30:** System calls include: read, write, open, close, lseek, stat, fstat, brk, fork, exec, exit, wait, waitpid, getpid, getppid

### Device I/O

- **FR31:** Kernel can handle timer interrupts for preemptive scheduling
- **FR32:** Kernel can receive and process keyboard interrupts
- **FR33:** Kernel can read keyboard input and buffer keystrokes
- **FR34:** Kernel can write characters to VGA text mode display
- **FR35:** Kernel can read and write disk sectors via ATA PIO
- **FR36:** Kernel can output debug messages to serial port

### File System

- **FR37:** Kernel can mount a file system at boot
- **FR38:** Kernel can represent files and directories as inodes
- **FR39:** Kernel can look up files by path (path traversal)
- **FR40:** Kernel can create new files and directories
- **FR41:** Kernel can read file contents into user buffers
- **FR42:** Kernel can write user buffers to file contents
- **FR43:** Kernel can delete files and directories
- **FR44:** Kernel can list directory contents
- **FR45:** Kernel can manage file descriptors per process
- **FR46:** File data can persist across system reboots

### Shell & Userspace

- **FR47:** Shell can display a command prompt and read user input
- **FR48:** Shell can parse and execute built-in commands
- **FR49:** Shell can execute external programs by path
- **FR50:** Shell supports cd command (change directory)
- **FR51:** Shell supports pwd command (print working directory)
- **FR52:** Shell supports ls command (list directory contents)
- **FR53:** Shell supports echo command (print arguments)
- **FR54:** Shell supports touch command (create empty file)
- **FR55:** Shell supports mkdir command (create directory)
- **FR56:** Text editor can open and display file contents
- **FR57:** Text editor can insert and delete text
- **FR58:** Text editor can save changes to file
- **FR59:** Minimal libc provides syscall wrappers for user programs
- **FR60:** Minimal libc provides basic string functions (strlen, strcpy, strcmp, etc.)
- **FR61:** Minimal libc provides basic printf-style output

### Development & Debugging

- **FR62:** Build system can compile all sources with single command (make)
- **FR63:** Build system can produce bootable disk image (make image)
- **FR64:** Build system can launch QEMU with disk image (make qemu)
- **FR65:** Build system can launch QEMU with GDB debugging (make debug)
- **FR66:** Kernel can output debug logs to serial port
- **FR67:** Kernel can display panic message with register state on fatal error
- **FR68:** System can be debugged with GDB via QEMU's GDB stub

## Non-Functional Requirements

### Code Quality

- **NFR1:** Functions shall not exceed 30 lines (rare exceptions documented)
- **NFR2:** Each source file shall have a header comment explaining its purpose
- **NFR3:** Complex logic shall have inline comments explaining "why", not "what"
- **NFR4:** Naming shall follow Linux kernel conventions where applicable
- **NFR5:** Assembly code shall be minimized; C preferred where feasible
- **NFR6:** No "magic numbers" — constants shall be named and documented

### Debuggability

- **NFR7:** All kernel panics shall display register state and stack trace
- **NFR8:** Serial debug output shall be available from earliest boot stage
- **NFR9:** GDB debugging shall work for both kernel and user code
- **NFR10:** Page faults shall report faulting address and access type
- **NFR11:** Assertions shall be available for development builds

### Build & Iteration

- **NFR12:** Full rebuild shall complete in under 30 seconds
- **NFR13:** Incremental builds shall complete in under 5 seconds
- **NFR14:** `make qemu` shall build and boot in single command
- **NFR15:** Build shall work on Linux host with standard toolchain
- **NFR16:** No external dependencies beyond cross-compiler and QEMU

### Correctness

- **NFR17:** System shall boot reliably (no intermittent failures)
- **NFR18:** Memory allocator shall not leak or corrupt memory
- **NFR19:** Context switches shall preserve all register state
- **NFR20:** File system shall not corrupt data on normal shutdown
- **NFR21:** System calls shall return correct error codes on failure

### Documentation

- **NFR22:** Each milestone shall have accompanying technical documentation
- **NFR23:** Documentation shall explain design decisions, not just implementation
- **NFR24:** Code comments shall note where Linux differs and why
- **NFR25:** Architecture overview shall map subsystems to Linux equivalents
