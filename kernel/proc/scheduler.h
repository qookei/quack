#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <new>
#include <lib/frg_allocator.h>
#include <frg/unique.hpp>
#include <frg/string.hpp>
#include <arch/task.h>
#include <mm/vm.h>
#include <spinlock.h>

#define THREAD_STOPPED			(0 << 0)
#define THREAD_RUNNING			(1 << 0)
#define THREAD_BLOCKED_IPC		(1 << 1)
#define THREAD_BLOCKED_IRQ		(1 << 2)

enum class state {
	stopped,
	running,
	blocked_ipc,
	blocked_irq,
};

struct thread {
	thread(frg::unique_ptr<arch_task, frg_allocator> &&task,
		frg::unique_ptr<address_space, frg_allocator> &&addr_space);

	frg::unique_ptr<arch_task, frg_allocator> _task;
	frg::unique_ptr<address_space, frg_allocator> _addr_space;

	uintptr_t _id;
	state _state;
	int _running_on;

	spinlock _access_lock;

	frg::default_list_hook<thread> _list_node;

	frg::string<frg_allocator> _identity;
};

struct thread_access_guard {
	thread_access_guard(thread *t)
	:_thread{t} {}

	~thread_access_guard() {
		_thread->_access_lock.unlock();
	}

	thread *get() {
		return _thread;
	}

	thread &operator*() {
		return *_thread;
	}

	thread *operator->() {
		return _thread;
	}
private:
	thread *_thread;
};

void sched_init(int n_cpus);
bool sched_ready(void);

void sched_resched(uint8_t irq, void *irq_state, int resched);

uint64_t sched_add(frg::unique_ptr<thread, frg_allocator> thread);
void sched_remove(uint64_t id);

void sched_run(uint64_t id);
void sched_stop(uint64_t id);
void sched_block(uint64_t id, state reason);

thread_access_guard sched_get(uint64_t id);
thread_access_guard sched_this();

void sched_page_fault(uintptr_t addr, void *irq_state);

#endif
