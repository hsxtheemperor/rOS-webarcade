#include "bash.h"

static uint32_t text_length(const char *text) {
    uint32_t length = 0;
    while (text && text[length]) length++;
    return length;
}

int bash_cmd_cat(int argc, char **argv) {
    static uint8_t contents[BASH_FILE_CAPACITY];
    if (argc != 2) {
        const char *usage = "usage: cat <file>\n";
        bash_write(usage, text_length(usage));
        return 2;
    }

    const char *path = argv[1][0] == '/' ? argv[1] + 1 : argv[1];
    uint32_t size = bash_file_read(path, contents, sizeof(contents));
    if (size == BASH_FILE_NOT_FOUND) {
        const char *error = "cat: file not found\n";
        bash_write(error, text_length(error));
        return 1;
    }
    if (size == BASH_FILE_TOO_LARGE) {
        const char *error = "cat: file is too large\n";
        bash_write(error, text_length(error));
        return 1;
    }

    if (size) bash_write((const char *)contents, size);
    if (!size || contents[size - 1] != '\n') bash_write("\n", 1);
    return 0;
}
