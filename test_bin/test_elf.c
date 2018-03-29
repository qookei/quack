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

int execve(const char *path) {
	int _val;
	asm ("int $0x30" : "=a"(_val) : "a"(2), "b"(path));
	return _val;
}

int fork() {
	int _val;
	asm ("mov $1, %%eax; int $0x30; mov %%eax, %0" : "=g"(_val));
	return _val;
}

void exit() {
	asm ("int $0x30" : : "a"(0));
}

size_t strlen(const char *s) {
	size_t len = 0;
	while(s[len]) {
		len++;
	}
	return len;
}

char *msg1 = "Usermode notepad v3.0!\n";
char *msg2 = "This is C code running in userspace!\n";


void _start(void) {
	write(1, msg1, strlen(msg1));
	write(1, msg2, strlen(msg2));

	char buffer[16];
	int d = open("/dev/mouse", 0);

	int u = fork();
	if (!u) {
		execve("/bin/test2");
	} else {
		while(1) {
			int i = read(d, buffer, 16);
			if (i > 0) {
				if(buffer[8] & (1<<2))
					write(1, "Button press\n", 13);
			}
		}
	}

}