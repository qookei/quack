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

int open(const char *name, uint32_t flags) {
	int _val;
	asm ("int $0x30" : "=a"(_val) : "a"(4), "b"(name), "c"(flags));
	return _val;
}

int print(char *buffer) {
	int _val;
	size_t count = 0;
	while (buffer[count])
		count++;

	return write(1, buffer, count);
}

int close(int handle) {
	int _val;
	asm ("int $0x30" : "=a"(_val) : "a"(7), "b"(handle));
	return _val;
}

int read(int handle, void *buffer, size_t count) {
	int _val;
	asm ("int $0x30" : "=a"(_val) : "a"(5), "b"(handle), "c"(buffer), "d"(count));
	return _val;
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

char buf[100];

void _start(void) {

	uint32_t uptime;

	int i = open("/dev/uptime", 0);
	read(i, &uptime, 4);
	close(i);

	int s = itoa(uptime, buf, 10);

	write(1, "\e[1m", 4);
	write(1, "\e[33m", 5);
	write(1, "            ..        OS: Quack\n", 32);
	write(1, "           ( '`<      Kernel: i686 Quack\n", 41);
	write(1, "            )(        Shell: Quack Bash\n", 40);
	//print(buf);
	//print("s\n");
	write(1, "     ( ----'  '.      DE: Pond 1.0\n", 35);
	write(1, "     (         ;\n", 17);
	write(1, "      (_______,'\n", 17);
	write(1, "\e[0m", 4);
	write(1, "\e[34m", 5);
	write(1, " ~^~^~^~^~^~^~^~^~^~^~\n", 23);
	write(1, "\e[37m", 5);
	exit();
}

