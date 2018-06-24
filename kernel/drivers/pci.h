#ifndef PCI_H
#define PCI_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
	uint32_t addr;
	uint32_t len;
	bool mmio;
	bool exists;
	bool prefetch;
} pci_bar_t;

typedef struct {	
	uint32_t port_base;
	uint32_t interrupt;
	
	uint16_t bus;
	uint16_t device;
	uint16_t function;
	
	pci_bar_t bar[6];
	
	uint16_t vendor_id;
	uint16_t device_id;
	
	uint8_t class_id;
	uint8_t subclass_id;
	uint8_t interface_id;
	
	uint8_t revision;
} pci_descriptor_t;

void pci_set_bus_mastering(bool, uint16_t, uint16_t, uint16_t);
void pci_bus_scan();

#endif
