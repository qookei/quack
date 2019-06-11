#include "ioapic.h"
#include <acpi/acpi.h>
#include <acpi/acpi.h>
#include <mm/mm.h>
#include <panic.h>

uint32_t ioapic_read(size_t ioapic, uint32_t reg) {
	if (ioapic >= madt_get_ioapic_count())
		panic(NULL, "Access to invalid ioapic %lu", ioapic);
	uint32_t *addr = (uint32_t *)(madt_get_ioapics()[ioapic].ioapic_addr + VIRT_PHYS_BASE);
	*addr = reg;
	return *(addr + 4);
}

void ioapic_write(size_t ioapic, uint32_t reg, uint32_t val) {
	if (ioapic >= madt_get_ioapic_count())
		panic(NULL, "Access to invalid ioapic %lu", ioapic);
	uint32_t *addr = (uint32_t *)(madt_get_ioapics()[ioapic].ioapic_addr + VIRT_PHYS_BASE);
	*addr = reg;
	*(addr + 4) = val;
}

size_t ioapic_get_gsi_window_size(size_t ioapic) {
	return (ioapic_read(ioapic, 1) >> 16) & 0xff;
}

size_t ioapic_get_by_gsi(uint32_t gsi) {
	for (size_t i = 0; i < madt_get_ioapic_count(); i++) {
		madt_ioapic_t ioapic = madt_get_ioapics()[i];
		size_t window_size = ioapic_get_gsi_window_size(i);
		if (gsi >= ioapic.gsi_base && gsi < ioapic.gsi_base + window_size) {
			return i;
		}
	}

	panic(NULL, "No IOAPIC handles GSI %u", gsi);
}

void ioapic_map_gsi_to_irq(uint8_t irq, uint32_t gsi, uint16_t flags, uint8_t apic, int masked) {
	size_t ioapic = ioapic_get_by_gsi(gsi);

	uint32_t pin = gsi - madt_get_ioapics()[ioapic].gsi_base;
	uint64_t ent = irq;

	if (flags & 2)
		ent |= (1 << 13);

	if (flags & 8)
		ent |= (1 << 15);

	if (masked) {
		ent |= (1 << 16);
	}

	ent |= ((uint64_t) apic) << 56;

	uint32_t reg = pin * 2 + 16;
	ioapic_write(ioapic, reg + 0, (uint32_t)ent);
	ioapic_write(ioapic, reg + 1, (uint32_t)(ent >> 32));
}

void ioapic_init_isos(int cpu, int masked) {

	// TODO: multicore

	for (size_t i = 0; i < madt_get_iso_count(); i++) {
		madt_iso_t iso = madt_get_isos()[i];
		ioapic_map_gsi_to_irq(iso.irq + 0x40, iso.gsi, iso.flags, 0, masked);
	}


	ioapic_map_gsi_to_irq(0x41, 1, 0, 0, 0);
}
