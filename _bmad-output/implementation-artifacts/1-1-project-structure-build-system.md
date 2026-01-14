# Story 1.1: Project Structure & Build System

Status: review

## Story

As a developer,
I want a complete project structure with working build system,
so that I can compile code, create bootable images, and run in QEMU with single commands.

## Acceptance Criteria

1. **AC1: Fresh Build**
   - Given a fresh clone of the repository
   - When I run `make`
   - Then all source files compile without errors
   - And build output goes to `build/` directory

2. **AC2: Bootable Image Creation**
   - Given a successful build
   - When I run `make image`
   - Then a bootable disk image `build/os-dev.img` is created

3. **AC3: QEMU Launch**
   - Given a bootable disk image exists
   - When I run `make qemu`
   - Then QEMU launches with the disk image attached

4. **AC4: Debug Launch**
   - Given a bootable disk image exists
   - When I run `make debug`
   - Then QEMU launches with GDB stub enabled, waiting for debugger connection

5. **AC5: Directory Structure**
   - Given the project structure
   - When I examine the directories
   - Then I find: boot/, kernel/, kernel/init/, kernel/lib/, kernel/include/, scripts/, build/
   - And Makefile and config.mk exist at project root
   - And kernel.ld linker script exists in scripts/

## Tasks / Subtasks

