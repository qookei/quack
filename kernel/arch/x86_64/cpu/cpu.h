#ifndef CPU_H
#define CPU_H

#include <irq/isr.h>

void cpu_dump_regs_irq(irq_cpu_state_t *);
int cpu_get_id(void);

void cpu_set_msr(uint32_t msr, uint64_t val);
uint64_t cpu_get_msr(uint32_t msr);

#endif
