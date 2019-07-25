#include "smp.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <kmesg.h>
#include <util.h>

#include <acpi/acpi.h>

#include <cpu/lapic.h>

#include <irq/idt.h>

#include <arch/mm.h>
#include <mm/pmm.h>
#include <mm/mm.h>
#include <cpu/gdt.h>
#include <cpu/cpu_data.h>
#include <cpu/cpu.h>
#include <cpu/ctx.h>

#define KERNEL_SMP 0x400000

// trampoline data
extern void *_trampoline_start;
extern void *_trampoline_end;
extern void *smp_entry;


static volatile int has_started = 0;

static void smp_c_entry(uint64_t core_id, uint64_t apic_id) {
	kmesg("smp", "core %lu started", core_id);
	kmesg("smp", "core %lu has lapic id %lu", core_id, apic_id);

	cpu_data_t *d = cpu_data_get(core_id);
	gdt_load(d->gdt);
	gdt_load_tss(GDT_TSS_SEL);
	gdt_load_gs_base((uintptr_t)d);

	idt_just_load();
	lapic_init();

	has_started = 1;

	test_foo_bar();

	while(1);
}

#define STACK_SIZE 0x10000

static uintptr_t alloc_stack(void) {
	uintptr_t ptr = (uintptr_t)pmm_alloc(STACK_SIZE / PAGE_SIZE);
	return ptr + VIRT_PHYS_BASE;
}

static uintptr_t prepare_data(int cpu, uint8_t lapic_id) {
	cpu_data_t *d = cpu_data_get(cpu);
	d->cpu_id = cpu;
	d->lapic_id = lapic_id;
	uintptr_t stk = d->stack_ptr = alloc_stack();

	d->tss = tss_create_new();
	tss_update_stack(d->tss, stk, 0);
	d->gdt = gdt_create_new(d->tss);

	return stk;
}

static int smp_init_single(uint32_t apic_id, uint32_t core_id) {
	ptrdiff_t len = (uintptr_t)&_trampoline_end - (uintptr_t)&_trampoline_start;

	arch_mm_map_kernel(-1, &_trampoline_start, &_trampoline_start,
			(len + PAGE_SIZE - 1) / PAGE_SIZE,
			ARCH_MM_FLAGS_READ | ARCH_MM_FLAGS_WRITE);

	memcpy(&_trampoline_start, (void *)(KERNEL_SMP + VIRT_PHYS_BASE), len);

	uintptr_t pml4 = (uintptr_t)arch_mm_get_ctx_kernel(-1);

	assert(!(pml4 & 0xFFFFFFFF00000000));

	uintptr_t stack = prepare_data(core_id, apic_id);

	uint64_t *data = (uint64_t *)(0x500 + VIRT_PHYS_BASE);
	data[0] = pml4;
	data[1] = stack;
	data[2] = (uintptr_t)(&smp_c_entry);
	data[3] = core_id;
	data[4] = apic_id;

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

	asm volatile("cli");

	if (!has_started) {
		kmesg("smp", "failed to start core %u!", core_id);
		return 0;
	} else {
		kmesg("smp", "successfully started core %u", core_id);
		return 1;
	}
}

static void setup_bsp(void) {
	prepare_data(0, 0);
	cpu_data_t *d = cpu_data_get(0);
	gdt_load(d->gdt);
	gdt_load_tss(GDT_TSS_SEL);
	gdt_load_gs_base((uintptr_t)d);
}

static int smp_core_count;

void smp_init(void) {
	setup_bsp();

	kmesg("smp", "bsp's id is %d", cpu_get_id());

	madt_lapic_t *l = madt_get_lapics();
	for (size_t i = 0; i < madt_get_lapic_count(); i++) {
		if (!(l[i].flags & 1))
			continue;

		if (!l[i].apic_id)
			continue;

		if (!smp_init_single(l[i].apic_id, smp_core_count + 1))
			continue;

		smp_core_count++;
	}

	kmesg("smp", "init done, %u working cores found", smp_core_count);

	//asm volatile("cli");
}
