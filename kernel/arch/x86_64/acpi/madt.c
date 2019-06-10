#include "acpi.h"
#include <kmesg.h>

#define TYPE_LAPIC 0
#define TYPE_IOAPIC 1
#define TYPE_ISO 2
#define TYPE_NMI 4

typedef struct {
	uint8_t proc_id;
	uint8_t apic_id;
	uint32_t flags;
} __attribute__((packed)) madt_lapic_t;

typedef struct {
	uint8_t ioapic_id;
	uint8_t reserved;
	uint32_t ioapic_addr;
	uint32_t gsi_base;
} __attribute__((packed)) madt_ioapic_t;

typedef struct {
	uint8_t bus;
	uint8_t irq;
	uint32_t gsi;
	uint16_t flags;
} __attribute__((packed)) madt_iso_t;

typedef struct {
	uint8_t proc_id;
	uint16_t flags;
	uint8_t lint;
} __attribute__((packed)) madt_nmi_t;

typedef struct {
	uint8_t type;
	uint8_t len;
	uint8_t data[];
} __attribute__((packed)) madt_ent_t;

typedef struct {
	sdt_t sdt;
	uint32_t lapic_addr;
	uint32_t flags;
	uint8_t entries[];
} __attribute__((packed)) madt_t;

static madt_t* madt;

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
				kmesg("acpi", "lapic: %u for cpu %u(%s)", lapic->apic_id, lapic->proc_id, (lapic->flags & 1) ? "enabled" : "disabled");
				break;
			case TYPE_IOAPIC:
				kmesg("acpi", "ioapic: %u at %016x gsi %u", ioapic->ioapic_id, ioapic->ioapic_addr, ioapic->gsi_base);
				break;
			case TYPE_ISO:
				kmesg("acpi", "iso: bus %u irq %u maps to gsi %u flags %u", iso->bus, iso->irq, iso->gsi, iso->flags);
				break;
			case TYPE_NMI:
				kmesg("acpi", "nmi: processor %u flags %u lint %u", nmi->proc_id, nmi->flags, nmi->lint);
				break;
			default:
				kmesg("acpi", "invalid entry");
		}
	}
}
