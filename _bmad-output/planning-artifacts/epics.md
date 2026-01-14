---
stepsCompleted: ['step-01-validate-prerequisites', 'step-02-design-epics', 'step-03-create-stories', 'step-04-final-validation']
status: 'complete'
completedAt: '2026-01-12'
inputDocuments:
  - '_bmad-output/planning-artifacts/prd.md'
  - '_bmad-output/planning-artifacts/architecture.md'
---

# os-dev - Epic Breakdown

## Overview

This document provides the complete epic and story breakdown for os-dev, decomposing the requirements from the PRD and Architecture into implementable stories.

## Requirements Inventory

### Functional Requirements

**Boot & Initialization (8 requirements)**
- FR1: System can boot from a raw disk image in QEMU
- FR2: Bootloader can load kernel from disk into memory
- FR3: Bootloader can transition CPU from real mode to protected mode
- FR4: Bootloader can enable the A20 line for full memory access
- FR5: Kernel can initialize the Global Descriptor Table (GDT)
- FR6: Kernel can initialize the Interrupt Descriptor Table (IDT)
- FR7: Kernel can set up initial page tables and enable paging
- FR8: Kernel can display text output to VGA during boot

**Memory Management (7 requirements)**
- FR9: Kernel can track physical memory allocation via bitmap or free list
- FR10: Kernel can allocate and free physical page frames
- FR11: Kernel can create and manage page directories and page tables
- FR12: Kernel can map virtual addresses to physical addresses
- FR13: Kernel can handle page faults and report faulting address
- FR14: Kernel can allocate kernel heap memory dynamically
- FR15: Kernel can provide separate virtual address spaces per process

**Process Management (10 requirements)**
- FR16: Kernel can create kernel threads with separate stacks
- FR17: Kernel can maintain process state in task_struct
- FR18: Kernel can perform context switches between processes
- FR19: Kernel can schedule processes using round-robin algorithm
- FR20: Kernel can transition processes to user mode (ring 3)
- FR21: Kernel can create new processes via fork()
- FR22: Kernel can replace process image via exec()
- FR23: Kernel can terminate processes via exit()
- FR24: Kernel can wait for child process termination via wait/waitpid()
- FR25: Kernel can track process hierarchy (parent/child relationships)

**System Call Interface (5 requirements)**
- FR26: User programs can invoke kernel services via int 0x80
- FR27: Kernel can dispatch system calls based on syscall number
- FR28: Kernel can pass arguments from user registers to syscall handlers
- FR29: Kernel can return results and error codes to user programs
- FR30: System calls include: read, write, open, close, lseek, stat, fstat, brk, fork, exec, exit, wait, waitpid, getpid, getppid

**Device I/O (6 requirements)**
- FR31: Kernel can handle timer interrupts for preemptive scheduling
- FR32: Kernel can receive and process keyboard interrupts
- FR33: Kernel can read keyboard input and buffer keystrokes
- FR34: Kernel can write characters to VGA text mode display
- FR35: Kernel can read and write disk sectors via ATA PIO
- FR36: Kernel can output debug messages to serial port

**File System (10 requirements)**
- FR37: Kernel can mount a file system at boot
- FR38: Kernel can represent files and directories as inodes
- FR39: Kernel can look up files by path (path traversal)
- FR40: Kernel can create new files and directories
- FR41: Kernel can read file contents into user buffers
- FR42: Kernel can write user buffers to file contents
- FR43: Kernel can delete files and directories
- FR44: Kernel can list directory contents
- FR45: Kernel can manage file descriptors per process
- FR46: File data can persist across system reboots

**Shell & Userspace (15 requirements)**
- FR47: Shell can display a command prompt and read user input
- FR48: Shell can parse and execute built-in commands
- FR49: Shell can execute external programs by path
- FR50: Shell supports cd command (change directory)
- FR51: Shell supports pwd command (print working directory)
- FR52: Shell supports ls command (list directory contents)
- FR53: Shell supports echo command (print arguments)
- FR54: Shell supports touch command (create empty file)
- FR55: Shell supports mkdir command (create directory)
- FR56: Text editor can open and display file contents
- FR57: Text editor can insert and delete text
- FR58: Text editor can save changes to file
- FR59: Minimal libc provides syscall wrappers for user programs
- FR60: Minimal libc provides basic string functions (strlen, strcpy, strcmp, etc.)
- FR61: Minimal libc provides basic printf-style output

**Development & Debugging (7 requirements)**
- FR62: Build system can compile all sources with single command (make)
- FR63: Build system can produce bootable disk image (make image)
- FR64: Build system can launch QEMU with disk image (make qemu)
- FR65: Build system can launch QEMU with GDB debugging (make debug)
- FR66: Kernel can output debug logs to serial port
- FR67: Kernel can display panic message with register state on fatal error
- FR68: System can be debugged with GDB via QEMU's GDB stub

### NonFunctional Requirements

**Code Quality (6 requirements)**
- NFR1: Functions shall not exceed 30 lines (rare exceptions documented)
- NFR2: Each source file shall have a header comment explaining its purpose
- NFR3: Complex logic shall have inline comments explaining "why", not "what"
- NFR4: Naming shall follow Linux kernel conventions where applicable
- NFR5: Assembly code shall be minimized; C preferred where feasible
- NFR6: No "magic numbers" — constants shall be named and documented

**Debuggability (5 requirements)**
- NFR7: All kernel panics shall display register state and stack trace
- NFR8: Serial debug output shall be available from earliest boot stage
- NFR9: GDB debugging shall work for both kernel and user code
- NFR10: Page faults shall report faulting address and access type
- NFR11: Assertions shall be available for development builds

**Build & Iteration (5 requirements)**
- NFR12: Full rebuild shall complete in under 30 seconds
- NFR13: Incremental builds shall complete in under 5 seconds
- NFR14: `make qemu` shall build and boot in single command
- NFR15: Build shall work on Linux host with standard toolchain
- NFR16: No external dependencies beyond cross-compiler and QEMU

**Correctness (5 requirements)**
- NFR17: System shall boot reliably (no intermittent failures)
- NFR18: Memory allocator shall not leak or corrupt memory
- NFR19: Context switches shall preserve all register state
- NFR20: File system shall not corrupt data on normal shutdown
- NFR21: System calls shall return correct error codes on failure

**Documentation (4 requirements)**
- NFR22: Each milestone shall have accompanying technical documentation
- NFR23: Documentation shall explain design decisions, not just implementation
- NFR24: Code comments shall note where Linux differs and why
- NFR25: Architecture overview shall map subsystems to Linux equivalents

### Additional Requirements

**From Architecture Document:**

1. **Starter Template**: xv6-inspired project structure with Linux-aligned naming conventions (kernel/mm/, kernel/proc/, kernel/fs/, etc.)

2. **Build System**: Non-recursive GNU Make with per-directory includes
   - Targets: `make`, `make qemu`, `make debug`, `make image`, `make clean`
   - Debug build by default (symbols, -O0)
   - Output to `build/` directory

3. **Memory Architecture**:
   - Physical Memory Manager: Bitmap allocator (1 bit per 4KB page frame)
   - Kernel Heap: Simple linked-list allocator with first-fit
   - Interface: `pmm_alloc_frame()`, `pmm_free_frame()`, `kmalloc()`, `kfree()`

4. **Filesystem Architecture**:
   - Custom minimal FS with on-disk layout: Superblock → Bitmaps → Inode table → Data blocks
   - Inode structure: type, size, 12 direct + 1 indirect block pointers (~16MB max file)
   - Directory format: Fixed-size entries (28-char filename + 4-byte inode number)

