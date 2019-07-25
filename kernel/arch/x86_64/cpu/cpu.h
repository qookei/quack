#ifndef CPU_H
#define CPU_H

#include <irq/isr.h>

void cpu_dump_regs_irq(irq_cpu_state_t *);
int cpu_get_id(void);

#endif
