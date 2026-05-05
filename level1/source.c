#include <stdio.h>

void run()
{
    fwrite("Good... Wait what?\n", 1, 0x13, stdout);
    system("/bin/sh");
}
int main()
{
    char buffer[0x50];
    gets(buffer);
    return 0;
}