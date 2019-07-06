#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <kmesg.h>
#include <util.h>

#include <cpu/lapic.h>

#include <arch/mm.h>
#include <mm/mm.h>

#define KERNEL_SMP 0x400000

extern void *_trampoline_start;
extern void *_trampoline_end;

extern void *smp_entry;

void lapic_write(uint32_t offset, uint32_t val);

volatile static int has_started = 0;

static void smp_c_entry(void) {
	kmesg("smp", "core started and entered C code");

	has_started = 1;

	while(1);
}

static uintptr_t alloc_stack(void) {
	uintptr_t ptr = (uintptr_t)pmm_alloc(0x10);
	return ptr + VIRT_PHYS_BASE;
}

void smp_init_single(uint32_t apic_id, uint32_t core_id) {
	ptrdiff_t len = (uintptr_t)&_trampoline_end - (uintptr_t)&_trampoline_start;

	arch_mm_map_kernel(-1, &_trampoline_start, &_trampoline_start,
			(len + PAGE_SIZE - 1) / PAGE_SIZE,
			ARCH_MM_FLAGS_READ | ARCH_MM_FLAGS_WRITE);

	memcpy(&_trampoline_start, (void *)(KERNEL_SMP + VIRT_PHYS_BASE), len);

	uintptr_t pml4 = (uintptr_t)arch_mm_get_ctx_kernel(-1);

	assert(!(pml4 & 0xFFFFFFFF00000000));

	uint64_t *data = (uint64_t *)(0x500 + VIRT_PHYS_BASE);
	data[0] = pml4;
	data[1] = alloc_stack();
	data[2] = (uintptr_t)(&smp_c_entry);

	has_started = 0;

	asm volatile("sti");

	kmesg("smp", "starting core %u (apic %u)!", core_id, apic_id);
	lapic_write(0x310, apic_id << 24);
	lapic_write(0x300, 0x500);

	lapic_sleep_ms(10);

	lapic_write(0x310, apic_id << 24);
	lapic_write(0x300,
		0x600 | (uint32_t)((uintptr_t)&smp_entry / PAGE_SIZE));

	size_t i = 0;
	while(i < 1000 && !has_started) {
		lapic_sleep_ms(100);
		i++;
	}

	if (!has_started) {
		kmesg("smp", "failed to start core %u!", core_id);
	} else {
		kmesg("smp", "successfully started core %u", core_id);
	}

	asm volatile("cli");
}
