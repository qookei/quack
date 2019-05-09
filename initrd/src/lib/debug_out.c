#include "debug_out.h"
#include "../sys/syscall.h"

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

void debugf(const char *fmt, ...) {
	va_list arg;
	va_start(arg, fmt);

	static char buf[512];
	uvsprintf(buf, fmt, arg);
	sys_debug_log(buf);

	va_end(arg);
}

void debug_hex_dump(uint8_t *data, size_t length) {
	size_t lines = (length + 7) / 8;
	for (size_t i = 0; i < lines; i++) {
		size_t bytes = (length - i * 8 >= 8) ? 8 : (length - i * 8);
		
		debugf("%8x: ", i * 8);
		
		for (size_t j = 0; j < bytes; j++) {
			debugf("%2x ", data[i * 8 + j]);
		}

		for (size_t j = 0; j < 8 - bytes; j++) {
			debugf("   ");
		}

		debugf("| ");

		for (size_t j = 0; j < bytes; j++) {
			debugf("%c", isprint(data[i * 8 + j]) ? data[i * 8 + j] : '.');
		}

		debugf("\n");
	}
}
