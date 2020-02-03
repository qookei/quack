#include "debug.h"
#include <io/port.h>
#include <arch/io.h>
#include <io/vga.h>
#include <cmdline.h>
#include <genfb/genfb.h>

static int debug_port_exists = -1;

static int use_debug_port = 1;
static int use_vga_con = 0;
static int use_fb_con = 0;

void debugcon_init(arch_video_mode_t *v) {
	use_debug_port = cmdline_has_value("debugcon", "e9");
	use_vga_con = cmdline_has_value("debugcon", "vga");
	use_fb_con = cmdline_has_value("debugcon", "fb");

	if (use_fb_con)
		genfb_init(v);
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

	if (use_fb_con)
		genfb_putch(c);
}
