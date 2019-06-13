#ifndef IOAPIC_H
#define IOAPIC_H

#include <stdint.h>

void ioapic_init(void);

uint8_t ioapic_get_vector_by_gsi(uint32_t gsi);
uint8_t ioapic_get_vector_by_irq(uint8_t irq);

#endif
