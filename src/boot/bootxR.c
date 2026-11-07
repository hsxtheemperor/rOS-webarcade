#include "bootxR.h"

__attribute__((import_module("bios"), import_name("launch_kernel")))
extern uint32_t bios_launch_kernel(void);

uint32_t wboot_start(void) {
    return bios_launch_kernel();
}