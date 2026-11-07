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

static int has_prefix(const char *value, const char *prefix) {
    uint32_t i = 0;
    while (prefix[i] && value[i] == prefix[i]) i++;
    return prefix[i] == '\0';
}

static void write_display_name(const char *path, const char *target, char *buffer, uint32_t capacity) {
    const char *cursor = path;
    const char *prefix = target;
    uint32_t prefix_length = 0;
    while (prefix[prefix_length] == '/') prefix_length++;
    while (target[prefix_length] && target[prefix_length] != '/') prefix_length++;
    if (target[0] != '\0') {
        if (has_prefix(path, target) && (path[prefix_length] == '\0' || path[prefix_length] == '/')) {
            cursor = path + prefix_length;
            if (*cursor == '/') cursor++;
        }
    }
    uint32_t length = 0;
    while (cursor[length] && cursor[length] != '/' && length + 1 < capacity) {
        buffer[length] = cursor[length];
        length++;
    }
    buffer[length] = '\0';
}

static int path_matches_target(const char *path, const char *target, char *display, uint32_t capacity) {
    const char *normalized = target;
    while (*normalized == '/') normalized++;
    if (*normalized == '\0') {
        uint32_t length = 0;
        while (path[length] && path[length] != '/') length++;
        if (length == 0) return 0;
        if (length >= capacity) length = capacity - 1;
        for (uint32_t i = 0; i < length; i++) display[i] = path[i];
        display[length] = '\0';
        return text_equal(display, "vdisk01") || has_prefix(path, "vdisk01/") || text_equal(path, "vdisk01");
    }

    uint32_t length = text_length(normalized);
    while (length > 0 && normalized[length - 1] == '/') length--;
    if (text_equal(path, normalized)) return 0;
    if (has_prefix(path, normalized) && (path[length] == '\0' || path[length] == '/')) {
        const char *tail = path + length;
        if (*tail == '/') tail++;
        if (*tail == '\0') return 0;
        write_display_name(path, normalized, display, capacity);
        return 1;
    }
    return 0;
}

int bash_cmd_ls(int argc, char **argv) {
    int long_format = 0;
    int human_readable = 0;
    int show_all = 0;
    const char *requested = "";
    int positional_seen = 0;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            if (positional_seen == 0) {
                requested = argv[i];
                positional_seen = 1;
            } else {
                const char *usage = "usage: ls [-a] [-l] [-h] [path]\n";
                bash_write(usage, text_length(usage));
                return 2;
            }
            continue;
        }

        if (text_equal(argv[i], "-a") || text_equal(argv[i], "--all")) {
            show_all = 1;
            continue;
        }
        if (text_equal(argv[i], "-l") || text_equal(argv[i], "--long")) {
            long_format = 1;
            continue;
        }
        if (text_equal(argv[i], "-h") || text_equal(argv[i], "--human-readable")) {
            human_readable = 1;
            continue;
        }
        if (text_equal(argv[i], "-lh") || text_equal(argv[i], "-al") || text_equal(argv[i], "-la") ||
            text_equal(argv[i], "-hl") || text_equal(argv[i], "-ah") || text_equal(argv[i], "-ha")) {
            long_format = 1;
            human_readable = 1;
            show_all = 1;
            continue;
        }

        const char *usage = "usage: ls [-a] [-l] [-h] [path]\n";
        bash_write(usage, text_length(usage));
        return 2;
    }

    uint32_t count = bash_file_count();
    uint32_t printed = 0;
    char seen[64][64];
    uint32_t seen_count = 0;

    for (uint32_t i = 0; i < count; i++) {
        char path[512];
        if (!bash_file_path(i, path, sizeof(path))) continue;
        char display[128];
        if (!path_matches_target(path, requested, display, sizeof(display))) continue;

        if (!show_all && display[0] == '.') continue;

        int duplicate = 0;
        for (uint32_t j = 0; j < seen_count; j++) {
            if (text_equal(seen[j], display)) {
                duplicate = 1;
                break;
            }
        }
        if (duplicate || display[0] == '\0') continue;

        for (uint32_t j = 0; j < text_length(display); j++) seen[seen_count][j] = display[j];
        seen[seen_count][text_length(display)] = '\0';
        seen_count++;

        if (long_format) {
            bash_write("-rw-r--r-- 1 root root 0 ", 25u);
            bash_write(display, text_length(display));
        } else {
            bash_write(display, text_length(display));
        }
        bash_write("\n", 1);
        printed++;
    }

    if (printed == 0) {
        const char *error = "ls: no such directory\n";
        bash_write(error, text_length(error));
        return 1;
    }
    return 0;
}
