#ifndef PS2_KBD
#define PS2_KBD

#include "../interrupt/isr.h"
#include "../io/ports.h"

void ps2_kbd_init();


void reset_buffer();
char getch();
char readch();

#endif