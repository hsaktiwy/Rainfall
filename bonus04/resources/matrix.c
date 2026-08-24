#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Entry {
    int id;
    unsigned int hash;
    char key[32];
    char value[32];
};

struct Entry table[16];
int entry_count = 0;

unsigned int djb2(const char *str);
void init_table(void);
void print_banner(void);
void lookup(const char *key);
void insert(const char *key, const char *val);
void handle_input(void);

unsigned int djb2(const char *str) {
    unsigned int hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash * 33) ^ c);
    }
    return hash;
}

void init_table(void) {
    memset(table, 0, sizeof(table));

    for (int i = 0; i < 4; i++) {
        table[i].id = i;
        snprintf(table[i].key, 32, "key-%04x", i * 0xdead);
        snprintf(table[i].value, 32, "val-%04x", i * 0xbeef);
        table[i].hash = djb2(table[i].key);
        entry_count++;
    }
}

void print_banner(void) {
    printf("\033[0;36m");
    puts("  [MATRIX] The cyberspace awaits.");
    puts("  [MATRIX] Key-value store v3.1");
    printf("\033[0m");
}

void lookup(const char *key) {
    unsigned int target_hash = djb2(key);

    for (int i = 0; i < entry_count; i++) {
        if (table[i].hash == target_hash) {
            if (strncmp(table[i].key, key, 32) == 0) {
                printf("[MATRIX] Found: %s => %s\n", table[i].key, table[i].value);
                return;
            }
        }
    }

    puts("[MATRIX] Key not found.");
}

void insert(const char *key, const char *val) {
    if (entry_count > 15) {
        return;
    }

    table[entry_count].id = entry_count;

    strncpy(table[entry_count].key, key, 31);
    table[entry_count].key[31] = '\0';

    strncpy(table[entry_count].value, val, 31);
    table[entry_count].value[31] = '\0';

    table[entry_count].hash = djb2(key);

    entry_count++;

    printf("[MATRIX] Inserted: %s\n", key);
}

void handle_input(void) {
    char input[112];
    char key_buf[32];
    char val_buf[32];
    char *colon;

    printf("[MATRIX] Command (GET/SET): ");
    fflush(stdout);

    gets(input);

    if (strncmp(input, "GET:", 4) == 0) {
        strncpy(key_buf, input + 4, 31);
        key_buf[31] = '\0';
        lookup(key_buf);
    }

    else if (strncmp(input, "SET:", 4) == 0) {
        colon = strchr(input + 4, ':');
        if (colon != NULL) {
            *colon = '\0';

            strncpy(key_buf, input + 4, 31);
            key_buf[31] = '\0';

            strncpy(val_buf, colon + 1, 31);
            val_buf[31] = '\0';

            insert(key_buf, val_buf);
        }
    }
    else {
        puts("[MATRIX] Unknown command.");
    }
}

int main(void) {
    init_table();
    print_banner();
    handle_input();
    return 0;
}