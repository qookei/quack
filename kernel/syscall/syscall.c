#include "syscall.h"
#include <mesg.h>

/*
	process:
	exit(return value)
	getpid() -> pid
	is_privileged(pid) -> -1 on error, is pid privileged

	wait:
	waitpid(pid) -> return status, signal, -1 if not a child
	ipc_wait() -> none
	waitirq() -> none

	sched:
	sched_spawn_new(parent pid, is privileged)	->	new pid
	sched_make_ready(pid, entry, stack) -> none
	sched_prioritize(pid) -> none

	mem:
	get_phys_from(pid, virt) -> phys page, NULL on error
	map_mem_to(pid, phys, virt) -> none
	unmap_mem_from(pid, virt) -> none
	alloc_mem_phys() -> phys page, NULL on error
	sbrk(increment) -> break ptr

	ipc:
	ipc_send(pid, size, data) -> status
	ipc_recv(dst) -> size(status on error)
	ipc_remove() -> none
	ipc_queue_length() -> length(status on error)
	ipc_get_sender() -> message sender, -1 if queue is empty

	misc:
	debug_log(msg, len) -> none

	irq:
	register_handler(int_no) -> none
	unregister_handler(int_no) -> none
*/

typedef void (*syscall_handler)(uintptr_t *, uintptr_t *, uintptr_t *, void *);

syscall_handler handlers[] = {
	exit_handler,
	getpid_handler,
	waitpid_handler,
	ipc_wait_handler,
	sched_spawn_new_handler,
	sched_make_ready_handler,
	get_phys_from_handler,
	map_mem_to_handler,
	unmap_mem_from_handler,
	alloc_mem_phys_handler,
	sbrk_handler,
	ipc_send_handler,
	ipc_recv_handler,
	ipc_remove_handler,
	ipc_queue_length_handler,
	ipc_get_sender_handler,
	debug_log_handler,
	is_privileged_handler,
	sched_prioritize_handler,
	register_handler_handler,
	unregister_handler_handler,
	waitirq_handler,
};

int do_syscall(interrupt_cpu_state *);

void syscall_init() {
	register_interrupt_handler(0x30, do_syscall);
}

int verify_addr(uint32_t pd, uint32_t addr, uint32_t len, uint32_t flags) {
	uint32_t caddr = addr;
	int failed = 0;

	while (caddr < addr + len) {
		uint32_t fl = get_flag(pd, (void *)caddr);
		if ((fl & flags) != flags) {
			failed = 1;
			break;
		}
		caddr += 0x1000;
	}

	return !failed;
}

int copy_to_user(void *dst, void *src, size_t len) {
	if (!dst || !src || !len)
		return 0;

	if (!verify_addr((uint32_t)(sched_get_current()->cr3), (uint32_t)dst, len, 0x7)) {
		return 0;
	}

	crosspd_memcpy(sched_get_current()->cr3, dst, def_cr3(), src, len);
	return 1;
}

int copy_from_user(void *dst, void *src, size_t len) {
	if (!dst || !src || !len)
		return 0;

	if (!verify_addr((uint32_t)(sched_get_current()->cr3), (uint32_t)src, len, 0x5)) {
		return 0;
	}

	crosspd_memcpy(def_cr3(), dst, sched_get_current()->cr3, src, len);
	return 1;
}

int do_syscall(interrupt_cpu_state *state) {
	if (state->eax < (sizeof(handlers) / sizeof(*handlers))
		&& handlers[state->eax])
		handlers[state->eax](&state->ebx, &state->ecx, &state->edx, state);

	return 1;
}
