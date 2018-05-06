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

int itoa(int value, char *sp, int radix) {
	char tmp[16], *tp = tmp;
	unsigned v;
	int i, sign = (radix == 10 && value < 0);

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

char input[128], path[1024], comm[1024];

void _start(void) {

	print("sh v1.1\n");
	
	char kb = 0;
	uint32_t pos = 0;

	while(1) {
		getwd(path);
		print(path);
		print("> ");
		for (int i = 0; i < 128; i++) {
			input[i] = 0;
			comm[i] = 0;
		}
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

		for (int i = 0; i < 1024; i++) {
			if (input[i] == ' ' || input[i] == 0)
				break;
			comm[i] = input[i];
		}

		if (comm[0] == 'c' && comm[1] == 'd') {
			// chdir
			int i = chdir(input + 3);
			if (i == -5)
				print("No such file or directory\n");
			else if (i == -4)
				print("Name too long\n");
			else if (i == -6)
				print("Not a directory\n");
			continue;
		} else if (comm[0] == 'p' && comm[1] == 'w' && comm[2] == 'd') {
			print(path);
			print("\n");
			continue;
		} else if (comm[0] == 'e' && comm[1] == 'c' && comm[2] == 'h' && comm[3] == 'o') {
			print(input + 5);
			print("\n");
			continue;
		} else if (comm[0] == '#')
			continue;

		int s = open(comm, 0);
		if (s < 0) {
			char buf[32];
			itoa(s, buf, 10);
			print("sh: ");
			if (s == -5) {
				print(comm);
				print(": command not found\n");
			} else if (s == -10) {
				print(comm);
				print(": Is a directory\n");
			} else {
				print("Error: ");
				print(buf);
				print("\n");
			}
		} else {
			close(s);
			int f = fork();
			if (!f) {
				int i = execve(comm);
				if (i < 0)
					print("Failed to exec!\n");
				exit();
			} else
				waitpid(f);
		}
	}
}

