#include "lapic.h"
#include <kmesg.h>
#include <panic.h>
#include <acpi/acpi.h>
#include <mm/mm.h>
#include <string.h>
#include <irq/isr.h>
#include <io/port.h>

static uintptr_t lapic_base;

uint32_t lapic_read(uint32_t offset) {
	return *(volatile uint32_t *)(lapic_base + offset);
}

void lapic_write(uint32_t offset, uint32_t val) {
	*(volatile uint32_t *)(lapic_base + offset) = val;
}

static void lapic_nmi_set(uint8_t vector, uint8_t lint, uint16_t flags) {
	uint32_t nmi = vector | 0x320;

	if (flags & 2)
		nmi |= (1 << 13);

	if (flags & 8)
		nmi |= (1 << 15);

	uint32_t off;

	switch(lint) {
		case 0:
			off = 0x350;
			break;
		case 1:
			off = 0x360;
			break;
		default:
			panic(NULL, "Invalid lint %u for nmi", lint);
	}

	lapic_write(off, nmi);
}

static void lapic_nmi_setup(void) {
	madt_nmi_t *nmis = madt_get_nmis();
	for (size_t i = 0; i < madt_get_nmi_count(); i++) {
		lapic_nmi_set(0xE0 + i, nmis[i].lint, nmis[i].flags);
	}
}

void lapic_eoi(void) {
	lapic_write(0xB0, 0);
}

void lapic_enable(void) {
	lapic_write(0xF0, lapic_read(0xF0) | 0x1FF);
}

void lapic_init(void) {
	// XXX: this might need changing to disable caching
	lapic_base = madt_get_lapic_base() + VIRT_PHYS_BASE;

	kmesg("lapic", "setting up the lapic");
	lapic_nmi_setup();
	lapic_enable();
}

static uint64_t lapic_speed_hz;

static volatile uint64_t pit_ticks = 0;
static int pit_irq(irq_cpu_state_t *s) {
	(void)s;
	pit_ticks ++;
	return 1;
}

static void pit_set_freq(uint32_t frequency) {
	uint16_t x = 1193182 / frequency;
	if ((1193182 % frequency) > (frequency / 2))
		x++;

	outb(0x40, (uint8_t)(x & 0x00ff));
	io_wait();
	outb(0x40, (uint8_t)(x / 0x0100));
	io_wait();
}

void lapic_timer_calc_freq(void) {
	uint8_t pit_vec = ioapic_get_vector_by_irq(0x0);
	pit_set_freq(1000);
	isr_register_handler(pit_vec, pit_irq);

	lapic_write(0x320, 0);	// vector 0, non periodic, fixed delivery mode
	lapic_write(0x3E0, 0x7); // 1x divider

	pit_ticks = 0;
	while(pit_ticks < 1000);

	lapic_write(0x380, 0xFFFFFFFF);
	pit_ticks = 0;
	while(pit_ticks < 1000);

	lapic_speed_hz = 0xFFFFFFFF - lapic_read(0x390);

	isr_unregister_handler(pit_vec);

	kmesg("lapic-timer", "timer speed is %luHz", lapic_speed_hz);
}

void lapic_timer_init(void) {
}

void lapic_timer_set_frequency(uint64_t freq) {
	uint64_t period = lapic_speed_hz / freq;
}
