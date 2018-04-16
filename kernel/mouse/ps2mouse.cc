#include "ps2mouse.h"
#include <multiboot.h>

extern multiboot_info_t *mbootinfo;

bool ps2mouse_interrupt(interrupt_cpu_state *state);

#define PS2_DATA_PORT 0x60
#define PS2_COMM_PORT 0x64
#define PS2_STAT_PORT 0x64

#define BITTEST(var,pos) ((var) & (1 << (pos)))

void inline ps2_wait_ready() {
	while (BITTEST(inb(PS2_STAT_PORT), 1));
}

void inline ps2_wait_response() {
	while (!BITTEST(inb(PS2_STAT_PORT), 0));
}

extern int printf(const char *, ...);
extern int kprintf(const char *, ...);

void ps2mouse_init() {
	ps2_wait_ready();
	outb(0x64, 0xD4);
	ps2_wait_ready();
	outb(0x60, 0xFF);

	outb(0x64, 0x20);
	uint8_t byte = inb(0x60);

	byte |= 2;
	byte &= ~(0x20);

	outb(0x64, 0x60);
	outb(0x60, byte);

	ps2_wait_ready();
	outb(0x64, 0xD4);
	ps2_wait_ready();
	outb(0x60, 0xF4);

	register_interrupt_handler(0x2C, ps2mouse_interrupt);
	
	printf("[ps2mse] init done\n");
}

int32_t mouse_x = 0;
int32_t mouse_y = 0;

bool ps2_mouse_left_down;
bool ps2_mouse_right_down;
bool ps2_mouse_middle_down;

uint8_t first;
uint8_t sec;
uint8_t third;

uint8_t which = 0;

bool ps2mouse_changed = false;

bool ps2mouse_haschanged() {
	bool a = ps2mouse_changed;
	ps2mouse_changed = false;
	return a;
}

int32_t ps2mouse_get_mouse_x() {
	return mouse_x;
}

int32_t ps2mouse_get_mouse_y() {
	return mouse_y;
}

uint8_t ps2mouse_get_mouse_buttons() {
	return ((ps2_mouse_left_down ? 1 : 0) << 2) | ((ps2_mouse_middle_down ? 1 : 0) << 1) | (ps2_mouse_right_down ? 1 : 0);
}

bool ps2mouse_interrupt(interrupt_cpu_state *unused) {
	if (!(inb(0x64) & 0x20)) {
		return true;
	}

	(void)unused;
	uint8_t state = inb(0x60);
	
	if (which == 0) {
		first = state;
		which++;
		if (!BITTEST(first, 3)) {
			which = 0;
			return true;
		}
	} else if (which == 1) {
		sec = state;
		which++;
	} else if (which == 2) {
		third = state;
		which = 0;
		
		uint8_t rel_x = sec;
		uint8_t rel_y = third;

		int16_t mouse_x_move = 0;
		int16_t mouse_y_move = 0;

		ps2_mouse_left_down = BITTEST(first, 0);
		ps2_mouse_right_down = BITTEST(first, 1);
		ps2_mouse_middle_down = BITTEST(first, 2);

		if (BITTEST(first, 4))
			mouse_x_move = (int8_t)rel_x;
		else
			mouse_x_move = rel_x;
			
		if (BITTEST(first, 5))
			mouse_y_move = (int8_t)rel_y;
		else
			mouse_y_move = rel_y;

		mouse_x += mouse_x_move;
		mouse_y -= mouse_y_move;

		if (mouse_x < 0) mouse_x = 0;
		if (mouse_y < 0) mouse_y = 0;
		
		if (mouse_x >= mbootinfo->framebuffer_width)  mouse_x = mbootinfo->framebuffer_width - 1;
		if (mouse_y >= mbootinfo->framebuffer_height) mouse_y = mbootinfo->framebuffer_height - 1;

		ps2mouse_changed = true;
	}

	return true;
}