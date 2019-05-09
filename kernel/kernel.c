#include <stddef.h>
#include <stdint.h>
/*#include <io/ports.h>
#include <pic/pic.h>
*/
#include <multiboot.h>
/*
#include <kheap/heap.h>
#include <sched/sched.h>
#include <sched/elf.h>*/
#include <kmesg.h>
/*#include <initrd.h>
#include <syscall/syscall.h>*/

void kernel_main(multiboot_info_t *mboot) {

	kmesg("kernel", "reached arch independent stage, halting for now");

	/*void *init_file;
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
	}*/

	//global_data_setup();

	//syscall_init();
	//sched_init();

	// TODO: perhaps parse a config file to see what to load

	//elf_create_proc(init_file, 1);
	//elf_create_proc(exec_file, 1);
	//elf_create_proc(initfs_file, 1);

	//initrd_t i = initrd_get_info();
	//task_ipcsend(sched_get_task(3), sched_get_task(3), i.size, i.data);

	//task_ipcsend(sched_get_task(1), sched_get_task(1), sizeof(multiboot_info_t), mboot);

	//asm volatile ("sti");

	while(1);
}
