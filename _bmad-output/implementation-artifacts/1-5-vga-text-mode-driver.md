# Story 1.5: VGA Text Mode Driver

Status: done

## Story

As a developer,
I want to display text on screen via VGA text mode,
so that I can see boot progress and debug output visually.

## Acceptance Criteria

1. **AC1: VGA Initialization**
   - Given kernel is running
   - When vga_init() is called
   - Then screen is cleared to black background

2. **AC2: Single Character Output**
   - Given VGA is initialized
   - When vga_putchar('H') is called
   - Then character 'H' appears at current cursor position
   - And cursor advances to next position

3. **AC3: String Output**
   - Given VGA is initialized
   - When vga_puts("Hello") is called
   - Then string "Hello" appears on screen

4. **AC4: Line Wrapping**
   - Given cursor is at end of line (column 79)
   - When another character is printed
   - Then cursor wraps to beginning of next line

5. **AC5: Screen Scrolling**
   - Given cursor is at bottom of screen (row 24)
   - When another line is printed
   - Then screen scrolls up by one line
   - And new line appears at bottom

6. **AC6: Boot Message Display**
   - Given kernel boot completes
   - When system is stable
   - Then "Hello from os-dev!" (or similar) is displayed on screen

7. **AC7: Code Structure**
   - Given kernel/drivers/vga.c source
   - When I examine the code
   - Then VGA memory is accessed at 0xB8000
   - And attribute byte uses light grey on black (0x07)

## Tasks / Subtasks

