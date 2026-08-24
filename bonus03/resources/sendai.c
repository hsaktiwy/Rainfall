#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef void (*disconnect_fn)(int);

struct Connection {
    int id;
    int port;
    int connected;
    char hostname[32];
    int pad;
    disconnect_fn disconnect;
};


struct Connection conns[8];

void disconnect_default(int id);
void init_connections(void);
void print_banner(void);
void info_leak(void);
void handle_command(void);


void disconnect_default(int id) {
    printf("[SENDAI] Connection %u closed.\n", id);
}

void init_connections(void) {
    for (int i = 0; i <= 7; i++) {
        conns[i].id = i;
        conns[i].connected = 0;
        conns[i].disconnect = disconnect_default;

        snprintf(conns[i].hostname, 32, "node-%04x.sprawl.net", i * 0x1337);

        conns[i].port = 0x400 + (i * 13);
    }
}

void print_banner(void) {
    printf("\033[0;34m");
    puts("  [SENDAI] Connection broker online.");
    puts("  [SENDAI] Ono-Sendai routing layer v5.0");
    printf("\033[0m");
}

void info_leak(void) {
    printf("[SENDAI] disconnect@binary: %p\n", (void *)disconnect_default);
}

void handle_command(void) {
    char input[64];
    char buffer[64];
    unsigned int conn_idx;

    while (1) {
        info_leak();

        printf("[SENDAI] Command: ");
        fflush(stdout);

        if (fgets(input, 64, stdin) == NULL) {
            break;
        }

        input[strcspn(input, "\n")] = '\0';

        if (strncmp(input, "CONNECT:", 8) == 0) {
            conn_idx = strtoul(input + 8, NULL, 10);
            if (conn_idx > 7) {
                continue;
            }

            conns[conn_idx].connected = 1;

            printf("[SENDAI] Connected to %s:%u\n", conns[conn_idx].hostname, conns[conn_idx].port);
        }

        else if (strncmp(input, "SEND:", 5) == 0) {
            conn_idx = strtoul(input + 5, NULL, 10);
            if (conn_idx > 7 || conns[conn_idx].connected == 0) {
                continue;
            }

            printf("[SENDAI] Data for conn %u: ", conn_idx);
            fflush(stdout);

            gets(buffer);

            printf("[SENDAI] Sent: ");
            printf(buffer);
            putchar('\n');
        }
        else if (strncmp(input, "DISCONNECT:", 11) == 0) {
            conn_idx = strtoul(input + 11, NULL, 10);
            if (conn_idx > 7) {
                continue;
            }

            if (conns[conn_idx].disconnect != NULL) {
                conns[conn_idx].disconnect(conn_idx);
            }
        }
        else if (strncmp(input, "QUIT", 4) == 0) {
            break;
        }
        else {
            puts("[SENDAI] Unknown command.");
        }
    }
}

int main(void) {
    init_connections();
    print_banner();
    handle_command();
    return 0;
}