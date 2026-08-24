#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct Block;

typedef void (*on_free_fn)(struct Block *);

struct Block {
    int id;
    int size;
    char name[16];
    on_free_fn on_free;
};

struct Block *blocks[8];
int block_count = 0;

void default_on_free(struct Block *b);
void print_banner(void);
void alloc_block(int size, const char *name);
void list_blocks(void);
void free_block(unsigned int index);

void default_on_free(struct Block *b) {
    (void)b;
}

void print_banner(void) {
    printf("\033[0;32m");
    puts("  [ZION] The station endures.");
    puts("  [ZION] Memory allocator v1.1");
    printf("\033[0m");
}

void alloc_block(int size, const char *name) {
    if (block_count > 7) {
        return;
    }

    struct Block *block = (struct Block *)malloc(sizeof(struct Block));// 0x20: 32 byte

    void *buf = malloc(size);// 0x40: 64byte

    if (block == NULL || buf == NULL) {
        free(block);
        free(buf);
        return;
    }

    block->id = block_count;
    block->size = size;
    block->on_free = default_on_free;
    strncpy(block->name, name, 15);

    blocks[block_count] = block;
    block_count++;

    printf("[ZION] Block %u allocated (%u bytes) label=%s\n", block->id, size, block->name);
    
    printf("[ZION] Data: ");
    fflush(stdout);

    read(STDIN_FILENO, buf, size + 0x40);

    free(buf);
}

void list_blocks(void) {
    for (int i = 0; i < block_count; i++) {
        if (blocks[i] != NULL) {
            printf("[ZION] Block %u: label=%s size=%u\n", blocks[i]->id, blocks[i]->name, blocks[i]->size);
        }
    }
}

void free_block(unsigned int index) {
    if (index >= block_count) {
        return;
    }

    if (blocks[index] == NULL) {
        return;
    }

    printf("[ZION] Freeing block %u\n", index);

    on_free_fn cleanup = blocks[index]->on_free;

    if (cleanup != NULL) {
        cleanup(blocks[index]);
    }

    free(blocks[index]);

    blocks[index] = NULL;
}

int main(void) {
    uid_t euid = geteuid();
    setreuid(euid, euid);

    print_banner();

    alloc_block(0x40, "ALPHA");
    alloc_block(0x40, "BETA");

    list_blocks();

    free_block(0);
    free_block(1);

    return 0;
}