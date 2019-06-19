#include "debug.h"
#include <io/port.h>
#include <arch/io.h>
#include <io/vga.h>

static int debug_port_exists = -1;

void arch_debug_write(char c) {
	#ifndef NO_DEBUG_OUT
	if (debug_port_exists == -1)
		debug_port_exists = inb(0xE9) == 0xE9 ? 1 : 0;

	if (debug_port_exists)
		outb(0xE9, c);
	#else
	(void)c;
	#endif
	//vga_putch(c);
}
