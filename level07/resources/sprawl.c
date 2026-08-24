
#include <stdio.h>

typedef struct {
    unsigned int magic;     // Offset 0 (4 bytes)
    unsigned short count;   // Offset 4 (2 bytes)
    unsigned short size;    // Offset 6 (2 bytes)
} PacketHeader;             // Total: 8 bytes

typedef struct {
    PacketHeader header;
    unsigned int checksum;
    void *data;
} Packet;

extern int packet_count; // Global at 0x404380
extern Packet packets[32]; // Global array at 0x404080

int read_header(PacketHeader *hdr) {
    printf("[SPRAWL] Header (hex): ");
    fflush(stdout);

    // scanf format string at 0x4020aa is likely "%x %hu %hu"
    int scanned = __isoc99_scanf("%x %hx %hx", &hdr->magic, &hdr->count, &hdr->size);

    if (scanned != 3) {
        return -1;
    }

    if (hdr->magic != 0xDEADBEEF) {
        return -1;
    }

    return 0;
}

void process_packet(void) {
    PacketHeader hdr;//8
    char payload[64];      // Stack buffer at -0x50(%rbp)
    unsigned short total_size; // Stack variable at -0x6a(%rbp)
    unsigned int checksum;

    if (read_header(&hdr) < 0) {
        puts("[SPRAWL] Invalid header.");
        return;
    }

    // count (16-bit) * size (16-bit) truncated to 16-bit unsigned short!
    total_size = (unsigned short)(hdr.count * hdr.size);

    if (total_size > 64) { // 0x40
        puts("[SPRAWL] Frame too large for relay buffer.");
        return;
    }

    printf("[SPRAWL] Transmit (%u bytes): ", total_size);
    fflush(stdout);
    getchar(); // Consumes remaining newline from scanf

    // Reads `hdr.count` elements of size 1 byte into local `payload` buffer
    // --- VULNERABILITY 2: Stack Buffer Overflow ---
    // If hdr.count is large (e.g., 0x8000) and hdr.size is 2,
    // total_size wraps to 0 (0 <= 64), but fread reads 0x8000 bytes!
    // CONDITION TO REACH THIS FUNCTION ARE (unsigned short)(hdr.count * hdr.size) <= 64, hdr->magic == 0xDEADBEEF
    fread(payload, 1, (size_t)hdr.count, stdin);

    // Calc checksum up to total_size (capped at 64 bytes)
    unsigned int calc_len = (total_size > 64) ? 64 : total_size;
    checksum = calc_checksum(payload, calc_len);// check sum is not of interest since it doesn't get out of the program

    if (packet_count <= 31) { // 0x1f
        packets[packet_count].header = hdr;
        packets[packet_count].checksum = checksum;
        packets[packet_count].data = NULL; // 0x404090 offset
        packet_count++;
    }

    printf("[SPRAWL] Packet %u received. Checksum: %08x\n", packet_count - 1, checksum);
}


void route_tag(){

    char buff[128]; //rsp 0x90
    printf("[SPRAWL] Route tag: ");
    fflush(stdout);
    fgets(stdin, sizeof(buff), 128);
    printf("[SPRAWL] Routing via ");
    printf(buff);
    fflush(stdout);
}


int main(){
    route_tag();
    process_packet();
    // dump_packets();
}