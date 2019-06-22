#include "debug.h"
#include <io/port.h>
#include <arch/io.h>
#include <io/vga.h>
#include <cmdline.h>

static int debug_port_exists = -1;

static int use_debug_port = 0;
static int use_vga_con = 0;

#define FORCE_DEBUG_E9
#define FORCE_DEBUG_VGA

void debugcon_init(void) {
	#ifndef FORCE_DEBUG_E9
	use_debug_port = cmdline_has_value("debugcon", "e9");
	#else
	use_debug_port = 1;
	#endif

	#ifndef FORCE_DEBUG_VGA
	use_vga_con = cmdline_has_value("debugcon", "vga");
	#else
	use_vga_con = 1;
	#endif
}

void arch_debug_write(char c) {
	if (use_debug_port) {
		if (debug_port_exists == -1)
			debug_port_exists = inb(0xE9) == 0xE9 ? 1 : 0;

		if (debug_port_exists)
			outb(0xE9, c);
	}

	if (use_vga_con)
		vga_putch(c);
}
