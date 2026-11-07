#ifndef ROS_FSYS_H
#define ROS_FSYS_H

#include <stdint.h>

#define WEBFS1_MAGIC_SIZE 8u
#define WEBFS1_HEADER_SIZE 12u

enum webfs1_status {
    WEBFS1_OK = 0,
    WEBFS1_INVALID_ARGUMENT,
    WEBFS1_BAD_MAGIC,
    WEBFS1_TRUNCATED,
    WEBFS1_INVALID_ENTRY
};

enum webfs1_status webfs1_mount_readonly(const uint8_t *image, uint32_t image_size);
uint32_t webfs1_file_count(void);
uint32_t webfs1_path_at(uint32_t index, char *buffer, uint32_t capacity);
const uint8_t *webfs1_read(const char *path, uint32_t *size);

#endif