# Level 9 — Writeup: C++ Heap Overflow & VTable Hijacking

## Vulnerability Analysis

By examining the assembly and using `ltrace`, we can see this binary was compiled from C++. The `main` function does the following:

1. **Object Allocation:** It creates two objects of a class (let's call it `N`) on the heap using the `new` operator (`_Znwj(108)`).
* Object 1 (`obj1`) is allocated at `0x0804a008`.
* Object 2 (`obj2`) is allocated adjacent to it at `0x0804a078`.


2. **The Vulnerable Copy:** It calls a method `N::setAnnotation(char*)` on `obj1`, passing our command-line argument (`argv[1]`). Inside this method, `memcpy` is used to copy our input into the object's internal buffer. **There are no bounds checks.**
3. **The Trigger:** At the very end of the program, it calls a virtual function on `obj2`.

### The Magic of C++ VTables

In C++, when a class has virtual functions, the compiler needs a way to know which function to execute at runtime. It does this by secretly inserting a hidden pointer at the very beginning of the object's memory (offset `+0`). This is called the **Virtual Table Pointer (VTable Pointer)**, and it points to an array of function addresses.

The vulnerability relies on the layout of these objects in the heap:

* `obj1` starts at `0x0804a008`. Its VTable pointer is the first 4 bytes.
* `obj1`'s character buffer starts right after it at `0x0804a00c` (offset `+4`).
* `obj2` starts exactly 108 bytes later at `0x0804a078`. The first 4 bytes of `obj2` are its VTable pointer.

Because `memcpy` doesn't stop copying, if we provide an argument longer than 108 bytes, we will spill out of `obj1`'s buffer and directly overwrite `obj2`'s VTable pointer!

## The Exploit Strategy: The "Fake VTable"

We can't just overwrite the VTable pointer with the address of our shellcode. If we do, the CPU will try to read memory *at* our shellcode to find a function address, which will cause a crash.

Instead, we must build a **Fake VTable**.

1. **The Fake VTable Location:** We control `obj1`'s buffer (starting at `0x0804a00c`). We will place a pointer here that points to our shellcode.
2. **The Shellcode Location:** We will place our actual shellcode right after our fake VTable, starting at `0x0804a010`.
3. **The Hijack:** We will overflow the buffer until we reach `obj2`'s VTable pointer (108 bytes away). We will overwrite `obj2`'s VTable pointer with the address of our Fake VTable (`0x0804a00c`).

**The Execution Flow:**
When the program tries to call `obj2`'s virtual method, it will look at `obj2`'s VTable pointer, which we changed to `0x0804a00c`. It will then look inside `0x0804a00c`, find the address `0x0804a010`, and execute the shellcode located there!

## Payload Construction

* **Fake VTable Pointer:** `0x0804a010` (Little Endian: `\x10\xa0\x04\x08`)
* **Shellcode:** Standard 23-byte execve `/bin/sh` shellcode.
* **Padding:** The total distance to `obj2`'s VTable is 108 bytes.
* 108 bytes - 4 bytes (Fake VTable) - 23 bytes (Shellcode) = **81 bytes of padding**.


* **Overwrite Value:** `0x0804a00c` (Little Endian: `\x0c\xa0\x04\x08`)

**Execution:**

```bash
level9@RainFall:~$ ./level9 $(python2 -c 'print "\x10\xa0\x04\x08" + "\x31\xc0\x50\x68\x2f\x2f\x73\x68\x68\x2f\x62\x69\x6e\x89\xe3\x31\xc9\x31\xd2\xb0\x0b\xcd\x80" + "A"*81 + "\x0c\xa0\x04\x08"')
$ whoami
bonus0
$ cd /home/user/bonus0
$ cat .pass
f3f0004b6f364cb5a4147e9ef827fa922a4861408845c26b6971ad770d906728

```