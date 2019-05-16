#include <stdint.h>
#include <stdarg.h>

static char *itoa(uint32_t i, int base, int padding) {
	static char buf[50];
	char *ptr = buf + 49;
	*ptr = '\0';

	do {
		*--ptr = "0123456789ABCDEF"[i % base];
		if (padding)
			padding--;
	} while ((i /= base) != 0);

	while (padding) {
		*--ptr = '0';
		padding--;
	}

	return ptr;
}

void uvsprintf(char *buf, const char *fmt, va_list arg) {
	uint32_t i;
	char *s;

	while(*fmt) {
		if (*fmt != '%') {
			*buf++ = *fmt;
			fmt++;
			continue;
		}

		fmt++;
		int padding = 0;
		if (*fmt >= '0' && *fmt <= '9')
			padding = *fmt++ - '0';

		switch (*fmt) {
			case 'c': {
				i = va_arg(arg, int);
				*buf++ = i;
				break;
			}

			case 'u': {
				i = va_arg(arg, int);
				char *c = itoa(i, 10, padding);
				while (*c)
					*buf++ = *c++;
				break;
			}

			case 'x': {
				i = va_arg(arg, int);
				char *c = itoa(i, 16, padding);
				while (*c)
					*buf++ = *c++;
				break;
			}

			case 's': {
				s = va_arg(arg, char *);
				while (*s)
					*buf++ = *s++;
				break;
			}

			case '%': {
				*buf++ = '%';
				break;
			}
		}

		fmt++;
	}

	*buf++ = '\0';
}

void uprintf(void (*write)(char *), const char *fmt, ...) {
	va_list arg;
	va_start(arg, fmt);

	static char buf[512];
	uvsprintf(buf, fmt, arg);
	write(buf);

	va_end(arg);
}

void usprintf(char *buf, const char *fmt, ...) {
	va_list arg;
	va_start(arg, fmt);

	uvsprintf(buf, fmt, arg);

	va_end(arg);
}

void outb(uint16_t port, uint8_t val) {
	asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void write_debug(char c) {
	outb(0xE9, c);
}

void write_debug_string(const char *c) {
	while (*c) {
		write_debug(*c);
		c++;
	}
}

void logf(const char *fmt, ...) {
	va_list arg;
	va_start(arg, fmt);

	static char buf[512];
	uvsprintf(buf, fmt, arg);
	write_debug_string(buf);

	va_end(arg);
}

void arch_entry(void *mboot, uint32_t magic) {

	if (magic != 0x2BADB002) {
		logf("quack: signature %x does not match 0x2BADB002\n", magic);
	}
	logf("quack: welcome to the x86_64 land\n");

	

	while(1);
}
