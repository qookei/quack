#include <stdint.h>
#include <stddef.h>

typedef struct {
	char name[128];
} dirent_t;

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

int get_ents(const char *path, dirent_t *result, size_t count) {
	int _val;
	asm ("int $0x30" : "=a"(_val) : "a"(15), "b"(path), "c"(result), "d"(count));
	return _val;
}

int getwd(char *buf) {
	int _val;
	asm ("int $0x30" : "=a"(_val) : "a"(12), "b"(buf));
	return _val;
}

void exit() {
	asm ("int $0x30" : : "a"(0));
}

size_t strlen(const char *s) {
	size_t n = 0;
	while (s[n]) n++;
	return n;
}

char path[1024];

void _start(void) {
	getwd(path);
	int n = get_ents(path, NULL, 0);
	
	if (n > 0) {
		dirent_t ents[n];
		get_ents(path, ents, n);
		for (int i = 0; i < n; i++) {
			print(ents[i].name);
			print(" ");
		}
		print("\n");
	} else {
		write(1, "Error occured!\n", 16); 
	}

	exit();
}

