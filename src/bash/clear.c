#include "bash.h"

int bash_cmd_clear(int argc, char **argv) {
    (void)argv;
    if (argc != 1) {
        const char *usage = "usage: clear\n";
        bash_write(usage, 13);
        return 2;
    }
    bash_clear();
    return 0;
}
