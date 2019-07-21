#include "isr.h"
#include <irq/idt.h>
#include <io/port.h>
#include <kmesg.h>
#include <panic.h>
#include <cpu/cpu.h>
#include <cpu/cpu_data.h>

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

void dispatch_interrupt(irq_cpu_state_t *state) {
	uint32_t irq = state->int_no;

	uint32_t cpu = 0; // TODO: get cpu id
	cpu_data_t *cpu_data = cpu_data_get_for_cpu(cpu);

	if (irq >= 0x20 && irq < 0x30) {
		// spurious pic irq
		// shouldn't happen, pic is masked
		cpu_data->spurious_pic++;
		kmesg("irq", "spurious pic interrupt on cpu %u", cpu);
	}

	int success = 0;
	if (irq_handlers[irq])
		success = irq_handlers[irq](state);

	if (!success) {
		if (irq < 0x20) {
			// cpu exception
			if (state->cs == 0x08) {
				panic(state, "unhandled exception %s (%u) error %02x", exc_names[irq], irq, state->err);
			} else {
				// user mode
				kmesg("irq", "user mode panic!");
				kmesg("irq", "how did we get here at this point!");
			}
		}
	}

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
