#include "syscall.h"

#include <sched/sched.h>
#include <io/serial.h>

void exit_handler(uintptr_t *exit_code,
			uintptr_t *unused1, uintptr_t *unused2,
			void *unused3) {
	(void)unused1;
	(void)unused2;
	(void)unused3;

	sched_kill(sched_get_current()->pid, *exit_code, SIGTERM);
}

void getpid_handler(uintptr_t *pid,
			uintptr_t *unused1, uintptr_t *unused2,
			void *unused3) {
	(void)unused1;
	(void)unused2;
	(void)unused3;

	*pid = sched_get_current()->pid;
}

void waitpid_handler(uintptr_t *pid,
			uintptr_t *unused1, uintptr_t *unused2, void *state) {
	(void)unused1;
	(void)unused2;

	task_waitpid(state, *pid, sched_get_current());
	task_switch_to(sched_schedule_next());
}

void ipc_wait_handler(uintptr_t *unused1,
			uintptr_t *unused2, uintptr_t *unused3,
			void *state) {
	(void)unused1;
	(void)unused2;
	(void)unused3;

	task_waitipc(state, sched_get_current());
	task_switch_to(sched_schedule_next());
}

void sched_spawn_new_handler(uintptr_t *pid,
			uintptr_t *is_privileged, uintptr_t *unused1,
			void *unused2) {
	(void)unused1;
	(void)unused2;

	if (!sched_get_current()->is_privileged)
		return;

	pid_t child = sched_task_spawn(sched_get_task(*pid), *is_privileged);
	*pid = child;
}

void sched_make_ready_handler(uintptr_t *pid,
			uintptr_t *entry, uintptr_t *stack,
			void *unused1) {
	(void)unused1;

	if (!sched_get_current()->is_privileged)
		return;

	sched_task_make_ready(sched_get_task(*pid), *entry, *stack);
}

void get_phys_from_handler(uintptr_t *pid, uintptr_t *virt, uintptr_t *phys, void *unused1) {
	(void)unused1;

	if (!sched_get_current()->is_privileged)
		return;

	uintptr_t pg_tab = sched_get_task(*pid)->cr3;

	void *addr = get_phys(pg_tab, (void *)(*virt));

	*phys = (uintptr_t)addr;
}

void map_mem_to_handler(uintptr_t *pid, uintptr_t *virt, uintptr_t *phys, void *unused1) {
	(void)unused1;

	if (!sched_get_current()->is_privileged)
		return;

	if (*virt >= 0xC0000000)
		return;

	uintptr_t pg_tab = sched_get_task(*pid)->cr3;

	set_cr3(pg_tab);
	map_page((void *)(*phys), (void *)(*virt), 0x7);
	set_cr3(def_cr3());
}

void unmap_mem_from_handler(uintptr_t *pid, uintptr_t *virt, uintptr_t *unused1, void *unused2) {
	(void)unused1;
	(void)unused2;

	if (!sched_get_current()->is_privileged)
		return;

	if (*virt >= 0xC0000000)
		return;

	uintptr_t pg_tab = sched_get_task(*pid)->cr3;

	set_cr3(pg_tab);
	unmap_page((void *)(*virt));
	set_cr3(def_cr3());
}

void alloc_mem_phys_handler(uintptr_t *phys, uintptr_t *unused1, uintptr_t *unused2, void *unused3) {
	(void)unused1;
	(void)unused2;
	(void)unused3;

	if (!sched_get_current()->is_privileged) {
		*phys = 0x0;
		return;
	}

	*phys = (uintptr_t)pmm_alloc();
}

void sbrk_handler(uintptr_t *increment, uintptr_t *break_ptr, uintptr_t *unused1, void *unused2) {
	(void)unused1;
	(void)unused2;

	*break_ptr = (uintptr_t)task_sbrk(*increment, sched_get_current());
}

