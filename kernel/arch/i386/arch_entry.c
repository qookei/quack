#include <stdint.h>

#include <cpu/gdt/gdt.h>

#include <cpu/int/idt.h>
#include <cpu/int/isr.h>

// TODO: NO! This assumes we use the PIT, don't do that.
void pit_freq(uint32_t frequency) {
	uint16_t x = 1193182 / frequency;
	if ((1193182 % frequency) > (frequency / 2))
		x++;

	outb(0x40, (uint8_t)(x & 0x00ff));
	outb(0x40, (uint8_t)(x / 0x0100));
}

extern void *isr_stack;

void kernel_main(void *);

void arch_entry(void *mboot, uint32_t magic) {
	if (((uintptr_t)mboot) - 0xC0000000 > 0x800000) {
		return;
	}

	gdt_setup();
	gdt_set_tss_stack((uint32_t)&isr_stack);

	pic_remap(0x20, 0x28);	// TODO: choose APIC over PIC whenever possible
	idt_init();				// TODO: fill all 256 interrupt vectors
	pit_freq(1000);			// TODO: use APIC timer when using the APIC

	pmm_init(mboot);		// TODO
	paging_init();				// TODO

	// TODO: Do some ACPI magic here to detect APICs and all that stuff
	// TODO: Prepare arch independent info for kernel 

	kernel_main(mboot);
}
