#ifndef ROS_PROC0_H
#define ROS_PROC0_H

#include <stdint.h>

#define PROC0_MAX_ARGUMENTS 16u
#define PROC0_COMMAND_CAPACITY 512u

struct proc0_api {
    void *context;
    void (*write)(void *context, const char *text, uint32_t length);
    uint32_t (*execute)(void *context, int argc, char **argv);
};

uint32_t proc0_start(const struct proc0_api *api);
uint32_t proc0_submit(const char *line, uint32_t length);

#endif