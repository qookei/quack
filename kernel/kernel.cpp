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
#include <util.h>

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

	void *startup_file;

	if (!initrd_read_file("startup", &startup_file))
		panic(NULL, "failed to load startup");

	sched_init(arch_cpu_get_count());

	auto t = frg::make_unique<thread>(
		frg_allocator::get(),
		std::move(frg::make_unique<arch_task>(frg_allocator::get())),
		std::move(frg::make_unique<address_space>(frg_allocator::get()))
	);

	constexpr size_t stack_size = 8 * 1024 * 1024;
	constexpr size_t stack_pages = stack_size / vm_page_size;

	t->_task->vmm_ctx() = t->_addr_space->vmm_ctx();
	t->_task->sp() = t->_addr_space->allocate(stack_pages, vm_perm::urw) + stack_size;
	assert(elf64_load(startup_file, *t));

	uint64_t id = sched_add(std::move(t));
	sched_run(id);

	asm volatile ("sti");
	while(1);
}
