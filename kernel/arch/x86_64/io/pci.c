#include "pci.h"

#include <stdint.h>
#include <stddef.h>
#include <io/port.h>
#include <util.h>
#include <mm/heap.h>
#include <string.h>
#include <kmesg.h>
#include <lai/helpers/pci.h>
#include <cpu/ioapic.h>
#include <arch/cpu.h>
#include <panic.h>

#include <cmdline.h>

uint8_t pci_read_byte(uint32_t bus, uint32_t slot, uint32_t func, uint16_t offset) {
	outd(0xCF8, (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFFFF) | 0x80000000);
	uint8_t v = inb(0xCFC + (offset % 4));
	return v;
}

void pci_write_byte(uint32_t bus, uint32_t slot, uint32_t func, uint16_t offset, uint8_t value) {
	outd(0xCF8, (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFFFF) | 0x80000000);
	outb(0xCFC + (offset % 4), value);
}

uint16_t pci_read_word(uint32_t bus, uint32_t slot, uint32_t func, uint16_t offset) {
	outd(0xCF8, (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFFFE) | 0x80000000);
	uint16_t v = inw(0xCFC + (offset % 4));
	return v;
}

void pci_write_word(uint32_t bus, uint32_t slot, uint32_t func, uint16_t offset, uint16_t value) {
	outd(0xCF8, (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFFFE) | 0x80000000);
	outw(0xCFC + (offset % 4), value);
}

uint32_t pci_read_dword(uint32_t bus, uint32_t slot, uint32_t func, uint16_t offset) {
	outd(0xCF8, (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFFFC) | 0x80000000);
	uint32_t v = ind(0xCFC + (offset % 4));
	return v;
}

void pci_write_dword(uint32_t bus, uint32_t slot, uint32_t func, uint16_t offset, uint32_t value) {
	outd(0xCF8, (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFFFC) | 0x80000000);
	outd(0xCFC + (offset % 4), value);
}

static pci_dev_t *devices = NULL;
static size_t n_devices = 0;

static size_t add_device_to_list(void) {
	n_devices++;
	devices = krealloc(devices, n_devices * sizeof(pci_dev_t));
	return n_devices - 1;
}

#define VENDOR_OFF 0
#define DEVICE_OFF 2
#define HEADER_TYPE_OFF 0xE
#define BASE_CLASS_OFF 0xB
#define SUB_CLASS_OFF 0xA
#define SUB_CLASS_OFF 0xA
#define SEC_BUS_OFF 0x19
#define BAR_OFF 0x10

static void pci_bus_enum(uint8_t bus, size_t parent_id);
static int halt_on_irq_route_fail = 1;