void ipc_send_handler(uintptr_t *pid, uintptr_t *size, uintptr_t *data, void *unused1) {
	(void)unused1;

	if (*size > 8388608) {		// 8 MiB limit
		*size = -1;
		return;
	}

	if (!verify_addr(sched_get_current()->cr3, *data, *size, 0x5)) {
		*size = -1;
		return;
	}

	void *kdata = kmalloc(*size);
	copy_from_user(kdata, (void *)(*data), *size);

	if (!task_ipcsend(sched_get_task(*pid), sched_get_current(), *size, kdata))
		*size = -1;
}

void ipc_recv_handler(uintptr_t *unused1, uintptr_t *size, uintptr_t *data, void *unused2) {
	(void)unused1;
	(void)unused2;

	void *rec_data;
	size_t rec_size = task_ipcrecv(&rec_data, sched_get_current());

	if (!(*data)) {
		*size = rec_size;
		return;
	}

	if (!verify_addr(sched_get_current()->cr3, *data, *size, 0x7)) {
		*size = -1;
		return;
	}

	copy_to_user((void *)(*data), rec_data, rec_size);

	*size = rec_size;
}

void ipc_remove_handler(uintptr_t *unused1, uintptr_t *unused2, uintptr_t *unused3, void *unused4) {
	(void)unused1;
	(void)unused2;
	(void)unused3;
	(void)unused4;

	task_ipcremov(sched_get_current());
}

void ipc_queue_length_handler(uintptr_t *len, uintptr_t *unused1, uintptr_t *unused2, void *unused3) {
	(void)unused1;
	(void)unused2;
	(void)unused3;

	*len = task_ipcqueuelen(sched_get_current());
}

void ipc_get_sender_handler(uintptr_t *sender, uintptr_t *unused1, uintptr_t *unused2, void *unused3) {
	(void)unused1;
	(void)unused2;
	(void)unused3;

	*sender = task_ipcgetsender(sched_get_current());
}

void debug_log_handler(uintptr_t *message, uintptr_t *length, uintptr_t *unused1, void *unused2) {
	(void)unused1;
	(void)unused2;

	if (!sched_get_current()->is_privileged)
		return;

	serial_write_byte('[');
	serial_write_byte('u');
	serial_write_byte(']');
	serial_write_byte(' ');
	
	char *buf = kmalloc(*length);
	if (copy_from_user(buf, (void *)(*message), *length)) {
		for (size_t i = 0; i < *length; i++)
			serial_write_byte(buf[i]);
	}

	kfree(buf);
}

void is_privileged_handler(uintptr_t *pid, uintptr_t *is_privileged, uintptr_t *unused1, void *unused2) {
	(void)unused1;
	(void)unused2;

	if (!sched_get_current()->is_privileged || !sched_get_task(*pid)) {
		*is_privileged = -1;
		return;
	}

	*is_privileged = sched_get_task(*pid)->is_privileged;
}

void sched_prioritize_handler(uintptr_t *pid, uintptr_t *unused1, uintptr_t *unused2, void *unused3) {
	(void)unused1;
	(void)unused2;
	(void)unused3;

	if (!sched_get_task(*pid) || !sched_get_task(*pid)->is_privileged) {
		return;
	}

	sched_move_after(sched_get_current(), sched_get_task(*pid));
}

#include <mesg.h>

void register_handler_handler(uintptr_t *int_no, uintptr_t *unused1, uintptr_t *unused2, void *unused3) {
	(void)unused1;
	(void)unused2;
	(void)unused3;

	if (!sched_get_current()->is_privileged) {
		return;
	}

	register_userspace_handler(*int_no, sched_get_current()->pid);
}

void unregister_handler_handler(uintptr_t *int_no, uintptr_t *unused1, uintptr_t *unused2, void *unused3) {
	(void)unused1;
	(void)unused2;
	(void)unused3;

	if (!sched_get_current()->is_privileged) {
		return;
	}

	unregister_userspace_handler(*int_no, sched_get_current()->pid);
}

void waitirq_handler(uintptr_t *unused1, uintptr_t *unused2, uintptr_t *unused3, void *state) {
	(void)unused1;
	(void)unused2;
	(void)unused3;

	if (!sched_get_current()->is_privileged) {
		return;
	}

	task_waitirq((interrupt_cpu_state *)state, sched_get_current());
	task_switch_to(sched_schedule_next());
}
