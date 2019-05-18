#include <stdint.h>
#include <stdarg.h>
#include <io/port.h>
#include <io/debug.h>
#include <multiboot.h>
#include <irq/idt.h>
#include <irq/isr.h>
#include <pic/pic.h>
#include <mm/pmm.h>
#include <mm/mm.h>

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

int ps2_kbd_handler(irq_cpu_state_t *state) {
	(void)state;

	uint8_t scancode = inb(0x60);
	logf("%x ", scancode);
}

void arch_entry(multiboot_info_t *mboot, uint32_t magic) {
	if (magic != 0x2BADB002) {
		logf("quack: signature %x does not match 0x2BADB002\n", magic);
	}

	mboot = (multiboot_info_t *)((uintptr_t)mboot + VIRT_PHYS_BASE);

	logf("quack: welcome to quack for x86_64\n");

	idt_init();
	pic_remap(0x20, 0x28);

	uintptr_t ptr = mboot->mmap_addr + VIRT_PHYS_BASE;

	pmm_init((multiboot_memory_map_t *)ptr, mboot->mmap_length / sizeof(multiboot_memory_map_t));

	isr_register_handler(0x21, ps2_kbd_handler);

	asm volatile("sti");

	while(1);
}
