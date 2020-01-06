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

#include <lib/unique.h>

uint8_t pci_read_byte(uint32_t bus, uint32_t slot, uint32_t func, uint16_t offset) {
	outd(0xCF8, (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFFFF) | 0x80000000);
	uint8_t v = inb(0xCFC + (offset % 4));
	return v;
}

uint8_t pci_read_byte(uint32_t bus, uint32_t slot, uint32_t func, pci_reg reg) {
	return pci_read_byte(bus, slot, func, (uint16_t)reg);
}

void pci_write_byte(uint32_t bus, uint32_t slot, uint32_t func, uint16_t offset, uint8_t value) {
	outd(0xCF8, (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFFFF) | 0x80000000);
	outb(0xCFC + (offset % 4), value);
}

void pci_write_byte(uint32_t bus, uint32_t slot, uint32_t func, pci_reg reg, uint8_t value) {
	pci_write_byte(bus, slot, func, (uint16_t)reg, value);
}

uint16_t pci_read_word(uint32_t bus, uint32_t slot, uint32_t func, uint16_t offset) {
	outd(0xCF8, (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFFFE) | 0x80000000);
	uint16_t v = inw(0xCFC + (offset % 4));
	return v;
}

uint16_t pci_read_word(uint32_t bus, uint32_t slot, uint32_t func, pci_reg reg) {
	return pci_read_word(bus, slot, func, (uint16_t)reg);
}

void pci_write_word(uint32_t bus, uint32_t slot, uint32_t func, uint16_t offset, uint16_t value) {
	outd(0xCF8, (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFFFE) | 0x80000000);
	outw(0xCFC + (offset % 4), value);
}

void pci_write_word(uint32_t bus, uint32_t slot, uint32_t func, pci_reg reg, uint16_t value) {
	pci_write_word(bus, slot, func, (uint16_t)reg, value);
}

uint32_t pci_read_dword(uint32_t bus, uint32_t slot, uint32_t func, uint16_t offset) {
	outd(0xCF8, (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFFFC) | 0x80000000);
	uint32_t v = ind(0xCFC + (offset % 4));
	return v;
}

uint32_t pci_read_dword(uint32_t bus, uint32_t slot, uint32_t func, pci_reg reg) {
	return pci_read_dword(bus, slot, func, (uint16_t)reg);
}

void pci_write_dword(uint32_t bus, uint32_t slot, uint32_t func, uint16_t offset, uint32_t value) {
	outd(0xCF8, (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFFFC) | 0x80000000);
	outd(0xCFC + (offset % 4), value);
}

void pci_write_dword(uint32_t bus, uint32_t slot, uint32_t func, pci_reg reg, uint32_t value) {
	pci_write_dword(bus, slot, func, (uint16_t)reg, value);
}

static frg::vector<unique_ptr<pci_dev>, frg_allocator> devices{frg_allocator::get()};
static frg::vector<unique_ptr<pci_bus>, frg_allocator> pci_buses{frg_allocator::get()};

pci_dev::pci_dev(uint8_t bus, uint8_t slot, uint8_t func)
:_parent{nullptr}, _bus{bus}, _slot{slot}, _func{func} {
}

bool pci_dev::exists() {
	return pci_read_word(_bus, _slot, _func, pci_reg::vendor) != 0xFFFF;
}

void pci_dev::init() {
	if (!exists())
		return;

	_vendor = pci_read_word(_bus, _slot, _func, pci_reg::vendor);
	_device = pci_read_word(_bus, _slot, _func, pci_reg::device);
	_base_class = pci_read_byte(_bus, _slot, _func, pci_reg::base_class);
	_sub_class = pci_read_byte(_bus, _slot, _func, pci_reg::sub_class);
	_prog_if = pci_read_byte(_bus, _slot, _func, pci_reg::prog_if);
	_header_type = pci_read_byte(_bus, _slot, _func, pci_reg::header_type);
	_irq_pin = pci_read_byte(_bus, _slot, _func, pci_reg::irq_pin);
	_sec_bus = pci_read_byte(_bus, _slot, _func, pci_reg::sec_bus);

	_irq = 0;
	_gsi = 0;
	_multifunction = (_header_type & 0x80);
}

