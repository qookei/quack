#ifndef ACPI_H
#define ACPI_H

#include <stddef.h>

void acpi_init(void);
void *acpi_find_table(const char *, size_t);

#endif
