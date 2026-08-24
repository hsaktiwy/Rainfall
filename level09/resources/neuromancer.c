#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Node {
    int id;
    int type;
    int status;
    char name[20];
    void (*handler)(void);
};

struct Node nodes[12];
int node_count = 0;

void default_handler(void) {
    puts("[NEUROMANCER] Default handler invoked.");
}

void show_node(int id) {
    if ((unsigned int)id < (unsigned int)node_count) {
        printf(
            "[NEUROMANCER] Node %u: %s type=%u flags=%08x\n", 
            nodes[id].id, 
            nodes[id].name, 
            nodes[id].type, 
            nodes[id].status
        );
    }
}


void init_nodes(void) {
    for (int i = 0; i <= 11; i++) {
        nodes[i].id = i;
        nodes[i].type = i % 3;
        nodes[i].status = 0;

        snprintf(nodes[i].name, 0x10, "NODE_%04x", i * 0x1a2b);

        nodes[i].handler = default_handler;
    }

    node_count = 12;
}

void print_banner(void) {
    printf("\033[0;35m");
    puts("  [NEUROMANCER] I am the land of the dead.");
    puts("  [NEUROMANCER] You should not be here.");
    printf("\033[0m");
}

void info_leak(void) {
    void *leaked_ptr = *(void **)0x403fd8; 
    printf("[NEUROMANCER] puts@libc: %p\n", leaked_ptr);
}

void console_handshake(void) {
    char buf[64]; // Allocated at -0x50(%rbp)

    printf("[NEUROMANCER] Console handle: ");
    fflush(stdout);

    if (fgets(buf, sizeof(buf), stdin) != NULL) {
        printf("[NEUROMANCER] Trace echo: ");
        printf(buf);
    }
}

void process_input(void) {
    char buf[128]; // Allocated at -0x90(%rbp)

    printf("[NEUROMANCER] Interface: ");
    fflush(stdout);

    /* 
     * VULNERABILITY: Unbounded Buffer Overflow
     * gets() reads infinite bytes into 128-byte stack buffer
     */
    gets(buf);

    if (strncmp(buf, "NODE:", 5) == 0) {
        int idx = (int)strtoul(buf + 5, NULL, 10);
        show_node(idx);
    }
    else if (strncmp(buf, "LIST", 4) == 0) {
        for (int i = 0; i < node_count; i++) {
            show_node(i);
        }
    } 
    else {
        puts( "[NEUROMANCER] Unknown command.");
    }
}

void handle_command(void) {
    info_leak();
    console_handshake();
    process_input();
}

int main(void) {
    init_nodes();
    print_banner();
    handle_command();
    return 0;
}