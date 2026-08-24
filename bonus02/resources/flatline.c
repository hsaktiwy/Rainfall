#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

struct construct;

typedef void (*execute_fn_t)(struct construct *);

struct construct {
    int count;
    int flag;
    char buff1[32];
    char buff2[64];
    execute_fn_t execute;
};

struct construct *constructs[8];
uint32_t construct_count = 0;


void default_execute(struct construct *con);
void print_banner(void);
struct construct *new_construct(const char *arg1);
void delete_construct(unsigned int index);
void update_construct(unsigned int index);
void run_construct(unsigned int index);


void default_execute(struct construct *con) {
    printf("[FLATLINE] Executing construct %u: %s\n", con->count, con->buff1);
}

void print_banner(void) {
    printf("\x1b[0;35m");
    printf("  [FLATLINE] ROM construct system.");
    printf("  [FLATLINE] Construct manager v2.0");
    printf("\x1b[0m");
}

struct construct *new_construct(const char *arg1) {
    if (construct_count > 7) {
        return NULL;
    }

    struct construct *con = (struct construct *)malloc(sizeof(struct construct)); // 0x70 bytes
    if (con == NULL) {
        return NULL;
    }

    con->count = construct_count;
    con->flag = 1;
    con->execute = (execute_fn_t)default_execute;

    strncpy(con->buff1, arg1, 0x1f);
    con->buff1[0x1f] = '\0';

    memset(con->buff2, 0, 0x40);

    constructs[construct_count] = con;
    construct_count++;

    printf("[FLATLINE] Construct %u created: %s\n", con->count, con->buff1);
    return con;
}

void delete_construct(unsigned int index) {
    if (index >= construct_count || constructs[index] == NULL) {
        return;
    }

    printf("[FLATLINE] Deleting construct %u\n", index);

    free(constructs[index]);
}

void update_construct(unsigned int index) {
    if (index >= construct_count || constructs[index] == NULL) {
        return;
    }

    printf("[FLATLINE] Update data for construct %u: ", index);
    fflush(stdout);

    read(STDIN_FILENO, constructs[index]->buff2, 0xa0);//0xa0:160

    printf("[FLATLINE] Construct %u updated.\n", index);
}

void run_construct(unsigned int index) {
    if (index >= construct_count || constructs[index] == NULL) {
        return;
    }
    execute_fn_t fn = constructs[index]->execute;
    fn((struct construct *)constructs[index]->buff2);
}

int main(void) {
    uid_t euid = geteuid();
    setreuid(euid, euid);

    print_banner();

    new_construct("DIXIE");
    new_construct("WINTERMUTE");

    delete_construct(0);
    update_construct(0);
    run_construct(0);
    return 0;
}