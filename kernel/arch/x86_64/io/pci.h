#ifndef PCI_H
#define PCI_H

#include <stdint.h>
#include <stddef.h>
#include <lai/core.h>

#include <new>
#include <frg/vector.hpp>
#include <frg/optional.hpp>
#include <lib/frg_allocator.h>

#define BAR_IO 1
#define BAR_MEM 2

uint8_t pci_read_byte(uint32_t, uint32_t, uint32_t, uint16_t);
void pci_write_byte(uint32_t, uint32_t, uint32_t, uint16_t, uint8_t);

uint16_t pci_read_word(uint32_t, uint32_t, uint32_t, uint16_t);
void pci_write_word(uint32_t, uint32_t, uint32_t, uint16_t, uint16_t);

uint32_t pci_read_dword(uint32_t, uint32_t, uint32_t, uint16_t);
void pci_write_dword(uint32_t, uint32_t, uint32_t, uint16_t, uint32_t);

struct pci_bar {
	enum class bar_type {
		io,
		mmio,
	};

	bar_type type;
	uintptr_t addr;
	size_t length;

	bool prefetch;
};

enum class pci_reg : uint16_t {
	vendor = 0x00,
	device = 0x02,
	header_type = 0x0E,
	base_class = 0x0B,
	sub_class = 0x0A,
	prog_if = 0x09,
	sec_bus = 0x19,
	bar0_off = 0x10,
	irq_pin = 0x3D,
};

struct pci_bus;

struct pci_dev {
	pci_dev(uint8_t bus, uint8_t slot, uint8_t func);

	pci_dev(const pci_dev &) = delete;
	pci_dev &operator=(const pci_dev &) = delete;

	bool exists();

	void init();

	frg::optional<pci_bar> fetch_bar(int bar);

	void attach_to(pci_bus *b);

	void route_irq(lai_state_t *state);

	uint8_t bus() const;
	uint8_t slot() const;
	uint8_t func() const;

	uint16_t vendor() const;
	uint16_t device() const;

	uint8_t base_class() const;
	uint8_t sub_class() const;
	uint8_t prog_if() const;

	uint8_t header_type() const;
	bool multifunction() const;

	uint8_t sec_bus() const;
	uint8_t irq_pin() const;

	uint8_t irq() const;
	uint8_t gsi() const;

	pci_bus *parent();

private:
	pci_bus *_parent;

	uint8_t _bus;
	uint8_t _slot;
	uint8_t _func;

	uint16_t _vendor;
	uint16_t _device;

	uint8_t _base_class;
	uint8_t _sub_class;
	uint8_t _prog_if;

	uint8_t _header_type;
	bool _multifunction;

	uint8_t _sec_bus;
	uint8_t _irq_pin;

	uint8_t _irq;
	uint8_t _gsi;
};

struct pci_bus {
	pci_bus(uint8_t bus, pci_dev *dev = nullptr);

	void set_acpi_node(lai_nsnode_t *node);
	void find_node(lai_state_t *state);

	lai_variable_t *prt();
	void fetch_prt(lai_state_t *state);
	bool has_prt() const;

	void enumerate(lai_state_t *state);
	void attach_device(pci_dev *dev);
	void attach_to(pci_bus *bus);

	pci_bus *parent();
	pci_dev *device();
private:
	pci_bus *_parent;

	pci_dev *_device;
	frg::vector<pci_dev *, frg_allocator> _children;

	uint8_t _bus;

	bool _has_prt;
	bool _tried_prt_fetch;
	lai_nsnode_t *_acpi_node;
	lai_variable_t _acpi_prt;
};

void pci_init(void);
/*
size_t pci_get_n_devices(void);
pci_dev *pci_get_devices(void);
*/
#endif
