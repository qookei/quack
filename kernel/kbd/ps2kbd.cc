#include "ps2kbd.h"
#include <tasking/tasking.h>

#define PS2_DATA_PORT 0x60
#define PS2_COMM_PORT 0x64
#define PS2_STAT_PORT 0x64

#define SC_MAX 0x57
#define SC_CAPSLOCK 0x3A
#define SC_ENTER 0x1C
#define SC_BACKSPACE 0x0E
#define SC_RIGHT_SHIFT 0x36
#define SC_LEFT_SHIFT 0x2A
#define SC_RIGHT_SHIFT_REL 0xB6
#define SC_LEFT_SHIFT_REL 0xAA
#define SC_F12 0x58

#define BITTEST(var,pos) ((var) & (1 << (pos)))

bool ps2_dual_channel;

bool caps, shift;

void inline ps2_wait_response () {
	while (!BITTEST(inb(PS2_STAT_PORT), 0));
}

void inline ps2_wait_ready () {
	while (BITTEST(inb(PS2_STAT_PORT), 1));
}

extern int printf(const char*, ...);
extern task_t *current_task;

bool ps2_interrupt(interrupt_cpu_state *state);

char ps2_keyboard_buffer[512] = {0};
uint32_t ps2_keyboard_buffer_idx = 0;

#define PS2_KBD_INIT_CONT

void ps2_kbd_init() {

#ifdef PS2_KBD_INIT_CONT

	outb(PS2_COMM_PORT, 0xAD);									// disable ports
	outb(PS2_COMM_PORT, 0xA7);
	while (BITTEST(inb(PS2_STAT_PORT), 1)) inb(PS2_DATA_PORT); 	// flush buffers

	// printf("[ps2kbd] buffer flush ok\n");

	outb(PS2_COMM_PORT, 0x20);
	ps2_wait_response();
	uint8_t config = inb(PS2_DATA_PORT);						// get config byte

	ps2_dual_channel = !BITTEST(config, 5);

	config &= ~((1<<6));										// patch it around
	
	outb(PS2_COMM_PORT, 0x60);									// write it back
	ps2_wait_ready();
	outb(PS2_DATA_PORT, config);

	// printf("[ps2kbd] patch config ok, %02x\n", config);

	outb(PS2_COMM_PORT, 0xAA);
	ps2_wait_response();
	uint8_t resp = inb(PS2_DATA_PORT);
	if(resp == 0x55) {
		// printf("[ps2kbd] controller selftest ok\n");
		//return;
	
	} else if(resp == 0xFC) {

		printf("[ps2kbd] controller selftest failure\n");
		return;
	} else {
		printf("[ps2kbd] reponse is apparently %02x\n", resp);
		
	}

	if (ps2_dual_channel) {
		outb(PS2_COMM_PORT, 0x20);								// check for second port
		ps2_wait_response();
		uint8_t config = inb(PS2_DATA_PORT);					// get config byte again
		if (!BITTEST(config, 5)) {
			ps2_dual_channel = true;
			outb(PS2_COMM_PORT, 0xA7);
		} else ps2_dual_channel = false;
	}

	// printf("[ps2kbd] check for second port ok\n");

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
		}
	}

	// printf("[ps2kbd] interface test ok\n");

	outb(PS2_COMM_PORT, 0xAE);
	if (ps2_dual_channel) outb(PS2_COMM_PORT, 0xA8);

	outb(PS2_COMM_PORT, 0x20);
	ps2_wait_response();
	config = inb(PS2_DATA_PORT) | 0x3;							// get config byte and patch it yet again 

	outb(PS2_COMM_PORT, 0x60);									// write it back
	ps2_wait_ready();
	outb(PS2_DATA_PORT, config);

	// printf("[ps2kbd] enable interrupts ok\n");

	ps2_wait_ready();
	outb(PS2_DATA_PORT, 0xFF); 									// keyboard should work now, i hope

	ps2_wait_ready();
	outb(PS2_DATA_PORT, 0xF0);

	ps2_wait_ready();
	outb(PS2_DATA_PORT, 0x01);


	printf("[ps2kbd] init done\n");


	// asm volatile ("sti");


