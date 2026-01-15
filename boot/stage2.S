/*
 * stage2.S - Stage 2 Bootloader
 *
 * =============================================================================
 * STAGE 2 BOOTLOADER (Story 1.3)
 * =============================================================================
 *
 * This bootloader performs the critical transition from real mode to protected
 * mode and loads the kernel. When complete, the CPU will be running 32-bit
 * protected mode code at physical address 0x100000.
 *
 * What this bootloader does:
 *   1. Enable A20 line (access memory above 1MB)
 *   2. Query BIOS for memory map (E820)
 *   3. Load kernel from disk to low memory (0x10000)
 *   4. Set up GDT (Global Descriptor Table)
 *   5. Switch to protected mode
 *   6. Copy kernel to 1MB (0x100000)
 *   7. Jump to kernel entry point
 *
 * Memory layout:
 *   0x00500 - 0x00510 : Memory map count and entries start
 *   0x07C00 - 0x07DFF : Stage 1 (can be overwritten now)
 *   0x07E00 - 0x087FF : Stage 2 (this code)
 *   0x10000 - 0x8FFFF : Kernel temporary location
 *   0x90000 - 0x9FFFF : Stack in protected mode
 *   0x100000+         : Kernel final location (1MB)
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
 * =============================================================================
 * Constants
 * =============================================================================
 */

/* Segment selectors for protected mode (index into GDT) */
.equ CODE_SEG, 0x08             /* GDT entry 1: code segment */
.equ DATA_SEG, 0x10             /* GDT entry 2: data segment */

/*
 * Disk layout constants
 *
 * IMPORTANT: CHS vs LBA sector numbering
 *   - CHS (BIOS INT 0x13): Sectors are 1-indexed (first sector is 1)
 *   - LBA (disk image dd seek): Sectors are 0-indexed (first sector is 0)
 *
 * Disk image layout (LBA):
 *   LBA 0:    Stage 1 (MBR)
 *   LBA 1-12: Stage 2 (12 sectors, 6KB)
 *   LBA 13+:  Kernel
 *
 * So to read kernel at LBA 13, we use CHS sector 14.
 */
.equ KERNEL_START_SECTOR, 14    /* CHS sector 14 = LBA sector 13 */
.equ KERNEL_SECTORS, 32         /* Kernel sectors to read (16KB, room for growth) */
.equ KERNEL_LOAD_SEG, 0x1000    /* Segment for 0x10000 */

/* Memory addresses */
.equ KERNEL_LOW_ADDR, 0x10000   /* Temporary kernel location */
.equ KERNEL_HIGH_ADDR, 0x100000 /* Final kernel location (1MB) */
.equ PM_STACK, 0x90000          /* Stack in protected mode */

/* Memory map constants */
.equ MMAP_COUNT_ADDR, 0x500     /* Address to store entry count */
.equ MMAP_ENTRIES_ADDR, 0x504   /* Address to store entries */
.equ E820_MAGIC, 0x534D4150     /* 'SMAP' in little-endian */

/* Kernel size in double-words for copy operation */
/* KERNEL_SECTORS * 512 bytes / 4 bytes per dword */
/* 32 sectors * 512 / 4 = 4096 dwords (16KB) */
.equ KERNEL_SIZE_DWORDS, 4096


/*
 * =============================================================================
 * Entry Point
 * =============================================================================
 */
_start:
    /* Clear direction flag for string operations */
    cld

    /* Save boot drive number immediately */
    movb %dl, boot_drive

    /* Print '2' to show stage 2 started */
    movb $'2', %al
    call print_char

    /* ====================================================================== */
    /* PHASE 1: Real Mode Operations (BIOS available)                         */
    /* ====================================================================== */

    /*
     * Step 1: Enable A20 line
     *
     * The A20 line must be enabled to access memory above 1MB.
     * We try the keyboard controller method first, then fast A20 as fallback.
     */
    call enable_a20
    call check_a20
    testb %al, %al
    jnz .a20_ok

    /* First attempt failed, try fast A20 */
    call enable_a20_fast
    call check_a20
    testb %al, %al
    jnz .a20_ok

    /* Second attempt failed, try keyboard controller again */
    call enable_a20
    call check_a20
    testb %al, %al
    jnz .a20_ok

    /* A20 enable failed after all attempts */
    jmp a20_error

