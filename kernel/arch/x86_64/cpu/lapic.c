#include "lapic.h"
#include <kmesg.h>
#include <panic.h>
#include <acpi/acpi.h>
#include <mm/mm.h>
#include <string.h>

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
		lapic_nmi_set(0x90 + i, nmis[i].lint, nmis[i].flags);
	}
}

void lapic_eoi(void) {
	lapic_write(0xb0, 0);
}

void lapic_enable(void) {
	lapic_write(0xf0, lapic_read(0xf0) | 0x1ff);
}

void lapic_init(void) {
	// XXX: this might need changing to disable caching
	lapic_base = madt_get_lapic_base() + VIRT_PHYS_BASE;

	kmesg("lapic", "setting up the lapic");
	lapic_nmi_setup();
	lapic_enable();
}