5. **Error Handling Strategy**:
   - Panic on kernel invariant violations (corrupted page tables, null pointers in critical structures)
   - Linux-style negative errno returns for recoverable errors (-ENOMEM, -EINVAL, -ENOENT, etc.)

6. **Logging Infrastructure**:
   - `printk()` with 4 levels: LOG_ERROR, LOG_WARN, LOG_INFO, LOG_DEBUG
   - Output: Serial (COM1) primary, VGA console secondary

7. **Testing Strategy**:
   - In-kernel tests: `test_<subsystem>()` functions called during `make test`
   - Host-side unit tests: Pure C algorithms compiled with host gcc

8. **Implementation Patterns**:
   - Naming: snake_case for functions/variables, UPPER_SNAKE for constants
   - Code style: K&R braces (function defs on new line), 4-space indentation
   - Header guards: PATH_FILENAME_H format
   - Assembly: AT&T syntax with documented input/output/clobber registers

9. **Project Directory Structure**:
   - boot/ (bootloader), kernel/ (core), libc/ (minimal C library), user/ (shell, editor)
   - tools/ (host utilities), scripts/ (linker scripts, gdbinit), docs/ (per-milestone)

10. **Architectural Boundaries**:
    - Boot → Kernel: `kmain()` entry point with defined register state
    - User → Kernel: `int 0x80` with Linux i386 ABI (eax=num, ebx/ecx/edx/esi/edi=args)
    - VFS → Filesystem: `struct inode_ops` function pointer vtable
    - Scheduler → Process: `struct task_struct` state machine

### FR Coverage Map

| FR | Epic | Description |
|----|------|-------------|
| FR1 | Epic 1 | Boot from raw disk image in QEMU |
| FR2 | Epic 1 | Bootloader loads kernel from disk |
| FR3 | Epic 1 | Real mode to protected mode transition |
| FR4 | Epic 1 | A20 line enablement |
| FR5 | Epic 1 | GDT initialization |
| FR6 | Epic 2 | IDT initialization |
| FR7 | Epic 3 | Initial page tables and paging |
| FR8 | Epic 1 | VGA text output during boot |
| FR9 | Epic 3 | Physical memory tracking (bitmap) |
| FR10 | Epic 3 | Physical page frame allocation |
| FR11 | Epic 3 | Page directory/table management |
| FR12 | Epic 3 | Virtual to physical address mapping |
| FR13 | Epic 3 | Page fault handling |
| FR14 | Epic 3 | Kernel heap allocation |
| FR15 | Epic 5 | Separate address spaces per process |
| FR16 | Epic 4 | Kernel thread creation |
| FR17 | Epic 4 | Process state in task_struct |
| FR18 | Epic 4 | Context switches |
| FR19 | Epic 4 | Round-robin scheduling |
| FR20 | Epic 5 | User mode transition (ring 3) |
| FR21 | Epic 6 | fork() implementation |
| FR22 | Epic 6 | exec() implementation |
| FR23 | Epic 6 | exit() implementation |
| FR24 | Epic 6 | wait/waitpid() implementation |
| FR25 | Epic 6 | Process hierarchy tracking |
| FR26 | Epic 5 | int 0x80 syscall invocation |
| FR27 | Epic 5 | Syscall dispatch |
| FR28 | Epic 5 | Argument passing to syscall handlers |
| FR29 | Epic 5 | Syscall result/error returns |
| FR30 | Epic 6 | Complete syscall set |
| FR31 | Epic 2 | Timer interrupt handling |
| FR32 | Epic 2 | Keyboard interrupt handling |
| FR33 | Epic 2 | Keyboard input buffering |
| FR34 | Epic 1 | VGA text mode display |
| FR35 | Epic 7 | ATA PIO disk read/write |
| FR36 | Epic 2 | Serial port debug output |
| FR37 | Epic 7 | File system mount at boot |
| FR38 | Epic 7 | Inode representation |
| FR39 | Epic 7 | Path traversal/lookup |
| FR40 | Epic 7 | File/directory creation |
| FR41 | Epic 7 | File read to user buffers |
| FR42 | Epic 7 | File write from user buffers |
| FR43 | Epic 7 | File/directory deletion |
| FR44 | Epic 7 | Directory listing |
| FR45 | Epic 7 | Per-process file descriptors |
| FR46 | Epic 7 | Data persistence across reboots |
| FR47 | Epic 8 | Shell command prompt |
| FR48 | Epic 8 | Built-in command execution |
| FR49 | Epic 8 | External program execution |
| FR50 | Epic 8 | cd command |
| FR51 | Epic 8 | pwd command |
| FR52 | Epic 8 | ls command |
| FR53 | Epic 8 | echo command |
| FR54 | Epic 8 | touch command |
| FR55 | Epic 8 | mkdir command |
| FR56 | Epic 9 | Text editor file display |
| FR57 | Epic 9 | Text insert/delete |
| FR58 | Epic 9 | Text editor file save |
| FR59 | Epic 8 | Libc syscall wrappers |
| FR60 | Epic 8 | Libc string functions |
| FR61 | Epic 8 | Libc printf-style output |
| FR62 | Epic 1 | Build system (make) |
| FR63 | Epic 1 | Bootable disk image (make image) |
| FR64 | Epic 1 | QEMU launch (make qemu) |
| FR65 | Epic 1 | GDB debug launch (make debug) |
| FR66 | Epic 1 | Serial debug logging |
| FR67 | Epic 1 | Panic handler with register state |
| FR68 | Epic 1 | GDB stub debugging |

**NFRs (1-25):** Cross-cutting quality attributes applied throughout all epics.

## Epic List

### Epic 1: Boot & Build Foundation
**User Value:** "I can boot my own kernel from bare metal and understand the boot process."

System boots from custom bootloader, transitions to protected mode, displays output, and has a complete build system for rapid iteration. Debug infrastructure (serial, panic handlers, GDB) available from first boot.

**FRs covered:** FR1-5, FR8, FR34, FR62-68

---

### Epic 2: Interrupt Handling & Device I/O
**User Value:** "My OS responds to hardware — I understand IDT, IRQs, and device drivers."

Keyboard input works, timer fires for preemptive scheduling foundation, serial debug output available. System handles interrupts correctly through the IDT.

**FRs covered:** FR6, FR31-33, FR36

---

### Epic 3: Memory Management
**User Value:** "I understand physical memory allocation and virtual memory — the MMU is no longer magic."

Physical frame allocator (bitmap) works, paging enabled with higher-half kernel, page faults handled with diagnostic output, kernel heap (kmalloc/kfree) available.

**FRs covered:** FR7, FR9-14

---

### Epic 4: Kernel Threads & Scheduling
**User Value:** "My OS runs multiple tasks — I understand context switching and scheduling."

Multiple kernel threads run concurrently with round-robin scheduling. task_struct maintains process state. Context switches preserve all register state correctly.

**FRs covered:** FR16-19

---

### Epic 5: User Mode & System Calls
**User Value:** "I understand the user/kernel boundary — ring transitions and syscall dispatch."

Processes run in user mode (ring 3), invoke kernel via int 0x80, return results correctly. Each process has its own virtual address space. Syscall dispatch table routes to handlers.

**FRs covered:** FR15, FR20, FR26-29

---

### Epic 6: Process Lifecycle
**User Value:** "I understand how processes are created, execute programs, and terminate."

fork() creates child processes with copied address space, exec() loads new programs, exit() terminates cleanly, wait/waitpid() synchronizes parent/child. Full syscall set available.

**FRs covered:** FR21-25, FR30

---

### Epic 7: File System
**User Value:** "I understand persistent storage — from disk blocks to files and directories."

ATA PIO driver reads/writes sectors. VFS layer with inodes, path traversal, create/read/write/delete for files and directories. Per-process file descriptor tables. Data persists across reboots.

