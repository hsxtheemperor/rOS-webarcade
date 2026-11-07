#include "bash.h"

static uint32_t text_length(const char *text) {
    uint32_t length = 0;
    while (text && text[length]) length++;
    return length;
}

static uint32_t text_equal(const char *left, const char *right) {
    while (*left && *right && *left == *right) {
        left++;
        right++;
    }
    return *left == '\0' && *right == '\0';
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

static int is_flag(const char *arg) {
    return arg != 0 && arg[0] == '-' && arg[1] != '\0';
}

static int has_flag(int argc, char **argv, const char *name) {
    for (int i = 1; i < argc; i++) {
        if (!is_flag(argv[i])) continue;
        if (argv[i][1] == name[0] && name[1] == '\0') return 1;
        if (argv[i][1] == '-' && argv[i][2] == name[0] && name[1] == '\0') return 1;
        if (argv[i][1] == name[0] && argv[i][2] == '\0') return 1;
    }
    return 0;
}

static int is_vdisk_name(const char *name) {
    const char *cursor = name;
    if (cursor == 0 || !cursor[0]) return 0;
    if (cursor[0] != 'v' || cursor[1] != 'd' || cursor[2] != 'i' || cursor[3] != 's' || cursor[4] != 'k') return 0;
    cursor += 5;
    if (*cursor == '\0') return 0;
    while (*cursor) {
        if (*cursor < '0' || *cursor > '9') return 0;
        cursor++;
    }
    return 1;
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

static void write_disk_row(const char *name, uint32_t size, int show_fstype, int human_readable) {
    bash_write(name, text_length(name));
    bash_write("  ", 2u);
    if (show_fstype) {
        bash_write("disk  webfs1  ", 14u);
    } else {
        bash_write("disk  ", 7u);
    }
    if (human_readable) {
        write_human_size(size);
    } else {
        write_u32(size);
    }
    if (show_fstype) {
        bash_write("  1\n", 4u);
    } else {
        bash_write(" bytes  1\n", 10u);
    }
}

int bash_cmd_lsblk(int argc, char **argv) {
    int show_fstype = 0;
    int human_readable = 0;
    int has_path = 0;
    const char *target = 0;

    for (int i = 1; i < argc; i++) {
        if (!is_flag(argv[i])) {
            if (!has_path) {
                target = argv[i];
                has_path = 1;
            } else {
                const char *usage = "usage: lsblk [-f] [-h]\n";
                bash_write(usage, text_length(usage));
                return 2;
            }
            continue;
        }

        if (argv[i][1] == 'f' && argv[i][2] == '\0') {
            show_fstype = 1;
        } else if ((argv[i][1] == 'h' || (argv[i][1] == '-' && argv[i][2] == 'h')) && argv[i][3] == '\0') {
            human_readable = 1;
        } else if (argv[i][1] == 'g' && argv[i][2] == '\0') {
            human_readable = 1;
        } else {
            const char *usage = "usage: lsblk [-f] [-h]\n";
            bash_write(usage, text_length(usage));
            return 2;
        }
    }

    if (target != 0 && target[0] != '\0') {
        if (!is_vdisk_name(target)) {
            const char *usage = "lsblk: invalid target\n";
            bash_write(usage, text_length(usage));
            return 2;
        }
    }

    uint32_t size = bash_disk_size();
    char names[16][32];
    uint32_t count = 0;

    for (uint32_t i = 0; i < bash_file_count(); i++) {
        char path[512];
        if (!bash_file_path(i, path, sizeof(path))) continue;
        const char *start = path;
        while (*start == '/') start++;
        const char *slash = start;
        while (*slash && *slash != '/') slash++;
        if (slash == start) continue;
        char root[32];
        uint32_t root_length = (uint32_t)(slash - start);
        if (root_length >= sizeof(root)) root_length = sizeof(root) - 1u;
        for (uint32_t j = 0; j < root_length; j++) root[j] = start[j];
        root[root_length] = '\0';
        if (!is_vdisk_name(root)) continue;
        int seen = 0;
        for (uint32_t j = 0; j < count; j++) {
            if (text_equal(names[j], root)) {
                seen = 1;
                break;
            }
        }
        if (!seen && count < sizeof(names) / sizeof(names[0])) {
            for (uint32_t j = 0; j < root_length; j++) names[count][j] = root[j];
            names[count][root_length] = '\0';
            count++;
        }
    }

    if (count == 0u) {
        static const char fallback[] = "NAME TYPE SIZE RO\nvdisk01 disk 0 bytes 1\n";
        bash_write(fallback, sizeof(fallback) - 1u);
        return 0;
    }

    if (show_fstype) {
        static const char header[] = "NAME  TYPE  FSTYPE  SIZE  RO\n";
        bash_write(header, sizeof(header) - 1u);
    } else {
        static const char header[] = "NAME  TYPE  SIZE  RO\n";
        bash_write(header, sizeof(header) - 1u);
    }

    for (uint32_t i = 0; i < count; i++) {
        if (target != 0 && target[0] != '\0' && !text_equal(target, names[i])) continue;
        write_disk_row(names[i], size, show_fstype, human_readable);
    }

    return 0;
}
