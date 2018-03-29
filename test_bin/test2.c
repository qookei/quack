#include <stdint.h>
#include <stddef.h>

int write(int handle, void *buffer, size_t count) {
	int _val;
	asm ("int $0x30" : "=a"(_val) : "a"(6), "b"(handle), "c"(buffer), "d"(count));
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

char *msg1 = "Second process that was loaded via execve!\n";
char *msg2 = "The first process forked and the child called execve\nso here I am!";


void _start(void) {
	write(1, msg1, strlen(msg1));
	write(1, msg2, strlen(msg2));

	exit();
}