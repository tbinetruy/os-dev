/*
 * kernel/include/keyboard.h - PS/2 Keyboard Driver
 *
 * Implements a basic keyboard driver using the 8042 PS/2 controller.
 * Scancodes are translated from Set 1 (XT) to ASCII and stored in
 * a circular buffer for retrieval by kernel code.
 *
 * The driver handles:
 *   - IRQ 1 interrupt registration
 *   - Scancode to ASCII translation (US QWERTY, lowercase only)
 *   - Circular buffer for keystroke storage
 *   - Non-blocking character retrieval
 *
 * Deferred features (future enhancement):
 *   - Shift/Caps Lock handling
 *   - Control key combinations
 *   - Extended keys (arrows, function keys)
 *
 * References:
 *   - OSDev Wiki: PS/2 Keyboard
 *   - OSDev Wiki: Scancode Set 1
 */

#ifndef KERNEL_INCLUDE_KEYBOARD_H
#define KERNEL_INCLUDE_KEYBOARD_H

#include <types.h>

/*
 * =============================================================================
 * 8042 PS/2 Controller I/O Ports
 * =============================================================================
 */

/*
 * KEYBOARD_DATA_PORT - Data register (port 0x60)
 *
 * Read: Returns scancode or data from controller
 * Write: Send command/data to keyboard
 */
#define KEYBOARD_DATA_PORT      0x60

/*
 * KEYBOARD_STATUS_PORT - Status/Command register (port 0x64)
 *
 * Read: Status register
 * Write: Command register
 *
 * Note: Status port not required for basic input, but useful for
 *       advanced operations (LED control, self-test, etc.)
 */
#define KEYBOARD_STATUS_PORT    0x64

/*
 * =============================================================================
 * Keyboard Buffer Configuration
 * =============================================================================
 */

/*
 * KEYBOARD_BUFFER_SIZE - Size of circular keystroke buffer
 *
 * Must be power of 2 for efficient modulo operation.
 * 256 bytes provides reasonable buffering for typical typing.
 */
#define KEYBOARD_BUFFER_SIZE    256

/*
 * =============================================================================
 * Function Declarations
 * =============================================================================
 */

/*
 * keyboard_init - Initialize keyboard driver
 *
 * Initializes the keyboard buffer, registers the IRQ 1 handler,
 * and unmasks the keyboard IRQ to enable interrupts.
 *
 * Call this after PIC is initialized but before enabling
 * interrupts with STI.
 *
 * Initialization sequence:
 *   1. Reset buffer head/tail to 0
 *   2. Register keyboard_handler with IRQ 1
 *   3. Enable IRQ 1 via pic_clear_mask()
 */
void keyboard_init(void);

/*
 * keyboard_getchar - Get next character from keyboard buffer
 *
 * Retrieves and removes the oldest character from the buffer.
 * This is a non-blocking call - returns immediately.
 *
 * Thread safety: Disables interrupts during buffer access to
 * prevent race conditions with the IRQ handler.
 *
 * Returns:
 *   Character value (0-255) if buffer has data
 *   -1 if buffer is empty
 */
int keyboard_getchar(void);

/*
 * keyboard_has_data - Check if keyboard buffer has data
 *
 * Returns true if there are characters waiting in the buffer.
 * Use this to poll for input without consuming characters.
 *
 * Returns:
 *   true if buffer contains characters
 *   false if buffer is empty
 */
bool keyboard_has_data(void);

#endif /* KERNEL_INCLUDE_KEYBOARD_H */
