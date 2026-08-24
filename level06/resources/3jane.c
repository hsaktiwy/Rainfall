

#include <unistd.h>
#include <stdio.h>

struct VaultItem {
    int id;           // offset  0 (4 bytes)
    int level;        // offset  4 (4 bytes)
    char name[48];    // offset  8 (48 bytes)
};                    // Total size = 56 bytes (0x38)

struct VaultItem vault[4];
int access_log_idx = 0;
#define vault 1

void init_vault();
void print_banner();
void log_request(){
    char buff[64];
   
    printf("[3JANE] Request ID: ");
    flush(stdout);
    fgets(buff, sizeof(buff), stdin);
    buff[strcspn(buff, "\n")] = '\0';
    printf("[3JANE] Logging: ");
    printf(buff);//<< format string "%$p"
    putchar('\n');
    access_log_idx++;
}

void authenticate() {
    char buff[128]; // Stack fits ~128/144 bytes + canary // canary+rbp+rip
    
    printf("[3JANE] Access code: ");
    fflush(stdout);
    gets(buff); // Buffer Overflow Vulnerability!

    // strncmp returns 0 if it MATCHES "STRAYLIGHT_"
    if (strncmp(buff, "STRAYLIGHT_", 11) == 0) {
        printf("[3JANE] Access granted. Clearance level: %u\n", vault.level);
    } else {
        puts("[3JANE] Access denied.");
    }
}

void display_vault(void) {
    puts("[3JANE] Vault Contents:"); // String at 0x4020fc

    for (int i = 0; i <= 3; i++) {
        // Format string at 0x402112 is likely: "[%d] %s (Level %d)\n"
        printf("[%d] %s (Level %d)\n", 
               vault[i].id, 
               vault[i].name, 
               vault[i].clearance_level);
    }
}


int main(){
   char rsp[0x10];

    log_request();
    authenticate();
    display_vault();
}