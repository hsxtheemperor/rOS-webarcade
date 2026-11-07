#include "bash.h"

struct command_entry {
    const char *name;
    int (*run)(int argc, char **argv);
};

static const struct command_entry commands[] = {
    { "cat", bash_cmd_cat },
    { "clear", bash_cmd_clear },
    { "cp", bash_cmd_cp },
    { "date", bash_cmd_date },
    { "df", bash_cmd_df },
    { "echo", bash_cmd_echo },
    { "free", bash_cmd_free },
    { "grep", bash_cmd_grep },
    { "head", bash_cmd_head },
    { "help", bash_cmd_help },
    { "history", bash_cmd_history },
    { "ls", bash_cmd_ls },
    { "lsblk", bash_cmd_lsblk },
    { "mkdir", bash_cmd_mkdir },
    { "mv", bash_cmd_mv },
    { "ps", bash_cmd_ps },
    { "pwd", bash_cmd_pwd },
    { "rm", bash_cmd_rm },
    { "rmdir", bash_cmd_rmdir },
    { "tail", bash_cmd_tail },
    { "touch", bash_cmd_touch },
    { "tree", bash_cmd_tree },
    { "uname", bash_cmd_uname },
    { "wc", bash_cmd_wc },
    { "whoami", bash_cmd_whoami }
};

static char argument_storage[BASH_ARGUMENT_CAPACITY];
static char *arguments[BASH_MAX_ARGUMENTS];

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

uint32_t bash_arguments_buffer(void) {
    return (uint32_t)(uintptr_t)argument_storage;
}

int bash_execute(uint32_t argc) {
    if (argc == 0 || argc > BASH_MAX_ARGUMENTS) return 2;

    uint32_t offset = 0;
    for (uint32_t i = 0; i < argc; i++) {
        if (offset >= sizeof(argument_storage)) return 2;
        arguments[i] = argument_storage + offset;
        while (offset < sizeof(argument_storage) && argument_storage[offset]) offset++;
        if (offset == sizeof(argument_storage)) return 2;
        offset++;
    }

    for (uint32_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        if (text_equal(commands[i].name, arguments[0])) {
            return commands[i].run((int)argc, arguments);
        }
    }

    const char *error = "bash: command not found: ";
    bash_write(error, text_length(error));
    bash_write(arguments[0], text_length(arguments[0]));
    bash_write("\n", 1);
    return 127;
}
