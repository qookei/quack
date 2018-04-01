#include <stdint.h>
#include <stddef.h>

int read(int handle, void *buffer, size_t count) {
	int _val;
	asm ("int $0x30" : "=a"(_val) : "a"(5), "b"(handle), "c"(buffer), "d"(count));
	return _val;
}

int write(int handle, void *buffer, size_t count) {
	int _val;
	asm ("int $0x30" : "=a"(_val) : "a"(6), "b"(handle), "c"(buffer), "d"(count));
	return _val;
}

size_t strlen(const char *s) {
	size_t len = 0;
	while(s[len]) {
		len++;
	}
	return len;
}

char input[128];

void _start(void) {

	write(1, "Note v1.0!\n", 11);
	
	for (int i = 0; i < 128; i++) input[i] = 0;
	
	while(1) {
		read(0, input, 128);
		write(1, input, strlen(input));
		for (int i = 0; i < 128; i++) input[i] = 0;
	}
}