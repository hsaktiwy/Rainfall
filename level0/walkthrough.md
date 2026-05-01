using gdb to analyze the assembly code of the source.c file in level0, we can see that the program takes an input from the user, converts it to an integer using atoi, and compares it to the value 0x1a7 (423 in decimal). If the input is not equal to 423, the program jumps to a different part of the code. If the input is equal to 423, then the executable extract the effective user ID and group ID (level1, users), and set them as the real and effective IDs for the process, allowing it to execute a shell with elevated privileges (level1).
for more details, please refer to the level0_asm.md file in the Ressources folder.
*** Our Exploit: ***
```sh
level0@RainFall:~$ ./level0 423
$ whoami
level1
$ groups
level0 level1 users
$ pwd
/home/user/level0
$ cd /home/user/level1
$ cat .pass
1fe8a524fa4bec01ca4ea2a869af2a02260d4a7d5fe7e7c24d8617e6dca12d3a
```
*** PASS: 1fe8a524fa4bec01ca4ea2a869af2a02260d4a7d5fe7e7c24d8617e6dca12d3a ***