#include <stdint.h>
#include <stdarg.h>
#include <io/port.h>
#include <multiboot.h>
#include <irq/idt.h>
#include <irq/isr.h>
#include <pic/pic.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <mm/mm.h>
#include <kmesg.h>

#include <util.h>

int ps2_kbd_handler(irq_cpu_state_t *state) {
	(void)state;

	uint8_t scancode = inb(0x60);
	kmesg("kbd", "%x ", scancode);
}

void arch_entry(multiboot_info_t *mboot, uint32_t magic) {
	if (magic != 0x2BADB002) {
		kmesg("kernel", "signature %x does not match 0x2BADB002", magic);
	}

	mboot = (multiboot_info_t *)((uintptr_t)mboot + VIRT_PHYS_BASE);

	kmesg("kernel", "welcome to quack for x86_64");

	idt_init();
	pic_remap(0x20, 0x28);

	uintptr_t ptr = mboot->mmap_addr + VIRT_PHYS_BASE;

	pmm_init((multiboot_memory_map_t *)ptr, mboot->mmap_length / sizeof(multiboot_memory_map_t));

	vmm_init();

	assert(1 == 2);

	isr_register_handler(0x21, ps2_kbd_handler);

	asm volatile("sti");

	while(1);
}
