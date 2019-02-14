#include <stddef.h>
#include <stdint.h>
#include <io/ports.h>
#include <interrupt/idt.h>
#include <interrupt/isr.h>
#include <pic/pic.h>
#include <multiboot.h>
#include <paging/pmm.h>
#include <paging/paging.h>
#include <kheap/heap.h>
#include <sched/sched.h>
#include <sched/elf.h>
#include <mesg.h>
#include <fs/ustar.h>
#include <syscall/syscall.h>

void pit_freq(uint32_t frequency) {
	uint16_t x = 1193182 / frequency;
	if ((1193182 % frequency) > (frequency / 2))
		x++;

	outb(0x40, (uint8_t)(x & 0x00ff));
	outb(0x40, (uint8_t)(x / 0x0100));
}

void gdt_new_setup(void);
void gdt_set_tss_stack(uint32_t);
void gdt_ltr(void);

// TODO: move this to a different file
// together with ustar code prehaps 
void *kernel_copy_initrd(multiboot_info_t *mboot, size_t *isize) {
	multiboot_module_t* p = (multiboot_module_t*) (0xC0000000 + mboot->mods_addr);

	uintptr_t start_addr = p->mod_start;
	uintptr_t end_addr = p->mod_end - 1;
	size_t size = end_addr - start_addr + 1;

	size_t page_size = (size + 0xFFF) / 0x1000 * 0x1000;

	*isize = size;

	for (size_t i = 0; i <= page_size; i += 0x1000) {
		map_page((void*)((start_addr & 0xFFFFF000) + i), (void*)(0xE0000000 + i), 0x3);
	}

	void *initrd = kmalloc(size);

	memcpy(initrd, (void *)0xE0000000, size);

	for (size_t i = 0; i <= page_size; i += 0x1000) {
		unmap_page((void*)(0xE0000000 + i));
	}

	return initrd;
}

extern void *isr_stack;

void kernel_main(multiboot_info_t *mboot) {
	if (((uintptr_t)mboot) - 0xC0000000 > 0x800000) {
		early_mesg(LEVEL_ERR, "mboot", "hdr out of page");
		return;
	}

	gdt_new_setup();
	early_mesg(LEVEL_INFO, "cpu", "GDT ok");

	pic_remap(0x20, 0x28);
	idt_init();
	pit_freq(1000);
	early_mesg(LEVEL_INFO, "cpu", "interrupts ok");

	pmm_init(mboot);
	paging_init();

	size_t initrd_sz;
	void *initrd = kernel_copy_initrd(mboot, &initrd_sz);
	void *init_file;

	if (!ustar_read(initrd, initrd_sz, "init", &init_file)) {
		early_mesg(LEVEL_ERR, "kernel", "failed to load init");
		while(1);
	}

	syscall_init();

	sched_init();

	elf_create_proc(init_file, 1);

	void *exec_file;
	size_t exec_size = ustar_read(initrd, initrd_sz, "exec", &exec_file);
	task_ipcsend(sched_get_task(1), sched_get_task(1), exec_size, exec_file);

	void *test_irq_file;
	size_t test_irq_size = ustar_read(initrd, initrd_sz, "test-irq", &test_irq_file);
	task_ipcsend(sched_get_task(1), sched_get_task(1), test_irq_size, test_irq_file);

	void *vgatty_file;
	size_t vgatty_size = ustar_read(initrd, initrd_sz, "vgatty", &vgatty_file);
	task_ipcsend(sched_get_task(1), sched_get_task(1), vgatty_size, vgatty_file);

	gdt_set_tss_stack((uintptr_t)(&isr_stack));
	gdt_ltr();

	asm volatile ("sti");

	while(1);
}
