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
#include <io/debug.h>
#include <cpu/lapic.h>
#include <cpu/ioapic.h>
#include <cpu/cpu_data.h>
#include <cpu/smp.h>
#include <io/pci.h>
#include <cpu/ctx.h>

#include <kmesg.h>
#include <util.h>
#include <mm/heap.h>
#include <cmdline.h>

#include <arch/info.h>

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

	arch_video_mode_t *vid = NULL;
	if (mboot->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO
	&& mboot->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_RGB) {

		vid = kcalloc(sizeof(arch_video_mode_t), 1);
		vid->addr = mboot->framebuffer_addr;
		vid->pitch = mboot->framebuffer_pitch;

		vid->width = mboot->framebuffer_width;
		vid->height = mboot->framebuffer_height;
		vid->bpp = mboot->framebuffer_bpp;

		vid->red_off = mboot->framebuffer_red_field_position;
		vid->green_off = mboot->framebuffer_green_field_position;
		vid->blue_off = mboot->framebuffer_blue_field_position;

		vid->red_size = mboot->framebuffer_red_mask_size;
		vid->green_size = mboot->framebuffer_green_mask_size;
		vid->blue_size = mboot->framebuffer_blue_mask_size;
	}

	debugcon_init(vid);

	acpi_init();
	cpu_data_init();
	lapic_init();
	ioapic_init();

	lapic_timer_calc_freq();
	lapic_timer_init();

	ioapic_mask_gsi(ioapic_get_gsi_by_irq(0x0), 1);

	pci_init();

	smp_init();

	acpi_late_init();

	kmesg("kernel", "done initializing");

	arch_boot_info_t *info = kcalloc(sizeof(arch_boot_info_t), 1);

	if (mboot->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO) {
		if (mboot->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_RGB) {
			info->vid_mode = vid;
			info->flags |= ARCH_INFO_HAS_VIDEO_MODE;
		}
	}

	if (mboot->mods_count) {
		// only first module is taken into account
		multiboot_module_t *mod = (multiboot_module_t *)(mboot->mods_addr + VIRT_PHYS_BASE);
		info->initramfs = (void *)(mod->mod_start + VIRT_PHYS_BASE);
		info->initramfs_cmd = (const char *)(mod->cmdline + VIRT_PHYS_BASE);
		info->initramfs_size = (mod->mod_end - mod->mod_start);
		info->flags |= ARCH_INFO_HAS_INITRAMFS;
	}

	asm volatile("cli");

	//test_foo_bar();

	kernel_main(info);
}
