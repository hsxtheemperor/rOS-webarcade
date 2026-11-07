#include "bash.h"

static uint32_t text_length(const char *text) {
    uint32_t length = 0;
    while (text && text[length]) length++;
    return length;
}

int bash_cmd_touch(int argc, char **argv) {
    static const uint8_t empty[1] = { 0 };
    if (argc != 2) {
        const char *usage = "usage: touch <file>\n";
        bash_write(usage, text_length(usage));
        return 2;
    }
    if (bash_file_write(argv[1], empty, 0u) != 0u) {
        const char *error = "touch: unable to create file\n";
        bash_write(error, text_length(error));
        return 1;
    }
    return 0;
}
