#ifndef ARCH_CPU_H
#define ARCH_CPU_H

#include <stdint.h>

#if ARCH == x86_64 || ARCH == i386
#define ARCH_CPU_SPIN_PAUSE asm volatile ("pause")
#endif

void arch_cpu_dump_state(void *state);
void arch_cpu_trace_stack(void);
void arch_cpu_halt_forever(void);

int arch_cpu_get_this_id(void);
int arch_cpu_get_count(void);

void arch_cpu_ack_interrupt(uint8_t irq);

#endif
