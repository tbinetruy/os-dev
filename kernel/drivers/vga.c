/*
 * kernel/drivers/vga.c - VGA Text Mode Driver Implementation
 *
 * Provides text output via VGA text mode (80x25). Features:
 *   - Character and string output with cursor tracking
 *   - Automatic line wrapping and screen scrolling
 *   - Hardware cursor synchronization
 *   - Configurable foreground/background colors
 *
 * VGA text buffer is at 0xB8000, each cell is 2 bytes:
 *   - Low byte: ASCII character
 *   - High byte: Attribute (fg | bg << 4)
 */

#include <vga.h>
#include <asm.h>

/*
 * =============================================================================
 * Private State
 * =============================================================================
 */

/* VGA text buffer - volatile because hardware may change it */
static volatile uint16_t *vga_buffer = (volatile uint16_t *)VGA_BUFFER_ADDR;

/* Current cursor position */
static int cursor_row = 0;
static int cursor_col = 0;

/* Current text color attribute */
static uint8_t current_color = VGA_COLOR_DEFAULT;

/*
 * =============================================================================
 * Private Helper Functions
 * =============================================================================
 */

/*
 * vga_entry - Create a 16-bit VGA character entry
 *
 * Combines ASCII character with color attribute into format
 * expected by VGA hardware.
 *
 * @c: ASCII character
 * @color: Attribute byte (foreground | background << 4)
 *
 * Returns: 16-bit VGA entry (char | attr << 8)
 */
static inline uint16_t vga_entry(char c, uint8_t color)
{
    return (uint16_t)c | ((uint16_t)color << 8);
}

/*
 * vga_update_cursor - Update hardware cursor position
 *
 * Programs the VGA CRT controller to move the blinking cursor
 * to match our software cursor position.
 *
 * CRT controller registers:
 *   0x0E: Cursor location high byte
 *   0x0F: Cursor location low byte
 */
static void vga_update_cursor(void)
{
    uint16_t pos = cursor_row * VGA_WIDTH + cursor_col;

    outb(VGA_CRTC_INDEX, VGA_CURSOR_LOW);
    outb(VGA_CRTC_DATA, pos & 0xFF);
    outb(VGA_CRTC_INDEX, VGA_CURSOR_HIGH);
    outb(VGA_CRTC_DATA, (pos >> 8) & 0xFF);
}

/*
 * vga_scroll - Scroll screen up by one line
 *
 * Copies rows 1-24 to rows 0-23, then clears row 24.
 * Called when cursor reaches row 25 (off screen).
 */
static void vga_scroll(void)
{
    /* Move rows 1-24 up to rows 0-23 */
    for (int i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1); i++) {
        vga_buffer[i] = vga_buffer[i + VGA_WIDTH];
    }

    /* Clear the last row */
    for (int i = VGA_WIDTH * (VGA_HEIGHT - 1); i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = vga_entry(' ', current_color);
    }

    /* Move cursor to last row */
    cursor_row = VGA_HEIGHT - 1;
}

/*
 * =============================================================================
 * Public Functions
 * =============================================================================
 */

/*
 * vga_init - Initialize VGA driver and clear screen
 */
void vga_init(void)
{
    /* Reset cursor to top-left */
    cursor_row = 0;
    cursor_col = 0;

    /* Set default color: light grey on black */
    current_color = VGA_COLOR_DEFAULT;

    /* Clear entire screen */
    vga_clear();

    /* Sync hardware cursor */
    vga_update_cursor();
}

/*
 * vga_putchar - Print a single character at cursor position
 */
void vga_putchar(char c)
{
    /* Handle special characters */
    if (c == '\n') {
        /* Newline: move to start of next line */
        cursor_col = 0;
        cursor_row++;
    } else if (c == '\r') {
        /* Carriage return: move to start of current line */
        cursor_col = 0;
    } else {
        /* Printable character: write to buffer */
        int pos = cursor_row * VGA_WIDTH + cursor_col;
        vga_buffer[pos] = vga_entry(c, current_color);

        /* Advance cursor */
        cursor_col++;

        /* Wrap to next line if at end of current line */
        if (cursor_col >= VGA_WIDTH) {
            cursor_col = 0;
            cursor_row++;
        }
    }

    /* Scroll if cursor went past bottom of screen */
    if (cursor_row >= VGA_HEIGHT) {
        vga_scroll();
    }

    /* Update hardware cursor */
    vga_update_cursor();
}

/*
 * vga_puts - Print a null-terminated string
 */
void vga_puts(const char *str)
{
    while (*str) {
        vga_putchar(*str++);
    }
}

/*
 * vga_clear - Clear the entire screen
 */
void vga_clear(void)
{
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = vga_entry(' ', current_color);
    }

    /* Reset cursor to top-left */
    cursor_row = 0;
    cursor_col = 0;

    /* Update hardware cursor */
    vga_update_cursor();
}

/*
 * vga_set_color - Set text foreground and background colors
 */
void vga_set_color(uint8_t fg, uint8_t bg)
{
    current_color = fg | (bg << 4);
}
