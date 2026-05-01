#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (atoi(argv[1]) == 423) {
        char *arg[2];
        arg[0] = strdup("/bin/sh");
        arg[1] = NULL;

        gid_t gid = getegid();
        uid_t uid = geteuid();

        setresgid(gid, gid, gid);
        setresuid(uid, uid, uid);

        execv("/bin/sh", arg);
    } 
    else {
        fwrite("No !\n", 1, 5, stderr);
    }
    return 0;
}