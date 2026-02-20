    .section .text
    .globl _start

write_char:
    adrp x1, magic
    ldr x1, [x1, :lo12:magic]
    strb w0, [x1]
    ret

write_string:
    ldr w1, [x0]
    subs w1, w1, wzr
    b.eq .Lend // If == 0.
    adrp x3, magic
    ldr x2, [x3, :lo12:magic]
.Lstart:
    strb w1, [x2]
    ldr w1, [x0, #1]!
    subs w1, w1, wzr
    b.ne .Lstart
.Lend:
    ret

_start:
    subs x0, x0, xzr
    b.eq main // Only run main if you are the first thread.
    ret

main:
    adrp x1, .rodata
    add x1, x1, :lo12:.rodata
    movz w0, #0x48
    adrp x3, magic
    ldr x2, [x3, :lo12:magic]
.Lprint_loop:
    strb w0, [x2]
    ldr w0, [x1, #1]!
    subs w0, w0, wzr
    b.ne .Lprint_loop
    ret

    .section .rodata
    .asciz "?i. :D\n"
    .align 3

    .section .data
magic:
    .quad 0xffffffffffffffff
    