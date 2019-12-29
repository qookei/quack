#include "lapic.h"
#include <kmesg.h>
#include <panic.h>
#include <acpi/acpi.h>
#include <arch/mm.h>
#include <string.h>
#include <irq/isr.h>
#include <io/port.h>
#include <cpu/ioapic.h>

#define LVT_LINT0 0x350
#define LVT_LINT1 0x360

#define LAPIC_EOI 0xB0
#define LAPIC_SIVR 0xF0

#define LVT_TIMER 0x320
#define DIVIDE_CONFIG 0x3E0
#define INITIAL_COUNT 0x380
#define CURRENT_COUNT 0x390

#define LAPIC_TIMER_1X_DIV 0xB

// vector 0, non periodic, fixed delivery mode
#define CALIB_TIMER_CONFIG 0

static uintptr_t lapic_base;

uint32_t lapic_read(uint32_t offset) {
	return *(volatile uint32_t *)(lapic_base + offset);
}

void lapic_write(uint32_t offset, uint32_t val) {
	*(volatile uint32_t *)(lapic_base + offset) = val;
}

static void lapic_nmi_set(uint8_t vector, uint8_t lint, uint16_t flags) {
	uint32_t nmi = vector | 0x320;

	if (flags & 2)
		nmi |= (1 << 13);

	if (flags & 8)
		nmi |= (1 << 15);

	uint32_t off;

	switch(lint) {
		case 0:
			off = LVT_LINT0;
			break;
		case 1:
			off = LVT_LINT1;
			break;
		default:
			panic(NULL, "Invalid lint %u for nmi", lint);
	}

	lapic_write(off, nmi);
}

static void lapic_nmi_setup(void) {
	madt_nmi_t *nmis = madt_get_nmis();
	for (size_t i = 0; i < madt_get_nmi_count(); i++) {
		lapic_nmi_set(0xE0 + i, nmis[i].lint, nmis[i].flags);
	}
}

void lapic_eoi(void) {
	lapic_write(LAPIC_EOI, 0);
}

void lapic_enable(void) {
	lapic_write(LAPIC_SIVR, lapic_read(LAPIC_SIVR) | 0x1FF);
}

void lapic_init(void) {
	lapic_base = madt_get_lapic_base();

	arch_mm_map_kernel((void *)lapic_base, (void *)lapic_base, 4,
			ARCH_MM_FLAG_R | ARCH_MM_FLAG_W, ARCH_MM_CACHE_UC);

	kmesg("lapic", "setting up the lapic");
	lapic_nmi_setup();
	lapic_enable();
}

static uint64_t lapic_bsp_speed_hz;

static volatile uint64_t pit_ticks = 0;
static int __attribute__ ((noinline)) pit_irq(irq_cpu_state_t *s) {
	(void)s;

	pit_ticks++;

	return 1;
}

static void pit_set_freq(uint32_t frequency) {
	uint16_t x = 1193182 / frequency;
	if ((1193182 % frequency) > (frequency / 2))
		x++;

	outb(0x40, (uint8_t)(x & 0x00ff));
	io_wait();
	outb(0x40, (uint8_t)(x / 0x0100));
	io_wait();
}

void lapic_timer_calc_freq_bsp(void) {
	if (lapic_bsp_speed_hz)
		return; // already calculated

	uint8_t pit_vec = ioapic_get_vector_by_irq(0x0);
	pit_set_freq(1000);
	isr_register_handler(pit_vec, pit_irq);

	lapic_write(LVT_TIMER, CALIB_TIMER_CONFIG);
	lapic_write(DIVIDE_CONFIG, LAPIC_TIMER_1X_DIV);
	lapic_write(INITIAL_COUNT, 0xFFFFFFFF);

	asm volatile ("sti");
	pit_ticks = 0;
	while(pit_ticks < 1000);
	asm volatile ("cli");

	lapic_bsp_speed_hz = 0xFFFFFFFF - lapic_read(CURRENT_COUNT);

	isr_unregister_handler(pit_vec);

	kmesg("lapic-timer", "bsp timer speed is %luHz", lapic_bsp_speed_hz);
}

void lapic_timer_set_frequency(uint64_t timer_freq, uint64_t desired_freq) {
	uint64_t period = timer_freq / desired_freq;

	lapic_write(INITIAL_COUNT, period);

	kmesg("lapic-timer", "setting frequency to %luHz, period %lu", desired_freq, period);
}

volatile uint64_t lapic_bsp_ticks = 0;

static int lapic_timer_int(irq_cpu_state_t *s) {
	lapic_bsp_ticks++;

	return 1;
}

#define INITIAL_FREQ 1000 // Hz

void lapic_timer_init_bsp(void) {
	uint32_t vec = 0x31 | (1 << 17);
	lapic_write(LVT_TIMER, vec);
	lapic_write(DIVIDE_CONFIG, LAPIC_TIMER_1X_DIV);

	lapic_timer_set_frequency(lapic_bsp_speed_hz, INITIAL_FREQ);

	isr_register_handler(0x31, lapic_timer_int);
}

void lapic_sleep_ms_bsp(uint64_t ms) {
	volatile uint64_t end_tick = lapic_bsp_ticks + ms;
	while (end_tick > lapic_bsp_ticks);
}

static uint32_t lapic_timer_calc_freq_ap(void) {
	if (!lapic_bsp_speed_hz)
		panic(NULL, "lapic_..._ap called before lapic_..._bsp");

	lapic_write(LVT_TIMER, CALIB_TIMER_CONFIG);
	lapic_write(DIVIDE_CONFIG, LAPIC_TIMER_1X_DIV);
	lapic_write(INITIAL_COUNT, 0xFFFFFFFF);

	lapic_sleep_ms_bsp(1000);

	uint32_t lapic_speed_hz = 0xFFFFFFFF - lapic_read(CURRENT_COUNT);

	kmesg("lapic-timer", "ap timer speed is %luHz", lapic_speed_hz);

	return lapic_speed_hz;
}


void lapic_timer_init_ap(void) {
	uint32_t speed = lapic_timer_calc_freq_ap();

	uint32_t vec = 0x31 | (1 << 17);
	lapic_write(LVT_TIMER, vec);
	lapic_write(DIVIDE_CONFIG, LAPIC_TIMER_1X_DIV);

	lapic_timer_set_frequency(speed, INITIAL_FREQ);
}
