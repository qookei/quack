#include <stdint.h>
#include <stddef.h>

void *sbrk(int increment) {
	uint32_t _val;
	asm ("int $0x30" : "=a"(_val) : "a"(17), "b"(increment));
	return (void *)_val;
}

void exit() {
	asm ("int $0x30" : : "a"(0));
}

int write(int handle, void *buffer, size_t count) {
	int _val;
	asm ("int $0x30" : "=a"(_val) : "a"(6), "b"(handle), "c"(buffer), "d"(count));
	return _val;
}

int print(char *buffer) {
	int _val;
	size_t count = 0;
	while (buffer[count])
		count++;

	return write(1, buffer, count);
}

void memcpy(void *dst, const void *src, size_t size) {
	for (size_t i = 0; i < size; i++) {
		*((char *)dst++) = *((char *)src++);
	}
}

void memset(void *dst, int data, size_t size) {
	for (size_t i = 0; i < size; i++) {
		*((char *)dst++) = data;
	}
}

void _start(void) {
	write(1, "sbrk test!\n", 11);
	
	void *c = sbrk(0x1000);
	memset(c, 'A', 0x1000);
	memcpy(c, "Hello!\n", 8);
	write(1, c, 24);
	
	exit();
}
