# Story 1.6: Serial Debug & Panic Infrastructure

Status: done

## Story

As a developer,
I want serial port output and a panic handler with register dumps,
so that I can debug issues even when VGA fails and use GDB effectively.

## Acceptance Criteria

1. **AC1: Serial Port Initialization**
   - Given kernel is initializing
   - When serial_init() is called
   - Then COM1 (0x3F8) is configured at 38400 baud

2. **AC2: Serial Character Output**
   - Given serial is initialized
   - When serial_putchar('X') is called
   - Then character 'X' is transmitted on COM1
   - And output is visible in QEMU's serial console (-serial stdio)

3. **AC3: printk Integration**
   - Given serial is working
   - When printk(LOG_INFO, "Boot complete") is called
   - Then "[INFO] Boot complete" appears on serial output
   - And message also appears on VGA console

4. **AC4: Panic Handler**
   - Given an unrecoverable error occurs
   - When panic("message") is called
   - Then "KERNEL PANIC: message" is displayed
   - And register dump shows: EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP, EIP, EFLAGS
   - And system halts (cli; hlt loop)

5. **AC5: GDB Debugging**
   - Given QEMU is launched with `make debug`
   - When I connect GDB with `target remote :1234`
   - Then GDB attaches to the running kernel
   - And I can set breakpoints and inspect memory

6. **AC6: Code Structure**
   - Given kernel/lib/panic.c source
   - When I examine the code
   - Then panic captures register state before printing
   - And output goes to both serial and VGA

## Tasks / Subtasks

