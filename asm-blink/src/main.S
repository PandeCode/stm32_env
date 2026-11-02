// blinks LD2 (PA5) at 1 Hz in thumb-2 assembly, startup and all: the vector
// table, a reset handler, and SysTick for the delay. the addresses are the
// same ones stm32h5xx.h gives the c blink, from rm0481
        .syntax unified
        .cpu cortex-m33
        .thumb

        .equ CPU_HZ, 32000000
        .equ RCC_AHB2ENR, 0x44020C8C
        .equ GPIOAEN, 1 << 0
        .equ GPIOA_MODER, 0x42020000
        .equ GPIOA_BSRR, 0x42020018
        .equ LD2, 5
        // systick's control register, LOAD and VAL follow it
        .equ SYST_CSR, 0xE000E010
        .equ SYST_ENABLE_CPUCLK, 0x5
        .equ SYST_COUNTFLAG, 1 << 16

        // the core loads sp from word 0 and jumps to word 1, the rest are
        // the exceptions, all caught by one handler here
        .section .isr_vector, "a"
        .word _estack
        .word Reset_Handler
        .rept 14
        .word Default_Handler
        .endr

        .text
        .global Reset_Handler
        .thumb_func
Reset_Handler:
        // GPIOA clock on, the read back makes it land before GPIOA is touched
        ldr r0, =RCC_AHB2ENR
        ldr r1, [r0]
        orr r1, r1, #GPIOAEN
        str r1, [r0]
        ldr r1, [r0]

        // PA5 as output: MODER bits 11:10 to 01
        ldr r0, =GPIOA_MODER
        ldr r1, [r0]
        bic r1, r1, #(3 << (LD2 * 2))
        orr r1, r1, #(1 << (LD2 * 2))
        str r1, [r0]

        // systick wraps every 1 ms off the core clock
        ldr r0, =SYST_CSR
        ldr r1, =CPU_HZ / 1000 - 1
        str r1, [r0, #4]
        movs r1, #0
        str r1, [r0, #8]
        movs r1, #SYST_ENABLE_CPUCLK
        str r1, [r0]

        // BSRR: the low half sets pins, the high half clears them
        .global main
main:
        ldr r4, =GPIOA_BSRR
        movs r5, #(1 << LD2)
        lsls r6, r5, #16
blink:
        str r5, [r4]
        movw r0, #500
        bl delay_ms
        str r6, [r4]
        movw r0, #500
        bl delay_ms
        b blink

// waits r0 ms, counting systick wraps. countflag clears when read
        .thumb_func
delay_ms:
        ldr r1, =SYST_CSR
1:      ldr r2, [r1]
        tst r2, #SYST_COUNTFLAG
        beq 1b
        subs r0, r0, #1
        bne 1b
        bx lr

        .thumb_func
Default_Handler:
        b Default_Handler