- [x] **Task 1: Create VGA Header File** (AC: #2, #3, #7)
  - [x] 1.1 Create `kernel/include/vga.h` with VGA constants and colors
  - [x] 1.2 Define VGA_BUFFER address (0xB8000), VGA_WIDTH (80), VGA_HEIGHT (25)
  - [x] 1.3 Define color constants (VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY, etc.)
  - [x] 1.4 Declare vga_init(), vga_putchar(), vga_puts(), vga_clear() prototypes
  - [x] 1.5 Optionally declare vga_set_color() for attribute changes

- [x] **Task 2: Implement VGA Driver** (AC: #1, #2, #3, #4, #5, #7)
  - [x] 2.1 Create `kernel/drivers/vga.c`
  - [x] 2.2 Define static variables: cursor_row, cursor_col, current_color
  - [x] 2.3 Implement vga_entry() helper to create 16-bit VGA entries
  - [x] 2.4 Implement vga_init() to clear screen and reset cursor to (0,0)
  - [x] 2.5 Implement vga_putchar() with newline and wrap handling
  - [x] 2.6 Implement vga_puts() to print null-terminated strings
  - [x] 2.7 Implement vga_scroll() to scroll screen up when at bottom
  - [x] 2.8 Implement vga_clear() to clear screen

- [x] **Task 3: Hardware Cursor Control** (AC: #2)
  - [x] 3.1 Implement vga_update_cursor() using I/O ports 0x3D4/0x3D5
  - [x] 3.2 Call vga_update_cursor() after each character output
  - [x] 3.3 Document CRT controller register usage

- [x] **Task 4: Integrate into Kernel Boot** (AC: #1, #6)
  - [x] 4.1 Call vga_init() from kmain() after gdt_init()
  - [x] 4.2 Remove inline VGA code from main.c (replaced by driver)
  - [x] 4.3 Display "Hello from os-dev!" message using vga_puts()
  - [x] 4.4 Display memory map count using the driver

- [x] **Task 5: Update Build System** (AC: #7)
  - [x] 5.1 Add vga.c to kernel sources (Makefile wildcard should pick up automatically)
  - [x] 5.2 Ensure build completes without errors
  - [x] 5.3 Verify kernel size hasn't grown unexpectedly

- [x] **Task 6: Testing and Verification** (AC: #1-7)
  - [x] 6.1 Run `make qemu` and verify screen is cleared
  - [x] 6.2 Verify boot message appears correctly
  - [x] 6.3 Test scrolling by printing 26+ lines
  - [x] 6.4 Test line wrapping by printing 81+ characters on one line
  - [x] 6.5 Add test_vga() to kernel test suite

---

## Dev Notes

### What This Story Accomplishes

Story 1.4 left VGA code inline in main.c - direct writes to VGA_BUFFER with manual position tracking. This story creates a proper driver with:
- Encapsulated cursor state management
- Automatic line wrapping and scrolling
- Hardware cursor synchronization
- Clean API for future printk() integration (Story 1.6)

### VGA Text Mode Memory Layout (0xB8000)

Each character cell is 2 bytes (little-endian):
```
Byte 0: ASCII character code (0x00-0xFF)
Byte 1: Attribute byte (foreground | background << 4)
```

Screen dimensions: 80 columns x 25 rows = 2000 cells = 4000 bytes

**Attribute Byte Format:**
```
Bits 0-3: Foreground color (0-15)
Bits 4-6: Background color (0-7)
Bit 7:    Blink enable (or bright background on some hardware)
```

### VGA Color Values

| Value | Color | Value | Color |
|-------|-------|-------|-------|
| 0x0 | Black | 0x8 | Dark Gray |
| 0x1 | Blue | 0x9 | Light Blue |
| 0x2 | Green | 0xA | Light Green |
| 0x3 | Cyan | 0xB | Light Cyan |
| 0x4 | Red | 0xC | Light Red |
| 0x5 | Magenta | 0xD | Light Magenta |
| 0x6 | Brown | 0xE | Yellow |
| 0x7 | Light Grey | 0xF | White |

Default: 0x07 (light grey on black)

### Hardware Cursor Control

The VGA CRT controller uses I/O ports 0x3D4 (index) and 0x3D5 (data) to set cursor position:

```c
/* Set cursor position (linear offset from 0) */
void vga_update_cursor(int row, int col)
{
    uint16_t pos = row * VGA_WIDTH + col;

    outb(0x3D4, 0x0F);            /* Low byte index */
    outb(0x3D5, pos & 0xFF);       /* Low byte */
    outb(0x3D4, 0x0E);            /* High byte index */
    outb(0x3D5, (pos >> 8) & 0xFF); /* High byte */
}
```

**Note:** outb() should be defined in `kernel/include/asm.h` - check if it exists from previous stories, otherwise add it.

### Scrolling Implementation

When cursor reaches row 25 (beyond screen):
1. Copy rows 1-24 to rows 0-23 (memmove or manual copy)
2. Clear row 24 (fill with spaces)
3. Set cursor to row 24, column 0

**Memory copy pattern:**
```c
/* Move 24 rows up (each row = 80 * 2 = 160 bytes) */
for (int i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1); i++) {
    VGA_BUFFER[i] = VGA_BUFFER[i + VGA_WIDTH];
}

/* Clear last row */
for (int i = VGA_WIDTH * (VGA_HEIGHT - 1); i < VGA_WIDTH * VGA_HEIGHT; i++) {
    VGA_BUFFER[i] = vga_entry(' ', current_color);
}
```

### Special Character Handling

**vga_putchar() must handle:**
- `\n` (0x0A): Move to start of next line, scroll if needed
- `\r` (0x0D): Move to start of current line
- `\t` (0x09): Advance to next 8-column boundary (optional for MVP)
- `\b` (0x08): Move cursor back one position (optional for MVP)

Minimum required: `\n` handling for basic output.

### Implementation Pattern

```c
/* kernel/include/vga.h */
#ifndef KERNEL_DRIVERS_VGA_H
#define KERNEL_DRIVERS_VGA_H

#include <types.h>

#define VGA_WIDTH  80
#define VGA_HEIGHT 25

/* VGA colors */
#define VGA_COLOR_BLACK         0
#define VGA_COLOR_BLUE          1
#define VGA_COLOR_GREEN         2
#define VGA_COLOR_CYAN          3
#define VGA_COLOR_RED           4
#define VGA_COLOR_MAGENTA       5
#define VGA_COLOR_BROWN         6
#define VGA_COLOR_LIGHT_GREY    7
#define VGA_COLOR_DARK_GREY     8
#define VGA_COLOR_LIGHT_BLUE    9
#define VGA_COLOR_LIGHT_GREEN   10
#define VGA_COLOR_LIGHT_CYAN    11
#define VGA_COLOR_LIGHT_RED     12
#define VGA_COLOR_LIGHT_MAGENTA 13
#define VGA_COLOR_YELLOW        14
#define VGA_COLOR_WHITE         15

/* Initialize VGA driver and clear screen */
void vga_init(void);

/* Print a single character at cursor, advance cursor */
void vga_putchar(char c);

/* Print a null-terminated string */
void vga_puts(const char *str);

/* Clear the screen */
void vga_clear(void);

/* Set text color (optional) */
void vga_set_color(uint8_t fg, uint8_t bg);

#endif /* KERNEL_DRIVERS_VGA_H */
```

### File Locations (per Architecture)

| File | Purpose |
|------|---------|
| `kernel/include/vga.h` | VGA driver interface and constants |
| `kernel/drivers/vga.c` | VGA driver implementation |

**Directory Note:** `kernel/drivers/` does not exist yet - this will be the first file in that directory. Create the directory if needed.

### Integration with main.c

```c
#include <vga.h>

void kmain(void)
{
    gdt_init();      /* From Story 1.4 */
    vga_init();      /* NEW: Initialize VGA driver */

    /* Remove old inline VGA code, use driver instead */
    vga_puts("Hello from os-dev!\n");
    vga_puts("GDT OK\n");

    /* Display memory map count */
    vga_puts("Memory map entries: ");
    /* ... number formatting code ... */

    /* ... rest of kmain ... */
}
```

### I/O Port Access

Ensure `kernel/include/asm.h` contains port I/O functions:

```c
/* If not already present, add to asm.h */
static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}
```

### Common Pitfalls

1. **Forgetting volatile for VGA_BUFFER** - Compiler may optimize away writes
2. **Integer overflow in position calculation** - Use uint16_t for pos = row * 80 + col
3. **Not updating hardware cursor** - Blinking cursor stays at old position
4. **Off-by-one in scrolling** - Row 24 is the last valid row (0-indexed)
5. **Forgetting to handle \n** - Strings without explicit newlines won't advance cursor properly

### Relationship to Other Stories

- **Depends on:** Story 1.4 (GDT init, kernel runs in protected mode)
- **Required by:** Story 1.6 (serial debug + printk will use VGA for console output)
- **Related:** Story 2.1 (IDT can use VGA for exception output)

### Previous Story Intelligence

**From Story 1.4:**
- VGA inline code exists in main.c - shows working VGA access pattern
- vga_entry() helper function already defined inline - extract and reuse
- Colors: VGA_COLOR_LIGHT_GREEN (0x0A), VGA_COLOR_WHITE (0x0F), VGA_COLOR_BLACK (0x0)
- Screen clear loop: `for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)`
- Output at position: `VGA_BUFFER[pos] = vga_entry(char, fg, bg)`
- Makefile uses wildcard patterns - new files in kernel/drivers/ should be picked up automatically
- types.h exists with uint8_t, uint16_t, uint32_t

**From git history:**
- Recent commits follow pattern: `feat[story X.Y]: description`
- Build system works, kernel boots successfully
- Test infrastructure exists (test_runner.c, test_*.c pattern)

### Testing Strategy

1. **Boot test:** System boots, screen clears to black, message appears
2. **Cursor test:** After message, cursor should be on next line
3. **Scroll test:** Add code to print 26+ lines, verify scrolling works
4. **Wrap test:** Print 81+ character string, verify line wrap
5. **GDB inspection:**
   ```
   (gdb) x/10hx 0xB8000
   # Should show ASCII chars interleaved with 0x07 attributes
   ```
6. **In-kernel test:** Add test_vga() that prints, scrolls, and verifies buffer contents

### Project Structure Notes

**New Files:**
- `kernel/include/vga.h`
- `kernel/drivers/vga.c`

**Modified Files:**
- `kernel/init/main.c` - Call vga_init(), remove inline VGA code
- Potentially `kernel/include/asm.h` - Add outb/inb if not present

**New Directory:**
- `kernel/drivers/` - First driver file, directory may need to be created

### References

- [Source: _bmad-output/planning-artifacts/architecture.md#Project-Structure]
- [Source: _bmad-output/planning-artifacts/architecture.md#Milestone-to-Directory-Mapping]
- [Source: _bmad-output/planning-artifacts/epics.md#Story-1.5]
- [Source: _bmad-output/project-context.md#Critical-C-Rules]
- [Source: _bmad-output/project-context.md#Memory-Rules]
- [Source: OSDev Wiki - VGA Text Mode]
- [Source: OSDev Wiki - Text Mode Cursor]

---

## Dev Agent Record

### Agent Model Used

Claude Opus 4.5 (claude-opus-4-5-20251101)

### Debug Log References

- Build successful with kernel size: 8205 bytes (without tests), 12308 bytes (with tests)
- All tests compile and link correctly

### Completion Notes List

- Created `kernel/include/asm.h` with I/O port access functions (outb, inb, outw, inw, outl, inl, io_wait) and CPU control (cli, sti, hlt)
- Created `kernel/include/vga.h` with VGA constants (buffer address, dimensions, colors) and function prototypes
- Created `kernel/drivers/vga.c` implementing full VGA text mode driver with:
  - vga_init() - clear screen, reset cursor
  - vga_putchar() - print single char with newline/wrap handling
  - vga_puts() - print null-terminated string
  - vga_clear() - clear screen
  - vga_set_color() - change foreground/background colors
  - vga_scroll() - scroll screen up when at bottom
  - vga_update_cursor() - sync hardware cursor via CRT controller
- Updated `kernel/init/main.c` to use VGA driver instead of inline code
- Updated `Makefile` to include kernel/drivers/ in build
- Created `kernel/test/test_vga.c` with 10 unit tests covering constants, clear, putchar, puts, newline, color, and wrap
- Updated `kernel/test/test_runner.c` to call test_vga()

### File List

**New Files:**
- kernel/include/asm.h
- kernel/include/vga.h
- kernel/drivers/vga.c
- kernel/test/test_vga.c

**Modified Files:**
- kernel/init/main.c (added asm.h include, use hlt(), documented print_num)
- kernel/test/test_runner.c (added asm.h include, use hlt(), documented test_print_num, added scroll/\r tests)
- Makefile (added TEST_MODE for drivers)

### Change Log

- 2026-01-16: Story 1.5 implementation complete - VGA text mode driver with hardware cursor control
- 2026-01-16: Code review fixes applied:
  - Added scroll test (Test 10) and carriage return test (Test 9) to test_vga.c
  - Fixed hlt() usage: main.c and test_runner.c now use asm.h helper instead of inline asm
  - Documented intentional print_num duplication in both files
  - Fixed Makefile to apply TEST_MODE to kernel/drivers/*.c
