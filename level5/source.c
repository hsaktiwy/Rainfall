#include <stdio.h>
void o(void)
{
    system("/bin/sh");
    _exit(1);
}

void n(void)
{
    char v0[520]; 

    fgets(&v0, 0x200, stdin);
    printf(&v0);
    exit(1);
}

int main(void)
{
    n();
    return 0;
}
