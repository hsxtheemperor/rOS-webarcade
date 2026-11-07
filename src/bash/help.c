#include "bash.h"

int bash_cmd_help(int argc, char **argv) {
    (void)argv;
    if (argc != 1) {
        const char *usage = "usage: help\n";
        bash_write(usage, 12);
        return 2;
    }
    const char *commands = "Commands: cat, clear, cp, date, df [-h], echo, free [-h], grep, head, help, history, ls [-a] [-l] [-h], lsblk [-f] [-h], mkdir, mv, ps, pwd, rm, rmdir, tail, touch, tree, uname, wc, whoami\n";
    uint32_t length = 0;
    while (commands[length]) length++;
    bash_write(commands, length);
    return 0;
}
