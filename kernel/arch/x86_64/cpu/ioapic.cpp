#include "ioapic.h"
#include <acpi/acpi.h>
#include <mm/mm.h>
#include <panic.h>
#include <cpu/cpu_data.h>
#include <util.h>

uint32_t ioapic_read(size_t ioapic, uint32_t reg) {
	if (ioapic >= madt_get_ioapics().size())
		panic(NULL, "Access to invalid ioapic %lu", ioapic);
	volatile uint32_t *addr = (volatile uint32_t *)(madt_get_ioapics()[ioapic].ioapic_addr + VIRT_PHYS_BASE);
	*addr = reg;
	return *(addr + 4);
}

void ioapic_write(size_t ioapic, uint32_t reg, uint32_t val) {
	if (ioapic >= madt_get_ioapics().size())
		panic(NULL, "Access to invalid ioapic %lu", ioapic);
	volatile uint32_t *addr = (volatile uint32_t *)(madt_get_ioapics()[ioapic].ioapic_addr + VIRT_PHYS_BASE);
	*addr = reg;
	*(addr + 4) = val;
}

size_t ioapic_get_gsi_window_size(size_t ioapic) {
	return (ioapic_read(ioapic, 1) >> 16) & 0xff;
}

size_t ioapic_get_by_gsi(uint32_t gsi) {
	for (size_t i = 0; i < madt_get_ioapics().size(); i++) {
		madt_ioapic ioapic = madt_get_ioapics()[i];
		size_t window_size = ioapic_get_gsi_window_size(i);
		if (gsi >= ioapic.gsi_base && gsi <= ioapic.gsi_base + window_size) {
			return i;
		}
	}

	panic(NULL, "No IOAPIC handles GSI %u", gsi);
	return -1;
}

void ioapic_map_pin_to_irq(size_t ioapic, uint8_t irq, uint32_t pin, uint16_t flags, uint8_t apic, int masked) {
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

void ioapic_mask_pin(size_t ioapic, uint32_t pin, int masked) {
	uint32_t reg = pin * 2 + 16;
	uint64_t ent = 0;
	ent |= ioapic_read(ioapic, reg + 0);
	ent |= (uint64_t)ioapic_read(ioapic, reg + 1) << 32;

	if (masked) {
		ent |= (1 << 16);
	} else {
		ent &= ~(1 << 16);
	}

	ioapic_write(ioapic, reg + 0, (uint32_t)ent);
	ioapic_write(ioapic, reg + 1, (uint32_t)(ent >> 32));
}

void ioapic_mask_gsi(uint32_t gsi, int masked) {
	size_t ioapic = ioapic_get_by_gsi(gsi);

	uint32_t pin = gsi - madt_get_ioapics()[ioapic].gsi_base;

	ioapic_mask_pin(ioapic, pin, masked);
}

void ioapic_map_gsi_to_irq(uint8_t irq, uint32_t gsi, uint16_t flags, uint8_t apic, int masked) {
	size_t ioapic = ioapic_get_by_gsi(gsi);

	uint32_t pin = gsi - madt_get_ioapics()[ioapic].gsi_base;

	ioapic_map_pin_to_irq(ioapic, irq, pin, flags, apic, masked);
}

static madt_iso *ioapic_get_iso_by_gsi(uint32_t gsi) {
	size_t n_isos = madt_get_isos().size();
	auto isos = madt_get_isos();
	for (size_t i = 0; i < n_isos; i++) {
		if (isos[i].gsi == gsi) {
			return &isos[i];
		}
	}

	return NULL;
}

void ioapic_init(void) {
	size_t n_ioapics = madt_get_ioapics().size();

	uint8_t vec = 0x40;

	// map all ioapic pins into the idt
	for (size_t i = 0; i < n_ioapics && vec < 0xE0; i++) {
		size_t n_pins = ioapic_get_gsi_window_size(i);
		for (size_t j = 0; j <= n_pins; j++, vec++) {
			madt_iso *iso = ioapic_get_iso_by_gsi(vec - 0x40);
			if (!iso)
				ioapic_map_pin_to_irq(i, vec, j, 0, 0, 0);
			else
				ioapic_map_pin_to_irq(i, vec, j, iso->flags, 0, 0);
		}
	}

	assert(vec < 0xE0);
}

uint8_t ioapic_get_vector_by_gsi(uint32_t gsi) {
	uint8_t vec = 0x40;
	size_t n_ioapics = madt_get_ioapics().size();

	for (size_t i = 0; i < n_ioapics && vec < 0xE0; i++) {
		size_t n_pins = ioapic_get_gsi_window_size(i);
		for (size_t j = 0; j <= n_pins; j++, vec++) {
			if (madt_get_ioapics()[i].gsi_base + j == gsi)
				return vec;
		}
	}

	return -1;
}

uint8_t ioapic_get_vector_by_irq(uint8_t irq) {
	size_t n_isos = madt_get_isos().size();
	auto isos = madt_get_isos();
	for (size_t i = 0; i < n_isos; i++) {
		if (isos[i].bus == 0 && isos[i].irq == irq) {
			return ioapic_get_vector_by_gsi(isos[i].gsi);
		}
	}

	return ioapic_get_vector_by_gsi(irq);
}

uint32_t ioapic_get_gsi_by_irq(uint8_t irq) {
	size_t n_isos = madt_get_isos().size();
	auto isos = madt_get_isos();
	for (size_t i = 0; i < n_isos; i++) {
		if (isos[i].bus == 0 && isos[i].irq == irq) {
			return isos[i].gsi;
		}
	}

	return -1;
}
