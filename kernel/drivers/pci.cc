#include <io/ports.h>
#include "drv.h"
#include <mesg.h>
#include <paging/paging.h>

uint32_t pci_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
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

void pci_write_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value) {
	uint32_t address;
	uint32_t lbus  = (uint32_t)bus;
	uint32_t lslot = (uint32_t)slot;
	uint32_t lfunc = (uint32_t)func;

	address = (uint32_t)((lbus << 16) | (lslot << 11) |
			  (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));

	outl(0xCF8, address);
	outl(0xCFC, value);
}

void pci_set_bus_mastering(bool e, uint16_t bus, uint16_t device, uint16_t function) {
	uint16_t v = pci_read_word(bus, device, function, 0x04);
	if (e)
		v |= (1 << 2);
	else
		v &= ~(1 << 2);
	pci_write_word(bus, device, function, 0x04, v);
}

void pci_get_desc(pci_descriptor_t *desc, uint16_t bus, uint16_t device, uint16_t function) {
	memset(desc, 0, sizeof(pci_descriptor_t));
	desc->bus = bus;
	desc->device = device;
	desc->function = function;
	
	desc->vendor_id = pci_read_word(bus, device, function, 0x00);
	desc->device_id = pci_read_word(bus, device, function, 0x02);
	
	desc->class_id = pci_read_word(bus, device, function, 0x0B);
	desc->subclass_id = pci_read_word(bus, device, function, 0x0A);
	desc->interface_id = pci_read_word(bus, device, function, 0x09);
	
	for (uint32_t i = 0; i < 6; i++) {
		uint32_t off = 0x10 + 0x4 * i;
		uint32_t val = pci_read_word(bus, device, function, off);
		
		if (val == 0) continue;
		
		pci_write_word(bus, device, function, off, 0xFFFFFFFF);
		uint32_t b = pci_read_word(bus, device, function, off);
		pci_write_word(bus, device, function, off, val);
		
		desc->bar[i].mmio = !(val & 1);
		desc->bar[i].exists = true;
		if (desc->bar[i].mmio) {
			desc->bar[i].addr = val & 0xFFFFFFF0;
			desc->bar[i].prefetch = (val >> 0x3) & 0x1;
			desc->bar[i].len = ~(b & 0xFFFFFFF0) + 1;
		} else {
			desc->bar[i].addr = val & 0xFFFFFFFC;			
			desc->bar[i].len = ~(b & 0xFFFFFFFC) + 1;
		}
	}
	
	desc->revision = pci_read_word(bus, device, function, 0x08);
	desc->interrupt = pci_read_word(bus, device, function, 0x3C);
	
	desc->interrupt &= 0xFF;
}

void pci_check_function(uint8_t bus, uint8_t device, uint8_t function) {
	pci_descriptor_t desc;
	pci_get_desc(&desc, bus, device, function);
	
	#ifndef PCI_DISABLE_LOG
	
	early_mesg(LEVEL_INFO, "pci", "device %04x:%04x detected", desc.vendor_id, desc.device_id);
	
	if (desc.interrupt) early_mesg(LEVEL_INFO, "pci", "irq %u", desc.interrupt);
	
	for(uint32_t i = 0; i < 6; i++) {
		if (!desc.bar[i].exists) continue;
		if (!desc.bar[i].mmio)
			early_mesg(LEVEL_INFO, "pci", "bar %u i  addr %08x len %08x", i, desc.bar[i].addr, desc.bar[i].len);
		else
			early_mesg(LEVEL_INFO, "pci", "bar %u m%c addr %08x len %08x", i, desc.bar[i].prefetch ? 'p' : ' ', desc.bar[i].addr, desc.bar[i].len);
		 
	}
	
	early_newl(LEVEL_INFO);
	
	#endif
	
	drv_install_pci(&desc);
	
}

uint16_t pci_check_vendor(uint8_t bus, uint8_t slot, uint8_t function) {
	uint16_t vendor;
	
	vendor = pci_read_word(bus, slot, function, 0);
	return vendor;
}

bool pci_has_functions(uint16_t bus, uint16_t device) {
	return pci_read_word(bus, device, 0, 0x0E) & (1<<7);
}

void pci_check_device(uint8_t bus, uint8_t device) {
	uint8_t function = 0;

	if(pci_check_vendor(bus, device, function) == 0xFFFF)
		return;
	
	pci_check_function(bus, device, function);
	
	if(pci_has_functions(bus, device)) {
		for(function = 1; function < 8; function++) {
			if(pci_check_vendor(bus, device, function) != 0xFFFF) {
				pci_check_function(bus, device, function);
			}
		}
	}
}

void pci_bus_scan() {
	uint16_t bus;
	uint8_t device;

	for(bus = 0; bus < 256; bus++) {
		for(device = 0; device < 32; device++) {
			pci_check_device(bus, device);
		}
	}
}
