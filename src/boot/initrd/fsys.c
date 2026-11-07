#include "fsys.h"

static const uint8_t webfs1_magic[WEBFS1_MAGIC_SIZE] = {
    'W', 'E', 'B', 'F', 'S', '1', 0, 0
};

static const uint8_t *mounted_image;
static uint32_t mounted_count;
static enum webfs1_status mounted_status = WEBFS1_INVALID_ARGUMENT;

static uint32_t read_u16(const uint8_t *data) {
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8);
}

static uint32_t read_u32(const uint8_t *data) {
    return (uint32_t)data[0]
        | ((uint32_t)data[1] << 8)
        | ((uint32_t)data[2] << 16)
        | ((uint32_t)data[3] << 24);
}

static int paths_equal(const uint8_t *stored, uint32_t length, const char *path) {
    if (!path) return 0;
    uint32_t i = 0;
    while (i < length && path[i] && stored[i] == (uint8_t)path[i]) i++;
    return i == length && path[i] == '\0';
}

enum webfs1_status webfs1_mount_readonly(const uint8_t *image, uint32_t image_size) {
    mounted_image = 0;
    mounted_count = 0;
    mounted_status = WEBFS1_INVALID_ARGUMENT;

    if (!image) return mounted_status;
    if (image_size < WEBFS1_HEADER_SIZE) {
        mounted_status = WEBFS1_TRUNCATED;
        return mounted_status;
    }

    for (uint32_t i = 0; i < WEBFS1_MAGIC_SIZE; i++) {
        if (image[i] != webfs1_magic[i]) {
            mounted_status = WEBFS1_BAD_MAGIC;
            return mounted_status;
        }
    }

    uint32_t count = read_u32(image + WEBFS1_MAGIC_SIZE);
    uint32_t offset = WEBFS1_HEADER_SIZE;
    for (uint32_t i = 0; i < count; i++) {
        if (offset > image_size || image_size - offset < 6u) {
            mounted_status = WEBFS1_TRUNCATED;
            return mounted_status;
        }

        uint32_t path_size = read_u16(image + offset);
        uint32_t data_size = read_u32(image + offset + 2);
        offset += 6u;
        if (path_size == 0 || path_size > image_size - offset) {
            mounted_status = WEBFS1_INVALID_ENTRY;
            return mounted_status;
        }
        for (uint32_t j = 0; j < path_size; j++) {
            if (image[offset + j] == 0) {
                mounted_status = WEBFS1_INVALID_ENTRY;
                return mounted_status;
            }
        }
        offset += path_size;
        if (data_size > image_size - offset) {
            mounted_status = WEBFS1_TRUNCATED;
            return mounted_status;
        }
        offset += data_size;
    }

    if (offset != image_size) {
        mounted_status = WEBFS1_INVALID_ENTRY;
        return mounted_status;
    }

    mounted_image = image;
    mounted_count = count;
    mounted_status = WEBFS1_OK;
    return mounted_status;
}

uint32_t webfs1_file_count(void) {
    return mounted_status == WEBFS1_OK ? mounted_count : 0;
}

static int webfs1_entry(uint32_t requested, const uint8_t **path, uint32_t *path_size,
                        const uint8_t **data, uint32_t *data_size) {
    if (mounted_status != WEBFS1_OK || requested >= mounted_count) return 0;
    uint32_t offset = WEBFS1_HEADER_SIZE;
    for (uint32_t i = 0; i <= requested; i++) {
        uint32_t current_path_size = read_u16(mounted_image + offset);
        uint32_t current_data_size = read_u32(mounted_image + offset + 2);
        offset += 6u;
        const uint8_t *current_path = mounted_image + offset;
        offset += current_path_size;
        const uint8_t *current_data = mounted_image + offset;
        offset += current_data_size;
        if (i == requested) {
            if (path) *path = current_path;
            if (path_size) *path_size = current_path_size;
            if (data) *data = current_data;
            if (data_size) *data_size = current_data_size;
            return 1;
        }
    }
    return 0;
}

uint32_t webfs1_path_at(uint32_t index, char *buffer, uint32_t capacity) {
    const uint8_t *path;
    uint32_t length;
    if (!buffer || capacity == 0 || !webfs1_entry(index, &path, &length, 0, 0)) return 0;
    uint32_t copied = length < capacity - 1 ? length : capacity - 1;
    for (uint32_t i = 0; i < copied; i++) buffer[i] = (char)path[i];
    buffer[copied] = '\0';
    return copied == length;
}

const uint8_t *webfs1_read(const char *path, uint32_t *size) {
    if (size) *size = 0;
    if (!path || mounted_status != WEBFS1_OK) return 0;

    for (uint32_t i = 0; i < mounted_count; i++) {
        const uint8_t *entry_path;
        const uint8_t *data;
        uint32_t path_size;
        uint32_t data_size;
        if (!webfs1_entry(i, &entry_path, &path_size, &data, &data_size)) return 0;
        if (paths_equal(entry_path, path_size, path)) {
            if (size) *size = data_size;
            return data;
        }
    }
    return 0;
}