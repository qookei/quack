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

int print(char *buffer) {
	int _val;
	size_t count = 0;
	while (buffer[count])
		count++;

	return write(1, buffer, count);
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
	print("Note v1.0.1!\n");

	char kb = 0;
	uint32_t pos = 0;

	for (int i = 0; i < 128; i++) input[i] = 0;
	
	while(1) {
		read(0, input, 128);
		print(input);
		for (int i = 0; i < 128; i++) input[i] = 0;

		while (kb != '\n') {
			if (kb != 0)  {
				if (kb != '\b') {
					input[pos++] = kb;
					write(1, &kb, 1);
				} else if (pos > 0) {
					pos--;
					input[pos] = 0;
					print("\b");
				}
				kb = 0;
			}
			read(0, &kb, 1);
		}
		kb = 0;
		print("\n");
		if (pos < 1)
			continue;
		pos = 0;

	}
}

