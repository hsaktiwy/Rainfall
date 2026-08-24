nasm -f elf64 shellcode.s -o shellcode.o
ld shellcode.o -o shellcode

# Extract shellcode bytes format
objdump -d shellcode | grep '[0-9a-f]:' | grep -v 'file' | cut -f2 -d: | cut -f1-7 -d' ' | tr -s ' ' | tr '\t' ' ' | sed 's/ $//g' | sed 's/ /\\x/g' | paste -d '' -s