**FRs covered:** FR35, FR37-46

---

### Epic 8: Shell & Userspace
**User Value:** "I have a working command-line interface — end-to-end OS integration proven."

Shell displays prompt, parses commands, executes built-ins (cd, pwd, ls, echo, touch, mkdir) and external programs. Minimal libc provides syscall wrappers, string functions, and printf.

**FRs covered:** FR47-55, FR59-61

---

### Epic 9: Text Editor
**User Value:** "I can create and edit files interactively — the OS is complete and usable."

Basic text editor can open files, display contents, insert/delete text, and save changes. Demonstrates complex user interaction and file I/O working end-to-end.

**FRs covered:** FR56-58

---

## Epic 1: Boot & Build Foundation

**Goal:** System boots from custom bootloader, transitions to protected mode, displays output, and has a complete build system for rapid iteration. Debug infrastructure (serial, panic handlers, GDB) available from first boot.

### Story 1.1: Project Structure & Build System

As a developer,
I want a complete project structure with working build system,
So that I can compile code, create bootable images, and run in QEMU with single commands.

**Acceptance Criteria:**

**Given** a fresh clone of the repository
**When** I run `make`
**Then** all source files compile without errors
**And** build output goes to `build/` directory

**Given** a successful build
**When** I run `make image`
**Then** a bootable disk image `build/os-dev.img` is created

**Given** a bootable disk image exists
**When** I run `make qemu`
**Then** QEMU launches with the disk image attached

**Given** a bootable disk image exists
**When** I run `make debug`
**Then** QEMU launches with GDB stub enabled, waiting for debugger connection

**Given** the project structure
**When** I examine the directories
**Then** I find: boot/, kernel/, kernel/init/, kernel/lib/, kernel/include/, scripts/, build/
**And** Makefile and config.mk exist at project root
**And** kernel.ld linker script exists in scripts/

---

### Story 1.2: Stage 1 Bootloader (MBR)

As a developer,
I want a stage 1 bootloader that fits in the MBR and loads stage 2,
So that I understand the first step of the boot process from BIOS to my code.

**Acceptance Criteria:**

**Given** the disk image is booted in QEMU
**When** BIOS loads sector 0
**Then** stage 1 bootloader executes from 0x7C00

**Given** stage 1 is executing
**When** it completes initialization
**Then** it loads stage 2 from subsequent disk sectors into memory
**And** jumps to stage 2 entry point

**Given** stage 1 source code
**When** I examine boot/stage1.S
**Then** it is exactly 512 bytes with boot signature 0xAA55 at offset 510
**And** code is commented explaining each step

**Given** stage 1 fails to load stage 2
**When** disk read error occurs
**Then** an error indicator is displayed (character on screen)

---

### Story 1.3: Stage 2 Bootloader & Protected Mode

As a developer,
I want stage 2 to enable A20, switch to protected mode, and load the kernel,
So that I understand CPU mode transitions and memory access beyond 1MB.

**Acceptance Criteria:**

**Given** stage 2 is executing in real mode
**When** it initializes
**Then** it enables the A20 line for full memory access
**And** A20 enablement is verified before proceeding

**Given** A20 is enabled
**When** stage 2 prepares for protected mode
**Then** it sets up a minimal GDT for the transition
**And** disables interrupts
**And** sets CR0.PE bit to enter protected mode
**And** performs far jump to flush prefetch queue

**Given** CPU is in protected mode
**When** stage 2 loads the kernel
**Then** kernel is loaded from disk to physical address 0x100000 (1MB)
**And** kernel size is determined from disk or header

**Given** kernel is loaded
**When** stage 2 transfers control
**Then** it jumps to kernel entry point with defined register state
**And** memory map information is passed to kernel (if available from BIOS)

---

### Story 1.4: Kernel Entry & GDT Setup

As a developer,
I want the kernel to initialize properly with a correct GDT,
So that I have proper segment descriptors for kernel and future user mode.

**Acceptance Criteria:**

**Given** the bootloader jumps to kernel entry
**When** kmain() begins executing
**Then** the kernel runs at its linked address (0xC0100000 virtual, initially identity-mapped)

**Given** kernel is initializing
**When** GDT setup is called
**Then** a GDT is created with: null descriptor, kernel code (ring 0), kernel data (ring 0)
**And** placeholder entries exist for user code/data (ring 3) and TSS
**And** GDTR is loaded with lgdt instruction

**Given** GDT is loaded
**When** segment registers are reloaded
**Then** CS points to kernel code segment
**And** DS, ES, SS point to kernel data segment

**Given** kernel/init/gdt.c source
**When** I examine the code
**Then** GDT structure matches Intel SDM format
**And** comments explain each descriptor field

---

### Story 1.5: VGA Text Mode Driver

As a developer,
I want to display text on screen via VGA text mode,
So that I can see boot progress and debug output visually.

**Acceptance Criteria:**

**Given** kernel is running
**When** vga_init() is called
**Then** screen is cleared to black background

**Given** VGA is initialized
**When** vga_putchar('H') is called
**Then** character 'H' appears at current cursor position
**And** cursor advances to next position

**Given** VGA is initialized
**When** vga_puts("Hello") is called
**Then** string "Hello" appears on screen

**Given** cursor is at end of line
**When** another character is printed
**Then** cursor wraps to beginning of next line

**Given** cursor is at bottom of screen
**When** another line is printed
**Then** screen scrolls up by one line
**And** new line appears at bottom

**Given** kernel boot completes
**When** system is stable
**Then** "Hello from os-dev!" (or similar) is displayed on screen

**Given** kernel/drivers/vga.c source
**When** I examine the code
**Then** VGA memory is accessed at 0xB8000
**And** attribute byte uses light grey on black (0x07)

---

### Story 1.6: Serial Debug & Panic Infrastructure

As a developer,
I want serial port output and a panic handler with register dumps,
So that I can debug issues even when VGA fails and use GDB effectively.

**Acceptance Criteria:**

**Given** kernel is initializing
**When** serial_init() is called
**Then** COM1 (0x3F8) is configured at 38400 baud

**Given** serial is initialized
**When** serial_putchar('X') is called
**Then** character 'X' is transmitted on COM1
**And** output is visible in QEMU's serial console (-serial stdio)

**Given** serial is working
**When** printk(LOG_INFO, "Boot complete") is called
**Then** "[INFO] Boot complete" appears on serial output
**And** message also appears on VGA console

**Given** an unrecoverable error occurs
**When** panic("message") is called
**Then** "KERNEL PANIC: message" is displayed
**And** register dump shows: EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP, EIP, EFLAGS
**And** system halts (cli; hlt loop)

**Given** QEMU is launched with `make debug`
**When** I connect GDB with `target remote :1234`
**Then** GDB attaches to the running kernel
**And** I can set breakpoints and inspect memory

**Given** kernel/lib/panic.c source
**When** I examine the code
**Then** panic captures register state before printing
**And** output goes to both serial and VGA

---

## Epic 2: Interrupt Handling & Device I/O

**Goal:** Keyboard input works, timer fires for preemptive scheduling foundation, serial debug output available. System handles interrupts correctly through the IDT.

### Story 2.1: IDT Setup & Exception Handlers

As a developer,
I want an IDT that handles CPU exceptions with useful diagnostic output,
So that I can debug faults and understand x86 exception handling.

**Acceptance Criteria:**

**Given** kernel is initializing
**When** idt_init() is called
**Then** IDT is created with 256 entries
**And** IDTR is loaded with lidt instruction

**Given** IDT is initialized
**When** CPU exceptions 0-31 occur
**Then** each exception has a registered handler
**And** handler prints exception name (e.g., "Division Error", "Page Fault")

