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

#include <object/object.h>
#include <proc/groups.h>

extern "C" void __cxa_pure_virtual() {
	panic(nullptr, "pure virtual method called");
	__builtin_unreachable();
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

	kmesg("obj-test", "group test");
	object_group g;

	kmesg("obj-test", "1");
	auto res1 = g.check_membership_and(1, &object_group::add_member, 2);
	assert(!res1);
	kmesg("obj-test", "2");
	g.add_member(1);
	kmesg("obj-test", "3");
	auto res2 = g.check_membership_and(1, &object_group::add_member, 2);
	assert(res2);
	kmesg("obj-test", "4");
	auto res3 = g.check_membership_and(1, &object_group::remove_member, 2);
	kmesg("obj-test", "4.5");
	auto res3_5 = g.check_membership_and(1, &object_group::get_object, 0);
	kmesg("obj-test", "5");
	auto res4 = g.check_membership_and(1, &object_group::remove_member, 1);
	kmesg("obj-test", "6");
	auto res5 = g.check_membership_and(1, &object_group::get_object, 0);
	assert(res3);
	assert(res4);
	assert(*res3_5 == nullptr);
	assert(!res5);

	kmesg("obj-test", "7");
	auto res6 = g.check_membership(1);
	assert(!res6);
	kmesg("obj-test", "8");

	// TODO: parse a config file to see what to load

	auto &grp = global_object_groups::get();

	auto gid = grp.new_group(1);
	grp.check_membership_and(1, gid, &object_group::add_member, 2);
	auto foo = new ipc_queue;
	grp.check_membership_and(2, gid, &object_group::add_object, foo);
	grp.remove_group(1, gid);

	kmesg("obj-test", "test done");

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
