#!/bin/bash

# 1. Assemble and Link
nasm -f elf32 "$1" -o temp.o
ld -m elf_i386 temp.o -o temp.bin

# 2. Extract and format
echo -n 'Shellcode: "'
objdump -d temp.bin | grep '[0-9a-f]:' | grep -v 'file' | cut -f2 -d: | cut -f1-6 -d' ' | tr -s ' ' | tr '\t' ' ' | sed 's/ $//g' | sed 's/ /\\x/g' | paste -d '' -s | sed 's/^/\\x/' | tr -d '\n'
echo '"'

# 3. Cleanup
rm temp.o temp.bin