#ifndef DEBUG_H
#define DEBUG_H

#include <arch/info.h>

void debugcon_init(arch_video_mode_t *vid);

void arch_debug_write(char c);

#endif
