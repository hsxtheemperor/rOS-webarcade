#include "bash.h"

static uint32_t text_length(const char *text) {
    uint32_t length = 0;
    while (text && text[length]) length++;
    return length;
}

int bash_cmd_history(int argc, char **argv) {
    (void)argv;
    if (argc != 1) {
        const char *usage = "usage: history\n";
        bash_write(usage, text_length(usage));
        return 2;
    }
    bash_write("1  ls\n2  help\n3  pwd\n", 22u);
    return 0;
}
