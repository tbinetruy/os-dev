/*
 * kernel/test/test_vga.c - VGA driver unit tests
 *
 * Tests for VGA text mode driver functionality.
 * Verifies:
 *   - VGA buffer is at correct address (0xB8000)
 *   - Constants are defined correctly
 *   - Character output writes to correct buffer position
 *   - Screen clearing works
 *   - Line wrapping and scrolling work
 */

#ifdef TEST_MODE

#include <test.h>
#include <vga.h>
#include <types.h>

/* Direct access to VGA buffer for verification */
#define TEST_VGA_BUFFER ((volatile uint16_t *)0xB8000)

/*
 * Helper to extract character from VGA entry
 */
static inline char vga_get_char(uint16_t entry)
{
    return (char)(entry & 0xFF);
}

/*
 * Helper to extract attribute from VGA entry
 */
static inline uint8_t vga_get_attr(uint16_t entry)
{
    return (uint8_t)(entry >> 8);
}

/*
 * test_vga - VGA driver test suite
 *
 * Called from test_runner.c when TEST_MODE is enabled.
 */
void test_vga(void)
{
    TEST_BEGIN("vga");

    /* Test 1: VGA constants are correct */
    TEST_ASSERT_EQ(80, VGA_WIDTH);
    TEST_ASSERT_EQ(25, VGA_HEIGHT);
    TEST_ASSERT_EQ(0xB8000, VGA_BUFFER_ADDR);

    /* Test 2: Color constants are defined */
    TEST_ASSERT_EQ(0, VGA_COLOR_BLACK);
    TEST_ASSERT_EQ(7, VGA_COLOR_LIGHT_GREY);
    TEST_ASSERT_EQ(15, VGA_COLOR_WHITE);
    TEST_ASSERT_EQ(10, VGA_COLOR_LIGHT_GREEN);

    /* Test 3: vga_clear() clears the screen */
    vga_clear();
    /* Capture before assertions modify VGA */
    {
        char ch = vga_get_char(TEST_VGA_BUFFER[0]);
        uint8_t attr = vga_get_attr(TEST_VGA_BUFFER[0]);
        TEST_ASSERT_EQ(' ', ch);
        TEST_ASSERT_EQ(VGA_COLOR_DEFAULT, attr);
    }

    /* Test 4: vga_putchar() writes character and advances cursor */
    vga_clear();
    vga_putchar('A');
    vga_putchar('B');
    /* Check both before any TEST_ASSERT (which writes to VGA) */
    {
        char a = vga_get_char(TEST_VGA_BUFFER[0]);
        char b = vga_get_char(TEST_VGA_BUFFER[1]);
        TEST_ASSERT_EQ('A', a);
        TEST_ASSERT_EQ('B', b);
    }

    /* Test 5: vga_puts() writes string */
    vga_clear();
    vga_puts("Hi");
    /* Capture both before assertions modify VGA */
    {
        char h = vga_get_char(TEST_VGA_BUFFER[0]);
        char i = vga_get_char(TEST_VGA_BUFFER[1]);
        TEST_ASSERT_EQ('H', h);
        TEST_ASSERT_EQ('i', i);
    }

    /* Test 6: Newline moves to next row */
    vga_clear();
    vga_putchar('X');
    vga_putchar('\n');
    vga_putchar('Y');
    /* 'Y' should be at start of row 1 (position 80) */
    TEST_ASSERT_EQ('Y', vga_get_char(TEST_VGA_BUFFER[VGA_WIDTH]));

    /* Test 7: vga_set_color() changes output color */
    vga_clear();
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    vga_putchar('C');
    /* Attribute should be white (15) on blue (1): 0x1F */
    TEST_ASSERT_EQ(0x1F, vga_get_attr(TEST_VGA_BUFFER[0]));

    /* Test 8: Line wrapping at column 80 */
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_clear();
    /* Print 80 characters to fill first row */
    for (int i = 0; i < VGA_WIDTH; i++) {
        vga_putchar('.');
    }
    /* Next character should wrap to row 1 */
    vga_putchar('W');
    TEST_ASSERT_EQ('W', vga_get_char(TEST_VGA_BUFFER[VGA_WIDTH]));

    /* Test 9: Carriage return moves to start of current line */
    vga_clear();
    vga_puts("ABCDE");
    vga_putchar('\r');
    vga_putchar('X');
    /* Capture both before assertions modify VGA */
    {
        char x = vga_get_char(TEST_VGA_BUFFER[0]);
        char b = vga_get_char(TEST_VGA_BUFFER[1]);
        /* 'X' should overwrite 'A' at position 0 */
        TEST_ASSERT_EQ('X', x);
        /* 'B' should still be at position 1 */
        TEST_ASSERT_EQ('B', b);
    }

    /* Test 10: Screen scrolling when at bottom */
    vga_clear();
    /* Fill screen with 25 lines, each starting with row number */
    for (int row = 0; row < VGA_HEIGHT; row++) {
        vga_putchar('0' + (row % 10));
        vga_putchar('\n');
    }
    /* Screen should have scrolled - row 0 now contains what was row 1 */
    /* Row 1 started with '1', so position 0 should now be '1' */
    TEST_ASSERT_EQ('1', vga_get_char(TEST_VGA_BUFFER[0]));
    /* Last row (24) should be empty (space) after scroll */
    TEST_ASSERT_EQ(' ', vga_get_char(TEST_VGA_BUFFER[VGA_WIDTH * (VGA_HEIGHT - 1)]));

    /* Reset to default state for subsequent output */
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_clear();

    TEST_END();
}

#endif /* TEST_MODE */
