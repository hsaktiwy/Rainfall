#include <stdio.h>

void p(void *arg) {

    char buffer[64];//0xc -> 12
    //76
    // Flush stdout
    fflush(stdout);
    gets(buffer);
    void *arg_copy = arg;
    if ((buffer & 0xb0000000) == 0xb0000000) {
        _exit(1);
    }
        printf("(%p)\n", arg_copy);

    puts(buffer);
    strdup(buffer);
    return;
}

int main()
{
    p();
    return 0;
}