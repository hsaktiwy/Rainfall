#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int m = 0;

void p(char *buffer) {
    printf(buffer);
}

void n() {
    char buffer[512];
    if (fgets(buffer, 512, stdin) == NULL) {
        return;
    }

    p(buffer);
    if (m == 0x1025544) {
        system("/bin/cat /home/user/level5/.pass");
    }
}

int main() {
    n();
    return 0;
}