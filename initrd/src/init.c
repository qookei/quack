/*
 * quack init server
 * */

#include <stdint.h>
#include <stddef.h>

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
	serial_puts("init: nothing to do yet, halting");

	// halt, nothing to do here
	while(1) {
	}
}
