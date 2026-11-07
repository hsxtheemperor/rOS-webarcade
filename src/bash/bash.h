#ifndef ROS_BASH_H
#define ROS_BASH_H

#include <stdint.h>

#define BASH_MAX_ARGUMENTS 16u
#define BASH_ARGUMENT_CAPACITY 4096u
#define BASH_FILE_CAPACITY 32768u
#define BASH_FILE_NOT_FOUND 0xffffffffu
#define BASH_FILE_TOO_LARGE 0xfffffffeu

__attribute__((import_module("kernel"), import_name("write")))
extern void bash_write(const char *text, uint32_t length);

__attribute__((import_module("kernel"), import_name("clear")))
extern void bash_clear(void);

__attribute__((import_module("kernel"), import_name("file_count")))
extern uint32_t bash_file_count(void);

__attribute__((import_module("kernel"), import_name("file_path")))
extern uint32_t bash_file_path(uint32_t index, char *buffer, uint32_t capacity);

__attribute__((import_module("kernel"), import_name("file_read")))
extern uint32_t bash_file_read(const char *path, uint8_t *buffer, uint32_t capacity);

__attribute__((import_module("kernel"), import_name("file_write")))
extern uint32_t bash_file_write(const char *path, const uint8_t *buffer, uint32_t length);

__attribute__((import_module("kernel"), import_name("file_remove")))
extern uint32_t bash_file_remove(const char *path);

__attribute__((import_module("kernel"), import_name("dir_make")))
extern uint32_t bash_dir_make(const char *path);

__attribute__((import_module("kernel"), import_name("disk_size")))
extern uint32_t bash_disk_size(void);

uint32_t bash_arguments_buffer(void);
int bash_execute(uint32_t argc);
int bash_cmd_cat(int argc, char **argv);
int bash_cmd_clear(int argc, char **argv);
int bash_cmd_cp(int argc, char **argv);
int bash_cmd_date(int argc, char **argv);
int bash_cmd_df(int argc, char **argv);
int bash_cmd_echo(int argc, char **argv);
int bash_cmd_free(int argc, char **argv);
int bash_cmd_grep(int argc, char **argv);
int bash_cmd_head(int argc, char **argv);
int bash_cmd_help(int argc, char **argv);
int bash_cmd_history(int argc, char **argv);
int bash_cmd_ls(int argc, char **argv);
int bash_cmd_lsblk(int argc, char **argv);
int bash_cmd_mkdir(int argc, char **argv);
int bash_cmd_mv(int argc, char **argv);
int bash_cmd_ps(int argc, char **argv);
int bash_cmd_pwd(int argc, char **argv);
int bash_cmd_rm(int argc, char **argv);
int bash_cmd_rmdir(int argc, char **argv);
int bash_cmd_tail(int argc, char **argv);
int bash_cmd_touch(int argc, char **argv);
int bash_cmd_tree(int argc, char **argv);
int bash_cmd_uname(int argc, char **argv);
int bash_cmd_wc(int argc, char **argv);
int bash_cmd_whoami(int argc, char **argv);

#endif
