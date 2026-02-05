# Story 3.3: Page Fault Handler

Status: done

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a developer,
I want page faults to display diagnostic information,
so that I can debug memory issues and understand MMU behavior.

## Acceptance Criteria

1. **AC1: CR2 Address Capture**
   - Given IDT is initialized with page fault handler (from Epic 2)
   - When page fault exception (INT 14) fires
   - Then handler reads faulting address from CR2
   - And error code is parsed for fault reason

2. **AC2: Diagnostic Output**
   - Given page fault occurs
   - When handler executes
   - Then output shows: "PAGE FAULT at 0x[address]"
   - And output shows: "Error: [read/write] [user/kernel] [present/not-present]"
   - And faulting instruction (EIP) is displayed
   - And output goes to both serial and VGA

3. **AC3: Kernel Fault Panic**
   - Given page fault in kernel mode
   - When fault is not recoverable
   - Then system panics with full diagnostic info

4. **AC4: Error Code Parsing**
   - Given page fault error code
   - When I examine the bits
   - Then bit 0 indicates present (1) or not-present (0)
   - And bit 1 indicates write (1) or read (0)
   - And bit 2 indicates user mode (1) or kernel mode (0)

5. **AC5: Code Quality**
   - Given kernel/init/isr.c (or kernel/mm/fault.c)
   - When I examine page fault handler
   - Then CR2 is read immediately (before any memory access that could overwrite it)
   - And read_cr2() from asm.h is used (not raw inline assembly)
   - And NFR10 (report faulting address and access type) is satisfied

## Tasks / Subtasks