**Given** a division by zero occurs
**When** exception 0 fires
**Then** handler displays "EXCEPTION: Division Error (#DE)"
**And** faulting instruction address (EIP) is shown
**And** system halts or returns (depending on exception type)

**Given** a page fault occurs (exception 14)
**When** handler executes
**Then** faulting address from CR2 is displayed
**And** error code indicates read/write and user/kernel mode
**And** diagnostic info goes to both serial and VGA

**Given** kernel/init/idt.c source
**When** I examine the code
**Then** IDT entry structure matches Intel SDM format
**And** each exception has a stub that pushes exception number
**And** common handler dispatches based on exception number

---

### Story 2.2: PIC & Timer Driver

As a developer,
I want the PIC configured and timer firing at regular intervals,
So that I have the foundation for preemptive scheduling.

**Acceptance Criteria:**

**Given** kernel is initializing
**When** pic_init() is called
**Then** both PICs (master and slave) are remapped
**And** IRQ 0-7 map to INT 32-39
**And** IRQ 8-15 map to INT 40-47
**And** all IRQs initially masked except those explicitly enabled

**Given** PIC is initialized
**When** timer_init() is called
**Then** PIT channel 0 is configured for 100Hz (10ms interval)
**And** IRQ 0 (INT 32) is unmasked

**Given** timer is running
**When** each timer interrupt fires
**Then** global tick counter increments
**And** EOI is sent to PIC
**And** interrupt returns cleanly

**Given** timer is running for 1 second
**When** I check the tick counter
**Then** counter shows approximately 100 ticks

**Given** kernel/drivers/timer.c source
**When** I examine the code
**Then** PIT divisor calculation is documented
**And** timer_get_ticks() function returns current count

**Given** kernel/init/pic.c source
**When** I examine the code
**Then** ICW1-ICW4 initialization sequence is commented
**And** pic_send_eoi() handles both master and slave PIC

---

### Story 2.3: Keyboard Driver

As a developer,
I want to receive keyboard input and buffer keystrokes,
So that I can interact with my OS and build toward a shell.

**Acceptance Criteria:**

**Given** kernel is initializing
**When** keyboard_init() is called
**Then** IRQ 1 (INT 33) is unmasked
**And** keyboard controller is ready to receive scancodes

**Given** keyboard driver is active
**When** a key is pressed
**Then** IRQ 1 fires and handler executes
**And** scancode is read from port 0x60
**And** EOI is sent to PIC

**Given** scancode is received
**When** it is a key press (not release)
**Then** scancode is translated to ASCII (for printable keys)
**And** character is added to keyboard buffer

**Given** keyboard buffer has characters
**When** keyboard_getchar() is called
**Then** oldest character is returned and removed from buffer
**And** function blocks or returns -1 if buffer empty (implementation choice)

**Given** keyboard buffer is full
**When** new keystrokes arrive
**Then** oldest characters are dropped (circular buffer)
**Or** new characters are dropped (document which)

**Given** I type "hello" on keyboard
**When** characters are read from buffer
**Then** "hello" is returned in order

**Given** kernel/drivers/keyboard.c source
**When** I examine the code
**Then** scancode set 1 translation table exists
**And** buffer size is defined as constant (e.g., 256 bytes)
**And** special keys (shift, ctrl, etc.) are noted but may be deferred

---

## Epic 3: Memory Management

**Goal:** Physical frame allocator (bitmap) works, paging enabled with higher-half kernel, page faults handled with diagnostic output, kernel heap (kmalloc/kfree) available.

### Story 3.1: Physical Memory Manager

As a developer,
I want a physical memory manager that tracks and allocates page frames,
So that I understand physical memory management and have frames available for paging.

**Acceptance Criteria:**

**Given** kernel is initializing
**When** pmm_init(memory_size) is called
**Then** bitmap is created with 1 bit per 4KB frame
**And** frames used by kernel are marked as allocated
**And** frames below 1MB are marked as reserved (BIOS, VGA, etc.)

**Given** PMM is initialized
**When** pmm_alloc_frame() is called
**Then** first free frame is found via bitmap scan
**And** frame is marked as allocated in bitmap
**And** physical address of frame is returned

**Given** multiple frames are allocated
**When** pmm_alloc_frame() is called repeatedly
**Then** different frames are returned each time
**And** no frame is returned twice

**Given** a frame is in use
**When** pmm_free_frame(phys_addr) is called
**Then** frame is marked as free in bitmap
**And** frame can be allocated again

**Given** all frames are allocated
**When** pmm_alloc_frame() is called
**Then** 0 (or NULL) is returned indicating failure

**Given** kernel/mm/pmm.c source
**When** I examine the code
**Then** bitmap is stored in kernel BSS or allocated region
**And** PAGE_SIZE is defined as 4096
**And** pmm_get_free_count() returns number of free frames

---

### Story 3.2: Paging & Virtual Memory

As a developer,
I want paging enabled with a higher-half kernel mapping,
So that I understand virtual memory and have address space isolation foundation.

**Acceptance Criteria:**

**Given** PMM is initialized
**When** vmm_init() is called
**Then** kernel page directory is created
**And** kernel is mapped at 0xC0000000+ (higher-half)
**And** identity mapping exists for low memory during transition

**Given** page directory is set up
**When** paging is enabled (CR0.PG = 1)
**Then** kernel continues executing at higher-half addresses
**And** identity mapping can be removed after transition

**Given** paging is enabled
**When** vmm_map_page(virt, phys, flags) is called
**Then** page table entry is created/updated
**And** virtual address maps to specified physical address
**And** flags (present, writable, user) are set correctly

**Given** a mapping exists
**When** vmm_unmap_page(virt) is called
**Then** page table entry is cleared
**And** TLB is invalidated for that address (invlpg)

**Given** kernel/mm/vmm.c source
**When** I examine the code
**Then** page directory entries point to page tables
**And** KERNEL_BASE is defined as 0xC0000000
**And** P2V/V2P macros convert between physical and virtual

**Given** kernel/mm/page.c source
**When** I examine the code
**Then** page table entry format matches Intel SDM
**And** PAGE_PRESENT, PAGE_WRITABLE, PAGE_USER flags defined

---

### Story 3.3: Page Fault Handler

As a developer,
I want page faults to display diagnostic information,
So that I can debug memory issues and understand MMU behavior.

**Acceptance Criteria:**

**Given** IDT is initialized with page fault handler (from Epic 2)
**When** page fault exception (INT 14) fires
**Then** handler reads faulting address from CR2
**And** error code is parsed for fault reason

**Given** page fault occurs
**When** handler executes
**Then** output shows: "PAGE FAULT at 0x[address]"
**And** output shows: "Error: [read/write] [user/kernel] [present/not-present]"
**And** faulting instruction (EIP) is displayed

**Given** page fault in kernel mode
**When** fault is not recoverable
**Then** system panics with full diagnostic info

**Given** page fault error code
**When** I examine the bits
**Then** bit 0 indicates present (1) or not-present (0)
**And** bit 1 indicates write (1) or read (0)
**And** bit 2 indicates user mode (1) or kernel mode (0)

**Given** kernel/mm/vmm.c or kernel/init/idt.c
**When** I examine page fault handler
**Then** CR2 is read immediately (before any memory access that could overwrite it)
**And** NFR10 (report faulting address and access type) is satisfied

---

### Story 3.4: Kernel Heap Allocator

As a developer,
I want kmalloc/kfree for dynamic kernel memory allocation,
So that I can allocate variable-sized objects without manual frame management.

**Acceptance Criteria:**

**Given** paging is enabled
**When** heap_init() is called
**Then** kernel heap region is established (e.g., starting after kernel BSS)
**And** initial heap pages are mapped

