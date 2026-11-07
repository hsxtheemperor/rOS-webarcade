#include "bash.h"

static uint32_t text_length(const char *text) {
    uint32_t length = 0;
    while (text && text[length]) length++;
    return length;
}

static uint32_t parse_u32(const char *text) {
    uint32_t value = 0u;
    if (!text) return 0u;
    while (*text >= '0' && *text <= '9') {
        value = value * 10u + (uint32_t)(*text - '0');
        text++;
    }
    return value;
}

int bash_cmd_head(int argc, char **argv) {
    const char *path = 0;
    uint32_t limit = 10u;
    if (argc == 2) {
        path = argv[1];
    } else if (argc == 4 && argv[1][0] == '-' && argv[1][1] == 'n') {
        limit = parse_u32(argv[2]);
        path = argv[3];
    } else {
        const char *usage = "usage: head [-n N] <file>\n";
        bash_write(usage, text_length(usage));
        return 2;
    }

    static uint8_t buffer[BASH_FILE_CAPACITY];
    uint32_t size = bash_file_read(path, buffer, sizeof(buffer));
    if (size == BASH_FILE_NOT_FOUND) {
        const char *error = "head: file not found\n";
        bash_write(error, text_length(error));
        return 1;
    }
    if (size == BASH_FILE_TOO_LARGE) {
        const char *error = "head: file is too large\n";
        bash_write(error, text_length(error));
        return 1;
    }
    if (limit > size) limit = size;
    for (uint32_t i = 0; i < limit; i++) {
        char byte = (char)buffer[i];
        bash_write(&byte, 1u);
    }
    bash_write("\n", 1u);
    return 0;
}