- [x] **Task 1: Refactor page_fault_handler() to use read_cr2()** (AC: #1, #5)
  - [x] 1.1 In `kernel/init/isr.c`, replace inline `__asm__ volatile ("movl %%cr2, %0"...)` with `read_cr2()` from asm.h
  - [x] 1.2 Ensure `#include <asm.h>` is present in isr.c
  - [x] 1.3 Verify read_cr2() is called FIRST in the handler before any other memory operations

- [x] **Task 2: Update diagnostic output format** (AC: #2)
  - [x] 2.1 Change output header to: `"PAGE FAULT at 0x%x\n"` matching AC format exactly
  - [x] 2.2 Add consolidated error line: `"Error: %s %s %s\n"` with read/write, user/kernel, present/not-present
  - [x] 2.3 Display faulting EIP: `"Faulting EIP: 0x%x\n"` from regs->eip
  - [x] 2.4 Ensure output goes to both serial and VGA (printk already does this)

- [x] **Task 3: Improve error code bit parsing** (AC: #4)
  - [x] 3.1 Define PF_ERR_PRESENT (0x01), PF_ERR_WRITE (0x02), PF_ERR_USER (0x04), PF_ERR_RSVD (0x08), PF_ERR_IFETCH (0x10) as named constants
  - [x] 3.2 Use named constants instead of magic numbers for bit checks
  - [x] 3.3 Add documentation comment explaining each error code bit

- [x] **Task 4: Distinguish kernel vs user mode faults** (AC: #3)
  - [x] 4.1 If fault is in kernel mode (!user bit), call panic() with full context
  - [x] 4.2 If fault is in user mode (user bit set), also panic for now (but add TODO comment for future user-mode fault handling in Epic 5+)
  - [x] 4.3 Print faulting address and error code before panic (panic() takes const char* only)

- [x] **Task 5: Add page fault tests** (AC: #1-5)
  - [x] 5.1 Create `kernel/test/test_fault.c`
  - [x] 5.2 Test: Access to unmapped address triggers page fault handler (requires temporary fault capture mechanism)
  - [x] 5.3 Test: Verify page fault handler runs and captures correct CR2 address
  - [x] 5.4 Test: Verify error code bits are parsed correctly for not-present read fault
  - [x] 5.5 Add test_fault() to test_runner.c
  - [x] 5.6 **APPROACH**: Install a temporary page fault handler that records CR2 and error_code into globals, then returns (instead of panicking). After test, restore original behavior. This allows testing without crashing.

- [x] **Task 6: Integration and verification** (AC: #1-5)
  - [x] 6.1 Run `make test` - all existing tests pass (no regressions)
  - [x] 6.2 Run `make qemu` - kernel boots normally, no spurious page faults
  - [x] 6.3 Verify diagnostic output format by triggering a deliberate page fault in test

---

## Dev Notes

### What This Story Accomplishes

This is **Story 3.3 in Epic 3** (Memory Management). After this:
- Page fault handler provides clear, formatted diagnostic output
- Developers can debug memory issues by examining fault reports
- Foundation for future demand paging and user-mode fault handling exists
- Story 3.4 (Kernel Heap) can rely on page faults being handled meaningfully

### CRITICAL: Existing Implementation Already Exists

**The page fault handler is NOT new code** - it was already implemented in Story 2.1 (IDT Setup & Exception Handlers) at `kernel/init/isr.c` lines ~81-128. The existing `page_fault_handler()` function already:
- Reads CR2 (via inline asm, not the read_cr2() helper)
- Parses all 5 error code bits (present, write, user, rsvd, ifetch)
- Prints diagnostic output
- Calls `panic("Page fault in kernel")`

**This story REFINES the existing handler**, it does NOT create a new one from scratch. Key changes:
1. Replace inline asm with `read_cr2()` from asm.h
2. Standardize output format to match AC specifications
3. Add named constants for error code bits (remove magic numbers)
4. Add proper test coverage
5. Distinguish kernel vs user fault paths (both panic for now)

### Existing Exception Handler Architecture

**ISR dispatch flow:**
```
Exception 14 → isr14 stub (isr_stubs.S)
  → pushes err_code (CPU already pushed it)
  → pushes int_no (14)
  → jmp isr_common
    → pushal (saves registers)
    → pushes segment registers
    → calls isr_handler(regs)
      → checks regs->int_no == EXC_PAGE_FAULT (14)
      → calls page_fault_handler(regs)
```

**Key: Exception 14 is an error-code exception.** The CPU pushes the error code onto the stack automatically. The ISR stub macro `ISR_ERRCODE 14` does NOT push a dummy 0.

### struct registers Layout

From `kernel/include/isr.h` - this is what the handler receives:

```c
struct registers {
    /* Pushed by isr_common */
    uint32_t gs, fs, es, ds;
    /* From pushal */
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    /* Pushed by ISR stub */
    uint32_t int_no;
    uint32_t err_code;    /* For PF: CPU-pushed error code */
    /* Pushed by CPU */
    uint32_t eip;         /* Faulting instruction */
    uint32_t cs;
    uint32_t eflags;
    /* Only on privilege change (ring 3 -> ring 0) */
    uint32_t useresp;
    uint32_t ss;
};
```

### Page Fault Error Code Bit Layout

```
Bit 0 (P):    0 = Not-present page, 1 = Protection violation
Bit 1 (W/R):  0 = Read access, 1 = Write access
Bit 2 (U/S):  0 = Kernel mode, 1 = User mode
Bit 3 (RSVD): 0 = Normal, 1 = Reserved bit violation in PTE
Bit 4 (I/D):  0 = Not instruction fetch, 1 = Instruction fetch
```

**Named constants to define:**
```c
#define PF_ERR_PRESENT  0x01  /* Page was present (protection fault) */
#define PF_ERR_WRITE    0x02  /* Write access caused fault */
#define PF_ERR_USER     0x04  /* Fault occurred in user mode */
#define PF_ERR_RSVD     0x08  /* Reserved bit set in page entry */
#define PF_ERR_IFETCH   0x10  /* Instruction fetch caused fault */
```

### Testing Strategy: Recoverable Page Fault Testing

**Challenge:** Page faults currently cause a panic, which halts the system. Tests need to trigger page faults and verify the handler works WITHOUT crashing.

**Solution: Temporary test fault handler**

```c
/* In test_fault.c */
static volatile uint32_t test_fault_addr;
static volatile uint32_t test_fault_err;
static volatile int test_fault_triggered;

/*
 * Temporary page fault handler for testing.
 * Records fault info and skips the faulting instruction.
 */
static void test_page_fault_handler(struct registers *regs)
{
    test_fault_addr = read_cr2();
    test_fault_err = regs->err_code;
    test_fault_triggered = 1;

    /* Skip the faulting instruction by advancing EIP */
    /* The faulting instruction is a read (movl), typically 2-3 bytes */
    regs->eip += 2;  /* Skip past faulting mov instruction */
}
```

**Alternative approach (simpler):** Use vmm_map_page/vmm_unmap_page to test that unmapping works and verify the handler's output by checking global variables, then re-map before continuing. Or test by allocating a page, unmapping it, and verifying the fault is captured.

**Note:** Skipping instructions in the fault handler is fragile since instruction lengths vary. A safer approach:
1. Map a test page
2. Write a known value
3. Unmap the page
4. Install test handler that records fault info and remaps the page
5. Access the address (triggers fault → handler remaps → access succeeds on retry)

### read_cr2() Usage

The `read_cr2()` inline function already exists in `kernel/include/asm.h` (added in Story 3.2):

```c
static inline uint32_t read_cr2(void)
{
    uint32_t val;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(val));
    return val;
}
```

The current page_fault_handler() in isr.c uses raw inline asm instead. This story should switch to using the helper.

### Previous Story Intelligence

**From Story 3.2 (Paging & Virtual Memory):**
- `vmm_map_page(virt, phys, flags)` and `vmm_unmap_page(virt)` are available
- `vmm_get_physaddr(virt)` for virtual-to-physical translation
- `invlpg(addr)` for TLB invalidation
- `P2V()` / `V2P()` macros for address conversion
- All 394 tests pass with no regressions after Story 3.2
- Boot page tables map first 16MB for both identity and higher-half

**From Story 3.1 (PMM):**
- `pmm_alloc_frame()` / `pmm_free_frame()` for frame allocation
- `memset()` available for zeroing
- Test pattern: `TEST_ASSERT(condition, "test_name")` macro
- Test registration in `kernel/test/test_runner.c`

**Debug log learnings from Story 3.1:**
- printk format specifiers: don't use width specifiers (`%08x`), use `%x` instead
- Use `%X` for uppercase hex if needed

### Git Intelligence

**Recent commits:**
```
e345118 feat[story 3.2]: paging virtual memory.
1777f87 feat[story 3.1]: physical memory manager.
5dbe8a0 doc[epic 2]: retrospecive.
```

**Expected commit for this story:**
```
feat[story 3.3]: page fault handler.
```

**Files likely to be modified:**
- `kernel/init/isr.c` - Refactor page_fault_handler()
- `kernel/test/test_fault.c` - NEW: Page fault tests
- `kernel/test/test_runner.c` - Add test_fault() call

**Files NOT expected to change:**
- `kernel/init/isr_stubs.S` - ISR stubs already correct
- `kernel/include/isr.h` - struct registers already correct
- `kernel/include/asm.h` - read_cr2() already exists
- `kernel/mm/vmm.c` - No VMM changes needed

### Critical Architecture Constraints

**From Architecture Document:**
- Error handling: panic on kernel invariant violations
- Logging: printk with LOG_ERROR for failures
- Naming: snake_case functions, UPPER_SNAKE constants
- Style: K&R braces, 4-space indent
- Comments: Block comments for function documentation

**From Project Context:**
- Always save/restore interrupt state around critical sections
- Use `volatile` for hardware-related variables
- Document inline assembly with clobber lists
- No floating point in kernel
- Line limit ~80 characters

### Scope Boundaries

**IN SCOPE (this story):**
- Refine page_fault_handler() output format
- Add named constants for error code bits
- Use read_cr2() helper
- Add test coverage
- Kernel-mode faults → panic

**OUT OF SCOPE (future stories):**
- Demand paging (allocate on fault)
- User-mode fault handling (signal delivery)
- Copy-on-write (CoW) for fork()
- Stack growth on fault
- Memory-mapped files

### Project Structure Notes

- Handler stays in `kernel/init/isr.c` (not moved to separate file) since it's integrated with the ISR dispatch
- PF_ERR_* constants can be added to `kernel/include/isr.h` alongside struct registers
- Test file follows existing pattern: `kernel/test/test_fault.c`
- No new directories needed

### References

- [Source: _bmad-output/planning-artifacts/architecture.md#Error-Handling-Strategy]
- [Source: _bmad-output/planning-artifacts/architecture.md#Implementation-Patterns]
- [Source: _bmad-output/planning-artifacts/epics.md#Story-3.3]
- [Source: _bmad-output/project-context.md#Memory-Rules]
- [Source: _bmad-output/project-context.md#Testing-Rules]
- [Source: kernel/init/isr.c#page_fault_handler - existing implementation]
- [Source: kernel/include/asm.h#read_cr2 - CR2 helper function]
- [Source: kernel/include/isr.h#struct-registers - interrupt frame layout]
- [Source: kernel/init/isr_stubs.S - ISR stub macros]
- [Source: Intel SDM Vol 3 Section 4.7 - Page-Fault Exceptions]

---

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- No debug issues encountered. Build passed clean with -Werror on first attempt.

### Completion Notes List

- Replaced raw inline asm `__asm__ volatile ("movl %%cr2, %0"...)` with `read_cr2()` from asm.h; read_cr2() remains the very first call in page_fault_handler().
- Reformatted diagnostic output to match AC2 exactly: "PAGE FAULT at 0x[addr]", "Error: read/write user/kernel present/not-present", "Faulting EIP: 0x[eip]".
- Added PF_ERR_PRESENT/WRITE/USER/RSVD/IFETCH named constants to isr.h with Intel SDM documentation comment.
- Replaced all magic number bit checks with named constants.
- Added kernel vs user mode branching: kernel faults panic, user faults also panic for now with TODO(Epic 5+) comment.
- Implemented test hook mechanism: pf_set_test_hook() in isr.c (TEST_MODE only) allows tests to intercept page faults before the real handler runs.
- Test strategy: allocate frame, map page, unmap it, install test hook that records CR2/error_code and remaps the page, trigger fault by reading unmapped address, verify captured values. Avoids fragile EIP-skipping approach.
- 14 test assertions across 4 test functions (5 constant validation + 2 CR2 capture + 3 read error code + 4 write error code).
- All 408 tests pass (394 existing + 14 new). No regressions.
- Kernel boots normally with no spurious page faults.

### Change Log

- 2026-02-05: Story 3.3 implemented - page fault handler refined with read_cr2(), AC-compliant output format, named constants, kernel/user fault paths, and 10 new tests.
- 2026-02-05: Code review fixes - restored interrupt frame register dump, declared pf_set_test_hook() in isr.h, added write fault test, clarified task 4.3 wording. Bootloader fix: KERNEL_SECTORS 64→128 and chunk limit 64→128 to accommodate larger kernel binary.

### File List

- `kernel/init/isr.c` - Refactored page_fault_handler(): read_cr2(), named constants, AC2 output format, kernel/user branching, TEST_MODE hook, register dump
- `kernel/include/isr.h` - Added PF_ERR_PRESENT/WRITE/USER/RSVD/IFETCH constants, pf_set_test_hook() declaration under TEST_MODE
- `kernel/test/test_fault.c` - NEW: 4 test functions with 14 assertions (CR2 capture, read/write error code parsing, constant validation)
- `kernel/test/test_runner.c` - Added test_fault() registration
- `boot/stage2.S` - KERNEL_SECTORS 64→128, chunk read limit 64→128 (kernel exceeded 32KB)
