#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct Route {
    int id;
    int metric;
    char src[16];
    char dst[16];
};

// Global Variables
struct Route routing_table[8]; // 0x4040a0 (8 * 40 = 320 bytes / 0x140)
int route_count = 0;           // 0x4041e0
char relay_buf[512];           // 0x404200 (0x200 bytes)
int relay_len;                 // 0x404400

void init_routing(void) {
    memset(routing_table, 0, sizeof(routing_table)); // 0x140
    memset(relay_buf, 0, sizeof(relay_buf));         // 0x200

    // Set up initial route 0
    strncpy(routing_table[0].src, "...", 15); // String at 0x402008
    strncpy(routing_table[0].dst, "...", 15); // String at 0x40200d
    routing_table[0].metric = 1;              // 0x4040a4
    route_count = 1;                         // 0x4041e0
}

void add_route(const char *src, const char *dst, int metric) {
    if (route_count > 7) {
        return; // Routing table full
    }

    // Populate route_table[route_count]
    strncpy(routing_table[route_count].src, src, 15);
    strncpy(routing_table[route_count].dst, dst, 15);
    routing_table[route_count].metric = metric;
    routing_table[route_count].id = route_count; // Sets ID to current index

    route_count++;
}

void relay_status(void) {
    char buf[64]; // -0x50(%rbp) with 0x3f max read
    ssize_t bytes_read;

    printf("[MAELCUM] Tag this relay (who's asking?): "); // 0x402068 ("Enter status: " or similar)
    fflush(stdout);

    bytes_read = read(0, buf, 63);
    if (bytes_read < 0) {
        bytes_read = 0;
    }
    
    buf[bytes_read] = '\0';
    
    // Strip trailing newline/delimiters via strcspn
    buf[strcspn(buf, "\n")] = '\0'; // String at 0x402093

    printf("[MAELCUM] Logging relay tag: "); // 0x402095
    printf(buf);
    putchar('\n');
    fflush(stdout);
}

void relay_data(void) {
    char cmd_buf[48]; // -0x30(%rbp)
    unsigned int len;

    printf("[MAELCUM] Send it: "); // 0x4020b3
    fflush(stdout);

    read(0, cmd_buf, 64); // Note: Reading 64 bytes into 48-byte buffer!

    // Check if input starts with a specific prefix (e.g. "RELAY " or "DATA: ")
    if (strncmp(cmd_buf, "RELAY:", 6) == 0) { // String at 0x4020c7
        // Parse length from offset + 6
        len = (unsigned int)strtoul(cmd_buf + 6, NULL, 10);

        if (len > 0x1ff) { // Length > 511 check
            return;
        }

        printf("[MAELCUM] Ready for %u bytes: ", len); // 0x4020d0
        fflush(stdout);

        // Read up to 'len' bytes into global relay_buf
        relay_len = read(0, relay_buf, len);

        printf("[MAELCUM] Relayed %u bytes.\n", relay_len); // 0x4020ef
    } else {
        puts(""); //(Error message)
    }
}

void print_routes(void) {
    for (int i = 0; i < route_count; i++) {
        // Formats and prints routing_table[i].id, .metric, .src, .dst
        printf("[MAELCUM] Route %u: %s via %s (%u hops)\n", 
               routing_table[i].id, 
               routing_table[i].metric, 
               routing_table[i].src, 
               routing_table[i].dst); // String at 0x402128
    }
}

int main(void) {
    init_routing();
    print_banner(); // 0x40132a

    // Hardcoded routes added at startup
    add_route("FREESIDE", "GATEWAY_3", 3);
    add_route("VILLA_STRAYLIGHT", "TESSIER_GW", 5);

    relay_status();
    relay_data();
    print_routes();

    return 0;
}