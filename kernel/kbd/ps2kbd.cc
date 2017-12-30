#include "ps2kbd.h"

#define PS2_DATA_PORT 0x60
#define PS2_COMM_PORT 0x64
#define PS2_STAT_PORT 0x64

#define BITTEST(var,pos) ((var) & (1 << (pos)))

bool ps2_dual_channel;

void inline ps2_wait_response (){
	while (!(inb(PS2_STAT_PORT) & 0x1));
}

extern int printf(const char*, ...);

void ps2_kbd_init() {

	//asm volatile ("cli");

	outb(PS2_COMM_PORT, 0xAD);								// disable ports
	outb(PS2_COMM_PORT, 0xA7);
	while (!(inb(PS2_STAT_PORT) & 0x1)) inb(PS2_DATA_PORT); // flush buffers

	printf("[ps2kbd] buffer flush ok\n");

	outb(PS2_COMM_PORT, 0x20);
	ps2_wait_response();
	uint8_t config = inb(PS2_DATA_PORT);					// get config byte

	config = config & 0b00110100;							// patch it around
	ps2_dual_channel = !BITTEST(config, 5);

	outb(PS2_COMM_PORT, 0x60);								// write it back
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

	outb(PS2_DATA_PORT, 0xAB);
	ps2_wait_response();
	if (inb(PS2_DATA_PORT) != 0x00) {
		printf("[ps2kbd] port1 failure\n");
		return;
	}

	if (ps2_dual_channel) {
		outb(PS2_DATA_PORT, 0xA9);
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

	outb(PS2_COMM_PORT, 0x20);
	ps2_wait_response();
	config = inb(PS2_DATA_PORT) | 0x3;						// get config byte and patch it yet again 

	outb(PS2_COMM_PORT, 0x60);								// write it back
	outb(PS2_COMM_PORT, config);

	printf("[ps2kbd] enable interrupts ok\n");

	while (!(inb(PS2_STAT_PORT) & 0x2));
	outb(PS2_DATA_PORT, 0xFF); 								// keyboard should work now, i hope

	printf("[ps2kbd] done\n");


	//asm volatile ("sti");
}
