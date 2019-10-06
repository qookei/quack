#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <arch/task.h>

#define THREAD_STOPPED			(0 << 0)
#define THREAD_RUNNING			(1 << 0)
#define THREAD_BLOCKED_IPC		(1 << 1)
#define THREAD_BLOCKED_IRQ		(1 << 2)

struct thread {
	arch_task_t *task;
	int state;
	int running_on;
};

void sched_init(int n_cpu);
int sched_ready(void);

void sched_resched(uint8_t irq, void *irq_state, int resched);

int32_t sched_start(void);
int32_t sched_start_from_task(arch_task_t *task);
int32_t sched_clone(int32_t id);

void sched_destroy(int32_t id);

int32_t sched_start_from_elf(void *file);

void sched_set_state(int32_t id, int state);

#endif
