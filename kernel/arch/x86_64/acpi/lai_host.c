#include <lai/host.h>

#include <mm/heap.h>
#include <kmesg.h>
#include <io/port.h>
#include <mm/mm.h>

void *laihost_malloc(size_t len) {
	return kmalloc(len);
}

void *laihost_realloc(void *ptr, size_t len) {
	return krealloc(ptr, len);
}

void laihost_free(void *ptr) {
	kfree(ptr);
}

void laihost_log(int level, const char *msg) {
	(void)level;
	kmesg("lai", "%s", msg);
}

__attribute__((noreturn)) void laihost_panic(const char *msg) {
	kmesg("lai", "fatal error!");
	kmesg("lai", "%s", msg);
	__builtin_trap();
}

void *laihost_scan(char *sig, size_t idx) {
	kmesg("laihost", "TODO scan");
	__builtin_trap();
}

void *laihost_map(size_t addr, size_t count) {
	(void)count;
	return (void *)(addr + VIRT_PHYS_BASE);
}

void laihost_outb(uint16_t port, uint8_t val) {
	outb(port, val);
}

void laihost_outw(uint16_t port, uint16_t val) {
	outw(port, val);
}

void laihost_outd(uint16_t port, uint32_t val) {
	outl(port, val);
}

uint8_t laihost_inb(uint16_t port) {
	return inb(port);
}

uint16_t laihost_inw(uint16_t port) {
	return inw(port);
}

uint32_t laihost_ind(uint16_t port) {
	return inl(port);
}

/* TODO: implement this once PCI stuff is there
void laihost_pci_write(uint8_t, uint8_t, uint8_t, uint16_t, uint32_t);
uint32_t laihost_pci_read(uint8_t, uint8_t, uint8_t, uint16_t);
void laihost_sleep(uint64_t);
*/