.a20_ok:
    /* Print 'A' to show A20 is enabled */
    movb $'A', %al
    call print_char

    /*
     * Step 2: Get memory map from BIOS (E820)
     *
     * Query the BIOS for available memory regions. This information
     * is passed to the kernel for physical memory management.
     */
    call get_memory_map

    /* Print 'M' to show memory map retrieved */
    movb $'M', %al
    call print_char

    /*
     * Step 3: Load kernel from disk to low memory
     *
     * We load the kernel to 0x10000 (64KB) because:
     * - Real mode can only directly address the first 1MB
     * - We need BIOS to read the disk
     * - After switching to protected mode, we'll copy to 1MB
     */
    call load_kernel

    /* Print 'L' to show kernel loaded */
    movb $'L', %al
    call print_char

    /* ====================================================================== */
    /* PHASE 2: Switch to Protected Mode                                      */
    /* ====================================================================== */

    /*
     * The mode switch is a delicate operation that must be done in order:
     * 1. Disable interrupts (BIOS handlers won't work in PM)
     * 2. Load GDT
     * 3. Set CR0.PE bit
     * 4. Far jump to flush pipeline and load CS
     * 5. Reload segment registers
     */
    call enter_protected_mode

    /* Never returns - execution continues in protected mode */


/*
 * =============================================================================
 * A20 Line Functions
 * =============================================================================
 */

/*
 * enable_a20 - Enable A20 via keyboard controller (8042)
 *
 * The keyboard controller has an output port with bit 1 controlling A20.
 * This is the most compatible method but requires waiting for the controller.
 *
 * Port 0x64: Command/Status (read=status, write=command)
 * Port 0x60: Data port
 *
 * Status bit 1: Input buffer full (must wait until 0 before writing)
 *
 * Clobbers: AX
 */
enable_a20:
    /* Wait for input buffer to be empty */
    call wait_8042_input

    /* Send "write output port" command (0xD1) */
    movb $0xD1, %al
    outb %al, $0x64

    /* Wait for input buffer again */
    call wait_8042_input

    /* Write output port value with A20 enabled (0xDF) */
    /* 0xDF = 11011111b - all bits set including A20 (bit 1) */
    movb $0xDF, %al
    outb %al, $0x60

    /* Wait for command to complete */
    call wait_8042_input

    ret


/*
 * wait_8042_input - Wait for keyboard controller input buffer to be empty
 *
 * Polls status register until bit 1 (input buffer full) is clear.
 * Includes timeout to prevent infinite loop on broken hardware.
 *
 * Clobbers: AX, CX
 */
wait_8042_input:
    movw $0xFFFF, %cx           /* Timeout counter */
.wait_loop:
    inb $0x64, %al              /* Read status register */
    testb $0x02, %al            /* Test bit 1 (input buffer full) */
    jz .wait_done               /* Exit if buffer empty */
    decw %cx
    jnz .wait_loop              /* Continue if timeout not reached */
.wait_done:
    ret


/*
 * enable_a20_fast - Enable A20 via System Control Port A (port 0x92)
 *
 * Modern systems support a faster method using port 0x92.
 * WARNING: Bit 0 is system reset - must NOT set it!
 *
 * Clobbers: AX
 */
enable_a20_fast:
    inb $0x92, %al              /* Read current value */
    testb $0x02, %al            /* Check if A20 already enabled */
    jnz .fast_done              /* Already enabled, skip */
    orb $0x02, %al              /* Set bit 1 (A20 enable) */
    andb $0xFE, %al             /* Clear bit 0 (system reset) - CRITICAL! */
    outb %al, $0x92
.fast_done:
    ret


/*
 * check_a20 - Test if A20 line is enabled
 *
 * Strategy: Compare memory at 0x000500 with 0x100500. If A20 is disabled,
 * address 0x100500 wraps to 0x000500 (they're the same location).
 *
 * We use 0x500/0x100500 instead of 0x0/0x100000 to avoid the IVT.
 *
 * Returns: AL = 1 if A20 enabled, 0 if disabled
 * Clobbers: AX, BX, DS, ES, SI, DI
 */
