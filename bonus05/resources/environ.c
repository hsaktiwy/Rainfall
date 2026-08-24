#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct VaultEntry {
    int id;
    int pad;
    char key[32];
    char value[64];
};

extern char **environ;

struct VaultEntry vault[8];
int vault_count = 0;

void print_banner(void);
void store_var(const char *key, const char *val);
void init_vault(void);
void info_leak(void);
void lookup(const char *key);
void trace_query(void);
void handle_input(void);

void print_banner(void) {
    printf("\033[0;36m");
    puts("  [MATRIX] The environment holds secrets.");
    puts("  [ENVIRON] Variable vault v1.0");
    printf("\033[0m");
}

void store_var(const char *key, const char *val) {
    if (vault_count > 7) {
        return;
    }

    vault[vault_count].id = vault_count;
    vault[vault_count].pad = 0;

    strncpy(vault[vault_count].key, key, 31);
    vault[vault_count].key[31] = '\0';

    strncpy(vault[vault_count].value, val, 63);
    vault[vault_count].value[63] = '\0';

    vault_count++;
}

void init_vault(void) {
    char *val;

    val = getenv("PATH");
    if (val == NULL) {
        val = "/usr/bin";
    }
    store_var("PATH", val);

    val = getenv("HOME");
    if (val == NULL) {
        val = "/root";
    }
    store_var("HOME", val);

    val = getenv("TERM");
    if (val == NULL) {
        val = "xterm";
    }
    store_var("TERM", val);
}

void info_leak(void) {
    printf("[ENVIRON] environ@:   %p\n", (void *)environ);
    printf("[ENVIRON] vault@:     %p\n", (void *)vault);
}

void lookup(const char *key) {
    for (int i = 0; i < vault_count; i++) {
        if (strncmp(vault[i].key, key, 32) == 0) {
            printf("[ENVIRON] %s=%s\n", vault[i].key, vault[i].value);
            return;
        }
    }

    puts("[ENVIRON] Key not found.");
}

void trace_query(void) {
    char buf[64];
    printf("[ENVIRON] Trace tag: ");
    fflush(stdout);

    if (fgets(buf, 64, stdin) != NULL) {
        printf("[ENVIRON] Resolving ");

        printf(buf);
        fflush(stdout);
    }
}

void handle_input(void) {
    char input[80];

    info_leak();

    printf("[ENVIRON] Query: ");
    fflush(stdout);

    gets(input);

    if (strncmp(input, "GET:", 4) == 0) {
        lookup(input + 4);
    }
    else if (strncmp(input, "LIST", 4) == 0) {
        for (int i = 0; i < vault_count; i++) {
            printf("[ENVIRON] [%u] %s=%s\n", vault[i].id, vault[i].key, vault[i].value);
        }
    }
    else {
        puts("[ENVIRON] Unknown command.");
    }
}

int main(void) {
    init_vault();
    print_banner();
    trace_query();
    handle_input();
    return 0;
}