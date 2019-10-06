#include "scheduler.h"
#include <spinlock.h>
#include <mm/heap.h>
#include <string.h>
#include <panic.h>
#include <util.h>
#include <arch/cpu.h>
#include <loader/elf64.h>
#include <stdatomic.h>
#include <arch/mm.h>

// thread and id allocation

static spinlock_t threads_lock;
static struct thread **threads;
static int32_t n_threads;

// returned id may not be valid now, but may be valid after calling enlarge_table
static int32_t find_free_id(void) {
	int32_t i = 0;

	while (i < n_threads && threads[i])
		i++;

	return i;
}

#define DEFAULT_SIZE 16

static int32_t enlarge_table(void) {
	size_t old_count = n_threads;
	n_threads *= 2;

	if (!n_threads)
		n_threads = DEFAULT_SIZE;

	threads = krealloc(threads, sizeof(*threads) * n_threads);
	memset(threads + old_count, 0, sizeof(*threads) * (old_count ? old_count : DEFAULT_SIZE));

	return old_count;
}

static int32_t picker_idx = 0;

static int32_t get_next_thread_to_run(void) {
	int32_t picked_idx = -1;

	// this is to make sure we dont get stuck in
	// an infinite loop on an empty run list
	int32_t traversed = 0;

	while (picked_idx == -1) {
		if (!threads)
			break;

		if (threads[picker_idx] // exists?
				&& threads[picker_idx]->state == THREAD_RUNNING // is running?
				&& threads[picker_idx]->running_on == -1) // is not already scheduled
			picked_idx = picker_idx;

		if (++picker_idx >= n_threads)
			picker_idx = 0; // loop around

		traversed++;
		if (traversed > n_threads)
			break;
	}

	return picked_idx;
}

// main scheduler code

static int n_cpus;

struct sched_cpu_data {
	int32_t current_thread;
};

static struct sched_cpu_data *cpu_data;

static _Atomic int ready = 0;

void sched_init(int n_cpu) {
	if (!n_cpu)
		panic(NULL, "sched_init called with no processors");

	n_cpus = n_cpu;
	cpu_data = kmalloc(n_cpu * sizeof(struct sched_cpu_data));

	for (int i = 0; i < n_cpus; i++)
		cpu_data[i].current_thread = -1;

	ready = 1;
}

int sched_ready(void) {
	return ready;
}

void sched_resched(uint8_t irq, void *irq_state, int resched) {
	int cpu = arch_cpu_get_this_id();
	assert(cpu < n_cpus);

	struct sched_cpu_data *data = &cpu_data[cpu];
	int32_t thread_id = data->current_thread;

	spinlock_lock(&threads_lock);

	if (thread_id != -1 && !resched) {
		arch_task_save_irq_state(threads[thread_id]->task, irq_state);
		threads[thread_id]->running_on = -1;
	}

	int32_t next_thread = get_next_thread_to_run();
	data->current_thread = next_thread;

	// nothing to run, idle
	if (next_thread == -1) {
		spinlock_release(&threads_lock);

		// TODO: wrap this in an arch function
		arch_mm_drop_context();
		arch_cpu_ack_interrupt(irq);

		arch_task_idle_cpu();
	}

	threads[next_thread]->running_on = cpu;

	// running the same process
	if (next_thread == thread_id) {
		spinlock_release(&threads_lock);
		return;
	}

	spinlock_release(&threads_lock);

	// TODO: wrap this in an arch function
	arch_mm_drop_context();
	arch_cpu_ack_interrupt(irq);

	arch_task_switch_to(threads[next_thread]->task);
}

int32_t sched_start(void) {
	spinlock_lock(&threads_lock);

	int32_t i = find_free_id();

	if (i >= n_threads)
		i = enlarge_table();

	threads[i] = kcalloc(sizeof(struct thread), 1);
	threads[i]->running_on = -1;

	spinlock_release(&threads_lock);
	return i;
}

int32_t sched_start_from_task(arch_task_t *task) {
	int32_t i = sched_start();
	spinlock_lock(&threads_lock);

	threads[i]->task = task;

	spinlock_release(&threads_lock);
	return i;
}

int32_t sched_clone(int32_t id) {
	(void)id;
	assert(!"TODO: implement sched_clone()");
	__builtin_unreachable();
}

void sched_destroy(int32_t id) {
	if(id == -1)
		return;

	spinlock_lock(&threads_lock);

	arch_task_destroy(threads[id]->task);
	kfree(threads[id]);
	threads[id] = NULL;

	spinlock_release(&threads_lock);
}

int32_t sched_start_from_elf(void *file) {
	// TODO: use arch-agnostic functions (elf_xxx instead of elf64_xxx) 

	if (elf64_check(file))
		return -1;

	return sched_start_from_task(elf64_create_arch_task(file));
}

void sched_set_state(int32_t id, int state) {
	if(id == -1)
		return;

	spinlock_lock(&threads_lock);

	threads[id]->state = state;

	spinlock_release(&threads_lock);
}