check_a20:
    pushw %ds
    pushw %es
    pushw %si
    pushw %di

    /* Set up segments for comparison */
    /* ES:DI = 0x0000:0x0500 = physical 0x000500 */
    xorw %ax, %ax
    movw %ax, %es
    movw $0x0500, %di

    /* DS:SI = 0xFFFF:0x0510 = physical 0x100500 */
    /* 0xFFFF * 16 + 0x0510 = 0xFFFF0 + 0x510 = 0x100500 */
    movw $0xFFFF, %ax
    movw %ax, %ds
    movw $0x0510, %si

    /* Save original values at both locations */
    movb %es:(%di), %al
    pushw %ax
    movb %ds:(%si), %al
    pushw %ax

    /* Write different values to each location */
    movb $0x00, %es:(%di)       /* Write 0x00 to 0x000500 */
    movb $0xFF, %ds:(%si)       /* Write 0xFF to 0x100500 */

    /* Check if 0x000500 was overwritten (would happen if A20 disabled) */
    cmpb $0xFF, %es:(%di)

    /* Restore original values */
    popw %ax
    movb %al, %ds:(%si)
    popw %ax
    movb %al, %es:(%di)

    popw %di
    popw %si
    popw %es
    popw %ds

    /* Set return value based on comparison */
    jne .a20_is_enabled         /* Values differ = A20 enabled */
    xorb %al, %al               /* AL = 0: A20 disabled (wrapped) */
    ret
.a20_is_enabled:
    movb $0x01, %al             /* AL = 1: A20 enabled */
    ret


/*
 * =============================================================================
 * Memory Map Functions
 * =============================================================================
 */

/*
 * get_memory_map - Query BIOS for memory map using INT 0x15, EAX=0xE820
 *
 * The E820 interface returns memory map entries one at a time.
 * Each entry describes a region: base address, length, type.
 *
 * Entry types:
 *   1 = Usable RAM
 *   2 = Reserved
 *   3 = ACPI Reclaimable
 *   4 = ACPI NVS
 *   5 = Bad memory
 *
 * Stores entries at MMAP_ENTRIES_ADDR, count at MMAP_COUNT_ADDR.
 *
 * Clobbers: All general purpose registers
 */
get_memory_map:
    /* Initialize count to 0 */
    movl $0, (MMAP_COUNT_ADDR)

    /* ES:DI = buffer for entries */
    xorw %ax, %ax
    movw %ax, %es
    movw $MMAP_ENTRIES_ADDR, %di

    /* EBX = 0 for first call (continuation value) */
    xorl %ebx, %ebx

.e820_loop:
    /* Set up E820 call */
    movl $0xE820, %eax          /* E820 function number */
    movl $E820_MAGIC, %edx      /* 'SMAP' signature */
    movl $24, %ecx              /* Buffer size (24 bytes per entry) */
    int $0x15                   /* Call BIOS */

    /* Check for errors */
    jc .e820_done               /* Carry set = error or unsupported */
    cmpl $E820_MAGIC, %eax      /* BIOS returns 'SMAP' on success */
    jne .e820_done

    /* Valid entry received - check if it has non-zero size */
    /* Entry format: base(8), length(8), type(4), extended(4) */
    /* Only count entries with length > 0 */
    movl 8(%di), %eax           /* Low dword of length */
    orl 12(%di), %eax           /* High dword of length */
    jz .e820_skip               /* Skip zero-length entries */

    /* Increment count and advance buffer pointer */
    incl (MMAP_COUNT_ADDR)
    addw $24, %di

.e820_skip:
    /* EBX = 0 means this was the last entry */
    testl %ebx, %ebx
    jz .e820_done

    /* Continue to next entry */
    jmp .e820_loop

.e820_done:
    ret


/*
 * =============================================================================
 * Kernel Loading Functions
 * =============================================================================
 */

