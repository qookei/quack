#include "isr.h"
#include <irq/idt.h>
#include <io/port.h>
#include <kmesg.h>
#include <panic.h>
#include <cpu/cpu.h>
#include <cpu/cpu_data.h>
#include <mm/vmm.h>
#include <arch/mm.h>
#include <arch/cpu.h>

#include <generic_irq.h>
#include <cpu/lapic.h>

irq_handler_t irq_handlers[IDT_ENTRIES];

void irq_eoi(uint8_t irq) {
	if (irq >= 0x20 && irq < 0x30) {
		if (irq >= 0x28)
			outb(0xA0, 0x20);

		outb(0x20, 0x20);
	}

	if (irq >= 0x30)
		lapic_eoi();
}

static const char *exc_names[] = {
		"#DE", "#DB", "NMI", "#BP", "#OF", "#BR", "#UD", "#NM",
		"#DF", "-",   "#TS", "#NP", "#SS", "#GP", "#PF", "-",
		"#MF", "#AC", "#MC", "#XM", "#VE", "-",   "-",   "-",
		"-",   "-",   "-",   "-",   "-",   "-",   "#SX", "-", "-"
};

void exit_interrupt(uint8_t irq, int restore_ctx);

#define CHECK_MCE_STATE
#define IA32_MCG_STATUS 0x17A
#define IA32_MCG_CAP 0x179
#define IA32_MCi_CTL 0x400
#define IA32_MCi_STATUS 0x401
#define IA32_MCi_ADDR 0x402
#define IA32_MCi_MISC 0x403

extern "C" void dispatch_interrupt(irq_cpu_state_t *state) {
	vmm_save_context();
	vmm_set_context((pt_t *)arch_mm_get_ctx_kernel());

	uint32_t irq = state->int_no;

	uint32_t cpu = cpu_get_id();
	cpu_data_t *cpu_data = cpu_data_get(cpu);

	if (irq >= 0x20 && irq < 0x30) {
		// spurious pic irq
		// shouldn't happen, pic is masked
		cpu_data->spurious_pic++;
		kmesg("irq", "spurious pic interrupt on cpu %u", cpu);
	}

	int success = 0;
	if (irq_handlers[irq])
		success = irq_handlers[irq](state);

	// FIXME: this is kinda sorta a hack
	if (irq == 0x31) {
		success = 1;
		timer_irq_sink(irq, state);
	}

	if (irq == 18) {
		kmesg("irq", "machine check exception occured");
		#ifdef CHECK_MCE_STATE
		uint64_t mcg_status = cpu_get_msr(IA32_MCG_STATUS);
		if (mcg_status & (1 << 0)) kmesg("mce", "restart IP valid");
		if (mcg_status & (1 << 1)) kmesg("mce", "error IP valid");
		if (mcg_status & (1 << 2)) kmesg("mce", "machine check in progress");
		if (mcg_status & (1 << 3)) kmesg("mce", "local machine check exception signalled");

		uint64_t mcg_ctl = cpu_get_msr(IA32_MCG_CAP);
		uint8_t err_bank_count = mcg_ctl;
		kmesg("mce", "%u error-reporting banks available", err_bank_count);

		for (size_t i = 0; i < err_bank_count; i++) {
			uint64_t mci_status = cpu_get_msr(IA32_MCi_STATUS + 4 * i);
			if (mci_status & (1ull << 63)) kmesg("mce", "bank %lu: MCi_STATUS valid", i);
			if (mci_status & (1ull << 62)) kmesg("mce", "bank %lu: error overflow", i);
			if (mci_status & (1ull << 61)) kmesg("mce", "bank %lu: uncorrected error", i);
			if (mci_status & (1ull << 60)) kmesg("mce", "bank %lu: error reporting enabled", i);
			if (mci_status & (1ull << 59)) kmesg("mce", "bank %lu: MCi_MISC valid", i);
			if (mci_status & (1ull << 58)) kmesg("mce", "bank %lu: MCi_ADDR valid", i);
			if (mci_status & (1ull << 57)) kmesg("mce", "bank %lu: processor context corrupted", i);

			if (mci_status & (1ull << 58)) {
				uint64_t mci_addr = cpu_get_msr(IA32_MCi_ADDR + 4 * i);
				kmesg("mce", "bank %lu: address that caused mce: %016lx", i, mci_addr);
			}
		}
		#elif
		kmesg("irq", "mce info parsing disabled");
		#endif
		panic(state, "halting due to MCE");
	}

	if (!success) {
		if (irq < 0x20) {
			// cpu exception
			if (state->cs == 0x08) {
				panic(state, "unhandled exception %s (%u) error %02x", exc_names[irq], irq, state->err);
			} else {
				// user mode
				if (irq == 0x0E) {
					uintptr_t cr2;
					asm volatile ("mov %%cr2, %0" : "=r"(cr2));
					page_fault_sink(cr2, state);
				} else {
					kmesg("irq", "user mode panic!");
					panic(state, "unhandled exception %s (%u) error %02x", exc_names[irq], irq, state->err);
				}
			}
		}
	}

	exit_interrupt(irq, 1);
}

void exit_interrupt(uint8_t irq, int restore_ctx) {
	irq_eoi(irq);
	if (restore_ctx)
		vmm_restore_context();
}

void arch_cpu_ack_interrupt(uint8_t irq) {
	irq_eoi(irq);
}

int isr_register_handler(uint8_t irq, irq_handler_t handler) {
	if (irq_handlers[irq]) return 0;
	irq_handlers[irq] = handler;
	return 1;
}

int isr_unregister_handler(uint8_t irq) {
	if (!irq_handlers[irq]) return 0;
	irq_handlers[irq] = NULL;
	return 1;
}
