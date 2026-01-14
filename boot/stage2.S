/*
 * stage2.S - Stage 2 Bootloader Placeholder
 *
 * =============================================================================
 * STAGE 2 PLACEHOLDER (Story 1.2)
 * =============================================================================
 *
 * This is a minimal placeholder for the stage 2 bootloader.
 * It simply prints '2' to confirm stage 1 successfully loaded and jumped
 * to stage 2, then halts.
 *
 * The full stage 2 implementation (Story 1.3) will:
 *   - Enable A20 line
 *   - Switch to Protected Mode (32-bit)
 *   - Set up GDT (Global Descriptor Table)
 *   - Load kernel from disk
 *   - Jump to kernel entry point
 *
 * Memory location:
 *   Stage 2 is loaded by stage 1 at 0x7E00 (right after the MBR at 0x7C00)
 *
 * Input:
 *   DL = boot drive number (passed from stage 1)
 *
 * =============================================================================
 */

.code16
.section .text
.global _start

/*
 * Entry point - stage 1 jumps here after loading us to 0x7E00
 *
 * At this point:
 *   - We're still in 16-bit Real Mode
 *   - DL contains the boot drive number
 *   - Segments are set up (DS=ES=SS=0)
 *   - Stack is at 0x7C00, growing down
 */
_start:
    /*
     * Clear direction flag - ensure string ops go forward.
     * Stage 1 should have done this, but be defensive.
     */
    cld

    /*
     * Print '2' to confirm we reached stage 2
     *
     * Seeing "S2" on screen means:
     *   - Stage 1 executed successfully
     *   - Disk read worked (stage 2 loaded)
     *   - Jump to stage 2 worked
     */
    movb $'2', %al
    movb $0x0E, %ah     /* BIOS teletype function */
    xorb %bh, %bh       /* Page 0 */
    int $0x10           /* Call BIOS video interrupt */

    /*
     * Halt - stage 2 is not implemented yet
     *
     * In Story 1.3, this will be replaced with:
     *   - A20 enable
     *   - Protected mode switch
     *   - Kernel loading
     */
    cli                 /* Disable interrupts */
    hlt                 /* Halt CPU */

.Lhalt_loop:
    jmp .Lhalt_loop     /* Infinite loop if somehow resumed */

/*
 * Pad to fill 4 sectors (2KB) - matches STAGE2_SECTORS in stage1.S
 *
 * The actual size doesn't matter much for this placeholder, but we
 * ensure there's at least some data so the disk image is valid.
 * The Makefile will pad to the full 2KB when creating the image.
 */
