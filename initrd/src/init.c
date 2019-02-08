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

uint8_t test[] = {
  0x66, 0xba, 0xf8, 0x03, 0xb0, 0x74, 0xee, 0xb0, 0x65, 0xee, 0xb0, 0x73,
  0xee, 0xb0, 0x74, 0xee, 0xb0, 0x3a, 0xee, 0xb0, 0x20, 0xee, 0xb0, 0x68,
  0xee, 0xb0, 0x69, 0xee, 0xb0, 0x21, 0xee, 0xb0, 0x0a, 0xee, 0xb0, 0x0d,
  0xee, 0x31, 0xc0, 0x31, 0xdb, 0xcd, 0x30
};

size_t test_len = 43;

void _start(void) {
	serial_puts("init: welcome to quack");

	int32_t pid = sys_spawn_new(sys_getpid(), 1);
	uintptr_t stack = sys_alloc_phys();
	uintptr_t data = sys_alloc_phys();
	sys_map_to(pid, 0xA0000000, stack);
	sys_map_to(pid, 0x00001000, data);
	sys_map_to(sys_getpid(), 0x00001000, data);
	
	for (size_t i = 0; i < test_len; i++) {
		*((uint8_t *)(0x00001000 + i)) = test[i];
	}

	sys_unmap_from(sys_getpid(), 0x00001000);
	sys_make_ready(pid, 0x00001000, 0xA0000000);

	serial_puts("init: exiting");
	sys_exit(0);
}
