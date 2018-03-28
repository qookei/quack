#include "ps2mouse.h"

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
	printf("ps2 mouse init\n");

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

	// ps2_wait_ready();
	// outb(0x64, 0xD4);
	// ps2_wait_ready();
	// outb(0x60, 0xE8);
	// ps2_wait_ready();
	// outb(0x64, 0xD4);
	// ps2_wait_ready();
	// outb(0x60, 0x00);

	
	register_interrupt_handler(0x2C, ps2mouse_interrupt);
}

int32_t mouse_x = 0;
int32_t mouse_y = 0;

uint8_t first;
uint8_t sec;
uint8_t third;

uint8_t which = 0;

extern void vesa_ppx(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b);
extern void vesa_resetppx(uint32_t x, uint32_t y);


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
		vesa_resetppx(mouse_x, mouse_y);
		vesa_resetppx(mouse_x + 1, mouse_y);
		vesa_resetppx(mouse_x + 1, mouse_y + 1);
		vesa_resetppx(mouse_x, mouse_y + 1);
		
		uint8_t rel_x = sec;
		uint8_t rel_y = third;

		int16_t mouse_x_move = 0;
		int16_t mouse_y_move = 0;

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

		if(mouse_x < 4) mouse_x = 4;
		if(mouse_y < 4) mouse_y = 4;

		if(mouse_x > 1019) mouse_x = 1019;
		if(mouse_y > 763)  mouse_y = 763;

		vesa_ppx(mouse_x, mouse_y, 0xFF, 0xFF, 0xFF);
		vesa_ppx(mouse_x + 1, mouse_y, 0xFF, 0xFF, 0xFF);
		vesa_ppx(mouse_x + 1, mouse_y + 1, 0xFF, 0xFF, 0xFF);
		vesa_ppx(mouse_x, mouse_y + 1, 0xFF, 0xFF, 0xFF);
	}

	return true;
}