static void pci_fun_check(uint8_t bus, uint8_t dev, uint8_t fun,
				size_t parent_id) {
	if (pci_read_word(bus, dev, fun, VENDOR_OFF) == 0xFFFF)
		return; // not present

	size_t dev_id = add_device_to_list();
	pci_dev_t *d = &devices[dev_id];
	memset(d, 0, sizeof(pci_dev_t));

	d->parent = NULL;
	d->parent_id = parent_id;
	d->desc.bus = bus;
	d->desc.dev = dev;
	d->desc.fun = fun;
	d->vendor = pci_read_word(bus, dev, fun, VENDOR_OFF);
	d->device = pci_read_word(bus, dev, fun, DEVICE_OFF);
	d->base_class = pci_read_byte(bus, dev, fun, BASE_CLASS_OFF);
	d->sub_class = pci_read_byte(bus, dev, fun, SUB_CLASS_OFF);
	d->acpi_node = NULL;
	d->acpi_prt = (lai_variable_t)LAI_VAR_INITIALIZER;

	if (pci_read_byte(bus, dev, fun, HEADER_TYPE_OFF) & 0x7F) {
		for (int i = 0; i < 6; i++) {
			uint16_t idx = BAR_OFF + i * 4;
			size_t len = 0;

			uint32_t bar_val = pci_read_dword(bus, dev, fun, idx);

			pci_write_dword(bus, dev, fun, idx, 0xFFFFFFFF);
			uint32_t raw_len = pci_read_dword(bus, dev, fun, idx);
			pci_write_dword(bus, dev, fun, idx, bar_val);

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
					top_bar = pci_read_dword(bus, dev, fun, idx + 4);
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

	if (pci_read_byte(bus, dev, fun, BASE_CLASS_OFF) == 0x6 &&
			pci_read_byte(bus, dev, fun, SUB_CLASS_OFF) == 0x4) {
		// bridge
		pci_bus_enum(pci_read_byte(bus, dev, fun, SEC_BUS_OFF), dev_id);
	}
}

static void pci_dev_check(uint8_t bus, uint8_t dev, size_t parent_id) {
	assert(dev < 32);

	uint8_t fun = 0;

	if (pci_read_word(bus, dev, fun, VENDOR_OFF) == 0xFFFF)
		return; // not present

	pci_fun_check(bus, dev, fun, parent_id);

	if (pci_read_byte(bus, dev, fun, HEADER_TYPE_OFF) & 0x80) {
		for (fun = 1; fun < 8; fun++) {
			pci_fun_check(bus, dev, fun, parent_id);
		}
	}
}

static void pci_bus_enum(uint8_t bus, size_t parent_id) {
	for (uint8_t dev = 0; dev < 32; dev++) {
		pci_dev_check(bus, dev, parent_id);
	}
}

static void pci_bus_enum_all(void) {
	if (!(pci_read_byte(0, 0, 0, HEADER_TYPE_OFF) & 0x80)) {
		pci_bus_enum(0, 0xFFFFFFFFFFFFFFFF);
	} else {
		for (uint8_t fun = 0; fun < 8; fun++) {
			if (pci_read_word(0, 0, fun, VENDOR_OFF) != 0xFFFF)
				break;

			pci_bus_enum(fun, 0xFFFFFFFFFFFFFFFF);
		}
	}
}

static inline int pci_dev2bridge_pin(int pin, int dev) {
	return (((pin - 1) + (dev % 4)) % 4) + 1;
}

#define PCI_PNP_ID		"PNP0A03"
#define PCIE_PNP_ID		"PNP0A08"

typedef struct {
	lai_nsnode_t *node;
	uint8_t bus;
	lai_variable_t prt;
} pci_root_bus_t;

static pci_root_bus_t *pci_bus_cache;
static size_t n_pci_bus_cache;

static pci_root_bus_t *pci_add_bus_to_cache() {
	n_pci_bus_cache++;
	pci_bus_cache = krealloc(pci_bus_cache, n_pci_bus_cache * sizeof(pci_root_bus_t));
	return &pci_bus_cache[n_pci_bus_cache - 1];
}

static pci_root_bus_t *pci_get_root_bus_node(uint8_t bus) {
	for (size_t i = 0; i < n_pci_bus_cache; i++)
		if (pci_bus_cache[i].bus == bus)
			return &pci_bus_cache[i];
	return NULL;
}

// This function iterates over the ACPI namespace to find all PCI(e)
// root busses, and sets their pci_root_bus_t->node fields to the corresponding node.
static void pci_find_root_busses(lai_state_t *state) {
	LAI_CLEANUP_VAR lai_variable_t pci_pnp_id = LAI_VAR_INITIALIZER;
	LAI_CLEANUP_VAR lai_variable_t pcie_pnp_id = LAI_VAR_INITIALIZER;
	lai_eisaid(&pci_pnp_id, PCI_PNP_ID);
	lai_eisaid(&pcie_pnp_id, PCIE_PNP_ID);

	lai_nsnode_t *sb_handle = lai_resolve_path(NULL, "\\_SB_");
	assert(sb_handle);
	struct lai_ns_child_iterator iter = LAI_NS_CHILD_ITERATOR_INITIALIZER(sb_handle);
	lai_nsnode_t *node;
	while ((node = lai_ns_child_iterate(&iter))) {
		if (lai_check_device_pnp_id(node, &pci_pnp_id, state) &&
			lai_check_device_pnp_id(node, &pcie_pnp_id, state)) {
				continue;
		}

		pci_root_bus_t *b = pci_add_bus_to_cache();
		memset(b, 0, sizeof(pci_root_bus_t));
		b->prt = (lai_variable_t)LAI_VAR_INITIALIZER;
		b->node = node;

		// this is a root bus
		LAI_CLEANUP_VAR lai_variable_t bus_number = LAI_VAR_INITIALIZER;
		uint64_t bbn_result = 0;
		lai_nsnode_t *bbn_handle = lai_resolve_path(node, "_BBN");
		if (bbn_handle) {
			if (lai_eval(&bus_number, bbn_handle, state)) {
				continue;
			}
			lai_obj_get_integer(&bus_number, &bbn_result);
		}

		b->bus = bbn_result;

		lai_nsnode_t *prt_handle = lai_resolve_path(node, "_PRT");
		if (!prt_handle) {
			kmesg("pci", "root bus %02x has no prt...", bbn_result);
		} else {
			if (lai_eval(&b->prt, prt_handle, state)) {
				kmesg("pci", "failed to eval prt for root bus %02x...", bbn_result);
			} else {
				kmesg("pci", "found a prt for root bus %02x", bbn_result);
			}
		}
	}
}

static void pci_find_node(pci_dev_t *dev, lai_state_t *state) {
	if (dev->acpi_node) {
		return;
	}
	if (dev->parent) {
		pci_find_node(dev->parent, state);
	}

	lai_nsnode_t *bus;
	pci_root_bus_t *r;
	if (!(r = pci_get_root_bus_node(dev->desc.bus))) {
		// this device is not on a root bus
		if (!dev->parent)
			panic(NULL, "device %02x.%02x.%01x not on root bus does not have a parent node!",
					dev->desc.bus, dev->desc.dev, dev->desc.fun);
		bus = dev->parent->acpi_node;
	} else {
		// this device is on a root bus
		bus = r->node;
	}

	dev->acpi_node = lai_pci_find_device(bus, dev->desc.dev, dev->desc.fun, state);
}

static void pci_route_irqs_all(void) {
	LAI_CLEANUP_STATE lai_state_t state;
	lai_init_state(&state);

	// find parents for devices (done now to avoid resetting pointers
	// after krealloc)
	for (size_t i = 0; i < n_devices; i++) {
		pci_dev_t *dev = &devices[i];
		if (dev->parent_id == 0xFFFFFFFFFFFFFFFF)
			continue;
		pci_dev_t *parent = &devices[dev->parent_id];
		dev->parent = parent;
	}

	pci_find_root_busses(&state);

	// find nodes for child devices
	for (size_t i = 0; i < n_devices; i++) {
		pci_dev_t *dev = &devices[i];
		pci_find_node(dev, &state);

		if (dev->acpi_node) {
			// attempt to get the prt
			if (!lai_obj_get_type(&dev->acpi_prt)) {
				lai_nsnode_t *prt_handle = lai_resolve_path(dev->acpi_node, "_PRT");
				if (prt_handle) {
					if (lai_eval(&dev->acpi_prt, prt_handle, &state)) {
						kmesg("pci", "failed to eval "
							"prt for %02x.%02x.%01x...",
							dev->desc.bus,
							dev->desc.dev,
							dev->desc.fun);
					} else {
						kmesg("pci", "found a prt "
							"for %02x.%02x.%01x...",
							dev->desc.bus,
							dev->desc.dev,
							dev->desc.fun);
					}
				}
			} else {
				kmesg("pci", "%02x.%02x.%01x already has a prt...",
					dev->desc.bus,
					dev->desc.dev,
					dev->desc.fun);
			}
		}
	}

	// finally route interrupts
	for (size_t i = 0; i < n_devices; i++) {
		pci_dev_t *dev = &devices[i];

		uint8_t irq_pin;
		irq_pin = pci_read_byte(dev->desc.bus, dev->desc.dev, dev->desc.fun, 0x3D);

		if (!irq_pin)
			continue;

		pci_dev_t *tmp = dev;
		lai_variable_t *prt = NULL;

		while(1) {
			if (tmp->parent) {
				// not on a root hub
				// does the parent have a prt?
				if (!lai_obj_get_type(&tmp->parent->acpi_prt)) {
					// no
					// translate irq
					irq_pin = pci_dev2bridge_pin(irq_pin, tmp->desc.dev);
				} else {
					// yes, we're done
					prt = &tmp->parent->acpi_prt;
					break;
				}
				tmp = tmp->parent;
			} else {
				// on one of the root hubs
				pci_root_bus_t *bus = pci_get_root_bus_node(tmp->desc.bus);
				prt = &bus->prt;
				break;
			}
		}

		assert(prt && "failed to find prt");

		struct lai_prt_iterator iter = LAI_PRT_ITERATOR_INITIALIZER(prt);
		lai_api_error_t err;

		int found = 0;

		while (!(err = lai_pci_parse_prt(&iter))) {
			if (iter.slot == tmp->desc.dev &&
				(iter.function == tmp->desc.fun || iter.function == -1) &&
				iter.pin == (irq_pin - 1)) { // pin - 1 since acpi starts from 0 and pci from 1
				// TODO: care about flags for the IRQ

				dev->irq = ioapic_get_vector_by_gsi(iter.gsi);
				kmesg("pci", "device %02x.%02x.%01x successfully routed to gsi %u (irq %u)",
						dev->desc.bus, dev->desc.dev, dev->desc.fun, iter.gsi, dev->irq);
				found = 1;
				break;
			}
		}

		if (!found) {
			kmesg("pci", "routing failed for device %02x.%02x.%01x",
					dev->desc.bus, dev->desc.dev, dev->desc.fun);
			if (!dev->parent)
				kmesg("pci", "\tdevice is on a root bus");
			else {
				pci_dev_t *tmp = dev->parent;
				size_t i = 1;
				while (tmp) {
					kmesg("pci", "\tparent bridge #%u:");
					kmesg("pci", "\t\t%02x.%02x.%01x",
							tmp->desc.bus, tmp->desc.dev, tmp->desc.fun);
					if (lai_obj_get_type(&tmp->acpi_prt)) {
						kmesg("pci", "\t\thas a prt, not looking further");
						kmesg("pci", "\t\tprt type = %u", lai_obj_get_type(&tmp->acpi_prt));
						break;
					} else {
						kmesg("pci", "\t\tdoes not have a prt");
					}
					tmp = tmp->parent;
					i++;
				}
				kmesg("pci", "\tthere were %u bridges between the device and the root hub", i);
			}
		}
	}
}

void pci_init(void) {
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
