# os-dev Test Framework

This directory contains the test infrastructure for os-dev, split into two layers:

1. **Host-side tests** (`tests/host/`) - Pure algorithm tests running on the development machine
2. **In-kernel tests** (`kernel/test/`) - Subsystem tests running in QEMU

## Quick Start

```bash
# Run host-side tests (fast, no QEMU needed)
cd tests && make

# Run in-kernel tests (requires QEMU)
make test  # from project root, once build system is set up
```

---

## Host-Side Tests

### Overview

Host-side tests use the [Unity](https://github.com/ThrowTheSwitch/Unity) testing framework to test pure algorithms that don't depend on kernel hardware access. These tests:

- Run natively on Linux/macOS
- Compile with host `gcc`, not the cross-compiler
- Execute in milliseconds for fast iteration
- Cover: bitmap operations, string functions, data structures

### Directory Structure

```
tests/
├── host/
│   ├── unity/           # Unity test framework
│   │   ├── unity.c
│   │   ├── unity.h
│   │   └── unity_internals.h
│   ├── test_example.c   # Example/template test
│   ├── test_gdt.c       # GDT encoding tests (kernel-linked)
│   └── test_string.c    # String function tests (add when implemented)
├── Makefile             # Host test build
└── README.md            # This file
```

### Running Tests

```bash
cd tests

# Build and run all tests
make

# Build without running
make build

# Run specific test
make test_bitmap
./test_bitmap

# Clean build artifacts
make clean
```

### Writing New Tests

1. Create `tests/host/test_NAME.c`:

```c
#include "unity/unity.h"

void setUp(void) { }
void tearDown(void) { }

void test_my_function(void)
{
    TEST_ASSERT_EQUAL_INT(expected, actual);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_my_function);
    return UNITY_END();
}
```

2. Run `make` - the Makefile auto-discovers `test_*.c` files

### Kernel-Linked Tests

Some tests need to link against actual kernel code (not just headers). For example,
`test_gdt.c` tests the real `gdt_set_gate` implementation from `kernel/init/gdt.c`.

To create a kernel-linked test:

1. Create `tests/host/test_NAME.c` as usual
2. Add to `tests/Makefile`:
   ```makefile
   KERNEL_SRCS_NAME = ../kernel/path/to/source.c
   ```

The build system will automatically link the specified kernel sources when building
that test. Multiple kernel sources can be listed space-separated.

**Example:**
```makefile
# In tests/Makefile
KERNEL_SRCS_gdt = ../kernel/init/gdt.c
KERNEL_SRCS_idt = ../kernel/init/idt.c ../kernel/lib/string.c
```

**Note:** Kernel code must be compilable with the host compiler. Use conditional
compilation (`#if defined(__STDC_HOSTED__)`) to exclude hardware-dependent code.

### Unity Assertions Reference

| Assertion | Usage |
|-----------|-------|
| `TEST_ASSERT_TRUE(cond)` | Condition is true |
| `TEST_ASSERT_FALSE(cond)` | Condition is false |
| `TEST_ASSERT_EQUAL_INT(exp, act)` | Integers equal |
| `TEST_ASSERT_EQUAL_HEX32(exp, act)` | 32-bit hex equal |
| `TEST_ASSERT_NULL(ptr)` | Pointer is NULL |
| `TEST_ASSERT_NOT_NULL(ptr)` | Pointer is not NULL |
| `TEST_ASSERT_EQUAL_STRING(exp, act)` | Strings equal |
| `TEST_ASSERT_EQUAL_MEMORY(exp, act, len)` | Memory blocks equal |
| `TEST_ASSERT_GREATER_THAN(threshold, actual)` | actual > threshold |
| `TEST_ASSERT_LESS_THAN(threshold, actual)` | actual < threshold |

See [Unity documentation](https://github.com/ThrowTheSwitch/Unity/blob/master/docs/UnityAssertionsReference.md) for full reference.

---

## In-Kernel Tests

### Overview

In-kernel tests run inside QEMU and test subsystems that require hardware access or kernel state. These tests:

- Execute during kernel boot when `TEST_MODE` is defined
- Output results via serial port in `[PASS]/[FAIL]` format
- Test: PMM, VMM, scheduler, context switch, syscalls

### Directory Structure

```
kernel/
├── include/
│   └── test.h           # Test assertion macros
└── test/
    ├── test_runner.c    # Test harness and orchestration
    ├── test_example.c   # Example/template test
    ├── test_pmm.c       # Physical memory tests (add for Milestone 3)
    └── test_vmm.c       # Virtual memory tests (add for Milestone 4)
```

### Build Integration

In the root Makefile (once implemented):

```makefile
# Test mode: compile with TEST_MODE, output via serial
test: CFLAGS += -DTEST_MODE
test: all
    qemu-system-i386 -nographic -serial mon:stdio \
        -drive file=$(BUILD)/os-dev.img,format=raw
```

### Writing New Tests

1. Create `kernel/test/test_SUBSYSTEM.c`:

```c
#ifdef TEST_MODE

#include <test.h>

void test_pmm(void)
{
    TEST_BEGIN("pmm");

    uint32_t frame = pmm_alloc_frame();
    TEST_ASSERT_NOT_NULL(frame);

    pmm_free_frame(frame);
    uint32_t reused = pmm_alloc_frame();
    TEST_ASSERT_EQ(frame, reused);

    TEST_END();
}

#endif /* TEST_MODE */
```

2. Register in `kernel/test/test_runner.c`:

```c
extern void test_pmm(void);

void test_run_all(void)
{
    /* ... */
    test_pmm();
    /* ... */
}
```

### In-Kernel Assertions Reference

| Macro | Usage |
|-------|-------|
| `TEST_BEGIN(suite)` | Start test suite |
| `TEST_END()` | End test suite |
| `TEST_ASSERT(cond)` | Condition is true |
| `TEST_ASSERT_EQ(exp, act)` | Values equal |
| `TEST_ASSERT_NEQ(a, b)` | Values not equal |
| `TEST_ASSERT_NULL(ptr)` | Pointer is NULL |
| `TEST_ASSERT_NOT_NULL(ptr)` | Pointer is not NULL |
| `TEST_ASSERT_GT(a, b)` | a > b |
| `TEST_ASSERT_LT(a, b)` | a < b |
| `TEST_FAIL(msg)` | Unconditional failure |
| `TEST_SKIP(reason)` | Skip test |

### Output Format

```
========================================
       OS-DEV KERNEL TEST SUITE
========================================

[pmm] Running tests...
[PASS] pmm_alloc_frame returns non-null
[PASS] frames are unique
[FAIL] pmm_free_frame: expected 0, got 1 (test_pmm.c:42)
[pmm] 2 passed, 1 failed

========================================
  TOTAL: 2 passed, 1 failed
========================================

*** TESTS FAILED ***
```

---

## Test Strategy by Milestone

| Milestone | Host Tests | In-Kernel Tests |
|-----------|------------|-----------------|
| 3. PMM | `test_bitmap.c` | `test_pmm.c` |
| 4. Paging | - | `test_vmm.c` |
| 5-6. Processes | - | `test_sched.c` |
| Kernel lib | `test_string.c` | - |

### Test Coverage Goals

- **PMM**: Alloc/free correctness, bitmap operations, memory exhaustion
- **VMM**: Page mapping, unmapping, address space creation
- **Scheduler**: Round-robin fairness, context switch preservation
- **String functions**: memcpy, memset, strlen edge cases

---

## Conditional Compilation

All test code is stripped from release builds:

```c
#ifdef TEST_MODE
/* Test code here - only compiled when TEST_MODE is defined */
#endif
```

Build commands:
- `make` - Release build (no test code)
- `make test` - Test build (TEST_MODE defined)

---

## Adding Tests for New Subsystems

1. **Decide test layer:**
   - Pure algorithm with no hardware deps → Host test
   - Requires kernel state/hardware → In-kernel test

2. **Create test file** in appropriate directory

3. **Follow naming convention:** `test_SUBSYSTEM.c`

4. **For in-kernel tests:** Register in `test_runner.c`

5. **Run tests** to verify they pass

---

## Troubleshooting

### Host tests won't compile

```bash
# Check gcc is available
gcc --version

# Ensure Unity files exist
ls tests/host/unity/

# Build with verbose output
make V=1
```

### In-kernel tests hang

- Check serial output is configured (`-serial mon:stdio`)
- Verify `printk` is working before test framework
- Add debug output in `test_runner.c`

### Test output garbled

- Ensure serial baud rate matches (38400)
- Check VGA driver isn't interfering with serial
