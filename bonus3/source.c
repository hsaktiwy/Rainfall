#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
    char buffer[132]; 
    FILE *file;
    memset(buffer, 0, 132);
    file = fopen("/home/user/end/.pass", "r");
    if (file == NULL || argc != 2) {
        return -1;
    }
    fread(buffer, 1, 66, file);
    int index = atoi(argv[1]);
    buffer[index] = '\0';

    fread(buffer + 66, 1, 65, file);
    fclose(file);

    if (strcmp(buffer, argv[1]) == 0) {
        execl("/bin/sh", "sh", NULL);
    } else {
        puts(buffer + 66);
    }

    return 0;
}
