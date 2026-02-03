# Story 2.2: PIC & Timer Driver

Status: done

## Story

As a developer,
I want the PIC configured and timer firing at regular intervals,
so that I have the foundation for preemptive scheduling.

## Acceptance Criteria

1. **AC1: PIC Initialization**
   - Given kernel is initializing
   - When pic_init() is called
   - Then both PICs (master and slave) are remapped
   - And IRQ 0-7 map to INT 32-39
   - And IRQ 8-15 map to INT 40-47
   - And all IRQs initially masked except those explicitly enabled

2. **AC2: Timer Initialization**
   - Given PIC is initialized
   - When timer_init() is called
   - Then PIT channel 0 is configured for 100Hz (10ms interval)
   - And IRQ 0 (INT 32) is unmasked

3. **AC3: Timer Interrupt Handling**
   - Given timer is running
   - When each timer interrupt fires
   - Then global tick counter increments
   - And EOI is sent to PIC
   - And interrupt returns cleanly

4. **AC4: Tick Counter Accuracy**
   - Given timer is running for 1 second
   - When I check the tick counter
   - Then counter shows approximately 100 ticks

5. **AC5: PIC Code Quality**
   - Given kernel/init/pic.c source
   - When I examine the code
   - Then ICW1-ICW4 initialization sequence is commented
   - And pic_send_eoi() handles both master and slave PIC

6. **AC6: Timer Code Quality**
   - Given kernel/drivers/timer.c source
   - When I examine the code
   - Then PIT divisor calculation is documented
   - And timer_get_ticks() function returns current count

## Tasks / Subtasks

