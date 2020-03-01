#include <stddef.h>
#include <stdint.h>
#include <kmesg.h>
#include <initrd.h>
#include <arch/info.h>
#include <arch/task.h>
#include <arch/mm.h>
#include <arch/cpu.h>
#include <panic.h>
#include <loader/elf64.h>
#include <proc/scheduler.h>
#include <mm/vm.h>

// TODO: use elf64_xxx and elf32_xxx depending on arch
static arch_task_t *load_elf_task(void *file) {
	int err;
	if ((err = elf64_check(file)))
		panic(NULL, "not a valid elf file: %d", err);

	return elf64_create_arch_task(file);
}

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

	if (!(info->flags & ARCH_INFO_HAS_INITRAMFS))
		panic(NULL, "missing initramfs");

	// TODO: parse a config file to see what to load

	// mm/vm.h test
	address_space s;
	s.create();

	uintptr_t addr1 = s.allocate_eager(4, vm_perm::urwx);
	kmesg("vm-test", "eagerly allocated 4 pages at 0x%016lx", addr1);

	uintptr_t addr2 = s.allocate_lazy(4, vm_perm::urwx);
	kmesg("vm-test", "lazily allocated 4 pages at 0x%016lx", addr2);

	uintptr_t addr3 = s.map(0xB8000, 1, vm_perm::urwx);
	kmesg("vm-test", "mapped 1 page from 0xB8000 to 0x%016lx", addr3);


	uintptr_t addr4 = s.allocate_exact_eager(0xDEAD0000, 4, vm_perm::urwx);
	kmesg("vm-test", "eagerly allocated 4 pages at 0x%016lx (specified explicitly)", addr4);

	uintptr_t addr5 = s.allocate_exact_lazy(0xCAFE0000, 4, vm_perm::urwx);
	kmesg("vm-test", "lazily allocated 4 pages at 0x%016lx (specified explicitly)", addr5);

	uintptr_t addr6 = s.map_exact(0xF00F0000, 0xB8000, 1, vm_perm::urwx);
	kmesg("vm-test", "mapped 1 page from 0xB8000 to 0x%016lx (specified explicitly)", addr6);

	s.debug();

	kmesg("vm-test", "destroying all regions");

	s.destroy(addr1);
	s.destroy(addr2);
	s.destroy(addr3);
	s.destroy(addr4);
	s.destroy(addr5);
	s.destroy(addr6);

	s.debug();

	kmesg("vm-test", "everything seems to have worked");

	void *startup_file;

	if (!initrd_read_file("startup", &startup_file))
		panic(NULL, "failed to load startup");

	sched_init(arch_cpu_get_count());

	arch_task_t *task = load_elf_task(startup_file);

	struct mem_region stack = {
		.start = 0x7fffffffc000,
		.end   = 0x800000000000
	};

	arch_task_alloc_mem_region(task, &stack, vm_perm::urw);

	arch_task_load_stack_ptr(task, 0x800000000000);

	int32_t id = sched_start_from_task(task);

	id = sched_start_from_task(task);

	sched_set_state(id, THREAD_RUNNING);

	asm volatile ("sti");
	while(1);
}
