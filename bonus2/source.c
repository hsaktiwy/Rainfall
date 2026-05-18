#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int lang_mode = 0;

void greetuser(char *input) {
    char greeting[72];
    if (lang_mode == 1) {
        strcpy(greeting, "Hyvää päivää "); // 18 bytes
    } else if (lang_mode == 2) {
        strcpy(greeting, "Goedemiddag! "); // 13 bytes
    } else {
        strcpy(greeting, "Hello ");      // 6 bytes
    }
    strcat(greeting, input); 
    puts(greeting);
}

int main(int argc, char **argv) {
    char buffer[76]; // 19 * 4 bytes initialized to 0
    char *lang;

    if (argc != 3) {
        return 1;
    }
    memset(buffer, 0, 76);
    strncpy(buffer, argv[1], 40);
    strncpy(buffer + 40, argv[2], 32);
    lang = getenv("LANG");
    if (lang != NULL) {
        if (memcmp(lang, "fi", 2) == 0) {
            lang_mode = 1;
        } else if (memcmp(lang, "nl", 2) == 0) {
            lang_mode = 2;
        }
    }
    greetuser(buffer);
    return 0;
}