**Given** heap is initialized
**When** kmalloc(size) is called
**Then** block of at least 'size' bytes is returned
**And** returned pointer is aligned to 4 or 8 bytes
**And** memory is from kernel heap region

**Given** kmalloc returns a pointer
**When** I write to the allocated memory
**Then** no page fault occurs
**And** data is preserved until freed

**Given** memory was allocated with kmalloc
**When** kfree(ptr) is called
**Then** memory is returned to free pool
**And** memory can be reused by future kmalloc calls

**Given** heap runs out of space
**When** kmalloc needs more memory
**Then** new pages are allocated via PMM
**And** pages are mapped into heap region
**Or** NULL is returned if no frames available

**Given** kernel/mm/heap.c source
**When** I examine the code
**Then** free list or block header structure is documented
**And** first-fit algorithm is used (per Architecture)
**And** coalescing of adjacent free blocks is implemented

**Given** I allocate and free memory repeatedly
**When** checking for leaks
**Then** free frame count returns to original value
**And** NFR18 (no memory leaks) is verifiable

---

## Epic 4: Kernel Threads & Scheduling

**Goal:** Multiple kernel threads run concurrently with round-robin scheduling. task_struct maintains process state. Context switches preserve all register state correctly.

### Story 4.1: Task Structure & Thread Creation

As a developer,
I want a task_struct and the ability to create kernel threads,
So that I can represent multiple execution contexts and understand process state.

**Acceptance Criteria:**

**Given** kernel/proc/task.h exists
**When** I examine struct task_struct
**Then** it contains: pid, state, kernel stack pointer, page directory pointer
**And** it contains: saved register context (ESP, EBP, EIP, etc.)
**And** it contains: next pointer for scheduling queue
**And** naming follows Linux conventions

**Given** kernel is initializing
**When** proc_init() is called
**Then** task_struct for the initial kernel thread (PID 0 or 1) is created
**And** current task pointer is set

**Given** proc subsystem is initialized
**When** thread_create(entry_function) is called
**Then** new task_struct is allocated via kmalloc
**And** unique PID is assigned (sequential)
**And** 4KB kernel stack is allocated
**And** stack is initialized with entry point and initial register state
**And** task state is set to READY

**Given** task states are defined
**When** I examine the code
**Then** states include: RUNNING, READY, BLOCKED, ZOMBIE
**And** only one task can be RUNNING at a time

**Given** multiple threads are created
**When** I examine the task list
**Then** each has unique PID
**And** each has separate kernel stack

---

### Story 4.2: Context Switch

As a developer,
I want to switch execution between kernel threads,
So that I understand how the CPU transitions between tasks.

**Acceptance Criteria:**

**Given** two kernel threads exist (A and B)
**When** context_switch(A, B) is called
**Then** thread A's registers are saved to its task_struct
**And** thread B's registers are restored from its task_struct
**And** execution continues in thread B

**Given** context switch is implemented
**When** I examine kernel/proc/switch.S
**Then** it saves: EBX, ESI, EDI, EBP, ESP
**And** it saves EIP (via call/ret mechanism or explicit save)
**And** it switches stack pointers
**And** it restores callee-saved registers

**Given** context switch completes
**When** thread B runs
**Then** all register values are exactly as when B was last switched out
**And** NFR19 (preserve all register state) is satisfied

**Given** kernel/proc/switch.S source
**When** I examine the code
**Then** calling convention is documented (cdecl)
**And** stack layout during switch is diagrammed in comments
**And** assembly uses AT&T syntax

**Given** thread A calls a function, then is switched out and back
**When** thread A resumes
**Then** it continues exactly where it left off
**And** local variables are preserved

---

### Story 4.3: Round-Robin Scheduler

As a developer,
I want a round-robin scheduler driven by timer interrupts,
So that multiple threads run concurrently with fair time slicing.

**Acceptance Criteria:**

**Given** multiple READY threads exist
**When** schedule() is called
**Then** next READY thread is selected from queue
**And** current thread is moved to end of ready queue (if still READY)
**And** context_switch is called to switch tasks

**Given** timer interrupt fires (from Epic 2)
**When** timer handler executes
**Then** schedule() is called
**And** preemptive multitasking occurs

**Given** threads A, B, C are READY
**When** scheduler runs over time
**Then** each thread gets CPU time in round-robin order: A, B, C, A, B, C...
**And** time slice is approximately 10ms (one timer tick)

**Given** only one thread is READY
**When** schedule() is called
**Then** that thread continues running
**And** no unnecessary context switch occurs

**Given** current thread blocks (sets state to BLOCKED)
**When** schedule() is called
**Then** blocked thread is not in ready queue
**And** next READY thread runs

**Given** kernel/proc/sched.c source
**When** I examine the code
**Then** ready queue is a simple linked list
**And** schedule() is clearly documented
**And** interrupts are disabled during queue manipulation

**Given** I create two threads that each print their ID in a loop
**When** system runs
**Then** output shows interleaved prints from both threads
**And** preemption is visibly working

---

## Epic 5: User Mode & System Calls

**Goal:** Processes run in user mode (ring 3), invoke kernel via int 0x80, return results correctly. Each process has its own virtual address space. Syscall dispatch table routes to handlers.

### Story 5.1: User Mode & TSS Setup

As a developer,
I want processes to run in user mode (ring 3) with proper privilege transitions,
So that I understand CPU protection rings and the user/kernel boundary.

**Acceptance Criteria:**

**Given** GDT exists from Epic 1
**When** user mode is being set up
**Then** GDT entries for user code (ring 3) and user data (ring 3) are populated
**And** segment selectors have RPL=3

**Given** kernel needs to handle privilege transitions
**When** tss_init() is called
**Then** TSS (Task State Segment) is created
**And** TSS.SS0 is set to kernel data segment
**And** TSS.ESP0 is set to kernel stack for current process
**And** TSS descriptor is added to GDT
**And** TR register is loaded with ltr instruction

**Given** TSS is configured
**When** interrupt occurs in user mode
**Then** CPU automatically switches to kernel stack (TSS.ESP0)
**And** user SS:ESP is saved on kernel stack

**Given** a process is ready to enter user mode
**When** enter_usermode(entry, user_stack) is called
**Then** iret is used to switch to ring 3
**And** CS is set to user code segment (RPL=3)
**And** SS is set to user data segment (RPL=3)
**And** EIP is set to user entry point
**And** ESP is set to user stack

**Given** process is running in user mode
**When** I examine segment registers
**Then** CS shows ring 3 (low 2 bits = 3)
**And** privileged instructions cause general protection fault

---

### Story 5.2: Per-Process Address Spaces

As a developer,
I want each process to have its own virtual address space,
So that processes are isolated from each other.

**Acceptance Criteria:**

**Given** a new user process is being created
**When** vmm_create_address_space() is called
**Then** new page directory is allocated
**And** kernel mappings (0xC0000000+) are copied/shared
**And** user space (below 0xC0000000) is initially empty

**Given** user address space exists
**When** user pages are mapped
**Then** pages are mapped with PAGE_USER flag set
**And** user code at 0x08048000 (traditional Linux base) is accessible
**And** user stack near 0xBFFFFFFF is accessible

**Given** two processes A and B exist
**When** I examine their page directories
**Then** each has different page directory physical address
**And** kernel mappings point to same physical memory
**And** user mappings are independent

**Given** context switch occurs between processes
**When** scheduler switches from A to B
**Then** CR3 is loaded with B's page directory
**And** TLB is flushed (implicit with CR3 load)
**And** B sees its own address space

**Given** process A writes to address 0x08048000
**When** process B reads from address 0x08048000
**Then** B sees its own data, not A's
**And** memory isolation is verified

