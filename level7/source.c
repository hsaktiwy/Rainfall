#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

char c[68];

struct DataObj {
    int id;
    char *buffer;
};

void m() {
    printf("%s - %d\n", c, (int)time(NULL));
}

int main(int argc, char **argv) {
    struct DataObj *obj1;
    struct DataObj *obj2;
    FILE *stream;

    obj1 = malloc(sizeof(struct DataObj));
    obj1->id = 1;
    obj1->buffer = malloc(8);
    obj2 = malloc(sizeof(struct DataObj));
    obj2->id = 2;
    obj2->buffer = malloc(8);
    strcpy(obj1->buffer, argv[1]);
    strcpy(obj2->buffer, argv[2]);
    stream = fopen("/home/user/level8/.pass", "r");
    if (stream != NULL) {
        fgets(c, 68, stream);
    }
    puts("~~");
    return 0;
}