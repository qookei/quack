#include "acpi.h"
#include <stdint.h>
#include <stddef.h>
#include <kmesg.h>
#include <mm/mm.h>
#include <string.h>

typedef struct {
	char sig[8];
	uint8_t checksum;
	char oemid[6];
	uint8_t rev;
	uint32_t rsdt_addr;
} rsdp_t;

static uint8_t acpi_rsdp_checksum(rsdp_t *rsdp) {
	uint8_t sum = 0;
	for (size_t i = 0; i < sizeof(rsdp_t); i++) sum += ((uint8_t *)rsdp)[i];
	return sum;
}

static void *acpi_find_rsdp(uintptr_t region_start, size_t region_len) {
	for (size_t idx = 0; idx < region_len; idx += 16) {
		rsdp_t *rsdp = (rsdp_t *)(region_start + idx + VIRT_PHYS_BASE);
		if (memcmp(rsdp->sig, "RSD PTR ", 8)) continue;
		if (acpi_rsdp_checksum(rsdp)) continue;
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

void acpi_init(void) {
	void *rsdt = acpi_find_rsdp(0x80000, 0x20000);
	if (!rsdt) rsdt = acpi_find_rsdp(0xf0000, 0x10000);

	kmesg("acpi", "lol");
}