---

### Story 5.3: System Call Infrastructure

As a developer,
I want user programs to invoke kernel services via int 0x80,
So that I understand the syscall interface and can build on it.

**Acceptance Criteria:**

**Given** IDT is initialized
**When** syscall_init() is called
**Then** interrupt 0x80 is registered with DPL=3 (callable from user mode)
**And** handler points to syscall entry stub

**Given** user program executes int 0x80
**When** syscall entry stub runs
**Then** registers are saved (user context)
**And** syscall number is read from EAX
**And** arguments are read from EBX, ECX, EDX, ESI, EDI

**Given** syscall number is valid
**When** dispatcher looks up handler
**Then** syscall table is indexed by syscall number
**And** corresponding handler function is called
**And** arguments are passed to handler

**Given** syscall handler completes
**When** returning to user mode
**Then** return value is placed in EAX
**And** negative values indicate errors (-errno convention)
**And** iret returns to user code

**Given** syscall number is invalid
**When** dispatcher checks bounds
**Then** -ENOSYS is returned
**And** no crash occurs

**Given** kernel/syscall/syscall.c source
**When** I examine the code
**Then** syscall table is an array of function pointers
**And** SYS_read=0, SYS_write=1, etc. are defined
**And** placeholder handlers exist for future syscalls

**Given** a test syscall (e.g., sys_getpid)
**When** user program calls it via int 0x80 with EAX=getpid number
**Then** correct PID is returned in EAX
**And** round-trip user→kernel→user works correctly

---

## Epic 6: Process Lifecycle

**Goal:** fork() creates child processes with copied address space, exec() loads new programs, exit() terminates cleanly, wait/waitpid() synchronizes parent/child. Full syscall set available.

### Story 6.1: fork() Implementation

As a developer,
I want fork() to create a child process with copied address space,
So that I understand process creation and the fork model.

**Acceptance Criteria:**

**Given** a user process calls fork()
**When** sys_fork() executes in kernel
**Then** new task_struct is created for child
**And** child gets new unique PID
**And** parent PID is recorded in child's task_struct

**Given** fork() is creating child
**When** address space is copied
**Then** new page directory is allocated for child
**And** all user pages are copied (not shared) to new frames
**And** kernel mappings are shared (same physical pages)

**Given** fork() completes
**When** returning to user mode
**Then** parent receives child's PID as return value
**And** child receives 0 as return value
**And** both processes continue from same instruction

**Given** parent and child both run after fork
**When** child modifies a variable
**Then** parent's copy is unaffected
**And** memory isolation is maintained

**Given** fork() fails (out of memory)
**When** allocation fails
**Then** -ENOMEM is returned to parent
**And** no partial child is left behind

**Given** kernel/proc/proc.c and kernel/syscall/sys_proc.c
**When** I examine do_fork() implementation
**Then** task allocation, address space copy, and return value setup are clear
**And** FR21 and FR25 (process hierarchy) are satisfied

---

### Story 6.2: exec() Implementation

As a developer,
I want exec() to load and run a new program in the current process,
So that I understand program loading and address space replacement.

**Acceptance Criteria:**

**Given** a process calls exec(path, argv)
**When** sys_exec() executes in kernel
**Then** file at path is opened and read
**And** ELF header is validated (magic number, architecture)

**Given** ELF file is valid
**When** program segments are loaded
**Then** current user address space is cleared
**And** PT_LOAD segments are mapped at specified virtual addresses
**And** BSS is zeroed
**And** new user stack is set up

**Given** program is loaded
**When** exec() completes
**Then** execution jumps to ELF entry point (e_entry)
**And** argc/argv are placed on user stack
**And** process PID remains the same

**Given** exec() fails (file not found, invalid ELF)
**When** error occurs
**Then** appropriate errno is returned (-ENOENT, -ENOEXEC)
**And** original process continues running (exec failed)

**Given** libc/crt0.S exists
**When** user program starts
**Then** crt0 is the actual entry point
**And** crt0 calls main(argc, argv)
**And** crt0 calls exit() when main returns

**Given** kernel/proc/proc.c
**When** I examine do_exec() implementation
**Then** ELF parsing handles 32-bit i386 executables
**And** only essential ELF fields are used (keep it simple)

---

### Story 6.3: exit() and wait()

As a developer,
I want exit() to terminate processes and wait() to synchronize with children,
So that I understand process termination and zombie reaping.

**Acceptance Criteria:**

**Given** a process calls exit(status)
**When** sys_exit() executes
**Then** process state becomes ZOMBIE
**And** exit status is stored in task_struct
**And** all user memory is freed
**And** file descriptors are closed (if FS exists)
**And** schedule() is called (process never runs again)

**Given** child becomes ZOMBIE
**When** parent calls wait(status_ptr)
**Then** parent blocks if no zombie children exist
**And** when zombie child exists, its PID is returned
**And** exit status is written to status_ptr
**And** child's task_struct is fully freed

**Given** parent calls waitpid(pid, status_ptr, options)
**When** specific child PID is requested
**Then** only that child is waited for
**And** -ECHILD returned if PID is not a child

**Given** parent exits before child
**When** child becomes orphan
**Then** child is reparented to init (PID 1)
**Or** child is cleaned up immediately (simpler approach, document choice)

**Given** multiple children are zombies
**When** parent calls wait()
**Then** one zombie is reaped (any order acceptable for MVP)
**And** subsequent wait() calls reap remaining zombies

**Given** kernel/syscall/sys_proc.c
**When** I examine sys_exit() and sys_wait()
**Then** zombie state transition is clear
**And** parent notification/wakeup is implemented
**And** FR23, FR24 are satisfied

---

### Story 6.4: Complete Syscall Set

As a developer,
I want all core syscalls registered and functional,
So that user programs have the full interface they need.

**Acceptance Criteria:**

**Given** syscall table from Story 5.3
**When** all process syscalls are added
**Then** table includes: fork, exec, exit, wait, waitpid, getpid, getppid

**Given** getpid syscall
**When** user calls getpid()
**Then** current process PID is returned

**Given** getppid syscall
**When** user calls getppid()
**Then** parent process PID is returned

**Given** brk syscall (for heap)
**When** user calls brk(new_break)
**Then** program break is adjusted
**And** new pages are mapped if break increases
**And** current break is returned

**Given** syscall numbering
**When** I examine kernel/include/syscall.h
**Then** numbers match PRD specification:
  - 0: read, 1: write, 2: open, 3: lseek, 4: close
  - 5: stat, 6: fstat, 7: brk
  - 8: fork, 9: exec, 10: exit, 11: waitpid
  - 12: getpid, 13: getppid, 14: wait

**Given** syscalls not yet implemented (read, write, open, etc.)
**When** called before FS epic
**Then** stub returns -ENOSYS
**And** system does not crash

**Given** kernel/syscall/ directory
**When** I examine the organization
**Then** sys_proc.c has process syscalls
**And** sys_fs.c has file syscalls (stubs for now)
**And** NFR21 (correct error codes) is followed throughout

---

## Epic 7: File System

**Goal:** ATA PIO driver reads/writes sectors. VFS layer with inodes, path traversal, create/read/write/delete for files and directories. Per-process file descriptor tables. Data persists across reboots.

### Story 7.1: ATA PIO Disk Driver

As a developer,
I want to read and write disk sectors via ATA PIO,
So that I have the foundation for persistent storage.

**Acceptance Criteria:**

**Given** kernel is initializing
**When** ata_init() is called
**Then** primary ATA bus is probed (ports 0x1F0-0x1F7)
**And** drive 0 (master) is identified
**And** drive presence and type are detected