frg::optional<pci_bar> pci_dev::fetch_bar(int bar) {
	if (_header_type & 0x7F)
		return frg::null_opt;

	uint16_t bar_off = (uint16_t)pci_reg::bar0_off + bar * 4;

	uint64_t bar_hi = 0, bar_lo;
	uint64_t bar_size_hi, bar_size_lo;
	uintptr_t base;
	size_t size;

	bar_lo = pci_read_dword(_bus, _slot, _func, bar_off);

	if (!bar_lo)
		return frg::null_opt;

	bool is_mmio = !(bar_lo & 1);
	bool is_prefetch = is_mmio && (bar_lo & 0b1000);
	bool is_64bit = is_mmio && ((bar_lo & 0b110) == 0b100);

	if (is_64bit)
		bar_hi = pci_read_dword(_bus, _slot, _func, bar_off + 4);

	base = ((bar_hi << 32) | bar_lo) & ~(is_mmio ? 0xFull : 0x3ull);

	pci_write_dword(_bus, _slot, _func, bar_off, 0xFFFFFFFF);
	bar_size_lo = pci_read_dword(_bus, _slot, _func, bar_off);
	pci_write_dword(_bus, _slot, _func, bar_off, bar_lo);

	if (is_64bit) {
		pci_write_dword(_bus, _slot, _func, bar_off + 4, 0xFFFFFFFF);
		bar_size_hi = pci_read_dword(_bus, _slot, _func, bar_off + 4);
		pci_write_dword(_bus, _slot, _func, bar_off + 4, bar_hi);
	} else {
		bar_size_hi = 0xFFFFFFFF;
	}

	size = ~(((bar_size_hi << 32) | bar_size_lo) & ~(is_mmio ? 0xFull : 0x3ull));

	if (!is_mmio) {
		base &= 0xFFFF;
		size &= 0xFFFF;
	}

	return pci_bar{is_mmio ? pci_bar::bar_type::mmio : pci_bar::bar_type::io,
		base, size, is_prefetch};
}

void pci_dev::attach_to(pci_bus *bus) {
	_parent = bus;
	bus->attach_device(this);
}

static inline int pci_bridge_pin(int pin, int dev) {
	return (((pin - 1) + (dev % 4)) % 4) + 1;
}

void pci_dev::route_irq(lai_state_t *state) {
	uint8_t irq_pin = _irq_pin;

	if (!irq_pin) {
		return;
	}

	auto bus = _parent;
	auto bus_dev = this;
	lai_variable_t *prt = NULL;

	while(1) {
		bus->fetch_prt(state);
		if (!bus->has_prt()) {
			irq_pin = pci_bridge_pin(irq_pin, bus->device()->slot());
		} else {
			prt = bus->prt();
			break;
		}

		if (bus->parent())
			bus_dev = bus->device();

		bus = bus->parent();
	}

	if (!prt)
		return;

	lai_prt_iterator iter = LAI_PRT_ITERATOR_INITIALIZER(prt);
	lai_api_error_t err;

	while (!(err = lai_pci_parse_prt(&iter))) {
		if (iter.slot == bus_dev->slot()
				&& (iter.function == bus_dev->func()
					|| iter.function == -1)
				&& iter.pin == (irq_pin - 1)) {
			// TODO: care about flags for the IRQ

			_irq = ioapic_get_vector_by_gsi(iter.gsi);
			_gsi = iter.gsi;
			break;
		}
	}
}

uint8_t pci_dev::bus() const {
	return _bus;
}

uint8_t pci_dev::slot() const {
	return _slot;
}

uint8_t pci_dev::func() const {
	return _func;
}

uint16_t pci_dev::vendor() const {
	return _vendor;
}

uint16_t pci_dev::device() const {
	return _device;
}

uint8_t pci_dev::base_class() const {
	return _base_class;
}

uint8_t pci_dev::sub_class() const {
	return _sub_class;
}

uint8_t pci_dev::prog_if() const {
	return _prog_if;
}

uint8_t pci_dev::header_type() const {
	return _header_type;
}

bool pci_dev::multifunction() const {
	return _multifunction;
}

uint8_t pci_dev::sec_bus() const {
	return _sec_bus;
}

uint8_t pci_dev::irq_pin() const {
	return _irq_pin;
}

uint8_t pci_dev::irq() const {
	return _irq;
}

uint8_t pci_dev::gsi() const {
	return _gsi;
}

pci_bus *pci_dev::parent() {
	return _parent;
}

pci_bus::pci_bus(uint8_t bus, pci_dev *dev)
: _parent{nullptr}, _device{dev}, _children{frg_allocator::get()}, _bus{bus} {
	kmesg("pci", "bruh, bus = %02x", bus);
}

void pci_bus::set_acpi_node(lai_nsnode_t *node) {
	_acpi_node = node;
}

