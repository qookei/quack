#include <lai/host.h>

#include <mm/heap.h>
#include <kmesg.h>
#include <io/port.h>
#include <mm/mm.h>
#include "acpi.h"
#include <string.h>
#include <io/pci.h>
#include <panic.h>
#include <util.h>

void *laihost_malloc(size_t len) {
	void *mem = kmalloc(len);
	memset(mem, 0, len);
	return mem;
}

void *laihost_realloc(void *ptr, size_t len) {
	void *mem = krealloc(ptr, len);
	memset(mem, 0, len);
	return mem;
}

extern uintptr_t top;

void laihost_free(void *ptr) {
	kfree(ptr);
}

void laihost_log(int level, const char *msg) {
	(void)level;

	kmesg("lai", "%s", msg);
}

__attribute__((noreturn)) void laihost_panic(const char *msg) {
	kmesg("lai", "%s", msg);
	arch_cpu_trace_stack();
	arch_cpu_halt_forever();
	__builtin_unreachable();
}

void *laihost_scan(const char *sig, size_t idx) {
	return acpi_find_table(sig, idx);
}

void *laihost_map(size_t addr, size_t count) {
	(void)count;
	if ((addr + count) > 0xFFFFFFFF)
		panic(NULL, "laihost_map called with address,count pair which goes out of bounds");
	return (void *)(addr + VIRT_PHYS_BASE);
}

void laihost_outb(uint16_t port, uint8_t val) {
	outb(port, val);
}

void laihost_outw(uint16_t port, uint16_t val) {
	outw(port, val);
}

void laihost_outd(uint16_t port, uint32_t val) {
	outd(port, val);
}

uint8_t laihost_inb(uint16_t port) {
	return inb(port);
}

uint16_t laihost_inw(uint16_t port) {
	return inw(port);
}

uint32_t laihost_ind(uint16_t port) {
	return ind(port);
}

void laihost_pci_writeb(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t fun, uint16_t off, uint8_t val) {
	assert(!seg && "TODO");
	pci_write_byte(bus, dev, fun, off, val);
}

uint8_t laihost_pci_readb(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t fun, uint16_t off) {
	assert(!seg && "TODO");
	return pci_read_byte(bus, dev, fun, off);
}

void laihost_pci_writew(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t fun, uint16_t off, uint16_t val) {
	assert(!seg && "TODO");
	pci_write_word(bus, dev, fun, off, val);
}

uint16_t laihost_pci_readw(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t fun, uint16_t off) {
	assert(!seg && "TODO");
	return pci_read_word(bus, dev, fun, off);
}

void laihost_pci_writed(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t fun, uint16_t off, uint32_t val) {
	assert(!seg && "TODO");
	pci_write_dword(bus, dev, fun, off, val);
}

uint32_t laihost_pci_readd(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t fun, uint16_t off) {
	assert(!seg && "TODO");
	return pci_read_dword(bus, dev, fun, off);
}

void laihost_sleep(uint64_t time) {}