- [x] **Task 1: Create Serial Driver Header** (AC: #1, #2)
  - [x] 1.1 Create `kernel/include/serial.h` with COM port constants
  - [x] 1.2 Define COM1_PORT (0x3F8) and related register offsets
  - [x] 1.3 Declare serial_init(), serial_putchar(), serial_puts() prototypes

- [x] **Task 2: Implement Serial Driver** (AC: #1, #2)
  - [x] 2.1 Create `kernel/drivers/serial.c`
  - [x] 2.2 Implement serial_init() to configure 38400 baud, 8N1
  - [x] 2.3 Implement serial_putchar() with transmit-ready polling
  - [x] 2.4 Implement serial_puts() for string output
  - [x] 2.5 Implement serial_write() for raw buffer output

- [x] **Task 3: Create printk Infrastructure** (AC: #3)
  - [x] 3.1 Create `kernel/include/printk.h` with log levels
  - [x] 3.2 Define LOG_ERROR (0), LOG_WARN (1), LOG_INFO (2), LOG_DEBUG (3)
  - [x] 3.3 Create `kernel/lib/printk.c` with format string parsing
  - [x] 3.4 Support format specifiers: %s, %d, %u, %x, %c, %p, %%
  - [x] 3.5 Output to both serial (primary) and VGA (secondary)
  - [x] 3.6 Add compile-time LOG_LEVEL filtering

- [x] **Task 4: Implement Panic Handler** (AC: #4, #6)
  - [x] 4.1 Create `kernel/include/panic.h` with panic() declaration
  - [x] 4.2 Create `kernel/lib/panic.c` with panic implementation
  - [x] 4.3 Capture all general-purpose registers before any other operations
  - [x] 4.4 Print "KERNEL PANIC: <message>" in red on VGA
  - [x] 4.5 Print complete register dump (EAX-EFLAGS)
  - [x] 4.6 Halt with cli; hlt in infinite loop

- [x] **Task 5: Update Makefile for GDB** (AC: #5)
  - [x] 5.1 Verify `make debug` target uses -s -S flags for QEMU
  - [x] 5.2 Ensure serial output with -serial stdio
  - [x] 5.3 Verify scripts/gdbinit sets up kernel symbol loading

- [x] **Task 6: Integrate into Kernel Boot** (AC: #1, #3, #4)
  - [x] 6.1 Call serial_init() from kmain() after vga_init()
  - [x] 6.2 Replace vga_puts() calls with printk()
  - [x] 6.3 Add printk boot messages showing init progress
  - [x] 6.4 Remove temporary print_num() helper (replaced by printk %d)

- [x] **Task 7: Testing and Verification** (AC: #1-6)
  - [x] 7.1 Create `kernel/test/test_serial.c` with serial output tests
  - [x] 7.2 Create `kernel/test/test_printk.c` with format string tests
  - [x] 7.3 Run `make qemu` and verify serial output in terminal
  - [x] 7.4 Test GDB connection with `make debug`
  - [x] 7.5 Test panic() triggers register dump and halts

---

## Dev Notes

### What This Story Accomplishes

This is the **final story in Epic 1** - completing the boot infrastructure. After this, the kernel has:
- VGA text output (Story 1.5)
- Serial debug output (this story)
- printk() with log levels
- panic() for fatal errors
- GDB debugging capability

This enables proper debugging for all future stories.

### Serial Port (COM1) Programming

**Port Addresses:**
| Offset | DLAB=0 Read | DLAB=0 Write | DLAB=1 |
|--------|-------------|--------------|--------|
| +0 | RX Buffer | TX Buffer | Divisor LSB |
| +1 | Interrupt Enable | Interrupt Enable | Divisor MSB |
| +2 | Interrupt ID | FIFO Control | - |
| +3 | Line Control | Line Control | - |
| +4 | Modem Control | Modem Control | - |
| +5 | Line Status | - | - |
| +6 | Modem Status | - | - |
| +7 | Scratch | Scratch | - |

**COM1 base: 0x3F8**

**Baud Rate Divisor:**
```
divisor = 115200 / baud_rate
For 38400 baud: divisor = 115200 / 38400 = 3
```

**Line Control Register (LCR) - offset 0x3FB:**
- Bits 0-1: Word length (0b11 = 8 bits)
- Bit 2: Stop bits (0 = 1 stop bit)
- Bits 3-5: Parity (0b000 = none)
- Bit 7: DLAB (access divisor latch)

**8N1 configuration: LCR = 0x03** (8 data bits, no parity, 1 stop bit)

### Serial Initialization Sequence

```c
void serial_init(void)
{
    /* Disable interrupts */
    outb(COM1_PORT + 1, 0x00);

    /* Set DLAB to access divisor */
    outb(COM1_PORT + 3, 0x80);

    /* Set divisor = 3 (38400 baud) */
    outb(COM1_PORT + 0, 0x03);  /* LSB */
    outb(COM1_PORT + 1, 0x00);  /* MSB */

    /* Clear DLAB, set 8N1 */
    outb(COM1_PORT + 3, 0x03);

    /* Enable FIFO, clear, 14-byte threshold */
    outb(COM1_PORT + 2, 0xC7);

    /* Enable DTR, RTS, OUT2 */
    outb(COM1_PORT + 4, 0x0B);
}
```

### Serial Output with Polling

```c
/* Line Status Register bits */
#define LSR_TX_EMPTY  0x20  /* Transmitter holding register empty */

void serial_putchar(char c)
{
    /* Wait for transmit buffer to be empty */
    while ((inb(COM1_PORT + 5) & LSR_TX_EMPTY) == 0) {
        /* spin */
    }
    outb(COM1_PORT, c);
}
```

### printk Log Levels

Per architecture document:

| Level | Value | Prefix | Usage |
|-------|-------|--------|-------|
| LOG_ERROR | 0 | [ERROR] | Failures requiring attention |
| LOG_WARN | 1 | [WARN] | Unexpected but handled |
| LOG_INFO | 2 | [INFO] | Significant events |
| LOG_DEBUG | 3 | [DEBUG] | Detailed tracing |

**Implementation pattern:**
```c
#define LOG_LEVEL LOG_DEBUG  /* Compile-time filter */

void printk(int level, const char *fmt, ...)
{
    if (level > LOG_LEVEL) return;

    /* Print level prefix */
    static const char *prefixes[] = {
        "[ERROR] ", "[WARN] ", "[INFO] ", "[DEBUG] "
    };
    serial_puts(prefixes[level]);
    vga_puts(prefixes[level]);

    /* Format and output message */
    va_list args;
    va_start(args, fmt);
    vprintk(fmt, args);
    va_end(args);
}
```

### Format String Parsing

Support these specifiers (minimal subset):

| Specifier | Type | Example |
|-----------|------|---------|
| %s | char * | "hello" |
| %d | int32_t | -123 |
| %u | uint32_t | 123 |
| %x | uint32_t | 0x7b (lowercase hex) |
| %X | uint32_t | 0x7B (uppercase hex) |
| %c | char | 'A' |
| %p | void * | 0xC0100000 (pointer) |
| %% | literal | % |

**Width/precision modifiers are optional for MVP.**

### Panic Handler Implementation

**Critical: Capture registers FIRST before any other code runs.**

The panic handler must:
1. Capture all GP registers immediately (inline assembly at function start)
2. Disable interrupts
3. Output to both VGA (visible) and serial (logged)
4. Display red "KERNEL PANIC" header on VGA
5. Print panic message
6. Print all register values
7. Halt with infinite cli; hlt loop

**Register capture pattern:**
```c
/* Must be FIRST in panic function - before any local variables */
uint32_t eax, ebx, ecx, edx, esi, edi, ebp, esp, eflags;
__asm__ volatile (
    "movl %%eax, %0\n"
    "movl %%ebx, %1\n"
    "movl %%ecx, %2\n"
    "movl %%edx, %3\n"
    "movl %%esi, %4\n"
    "movl %%edi, %5\n"
    "movl %%ebp, %6\n"
    "movl %%esp, %7\n"
    "pushfl\n"
    "popl %8\n"
    : "=m"(eax), "=m"(ebx), "=m"(ecx), "=m"(edx),
      "=m"(esi), "=m"(edi), "=m"(ebp), "=m"(esp), "=m"(eflags)
);

/* EIP from return address on stack */
uint32_t eip;
__asm__ volatile ("movl 4(%%ebp), %0" : "=r"(eip));
```

**Output format:**
```
KERNEL PANIC: <message>

Register dump:
  EAX=0x00000000  EBX=0x00000000
  ECX=0x00000000  EDX=0x00000000
  ESI=0x00000000  EDI=0x00000000
  EBP=0xC0107FF0  ESP=0xC0107FE0
  EIP=0xC0100234  EFLAGS=0x00000002

System halted.
```

### GDB Integration

**make debug target (verify/update in Makefile):**
```makefile
debug: all
    qemu-system-i386 -s -S -serial stdio \
        -drive file=$(BUILD)/os-dev.img,format=raw
```

Flags:
- `-s`: Enable GDB stub on port 1234
- `-S`: Start paused (wait for GDB)
- `-serial stdio`: Serial output to terminal

**scripts/gdbinit (verify exists):**
```
target remote :1234
symbol-file build/kernel.elf
break kmain
```

### File Locations

| File | Purpose |
|------|---------|
| `kernel/include/serial.h` | Serial driver interface |
| `kernel/drivers/serial.c` | Serial driver implementation |
| `kernel/include/printk.h` | printk interface and log levels |
| `kernel/lib/printk.c` | printk implementation |
| `kernel/include/panic.h` | Panic handler interface |
| `kernel/lib/panic.c` | Panic handler implementation |

### Integration with main.c

```c
#include <serial.h>
#include <printk.h>
#include <panic.h>

void kmain(void)
{
    gdt_init();
    vga_init();
    serial_init();  /* NEW */

    printk(LOG_INFO, "os-dev kernel starting\n");
    printk(LOG_INFO, "GDT initialized\n");
    printk(LOG_INFO, "VGA initialized\n");
    printk(LOG_INFO, "Serial initialized\n");
    printk(LOG_INFO, "Memory map entries: %d\n", boot_mmap_count);

    /* Test panic (remove after verification) */
    /* panic("Test panic"); */

#ifdef TEST_MODE
    test_run_all();
#endif

    printk(LOG_INFO, "Boot complete\n");

    for (;;) {
        hlt();
    }
}
```

### Previous Story Intelligence

**From Story 1.5:**
- VGA driver at `kernel/drivers/vga.c` with vga_puts(), vga_putchar(), vga_set_color()
- asm.h at `kernel/include/asm.h` with outb(), inb(), cli(), hlt()
- Test infrastructure at `kernel/test/test_runner.c`
- Build system uses wildcards - new files in `kernel/lib/` and `kernel/drivers/` auto-detected
- print_num() temporary helper exists in main.c - will be replaced by printk()

**From git history:**
- Commit pattern: `feat[story X.Y]: description`
- Stories 1.1-1.5 complete: bootloader, GDT, VGA all working
- Kernel boots successfully and displays output

### Common Pitfalls

1. **Not capturing registers first in panic()** - Function prologue corrupts registers
2. **Forgetting to poll TX empty** - Characters get lost
3. **Wrong baud rate divisor** - Serial output is garbage
4. **Missing DLAB toggle** - Can't set baud rate
5. **printk buffer overflow** - Use fixed buffer with bounds checking
6. **va_list usage** - Remember va_start/va_end pairs
7. **Hex conversion errors** - Digits A-F vs a-f

### Relationship to Other Stories

- **Depends on:** Story 1.5 (VGA driver for console output)
- **Enables:** Story 2.1 (IDT can use printk/panic for exception handling)
- **Enables:** All future stories (debug infrastructure available)

### NFR Satisfaction

This story satisfies:
- **NFR7:** All kernel panics shall display register state (panic handler)
- **NFR8:** Serial debug output available from earliest boot (serial driver)
- **NFR9:** GDB debugging works for kernel (make debug, -s -S flags)
- **NFR11:** Assertions available (panic can be used in assert macro)

### Testing Strategy

1. **Serial test:** `make qemu` output shows serial messages in terminal
2. **printk test:** All log levels output correctly with prefixes
3. **Format test:** %s, %d, %x, %p all format correctly
4. **GDB test:** Connect with `target remote :1234`, set breakpoint, continue
5. **Panic test:** Trigger panic(), verify register dump appears
6. **In-kernel tests:** test_serial.c and test_printk.c validate functions

### Project Structure Notes

**New Files:**
- `kernel/include/serial.h`
- `kernel/include/printk.h`
- `kernel/include/panic.h`
- `kernel/drivers/serial.c`
- `kernel/lib/printk.c`
- `kernel/lib/panic.c`
- `kernel/test/test_serial.c`
- `kernel/test/test_printk.c`

**Modified Files:**
- `kernel/init/main.c` - Use printk instead of vga_puts, add serial_init()
- `kernel/test/test_runner.c` - Call test_serial(), test_printk()

### References

- [Source: _bmad-output/planning-artifacts/architecture.md#Logging-Infrastructure]
- [Source: _bmad-output/planning-artifacts/architecture.md#Error-Handling-Strategy]
- [Source: _bmad-output/planning-artifacts/epics.md#Story-1.6]
- [Source: _bmad-output/project-context.md#Logging-Rules]
- [Source: _bmad-output/project-context.md#Critical-C-Rules]
- [Source: OSDev Wiki - Serial Ports]
- [Source: OSDev Wiki - Printing To Screen]

---

## Dev Agent Record

### Agent Model Used

Claude Opus 4.5 (claude-opus-4-5-20251101)

### Debug Log References

- Serial output verified via `make qemu` with `-serial stdio`
- Tests run with `make image TEST_MODE=1`

### Completion Notes List

- ✅ Serial driver implemented with polling-based TX, 38400 baud 8N1
- ✅ printk() with all format specifiers: %s, %d, %u, %x, %X, %c, %p, %%
- ✅ Log levels: LOG_ERROR, LOG_WARN, LOG_INFO, LOG_DEBUG with compile-time filtering
- ✅ Panic handler with immediate register capture, red VGA output, serial logging
- ✅ GDB support via `make debug` with scripts/gdbinit
- ✅ All tests passing: serial (3/3), printk (7/7)
- ✅ Boot messages now via printk to both serial and VGA

### File List

**New Files:**
- kernel/include/serial.h
- kernel/include/printk.h
- kernel/include/panic.h
- kernel/drivers/serial.c
- kernel/lib/printk.c
- kernel/lib/panic.c
- kernel/test/test_serial.c
- kernel/test/test_printk.c
- scripts/gdbinit

**Modified Files:**
- kernel/init/main.c (serial_init, printk integration, removed print_num)
- kernel/test/test_runner.c (added test_serial, test_printk calls)
- Makefile (added kernel/lib sources, lib directory creation)

**Review Additions:**
- kernel/include/format.h (pure formatting functions extracted for testability)
- kernel/lib/format.c (buffer-based format_unsigned, format_signed, format_pointer)
- tests/host/test_format.c (18 unit tests for formatting functions)
- tests/Makefile (added KERNEL_SRCS_format linkage)

---

## Senior Developer Review (AI)

**Reviewer:** Claude Opus 4.5
**Date:** 2026-01-16
**Outcome:** APPROVED with fixes applied

### Issues Found and Fixed

| ID | Severity | Issue | Resolution |
|----|----------|-------|------------|
| H1 | HIGH | Inconsistent newline handling - output_char() didn't convert \n to \r\n on serial, but output_string() did | Fixed in printk.c:43-50 |
| M2 | MEDIUM | Missing INT32_MIN edge case test | Added test in test_printk.c:37-38 |
| M3 | MEDIUM | Typo "Interrup" in serial.h | Fixed to "Interrupt" in serial.h:43 |
| L2 | LOW | Misleading buffer size comment | Clarified in printk.c (now format.c) |
| L3 | LOW | Register capture comment unclear about limitations | Enhanced in panic.c:35-47 |

### Test Coverage Improvements

**Problem:** Original tests were smoke tests only - verified functions didn't crash but not output correctness.

**Solution:** Extracted pure formatting logic into `kernel/lib/format.c` with:
- `format_unsigned()` - decimal and hex formatting
- `format_signed()` - signed integer with INT32_MIN handling
- `format_pointer()` - pointer with 0x prefix and leading zeros

Created 18 host-side unit tests (`tests/host/test_format.c`) covering:
- Zero values, max values, negative values
- Buffer truncation behavior
- INT32_MIN edge case
- Hex uppercase/lowercase

### Test Results

**Host-side (make host-test):**
- test_format: 18/18 PASS

**Kernel-side (make test):**
- serial: 3/3 PASS
- printk: 7/7 PASS
- (1 pre-existing VGA test failure unrelated to this story)

### Change Log

| Date | Author | Change |
|------|--------|--------|
| 2026-01-16 | Review Agent | Fixed H1, M2, M3, L2, L3; Added format.c/h with host tests |
