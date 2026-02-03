/*
 * kernel/drivers/keyboard.c - PS/2 Keyboard Driver
 *
 * Implements keyboard input handling via IRQ 1. Translates scancodes
 * from Set 1 (XT) to ASCII and buffers characters for retrieval.
 *
 * The driver is interrupt-driven:
 *   1. Key press generates IRQ 1
 *   2. Handler reads scancode from port 0x60
 *   3. Scancode translated to ASCII via lookup table
 *   4. Character added to circular buffer
 *   5. Application retrieves via keyboard_getchar()
 *
 * References:
 *   - OSDev Wiki: PS/2 Keyboard
 *   - OSDev Wiki: Scancode Set 1
 */

#include <keyboard.h>
#include <types.h>
#include <asm.h>
#include <pic.h>
#include <isr.h>
#include <printk.h>

/*
 * =============================================================================
 * Scancode Set 1 Translation Table
 * =============================================================================
 *
 * US QWERTY layout, lowercase only (shift handling deferred).
 * Index is the scancode, value is the ASCII character.
 * Value of 0 means non-printable (ignored).
 *
 * Make codes (key press): bit 7 = 0
 * Break codes (key release): bit 7 = 1 (scancode + 0x80)
 */
static const char scancode_to_ascii[128] = {
    /* 0x00 */ 0,    0x1B, '1',  '2',  '3',  '4',  '5',  '6',
    /* 0x08 */ '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',
    /* 0x10 */ 'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',
    /* 0x18 */ 'o',  'p',  '[',  ']',  '\n', 0,    'a',  's',
    /* 0x20 */ 'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',
    /* 0x28 */ '\'', '`',  0,    '\\', 'z',  'x',  'c',  'v',
    /* 0x30 */ 'b',  'n',  'm',  ',',  '.',  '/',  0,    '*',
    /* 0x38 */ 0,    ' ',  0,    0,    0,    0,    0,    0,
    /* 0x40 */ 0,    0,    0,    0,    0,    0,    0,    '7',
    /* 0x48 */ '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
    /* 0x50 */ '2',  '3',  '0',  '.',  0,    0,    0,    0,
    /* 0x58-0x7F: Extended/reserved - all 0 */
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

/*
 * =============================================================================
 * Circular Keyboard Buffer
 * =============================================================================
 *
 * Characters are added at head (by IRQ handler) and removed from tail
 * (by keyboard_getchar). When head == tail, buffer is empty.
 * When (head + 1) % size == tail, buffer is full.
 *
 * volatile is required because head is modified in interrupt context.
 */
static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static volatile uint32_t buffer_head = 0;  /* Write position (IRQ handler) */
static volatile uint32_t buffer_tail = 0;  /* Read position (keyboard_getchar) */

/*
 * Extended scancode state tracking
 *
 * Some keys send multi-byte sequences starting with 0xE0 or 0xE1.
 * When we receive a prefix byte, we set this flag and ignore the
 * following byte(s) to prevent extended keys from producing
 * unexpected characters (e.g., arrow keys producing numpad digits).
 */
static volatile bool awaiting_extended = false;

/*
 * buffer_put - Add character to keyboard buffer
 *
 * Called from IRQ handler to add a translated character.
 * If buffer is full, the character is dropped (no overwrite).
 *
 * @c: Character to add to buffer
 */
static void buffer_put(char c)
{
    uint32_t next_head = (buffer_head + 1) % KEYBOARD_BUFFER_SIZE;

    /* If buffer full, drop the character */
    if (next_head == buffer_tail) {
        return;
    }

    keyboard_buffer[buffer_head] = c;
    buffer_head = next_head;
}

/*
 * buffer_get - Remove and return oldest character from buffer
 *
 * Called from keyboard_getchar() to retrieve buffered input.
 * Caller must ensure interrupts are disabled to prevent race.
 *
 * Returns:
 *   Character value (0-255) if buffer has data
 *   -1 if buffer is empty
 */
static int buffer_get(void)
{
    if (buffer_head == buffer_tail) {
        return -1;  /* Buffer empty */
    }

    char c = keyboard_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
    return (int)(unsigned char)c;
}

/*
 * =============================================================================
 * Keyboard Interrupt Handler
 * =============================================================================
 */

/*
 * keyboard_handler - IRQ 1 interrupt handler
 *
 * Called by irq_handler() when keyboard interrupt fires.
 * Reads scancode from data port, translates to ASCII, and
 * buffers the character if printable.
 *
 * Extended scancodes (0xE0, 0xE1 prefixes) are tracked to prevent
 * arrow keys and other extended keys from producing numpad characters.
 *
 * Note: EOI is sent by irq_handler(), not here.
 *
 * @regs: Saved register state (unused)
 */
static void keyboard_handler(struct registers *regs)
{
    (void)regs;  /* Unused */

    /* Read scancode from data port - must always read to clear interrupt */
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    /* Handle extended scancode sequences (0xE0, 0xE1 prefixes) */
    if (scancode == 0xE0 || scancode == 0xE1) {
        /* Mark that next scancode is part of extended sequence */
        awaiting_extended = true;
        return;
    }

    /* If this is the second byte of an extended sequence, ignore it */
    if (awaiting_extended) {
        awaiting_extended = false;
        return;
    }

    /* Ignore key release (bit 7 set) */
    if (scancode & 0x80) {
        return;
    }

    /* Translate scancode to ASCII */
    char c = scancode_to_ascii[scancode];

    /* Buffer the character if printable (non-zero) */
    if (c != 0) {
        buffer_put(c);
    }
}

/*
 * =============================================================================
 * Public Interface
 * =============================================================================
 */

/*
 * keyboard_init - Initialize keyboard driver
 *
 * Initializes the buffer, registers IRQ 1 handler, and unmasks
 * the keyboard IRQ. Call after PIC is initialized.
 */
void keyboard_init(void)
{
    /* Initialize buffer pointers */
    buffer_head = 0;
    buffer_tail = 0;

    /* Register our handler for IRQ 1 (keyboard) */
    irq_register_handler(IRQ_KEYBOARD, keyboard_handler);

    /* Enable keyboard IRQ (unmask IRQ 1) */
    pic_clear_mask(IRQ_KEYBOARD);

    printk(LOG_INFO, "Keyboard initialized\n");
}

/*
 * keyboard_getchar - Get next character from keyboard buffer
 *
 * Non-blocking: returns immediately with character or -1.
 * Saves and restores interrupt state during buffer access to
 * prevent race with IRQ handler while respecting caller's
 * interrupt state.
 *
 * Returns:
 *   Character value (0-255) if buffer has data
 *   -1 if buffer is empty
 */
int keyboard_getchar(void)
{
    /* Save interrupt state and disable during buffer access */
    uint32_t eflags = read_eflags();
    cli();

    int c = buffer_get();

    /* Restore previous interrupt state */
    write_eflags(eflags);

    return c;
}

/*
 * keyboard_has_data - Check if keyboard buffer has data
 *
 * Returns true if there are characters waiting in the buffer.
 * Uses interrupt-safe read for consistency with keyboard_getchar().
 *
 * Returns:
 *   true if buffer contains characters
 *   false if buffer is empty
 */
bool keyboard_has_data(void)
{
    uint32_t eflags = read_eflags();
    cli();

    bool has_data = (buffer_head != buffer_tail);

    write_eflags(eflags);
    return has_data;
}

/*
 * =============================================================================
 * Test Support Functions
 * =============================================================================
 *
 * These functions are only available when TEST_MODE is enabled.
 * They provide access to internal state for testing purposes.
 */

#ifdef TEST_MODE

/*
 * keyboard_get_scancode_char - Get ASCII for a scancode (test helper)
 *
 * Exposes the scancode translation table for testing.
 *
 * @scancode: Scancode value (0-127)
 * Returns: ASCII character or 0 if non-printable
 */
char keyboard_get_scancode_char(uint8_t scancode)
{
    if (scancode >= 128) {
        return 0;
    }
    return scancode_to_ascii[scancode];
}

/*
 * keyboard_test_buffer_put - Add character to buffer (test helper)
 *
 * Allows tests to simulate keyboard input without actual hardware.
 *
 * @c: Character to add
 */
void keyboard_test_buffer_put(char c)
{
    buffer_put(c);
}

/*
 * keyboard_test_buffer_clear - Clear the keyboard buffer (test helper)
 *
 * Resets buffer to empty state for test isolation.
 */
void keyboard_test_buffer_clear(void)
{
    buffer_head = 0;
    buffer_tail = 0;
}

#endif /* TEST_MODE */