- [x] **Task 1: Create Directory Structure** (AC: #5)
  - [x] 1.1 Create boot/ directory for bootloader code
  - [x] 1.2 Create kernel/ with subdirectories: init/, lib/, include/, mm/, proc/, drivers/, syscall/, fs/
  - [x] 1.3 Create libc/ directory (empty for now)
  - [x] 1.4 Create user/ directory with shell/, edit/, bin/ subdirs (empty for now)
  - [x] 1.5 Create scripts/ directory for linker scripts
  - [x] 1.6 Create tools/ directory for host utilities
  - [x] 1.7 Create docs/ directory for milestone documentation

- [x] **Task 2: Create config.mk** (AC: #1)
  - [x] 2.1 Define cross-compiler variables (CC, AS, LD)
  - [x] 2.2 Set i686-elf-gcc as cross-compiler
  - [x] 2.3 Define CFLAGS with -m32, -ffreestanding, -nostdlib, -fno-builtin, -fno-stack-protector
  - [x] 2.4 Define ASFLAGS for assembly compilation
  - [x] 2.5 Define LDFLAGS for linking
  - [x] 2.6 Set debug build flags by default (-g, -O0)

- [x] **Task 3: Create Kernel Linker Script** (AC: #5)
  - [x] 3.1 Create scripts/kernel.ld
  - [x] 3.2 Set kernel virtual base at 0xC0100000 (higher-half)
  - [x] 3.3 Define sections: .text, .rodata, .data, .bss
  - [x] 3.4 Export symbols for kernel_start, kernel_end, bss_start, bss_end
  - [x] 3.5 Ensure 4KB page alignment for sections

- [x] **Task 4: Create Top-Level Makefile** (AC: #1, #2, #3, #4)
  - [x] 4.1 Include config.mk for compiler settings
  - [x] 4.2 Define BUILD directory variable
  - [x] 4.3 Create `all` target that builds kernel binary
  - [x] 4.4 Create `image` target that produces os-dev.img
  - [x] 4.5 Create `qemu` target: `qemu-system-i386 -drive file=$(BUILD)/os-dev.img,format=raw`
  - [x] 4.6 Create `debug` target: add `-s -S` flags to QEMU
  - [x] 4.7 Create `clean` target to remove build artifacts
  - [x] 4.8 Create placeholder build.mk includes for each subdirectory

- [x] **Task 5: Create Minimal Kernel Stub** (AC: #1)
  - [x] 5.1 Create kernel/init/main.c with empty kmain() function
  - [x] 5.2 Create kernel/include/types.h with basic type definitions
  - [x] 5.3 Ensure the stub compiles successfully

- [x] **Task 6: Create Placeholder Boot Stub** (AC: #2)
  - [x] 6.1 Create boot/stage1.S with minimal MBR (512 bytes, boot signature)
  - [x] 6.2 Stage1 should halt after boot (placeholder for Story 1.2)
  - [x] 6.3 Ensure boot signature 0xAA55 at offset 510

- [x] **Task 7: Create Disk Image Assembly** (AC: #2)
  - [x] 7.1 Create script or Makefile target to assemble disk image
  - [x] 7.2 Place stage1 at sector 0
  - [x] 7.3 Placeholder sectors for stage2 and kernel (will be populated in later stories)

- [x] **Task 8: Verify Build System** (AC: #1, #2, #3, #4)
  - [x] 8.1 Run `make` and verify no compilation errors
  - [x] 8.2 Run `make image` and verify os-dev.img is created
  - [x] 8.3 Run `make qemu` and verify QEMU launches (will show minimal boot)
  - [x] 8.4 Run `make debug` and verify GDB can connect
  - [x] 8.5 Run `make clean` and verify build/ is removed

## Dev Notes

### Critical Architecture Patterns

**Naming Conventions (MUST FOLLOW):**
- Functions/variables: `snake_case`
- Structs: `struct snake_case` (NOT typedef'd)
- Constants/macros: `UPPER_SNAKE`
- Header guards: `PATH_FILENAME_H` (e.g., `KERNEL_INCLUDE_TYPES_H`)

**Code Style:**
- 4-space indentation (NO tabs)
- K&R braces: function definitions brace on NEW line, control structures brace on SAME line
- Pointer style: `int *ptr` (asterisk with variable, not type)
- Line limit: ~80 characters

**Header File Structure:**
```c
#ifndef KERNEL_INCLUDE_TYPES_H
#define KERNEL_INCLUDE_TYPES_H

/* 1. Includes */
/* 2. Constants */
/* 3. Type definitions */
/* 4. Function declarations */
/* 5. Inline functions */

#endif /* KERNEL_INCLUDE_TYPES_H */
```

### Cross-Compiler Requirements

The build system MUST use i686-elf-gcc cross-compiler, NOT the host gcc.

**Required Variables in config.mk:**
```makefile
CROSS := i686-elf-
CC := $(CROSS)gcc
AS := $(CROSS)as
LD := $(CROSS)ld
OBJCOPY := $(CROSS)objcopy
```

**Required CFLAGS:**
```makefile
CFLAGS := -m32 -std=gnu99 -ffreestanding -nostdlib
CFLAGS += -fno-builtin -fno-stack-protector -fno-pic
CFLAGS += -Wall -Wextra -Werror
CFLAGS += -g -O0  # Debug build default
```

### Linker Script Critical Details

**Higher-Half Kernel Layout:**
- Kernel is linked at virtual address 0xC0100000
- Physical load address is 0x100000 (1MB mark)
- The bootloader will set up identity mapping before enabling paging

**Required Symbols to Export:**
- `_kernel_start` - Start of kernel in memory
- `_kernel_end` - End of kernel (for memory manager)
- `_bss_start`, `_bss_end` - For BSS zeroing

**Section Alignment:**
- All sections must be 4KB (0x1000) aligned for paging

### Disk Image Layout

```
Sector 0:     Stage 1 bootloader (512 bytes, MBR)
Sectors 1-N:  Stage 2 bootloader (will be added in Story 1.3)
Sectors N+1:  Kernel binary (will be loaded by Stage 2)
```

For this story, only sector 0 needs valid content. The image can be padded.

### MBR Boot Signature

The stage1 bootloader MUST:
- Be exactly 512 bytes
- Have bytes 510-511 set to 0x55, 0xAA (little-endian: 0xAA55)
- Start executing at 0x7C00 when BIOS loads it

**Minimal stage1.S for this story:**
```asm
.code16
.global _start

_start:
    cli                 /* Disable interrupts */
    hlt                 /* Halt - placeholder for Story 1.2 */
    jmp _start          /* Loop if somehow resumed */

.org 510
.word 0xAA55            /* Boot signature */
```

### Project Structure Notes

**Directory Layout (from Architecture):**
```
os-dev/
├── boot/                    # Bootloader (Milestone 1)
│   ├── stage1.S             # Stage 1: MBR, 512 bytes
│   └── boot.ld              # Bootloader linker script (if needed)
├── kernel/
│   ├── init/                # Kernel entry and setup
│   │   └── main.c           # kmain() entry point
│   ├── mm/                  # Memory management (empty for now)
│   ├── proc/                # Process management (empty for now)
│   ├── syscall/             # System calls (empty for now)
│   ├── drivers/             # Device drivers (empty for now)
│   ├── fs/                  # File system (empty for now)
│   ├── lib/                 # Kernel utility functions (empty for now)
│   └── include/             # Kernel headers
│       └── types.h          # Basic type definitions
├── libc/                    # Minimal C library (empty for now)
├── user/                    # User programs (empty for now)
├── tools/                   # Host-side tools (empty for now)
├── scripts/
│   ├── kernel.ld            # Kernel linker script
│   └── gdbinit              # GDB initialization (optional)
├── docs/                    # Per-milestone documentation
├── build/                   # Build output (gitignored)
├── Makefile                 # Top-level Makefile
├── config.mk                # Build configuration
└── .gitignore
```

### QEMU Command Line

**Standard run:**
```bash
qemu-system-i386 -drive file=build/os-dev.img,format=raw -serial stdio
```

**Debug mode:**
```bash
qemu-system-i386 -drive file=build/os-dev.img,format=raw -serial stdio -s -S
```
- `-s`: Start GDB server on port 1234
- `-S`: Freeze CPU at startup (wait for GDB)
- `-serial stdio`: Route serial output to terminal

### References

- [Source: _bmad-output/planning-artifacts/architecture.md#Project-Structure]
- [Source: _bmad-output/planning-artifacts/architecture.md#Build-System]
- [Source: _bmad-output/planning-artifacts/architecture.md#Implementation-Patterns]
- [Source: _bmad-output/planning-artifacts/prd.md#Build-System]
- [Source: _bmad-output/project-context.md#Build-Commands]
- [Source: _bmad-output/project-context.md#Critical-C-Rules]

## Dev Agent Record

### Agent Model Used

Claude Opus 4.5 (claude-opus-4-5-20251101)

### Completion Notes List

- Created full directory structure: boot/, kernel/, libc/, user/, scripts/, tools/, docs/, build/
- Kernel subdirectories: init/, lib/, include/, mm/, proc/, drivers/, syscall/, fs/
- User subdirectories: shell/, edit/, bin/
- Created config.mk with i686-elf cross-compiler toolchain settings
- Created kernel linker script at scripts/kernel.ld with higher-half layout (0xC0100000)
- Created top-level Makefile with targets: all, image, qemu, debug, clean
- Created minimal kernel stub (kernel/init/main.c) with kmain() entry point
- Created types.h with fixed-width integer types
- Created stage1 bootloader using GAS syntax (NASM not available)
- Boot signature 0xAA55 verified at offset 510
- All acceptance criteria verified:
  - AC1: `make` compiles without errors
  - AC2: `make image` creates build/os-dev.img
  - AC3: `make qemu` launches QEMU
  - AC4: `make debug` launches with -s -S flags
  - AC5: Directory structure complete

### Change Log

| Date | Task | Status | Notes |
|------|------|--------|-------|
| 2026-01-14 | Story 1.1 | Complete | Initial project structure and build system |

### File List

**New Files:**
- .gitignore
- config.mk
- Makefile
- scripts/kernel.ld
- kernel/include/types.h
- kernel/init/main.c
- boot/stage1.S

**New Directories:**
- boot/
- kernel/init/, kernel/lib/, kernel/include/, kernel/mm/, kernel/proc/, kernel/drivers/, kernel/syscall/, kernel/fs/
- libc/
- user/shell/, user/edit/, user/bin/
- scripts/
- tools/
- docs/
- build/ (gitignored)
