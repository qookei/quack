#include "isr.h"
#include <irq/idt.h>
#include <io/port.h>

irq_handler_t irq_handlers[IDT_ENTRIES];

void logf(const char *, ...);		// XXX: remove

void irq_eoi(uint8_t irq) {
	if (irq >= 0x20 && irq < 0x30) {
		if (irq >= 0x28)
			outb(0xA0, 0x20);

		outb(0x20, 0x20);
	}
}

void dispatch_interrupt(irq_cpu_state_t *state) {
	uint32_t irq = state->int_no;

	uint32_t ip_high = state->rip >> 32;
	uint32_t ip_low = state->rip & 0xFFFFFFFF;

	int success = 0;
	if (irq_handlers[irq])
		success = irq_handlers[irq](state);

	if (!success) {
		if (irq < 0x20) {
			// cpu exception
			if (state->cs == 0x08) {
				// kernel fault, panic
				logf("quack: kernel panic!\n");
				logf("quack: TODO: dump registers and stack!\n");
				while(1) asm volatile("hlt");
			} else {
				// user mode
				logf("quack: user mode panic!\n");
				logf("quack: how did we get here at this point!\n");
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
