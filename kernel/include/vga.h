/*
 * kernel/include/vga.h - VGA Text Mode Driver Interface
 *
 * Provides text output to the VGA display in 80x25 text mode.
 * The VGA text buffer is located at physical address 0xB8000.
 *
 * Features:
 *   - Character and string output with automatic cursor advance
 *   - Line wrapping at column 80
 *   - Screen scrolling when reaching bottom
 *   - Hardware cursor synchronization
 *   - Configurable text colors
 *
 * Memory Layout (per character cell):
 *   Byte 0: ASCII character code (0x00-0xFF)
 *   Byte 1: Attribute byte (foreground | background << 4)
 *
 * Screen: 80 columns x 25 rows = 2000 cells = 4000 bytes
 */

#ifndef KERNEL_INCLUDE_VGA_H
#define KERNEL_INCLUDE_VGA_H

#include <types.h>

/*
 * =============================================================================
 * VGA Constants
 * =============================================================================
 */

/* VGA text buffer physical address */
#define VGA_BUFFER_ADDR 0xB8000

/* Screen dimensions */
#define VGA_WIDTH  80
#define VGA_HEIGHT 25

/*
 * =============================================================================
 * VGA Color Constants
 * =============================================================================
 *
 * Standard 16-color VGA palette. Foreground colors can use all 16.
 * Background colors use bits 4-6 (0-7) unless blink is disabled.
 */
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

/* Default color: light grey on black (standard terminal) */
#define VGA_COLOR_DEFAULT       0x07

/*
 * =============================================================================
 * VGA CRT Controller Ports (for hardware cursor)
 * =============================================================================
 */
#define VGA_CRTC_INDEX  0x3D4
#define VGA_CRTC_DATA   0x3D5
#define VGA_CURSOR_HIGH 0x0E
#define VGA_CURSOR_LOW  0x0F

/*
 * =============================================================================
 * Public Functions
 * =============================================================================
 */

/*
 * vga_init - Initialize VGA driver and clear screen
 *
 * Resets cursor to (0,0), sets default color (light grey on black),
 * clears entire screen, and updates hardware cursor.
 *
 * Must be called before any other VGA functions.
 */
void vga_init(void);

/*
 * vga_putchar - Print a single character at cursor position
 *
 * Handles special characters:
 *   '\n' - Move to start of next line (scroll if at bottom)
 *   '\r' - Move to start of current line
 *
 * For printable characters:
 *   - Write character at cursor
 *   - Advance cursor
 *   - Wrap to next line if at column 80
 *   - Scroll if at row 25
 *   - Update hardware cursor
 *
 * @c: ASCII character to print
 */
void vga_putchar(char c);

/*
 * vga_puts - Print a null-terminated string
 *
 * Prints each character using vga_putchar(), handling newlines
 * and wrapping automatically.
 *
 * @str: Null-terminated string to print
 */
void vga_puts(const char *str);

/*
 * vga_clear - Clear the entire screen
 *
 * Fills screen with spaces using current color, resets cursor
 * to (0,0), and updates hardware cursor.
 */
void vga_clear(void);

/*
 * vga_set_color - Set text foreground and background colors
 *
 * Changes the color attribute used for subsequent character output.
 * Does not affect characters already on screen.
 *
 * @fg: Foreground color (0-15, use VGA_COLOR_* constants)
 * @bg: Background color (0-7, use VGA_COLOR_* constants)
 */
void vga_set_color(uint8_t fg, uint8_t bg);

#endif /* KERNEL_INCLUDE_VGA_H */
