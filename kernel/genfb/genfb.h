#ifndef GENFB_H
#define GENFB_H

#include <arch/info.h>

void genfb_init(arch_video_mode_t *mode);
void genfb_putch(char c);

#endif
