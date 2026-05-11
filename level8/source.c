#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *auth = NULL;
char *service = NULL;

int main(int argc, char **argv) {
    char buffer[128];

    while (1) {
        printf("%p, %p \n", auth, service);

        if (fgets(buffer, 128, stdin) == NULL) {
            break;
        }

        if (memcmp(buffer, "auth ", 5) == 0) {
            auth = malloc(4);
            memset(auth, 0, 4);
            if (strlen(buffer + 5) <= 30) {
                strcpy(auth, buffer + 5);
            }
        }

        if (memcmp(buffer, "reset", 5) == 0) {
            free(auth);
        }
        if (memcmp(buffer, "servic", 6) == 0) {
            service = strdup(buffer + 7); 
        }

        if (memcmp(buffer, "login", 5) == 0) {
            if ( *((int *)(auth + 32)) != 0 ) {
                system("/bin/sh");
            } else {
                fwrite("Password:\n", 1, 10, stdout);
            }
        }
    }

    return 0;
}