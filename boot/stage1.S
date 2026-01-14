/*
 * stage1.S - Stage 1 Bootloader (Master Boot Record)
 *
 * =============================================================================
 * STAGE 1 BOOTLOADER - MBR (Story 1.2)
 * =============================================================================
 *
 * This is the first code that runs after BIOS loads us at 0x7C00.
 * It must fit in exactly 512 bytes with boot signature 0xAA55 at offset 510.
 *
 * What this bootloader does:
 *   1. Sets up segment registers (DS, ES, SS = 0)
 *   2. Sets up stack below 0x7C00
 *   3. Saves boot drive number from DL
 *   4. Prints 'S' to show stage 1 is running
 *   5. Loads stage 2 from disk sectors 2-5 to 0x7E00
 *   6. Jumps to stage 2, passing boot drive in DL
 *
 * Memory layout during stage 1:
 *   0x00000 - 0x003FF : Interrupt Vector Table (IVT)
 *   0x00400 - 0x004FF : BIOS Data Area (BDA)
 *   0x00500 - 0x07BFF : Free (stack grows down from 0x7C00)
 *   0x07C00 - 0x07DFF : Stage 1 bootloader (this code, 512 bytes)
 *   0x07E00 - 0x087FF : Stage 2 bootloader (loaded here, 2KB)
 *
 * =============================================================================
 */

.code16
.section .text
.global _start

/*
 * =============================================================================
 * Entry Point
 * =============================================================================
 * BIOS jumps here after loading us to 0x7C00.
 * DL register contains the boot drive number - MUST save immediately!
 */
_start:
    /*
     * cli - Disable interrupts during setup
     *
     * We disable interrupts while setting up segment registers and stack
     * to prevent an interrupt from firing with invalid stack or segments.
     */
    cli

    /*
     * cld - Clear direction flag
     *
     * Ensure string operations go forward (increment). Most BIOSes leave
     * DF=0 but it's not guaranteed, so we clear it explicitly.
     */
    cld

    /*
     * Save boot drive number FIRST
     *
     * BIOS passes the boot drive in DL:
     *   0x00 = First floppy (A:)
     *   0x80 = First hard disk
     *
     * We MUST save this immediately before any code clobbers DL.
     * We use absolute addressing with segment 0 since we haven't
     * set up DS yet, but it will be 0 anyway on most BIOSes.
     */
    movb %dl, boot_drive

    /*
     * Set up segment registers
     *
     * Real mode uses segment:offset addressing where:
     *   Physical Address = (Segment * 16) + Offset
     *
     * We set all data segments to 0 so that offsets equal physical addresses.
     * This simplifies our code - when we reference 'boot_drive', the
     * assembler generates an offset, and with DS=0, that offset IS the
     * physical address.
     */
    xorw %ax, %ax       /* AX = 0 */
    movw %ax, %ds       /* Data Segment = 0 */
    movw %ax, %es       /* Extra Segment = 0 (needed for int 0x13) */
    movw %ax, %ss       /* Stack Segment = 0 */

    /*
     * Set up stack
     *
     * The stack grows DOWN in x86. We place the stack pointer at 0x7C00,
     * just below where our bootloader code sits. This gives us ~29KB of
     * stack space (from 0x7C00 down to 0x0500).
     *
     * IMPORTANT: Set SP AFTER SS to avoid issues with interrupts.
     */
    movw $0x7C00, %sp   /* Stack Pointer = 0x7C00, grows down */

    /*
     * Re-enable interrupts
     *
     * Segments and stack are now set up correctly. We need interrupts
     * enabled for BIOS calls (int 0x10 for video, int 0x13 for disk).
     */
    sti

    /*
     * =============================================================================
     * Print 'S' to indicate Stage 1 is running
     * =============================================================================
     */
    movb $'S', %al
    call print_char

    /*
     * =============================================================================
     * Load Stage 2 from disk
     * =============================================================================
     */
    call load_stage2

    /*
     * =============================================================================
     * Jump to Stage 2
     * =============================================================================
     * Pass boot drive number in DL so stage 2 can load the kernel.
     * Use far jump (ljmp) to ensure CS is set correctly.
     */
    movb boot_drive, %dl    /* Pass boot drive to stage 2 */
    ljmp $0x0000, $0x7E00   /* Far jump to stage 2 at 0x0000:0x7E00 */


