#include "pci.h"

#include <stdint.h>
#include <stddef.h>
#include <io/port.h>

uint32_t pci_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint16_t offset) {
	uint32_t address;
	uint32_t lbus  = (uint32_t)bus;
	uint32_t lslot = (uint32_t)slot;
	uint32_t lfunc = (uint32_t)func;
	uint32_t tmp = 0;

	address = (uint32_t)((lbus << 16) | (lslot << 11) |
				(lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));

	outl(0xCF8, address);
	tmp = (inl(0xCFC) >> ((offset & 2) * 8));
	return tmp;
}

void pci_write_word(uint8_t bus, uint8_t slot, uint8_t func, uint16_t offset, uint32_t value) {
	uint32_t address;
	uint32_t lbus  = (uint32_t)bus;
	uint32_t lslot = (uint32_t)slot;
	uint32_t lfunc = (uint32_t)func;

	address = (uint32_t)((lbus << 16) | (lslot << 11) |
				(lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));

	outl(0xCF8, address);
	outl(0xCFC, value);
}
