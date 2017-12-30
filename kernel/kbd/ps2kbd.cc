#include "ps2kbd.h"

#define PS2_DATA_PORT 0x60
#define PS2_COMM_PORT 0x64
#define PS2_STAT_PORT 0x64

#define BITTEST(var,pos) ((var) & (1 << (pos)))

bool ps2_dual_channel;

void inline ps2_wait_response () {
	while (!BITTEST(inb(PS2_STAT_PORT), 0));
}

void inline ps2_wait_ready () {
	while (BITTEST(inb(PS2_STAT_PORT), 1));
}

extern int printf(const char*, ...);

char ps2_keyboard_buffer[512] = {0};
uint32_t ps2_keyboard_buffer_idx = 0;

void ps2_kbd_init() {

#ifdef PS2_KBD_INIT_CONT

	asm volatile ("cli");

	outb(PS2_COMM_PORT, 0xAD);								// disable ports
	outb(PS2_COMM_PORT, 0xA7);
	while (BITTEST(inb(PS2_STAT_PORT), 1)) inb(PS2_DATA_PORT); // flush buffers

	printf("[ps2kbd] buffer flush ok\n");

	outb(PS2_COMM_PORT, 0x20);
	ps2_wait_response();
	uint8_t config = inb(PS2_DATA_PORT);					// get config byte

	ps2_dual_channel = !BITTEST(config, 5);

	config &= ~((1<<6));									// patch it around
	
	outb(PS2_COMM_PORT, 0x60);								// write it back
	ps2_wait_ready();
	outb(PS2_DATA_PORT, config);

	printf("[ps2kbd] patch config ok, %02x\n", config);

	outb(PS2_COMM_PORT, 0xAA);
	ps2_wait_response();
	uint8_t resp = inb(PS2_DATA_PORT);
	if(resp == 0x55) {
		printf("[ps2kbd] controller selftest ok\n");
		//return;
	
	} else if(resp == 0xFC) {

		printf("[ps2kbd] controller selftest failure\n");
		return;
	} else {
		printf("[ps2kbd] reponse is apparently %02x\n", resp);
		
	}

	if (ps2_dual_channel) {
		outb(PS2_COMM_PORT, 0x20);							// check for second port
		ps2_wait_response();
		uint8_t config = inb(PS2_DATA_PORT);				// get config byte again
		if (!BITTEST(config, 5)) {
			ps2_dual_channel = true;
			outb(PS2_COMM_PORT, 0xA7);
		} else ps2_dual_channel = false;
	}

	printf("[ps2kbd] check for second port ok\n");

	outb(PS2_COMM_PORT, 0xAB);
	ps2_wait_response();
	resp = inb(PS2_DATA_PORT);
	if (resp != 0x00) {
		printf("[ps2kbd] port1 failure, %02x\n", resp);
		return;
	}

	if (ps2_dual_channel) {
		outb(PS2_COMM_PORT, 0xA9);
		ps2_wait_response();
		if (inb(PS2_DATA_PORT) != 0x00) {
			printf("[ps2kbd] port2 failure\n");
			return;
				// ps2 port sukk
		}
	}

	printf("[ps2kbd] interface test ok\n");

	outb(PS2_COMM_PORT, 0xAE);
	if (ps2_dual_channel) outb(PS2_COMM_PORT, 0xA8);

	//outb(PS2_COMM_PORT, 0x20);
	//ps2_wait_response();
	//config = inb(PS2_DATA_PORT) | 0x3;						// get config byte and patch it yet again 

	// outb(PS2_COMM_PORT, 0x60);								// write it back
	// ps2_wait_ready();
	// outb(PS2_DATA_PORT, config);

	// printf("[ps2kbd] enable interrupts ok\n");

	while (!BITTEST(inb(PS2_STAT_PORT), 2));
	outb(PS2_DATA_PORT, 0xFF); 								// keyboard should work now, i hope

	printf("[ps2kbd] done\n");


	asm volatile ("sti");

#endif


}

char getch() {
	// wait for keyboard buffer to be not empty, then return the character and remove it from the buffer
	while (ps2_keyboard_buffer_idx == 0);
	while (ps2_keyboard_buffer[ps2_keyboard_buffer_idx - 1] == 0);
	return ps2_keyboard_buffer[--ps2_keyboard_buffer_idx];
}