#endif

	register_interrupt_handler(0x21, ps2_interrupt);

}

const char lower_normal[] = { '\0', '?', '1', '2', '3', '4', '5', '6',     
		'7', '8', '9', '0', '-', '=', '\b', '\t', 'q', 'w', 'e', 'r', 't', 'y', 
				'u', 'i', 'o', 'p', '[', ']', '\n', '\0', 'a', 's', 'd', 'f', 'g', 
				'h', 'j', 'k', 'l', ';', '\'', '`', '\0', '\\', 'z', 'x', 'c', 'v', 
				'b', 'n', 'm', ',', '.', '/', '\0', '\0', '\0', ' '};

const char upper_shift[] = { '\0', '?', '!', '@', '#', '$', '%', '^',     
		'&', '*', '(', ')', '_', '+', '\b', '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 
				'U', 'I', 'O', 'P', '{', '}', '\n', '\0', 'A', 'S', 'D', 'F', 'G', 
				'H', 'J', 'K', 'L', ':', '"', '~', '\0', '|', 'Z', 'X', 'C', 'V', 
				'B', 'N', 'M', '<', '>', '?', '\0', '\0', '\0', ' '};

const char upper_caps[] = { '\0', '?', '1', '2', '3', '4', '5', '6',     
		'7', '8', '9', '0', '-', '=', '\b', '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 
				'U', 'I', 'O', 'P', '[', ']', '\n', '\0', 'A', 'S', 'D', 'F', 'G', 
				'H', 'J', 'K', 'L', ';', '\'', '`', '\0', '\\', 'Z', 'X', 'C', 'V', 
				'B', 'N', 'M', ',', '.', '/', '\0', '\0', '\0', ' '};

const char lower_shift_caps[] = { '\0', '?', '!', '@', '#', '$', '%', '^',     
		'&', '*', '(', ')', '_', '+', '\b', '\t', 'q', 'w', 'e', 'r', 't', 'y', 
				'u', 'i', 'o', 'p', '{', '}', '\n', '\0', 'a', 's', 'd', 'f', 'g', 
				'h', 'j', 'k', 'l', ':', '"', '~', '\0', '|', 'z', 'x', 'c', 'v', 
				'b', 'n', 'm', '<', '>', '?', '\0', '\0', '\0', ' '};


bool ps2_interrupt(interrupt_cpu_state *state) {
	uint8_t sc = inb(0x60);
	
	if (sc == SC_RIGHT_SHIFT_REL || sc == SC_LEFT_SHIFT_REL)
		shift = false;
	else if (sc == SC_RIGHT_SHIFT || sc == SC_LEFT_SHIFT)
		shift = true;
	else if (sc == SC_CAPSLOCK)
		caps = !caps;
	else if (sc == SC_F12) {
		uint32_t pid = current_task->pid;
		tasking_schedule_next();
		kill_task(pid);
		tasking_schedule_after_kill();
	} else if (sc < SC_MAX) {
		
		char c = 0;
		
		if (caps && !shift)
			c = upper_caps[sc];
		else if (!caps && shift)
			c = upper_shift[sc];
		else if (caps && shift)
			c = lower_shift_caps[sc];
		else
			c = lower_normal[sc];
	
		if (ps2_keyboard_buffer_idx < 512) {
			ps2_keyboard_buffer[ps2_keyboard_buffer_idx++] = c;
		}
	}


	return true;
}

void ps2_kbd_reset_buffer() {

	uint8_t tmp = 0x01;
    while (inb(0x64) & 0x01)
        tmp = inb(0x60);

	ps2_keyboard_buffer_idx = 0;
	memset(ps2_keyboard_buffer, 0, 512);
}

char getch() {
	// wait for keyboard buffer to be not empty, then return the character and remove it from the buffer
	while (ps2_keyboard_buffer_idx == 0);
	while (ps2_keyboard_buffer[ps2_keyboard_buffer_idx - 1] == 0);
	return ps2_keyboard_buffer[--ps2_keyboard_buffer_idx];
}

char readch() {
	if (ps2_keyboard_buffer_idx == 0) return 0;
	return ps2_keyboard_buffer[--ps2_keyboard_buffer_idx];
}
