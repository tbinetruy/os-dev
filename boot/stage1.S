/*
 * stage1.S - Stage 1 Bootloader (Master Boot Record)
 *
 * =============================================================================
 * WHAT HAPPENS WHEN YOU TURN ON A PC?
 * =============================================================================
 *
 * 1. CPU starts executing BIOS code (firmware burned into ROM chip)
 *
 * 2. BIOS performs POST (Power-On Self Test):
 *    - Checks RAM is working
 *    - Detects connected devices (keyboard, disk drives, etc.)
 *    - Initializes hardware to a known state
 *
 * 3. BIOS looks for a "bootable" disk:
 *    - Reads the FIRST sector (512 bytes) from each disk
 *    - Checks if bytes 510-511 contain the "boot signature" (0x55, 0xAA)
 *    - First disk with valid signature is the boot disk
 *
 * 4. BIOS loads that 512-byte sector into RAM at address 0x7C00
 *
 * 5. BIOS jumps to address 0x7C00, and YOUR CODE starts running!
 *
 * This first 512-byte sector is called the "Master Boot Record" (MBR).
 * That's what this file creates.
 *
 * =============================================================================
 * WHY 0x7C00?
 * =============================================================================
 *
 * This magic address comes from the original IBM PC design in 1981.
 *
 * The IBM PC had 32KB of RAM minimum. The designers needed a place to load
 * the boot sector that:
 *   - Was above the interrupt vector table (0x0000-0x03FF)
 *   - Was above the BIOS data area (0x0400-0x04FF)
 *   - Left room for the boot sector to use as stack/data
 *   - Left room for the loaded OS
 *
 * They chose 0x7C00, which is 1KB below the 32KB boundary (0x8000).
 * This gave 512 bytes for the boot sector + 512 bytes for its stack.
 *
 * Modern PCs have gigabytes of RAM, but still load the boot sector at
 * 0x7C00 for backwards compatibility.
 *
 * =============================================================================
 * REAL MODE: THE CPU'S PRIMITIVE STATE
 * =============================================================================
 *
 * When the CPU starts, it's in "Real Mode" - a compatibility mode that
 * emulates the original 8086 processor from 1978. This means:
 *
 * - 16-bit registers (AX, BX, CX, DX, etc.)
 * - 20-bit address space (1MB maximum addressable memory)
 * - No memory protection (any code can access any memory)
 * - No multitasking support
 * - Direct hardware access (no permission levels)
 *
 * Memory addressing in Real Mode uses SEGMENT:OFFSET pairs:
 *   Physical Address = (Segment * 16) + Offset
 *
 * For example, 0x07C0:0x0000 and 0x0000:0x7C00 both point to physical
 * address 0x7C00. This segmentation model is confusing and limiting,
 * which is why we switch to Protected Mode as soon as possible.
 *
 * =============================================================================
 * THE BOOT SIGNATURE: 0xAA55
 * =============================================================================
 *
 * The last two bytes of the MBR MUST be 0x55 at offset 510 and 0xAA at
 * offset 511. This is the "boot signature" that tells BIOS this sector
 * is bootable.
 *
 * Why 0x55AA? In binary: 01010101 10101010
 * This alternating bit pattern was chosen because:
 *   - It's unlikely to appear by accident in random data
 *   - It tests that all data lines are working correctly
 *
 * Note: x86 is little-endian, so when we write ".word 0xAA55", it stores
 * 0x55 at the lower address (510) and 0xAA at the higher address (511).
 *
 * =============================================================================
 * WHAT THIS PLACEHOLDER DOES
 * =============================================================================
 *
 * This is a minimal MBR for Story 1.1. It just:
 *   1. Disables interrupts (cli)
 *   2. Halts the CPU (hlt)
 *   3. Loops forever if somehow resumed
 *
 * In Story 1.2, we'll expand this to:
 *   - Set up segment registers
 *   - Set up a stack
 *   - Load stage 2 bootloader from disk
 *   - Jump to stage 2
 *
 * =============================================================================
 * WHY MULTIPLE BOOT STAGES?
 * =============================================================================
 *
 * 512 bytes is TINY. That's not enough to:
 *   - Read files from a filesystem
 *   - Switch to Protected Mode
 *   - Load a kernel
 *   - Set up paging
 *
 * So we use a multi-stage boot process:
 *
 *   Stage 1 (this file, 512 bytes):
 *     - Fits in MBR
 *     - Loads stage 2 from known disk sectors
 *     - Jumps to stage 2
 *
 *   Stage 2 (several KB, loaded by stage 1):
 *     - Switches to Protected Mode (32-bit)
 *     - Sets up GDT (Global Descriptor Table)
 *     - Enables A20 line (access memory above 1MB)
 *     - Loads kernel from disk
 *     - Sets up initial page tables
 *     - Jumps to kernel
 *
 *   Kernel (loaded by stage 2):
 *     - Full operating system
 *     - Sets up interrupts, memory management, etc.
 *
 * =============================================================================
 * GNU ASSEMBLER (GAS) SYNTAX NOTES
 * =============================================================================
 *
 * This file uses AT&T/GAS syntax (not Intel/NASM). Key differences:
 *
 *   AT&T (GAS):                Intel (NASM):
 *   movl %eax, %ebx           mov ebx, eax       ; source, dest reversed
 *   movl $42, %eax            mov eax, 42        ; $ prefix for immediates
 *   movl (%ebx), %eax         mov eax, [ebx]     ; () vs [] for memory
 *
 * Directives:
 *   .code16       - Generate 16-bit code (Real Mode)
 *   .section .text - Put following code in .text section
 *   .global _start - Export _start symbol (linker can see it)
 *   .org 510      - Set location counter to offset 510
 *   .word 0xAA55  - Emit a 16-bit word (2 bytes)
 *
 * =============================================================================
 */

