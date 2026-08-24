#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Vault {
    int magic;
    int max_size;
    int max_sessions;
    char name[16];
    char flag[256];
};

struct Session {
    int id;
    int authenticated;
    int active;
    char tag[32];
};

struct Vault vault;
struct Session sessions[4];
int session_count = 0;

void init_vault(void);
void init_sessions(void);
void print_banner(void);
void format_log(void);
int create_session(const char *tag);
void authenticate(void);


void init_vault(void) {
    vault.magic = 0xcafebabe;
    vault.max_size = 0x200;
    vault.max_sessions = 3;

    strncpy(vault.name, "T-A_VAULT", 0x0f);

    memset(vault.flag, 0, 0x100);
    strncpy(vault.flag, "TESSIER-ASHPOOL SA — Final authentication required.", 0xff);
}

void init_sessions(void) {
    memset(sessions, 0, sizeof(sessions));
}

void print_banner(void) {
    printf("\033[0;31m");
    puts("  [TESSIER-ASHPOOL SA] This system is beyond you.");
    puts("  [T-A] Final authentication required.");
    printf("\033[0m");
}

void format_log(void) {
    char buf[32];

    printf( "[T-A] Session tag: ");
    fflush(stdout);

    if (fgets(buf, 0x20, stdin) != NULL) {
        size_t len = strcspn(buf, "\n");
        buf[len] = '\0';

        printf("[T-A] Tag accepted: ");

        printf(buf);
        putchar('\n');
    }
}

int create_session(const char *tag) {
    if (session_count > 3) {
        return -1;
    }

    int idx = session_count;

    sessions[idx].id = idx;
    sessions[idx].authenticated = 0;
    sessions[idx].active = 1;

    strncpy(sessions[idx].tag, tag, 0x1f);

    session_count++;
    return idx;
}

void authenticate(void) {
    char input_buf[64];
    int session_id;

    printf("[T-A] Authentication token: ");
    fflush(stdout);


    gets(input_buf);

    session_id = create_session(input_buf);

    if (session_id < 0) {
        puts("[T-A] Session limit reached.");
        return;
    }

    if (strncmp(input_buf, "TA_ROOT_", 8) == 0) {
        sessions[session_id].authenticated = 0xff;

        printf("[T-A] Root access granted. Session %d.\n", session_id);
        printf("[T-A] Vault: %s\n", vault.flag);
    } else {
        printf("[T-A] Access denied. Session %d terminated.\n", session_id);
        sessions[session_id].active = 0;
    }
}

int main(void) {
    init_vault();
    init_sessions();
    print_banner();
    format_log();
    authenticate();
    return 0;
}