#include "bash.h"

static uint32_t text_length(const char *text) {
    uint32_t length = 0;
    while (text && text[length]) length++;
    return length;
}

int bash_cmd_cp(int argc, char **argv) {
    if (argc != 3) {
        const char *usage = "usage: cp <src> <dst>\n";
        bash_write(usage, text_length(usage));
        return 2;
    }

    static uint8_t buffer[BASH_FILE_CAPACITY];
    uint32_t size = bash_file_read(argv[1], buffer, sizeof(buffer));
    if (size == BASH_FILE_NOT_FOUND) {
        const char *error = "cp: source not found\n";
        bash_write(error, text_length(error));
        return 1;
    }
    if (size == BASH_FILE_TOO_LARGE) {
        const char *error = "cp: source is too large\n";
        bash_write(error, text_length(error));
        return 1;
    }
    if (bash_file_write(argv[2], buffer, size) != 0u) {
        const char *error = "cp: unable to write destination\n";
        bash_write(error, text_length(error));
        return 1;
    }
    return 0;
}
