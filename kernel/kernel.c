#include <stddef.h>
#include <stdint.h>
#include <kmesg.h>
#include <initrd.h>
#include <arch/info.h>
#include <devmgr.h>
#include <kobj.h>

void kernel_main(arch_boot_info_t *info) {
	kmesg("kernel", "reached arch independent stage");

	kmesg("kernel", "info:");
	kmesg("kernel", "\tflags:");
	if (info->flags & ARCH_INFO_HAS_INITRAMFS) kmesg("kernel", "\t- initramfs");
	if (info->flags & ARCH_INFO_HAS_VIDEO_MODE) kmesg("kernel", "\t- video mode");

	if (info->flags & ARCH_INFO_HAS_INITRAMFS) {
		kmesg("kernel", "");
		kmesg("kernel", "\tinitramfs:");
		kmesg("kernel", "\t\taddr: %016p", info->initramfs);
		kmesg("kernel", "\t\tcmdline: %s", info->initramfs_cmd);
		kmesg("kernel", "\t\tsize: %lu bytes", info->initramfs_size);
		initrd_init(info->initramfs, info->initramfs_size);
	}

	if (info->flags & ARCH_INFO_HAS_VIDEO_MODE) {
		kmesg("kernel", "");
		kmesg("kernel", "\tvideo mode:");
		kmesg("kernel", "\t\tfb addr: %016lx", info->vid_mode->addr);
		kmesg("kernel", "\t\twidth: %u", info->vid_mode->width);
		kmesg("kernel", "\t\theight: %u", info->vid_mode->height);
		kmesg("kernel", "\t\tbpp: %u", info->vid_mode->bpp);
	}

	devmgr_init();
	arch_devmgr_fill_devices();
	devmgr_dump_devices();

	kmesg("kernel", "halting for now");

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
