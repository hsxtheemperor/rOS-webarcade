#include "bash.h"

static uint32_t text_length(const char *text) {
    uint32_t length = 0;
    while (text && text[length]) length++;
    return length;
}

int bash_cmd_whoami(int argc, char **argv) {
    (void)argv;
    if (argc != 1) {
        const char *usage = "usage: whoami\n";
        bash_write(usage, text_length(usage));
        return 2;
    }
    bash_write("root\n", 5u);
    return 0;
}
