#include "bash.h"

static uint32_t text_length(const char *text) {
    uint32_t length = 0;
    while (text && text[length]) length++;
    return length;
}

int bash_cmd_grep(int argc, char **argv) {
    if (argc != 3) {
        const char *usage = "usage: grep <pattern> <file>\n";
        bash_write(usage, text_length(usage));
        return 2;
    }

    static uint8_t buffer[BASH_FILE_CAPACITY];
    uint32_t size = bash_file_read(argv[2], buffer, sizeof(buffer));
    if (size == BASH_FILE_NOT_FOUND) {
        const char *error = "grep: file not found\n";
        bash_write(error, text_length(error));
        return 1;
    }
    if (size == BASH_FILE_TOO_LARGE) {
        const char *error = "grep: file is too large\n";
        bash_write(error, text_length(error));
        return 1;
    }

    const char *needle = argv[1];
    uint32_t needle_length = 0;
    while (needle[needle_length]) needle_length++;

    for (uint32_t i = 0; i + needle_length <= size; i++) {
        uint32_t match = 1u;
        for (uint32_t j = 0; j < needle_length; j++) {
            if ((char)buffer[i + j] != needle[j]) {
                match = 0u;
                break;
            }
        }
        if (match) {
            bash_write("pattern found\n", 14u);
            return 0;
        }
    }
    bash_write("no matches\n", 11u);
    return 1;
}
