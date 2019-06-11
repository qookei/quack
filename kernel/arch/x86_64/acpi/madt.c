#include "acpi.h"
#include <kmesg.h>
#include <mm/heap.h>
#include <string.h>

#define TYPE_LAPIC 0
#define TYPE_IOAPIC 1
#define TYPE_ISO 2
#define TYPE_NMI 4

static madt_t* madt;

static size_t nmi_count = 0;
static size_t iso_count = 0;
static size_t ioapic_count = 0;
static size_t lapic_count = 0;

static madt_lapic_t *lapic_tab = NULL;
static madt_ioapic_t *ioapic_tab = NULL;
static madt_iso_t *iso_tab = NULL;
static madt_nmi_t *nmi_tab = NULL;

#define APPEND_TAB_ENT(cnt, arr, ent, type) {			\
		cnt++;						\
		arr = krealloc(arr, cnt * sizeof(type));	\
		memcpy(&arr[cnt - 1], ent, sizeof(type));	\
	}

void madt_init() {
	madt = acpi_find_table("APIC", 0);
	if (!madt) {
		kmesg("acpi", "failed to find madt table");
		return;
	}

	kmesg("acpi", "scanning madt");
	size_t off = 0, n_entries = madt->sdt.len - sizeof(madt_t);

	while (off < n_entries) {
		madt_ent_t *ent = (void *)((uintptr_t)madt->entries + off);
		off += ent->len;
		madt_lapic_t *lapic = (void *)ent->data;
		madt_ioapic_t *ioapic = (void *)ent->data;
		madt_iso_t *iso = (void *)ent->data;
		madt_nmi_t *nmi = (void *)ent->data;

		switch (ent->type) {
			case TYPE_LAPIC:
				APPEND_TAB_ENT(lapic_count, lapic_tab, lapic, madt_lapic_t);
				kmesg("acpi", "lapic: %u for cpu %u(%s)", lapic->apic_id, lapic->proc_id, (lapic->flags & 1) ? "enabled" : "disabled");
				break;
			case TYPE_IOAPIC:
				APPEND_TAB_ENT(ioapic_count, ioapic_tab, ioapic, madt_ioapic_t);
				kmesg("acpi", "ioapic: %u at %016x gsi %u", ioapic->ioapic_id, ioapic->ioapic_addr, ioapic->gsi_base);
				break;
			case TYPE_ISO:
				APPEND_TAB_ENT(iso_count, iso_tab, iso, madt_iso_t);
				kmesg("acpi", "iso: bus %u irq %u maps to gsi %u flags %u", iso->bus, iso->irq, iso->gsi, iso->flags);
				break;
			case TYPE_NMI:
				APPEND_TAB_ENT(nmi_count, nmi_tab, nmi, madt_nmi_t);
				kmesg("acpi", "nmi: processor %u flags %u lint %u", nmi->proc_id, nmi->flags, nmi->lint);
				break;
			default:
				kmesg("acpi", "invalid entry");
		}
	}
}

uintptr_t madt_get_lapic_base(void) {
	return madt->lapic_addr;
}

size_t madt_get_nmi_count(void) {
	return nmi_count;
}

size_t madt_get_lapic_count(void) {
	return lapic_count;
}

size_t madt_get_ioapic_count(void) {
	return ioapic_count;
}

size_t madt_get_iso_count(void) {
	return iso_count;
}

madt_nmi_t *madt_get_nmis(void) {
	return nmi_tab;
}

madt_lapic_t *madt_get_lapics(void) {
	return lapic_tab;
}

madt_ioapic_t *madt_get_ioapics(void) {
	return ioapic_tab;
}

madt_iso_t *madt_get_isos(void) {
	return iso_tab;
}
