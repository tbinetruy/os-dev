/*
 * kernel/init/gdt_flush.S - Load GDT and reload segment registers
 *
 * This assembly routine loads a new GDT into the CPU's GDTR register
 * and reloads all segment registers with the new kernel selectors.
 *
 * After LGDT, we must reload CS via a far jump (ljmp) and then
 * reload all data segment registers (DS, ES, FS, GS, SS).
 *
 * References:
 *   - Intel SDM Vol 3, Section 3.4.3: Segment Registers
 *   - Intel SDM Vol 3, Section 3.4.4: Segment Loading Instructions
 */

.code32
.section .text

/*
 * gdt_flush - Load new GDT and reload segment registers
 *
 * Purpose:
 *   Load the GDT pointer into GDTR and reload all segment registers
 *   with the new kernel selectors.
 *
 * Input:
 *   4(%esp) - Linear address of gdt_ptr structure (6 bytes: limit + base)
 *
 * Output:
 *   None (returns normally)
 *
 * Clobbers:
 *   EAX
 *
 * Segment register state after call:
 *   CS = 0x08 (KERNEL_CS)
 *   DS = ES = FS = GS = SS = 0x10 (KERNEL_DS)
 */
.global gdt_flush
.type gdt_flush, @function
gdt_flush:
    /* Get pointer to gdt_ptr structure from stack */
    movl 4(%esp), %eax

    /* Load GDT register
     *
     * LGDT loads the 6-byte value at the operand address into GDTR:
     *   - Bytes 0-1: GDT limit (size - 1)
     *   - Bytes 2-5: GDT base address
     */
    lgdt (%eax)

    /*
     * Reload CS via far jump
     *
     * CS cannot be loaded with a MOV instruction. The only way to
     * change CS is through a far jump, far call, or interrupt return.
     *
     * The far jump format is: ljmp $segment, $offset
     * We jump to .reload_segments with CS = 0x08 (KERNEL_CS)
     */
    ljmp $0x08, $.reload_segments

.reload_segments:
    /*
     * Reload data segment registers
     *
     * All data segment registers (DS, ES, FS, GS, SS) can be loaded
     * with MOV. We set them all to KERNEL_DS (0x10).
     *
     * Note: SS reload temporarily disables interrupts for the next
     * instruction to prevent stack issues.
     */
    movw $0x10, %ax         /* KERNEL_DS selector */
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    ret

.size gdt_flush, . - gdt_flush