/*
 * load_kernel - Load kernel from disk to low memory (0x10000)
 *
 * Uses BIOS INT 0x13 to read sectors. Must be called before protected mode
 * switch since BIOS is unavailable in protected mode.
 *
 * The kernel is loaded in chunks because BIOS can only read a limited number
 * of sectors at once (typically 128 max, but we use smaller chunks for safety).
 *
 * Clobbers: All general purpose registers
 */
load_kernel:
    /* Set up destination address ES:BX = 0x1000:0x0000 = 0x10000 */
    movw $KERNEL_LOAD_SEG, %ax
    movw %ax, %es
    xorw %bx, %bx

    /* Initialize sector counter and current sector */
    movw $KERNEL_SECTORS, %cx   /* Total sectors to read */
    movb $KERNEL_START_SECTOR, %cl  /* Actually store start sector for CHS */
    movw $KERNEL_SECTORS, %si   /* Sectors remaining counter */

    /* Read sectors in chunks of up to 64 sectors at a time */
    movb $KERNEL_START_SECTOR, sector_num
    movw $KERNEL_LOAD_SEG, segment_num

.read_loop:
    /* Check if we have sectors remaining */
    cmpw $0, %si
    je .load_done

    /* Calculate how many sectors to read this iteration (max 64) */
    movw %si, %ax
    cmpw $64, %ax
    jbe .chunk_ok
    movw $64, %ax
.chunk_ok:
    movw %ax, %di               /* DI = sectors to read this chunk */

    /* Set up BIOS disk read */
    /* Store chunk size in a memory location since we can't use %dil in 16-bit */
    movw %di, %ax
    movb %al, chunk_size        /* Save low byte of chunk count */

    movb (boot_drive), %dl      /* Drive number */
    movb $0x02, %ah             /* Read sectors function */
    movb (chunk_size), %al      /* Number of sectors */
    movb $0, %ch                /* Cylinder 0 */
    movb (sector_num), %cl      /* Sector number */
    movb $0, %dh                /* Head 0 */
    movw (segment_num), %ax
    movw %ax, %es
    xorw %bx, %bx               /* ES:BX = destination */
    movb $0x02, %ah             /* Restore AH (clobbered by ES load) */
    movb (chunk_size), %al      /* Restore AL */

    /* Retry loop for reliability */
    movb $3, retry_count
.retry_read:
    pusha
    int $0x13
    jnc .read_ok
    popa

    /* Reset disk and retry */
    decb retry_count
    jz .disk_error
    xorb %ah, %ah
    int $0x13
    jmp .retry_read

.read_ok:
    popa

    /* Update counters for next chunk */
    subw %di, %si               /* Remaining sectors -= chunk size */

    /* Advance sector number (CHS sector 1-63, then head/cylinder) */
    /* For simplicity, we assume kernel fits on cylinder 0 */
    movb (chunk_size), %al
    addb %al, (sector_num)

    /* Advance destination segment (each sector = 512 bytes = 0x20 paragraphs) */
    movw %di, %ax
    shlw $5, %ax                /* AX = sectors * 32 (paragraphs per sector) */
    addw %ax, (segment_num)

    jmp .read_loop

.load_done:
    ret


/*
 * =============================================================================
 * Protected Mode Switch
 * =============================================================================
 */

/*
 * enter_protected_mode - Switch CPU from real mode to protected mode
 *
 * This function performs the critical mode transition:
 *   1. Disable interrupts
 *   2. Load GDT
 *   3. Set CR0.PE bit
 *   4. Far jump to flush pipeline and load CS
 *
 * This function NEVER returns - execution continues at protected_mode_entry.
 *
 * Clobbers: Everything (we're switching modes)
 */
