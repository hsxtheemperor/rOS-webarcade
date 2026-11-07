#include "bash.h"

int bash_cmd_echo(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (i > 1) bash_write(" ", 1);
        uint32_t length = 0;
        while (argv[i][length]) length++;
        bash_write(argv[i], length);
    }
    bash_write("\n", 1);
    return 0;
}
