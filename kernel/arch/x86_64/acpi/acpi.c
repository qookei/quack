#include "acpi.h"
#include <stdint.h>
#include <stddef.h>
#include <kmesg.h>
#include <mm/mm.h>
#include <string.h>
#include <lai/core.h>

typedef struct {
	char sig[8];
	uint8_t checksum;
	char oemid[6];
	uint8_t rev;
	uint32_t rsdt_addr;
} rsdp_t;

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
	sdt_t sdt;
	uint32_t table_ptr[];
} rsdt_t;

static uint8_t acpi_calc_checksum(void *ptr, size_t len) {
	uint8_t sum = 0;
	for (size_t i = 0; i < len; i++) sum += ((uint8_t *)ptr)[i];
	return sum;
}

static void *acpi_find_rsdp(uintptr_t region_start, size_t region_len) {
	for (size_t idx = 0; idx < region_len; idx += 16) {
		rsdp_t *rsdp = (rsdp_t *)(region_start + idx + VIRT_PHYS_BASE);
		if (memcmp(rsdp->sig, "RSD PTR ", 8)) continue;
		if (acpi_calc_checksum(rsdp, sizeof(rsdp_t))) continue;
		char oem[7];
		memset(oem, 0, 7);
		memcpy(oem, rsdp->oemid, 6);
		kmesg("acpi", "found rsdp at %016lx:\n"
					  "\toem: '%s'\n"
					  "\trevision: %u\n"
					  "\trsdt ptr: %016lx", 
					  (region_start + idx),
					  oem,
					  rsdp->rev,
					  rsdp->rsdt_addr);
		return (void *)((uintptr_t)rsdp->rsdt_addr);
	}

	return NULL;
}

static rsdt_t *rsdt;

void acpi_init(void) {
	void *rsdt_ptr = acpi_find_rsdp(0x80000, 0x20000);
	if (!rsdt_ptr) rsdt_ptr = acpi_find_rsdp(0xf0000, 0x10000);
	if (!rsdt_ptr) {
		kmesg("acpi", "failed to find rsdp!");
		__builtin_trap();
	}

	rsdt = (rsdt_t *)((uintptr_t)rsdt_ptr + VIRT_PHYS_BASE);
	
	lai_create_namespace();
	
	kmesg("acpi", "init done");

	lai_enter_sleep(5);
}

static void *acpi_find_dsdt(void) {
	acpi_fadt_t *fadt = acpi_find_table("FACP", 0);
	if (!fadt) return NULL;

	uintptr_t dsdt = fadt->dsdt;
	kmesg("acpi", "found DSDT at %016lx", dsdt);
	return (void *)(dsdt + VIRT_PHYS_BASE);
}

void *acpi_find_table(const char *sig, size_t idx) {
	if (!strcmp(sig, "DSDT")) return acpi_find_dsdt();

	size_t curr_idx = 0;

	sdt_t *sdt = NULL;
	size_t entries = (rsdt->sdt.len - sizeof(sdt_t)) / 4;

	for (size_t i = 0; i < entries; i++) {
		sdt = (sdt_t *)(rsdt->table_ptr[i] + VIRT_PHYS_BASE);
		if (acpi_calc_checksum(sdt, sdt->len)) continue;
		if (memcmp(sdt->sig, sig, 4)) continue;
		if (curr_idx++ == idx) {
			kmesg("acpi", "table '%s' has been found", sig);
			return sdt;
		}
	}

	kmesg("acpi", "table '%s' has not been found", sig);
	return NULL;
}
