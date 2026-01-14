---
stepsCompleted: [1, 2, 3, 4, 5]
inputDocuments: []
date: 2026-01-11
author: Thomas
projectName: os-dev
---

# Product Brief: os-dev

## Executive Summary

**os-dev** is an educational operating system built from scratch in C, designed to provide a progressive, hands-on learning path through OS internals. Unlike existing educational resources that either present scattered information or complete systems without clear learning paths, os-dev is built incrementally — each component minimal enough to understand every line of code, yet architected for future extension. The end goal: gain sufficient depth to navigate the Linux kernel codebase and comprehend kernel development articles.

---

## Core Vision

### Problem Statement

Developers seeking to truly understand operating system internals face a fragmented learning landscape. Existing resources fall into three frustrating categories: scattered wikis requiring constant cross-referencing, complete educational OSes that present everything at once with no clear entry point, or textbooks that explain concepts without providing followable implementations.

### Problem Impact

Without a clear, incremental path from "blank screen" to "working system," learners either:
- Give up due to overwhelming complexity
- Cargo-cult code they don't fully understand
- Remain permanently at the conceptual level, unable to read real kernel code

### Why Existing Solutions Fall Short

| Resource | Limitation |
|----------|------------|
| **OSDev Wiki** | Scattered, hard to navigate, no structured progression |
| **xv6** | Complete system with no entry point; difficult to even boot |
| **Minix Book** | Concept-heavy; implementation details hard to follow |

### Proposed Solution

Build a minimal operating system in C, bootable in QEMU, implementing:
- **Virtual memory** with paging
- **Process scheduling** (single-core, simple algorithm)
- **Basic file system** for read/write operations
- **System calls** for userspace programs
- **Simple drivers** (keyboard, display, storage)

**v1.0 Target:** Boot to a shell supporting `cd`, `pwd`, `ls`, `echo`, `touch`, plus a basic text editor — proving end-to-end understanding from hardware initialization to file I/O.

### Key Differentiators

1. **Incremental construction** — each milestone works before building the next
2. **"Understand every line" philosophy** — no magic, no cargo-culting
3. **Simple-first, extensible architecture** — clean code that can grow
4. **Linux comprehension as the goal** — structured to map to real kernel subsystems (scheduler → memory → filesystem → drivers)

---

## Target Users

### Primary User

**Thomas — The Curious Systems Explorer**

- **Background:** Working developer with backend/devops/blockchain experience. Has built a smart contract compiler and worked with databases, so systems programming concepts aren't foreign — but OS internals are unexplored territory.
- **Motivation:** Pure curiosity. Wants to understand how his own machine actually works. End goal: read Linux kernel code and understand kernel hacking articles.
- **Learning Style:** Incremental builder. Needs to understand every line of code to the point of being able to re-implement it. No magic, no cargo-culting.
- **Pain Points:** Existing resources are either scattered (OSDev), overwhelming (xv6), or too conceptual (Minix book).
- **Success Criteria:** Can boot os-dev, run shell commands, create files — and then open Linux source code and recognize patterns.

### Secondary Users (Potential)

**Future Learners — If Open-Sourced**

- Developers with similar backgrounds who want a structured, incremental path into OS development
- Would benefit from the tutorial-style progression and clean, documented code
- os-dev could become a teaching resource: "Build an OS from scratch, understand Linux"

### Code Quality as User Need

Since Thomas (and future learners) will revisit code constantly, the codebase itself must be user-friendly:

| Principle | Implementation |
|-----------|----------------|
| **Modular architecture** | Clear separation of subsystems |
| **Smart comments** | Explain *why*, not *what* |
| **Small functions** | But not over-abstracted |
| **Easy navigation** | Logical file structure |
| **Architecture docs** | High-level maps, magic number explanations |
| **Linux-aligned naming** | Syscall names, key functions/variables match Linux conventions where sensible |

### User Journey

1. **Discovery:** Thomas decides to learn OS internals, finds existing resources frustrating
2. **Onboarding:** Boots first "Hello World" kernel in QEMU — proof of life
3. **Core Loop:** Implement feature → understand every line → see it work → map to Linux concepts
4. **Aha Moment:** Opens Linux scheduler code and thinks "I know what this does"
5. **Long-term:** Comfortable reading kernel articles, considering contributions

---

## Success Metrics

### Architecture Decision

**Target: 32-bit (i386)** — Chosen for simpler paging (2-level), abundant OSDev resources, xv6 comparability, and cleaner learning path. Concepts transfer directly to 64-bit for future exploration.

### Learning Milestones

**Boot & Initialization**
- "I can explain how BIOS finds and loads the bootloader"
- "I understand real mode vs protected mode and why we transition"
- "I can trace exactly how the bootloader hands control to my kernel"
- "I understand the GDT and why it exists"

**Memory Management**
- "I understand physical vs virtual addresses and why both exist"
- "I can explain page tables and walk a virtual→physical translation by hand"
- "I understand why the kernel is mapped into every process's address space"
- "I can explain how a simple physical memory allocator works"
- "I understand what a page fault is and how the kernel handles it"

**Processes & Scheduling**
- "I understand that a process is just data structures + address space"
- "I can explain exactly what happens during a context switch"
- "I understand how the scheduler decides what runs next"
- "I can explain how `fork()` creates a new process"
- "I understand how `exec()` replaces a process image"

