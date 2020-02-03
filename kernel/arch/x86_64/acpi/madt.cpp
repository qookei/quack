#include "acpi.h"
#include <kmesg.h>
#include <mm/heap.h>
#include <string.h>

#define TYPE_LAPIC 0
#define TYPE_IOAPIC 1
#define TYPE_ISO 2
#define TYPE_NMI 4

static madt* _madt;

static frg::vector<madt_lapic, frg_allocator> madt_lapics{frg_allocator::get()};
static frg::vector<madt_nmi, frg_allocator> madt_nmis{frg_allocator::get()};
static frg::vector<madt_iso, frg_allocator> madt_isos{frg_allocator::get()};
static frg::vector<madt_ioapic, frg_allocator> madt_ioapics{frg_allocator::get()};

void madt_init() {
	_madt = (madt *)acpi_find_table("APIC", 0);
	if (!_madt) {
		kmesg("acpi", "failed to find madt table");
		return;
	}

	kmesg("acpi", "scanning madt");
	size_t off = 0, n_entries = _madt->sdt.len - sizeof(madt);

	while (off < n_entries) {
		madt_ent *ent = reinterpret_cast<madt_ent *>(reinterpret_cast<uintptr_t>(_madt->entries) + off);
		off += ent->len;
		madt_lapic *lapic = reinterpret_cast<madt_lapic *>(ent->data);
		madt_ioapic *ioapic = reinterpret_cast<madt_ioapic *>(ent->data);
		madt_iso *iso = reinterpret_cast<madt_iso *>(ent->data);
		madt_nmi *nmi = reinterpret_cast<madt_nmi *>(ent->data);

		switch (ent->type) {
			case TYPE_LAPIC:
				madt_lapics.push(*lapic);
				kmesg("acpi", "lapic: %u for cpu %u(%s)", lapic->apic_id, lapic->proc_id, (lapic->flags & 1) ? "enabled" : "disabled");
				break;
			case TYPE_IOAPIC:
				madt_ioapics.push(*ioapic);
				kmesg("acpi", "ioapic: %u at %016x gsi %u", ioapic->ioapic_id, ioapic->ioapic_addr, ioapic->gsi_base);
				break;
			case TYPE_ISO:
				madt_isos.push(*iso);
				kmesg("acpi", "iso: bus %u irq %u maps to gsi %u flags %u", iso->bus, iso->irq, iso->gsi, iso->flags);
				break;
			case TYPE_NMI:
				madt_nmis.push(*nmi);
				kmesg("acpi", "nmi: processor %u flags %u lint %u", nmi->proc_id, nmi->flags, nmi->lint);
				break;
			default:
				kmesg("acpi", "invalid entry");
		}
	}
}

uintptr_t madt_get_lapic_base(void) {
	return _madt->lapic_addr;
}

frg::vector<madt_lapic, frg_allocator> &madt_get_lapics() {
	return madt_lapics;
}

frg::vector<madt_nmi, frg_allocator> &madt_get_nmis() {
	return madt_nmis;
}

frg::vector<madt_iso, frg_allocator> &madt_get_isos() {
	return madt_isos;
}

frg::vector<madt_ioapic, frg_allocator> &madt_get_ioapics() {
	return madt_ioapics;
}
