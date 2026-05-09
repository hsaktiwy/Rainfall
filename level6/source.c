#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void n() {
    system("/bin/cat /home/user/level7/.pass");
}

void m() {
    puts("Nope");
}

int main(int argc, char **argv) {
    char *buf1 = malloc(64);
    void (**func_ptr)() = malloc(4);

    *func_ptr = m;
    strcpy(buf1, argv[1]); 
    (*func_ptr)(); 
    
    return 0;
}