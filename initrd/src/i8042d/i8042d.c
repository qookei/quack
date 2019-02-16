/*
 * quack i8042 driver/daemon
 * */

#include <stdint.h>
#include <stddef.h>

#include "../sys/syscall.h"

#define bittest(var,pos) ((var) & (1 << (pos)))
#define ps2_wait_ready() {while(bittest(inb(0x64), 1));}
#define ps2_wait_response() {while(bittest(inb(0x64), 0));}

void outb(uint16_t port, uint8_t val) {
	asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

uint8_t inb(uint16_t port) {
	uint8_t ret;
	asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

const char lower_normal[] = { '\0', '?', '1', '2', '3', '4', '5', '6',
		'7', '8', '9', '0', '-', '=', '\b', '\t', 'q', 'w', 'e', 'r', 't', 'y',
		'u', 'i', 'o', 'p', '[', ']', '\n', '\0', 'a', 's', 'd', 'f', 'g',
		'h', 'j', 'k', 'l', ';', '\'', '`', '\0', '\\', 'z', 'x', 'c', 'v',
		'b', 'n', 'm', ',', '.', '/', '\0', '\0', '\0', ' '};

void _start(void) {
	char num_buf[2] = {0, 0};

	sys_debug_log("i8042d: starting\n");

	sys_register_handler(0x21);

	int s = 0;

	sys_enable_ports(0x60, 1);
	sys_enable_ports(0x64, 1);

	while (inb(0x64) & 0x01)
		(void)inb(0x60);

	sys_debug_log("i8042d: entering irq wait loop\n");

	sys_map_to(sys_getpid(), 0xB8000, 0xB8000); // map VGA text mode for testing

	int i = 0;

	while(1) {
		int i = sys_wait(WAIT_IRQ | WAIT_IPC, 0, NULL, NULL);
		if (i == WAIT_IRQ) {
			uint8_t b = inb(0x60);
			sys_debug_log("i8042d: putc here!\n");
		} else {
			sys_debug_log("i8042d: parse ipc message here!\n");
			sys_ipc_remove();
		}
	}

	sys_debug_log("i8042d: exiting\n");
	sys_exit(0);
}