/*
 * =============================================================================
 * print_char - Print a character using BIOS
 * =============================================================================
 * Input:
 *   AL = character to print
 * Output:
 *   None
 * Clobbers:
 *   AH, BH
 *
 * Uses BIOS INT 0x10, AH=0x0E (Teletype Output):
 *   AH = 0x0E   (function number)
 *   AL = char   (character to print)
 *   BH = page   (display page, usually 0)
 */
print_char:
    movb $0x0E, %ah     /* BIOS teletype function */
    xorb %bh, %bh       /* Page 0 */
    int $0x10           /* Call BIOS video interrupt */
    ret


/*
 * =============================================================================
 * load_stage2 - Load stage 2 bootloader from disk
 * =============================================================================
 * Input:
 *   None (uses boot_drive global)
 * Output:
 *   On success: returns normally
 *   On error: jumps to error handler, does not return
 * Clobbers:
 *   AX, BX, CX, DX, SI
 *
 * Uses BIOS INT 0x13, AH=0x02 (Read Sectors):
 *   AH = 0x02         (function number)
 *   AL = num_sectors  (number of sectors to read)
 *   CH = cylinder     (cylinder number, 0-based)
 *   CL = sector       (sector number, 1-based)
 *   DH = head         (head number, 0-based)
 *   DL = drive        (drive number)
 *   ES:BX = buffer    (destination address)
 *
 * CHS addressing:
 *   Sector 1 = MBR (stage 1, already loaded by BIOS)
 *   Sectors 2-5 = Stage 2 (4 sectors = 2KB)
 *
 * IMPORTANT: CHS sector numbering is 1-based, not 0-based!
 *
 * Includes retry logic (3 attempts) and disk reset on failure.
 */
load_stage2:
    movw $DISK_RETRIES, %si     /* SI = retry counter */
    movb boot_drive, %dl        /* DL = drive number (preserved across INT 0x13) */

.Lretry_read:
    /* Reset disk system (AH=0x00) - helps recover from errors */
    xorb %ah, %ah
    int $0x13

    /* Ensure ES = 0 for correct buffer address */
    xorw %ax, %ax
    movw %ax, %es

    /* Set up read parameters */
    movb $0x02, %ah             /* BIOS read sectors function */
    movb $STAGE2_SECTORS, %al   /* Number of sectors to read */
    movb $0, %ch                /* Cylinder 0 */
    movb $2, %cl                /* Start at sector 2 (after MBR) */
    movb $0, %dh                /* Head 0 */
    movw $0x7E00, %bx           /* Load to ES:BX = 0x0000:0x7E00 */
    int $0x13                   /* Call BIOS disk interrupt */
    jc .Lread_failed            /* Jump if carry flag set (error) */

    /* Verify correct number of sectors were read */
    cmpb $STAGE2_SECTORS, %al
    jne .Lread_failed           /* Wrong count = failure */
    ret                         /* Success! */

.Lread_failed:
    decw %si                    /* Decrement retry counter */
    jnz .Lretry_read            /* Retry if counter > 0 */
    jmp error                   /* All retries exhausted */


/*
 * =============================================================================
 * error - Error handler
 * =============================================================================
 * Called when disk read fails. Prints 'E' and halts.
 * This function never returns.
 */
error:
    movb $'E', %al      /* Print 'E' for Error */
    call print_char
.Lhalt:
    cli                 /* Disable interrupts */
    hlt                 /* Halt CPU */
    jmp .Lhalt          /* Loop if NMI or SMI resumes execution */


/*
 * =============================================================================
 * Data Section
 * =============================================================================
 */

/* Boot drive number, saved from DL at startup */
boot_drive:
    .byte 0

/*
 * =============================================================================
 * Constants
 * =============================================================================
 */

/* Number of sectors to load for stage 2 (4 sectors = 2KB) */
.equ STAGE2_SECTORS, 4

/* Number of disk read retry attempts before giving up */
.equ DISK_RETRIES, 3


/*
 * =============================================================================
 * Boot Signature
 * =============================================================================
 * The MBR MUST be exactly 512 bytes with the boot signature 0xAA55
 * at bytes 510-511. The .org directive pads with zeros to reach
 * offset 510, then we emit the signature.
 *
 * x86 is little-endian, so .word 0xAA55 stores:
 *   Byte 510: 0x55
 *   Byte 511: 0xAA
 */
.org 510
.word 0xAA55
