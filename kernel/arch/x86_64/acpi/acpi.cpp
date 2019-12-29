#include "acpi.h"
#include <stdint.h>
#include <stddef.h>
#include <kmesg.h>
#include <mm/mm.h>
#include <string.h>
#include <lai/core.h>
#include <lai/helpers/sci.h>
#include <irq/isr.h>
#include <cmdline.h>
#include <cpu/ioapic.h>

typedef struct {
	char sig[8];
	uint8_t checksum;
	char oemid[6];
	uint8_t rev;
	uint32_t rsdt_addr;
} rsdp_t;

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

static uint16_t acpi_get_sci_vector(void) {
	acpi_fadt_t *fadt;
	if ((fadt = (acpi_fadt_t *)acpi_find_table("FACP", 0))) {
		return fadt->sci_irq;
	}

	return 0;
}

int acpi_sci_handler(irq_cpu_state_t *state) {
	(void)state;

	uint16_t ev = lai_get_sci_event();
	
	const char *ev_name = "?";
	if (ev & ACPI_POWER_BUTTON) ev_name = "power button";
	if (ev & ACPI_SLEEP_BUTTON) ev_name = "sleep button";
	if (ev & ACPI_WAKE) ev_name = "sleep wake up";

	kmesg("acpi", "a sci event has occured: %04x(%s)", ev, ev_name);
	
	return 1;
}

static rsdt_t *rsdt;

void acpi_init(void) {
	if (cmdline_has_value("acpi", "disable")) {
		kmesg("acpi", "disabled by user");
		return;
	}

	void *rsdt_ptr = acpi_find_rsdp(0x80000, 0x20000);
	if (!rsdt_ptr) rsdt_ptr = acpi_find_rsdp(0xe0000, 0x20000);
	if (!rsdt_ptr) {
		kmesg("acpi", "failed to find rsdp!");
		__builtin_trap();
	}

	rsdt = (rsdt_t *)((uintptr_t)rsdt_ptr + VIRT_PHYS_BASE);

	if (cmdline_has_value("acpi", "debug")) {
		kmesg("acpi", "debugging enabled by user");
		lai_enable_tracing(LAI_TRACE_IO | LAI_TRACE_OP);
	}

	lai_create_namespace();

	uint16_t sci_irq;
	if ((sci_irq = acpi_get_sci_vector())) {
		kmesg("acpi", "sci interrupt vector: %u", sci_irq);
	} else
		kmesg("acpi", "there is no defined sci interrupt vector");

	lai_enable_acpi(1);

	madt_init();

	kmesg("acpi", "init done");
}


void acpi_late_init(void) {
	uint16_t sci_irq;
	if ((sci_irq = acpi_get_sci_vector())) {
		isr_register_handler(ioapic_get_vector_by_irq(sci_irq),
				acpi_sci_handler);
	}
}

static void *acpi_find_dsdt(void) {
	acpi_fadt_t *fadt = (acpi_fadt_t *)acpi_find_table("FACP", 0);
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