**Given** ATA driver is initialized
**When** ata_read_sector(lba, buffer) is called
**Then** sector at logical block address is read
**And** 512 bytes are copied to buffer
**And** function returns 0 on success

**Given** ATA driver is initialized
**When** ata_write_sector(lba, buffer) is called
**Then** 512 bytes from buffer are written to disk
**And** sector at LBA is updated
**And** function returns 0 on success

**Given** invalid LBA is requested
**When** read or write is attempted
**Then** error is returned
**And** system does not crash

**Given** kernel/drivers/ata.c source
**When** I examine the code
**Then** PIO read/write sequences follow ATA specification
**And** BSY and DRQ status bits are polled correctly
**And** 28-bit LBA addressing is used (sufficient for MVP)

**Given** I write data to sector N, then read sector N
**When** comparing buffers
**Then** data matches exactly
**And** round-trip works correctly

---

### Story 7.2: Filesystem On-Disk Format & mkfs

As a developer,
I want a defined on-disk filesystem format and a tool to create it,
So that I have structured persistent storage.

**Acceptance Criteria:**

**Given** filesystem design from Architecture
**When** I examine the on-disk layout
**Then** block 0 is superblock (magic, block count, inode count, pointers)
**And** blocks 1-N are inode bitmap + block bitmap
**And** blocks N+1-M are inode table (fixed-size entries)
**And** blocks M+1-end are data blocks

**Given** inode structure is defined
**When** I examine struct inode on disk
**Then** it contains: type (file/dir), size, 12 direct block pointers
**And** it contains: 1 indirect block pointer
**And** total addressable size is ~16MB per file

**Given** directory entry format is defined
**When** I examine directory structure
**Then** entries are fixed-size: 28 bytes name + 4 bytes inode number
**And** name is null-terminated within 28 bytes

**Given** tools/mkfs.c exists
**When** mkfs is compiled and run on host
**Then** it creates a formatted filesystem image
**And** superblock is initialized with correct values
**And** root directory (inode 0 or 1) is created with "." and ".." entries
**And** bitmaps mark root inode and its data block as allocated

**Given** formatted filesystem image
**When** kernel mounts it at boot
**Then** superblock is read and validated (magic number check)
**And** root inode is accessible
**And** filesystem is ready for operations

---

### Story 7.3: VFS Layer & Inode Operations

As a developer,
I want a VFS layer that handles path lookup and directory operations,
So that I can navigate the filesystem hierarchy.

**Acceptance Criteria:**

**Given** filesystem is mounted
**When** vfs_lookup(path) is called with "/"
**Then** root inode is returned

**Given** root directory contains "bin" subdirectory
**When** vfs_lookup("/bin") is called
**Then** inode for "bin" directory is returned

**Given** path "/bin/shell" exists
**When** vfs_lookup("/bin/shell") is called
**Then** inode for "shell" file is returned

**Given** path does not exist
**When** vfs_lookup("/nonexistent") is called
**Then** NULL is returned (or -ENOENT error)

**Given** inode is a directory
**When** vfs_readdir(inode, index) is called
**Then** directory entry at index is returned
**And** entries can be iterated to list directory contents

**Given** kernel/fs/vfs.c source
**When** I examine the code
**Then** struct inode_ops defines: lookup, readdir, read, write, create
**And** path parsing splits on '/' correctly
**And** ".." navigation works (parent directory)

