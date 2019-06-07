#ifndef PCI_H
#define PCI_H

#include <stdint.h>

uint32_t pci_read_word(uint8_t, uint8_t, uint8_t, uint16_t);
void pci_write_word(uint8_t, uint8_t, uint8_t, uint16_t, uint32_t);

#endif
