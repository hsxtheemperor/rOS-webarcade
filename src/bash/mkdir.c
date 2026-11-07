#include "bash.h"

static uint32_t text_length(const char *text) {
    uint32_t length = 0;
    while (text && text[length]) length++;
    return length;
}

static const char *normalize_path(const char *path) {
    return path && path[0] == '/' ? path + 1 : path;
}

int bash_cmd_mkdir(int argc, char **argv) {
    if (argc != 2) {
        const char *usage = "usage: mkdir <dir>\n";
        bash_write(usage, text_length(usage));
        return 2;
    }
    const char *path = normalize_path(argv[1]);
    if (bash_dir_make(path) != 0u) {
        const char *error = "mkdir: unable to create directory\n";
        bash_write(error, text_length(error));
        return 1;
    }
    return 0;
}
