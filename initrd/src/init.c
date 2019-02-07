/*
 * quack init server
 * */

#include <stdint.h>
#include <stddef.h>

#include "sys/syscall.h"

#define PORT 0x3F8

void outb(uint16_t port, uint8_t val) {
	asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

uint8_t inb(uint16_t port) {
	uint8_t val;
	asm volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
	return val;
}

void serial_write(uint8_t byte) {
	if (byte == '\n') serial_write('\r');
	while((inb(PORT + 5) & 0x20) == 0);
	outb(PORT, byte);
}

void serial_puts(const char *s) {
	while(*s)
		serial_write(*s++);

	serial_write('\n');
}

void _start(void) {
	serial_puts("init: welcome to quack");

	serial_puts("init: sending ipc message to self");
	sys_ipc_send(0, 5, "aaaa");
	serial_puts("init: queue length is equal to");
	serial_write('0' + sys_ipc_queue_length());
	serial_write('\n');

	serial_puts("init: reading ipc queue top");
	char buf[5];
	sys_ipc_recv(buf);

	serial_puts("init: ipc message contents:");
	serial_puts(buf);

	serial_puts("init: removing top ipc message from queue");
	sys_ipc_remove();

	serial_puts("init: exiting");
	sys_exit(0);
}
