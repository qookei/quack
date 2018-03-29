#ifndef PS2_MOUSE
#define PS2_MOUSE

#include <stdint.h>
#include <stddef.h>
#include <interrupt/isr.h>

void ps2mouse_init();

bool ps2mouse_haschanged();
int32_t ps2mouse_get_mouse_x();
int32_t ps2mouse_get_mouse_y();
uint8_t ps2mouse_get_mouse_buttons();

#endif