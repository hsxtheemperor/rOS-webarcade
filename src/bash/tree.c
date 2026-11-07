#include "bash.h"

int bash_cmd_tree(int argc, char **argv) {
    (void)argv;
    if (argc != 1) {
        const char *usage = "usage: tree\n";
        bash_write(usage, 12);
        return 2;
    }

    bash_write(".\n|- vdisk01/\n", 14);
    uint32_t count = bash_file_count();
    char path[512];
    for (uint32_t i = 0; i < count; i++) {
        if (!bash_file_path(i, path, sizeof(path))) continue;
        bash_write("`- ", 3);
        uint32_t length = 0;
        while (path[length]) length++;
        bash_write(path, length);
        bash_write("\n", 1);
    }
    return 0;
}
