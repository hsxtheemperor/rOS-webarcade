#include "bash.h"

static uint32_t text_length(const char *text) {
    uint32_t length = 0;
    while (text && text[length]) length++;
    return length;
}

int bash_cmd_uname(int argc, char **argv) {
    (void)argv;
    if (argc != 1) {
        const char *usage = "usage: uname\n";
        bash_write(usage, text_length(usage));
        return 2;
    }
    bash_write("rOS wasm32\n", 10u);
    return 0;
}
