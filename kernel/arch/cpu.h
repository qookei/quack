#ifndef ARCH_CPU_H
#define ARCH_CPU_H

#if ARCH == x86_64 || ARCH == i386
#define ARCH_CPU_SPIN_PAUSE asm volatile ("pause")
#endif

void arch_cpu_dump_state(void *state);
void arch_cpu_trace_stack(void);
void arch_cpu_halt_forever(void);

void arch_cpu_atomic_loop_test_and_set(volatile int *var);
void arch_cpu_atomic_unset(volatile int *var);

int arch_cpu_get_this_id(void);

#endif
