    .section .text
    .globl _start

_start:
    subs x0, x0, xzr
    b.eq main                 // Correct run should jump to main.
    udf #0                    // Terminate non zero threads immediately.

main:
    adrp x20, slot
    add x20, x20, :lo12:slot

    movz x1, #0xaaaa
    movz x2, #0xbbbb, lsl #16
    orr x3, xzr, #0xff

    subs x5, x3, x3
    csel x6, x1, x2, eq
    subs x5, x3, x1
    csel x7, x1, x2, eq

    str x6, [x20]
    ldr x8, [x20]
    str w7, [x20, #8]
    ldr w9, [x20, #8]
    strb w3, [x20, #12]
    ldr w10, [x20, #12]

    add x11, x8, #5
    add x12, x11, #1, lsl #12

    movz x14, #0xdead
    subs x13, x8, x6
    b.ne done
    subs x13, x9, x2
    strb w13, [sp, #-1]!      // (Print) Move some byte to 0xFFFFFFFFFFFFFFFF.
    movz w14, #0x0a           // Immediate newline byte.
    strb w14, [sp]            // Print '\n'.
    b.ne done                 // Should jump.
    subs x13, x10, x3
    b.ne done
    movz x14, #0x600d

done:
    udf #0x999                // Normal termination point.

    .section .data
    .align 4
slot:
    .quad 0
    .quad 0
