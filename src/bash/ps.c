#include "bash.h"

static uint32_t text_length(const char *text) {
    uint32_t length = 0;
    while (text && text[length]) length++;
    return length;
}

int bash_cmd_ps(int argc, char **argv) {
    (void)argv;
    if (argc != 1) {
        const char *usage = "usage: ps\n";
        bash_write(usage, text_length(usage));
        return 2;
    }
    bash_write("PID TTY  TIME CMD\n", 17u);
    bash_write("1   ?    0:00 proc0\n", 20u);
    return 0;
}
