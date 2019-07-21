#ifndef ACPI_H
#define ACPI_H

#include <stddef.h>
#include <stdint.h>

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

void acpi_init(void);
void *acpi_find_table(const char *, size_t);

void madt_init(void);
void acpi_late_init(void);
uintptr_t madt_get_lapic_base(void);

size_t madt_get_nmi_count(void);
size_t madt_get_lapic_count(void);
size_t madt_get_ioapic_count(void);
size_t madt_get_iso_count(void);

madt_nmi_t *madt_get_nmis(void);
madt_lapic_t *madt_get_lapics(void);
madt_ioapic_t *madt_get_ioapics(void);
madt_iso_t *madt_get_isos(void);

#endif
