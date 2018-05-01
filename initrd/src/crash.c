#include <stdint.h>
#include <stddef.h>

int open(const char *name, uint32_t flags) {
	int _val;
	asm ("int $0x30" : "=a"(_val) : "a"(4), "b"(name), "c"(flags));
	return _val;
}

int write(int handle, void *buffer, size_t count) {
	int _val;
	asm ("int $0x30" : "=a"(_val) : "a"(6), "b"(handle), "c"(buffer), "d"(count));
	return _val;
}

int exit() {
	asm ("int $0x30" : : "a"(0));
}

void _start(void) {
	int i = open("/dev/initrd", 0);
	write(i, (void *)0xC0000000, 41984);
	exit();
}
