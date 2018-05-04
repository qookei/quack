#ifndef PS2_KBD
#define PS2_KBD

#include <interrupt/isr.h>
#include <io/ports.h>
#include <string.h>

void ps2_kbd_init();

bool ps2_load_keyboard_map(const char *);

void ps2_kbd_reset_buffer();
char getch();
char readch();

#endif
