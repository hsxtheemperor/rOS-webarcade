#ifndef ROS_MKWEBFS_H
#define ROS_MKWEBFS_H

#include <stddef.h>
#include <stdint.h>

int webfs1_build_image_from_entries(const char *output_path,
    const char *const *entries,
    size_t entry_count,
    const char *const *source_paths,
    size_t source_count);

#endif
