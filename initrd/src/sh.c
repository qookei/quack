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

int chdir(const char *path) {
	int _val;
	asm ("int $0x30" : "=a"(_val) : "a"(11), "b"(path));
	return _val;
}

int getwd(char *buf) {
	int _val;
	asm ("int $0x30" : "=a"(_val) : "a"(12), "b"(buf));
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
char path[1024] __attribute__((aligned(4096)));

void _start(void) {

	write(1, "sh v1.0\n", 8);
	
	char kb = 0;
	uint32_t pos = 0;

	while(1) {
		getwd(path);
		write(1, path, strlen(path));
		write(1, "> ", 2);
		for(int i = 0; i < 128; i++) input[i] = 0;
		while(kb != '\n') {
			if (kb != 0)  {
				if (kb != '\b') {
					input[pos++] = kb;
					write(1, &kb, 1);
				} else if (pos > 0) {
					input[pos] = 0;
					pos--;
					write(1, "\b", 1);
				}
				kb = 0;
			}
			read(0, &kb, 1);
		}
		kb = 0;
		write(1, "\n", 1);
		if (pos < 1)
			continue;
		pos = 0;

		if (input[0] == 'c' && input[1] == 'd' && input[2] == ' ') {
			// chdir
			int i = chdir(input + 3);
			if (i == -5)
				write(1, "No such file or directory\n", 26);
			else if (i == -4)
				write(1, "Name too long\n", 14);
			else if (i == -6)
				write(1, "Not a directory\n", 16);
			continue;
		}

		int s = open(input, 0);
		if (s < 0) {
			write(1, "File not found!\n", 16);
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