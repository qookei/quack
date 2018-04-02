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

int waitpid(int pid) {
	int _val;
	asm ("int $0x30" : "=a"(_val) : "a"(3), "b"(pid));
	return _val;	
}

void exit() {
	asm ("int $0x30" : : "a"(0));
}

int close(int handle) {
	int _val;
	asm ("int $0x30" : "=a"(_val) : "a"(7), "b"(handle));
	return _val;
}


size_t strlen(const char *s) {
	size_t len = 0;
	while(s[len]) {
		len++;
	}
	return len;
}

int itoa(int value, char *sp, int radix) {
    char tmp[16];
    char *tp = tmp;
    int i;
    unsigned v;

    int sign = (radix == 10 && value < 0);    
    if (sign)
        v = -value;
    else
        v = (unsigned)value;

    while (v || tp == tmp) {
        i = v % radix;
        v /= radix; // v/=radix uses less CPU clocks than v=v/radix does
        if (i < 10)
          *tp++ = i+'0';
        else
          *tp++ = i + 'a' - 10;
    }

    int len = tp - tmp;

    if (sign) {
        *sp++ = '-';
        len++;
    }

    while (tp > tmp)
        *sp++ = *--tp;

    return len;
}


char input[128];

void _start(void) {

	write(1, "sh v1.0\n", 8);
	
	char kb = 0;
	uint32_t pos = 0;

	while(1) {
		write(1, "> ", 2);
		for(int i = 0; i < 128; i++) input[i] = 0;
		while(kb != '\n') {
			if (kb != 0)  {
				if (kb != '\b') {
					input[pos++] = kb;
				} else {
					input[pos] = 0;
					pos--;
				}
				write(1, &kb, 1);
				kb = 0;
			}
			read(0, &kb, 1);
		}
		kb = 0;
		write(1, "\n", 1);
		pos = 0;

		int s = open(input, 0);
		if (s < 0) {
			write(1, "File not found!\n", 17);
		} else {
			close(s);
			int f = fork();
			if (!f) {
				int i = execve(input);
				if (i < 0)
					write(1, "Failed to exec!\n", 16);
				exit();
			} else {
				waitpid(f);
			}
		}
	}
}