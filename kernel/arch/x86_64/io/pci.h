#ifndef PCI_H
#define PCI_H

#include <stdint.h>
#include <stddef.h>

#define BAR_IO 1
#define BAR_MEM 2

uint32_t pci_read_word(uint8_t, uint8_t, uint8_t, uint16_t);
void pci_write_word(uint8_t, uint8_t, uint8_t, uint16_t, uint32_t);

typedef struct {
	int enabled;
	uint8_t type;
	uintptr_t addr;
	size_t length;
	int prefetch;
} pci_bar_t;

typedef struct {
	uint8_t irq;

	uint8_t bus;
	uint8_t dev;
	uint8_t fun;

	uint16_t vendor;
	uint16_t device;

	uint8_t base_class;
	uint8_t sub_class;

	pci_bar_t bar[6];
} pci_dev_t;

void pci_init(void);
size_t pci_get_n_devices(void);
pci_dev_t *pci_get_devices(void);

#endif
