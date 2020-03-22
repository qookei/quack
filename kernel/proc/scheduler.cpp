#include "scheduler.h"
#include <spinlock.h>
#include <frg/mutex.hpp>
#include <mm/heap.h>
#include <string.h>
#include <panic.h>
#include <util.h>
#include <arch/cpu.h>
#include <loader/elf64.h>
#include <arch/mm.h>

#include <frg/hash_map.hpp>
#include <frg/vector.hpp>

#include <atomic>

thread::thread(frg::unique_ptr<arch_task, frg_allocator> &&task,
		frg::unique_ptr<address_space, frg_allocator> &&addr_space)
: _task{std::move(task)}, _addr_space{std::move(addr_space)},
_id{0}, _state{state::stopped}, _running_on{-1}, _access_lock{},
_list_node{}, _identity{64, 0, frg_allocator::get()} {}

static spinlock threads_lock;

static frg::hash_map<
	uint64_t,
	thread *,
	frg::hash<uint64_t>,
	frg_allocator
> threads{frg::hash<uint64_t>{}, frg_allocator::get()};

static frg::intrusive_list<
	thread,
	frg::locate_member<
		thread,
		frg::default_list_hook<thread>,
		&thread::_list_node
	>
> schedule;

static std::atomic<uint64_t> next_id;

static uint64_t allocate_id() {
	return next_id++;
}

static frg::optional<uint64_t> get_next_thread_to_run(void) {
	static size_t idx = 0;
	size_t traversed = 0;

	while (traversed < 2) {
		size_t i = 0;
		for (auto thread : schedule) {
			if (i < idx) {
				i++;
				continue;
			}

			if (thread->_running_on == -1) {
				idx = i;
				return thread->_id;
			}

			i++;
		}

		idx = 0;
		traversed++;
	}

	return frg::null_opt;
}

struct sched_cpu_data {
	uint64_t current_thread;
	bool running;
};

static frg::vector<sched_cpu_data, frg_allocator> sched_procs{frg_allocator::get()};

static std::atomic_bool ready;

void sched_init(int n_cpus) {
	if (!n_cpus)
		panic(NULL, "sched_init called with no processors");

	for (int i = 0; i < n_cpus; i++)
		sched_procs.push_back({0, false});

	ready = true;
}

bool sched_ready(void) {
	return ready;
}

void sched_resched(uint8_t irq, void *irq_state, int resched) {
	int cpu = arch_cpu_get_this_id();

	sched_cpu_data &data = sched_procs[cpu];

	threads_lock.lock();

	if (data.running && !resched) {
		threads[data.current_thread]->_task->load_irq_state(irq_state);
		threads[data.current_thread]->_running_on = -1;
	}

	auto next_thread = get_next_thread_to_run();

	if (next_thread && data.running && *next_thread == data.current_thread) {
		data.running = true;
		threads[*next_thread]->_running_on = cpu;
		threads_lock.unlock();
		return;
	}

	// TODO: wrap this in an arch function
	arch_mm_drop_context();
	arch_cpu_ack_interrupt(irq);

	if (next_thread) {
		data.running = true;
		data.current_thread = *next_thread;
		threads[*next_thread]->_running_on = cpu;

		threads_lock.unlock();
		threads[*next_thread]->_task->enter();
	} else {
		data.running = false;
		threads_lock.unlock();
		arch_task_idle_cpu();
	}
}

uint64_t sched_add(frg::unique_ptr<thread, frg_allocator> thread) {
	frg::unique_lock guard{threads_lock};
	assert(thread->_state == state::stopped);

	uint64_t id = allocate_id();

	threads[id] = thread.release();

	return id;
}

void sched_remove(uint64_t id) {
	frg::unique_lock guard{threads_lock};
	for (auto thread : schedule) {
		if (thread->_id == id) {
			schedule.erase(thread);
			break;
		}
	}

	auto item = threads.remove(id);
	if (item)
		delete *item;
}

void sched_run(uint64_t id) {
	frg::unique_lock guard{threads_lock};

	schedule.push_back(threads[id]);
}

void sched_stop(uint64_t id) {
	sched_block(id, state::stopped);
}

void sched_block(uint64_t id, state reason) {
	frg::unique_lock guard{threads_lock};

	for (auto thread : schedule) {
		if (thread->_id == id) {
			schedule.erase(thread);
			break;
		}
	}

	sched_stop(id);

	threads[id]->_state = reason;
}

void sched_page_fault(uintptr_t address, void *irq_state) {
	int cpu = arch_cpu_get_this_id();
	sched_cpu_data &data = sched_procs[cpu];

	assert(data.running);

	auto thread = threads[data.current_thread];
	if (!thread->_addr_space->fault_hit(address))
		panic(irq_state, "TODO: handle killing the thread on fault");
}

thread_access_guard sched_get(uint64_t id) {
	auto thread = threads[id];
	assert(thread);

	thread->_access_lock.lock();

	return thread_access_guard{thread};
}

thread_access_guard sched_this() {
	int cpu = arch_cpu_get_this_id();
	sched_cpu_data &data = sched_procs[cpu];

	assert(data.running);

	return sched_get(data.current_thread);
}
