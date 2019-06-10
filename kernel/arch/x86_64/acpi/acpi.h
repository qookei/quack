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

void acpi_init(void);
void *acpi_find_table(const char *, size_t);
void madt_init(void);

#endif
