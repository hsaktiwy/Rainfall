#include <stdio.h>

extern unsigned int m;

void v(void)
{
    char v0[520];  // [bp-0x20c]

    fgets(&v0, 0x200, stdin);
    printf(&v0);
    if (m == 64)
    {
        fwrite("Wait what?!\n", 1, 12, stdout);
        system("/bin/sh");
    }
    return;
}

void main(void)
{
    v();
    return;
}