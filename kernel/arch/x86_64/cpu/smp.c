#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <kmesg.h>

#include <cpu/lapic.h>

#include <arch/mm.h>
#include <mm/mm.h>

#define KERNEL_SMP 0x400000

extern void *_trampoline_start;
extern void *_trampoline_end;

extern void *smp_entry;

void lapic_write(uint32_t offset, uint32_t val);

void smp_init_single(uint32_t apic_id, uint32_t core_id) {
	ptrdiff_t len = (uintptr_t)&_trampoline_end - (uintptr_t)&_trampoline_start;

	kmesg("smp", "trampoline is %ld bytes long", len);
	kmesg("smp", "trampoline start: %016p", &_trampoline_start);
	kmesg("smp", "smp entry: %016p", &smp_entry);

	arch_mm_map_kernel(-1, &_trampoline_start, &_trampoline_start, (len + PAGE_SIZE - 1) / PAGE_SIZE, ARCH_MM_FLAGS_READ | ARCH_MM_FLAGS_WRITE);

	memcpy(&_trampoline_start, (void *)(KERNEL_SMP + VIRT_PHYS_BASE), len);

	for(size_t i = 0; i < len; i++) {
		kmesg("smp", "@ %u = %02x", i, ((uint8_t *)(&_trampoline_start))[i]);
	}

	asm volatile("sti");

	kmesg("smp", "starting core %u (apic %u)!", core_id, apic_id);
	lapic_write(0x310, apic_id << 24);
	lapic_write(0x300, 0x500);

	lapic_sleep_ms(10);

	lapic_write(0x310, apic_id << 24);
	lapic_write(0x300, 0x1600);
	lapic_sleep_ms(100000);
	//kmesg("smp", "core supposedly started!");
	while(1);
}
