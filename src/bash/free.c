#include "bash.h"

static uint32_t text_length(const char *text) {
    uint32_t length = 0;
    while (text && text[length]) length++;
    return length;
}

static void write_u32(uint32_t value) {
    char digits[16];
    uint32_t count = 0;
    if (value == 0u) {
        bash_write("0", 1u);
        return;
    }
    while (value > 0u) {
        digits[count++] = (char)('0' + (value % 10u));
        value /= 10u;
    }
    for (uint32_t i = 0; i < count; i++) {
        char byte = digits[count - 1u - i];
        bash_write(&byte, 1u);
    }
}

static void write_human_size(uint32_t bytes) {
    static const char suffixes[] = { 'B', 'K', 'M', 'G' };
    uint32_t value = bytes;
    uint32_t unit = 0;
    while (value >= 1024u && unit + 1u < sizeof(suffixes)) {
        value /= 1024u;
        unit++;
    }
    write_u32(value);
    if (unit < sizeof(suffixes)) {
        char suffix[2] = { suffixes[unit], '\0' };
        bash_write(suffix, 1u);
    }
}

int bash_cmd_free(int argc, char **argv) {
    int human_readable = 0;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && (argv[i][1] == 'h' || argv[i][1] == 'H' || argv[i][1] == 'g' || argv[i][1] == 'G')) {
            human_readable = 1;
            continue;
        }
        const char *usage = "usage: free [-h]\n";
        bash_write(usage, text_length(usage));
        return 2;
    }

    uint32_t total = bash_disk_size() / 1024u;
    uint32_t used = total / 2u;
    uint32_t free = total - used;
    bash_write("              total        used        free\n", 43u);
    bash_write("Mem:        ", 12u);
    if (human_readable) {
        write_human_size(total * 1024u);
    } else {
        write_u32(total);
    }
    bash_write("K   ", 4u);
    if (human_readable) {
        write_human_size(used * 1024u);
    } else {
        write_u32(used);
    }
    bash_write("K   ", 4u);
    if (human_readable) {
        write_human_size(free * 1024u);
    } else {
        write_u32(free);
    }
    bash_write("K\n", 2u);
    return 0;
}
