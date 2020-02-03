#ifndef ACPI_H
#define ACPI_H

#include <stddef.h>
#include <stdint.h>
#include <new>

#include <frg/vector.hpp>
#include <lib/frg_allocator.h>

typedef struct {
	char sig[4];
	uint32_t len;
	uint8_t rev;
	uint8_t checksum;
	char oemid[6];
	char oemid_table[6];
	uint32_t oem_rev;
	uint32_t creator_id;
	uint32_t creator_rev;
} sdt_t;

struct __attribute__((packed)) madt_lapic {
	uint8_t proc_id;
	uint8_t apic_id;
	uint32_t flags;
};

struct __attribute__((packed)) madt_ioapic {
	uint8_t ioapic_id;
	uint8_t reserved;
	uint32_t ioapic_addr;
	uint32_t gsi_base;
};

struct __attribute__((packed)) madt_iso {
	uint8_t bus;
	uint8_t irq;
	uint32_t gsi;
	uint16_t flags;
};

struct __attribute__((packed)) madt_nmi {
	uint8_t proc_id;
	uint16_t flags;
	uint8_t lint;
};

struct __attribute__((packed)) madt_ent {
	uint8_t type;
	uint8_t len;
	uint8_t data[];
};

struct __attribute__((packed)) madt {
	sdt_t sdt;
	uint32_t lapic_addr;
	uint32_t flags;
	uint8_t entries[];
};

void acpi_init(void);
void *acpi_find_table(const char *, size_t);

void madt_init(void);
void acpi_late_init(void);
uintptr_t madt_get_lapic_base(void);

frg::vector<madt_lapic, frg_allocator> &madt_get_lapics();
frg::vector<madt_nmi, frg_allocator> &madt_get_nmis();
frg::vector<madt_iso, frg_allocator> &madt_get_isos();
frg::vector<madt_ioapic, frg_allocator> &madt_get_ioapics();

template<typename F>
void madt_enumerate_lapic(F functor) {
	for (auto &lapic : madt_get_lapics())
		functor(lapic);
}

template<typename F>
void madt_enumerate_nmis(F functor) {
	for (auto &nmi : madt_get_nmis())
		functor(nmi);
}

template<typename F>
void madt_enumerate_isos(F functor) {
	for (auto &iso : madt_get_isos())
		functor(iso);
}

template<typename F>
void madt_enumerate_ioapic(F functor) {
	for (auto &ioapic : madt_get_ioapics())
		functor(ioapic);
}

#endif
