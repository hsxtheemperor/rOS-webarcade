#include "proc0.h"

static struct proc0_api process_api;
static uint32_t process_running;
static char command_storage[PROC0_COMMAND_CAPACITY];
static char original_line[PROC0_COMMAND_CAPACITY];
static char *arguments[PROC0_MAX_ARGUMENTS];

static uint32_t text_length(const char *text) {
    uint32_t length = 0;
    while (text && text[length]) length++;
    return length;
}

static uint32_t is_space(char value) {
    return value == ' ' || value == '\t';
}

static int tokenize(char *line, int *argument_count) {
    int count = 0;
    char *read = line;
    char *write = line;

    while (*read) {
        while (is_space(*read)) read++;
        if (!*read) break;
        if (count == (int)PROC0_MAX_ARGUMENTS) return 0;
        arguments[count++] = write;

        char quote = 0;
        while (*read) {
            if (quote) {
                if (*read == quote) {
                    quote = 0;
                    read++;
                } else if (*read == '\\' && read[1]) {
                    read++;
                    *write++ = *read++;
                } else {
                    *write++ = *read++;
                }
            } else if (*read == '\'' || *read == '"') {
                quote = *read++;
            } else if (is_space(*read)) {
                read++;
                break;
            } else if (*read == '\\' && read[1]) {
                read++;
                *write++ = *read++;
            } else {
                *write++ = *read++;
            }
        }
        if (quote) return 0;
        *write++ = '\0';
    }

    *argument_count = count;
    return 1;
}

static uint32_t text_equal(const char *left, const char *right) {
    while (*left && *right && *left == *right) {
        left++;
        right++;
    }
    return *left == '\0' && *right == '\0';
}

uint32_t proc0_start(const struct proc0_api *api) {
    if (!api || !api->write || !api->execute || process_running) return 0;
    process_api = *api;
    process_running = 1;
    return 1;
}

uint32_t proc0_submit(const char *line, uint32_t length) {
    if (!process_running || !line || length >= sizeof(command_storage)) return 0;

    for (uint32_t i = 0; i < length; i++) {
        command_storage[i] = line[i];
        original_line[i] = line[i];
    }
    command_storage[length] = '\0';
    original_line[length] = '\0';

    int argc = 0;
    if (!tokenize(command_storage, &argc)) {
        const char *error = "\nbash: invalid quoting or too many arguments\n";
        process_api.write(process_api.context, "$ ", 2);
        process_api.write(process_api.context, original_line, length);
        process_api.write(process_api.context, error, text_length(error));
        return 1;
    }
    if (argc == 0) return 1;
    if (argc == 1 && text_equal(arguments[0], "clear")) {
        process_api.execute(process_api.context, argc, arguments);
        return 1;
    }
    process_api.write(process_api.context, "$ ", 2);
    process_api.write(process_api.context, original_line, text_length(original_line));
    process_api.write(process_api.context, "\n", 1);
    process_api.execute(process_api.context, argc, arguments);
    return 1;
}
