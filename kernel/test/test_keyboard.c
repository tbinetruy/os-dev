/*
 * kernel/test/test_keyboard.c - Keyboard driver tests
 *
 * Verifies keyboard initialization, scancode translation, and
 * buffer operations. Full integration testing requires manual
 * verification in QEMU by typing and observing output.
 *
 * Test coverage:
 *   - Buffer starts empty
 *   - keyboard_getchar() returns -1 on empty buffer
 *   - Scancode translation for common keys
 *   - Buffer put/get operations (FIFO order)
 *   - Buffer overflow handling (drops new chars)
 */

#ifdef TEST_MODE

#include <test.h>
#include <keyboard.h>
#include <printk.h>
#include <isr.h>
#include <pic.h>

/*
 * Test helper declarations (defined in keyboard.c under TEST_MODE)
 */
extern char keyboard_get_scancode_char(uint8_t scancode);
extern void keyboard_test_buffer_put(char c);
extern void keyboard_test_buffer_clear(void);

/*
 * Test helper from isr.c - verifies handler registration
 */
extern bool irq_has_handler(uint8_t irq);

/*
 * test_keyboard - Run keyboard driver tests
 */
void test_keyboard(void)
{
    TEST_BEGIN("keyboard");

    /*
     * Test 1: IRQ 1 handler is registered
     *
     * keyboard_init() should register a handler for IRQ 1 (keyboard).
     * This verifies the driver is correctly hooked into the IRQ system.
     */
    TEST_ASSERT(irq_has_handler(IRQ_KEYBOARD) == true);

    /*
     * Test 2: Buffer starts empty after init
     *
     * After initialization, the buffer should be empty.
     * keyboard_has_data() should return false.
     */
    keyboard_test_buffer_clear();
    TEST_ASSERT(keyboard_has_data() == false);

    /*
     * Test 3: keyboard_getchar() returns -1 on empty buffer
     *
     * When buffer is empty, getchar should return -1 immediately
     * (non-blocking behavior).
     */
    TEST_ASSERT_EQ(-1, keyboard_getchar());

    /*
     * Test 4: Scancode translation - letter 'a'
     *
     * Scancode 0x1E should translate to 'a'.
     */
    TEST_ASSERT_EQ('a', keyboard_get_scancode_char(0x1E));

    /*
     * Test 5: Scancode translation - Enter key
     *
     * Scancode 0x1C should translate to '\n'.
     */
    TEST_ASSERT_EQ('\n', keyboard_get_scancode_char(0x1C));

    /*
     * Test 6: Scancode translation - Space
     *
     * Scancode 0x39 should translate to ' '.
     */
    TEST_ASSERT_EQ(' ', keyboard_get_scancode_char(0x39));

    /*
     * Test 7: Scancode translation - Backspace
     *
     * Scancode 0x0E should translate to '\b'.
     */
    TEST_ASSERT_EQ('\b', keyboard_get_scancode_char(0x0E));

    /*
     * Test 8: Scancode translation - Tab
     *
     * Scancode 0x0F should translate to '\t'.
     */
    TEST_ASSERT_EQ('\t', keyboard_get_scancode_char(0x0F));

    /*
     * Test 9: Scancode translation - non-printable returns 0
     *
     * Left Shift (scancode 0x2A) is a modifier key and should
     * return 0 (non-printable).
     */
    TEST_ASSERT_EQ(0, keyboard_get_scancode_char(0x2A));

    /*
     * Test 10: Buffer put/get - single character
     *
     * Put a character, verify buffer has data, then get it.
     */
    keyboard_test_buffer_clear();
    keyboard_test_buffer_put('x');
    TEST_ASSERT(keyboard_has_data() == true);
    TEST_ASSERT_EQ('x', keyboard_getchar());
    TEST_ASSERT(keyboard_has_data() == false);

    /*
     * Test 11: Buffer FIFO order
     *
     * Characters should come out in the order they were put in.
     */
    keyboard_test_buffer_clear();
    keyboard_test_buffer_put('h');
    keyboard_test_buffer_put('e');
    keyboard_test_buffer_put('l');
    keyboard_test_buffer_put('l');
    keyboard_test_buffer_put('o');
    TEST_ASSERT_EQ('h', keyboard_getchar());
    TEST_ASSERT_EQ('e', keyboard_getchar());
    TEST_ASSERT_EQ('l', keyboard_getchar());
    TEST_ASSERT_EQ('l', keyboard_getchar());
    TEST_ASSERT_EQ('o', keyboard_getchar());
    TEST_ASSERT_EQ(-1, keyboard_getchar());  /* Buffer now empty */

    /*
     * Test 12: Buffer overflow handling
     *
     * When buffer is full, new characters should be dropped.
     * Buffer should not corrupt and oldest chars preserved.
     *
     * Fill buffer to capacity (KEYBOARD_BUFFER_SIZE - 1 chars,
     * since one slot is always empty to distinguish full from empty).
     */
    keyboard_test_buffer_clear();
    for (int i = 0; i < KEYBOARD_BUFFER_SIZE - 1; i++) {
        keyboard_test_buffer_put('A');
    }
    /* Buffer should be full now - this char should be dropped */
    keyboard_test_buffer_put('B');

    /* First char should still be 'A' (not overwritten by 'B') */
    TEST_ASSERT_EQ('A', keyboard_getchar());

    /* Drain and verify all remaining are 'A' */
    for (int i = 0; i < KEYBOARD_BUFFER_SIZE - 2; i++) {
        TEST_ASSERT_EQ('A', keyboard_getchar());
    }
    TEST_ASSERT_EQ(-1, keyboard_getchar());  /* Buffer empty */

    /*
     * Clean up buffer for other tests
     */
    keyboard_test_buffer_clear();

    TEST_END();
}

#endif /* TEST_MODE */
