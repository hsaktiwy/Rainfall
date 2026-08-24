section .text
global _start

_start:
    mov rax, 0x00000000004013d2
    push rbp
    mov rbp, rsp
    call rax
    leave
    ret