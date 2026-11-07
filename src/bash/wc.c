#include "bash.h"

static uint32_t text_length(const char *text) {
    uint32_t length = 0;
    while (text && text[length]) length++;
    return length;
}

static void write_u32(uint32_t value) {
    char digits[16];
    uint32_t count = 0;
    if (value == 0u) {
        bash_write("0", 1u);
        return;
    }
    while (value > 0u) {
        digits[count++] = (char)('0' + (value % 10u));
        value /= 10u;
    }
    for (uint32_t i = 0; i < count; i++) {
        char byte = digits[count - 1u - i];
        bash_write(&byte, 1u);
    }
}

int bash_cmd_wc(int argc, char **argv) {
    if (argc != 2) {
        const char *usage = "usage: wc <file>\n";
        bash_write(usage, text_length(usage));
        return 2;
    }
    static uint8_t buffer[BASH_FILE_CAPACITY];
    uint32_t size = bash_file_read(argv[1], buffer, sizeof(buffer));
    if (size == BASH_FILE_NOT_FOUND) {
        const char *error = "wc: file not found\n";
        bash_write(error, text_length(error));
        return 1;
    }
    if (size == BASH_FILE_TOO_LARGE) {
        const char *error = "wc: file is too large\n";
        bash_write(error, text_length(error));
        return 1;
    }

    uint32_t lines = 0u, words = 0u, chars = 0u;
    int in_word = 0;
    for (uint32_t i = 0; i < size; i++) {
        char value = (char)buffer[i];
        chars++;
        if (value == '\n') lines++;
        if (value == ' ' || value == '\t' || value == '\n' || value == '\r') {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            words++;
        }
    }
    write_u32(lines + 1u);
    bash_write(" ", 1u);
    write_u32(words);
    bash_write(" ", 1u);
    write_u32(chars);
    bash_write(" ", 1u);
    bash_write(argv[1], text_length(argv[1]));
    bash_write("\n", 1u);
    return 0;
}
