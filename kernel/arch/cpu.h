#ifndef ARCH_CPU_H
#define ARCH_CPU_H

void arch_cpu_dump_state(void *state);
void arch_cpu_trace_stack(void);
void arch_cpu_halt_forever(void);

#endif