**Given** in-memory inode cache
**When** same inode is looked up multiple times
**Then** cached inode is returned (avoid repeated disk reads)
**And** cache is simple (MVP doesn't need complex eviction)

---

### Story 7.4: File Descriptors & read/write

As a developer,
I want open/read/write/close syscalls working with file descriptors,
So that user programs can perform file I/O.

**Acceptance Criteria:**

**Given** per-process file descriptor table
**When** process is created
**Then** FD table is initialized (array of pointers, initially NULL)
**And** FDs 0, 1, 2 may be reserved for stdin/stdout/stderr

**Given** user calls open(path, flags)
**When** sys_open() executes
**Then** path is looked up via VFS
**And** file struct is created (inode pointer, offset=0, flags)
**And** lowest available FD is assigned
**And** FD number is returned to user

**Given** valid file descriptor
**When** read(fd, buf, count) is called
**Then** data is read from file at current offset
**And** offset advances by bytes read
**And** actual bytes read is returned (may be less than count at EOF)

**Given** valid file descriptor opened for writing
**When** write(fd, buf, count) is called
**Then** data is written to file at current offset
**And** offset advances by bytes written
**And** file size increases if writing past end
**And** actual bytes written is returned

**Given** file descriptor
**When** lseek(fd, offset, whence) is called
**Then** file offset is adjusted (SEEK_SET, SEEK_CUR, SEEK_END)
**And** new offset is returned

**Given** file descriptor
**When** close(fd) is called
**Then** FD entry is cleared
**And** file struct reference count decremented
**And** resources freed if no more references

**Given** invalid FD (negative, too large, or not open)
**When** read/write/close is called
**Then** -EBADF is returned

---

### Story 7.5: File Creation & Deletion

As a developer,
I want to create and delete files and directories,
So that the filesystem is fully functional and data persists.

**Acceptance Criteria:**

**Given** user calls open(path, O_CREAT) for new file
**When** sys_open() executes
**Then** new inode is allocated (bitmap updated)
**And** directory entry is added to parent directory
**And** inode is initialized (type=file, size=0)
**And** FD for new file is returned

**Given** user calls mkdir(path)
**When** sys_mkdir() executes
**Then** new inode is allocated with type=directory
**And** directory entry added to parent
**And** new directory contains "." and ".." entries
**And** 0 returned on success

**Given** user calls unlink(path) on a file
**When** sys_unlink() executes
**Then** directory entry is removed from parent
**And** inode link count decremented
**And** if link count reaches 0, inode and data blocks freed

**Given** user calls rmdir(path) on empty directory
**When** sys_rmdir() executes
**Then** directory is removed (if empty)
**And** -ENOTEMPTY returned if directory has entries

**Given** filesystem modifications are made
**When** system reboots
**Then** changes persist (superblock, bitmaps, inodes written to disk)
**And** FR46 (data persistence) is satisfied

**Given** stat(path, statbuf) syscall
**When** user calls it
**Then** file metadata is returned (size, type, inode number)
**And** fstat(fd, statbuf) works similarly for open files

**Given** kernel/fs/ directory
**When** I examine the organization
**Then** vfs.c has VFS layer, inode.c has inode ops
**And** file.c has file descriptor logic
**And** osfs.c has our custom filesystem implementation

---

## Epic 8: Shell & Userspace

**Goal:** Shell displays prompt, parses commands, executes built-ins (cd, pwd, ls, echo, touch, mkdir) and external programs. Minimal libc provides syscall wrappers, string functions, and printf.

### Story 8.1: Minimal libc Foundation

As a developer,
I want a minimal C library for user programs,
So that user code can make syscalls and use basic functions.

**Acceptance Criteria:**

**Given** libc/syscall.S exists
**When** I examine syscall wrappers
**Then** each wrapper loads syscall number into EAX
**And** arguments go into EBX, ECX, EDX, ESI, EDI
**And** int 0x80 is executed
**And** result from EAX is returned

**Given** libc provides syscall wrappers
**When** user code calls read(), write(), open(), etc.
**Then** corresponding syscall is invoked
**And** return value and errno handling work correctly

**Given** libc/string.c exists
**When** I examine string functions
**Then** strlen, strcpy, strncpy, strcmp, strncmp are implemented
**And** memcpy, memset, memmove are implemented
**And** strchr, strrchr are implemented

**Given** libc/stdio.c exists
**When** printf(format, ...) is called
**Then** format string is parsed
**And** %s (string), %d (int), %x (hex), %c (char) are supported
**And** output goes to stdout (fd 1) via write syscall

**Given** libc/stdio.c exists
**When** puts(str) is called
**Then** string is written to stdout with newline

**Given** libc/stdlib.c exists
**When** malloc/free are called
**Then** memory is allocated via brk syscall
**And** simple allocator manages user heap

**Given** libc/crt0.S exists
**When** program starts
**Then** crt0 is entry point (called by kernel)
**And** crt0 sets up argc/argv from stack
**And** crt0 calls main(argc, argv)
**And** crt0 calls exit() with main's return value

**Given** libc/include/ headers exist
**When** I examine them
**Then** stdio.h, stdlib.h, string.h, unistd.h are present
**And** function prototypes match implementations

---

### Story 8.2: Shell Core & Input Loop

As a developer,
I want a shell that displays a prompt and reads commands,
So that I can interact with the OS.

**Acceptance Criteria:**

**Given** shell is the init process (PID 1)
**When** kernel finishes booting
**Then** shell is exec'd as first user process
**And** shell begins execution

**Given** shell is running
**When** main loop executes
**Then** prompt is displayed (e.g., "os-dev$ ")
**And** shell waits for user input

**Given** user types a command and presses Enter
**When** input is received
**Then** line is read into buffer (up to newline)
**And** line is parsed into command and arguments

**Given** command line "echo hello world"
**When** parsed
**Then** argv[0] = "echo", argv[1] = "hello", argv[2] = "world"
**And** argc = 3

**Given** empty input (just Enter)
**When** processed
**Then** shell redisplays prompt
**And** no error occurs

**Given** input exceeds buffer size
**When** reading
**Then** input is truncated safely
**And** no buffer overflow occurs

**Given** user/shell/shell.c source
**When** I examine the code
**Then** main loop is clear: prompt → read → parse → execute → repeat
**And** parsing handles spaces between arguments
**And** FR47 (prompt and read input) is satisfied

---

### Story 8.3: Built-in Commands

As a developer,
I want shell built-in commands for filesystem navigation and basic operations,
So that I can use my OS interactively.

**Acceptance Criteria:**

**Given** user types "pwd"
**When** command executes
**Then** current working directory path is printed
**And** FR51 is satisfied

**Given** user types "cd /bin"
**When** command executes
**Then** current working directory changes to /bin
**And** subsequent pwd shows /bin
**And** FR50 is satisfied

**Given** user types "cd .."
**When** command executes
**Then** current directory moves to parent

**Given** user types "cd nonexistent"
**When** command executes
**Then** error message is displayed
**And** current directory unchanged

**Given** user types "ls"
**When** command executes
**Then** contents of current directory are listed
**And** FR52 is satisfied

**Given** user types "ls /bin"
**When** command executes
**Then** contents of /bin directory are listed

**Given** user types "echo hello world"
**When** command executes
**Then** "hello world" is printed to stdout
**And** FR53 is satisfied

**Given** user types "touch newfile.txt"
**When** command executes
**Then** empty file newfile.txt is created
**And** FR54 is satisfied

**Given** user types "mkdir newdir"
**When** command executes
**Then** new directory newdir is created
**And** FR55 is satisfied

**Given** built-in command lookup
**When** shell processes a command
**Then** shell checks if command is built-in first
**And** built-ins execute without fork (cd must be built-in)

---

### Story 8.4: External Program Execution

As a developer,
I want the shell to run external programs,
So that I can execute binaries from the filesystem.

**Acceptance Criteria:**

**Given** user types "/bin/hello"
**When** command is not a built-in
**Then** shell calls fork() to create child
**And** child calls exec("/bin/hello", argv)
**And** parent calls wait() for child to finish

**Given** external program executes
**When** it completes
**Then** shell regains control
**And** prompt is redisplayed
**And** FR49 is satisfied

**Given** external program not found
**When** exec() fails
**Then** child prints error message
**And** child exits with error status
**And** parent continues normally

**Given** PATH-like search (optional for MVP)
**When** user types "hello" without path
**Then** shell searches /bin for "hello"
**Or** shell requires full path (document choice)

**Given** program prints output
**When** running
**Then** output appears on screen
**And** shell waits until program finishes

**Given** user/bin/ directory exists
**When** I examine it
**Then** simple test programs exist (e.g., hello.c that prints "Hello!")
**And** programs link against libc
**And** programs are included in filesystem image by mkfs

**Given** end-to-end test
**When** I boot the OS
**Then** shell prompt appears
**And** I can run ls, cd, pwd, echo, touch, mkdir
**And** I can execute /bin/hello
**And** OS is interactive and functional

---

## Epic 9: Text Editor

**Goal:** Basic text editor can open files, display contents, insert/delete text, and save changes. Demonstrates complex user interaction and file I/O working end-to-end.

### Story 9.1: Editor Core & File Display

As a developer,
I want a text editor that can open and display files,
So that I can view file contents interactively.

**Acceptance Criteria:**

**Given** user runs "edit filename.txt" from shell
**When** editor starts
**Then** file contents are loaded into memory buffer
**And** contents are displayed on screen
**And** cursor is positioned at start of file

**Given** file does not exist
**When** editor opens it
**Then** empty buffer is created
**And** editor shows empty screen (new file mode)

**Given** file is displayed
**When** user presses arrow keys
**Then** cursor moves up/down/left/right within text
**And** cursor position is visually indicated

**Given** file is longer than screen height
**When** cursor moves past visible area
**Then** display scrolls to keep cursor visible
**And** user can navigate entire file

**Given** editor is running
**When** user presses 'q' or Ctrl+Q (or designated quit key)
**Then** editor prompts to save if modified
**And** editor exits to shell

**Given** user/edit/edit.c source
**When** I examine the code
**Then** file is read into a buffer (array of lines or gap buffer)
**And** screen rendering is separate from buffer management
**And** FR56 (open and display) is satisfied

**Given** editor screen
**When** displayed
**Then** status line shows filename and cursor position
**And** help hints show basic commands (optional but helpful)

---

### Story 9.2: Text Editing & Save

As a developer,
I want to insert, delete, and save text in the editor,
So that I can create and modify files.

**Acceptance Criteria:**

**Given** editor is in insert mode (or default mode allows typing)
**When** user types printable characters
**Then** characters are inserted at cursor position
**And** cursor advances
**And** display updates to show new text
**And** FR57 (insert text) is satisfied

**Given** cursor is in text
**When** user presses Backspace
**Then** character before cursor is deleted
**And** cursor moves back
**And** display updates

**Given** cursor is in text
**When** user presses Delete (if supported)
**Then** character at cursor is deleted
**And** following text shifts left

**Given** cursor is at end of line
**When** user presses Enter
**Then** new line is created
**And** cursor moves to start of new line

**Given** buffer has been modified
**When** user presses Ctrl+S (or designated save key)
**Then** buffer contents are written to file
**And** file on disk is updated
**And** "Saved" message is displayed
**And** FR58 (save changes) is satisfied

**Given** user tries to quit with unsaved changes
**When** quit command is issued
**Then** editor prompts "Save changes? (y/n)"
**And** user can choose to save, discard, or cancel

**Given** save fails (disk full, permission error)
**When** write fails
**Then** error message is displayed
**And** buffer is preserved (not lost)

**Given** editor implementation
**When** I examine the code
**Then** buffer modification tracks "dirty" state
**And** save writes entire buffer to file (simple approach for MVP)
**And** editor handles files up to reasonable size (e.g., 64KB)

**Given** complete MVP test
**When** I boot os-dev, run editor, create a file, save, exit, and cat the file
**Then** file contains the text I entered
**And** end-to-end file creation via editor works
**And** OS is complete and usable

