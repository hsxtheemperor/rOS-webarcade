#ifndef ROS_JSAPI_H
#define ROS_JSAPI_H

#include <stdint.h>

__attribute__((import_module("browser"), import_name("create")))
extern uint32_t browser_create(const char *tag);

__attribute__((import_module("browser"), import_name("append")))
extern void browser_append(uint32_t parent, uint32_t child);

__attribute__((import_module("browser"), import_name("clear")))
extern void browser_clear(uint32_t element);

__attribute__((import_module("browser"), import_name("set_text")))
extern void browser_set_text(uint32_t element, const char *text);

__attribute__((import_module("browser"), import_name("set_style")))
extern void browser_set_style(uint32_t element, const char *property, const char *value);

__attribute__((import_module("browser"), import_name("set_attribute")))
extern void browser_set_attribute(uint32_t element, const char *name, const char *value);

__attribute__((import_module("browser"), import_name("on_enter")))
extern void browser_on_enter(uint32_t element);

__attribute__((import_module("browser"), import_name("get_value")))
extern uint32_t browser_get_value(uint32_t element, char *buffer, uint32_t capacity);

__attribute__((import_module("browser"), import_name("set_value")))
extern void browser_set_value(uint32_t element, const char *value);

__attribute__((import_module("browser"), import_name("focus")))
extern void browser_focus(uint32_t element);

__attribute__((import_module("browser"), import_name("vfs_count")))
extern uint32_t browser_vfs_count(void);

__attribute__((import_module("browser"), import_name("vfs_path")))
extern uint32_t browser_vfs_path(uint32_t index, char *buffer, uint32_t capacity);

__attribute__((import_module("browser"), import_name("vfs_read")))
extern uint32_t browser_vfs_read(const char *path, uint8_t *buffer, uint32_t capacity);

__attribute__((import_module("browser"), import_name("vfs_write")))
extern uint32_t browser_vfs_write(const char *path, const uint8_t *data, uint32_t length);

__attribute__((import_module("browser"), import_name("vfs_delete")))
extern uint32_t browser_vfs_delete(const char *path);

__attribute__((import_module("browser"), import_name("vfs_mkdir")))
extern uint32_t browser_vfs_mkdir(const char *path);

__attribute__((import_module("browser"), import_name("bash_execute")))
extern uint32_t browser_bash_execute(uint32_t argc, char **argv);

#endif
