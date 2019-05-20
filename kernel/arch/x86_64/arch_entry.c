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
#include <vsprintf.h>

void logf(const char *fmt, ...) {
	va_list arg;
	va_start(arg, fmt);

	static char buf[512];
	vsprintf(buf, fmt, arg);
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

	logf("vsprintf test:\n");
	logf("%%x, 0xDEADBEEF == %x\n", 0xDEADBEEF);
	logf("%%lx, 0xDEADBEEFCAFEBABE == %lx\n", 0xDEADBEEFCAFEBABE);
	logf("%%u, 1337 == %u\n", 1337);
	logf("%%d, -15 == %d\n", -15);
	logf("%%o, 0755 == %o\n", 0755);
	logf("%%c, 'a' == '%c'\n", 'a');
	logf("%%8x, 0xDEAD == %8x\n", 0xDEAD);
	logf("%%16lx, 0xDEADBEEFED == %16lx\n", 0xDEADBEEFED);
	logf("%%08x, 0xDEAD == %08x\n", 0xDEAD);
	logf("%%016lx, 0xDEADBEEFED == %016lx\n", 0xDEADBEEFED);
	logf("%%X, 0xDEADBEEF == %X\n", 0xDEADBEEF);
	logf("%%p, (void *)0xCAFEBABE == %p\n", (void *)0xCAFEBABE);
	logf("%%P, (void *)0xFFFF800000000000 == %P\n", (void *)0xFFFF800000000000);

	isr_register_handler(0x21, ps2_kbd_handler);

	asm volatile("sti");

	while(1);
}