enter_protected_mode:
    /*
     * Step 1: Disable interrupts
     *
     * We MUST disable interrupts before switching modes. BIOS interrupt
     * handlers are 16-bit real mode code - they will crash in protected mode.
     * We won't enable interrupts again until we set up an IDT (Story 2.1).
     */
    cli

    /*
     * Step 2: Load the Global Descriptor Table
     *
     * The GDT defines memory segments for protected mode. The CPU requires
     * a valid GDT before entering protected mode.
     */
    lgdt (gdt_ptr)

    /*
     * Step 3: Enable protected mode
     *
     * Set bit 0 (PE - Protection Enable) in Control Register 0.
     * After this instruction, we're technically in protected mode, but
     * still executing 16-bit code with stale segment registers.
     */
    movl %cr0, %eax
    orl $0x00000001, %eax       /* Set PE bit */
    movl %eax, %cr0

    /*
     * Step 4: Far jump to flush pipeline and load CS
     *
     * The CPU may have prefetched 16-bit instructions. The far jump:
     *   - Flushes the instruction pipeline
     *   - Loads CS with our 32-bit code segment selector
     *   - Continues execution at the 32-bit code label
     */
    ljmp $CODE_SEG, $protected_mode_entry


/*
 * =============================================================================
 * Error Handlers
 * =============================================================================
 */

a20_error:
    movb $'!', %al
    call print_char
    movb $'A', %al
    call print_char
    movb $'2', %al
    call print_char
    movb $'0', %al
    call print_char
    jmp halt

.disk_error:
disk_error:
    movb $'!', %al
    call print_char
    movb $'D', %al
    call print_char
    movb $'S', %al
    call print_char
    movb $'K', %al
    call print_char
    jmp halt

halt:
    cli
    hlt
    jmp halt


/*
 * =============================================================================
 * Utility Functions
 * =============================================================================
 */

/*
 * print_char - Print a character using BIOS INT 0x10
 *
 * Input: AL = character to print
 * Clobbers: AH, BH
 */
print_char:
    movb $0x0E, %ah             /* BIOS teletype function */
    xorb %bh, %bh               /* Page 0 */
    int $0x10
    ret


/*
 * =============================================================================
 * 32-BIT PROTECTED MODE CODE
 * =============================================================================
 *
 * Everything below this point is 32-bit code that runs after the mode switch.
 * The .code32 directive tells the assembler to generate 32-bit instructions.
 */

.code32

/*
 * protected_mode_entry - First 32-bit code after mode switch
 *
 * At this point:
 *   - CPU is in 32-bit protected mode
 *   - CS contains CODE_SEG (0x08) from the far jump
 *   - Other segment registers still have garbage (must reload)
 *   - Interrupts are disabled
 *   - Paging is disabled (physical = virtual)
 */
protected_mode_entry:
    /*
     * Step 5: Reload all data segment registers
     *
     * The segment registers still contain real mode values, which are
     * invalid in protected mode. Load them with our data segment selector.
     */
    movw $DATA_SEG, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    /*
     * Step 6: Set up stack
     *
     * Set up a stack at 0x90000. This is above where we loaded the kernel
     * (0x10000-0x8FFFF) and below the EBDA/video memory area.
     */
    movl $PM_STACK, %esp

    /*
     * Step 7: Copy kernel from low memory to 1MB
     *
     * The kernel was loaded to 0x10000 by BIOS. Now we copy it to its
     * final location at 0x100000 (1MB) where it will execute.
     */
    cld                         /* Clear direction flag (copy forward) */
    movl $KERNEL_LOW_ADDR, %esi /* Source: 0x10000 */
    movl $KERNEL_HIGH_ADDR, %edi /* Destination: 0x100000 */
    movl $KERNEL_SIZE_DWORDS, %ecx /* Count in double-words */
    rep movsl                   /* Copy ECX dwords from ESI to EDI */

    /*
     * Step 8: Prepare registers for kernel entry
     *
     * We pass boot information to the kernel via registers:
     *   EAX = 0 (reserved for magic number in future)
     *   EBX = pointer to memory map entries
     *   ECX = number of memory map entries
     *   EDX = 0 (reserved)
     */
    xorl %eax, %eax
    movl $MMAP_ENTRIES_ADDR, %ebx
    movl (MMAP_COUNT_ADDR), %ecx
    xorl %edx, %edx
    xorl %ebp, %ebp             /* Clear frame pointer */

    /*
     * Step 9: Jump to kernel!
     *
     * Transfer control to the kernel entry point at 1MB.
     * The kernel takes over from here - we never return.
     */
    jmp *kernel_entry_addr

