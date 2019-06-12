#include <stdint.h>
#include <stdarg.h>
#include <io/port.h>
#include <multiboot.h>
#include <irq/idt.h>
#include <irq/isr.h>
#include <irq/pic/pic.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <mm/mm.h>
#include <acpi/acpi.h>
#include <io/vga.h>
#include <cpu/lapic.h>
#include <cpu/ioapic.h>
#include <cpu/cpu_data.h>

#include <kmesg.h>
#include <util.h>
#include <mm/heap.h>
#include <cmdline.h>

int ps2_kbd(irq_cpu_state_t *s) {
	(void)s;
	uint8_t b = inb(0x60);
	kmesg("ps2", "keyboard %02x", b);
	return 1;
}

void arch_entry(multiboot_info_t *mboot, uint32_t magic) {
	vga_init();

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

	cmdline_init((void *)(VIRT_PHYS_BASE + mboot->cmdline));

	acpi_init();
	cpu_data_init();
	lapic_init();
	//ioapic_init();

	kmesg("kernel", "done initializing, enabling interrupts and stopping");
	ioapic_init_isos(0, 0);

	asm volatile ("sti");

	lapic_timer_calc_freq();

	isr_register_handler(0x41, ps2_kbd);

	while(1);
}
