---
project_name: 'os-dev'
user_name: 'Thomas'
date: '2026-01-12'
sections_completed: ['technology_stack', 'c_rules', 'assembly_rules', 'memory_rules', 'testing_rules', 'logging_rules', 'critical_donts', 'build_commands']
source_document: '_bmad-output/planning-artifacts/architecture.md'
---

# Project Context for AI Agents

_Critical rules for implementing os-dev. Read before writing any code._

---

## Technology Stack

| Component | Version/Spec |
|-----------|--------------|
| Language | C99 (with GNU extensions) |
| Assembly | AT&T syntax (GAS) |
| Target | i386 (32-bit), higher-half kernel at 0xC0000000 |
| Toolchain | i686-elf-gcc cross-compiler |
| Build | GNU Make (non-recursive) |
| Emulator | QEMU (qemu-system-i386) |
| Debugger | GDB with QEMU stub |

---

## Critical C Rules

**Naming (Linux kernel style):**
- Functions/variables: `snake_case`
- Structs: `struct snake_case` (not typedef)
- Constants/macros: `UPPER_SNAKE`
- Header guards: `PATH_FILENAME_H` (e.g., `KERNEL_MM_PMM_H`)

**Style:**
- 4-space indentation (no tabs)
- K&R braces: same line for control, new line for function definitions
- Pointer style: `int *ptr` (asterisk with variable)
- Line limit: ~80 characters

**Headers:**
- Order: own header → kernel-wide → subsystem
- Structure: includes → constants → types → function declarations
- Always use full path guards

**Error Handling:**
- Return negative errno on error: `-ENOMEM`, `-EINVAL`, `-EFAULT`
- Zero or positive = success
- Panic only for unrecoverable kernel invariant violations

---

## Critical Assembly Rules

**AT&T Syntax:**
```asm
movl %eax, %ebx    /* source, destination */
movl $0x10, %eax   /* $ for immediates */
movl (%ebx), %eax  /* () for memory access */
```

**Documentation Required:**
Every assembly function MUST document:
- Purpose
- Input registers
- Output registers
- Clobbered registers

**Labels:**
- Global: `snake_case` (e.g., `context_switch`)
- Local: `.prefixed` (e.g., `.loop_start`)

---

## Memory Rules

**No Standard Library:**
- No `malloc`/`free` until heap is implemented (Milestone 4)
- No `printf` until `printk` is implemented
- No `memcpy`/`memset` until kernel lib is written

**Memory Layout:**
- Kernel space: 0xC0000000 and above
- User space: 0x00000000 to 0xBFFFFFFF
- Kernel stack: allocated per-process

**Page Management:**
- PAGE_SIZE = 4096 bytes
- Use bitmap allocator for physical frames
- All page tables must be page-aligned

---

## Testing Rules

**In-Kernel Tests:**
- Each subsystem provides `test_<subsystem>()` function
- Called via `make test` target
- Output format: `[PASS] test_name` or `[FAIL] test_name: reason`
- Use serial output for test results

**Host-Side Tests:**
- Pure algorithms only (bitmap ops, string functions)
- Compile with host gcc, not cross-compiler
- Located in `tests/` directory

---

## Logging Rules

**printk Levels:**

| Level | Usage |
|-------|-------|
| `LOG_ERROR` | Failures requiring attention |
| `LOG_WARN` | Unexpected but handled |
| `LOG_INFO` | Significant events |
| `LOG_DEBUG` | Detailed tracing |

**Usage:**
```c
printk(LOG_INFO, "PMM: %d pages free\n", free_count);
```

---

## Critical Don'ts

**NEVER:**
- Use floating point in kernel (no FPU context save)
- Call user-space functions from kernel
- Dereference user pointers without validation
- Assume interrupts are enabled/disabled
- Use `goto` except for error cleanup
- Allocate large arrays on stack (kernel stack is small)

**ALWAYS:**
- Save/restore interrupt state around critical sections
- Check return values from allocation functions
- Use `volatile` for hardware registers
- Document any inline assembly with clobber lists
- Validate user pointers before dereferencing

---

## Build Commands

| Command | Purpose |
|---------|---------|
| `make` | Build everything |
| `make qemu` | Build and run in QEMU |
| `make debug` | Build and run with GDB stub |
| `make test` | Run in-kernel tests |
| `make clean` | Remove build artifacts |

---

## File References

- **Architecture Document:** `_bmad-output/planning-artifacts/architecture.md`
- **PRD:** `_bmad-output/planning-artifacts/prd.md`
- **Product Brief:** `_bmad-output/planning-artifacts/product-brief-os-dev-2026-01-11.md`