void pci_bus::find_node(lai_state_t *state) {
	if (_acpi_node)
		return;

	if (_parent)
		_parent->find_node(state);

	if (_parent && !_parent->_acpi_node)
		return;

	assert(_parent);
	assert(_device);

	_acpi_node = lai_pci_find_device(_parent->_acpi_node, _device->slot(), _device->func(), state);
}

lai_variable_t *pci_bus::prt() {
	return &_acpi_prt;
}

void pci_bus::fetch_prt(lai_state_t *state) {
	if (_tried_prt_fetch)
		return;

	_tried_prt_fetch = true;

	if (!_acpi_node)
		return;

	lai_nsnode_t *prt_handle = lai_resolve_path(_acpi_node, "_PRT");
	if (prt_handle) {
		_has_prt = !lai_eval(&_acpi_prt, prt_handle, state);
	}
}

bool pci_bus::has_prt() const {
	return _has_prt;
}

void pci_bus::enumerate(lai_state_t *state) {
	kmesg("pci", "bus %02x enumerate", _bus);
	for (uint8_t slot = 0; slot < 32; slot++) {
		for (uint8_t func = 0; func < 8; func++) {
			auto d = std::move(make_unique<pci_dev>(_bus, slot, func));

			if (!d->exists())
				continue;

			d->init();
			d->attach_to(this);

			if (d->base_class() == 0x06 && d->sub_class() == 0x04) {
				auto bridge = std::move(make_unique<pci_bus>(d->sec_bus(), d.get()));
				bridge->attach_to(this);
				bridge->find_node(state);
				bridge->enumerate(state);

				pci_buses.push_back(std::move(bridge));
			}

			devices.push_back(std::move(d));
		}
	}
}

void pci_bus::attach_device(pci_dev *dev) {
	_children.push_back(std::move(dev));
}

void pci_bus::attach_to(pci_bus *bus) {
	_parent = bus;
}

pci_bus *pci_bus::parent() {
	return _parent;
}

pci_dev *pci_bus::device() {
	return _device;
}

// This function iterates over the ACPI namespace to find all PCI(e)
// root buses, and creates pci_bus objects for them.
static void pci_find_root_buses(lai_state_t *state) {
	LAI_CLEANUP_VAR lai_variable_t pci_pnp_id = LAI_VAR_INITIALIZER;
	LAI_CLEANUP_VAR lai_variable_t pcie_pnp_id = LAI_VAR_INITIALIZER;
	lai_eisaid(&pci_pnp_id, "PNP0A03");
	lai_eisaid(&pcie_pnp_id, "PNP0A08");

	lai_nsnode_t *sb_handle = lai_resolve_path(NULL, "\\_SB_");
	assert(sb_handle);
	lai_ns_child_iterator iter = LAI_NS_CHILD_ITERATOR_INITIALIZER(sb_handle);
	lai_nsnode_t *node;
	while ((node = lai_ns_child_iterate(&iter))) {
		if (lai_check_device_pnp_id(node, &pci_pnp_id, state) &&
			lai_check_device_pnp_id(node, &pcie_pnp_id, state)) {
				continue;
		}

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

		auto bus = std::move(make_unique<pci_bus>(bbn_result));
		bus->set_acpi_node(node);

		pci_buses.push_back(std::move(bus));
	}
}

void pci_init(void) {
	LAI_CLEANUP_STATE lai_state_t state;
	lai_init_state(&state);

	pci_find_root_buses(&state);

	size_t initial_count = pci_buses.size();
	for (size_t i = 0; i < initial_count; i++) {
		pci_buses[i]->enumerate(&state);
	}

	for (auto &dev : devices) {
		dev->route_irq(&state);
		if (dev->irq()) {
			kmesg("pci", "device %02x.%02x.%01x routed to gsi %u (irq %u)",
				dev->bus(), dev->slot(), dev->func(), dev->gsi(), dev->irq());
		}
	}

	for (auto &dev : devices) {
		kmesg("pci", "%02x.%02x.%01x %04x:%04x class %02x subclass %02x prog_if %02x",
				dev->bus(), dev->slot(), dev->func(), dev->vendor(),
				dev->device(), dev->base_class(), dev->sub_class(), dev->prog_if());

		for (int i = 0; i < 6; i++) {
			auto bar = dev->fetch_bar(i);
			if (!bar)
				continue;
			kmesg("pci", "\tbar %d - %c%c %016lx %016lx", i,
					bar->type == pci_bar::bar_type::io ? 'i' : 'm',
					bar->prefetch ? 'p' : ' ', bar->addr, bar->length);
		}
	}
}

/*
size_t pci_get_n_devices(void) {
	return devices.size();
}

pci_dev *pci_get_devices(void) {
	return devices.begin();
}
*/
