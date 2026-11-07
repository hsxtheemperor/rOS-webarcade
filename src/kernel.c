#include <stdint.h>
#include "api/jsapi.h"
#include "boot/initrd/fsys.h"
#include "boot/initrd/proc0.h"

#define INITRD_CAPACITY (256u * 1024u)
#define INPUT_CAPACITY 512u
#define OUTPUT_CAPACITY 32768u
#define BRIDGE_CAPACITY 32768u
#define BASH_FILE_NOT_FOUND 0xffffffffu
#define BASH_FILE_TOO_LARGE 0xfffffffeu

struct kernel_process {
    uint32_t pid;
    uint32_t running;
    const char *name;
};

static uint8_t initrd_image[INITRD_CAPACITY];
static char bridge_buffer[BRIDGE_CAPACITY];
static char command_buffer[INPUT_CAPACITY];
static char terminal_output[OUTPUT_CAPACITY];
static uint32_t terminal_output_length;
static uint32_t terminal_element;
static uint32_t input_element;
static uint32_t kernel_started;
static uint32_t mounted_image_size;
static struct kernel_process processes[1];

static uint32_t text_length(const char *text) {
    uint32_t length = 0;
    while (text && text[length]) length++;
    return length;
}

static void console_write(void *context, const char *text, uint32_t length) {
    (void)context;
    if (!text) return;
    for (uint32_t i = 0; i < length; i++) {
        if (terminal_output_length + 1 >= OUTPUT_CAPACITY) break;
        terminal_output[terminal_output_length++] = text[i];
    }
    terminal_output[terminal_output_length] = '\0';
}

static void console_clear(void *context) {
    (void)context;
    terminal_output_length = 0;
    terminal_output[0] = '\0';
}

static uint32_t kernel_execute_bash(void *context, int argc, char **argv) {
    (void)context;
    return browser_bash_execute((uint32_t)argc, argv);
}

static uint32_t kernel_spawn_proc0(void) {
    if (processes[0].running) return 0;
    const struct proc0_api api = {
        .context = 0,
        .write = console_write,
        .execute = kernel_execute_bash
    };
    if (!proc0_start(&api)) return 0;
    processes[0].pid = 1;
    processes[0].name = "proc0";
    processes[0].running = 1;
    return 1;
}

static uint32_t element(uint32_t parent, const char *tag, const char *text) {
    uint32_t id = browser_create(tag);
    if (text) browser_set_text(id, text);
    browser_append(parent, id);
    return id;
}

static void style(uint32_t id, const char *property, const char *value) {
    browser_set_style(id, property, value);
}

static void render_terminal(void) {
    browser_clear(1);
    style(1, "margin", "0");
    style(1, "min-height", "100vh");
    style(1, "display", "block");
    style(1, "place-items", "normal");
    style(1, "background", "#000");
    style(1, "color", "#eee");
    style(1, "font-family", "ui-monospace, SFMono-Regular, Menlo, Consolas, monospace");
    style(1, "font-size", "16px");
    style(1, "line-height", "1.45");
    style(1, "padding", "8px");
    style(1, "box-sizing", "border-box");

    terminal_element = element(1, "pre", "");
    style(terminal_element, "overflow", "auto");
    style(terminal_element, "margin", "0");
    style(terminal_element, "white-space", "pre-wrap");
    style(terminal_element, "font", "inherit");
    style(terminal_element, "color", "#eee");

    uint32_t prompt = element(1, "div", 0);
    style(prompt, "display", "flex");
    style(prompt, "align-items", "baseline");
    uint32_t prompt_symbol = element(prompt, "span", "$ ");
    style(prompt_symbol, "color", "#7ee787");
    input_element = element(prompt, "input", 0);
    browser_set_attribute(input_element, "type", "text");
    browser_set_attribute(input_element, "aria-label", "Shell command");
    browser_set_attribute(input_element, "autocomplete", "off");
    browser_set_attribute(input_element, "autocapitalize", "off");
    browser_set_attribute(input_element, "spellcheck", "false");
    style(input_element, "flex", "1");
    style(input_element, "min-width", "0");
    style(input_element, "padding", "0");
    style(input_element, "border", "0");
    style(input_element, "outline", "none");
    style(input_element, "background", "transparent");
    style(input_element, "color", "#eee");
    style(input_element, "font", "inherit");
    style(input_element, "font-size", "16px");
    browser_on_enter(input_element);
}

uint32_t ros_initrd_buffer(void) {
    return (uint32_t)(uintptr_t)initrd_image;
}

uint32_t ros_initrd_capacity(void) {
    return INITRD_CAPACITY;
}

uint32_t ros_bash_bridge_buffer(void) {
    return (uint32_t)(uintptr_t)bridge_buffer;
}

uint32_t ros_bash_bridge_capacity(void) {
    return BRIDGE_CAPACITY;
}

void ros_bash_write(uint32_t length) {
    if (length <= BRIDGE_CAPACITY) console_write(0, bridge_buffer, length);
}

void ros_bash_clear(void) {
    console_clear(0);
}

uint32_t ros_bash_file_count(void) {
    return browser_vfs_count();
}

uint32_t ros_bash_file_path(uint32_t index) {
    return browser_vfs_path(index, bridge_buffer, BRIDGE_CAPACITY);
}

uint32_t ros_bash_file_read(uint32_t path_length) {
    if (path_length >= BRIDGE_CAPACITY) return BASH_FILE_NOT_FOUND;
    bridge_buffer[path_length] = '\0';
    uint32_t size = browser_vfs_read(bridge_buffer, (uint8_t *)bridge_buffer, BRIDGE_CAPACITY);
    if (size == 0xffffffffu || size == 0xfffffffeu) return size;
    return size;
}

uint32_t ros_bash_disk_size(void) {
    return mounted_image_size;
}

uint32_t ros_kernel_start(uint32_t initrd_length) {
    if (kernel_started || initrd_length > INITRD_CAPACITY) return 0;
    if (webfs1_mount_readonly(initrd_image, initrd_length) != WEBFS1_OK) return 0;

    mounted_image_size = initrd_length;
    kernel_started = 1;
    render_terminal();
    if (!kernel_spawn_proc0()) return 0;
    browser_focus(input_element);
    return 1;
}

void ros_terminal_submit(void) {
    if (!kernel_started || !processes[0].running || !input_element || !terminal_element) return;
    uint32_t length = browser_get_value(input_element, command_buffer, INPUT_CAPACITY);
    if (length >= INPUT_CAPACITY) length = INPUT_CAPACITY - 1;
    command_buffer[length] = '\0';

    if (length) proc0_submit(command_buffer, length);

    browser_set_text(terminal_element, terminal_output);
    browser_set_value(input_element, "");
    browser_focus(input_element);
}
