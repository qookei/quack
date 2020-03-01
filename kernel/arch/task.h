#ifndef ARCH_TASK_T
#define ARCH_TASK_T

#include <stddef.h>
#include <stdint.h>

struct arch_task_impl;

struct arch_task {
	arch_task();
	~arch_task();

	uintptr_t &ip();
	uintptr_t &sp();
	void *&vmm_ctx();

	void load_irq_state(void *state);

	[[noreturn]] void enter();

private:
	arch_task_impl *_impl_data;
};

void arch_task_idle_cpu(void);

#endif
