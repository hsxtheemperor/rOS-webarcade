#include "bash.h"

static uint32_t text_length(const char *text) {
    uint32_t length = 0;
    while (text && text[length]) length++;
    return length;
}

int bash_cmd_rm(int argc, char **argv) {
    if (argc != 2) {
        const char *usage = "usage: rm <path>\n";
        bash_write(usage, text_length(usage));
        return 2;
    }
    if (bash_file_remove(argv[1]) != 0u) {
        const char *error = "rm: unable to remove path\n";
        bash_write(error, text_length(error));
        return 1;
    }
    return 0;
}
