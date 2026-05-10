#include <stdio.h>
#include <stdlib.h>

int main()
{
    void *auth_ptr = NULL;
    void *service_ptr = NULL;

    while (1) {
        printf("%p, %p \n", auth_ptr, service_ptr);
        
        fgets(buffer, 128, stdin);

        if (strncmp(buffer, "auth ", 5) == 0) {
            auth_ptr = malloc(4);
            memset(auth_ptr, 0, 4);
            if (strlen(buffer + 5) <= 30) {
                strcpy(auth_ptr, buffer + 5); 
            }
        }
        
        else if (strncmp(buffer, "reset", 5) == 0) {
            free(auth_ptr);
        }
        
        else if (strncmp(buffer, "servic", 6) == 0) {
            service_ptr = strdup(buffer + 7);
        }
        else if (strncmp(buffer, "login", 5) == 0) {
            if ( *(int*)(auth_ptr + 32) != 0 ) { 
                system("/bin/sh"); 
            } else {
                fwrite("Password:\n", 1, 10, stdout);
            }
        }
    }
}