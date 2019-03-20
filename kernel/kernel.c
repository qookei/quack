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
#include <kmesg.h>
#include <initrd.h>
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

extern void *isr_stack;

void kernel_main(multiboot_info_t *mboot) {
	if (((uintptr_t)mboot) - 0xC0000000 > 0x800000) {
		kmesg("mboot", "hdr out of page");
		return;
	}

	gdt_new_setup();
	kmesg("cpu", "GDT ok");

	pic_remap(0x20, 0x28);
	idt_init();
	pit_freq(1000);
	kmesg("cpu", "interrupts ok");

	pmm_init(mboot);
	paging_init();

	void *init_file;
	void *exec_file;
	void *initfs_file;

	if (!mboot->mods_count) {
		kmesg("kernel", "no initrd present... halting");
		while(1);
	}

	initrd_init((multiboot_module_t*)(0xC0000000 + mboot->mods_addr));

	if (!initrd_read_file("init", &init_file)) {
		kmesg("kernel", "failed to load init... halting");
		while(1);
	}

	if (!initrd_read_file("exec", &exec_file)) {
		kmesg("kernel", "failed to load exec... halting");
		while(1);
	}

	if (!initrd_read_file("initfs", &initfs_file)) {
		kmesg("kernel", "failed to load initfs... halting");
		while(1);
	}

	global_data_setup();

	syscall_init();
	sched_init();

	elf_create_proc(init_file, 1);
	elf_create_proc(exec_file, 1);
	elf_create_proc(initfs_file, 1);

	initrd_t i = initrd_get_info();
	task_ipcsend(sched_get_task(3), sched_get_task(3), i.size, i.data);

	gdt_set_tss_stack((uintptr_t)&isr_stack + 0x4000);
	gdt_ltr();

	asm volatile ("sti");

	while(1);
}
