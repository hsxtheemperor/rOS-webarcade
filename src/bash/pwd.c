#include "bash.h"

int bash_cmd_pwd(int argc, char **argv) {
    (void)argv;
    if (argc != 1) {
        bash_write("usage: pwd\n", 11);
        return 2;
    }
    bash_write("/\n", 2);
    return 0;
}
