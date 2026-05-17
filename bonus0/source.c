#include <stdio.h>
#include <string.h>
#include <unistd.h>

void p(char *dest, char *prompt) {
    char local_buf[4096];
    puts(prompt);
    read(0, local_buf, 4096);
    char *newline = strchr(local_buf, '\n');
    if (newline != NULL) {
        *newline = '\0';
    }
    strncpy(dest, local_buf, 20); 
}

void pp(char *result) {
    char buf1[20];
    char buf2[20];
    
    p(buf1, (char *)0x80486a0); 
    p(buf2, (char *)0x80486a0);
    
    strcpy(result, buf1);
    int len = strlen(result);
    *(short *)(result + len) = *(short *)0x80486a4;
    strcat(result, buf2);
    
}

int main() {
    char buffer[42]; 
    pp(buffer);
    puts(buffer);
    return 0;
}