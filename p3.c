#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main() {
    // read the fun program in through mmap
    puts("    .data");
    puts("format: .byte '%', 'd', 10, 0");
    puts("    .text");
    puts("    .global main");
    puts("main:");
    puts("    mov $0,%rax");
    puts("    lea format(%rip),%rdi");
    puts("    mov $42,%rsi");
    puts("    .extern printf");
    puts("    call printf");
    puts("    mov $0,%rax");
    puts("    ret");
    puts("    .section .note.GNU-stack");

    return 0;
}
