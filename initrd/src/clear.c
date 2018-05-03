#include <stdint.h>
#include <stddef.h>

int print(char *buffer) {
	int _val;
	size_t count = 0;
	while (buffer[count])
		count++;
	asm ("int $0x30" : "=a"(_val) : "a"(6), "b"(1), "c"(buffer), "d"(count));
	return _val;
}

void exit() {
	asm ("int $0x30" : : "a"(0));
}

void _start(void) {
	print("\ec");
	exit();
}

