#ifndef GENERIC_IRQ_H
#define GENERIC_IRQ_H

#include <stdint.h>

void page_fault_sink(uintptr_t address, void *irq_state);
void timer_irq_sink(uint8_t irq, void *irq_state);
void resched_signal_sink(uint8_t irq, void *irq_state);

#endif
