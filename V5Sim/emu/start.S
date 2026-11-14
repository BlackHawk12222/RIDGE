/* V5Sim fake-VEXos boot code for QEMU's xilinx-zynq-a9 (the V5 brain is a Zynq-7000).
 * QEMU enters here in SVC mode with the MMU off. We set up the mode stacks, the FPU,
 * a flat MMU map (normal memory for DDR, device memory for peripherals), then call
 * vexos_main(), which jumps into the PROS program the same way VEXos does. */
    .syntax unified
    .arm
    .section .text.boot, "ax"
    .global _start
_start:
    cpsid   if
    /* Mode stacks (IRQ/FIQ/ABT/UND). SVC keeps a boot stack; PROS's vexMain swaps in its own. */
    cps     #0x12           /* IRQ */
    ldr     sp, =__irq_stack_top
    cps     #0x11           /* FIQ */
    ldr     sp, =__fiq_stack_top
    cps     #0x17           /* ABT */
    ldr     sp, =__abt_stack_top
    cps     #0x1b           /* UND */
    ldr     sp, =__und_stack_top
    cps     #0x1f           /* SYS */
    ldr     sp, =__sys_stack_top
    cps     #0x13           /* SVC */
    ldr     sp, =__svc_stack_top

    /* Enable VFP/NEON: CPACR cp10/cp11 full access, then FPEXC.EN */
    mrc     p15, 0, r0, c1, c0, 2
    orr     r0, r0, #(0xf << 20)
    mcr     p15, 0, r0, c1, c0, 2
    isb
    mov     r0, #0x40000000
    vmsr    fpexc, r0

    /* Clear .bss */
    ldr     r0, =__bss_start
    ldr     r1, =__bss_end
    mov     r2, #0
1:  cmp     r0, r1
    strlo   r2, [r0], #4
    blo     1b

    bl      mmu_init
    bl      vexos_main
2:  b       2b

/* Jump into the user program like VEXos: SVC mode, IRQs masked. r0 = entry. */
    .global vexos_jump
vexos_jump:
    bx      r0

/* Default exception vectors used before PROS installs its own VBAR table. */
    .align 5
    .global early_vectors
early_vectors:
    b   .
    b   .
    b   .
    b   .
    b   .
    nop
    b   .
    b   .