- [x] **Task 1: Create PIC Header** (AC: #1, #5)
  - [x] 1.1 Create `kernel/include/pic.h` with PIC port definitions
  - [x] 1.2 Define PIC1_COMMAND (0x20), PIC1_DATA (0x21)
  - [x] 1.3 Define PIC2_COMMAND (0xA0), PIC2_DATA (0xA1)
  - [x] 1.4 Define ICW1-ICW4 command bytes as named constants
  - [x] 1.5 Declare pic_init(), pic_send_eoi(), pic_set_mask(), pic_clear_mask()

- [x] **Task 2: Implement PIC Driver** (AC: #1, #5)
  - [x] 2.1 Create `kernel/init/pic.c`
  - [x] 2.2 Implement pic_init() with ICW1-ICW4 sequence
  - [x] 2.3 Remap PIC1 (IRQ 0-7) to INT 32-39
  - [x] 2.4 Remap PIC2 (IRQ 8-15) to INT 40-47
  - [x] 2.5 Mask all IRQs initially (set all bits in OCW1)
  - [x] 2.6 Implement pic_send_eoi() - handle slave PIC cascade
  - [x] 2.7 Implement pic_set_mask() and pic_clear_mask() for IRQ enable/disable

- [x] **Task 3: Create Timer Header** (AC: #2, #6)
  - [x] 3.1 Create `kernel/include/timer.h`
  - [x] 3.2 Define PIT ports: PIT_CHANNEL0 (0x40), PIT_CMD (0x43)
  - [x] 3.3 Define PIT_FREQUENCY (1193182 Hz) and TARGET_HZ (100)
  - [x] 3.4 Declare timer_init() and timer_get_ticks()

- [x] **Task 4: Implement Timer Driver** (AC: #2, #3, #4, #6)
  - [x] 4.1 Create `kernel/drivers/timer.c`
  - [x] 4.2 Implement global volatile tick counter
  - [x] 4.3 Calculate and document PIT divisor: 1193182 / 100 = 11931
  - [x] 4.4 Implement timer_init() to configure PIT channel 0
  - [x] 4.5 Configure PIT: mode 3 (square wave), lobyte/hibyte
  - [x] 4.6 Implement timer_handler() as IRQ 0 handler
  - [x] 4.7 Implement timer_get_ticks() to return current count

- [x] **Task 5: Add IRQ Handler Support to ISR** (AC: #3)
  - [x] 5.1 Add IRQ stub macros to `kernel/init/isr_stubs.S` (IRQ 0-15)
  - [x] 5.2 Create irq_common handler that calls irq_handler and sends EOI
  - [x] 5.3 Add irq_handler() to `kernel/init/isr.c` with handler dispatch
  - [x] 5.4 Add irq_register_handler() for registering IRQ callbacks

- [x] **Task 6: Register IDT Entries for IRQs** (AC: #1, #2)
  - [x] 6.1 Modify idt_init() to register IRQ handlers at INT 32-47
  - [x] 6.2 Use same interrupt gate type (0x8E) for IRQs

- [x] **Task 7: Integrate into Kernel Boot** (AC: #1, #2, #3)
  - [x] 7.1 Add pic_init() call in kmain() after idt_init()
  - [x] 7.2 Add timer_init() call after pic_init()
  - [x] 7.3 Enable interrupts with `sti` after timer is configured
  - [x] 7.4 Add printk messages for PIC and timer initialization

- [x] **Task 8: Testing and Verification** (AC: #1-6)
  - [x] 8.1 Create `kernel/test/test_pic.c` with PIC verification tests
  - [x] 8.2 Create `kernel/test/test_timer.c` with timer verification tests
  - [x] 8.3 Test: Verify IRQ remapping is correct (INT 32 for IRQ 0)
  - [x] 8.4 Test: Verify tick counter increments
  - [x] 8.5 Test: Verify approximately 100 ticks per second (use serial timestamp)
  - [x] 8.6 Run `make qemu` and verify timer interrupts fire continuously

---

## Dev Notes

### What This Story Accomplishes

This is the **second story in Epic 2** - Interrupt Handling & Device I/O. After this:
- PIC is properly remapped (no BIOS conflicts)
- Timer interrupts fire at 100Hz
- Foundation for preemptive scheduling is ready
- IRQ handling infrastructure enables keyboard driver (Story 2.3)

### 8259 PIC Overview

The i386 uses two cascaded 8259 PICs:
- **Master PIC (PIC1):** Handles IRQ 0-7
- **Slave PIC (PIC2):** Handles IRQ 8-15, connected to master's IRQ 2

**Default BIOS Mapping (conflicts with CPU exceptions):**
- IRQ 0-7 → INT 0x08-0x0F (overlaps exceptions!)
- IRQ 8-15 → INT 0x70-0x77

**Our Remapping (standard for protected mode):**
- IRQ 0-7 → INT 0x20-0x27 (32-39)
- IRQ 8-15 → INT 0x28-0x2F (40-47)

### PIC Initialization Sequence (ICW1-ICW4)

```c
/* ICW1: Initialize + ICW4 needed */
#define ICW1_INIT    0x10  /* Initialization command */
#define ICW1_ICW4    0x01  /* ICW4 needed */

/* ICW2: Interrupt vector offset */
#define PIC1_OFFSET  0x20  /* IRQ 0-7 map to INT 32-39 */
#define PIC2_OFFSET  0x28  /* IRQ 8-15 map to INT 40-47 */

/* ICW3: Master/Slave wiring */
#define ICW3_MASTER  0x04  /* IRQ2 has slave */
#define ICW3_SLAVE   0x02  /* Slave ID = 2 */

/* ICW4: 8086 mode */
#define ICW4_8086    0x01
```

**Initialization Sequence:**
```c
void pic_init(void)
{
    /* ICW1: Start initialization, expect ICW4 */
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    /* ICW2: Set interrupt vector offsets */
    outb(PIC1_DATA, PIC1_OFFSET);  /* IRQ 0-7 → INT 32-39 */
    io_wait();
    outb(PIC2_DATA, PIC2_OFFSET);  /* IRQ 8-15 → INT 40-47 */
    io_wait();

    /* ICW3: Configure cascade */
    outb(PIC1_DATA, ICW3_MASTER);  /* Master has slave on IRQ2 */
    io_wait();
    outb(PIC2_DATA, ICW3_SLAVE);   /* Slave's cascade identity */
    io_wait();

    /* ICW4: 8086 mode */
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    /* Mask all interrupts initially */
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}
```

### io_wait() Implementation

Small delay required between PIC I/O operations:
```c
static inline void io_wait(void)
{
    /* Port 0x80 is used for POST codes - safe to write */
    outb(0x80, 0);
}
```

### PIC End-of-Interrupt (EOI)

After handling an IRQ, must acknowledge to PIC:
```c
#define PIC_EOI  0x20  /* End of interrupt command */

void pic_send_eoi(uint8_t irq)
{
    /* If IRQ came from slave PIC, send EOI to both */
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}
```

### IRQ Masking

Enable/disable individual IRQs via OCW1 (data port):
```c
void pic_clear_mask(uint8_t irq)
{
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    value = inb(port) & ~(1 << irq);
    outb(port, value);
}

void pic_set_mask(uint8_t irq)
{
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    value = inb(port) | (1 << irq);
    outb(port, value);
}
```

### 8253/8254 PIT Overview

The Programmable Interval Timer (PIT) has 3 channels:
- **Channel 0:** System timer (IRQ 0) - **we use this**
- **Channel 1:** DRAM refresh (historical)
- **Channel 2:** PC speaker

**Ports:**
- 0x40: Channel 0 data
- 0x41: Channel 1 data
- 0x42: Channel 2 data
- 0x43: Command register

**Base Frequency:** 1,193,182 Hz (derived from 14.31818 MHz / 12)

### PIT Divisor Calculation

For 100Hz (10ms interval):
```
Divisor = 1,193,182 / 100 = 11,931.82 ≈ 11,932

Actual frequency = 1,193,182 / 11,932 = 100.007 Hz
```

This gives us a tick rate very close to 100Hz.

### PIT Mode/Command Register (0x43)

```
Bits 7-6: Channel select
  00 = Channel 0 (timer)
  01 = Channel 1
  10 = Channel 2
  11 = Read-back command

Bits 5-4: Access mode
  00 = Latch count
  01 = Low byte only
  10 = High byte only
  11 = Low byte then high byte

Bits 3-1: Operating mode
  000 = Mode 0: Interrupt on terminal count
  001 = Mode 1: Hardware re-triggerable one-shot
  010 = Mode 2: Rate generator
  011 = Mode 3: Square wave generator (most common)
  100 = Mode 4: Software triggered strobe
  101 = Mode 5: Hardware triggered strobe

Bit 0: BCD/Binary
  0 = 16-bit binary
  1 = BCD
```

**Our configuration: 0x36**
- Channel 0 (00)
- Lobyte/Hibyte (11)
- Mode 3 square wave (011)
- Binary (0)

### Timer Initialization

```c
#define PIT_CHANNEL0    0x40
#define PIT_CMD         0x43
#define PIT_FREQUENCY   1193182
#define TARGET_HZ       100

static volatile uint32_t ticks = 0;

void timer_init(void)
{
    uint16_t divisor = PIT_FREQUENCY / TARGET_HZ;

    /* Set command: Channel 0, lobyte/hibyte, mode 3, binary */
    outb(PIT_CMD, 0x36);

    /* Send divisor low byte then high byte */
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);

    /* Register timer handler for IRQ 0 */
    irq_register_handler(0, timer_handler);

    /* Enable IRQ 0 */
    pic_clear_mask(0);
}
```

### Timer Interrupt Handler

```c
static void timer_handler(struct registers *regs)
{
    (void)regs;  /* Unused for now */
    ticks++;

    /* Future: Call scheduler here for preemptive multitasking */
}

uint32_t timer_get_ticks(void)
{
    return ticks;
}
```

### IRQ Stub Pattern (Assembly)

Add to `isr_stubs.S`:
```asm
/* Macro for IRQ stubs - no error code needed */
.macro IRQ num, int_num
.global irq\num
irq\num:
    pushl $0           /* Dummy error code */
    pushl $\int_num    /* Interrupt number */
    jmp irq_common
.endm

/* IRQ 0-15 (mapped to INT 32-47) */
IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

/* Common IRQ handler */
irq_common:
    pushal
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs

    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    pushl %esp
    call irq_handler
    addl $4, %esp

    popl %gs
    popl %fs
    popl %es
    popl %ds
    popal
    addl $8, %esp
    iret
```

### IRQ Handler Dispatch (C)

```c
/* IRQ handler function pointer type */
typedef void (*irq_handler_t)(struct registers *);

/* IRQ handler table */
static irq_handler_t irq_handlers[16] = {0};

void irq_register_handler(uint8_t irq, irq_handler_t handler)
{
    irq_handlers[irq] = handler;
}

void irq_handler(struct registers *regs)
{
    uint8_t irq = regs->int_no - 32;

    if (irq_handlers[irq] != NULL) {
        irq_handlers[irq](regs);
    }

    pic_send_eoi(irq);
}
```

### Integration with main.c

```c
#include <pic.h>
#include <timer.h>

void kmain(void)
{
    gdt_init();
    vga_init();
    serial_init();
    idt_init();

    printk(LOG_INFO, "os-dev kernel starting\n");
    printk(LOG_INFO, "GDT initialized\n");
    printk(LOG_INFO, "VGA initialized\n");
    printk(LOG_INFO, "Serial initialized\n");
    printk(LOG_INFO, "IDT initialized\n");

    pic_init();        /* NEW - Story 2.2 */
    printk(LOG_INFO, "PIC initialized\n");

    timer_init();      /* NEW - Story 2.2 */
    printk(LOG_INFO, "Timer initialized (100Hz)\n");

    /* Enable interrupts now that PIC and timer are ready */
    __asm__ volatile ("sti");
    printk(LOG_INFO, "Interrupts enabled\n");

#ifdef TEST_MODE
    test_run_all();
#endif

    printk(LOG_INFO, "Boot complete\n");

    for (;;) {
        hlt();
    }
}
```

### File Locations

| File | Purpose |
|------|---------|
| `kernel/include/pic.h` | PIC constants and function declarations |
| `kernel/include/timer.h` | Timer constants and function declarations |
| `kernel/init/pic.c` | PIC initialization and EOI handling |
| `kernel/drivers/timer.c` | PIT configuration and tick counter |
| `kernel/init/isr_stubs.S` | Add IRQ stubs (irq0-irq15) |
| `kernel/init/isr.c` | Add irq_handler() and handler registration |

### Previous Story Intelligence (2.1)

**From Story 2.1:**
- IDT is initialized with 256 entries
- Exception handlers 0-31 are registered
- isr_common saves all registers before calling C handler
- struct registers defined for interrupt frame
- lidt loaded IDTR

**Code patterns established:**
- ISR stubs push exception number then jump to common handler
- isr_stubs.S uses macros for exception stub generation
- idt_set_gate() fills IDT entries

**Git history:**
```
38addb4 feat[story 2.1]: idt setup exception handlers.
1efc3a5 feat[story 1.6]: serial debug panic infrastructure.
```

### Common Pitfalls

1. **Forgetting io_wait()** - PIC needs small delays between I/O operations
2. **Wrong ICW order** - Must send ICW1-4 in exact sequence
3. **Not masking IRQs initially** - Can cause spurious interrupts
4. **Missing slave PIC EOI** - IRQs 8-15 require EOI to both PICs
5. **Enabling interrupts too early** - Must configure PIC before `sti`
6. **Wrong IRQ number calculation** - INT number = IRQ + 32
7. **Divisor byte order** - Send low byte first, then high byte to PIT
8. **Using wrong PIT mode** - Mode 3 (square wave) is standard for timer

### Relationship to Other Stories

- **Depends on:** Story 2.1 (IDT setup, interrupt handling infrastructure)
- **Enables:** Story 2.3 (Keyboard driver uses IRQ 1)
- **Enables:** Story 4.3 (Scheduler uses timer interrupt for preemption)

### Testing Strategy

1. **PIC remapping test:** Verify IRQ 0 maps to INT 32
2. **IRQ mask test:** Verify can enable/disable specific IRQs
3. **Timer tick test:** Verify ticks increment over time
4. **Tick rate test:** Count ticks over known interval (use serial timestamps)
5. **EOI test:** Verify interrupts continue firing (no IRQ starvation)
6. **Boot test:** `make qemu` boots and timer runs continuously

### Test Implementation Notes

Timer tick rate verification is challenging without real-time clock. Approaches:
1. Print tick count periodically, visually verify ~100/second
2. Use QEMU's `-icount` for deterministic timing (optional)
3. Simple sanity check: verify ticks > 0 after brief delay

### Project Structure Notes

**New Files:**
- `kernel/include/pic.h`
- `kernel/include/timer.h`
- `kernel/init/pic.c`
- `kernel/drivers/timer.c`
- `kernel/test/test_pic.c`
- `kernel/test/test_timer.c`

**Modified Files:**
- `kernel/init/isr_stubs.S` - Add IRQ stub macros and irq_common
- `kernel/init/isr.c` - Add irq_handler() and handler registration
- `kernel/include/isr.h` - Add IRQ handler types and declarations
- `kernel/init/idt.c` - Register IDT entries for INT 32-47
- `kernel/init/main.c` - Add pic_init(), timer_init(), sti
- `kernel/test/test_runner.c` - Add test_pic() and test_timer()

### References

- [Source: _bmad-output/planning-artifacts/architecture.md#Project-Structure]
- [Source: _bmad-output/planning-artifacts/architecture.md#Milestone-to-Directory-Mapping]
- [Source: _bmad-output/planning-artifacts/epics.md#Story-2.2]
- [Source: _bmad-output/project-context.md#Critical-C-Rules]
- [Source: _bmad-output/implementation-artifacts/2-1-idt-setup-exception-handlers.md]
- [Source: Intel SDM Vol. 3A - 8259A Programmable Interrupt Controller]
- [Source: OSDev Wiki - 8259 PIC]
- [Source: OSDev Wiki - Programmable Interval Timer]

---

## Dev Agent Record

### Agent Model Used

Claude Opus 4.5 (claude-opus-4-5-20251101)

### Debug Log References

- Fixed bootloader issue: KERNEL_SECTORS increased from 32 to 64 to accommodate larger test kernel (24KB)

### Completion Notes List

- **PIC Implementation**: Created pic.h with all 8259 PIC constants (ICW1-4, ports, EOI). Implemented pic.c with full ICW initialization sequence, IRQ remapping (IRQ 0-7→INT 32-39, IRQ 8-15→INT 40-47), EOI handling for master/slave cascade, and IRQ masking functions.

- **Timer Implementation**: Created timer.h with PIT constants and command definitions. Implemented timer.c with 100Hz tick configuration (divisor 11931), volatile tick counter, timer interrupt handler, and timer_get_ticks() accessor.

- **IRQ Infrastructure**: Extended isr_stubs.S with IRQ macro for stubs irq0-irq15 and irq_common handler. Added irq_handler() dispatch and irq_register_handler() to isr.c. Updated isr.h with IRQ handler type and declarations.

- **IDT Integration**: Modified idt_init() to register IRQ handlers at INT 32-47 using interrupt gates (0x8E).

- **Boot Integration**: Updated main.c to call pic_init(), timer_init(), enable interrupts with sti(), and add appropriate printk messages.

- **Testing**: Created comprehensive test_pic.c (22 tests) and test_timer.c (10 tests). All 97 kernel tests pass.

### File List

**New Files:**
- kernel/include/pic.h
- kernel/include/timer.h
- kernel/init/pic.c
- kernel/drivers/timer.c
- kernel/test/test_pic.c
- kernel/test/test_timer.c

**Modified Files:**
- kernel/init/isr_stubs.S - Added IRQ stubs (irq0-irq15) and irq_common handler
- kernel/init/isr.c - Added irq_handler(), irq_register_handler(), irq_handlers[] table
- kernel/include/isr.h - Added IRQ stub declarations, irq_handler_t typedef, function prototypes
- kernel/init/idt.c - Registered IRQ handlers at INT 32-47
- kernel/init/main.c - Added pic_init(), timer_init(), sti() calls and includes
- kernel/test/test_runner.c - Added test_pic() and test_timer() calls
- boot/stage2.S - Increased KERNEL_SECTORS from 32 to 64 (32KB max kernel size)

## Change Log

- 2026-02-03: Story 2.2 implemented - PIC remapping, timer at 100Hz, IRQ infrastructure, all tests passing
- 2026-02-03: Code review fixes applied:
  - Added IRQ bounds validation (>=16) to pic_set_mask/pic_clear_mask with warning logs
  - Added IRQ bounds validation to irq_register_handler with warning log
  - Fixed pic_clear_mask to also unmask cascade line (IRQ 2) for slave PIC IRQs
  - Removed dead code (unused mask save/restore) from pic_init
  - Added spurious IRQ detection for IRQ 7/15 using ISR read
  - Fixed timer test to use strict > assertion instead of >=
  - Fixed PIT_DIVISOR comment (11,931 not 11,932)
