#include <stddef.h>
#include <stdint.h>
#include <io/serial.h>
#include <io/ports.h>
#include <trace/stacktrace.h>
#include <interrupt/idt.h>
#include <interrupt/isr.h>
#include <pic/pic.h>
#include <multiboot.h>
#include <paging/pmm.h>
#include <paging/paging.h>
#include <kheap/heap.h>
#include <tasking/tasking.h>
#include <mesg.h>

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

void kernel_main(multiboot_info_t *mboot) {
	serial_init();
	
	if (((uintptr_t)mboot) - 0xC0000000 > 0x800000) {
		early_mesg(LEVEL_ERR, "mboot", "hdr out of page");
		return;
	}

	gdt_new_setup();
	early_mesg(LEVEL_INFO, "cpu", "GDT ok");

	pic_remap(0x20, 0x28);
	idt_init();
	early_mesg(LEVEL_INFO, "cpu", "interrupts ok\n");
	
	pmm_init(mboot);

	paging_init();

	early_mesg(LEVEL_INFO, "mem", "%u out of %u bytes free", free_pages() * 4096, max_pages() * 4096);
	
	uint32_t stack;

	pit_freq(4000);

	tasking_setup(NULL);

	asm volatile ("mov %%esp, %0" : "=r"(stack));

	gdt_set_tss_stack(stack);
	gdt_ltr();

	asm volatile ("sti");
	
	while(1);
}
