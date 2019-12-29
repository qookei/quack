#include "generic_irq.h"
#include <proc/scheduler.h>

void timer_irq_sink(uint8_t irq, void *irq_state) {
	if (sched_ready()) {
		sched_resched(irq, irq_state, 0);
	}
}

void resched_signal_sink(uint8_t irq, void *irq_state) {
	sched_resched(irq, irq_state, 1);
}
