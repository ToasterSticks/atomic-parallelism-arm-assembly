    .section .text
    .globl _start

_start:
    adrp x4, counter
    add x4, x4, :lo12:counter

    subs x0, x0, xzr
    b.eq waiting

    // Not the first thread.
    movz x5, #1
    ldaddal x5, x5, [x4] // Increment counter.

    udf #0

waiting: // First thread only.
    // Wait for counter to be accessed by all other threads.
    movz x1, #3
    movz x5, #0x51
.Lwait_loop:
    subs x3, x1, x15 // Reset x3.
    casal x3, x5, [x4]
    subs x3, x3, x1
    b.ne .Lwait_loop

    ldr x29, [x4]
    udf #0x445 // Invalid instruction.

    .section .data
counter:
    .quad 0