**System Calls**
- "I understand the user/kernel mode boundary and why it exists"
- "I can trace a syscall from user code through the interrupt to kernel handler and back"
- "I understand why syscalls are the *only* way userspace talks to kernel"

**Interrupts & Drivers**
- "I understand the difference between interrupts and polling"
- "I can explain how the IDT routes interrupts to handlers"
- "I understand memory-mapped I/O vs port I/O"
- "I can trace a keypress from hardware interrupt to character on screen"

**File Systems**
- "I understand what an inode is and why files need metadata separate from data"
- "I can explain how directories are just special files containing name→inode mappings"
- "I understand how disk blocks get allocated to files"
- "I can trace a `read()` call from syscall to disk blocks to user buffer"

**The Linux Connection**
- "I can follow an LWN article about the scheduler and understand the tradeoffs"
- "I can read a kernel mailing list thread about memory management and follow the debate"
- "I can open Linux's `fork()` implementation and identify the key steps"

### Project Milestones

| # | Milestone | Proves |
|---|-----------|--------|
| 1 | Boot to "Hello World" | Bootloader, kernel entry, VGA text output |
| 2 | Keyboard input echoed to screen | IDT, interrupt handling, basic drivers |
| 3 | Physical memory manager | Memory layout, allocation |
| 4 | Paging enabled | Page tables, virtual memory |
| 5 | First kernel thread | Context structures, stack setup |
| 6 | Multiple threads + scheduler | Context switching, scheduling algorithm |
| 7 | User mode process running | Ring transitions, separate address spaces |
| 8 | System calls working | User/kernel boundary, syscall dispatch |
| 9 | `fork()` and multiple processes | Process creation, memory copying |
| 10 | Basic file system | Storage driver, inodes, directories |
| 11 | Shell with `cd`, `pwd`, `ls`, `echo`, `touch` | End-to-end integration |
| 12 | Basic text editor | File I/O, complex user interaction |

### Code Quality Metrics

| Metric | Target |
|--------|--------|
| **Function length** | ≤30 lines (rare exceptions documented with rationale) |
| **Test coverage** | High coverage for core subsystems |
| **Test quality** | Well-factored, readable, educational — learn by reading tests |
| **Documentation** | Per-milestone technical doc: file structure, API design, key concepts |

---

## MVP Scope

### Core Features

**Bootloader (Custom)**
- Stage 1: BIOS loads from disk, sets up stack, loads stage 2
- Stage 2: Enable A20, switch to protected mode, load kernel, jump to C code

**Kernel Core**
- GDT/IDT setup
- Interrupt handling (timer, keyboard, exceptions)
- Physical memory manager (bitmap or simple allocator)
- Virtual memory with 2-level paging
- Kernel heap allocator

**Process Management**
- Process/thread data structures (PCB)
- Context switching
- Round-robin scheduler (interface designed for future algorithms)
- User mode support (ring 3)
- `fork()` and `exec()` implementation

**System Calls**
- Syscall dispatch mechanism (int 0x80 or similar)
- Core syscalls: `fork`, `exec`, `exit`, `wait`, `read`, `write`, `open`, `close`, `getpid`, etc.
- Linux-compatible naming where sensible

**Drivers**
- VGA text mode driver
- PS/2 keyboard driver
- Storage driver (ATA PIO for simplicity)

**File System**
- VFS layer (abstraction for pluggable FS implementations)
- Custom minimal FS (inodes, directories, file data blocks)
- Designed to make FAT12/ext2 implementation straightforward later

**Userspace**
- Minimal custom libc (basic syscall wrappers, string functions, printf-lite)
- Shell with: `cd`, `pwd`, `ls`, `echo`, `touch`
- Basic text editor

### Out of Scope for MVP

| Feature | Rationale |
|---------|-----------|
| Multi-core / SMP | Complexity; single-core teaches core concepts |
| GPU / Graphics | VGA text sufficient for learning |
| Networking | Large scope; deferred to future |
| Advanced schedulers | Round-robin first; interface allows future additions |
| Sound | Not essential for core OS understanding |
| Shared libraries / dynamic loading | Static linking sufficient for MVP |
| Full POSIX compliance | Linux-like naming, not certification |
| Signals (full implementation) | Could add basic version, but not required |
| Pipes / IPC beyond basic | Deferred complexity |

### MVP Success Criteria

1. **Boot Success:** QEMU boots to shell prompt from cold start
2. **Process Proof:** Can run multiple processes simultaneously (e.g., shell + background task)
3. **Memory Proof:** Processes have isolated address spaces; page faults handled
4. **File Proof:** Create file with editor, `ls` shows it, content persists across reboot (if FS on disk)
5. **Code Quality:** Functions ≤30 lines, tests pass, per-milestone documentation complete
6. **Learning Validation:** Can explain each subsystem well enough to re-implement from scratch

### Future Vision (Post-MVP)

| Phase | Features |
|-------|----------|
| **v1.1** | Networking stack (simple TCP/IP), pipes, signals |
| **v1.2** | FAT12 or ext2-lite file system implementation |
| **v2.0** | 64-bit port (apply all learnings to x86_64) |
| **v2.1** | SMP support (multi-core) |
| **Beyond** | More complex scheduler (CFS-like), shared libraries, graphics |