/* Kernel entry point address (indirect jump target) */
kernel_entry_addr:
    .long KERNEL_HIGH_ADDR


/*
 * =============================================================================
 * DATA SECTION
 * =============================================================================
 */

.section .data

/* Boot drive number saved from DL */
boot_drive:
    .byte 0

/* Disk read state variables */
sector_num:
    .byte 0
segment_num:
    .word 0
retry_count:
    .byte 0
chunk_size:
    .byte 0


/*
 * =============================================================================
 * Global Descriptor Table (GDT)
 * =============================================================================
 *
 * The GDT defines memory segments for protected mode. Each entry is 8 bytes
 * and describes a segment's base address, limit, and access permissions.
 *
 * We use a "flat" memory model where both code and data segments span the
 * entire 4GB address space. This is standard for modern OSes - we'll use
 * paging (not segmentation) for memory protection.
 *
 * GDT Entry Format (8 bytes):
 *   Bytes 0-1: Limit bits 0-15
 *   Bytes 2-3: Base bits 0-15
 *   Byte 4:    Base bits 16-23
 *   Byte 5:    Access byte
 *   Byte 6:    Flags (4 bits) + Limit bits 16-19 (4 bits)
 *   Byte 7:    Base bits 24-31
 *
 * Note: GDT is in .rodata since it's read-only after LGDT loads it.
 */

.section .rodata
.align 8
gdt_start:
    /*
     * Entry 0: Null Descriptor (required by CPU)
     *
     * The CPU requires the first GDT entry to be null. Any attempt to
     * load a segment register with selector 0x00 causes a fault.
     */
    .quad 0x0000000000000000

    /*
     * Entry 1: Kernel Code Segment (selector = 0x08)
     *
     * Base = 0x00000000, Limit = 0xFFFFF (with 4KB granularity = 4GB)
     * Access = 0x9A: Present, Ring 0, Code segment, Executable, Readable
     * Flags = 0xC: 4KB granularity, 32-bit segment
     *
     * Breakdown:
     *   Limit 0-15:  0xFFFF
     *   Base 0-15:   0x0000
     *   Base 16-23:  0x00
     *   Access:      0x9A = 10011010b (Present=1, DPL=00, S=1, Type=1010)
     *   Flags+Limit: 0xCF = 11001111b (G=1, D=1, L=0, AVL=0, Limit=F)
     *   Base 24-31:  0x00
     */
    .word 0xFFFF                /* Limit bits 0-15 */
    .word 0x0000                /* Base bits 0-15 */
    .byte 0x00                  /* Base bits 16-23 */
    .byte 0x9A                  /* Access: present, ring 0, code, readable */
    .byte 0xCF                  /* Flags: 4KB granularity, 32-bit */
    .byte 0x00                  /* Base bits 24-31 */

    /*
     * Entry 2: Kernel Data Segment (selector = 0x10)
     *
     * Same as code segment except:
     *   Access = 0x92: Data segment (not executable), Writable
     */
    .word 0xFFFF                /* Limit bits 0-15 */
    .word 0x0000                /* Base bits 0-15 */
    .byte 0x00                  /* Base bits 16-23 */
    .byte 0x92                  /* Access: present, ring 0, data, writable */
    .byte 0xCF                  /* Flags: 4KB granularity, 32-bit */
    .byte 0x00                  /* Base bits 24-31 */

gdt_end:

/*
 * GDT Pointer Structure (for LGDT instruction)
 *
 * The LGDT instruction expects a 6-byte structure:
 *   - 2 bytes: Limit (size of GDT minus 1)
 *   - 4 bytes: Base address of GDT
 */
gdt_ptr:
    .word gdt_end - gdt_start - 1   /* Limit: GDT size - 1 */
    .long gdt_start                  /* Base: GDT address */


/*
 * =============================================================================
 * PADDING
 * =============================================================================
 *
 * Stage 2 occupies sectors 1-4 (2KB total). We pad to ensure the binary
 * is at least large enough. The Makefile will handle final sizing.
 */
