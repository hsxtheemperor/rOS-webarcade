#include "mkwebfs.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WEBFS1_MAGIC "WEBFS1\0\0"
#define WEBFS1_PATH_LIMIT 65535u

static void write_u16_le(uint8_t *out, uint16_t value) {
    out[0] = (uint8_t)(value & 0xffu);
    out[1] = (uint8_t)((value >> 8) & 0xffu);
}

static void write_u32_le(uint8_t *out, uint32_t value) {
    out[0] = (uint8_t)(value & 0xffu);
    out[1] = (uint8_t)((value >> 8) & 0xffu);
    out[2] = (uint8_t)((value >> 16) & 0xffu);
    out[3] = (uint8_t)((value >> 24) & 0xffu);
}

static int write_all(FILE *file, const void *data, size_t length) {
    return fwrite(data, 1, length, file) == length;
}

static int append_directory_entry(FILE *file, const char *path) {
    size_t path_length = strlen(path);
    if (path_length == 0 || path_length > WEBFS1_PATH_LIMIT) return 0;

    uint8_t header[6];
    write_u16_le(header, (uint16_t)path_length);
    write_u32_le(header + 2, 0u);
    return write_all(file, header, sizeof(header)) && write_all(file, path, path_length);
}

static int append_file_entry(FILE *file, const char *path, const char *source_path) {
    FILE *source = fopen(source_path, "rb");
    if (!source) return 0;

    if (fseek(source, 0L, SEEK_END) != 0) {
        fclose(source);
        return 0;
    }
    long size = ftell(source);
    if (size < 0 || (uint32_t)size > 0xffffffffu) {
        fclose(source);
        return 0;
    }
    rewind(source);

    size_t path_length = strlen(path);
    if (path_length == 0 || path_length > WEBFS1_PATH_LIMIT) {
        fclose(source);
        return 0;
    }

    uint8_t header[6];
    write_u16_le(header, (uint16_t)path_length);
    write_u32_le(header + 2, (uint32_t)size);
    if (!write_all(file, header, sizeof(header)) || !write_all(file, path, path_length)) {
        fclose(source);
        return 0;
    }

    unsigned char buffer[4096];
    uint32_t remaining = (uint32_t)size;
    while (remaining > 0) {
        size_t chunk = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
        if (fread(buffer, 1, chunk, source) != chunk) {
            fclose(source);
            return 0;
        }
        if (!write_all(file, buffer, chunk)) {
            fclose(source);
            return 0;
        }
        remaining -= (uint32_t)chunk;
    }

    fclose(source);
    return 1;
}

static size_t count_parent_directories(const char *path) {
    size_t count = 0;
    char buffer[1024];
    size_t length = strlen(path);
    if (length == 0 || length >= sizeof(buffer)) return 0;

    memcpy(buffer, path, length + 1);
    for (size_t i = 1; i < length; i++) {
        if (buffer[i] == '/') {
            buffer[i] = '\0';
            if (buffer[0] != '\0') count++;
            buffer[i] = '/';
        }
    }
    return count;
}

static int add_parent_directories(FILE *file, const char *path) {
    char buffer[1024];
    size_t length = strlen(path);
    if (length == 0 || length >= sizeof(buffer)) return 0;

    memcpy(buffer, path, length + 1);
    for (size_t i = 1; i < length; i++) {
        if (buffer[i] == '/') {
            buffer[i] = '\0';
            if (buffer[0] != '\0' && !append_directory_entry(file, buffer)) return 0;
            buffer[i] = '/';
        }
    }
    return 1;
}

int webfs1_build_image_from_entries(const char *output_path,
    const char *const *entries,
    size_t entry_count,
    const char *const *source_paths,
    size_t source_count) {
    if (!output_path || !entries || !source_paths || entry_count != source_count) return 0;

    size_t total_count = 0;
    for (size_t i = 0; i < entry_count; i++) {
        const char *entry = entries[i];
        if (!entry || strlen(entry) == 0 || strlen(entry) > WEBFS1_PATH_LIMIT) return 0;
        total_count += count_parent_directories(entry) + 1u;
    }

    FILE *file = fopen(output_path, "wb");
    if (!file) {
        fprintf(stderr, "mkwebfs: unable to open output %s: %s\n", output_path, strerror(errno));
        return 0;
    }

    if (!write_all(file, WEBFS1_MAGIC, sizeof(WEBFS1_MAGIC) - 1u)) {
        fclose(file);
        return 0;
    }

    uint8_t count_bytes[4];
    write_u32_le(count_bytes, (uint32_t)total_count);
    if (!write_all(file, count_bytes, sizeof(count_bytes))) {
        fclose(file);
        return 0;
    }

    for (size_t i = 0; i < entry_count; i++) {
        const char *entry = entries[i];
        const char *source_path = source_paths[i];
        if (!source_path) {
            fclose(file);
            return 0;
        }
        if (!add_parent_directories(file, entry)) {
            fclose(file);
            return 0;
        }
        if (!append_file_entry(file, entry, source_path)) {
            fprintf(stderr, "mkwebfs: unable to read %s\n", source_path);
            fclose(file);
            return 0;
        }
    }

    fclose(file);
    return 1;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: mkwebfs --output <path> --entry <relative> <source> [--entry <relative> <source> ...]\n");
        return 1;
    }

    const char *output_path = NULL;
    size_t entry_count = 0;
    size_t capacity = 8u;
    const char **entries = calloc(capacity, sizeof(*entries));
    const char **sources = calloc(capacity, sizeof(*sources));
    if (!entries || !sources) {
        free(entries);
        free(sources);
        fprintf(stderr, "mkwebfs: out of memory\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--output") == 0 || strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= argc) {
                free(entries);
                free(sources);
                return 1;
            }
            output_path = argv[++i];
        } else if (strcmp(argv[i], "--entry") == 0 || strcmp(argv[i], "-e") == 0) {
            if (i + 2 >= argc) {
                free(entries);
                free(sources);
                return 1;
            }
            if (entry_count >= capacity) {
                size_t new_capacity = capacity * 2u;
                const char **new_entries = realloc(entries, new_capacity * sizeof(*entries));
                if (!new_entries) {
                    free(entries);
                    free(sources);
                    return 1;
                }
                entries = new_entries;

                const char **new_sources = realloc(sources, new_capacity * sizeof(*sources));
                if (!new_sources) {
                    free(entries);
                    free(sources);
                    return 1;
                }
                sources = new_sources;
                capacity = new_capacity;
            }
            entries[entry_count] = argv[++i];
            sources[entry_count] = argv[++i];
            entry_count++;
        }
    }

    if (!output_path || entry_count == 0) {
        free(entries);
        free(sources);
        fprintf(stderr, "mkwebfs: missing output or entries\n");
        return 1;
    }

    int ok = webfs1_build_image_from_entries(output_path, entries, entry_count, sources, entry_count);
    free(entries);
    free(sources);
    return ok ? 0 : 1;
}
