#include "debug.h"
#include <io/port.h>

static int debug_port_exists = -1;

void debug_putch(char c) {
	#ifndef NO_DEBUG_OUT
	if (debug_port_exists == -1)
		debug_port_exists = inb(0xE9) == 0xE9 ? 1 : 0;

	if (debug_port_exists)
		outb(0xE9, c);
	#else
	(void)c;
	#endif
}

void debug_putstr(const char *s) {
	while (*s) {
		debug_putch(*s);
		s++;
	}
}
