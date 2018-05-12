#include <stdint.h>
#include <stddef.h>

int read(int handle, void *buffer, size_t count) {
	int _val;
	asm ("int $0x30" : "=a"(_val) : "a"(5), "b"(handle), "c"(buffer), "d"(count));
	return _val;
}

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

int print(char *buffer) {
	int _val;
	size_t count = 0;
	while (buffer[count])
		count++;
	return write(1, buffer, count);
}

void exit() {
	asm ("int $0x30" : : "a"(0));
}

int close(int handle) {
	int _val;
	asm ("int $0x30" : "=a"(_val) : "a"(7), "b"(handle));
	return _val;
}

char input[1024];

void _start(void) {
	/*do stuff xd*/
	print("Dump utility\nEnter a filename: ");

	char kb = 0;
	uint32_t pos = 0;
	
	for (int i = 0; i < 1024; i++) input[i] = 0;

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
	pos = 0;

	int s = open(input, 0);
	if (s < 0)
		print("\nFile not found!\n");
	else {
		int sz = read(s, input, 1024);
		write(1, input, sz);
		close(s);
	}

	exit();
}
