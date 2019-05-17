#include <stdint.h>
#include <stdarg.h>
#include <io/port.h>
#include <io/debug.h>
#include <multiboot.h>
#include <irq/idt.h>

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

void logf(const char *fmt, ...) {
	va_list arg;
	va_start(arg, fmt);

	static char buf[512];
	uvsprintf(buf, fmt, arg);
	debug_putstr(buf);

	va_end(arg);
}

void arch_entry(void *mboot, uint32_t magic) {
	if (magic != 0x2BADB002) {
		logf("quack: signature %x does not match 0x2BADB002\n", magic);
	}

	logf("quack: welcome to the x86_64 land\n");

	logf("quack: multiboot header is at %x\n", (uint32_t)mboot);

	multiboot_info_t *mbt = mboot;

	const char *bootloader = (const char *)mbt->boot_loader_name;
	logf("quack: booted by %s\n", bootloader);

	size_t mmap_ents = mbt->mmap_length / sizeof(multiboot_memory_map_t);
	multiboot_memory_map_t *map = (multiboot_memory_map_t *)mbt->mmap_addr;

	for (size_t i = 0 ; i < mmap_ents; i++) {
		uint32_t start = map[i].addr;
		uint32_t end = start + map[i].len;
		uint32_t size = map[i].len;
		uint32_t type = map[i].type;

		const char *types[] = {
			"??",
			"Available",
			"Reserved",
			"ACPI Reclaimable",
			"NVS",
			"Bad RAM"
		};

		logf("%8x - %8x -- %u -- %s\n", start, end, size, types[type]);
	}

	idt_init();

	asm volatile("sti");

	while(1);
}