/*
 * .code16
 *
 * Tell the assembler to generate 16-bit code. The CPU is in Real Mode
 * when the BIOS jumps to us, so we need 16-bit instructions.
 *
 * Without this, the assembler would generate 32-bit code, which would
 * crash immediately because the CPU would misinterpret the opcodes.
 */
.code16

/*
 * .section .text
 *
 * Put the following code in the .text section (executable code).
 * The linker will use this when creating the final binary.
 */
.section .text

/*
 * .global _start
 *
 * Export the _start symbol so the linker can find it.
 * The linker needs to know where execution begins.
 */
.global _start

/*
 * _start:
 *
 * This is where the BIOS jumps after loading us to 0x7C00.
 * This is the very first instruction our OS ever executes!
 */
_start:
    /*
     * cli - Clear Interrupt Flag
     *
     * Disables hardware interrupts. While we're setting up (segment
     * registers, stack, etc.), we don't want interrupts firing and
     * potentially using invalid stack or segments.
     *
     * We'll re-enable interrupts (sti) after everything is set up.
     * For this placeholder, we just halt, so it doesn't matter.
     */
    cli

    /*
     * hlt - Halt
     *
     * Stops the CPU until the next interrupt. Since we disabled
     * interrupts above, this effectively stops the CPU forever.
     *
     * This is a placeholder. In Story 1.2, we'll replace this with
     * actual bootloader code that loads stage 2.
     */
    hlt

    /*
     * jmp _start
     *
     * Infinite loop. If the CPU somehow resumes after hlt (which
     * shouldn't happen with interrupts disabled), jump back and halt again.
     *
     * In real bootloader code, this loop would never be reached.
     */
    jmp _start

/*
 * .org 510
 *
 * Set the location counter to offset 510 (bytes from start of section).
 * The assembler will automatically pad with zeros to reach this offset.
 *
 * This ensures the boot signature lands at exactly bytes 510-511 of
 * the 512-byte boot sector, which is where BIOS expects to find it.
 *
 * Without this, if our code was only 5 bytes, the signature would be
 * at offset 5, and BIOS wouldn't recognize this as a bootable disk.
 */
.org 510

/*
 * .word 0xAA55
 *
 * Emit the boot signature. This 16-bit word is stored in little-endian:
 *   Offset 510: 0x55
 *   Offset 511: 0xAA
 *
 * The BIOS reads these two bytes to verify this is a bootable sector.
 * Without this signature, the BIOS will reject this disk and try the
 * next one (or display "No bootable device found").
 */
.word 0xAA55
