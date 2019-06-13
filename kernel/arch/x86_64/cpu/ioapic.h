#ifndef IOAPIC_H
#define IOAPIC_H

#include <stdint.h>
#include <stddef.h>

void ioapic_init(void);

uint8_t ioapic_get_vector_by_gsi(uint32_t);
uint8_t ioapic_get_vector_by_irq(uint8_t);

void ioapic_mask_pin(size_t, uint32_t, int);
void ioapic_mask_gsi(uint32_t, int);
uint32_t ioapic_get_gsi_by_irq(uint8_t);

#endif
