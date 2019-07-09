#include "pci.h"

#include <stdint.h>
#include <stddef.h>
#include <io/port.h>
#include <util.h>
#include <mm/heap.h>
#include <string.h>
#include <kmesg.h>
#include <lai/core.h>
#include <cpu/ioapic.h>
#include <arch/cpu.h>

#include <cmdline.h>

uint32_t pci_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint16_t offset) {
	uint32_t address;
	uint32_t lbus  = (uint32_t)bus;
	uint32_t lslot = (uint32_t)slot;
	uint32_t lfunc = (uint32_t)func;
	uint32_t tmp = 0;

	address = (uint32_t)((lbus << 16) | (lslot << 11) |
				(lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));

	outl(0xCF8, address);
	tmp = inl(0xCFC);
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

static pci_dev_t *devices = NULL;
static size_t n_devices = 0;

static pci_dev_t *add_device_to_list(void) {
	n_devices++;
	devices = krealloc(devices, n_devices * sizeof(pci_dev_t));
	return &devices[n_devices - 1];
}

static inline uint16_t get_vendor(uint8_t bus, uint8_t dev, uint8_t fun) {
	return pci_read_word(bus, dev, fun, 0) & 0xFFFF;
}

static inline uint16_t get_device(uint8_t bus, uint8_t dev, uint8_t fun) {
	return (pci_read_word(bus, dev, fun, 0) >> 16) & 0xFFFF;
}

static inline uint8_t get_header_type(uint8_t bus, uint8_t dev, uint8_t fun) {
	return (pci_read_word(bus, dev, fun, 0xC) & 0x00FF0000) >> 16;
}

static inline uint8_t get_base_class(uint8_t bus, uint8_t dev, uint8_t fun) {
	return (pci_read_word(bus, dev, fun, 0x8) & 0xFF000000) >> 24;
}

static inline uint8_t get_sub_class(uint8_t bus, uint8_t dev, uint8_t fun) {
	return (pci_read_word(bus, dev, fun, 0x8) & 0x00FF0000) >> 16;
}

static inline uint8_t get_sec_bus(uint8_t bus, uint8_t dev, uint8_t fun) {
	return (pci_read_word(bus, dev, fun, 0x18) & 0x0000FF00) >> 8;
}

static void pci_bus_enum(uint8_t bus, pci_desc_t parent);
static int halt_on_irq_route_fail = 1;

static void pci_fun_check(uint8_t bus, uint8_t dev, uint8_t fun,
				pci_desc_t parent) {
	if (get_vendor(bus, dev, fun) == 0xFFFF)
		return; // not present

	if (get_base_class(bus, dev, fun) == 0x6 && get_sub_class(bus, dev, fun) == 0x4) {
		// bridge
		pci_desc_t d;
		d.bus = bus;
		d.dev = dev;
		d.fun = fun;
		pci_bus_enum(get_sec_bus(bus, dev, fun), d);
	}

	pci_dev_t *d = add_device_to_list();
	memset(d, 0, sizeof(pci_dev_t));

	d->parent = parent;
	d->desc.bus = bus;
	d->desc.dev = dev;
	d->desc.fun = fun;
	d->vendor = get_vendor(bus, dev, fun);
	d->device = get_device(bus, dev, fun);
	d->base_class = get_base_class(bus, dev, fun);
	d->sub_class = get_sub_class(bus, dev, fun);

	if (get_header_type(bus, dev, fun))
		return; // bridges dont have bars

	for (int i = 0; i < 6; i++) {
		uint16_t idx = 0x10 + i * 4;
		size_t len = 0;

		uint32_t bar_val = pci_read_word(bus, dev, fun, idx);

		pci_write_word(bus, dev, fun, idx, 0xFFFFFFFF);
		uint32_t raw_len = pci_read_word(bus, dev, fun, idx);
		pci_write_word(bus, dev, fun, idx, bar_val);

		int io = bar_val & 1;

		if (io)
			len = ~(raw_len & (~0x3)) + 1;
		else
			len = ~(raw_len & (~0xF)) + 1;

		if (!bar_val)
			continue;

		if (io) {
			d->bar[i].enabled = 1;
			d->bar[i].addr = (bar_val & (~0x3)) & 0xFFFF;
			d->bar[i].type = BAR_IO;
			d->bar[i].length = len & 0xFFFF;
		} else {
			uint8_t type = (bar_val >> 1) & 3;
			uint32_t top_bar = 0;

			if (type == 0x2) {
				// read next bar
				top_bar = pci_read_word(bus, dev, fun, idx + 4);
			}

			d->bar[i].enabled = 1;
			d->bar[i].addr = ((uintptr_t)top_bar << 32) | (bar_val & (~0xF));
			d->bar[i].type = BAR_MEM;
			d->bar[i].length = len;
			d->bar[i].prefetch = (bar_val >> 3) & 1;

			if (top_bar)
				i++;
		}
	}
}

static void pci_dev_check(uint8_t bus, uint8_t dev, pci_desc_t parent) {
	assert(dev < 32);

	uint8_t fun = 0;

	if (get_vendor(bus, dev, fun) == 0xFFFF)
		return; // not present

	pci_fun_check(bus, dev, fun, parent);

	if (get_header_type(bus, dev, fun) & 0x80) {
		for (fun = 1; fun < 8; fun++) {
			pci_fun_check(bus, dev, fun, parent);
		}
	}
}

static void pci_bus_enum(uint8_t bus, pci_desc_t parent) {
	for (uint8_t dev = 0; dev < 32; dev++) {
		pci_dev_check(bus, dev, parent);
	}
}

static void pci_bus_enum_all(void) {
	pci_desc_t desc = {0, 0, 0};
	if (!(get_header_type(0, 0, 0) & 0x80)) {
		pci_bus_enum(0, desc);
	} else {
		for (uint8_t fun = 0; fun < 8; fun++) {
			if (get_vendor(0, 0, fun) != 0xFFFF)
				break;

			desc.fun = fun;
			pci_bus_enum(fun, desc);
		}
	}
}

static inline int pci_dev2bridge_pin(int pin, int dev) {
	return (((pin - 1) + (dev % 4)) % 4) + 1;
}

static inline const char *pin_to_name(int pin) {
	static const char *names[] = {
		"none",
		"INTA#",
		"INTB#",
		"INTC#",
		"INTD#"
	};

	return names[pin];
}

static pci_dev_t *get_dev_by_desc(pci_desc_t desc) {
	for (size_t i = 0; i < n_devices; i++) {
		pci_dev_t *dev = &devices[i];
		if (dev->desc.bus == desc.bus &&
			dev->desc.dev == desc.dev &&
			dev->desc.fun == desc.fun)
			return dev;
	}

	return NULL;
}

static void pci_route_irqs_all(void) {
	for (size_t i = 0; i < n_devices; i++) {
		pci_dev_t *dev = &devices[i];

		uint8_t irq_pin;
		irq_pin = pci_read_word(dev->desc.bus, dev->desc.dev, dev->desc.fun, 0x3C) >> 8;

		if (!irq_pin)
			continue;

		if (dev->desc.bus) {
			// behind a bus
			kmesg("pci", "device %02x.%02x.%01x is behind a bus", dev->desc.bus, dev->desc.dev, dev->desc.fun);
			kmesg("pci", "it's interrupt line is %s", pin_to_name(irq_pin));
			kmesg("pci", "it's connected to line %s on the bridge", pin_to_name(pci_dev2bridge_pin(irq_pin, dev->desc.dev)));

			pci_desc_t bridge = dev->parent;
			uint8_t bridge_dev = dev->desc.dev;

			uint8_t pin = irq_pin;
			while(bridge.bus) {
				pin = pci_dev2bridge_pin(pin, bridge_dev);
				bridge_dev = bridge.dev;
				pci_dev_t *d = get_dev_by_desc(bridge);
				if (d->parent.bus)
					bridge = get_dev_by_desc(bridge)->parent;
				else
					break;
			}

			// bridge points to the pci-to-pci bridge connected to the pci root
			acpi_resource_t res;
			if (lai_pci_route_pin(&res, bridge.bus, bridge.dev, bridge.fun, pin)) {
				kmesg("pci", "routing irq for bridge "
					"%02x.%02x.%01x failed. %s", bridge.bus, bridge.dev, bridge.fun,
					halt_on_irq_route_fail ? "halting!" : "ignoring!");

				dev->irq = 0;
				if (halt_on_irq_route_fail)
					arch_cpu_halt_forever();
			} else {
				kmesg("pci", "routing irq for bridge "
					"%02x.%02x.%01x succeeded. "
					"device %02x.%02x.%01x routed to gsi %u",
					bridge.bus, bridge.dev, bridge.fun,
					dev->desc.bus, dev->desc.dev, dev->desc.fun, res.base);
				dev->irq = ioapic_get_vector_by_gsi(res.base);
			}
		} else {
			acpi_resource_t res;
			if (lai_pci_route_pin(&res, dev->desc.bus, dev->desc.dev, dev->desc.fun, irq_pin)) {
				kmesg("pci", "routing irq for device "
					"%02x.%02x.%01x failed. %s", dev->desc.bus, dev->desc.dev, dev->desc.fun,
					halt_on_irq_route_fail ? "halting!" : "ignoring!");

				dev->irq = 0;
				if (halt_on_irq_route_fail)
					arch_cpu_halt_forever();
			} else {
				kmesg("pci", "routing irq for device "
					"%02x.%02x.%01x succeeded. routed to gsi %u", dev->desc.bus, dev->desc.dev, dev->desc.fun, res.base);
				dev->irq = ioapic_get_vector_by_gsi(res.base);
			}
		}
	}
	arch_cpu_halt_forever();
}

void pci_init(void) {
	if (cmdline_has_value("pci", "no-halt"))
		halt_on_irq_route_fail = 0;

	pci_bus_enum_all();
	pci_route_irqs_all();

	for (size_t i = 0; i < n_devices; i++) {
		pci_dev_t *dev = &devices[i];
		kmesg("pci", "%02x.%02x.%01x %04x:%04x class %02x subclass %02x",
				dev->desc.bus, dev->desc.dev, dev->desc.fun, dev->vendor,
				dev->device, dev->base_class, dev->sub_class);
		for (int j = 0; j < 6; j++) {
			pci_bar_t *bar = &dev->bar[j];
			if (!bar->enabled)
				continue;
			kmesg("pci", "\tbar %u - %c%c %016lx %016lx",
					j, bar->type == BAR_IO ? 'i' : 'm',
					bar->prefetch ? 'p' : ' ', bar->addr, bar->length);
		}
	}
}

size_t pci_get_n_devices(void) {
	return n_devices;
}

pci_dev_t *pci_get_devices(void) {
	return devices;
}
