#include "task.h"
#include "elf.h"
#include <io/ports.h>
#include <multiboot.h>
#include <interrupt/isr.h>
#include <trace/stacktrace.h>
#include <panic.h>
#include <mesg.h>
#include <sched/sched.h>

void task_destroy_address_space(task_t *t) {
	set_cr3(t->cr3);

	uint32_t kernel_addr = (0xC0000000 >> 22);

	uint32_t *pd = (uint32_t *)0xFFFFF000;

	for (uint32_t i = 0; i < kernel_addr; i++) {
		uint32_t pt = pd[i];
		if ((pt & 0xFFF)) {
			map_page((void*)(pt & 0xFFFFF000), (void*)0xE0000000, 0x3);
			uint32_t *pt_p = (uint32_t *)0xE0000000;
			for(uint32_t j = 0; j < 1024; j++) {
				uint32_t addr = pt_p[j];
				if ((addr & 0xFFF))
					pmm_free((void*)(addr & 0xFFFFF000));
			}

			unmap_page((void*)0xE0000000);
			pmm_free((void *)(pt & 0xFFFFF000));
		}
	}

	set_cr3(def_cr3());

	destroy_page_directory((void *)t->cr3);
}

void task_save_cpu_state(interrupt_cpu_state *state, task_t *t) {
	if (!t)
		return;

	t->st.eax = state->eax;
	t->st.ebx = state->ebx;
	t->st.ecx = state->ecx;
	t->st.edx = state->edx;
	t->st.esi = state->esi;
	t->st.edi = state->edi;
	t->st.esp = state->esp;
	t->st.ebp = state->ebp;
	t->st.eip = state->eip;
	t->st.eflags = state->eflags;
	t->st.seg = state->ds;
	t->st.ss = state->ds;
	t->st.cs = state->cs;
}

void task_init() {
	register_interrupt_handler(0x20, task_int_handler);
}

void task_kill(task_t *t, int ret_val, int sig) {
	pid_t dead_pid = t->pid;
	task_t *p = t->parent;

	task_destroy_address_space(t);

	kfree(t);

	unregister_all_handlers_for_pid(dead_pid);

	// notify parent that child is dead	
	if (p) {
		if (sched_exists(p)) {
			if (p->waiting_status == WAIT_PROC) {
				if (((signed)p->waiting_info) <= 0 ||
					(p->waiting_info > 0 &&
					(signed)p->waiting_info == dead_pid)) {
					p->waiting_status = WAIT_NONE;
					p->waiting_info = 0;
					p->st.eax = ret_val;
					p->st.ebx = sig;
					sched_wake_up(p);
				}
			}
		}
	}
}

task_t *task_create_new(int is_privileged) {
	task_t *t = (task_t*)kmalloc(sizeof(task_t));
	memset(t, 0, sizeof(task_t));

	uintptr_t pd = create_page_directory();

	t->cr3 = pd;	

	t->st.seg = 0x23;
	t->st.cs = 0x1B;
	t->st.ss = 0x23;
	
	t->st.eflags = 0x202 | (is_privileged ? (0x3 << 12) : 0);
	
	t->ipc_message_queue = (ipc_message_t **)kmalloc(IPC_MAX_QUEUE * sizeof(ipc_message_t *));
	memset(t->ipc_message_queue, 0, IPC_MAX_QUEUE * sizeof(ipc_message_t *));

	t->is_privileged = is_privileged;

	return t;
}

int task_ipcsend(task_t *recv, task_t *send, uint32_t size, void *data) {
	if (!recv || !send)
		return 0;
		
	uint32_t i = 0;
	
	for (; i < IPC_MAX_QUEUE; i++) {
		if (!recv->ipc_message_queue[i])
			break;
	}
	
	if (i == IPC_MAX_QUEUE)
		return 0;

	recv->ipc_message_queue[i] = (ipc_message_t *)kmalloc(sizeof(ipc_message_t));
	recv->ipc_message_queue[i]->size = size;
	recv->ipc_message_queue[i]->data = data;
	recv->ipc_message_queue[i]->sender = send->pid;

	if (recv->waiting_status == WAIT_IPC) {
		recv->waiting_status = WAIT_NONE;
		sched_wake_up(recv);
	}

	return 1;
}

uint32_t task_ipcrecv(void **data, task_t *t) {
	if (!t->ipc_message_queue[0])
		return -1;

	if (data)	
		*data = t->ipc_message_queue[0]->data;
	return t->ipc_message_queue[0]->size;
}

void task_ipcremov(task_t *t) {
	if (!t->ipc_message_queue[0])
		return;
	
	kfree(t->ipc_message_queue[0]);
	
	uint32_t i = 1;
	
	for (; i < IPC_MAX_QUEUE; i++) {
		if (!t->ipc_message_queue[i])
			break;
	}
	
	for (uint32_t j = 1; j < i; j++) {
		t->ipc_message_queue[j - 1] = t->ipc_message_queue[j];
	}
	
	t->ipc_message_queue[i - 1] = NULL;
}

uint32_t task_ipcgetsender(task_t *t) {
	if (task_ipcqueuelen(t)) {
		return t->ipc_message_queue[0]->sender;
	}
	
	return 0xFFFFFFFF;
}

uint32_t task_ipcqueuelen(task_t *t) {
	for (uint32_t i = 0; i < IPC_MAX_QUEUE; i++) {
		if (!t->ipc_message_queue[i])
			return i;
	}
	
	return IPC_MAX_QUEUE;
}

extern volatile int is_servicing_driver;

void task_waitpid(interrupt_cpu_state *state, pid_t child, task_t *t) {
	task_save_cpu_state(state, t);
	
	t->waiting_status = WAIT_PROC;
	t->waiting_info = child;

	if (child != -1) {
		task_t *c = sched_get_task(child);

		if (!c || c->parent != t) {
			early_mesg(LEVEL_WARN, "task", "tried to wait on a nonexistent process or on someone else's child");
			t->waiting_status = WAIT_NONE;
			t->waiting_info = 0;
			t->st.eax = -1;
			return;
		}
	}

	if (is_servicing_driver)
		is_servicing_driver = 0;
	
	sched_suspend(t);
}

void task_waitipc(interrupt_cpu_state *state, task_t *t) {
	task_save_cpu_state(state, t);

	if (!task_ipcqueuelen(t)) {
		t->waiting_status = WAIT_IPC;
		is_servicing_driver = 0;
		sched_suspend(t);
	}
}

int task_waitirq(interrupt_cpu_state *state, task_t *t) {
	if (t->pending_irqs > 0)
		t->pending_irqs--;
	
	if (t->pending_irqs) {
		return 0;
	}
	
	task_save_cpu_state(state, t);

	t->waiting_status = WAIT_IRQ;
	t->pending_irqs = 0;
	is_servicing_driver = 0;
	
	sched_suspend(t);
	return 1;
}

int task_wakeup_irq(task_t *t) {
	if(t->waiting_status == WAIT_IRQ || t->pending_irqs) {
		t->waiting_status = WAIT_NONE;
		t->pending_irqs++;
		if (t->pending_irqs == 1) {
			sched_wake_up(t);
			return 1;
		}
	}

	return 0;
}

void task_init_heap(size_t size, task_t *t) {
	size_t pages = (size + 0x1000 - 1) / 0x1000;
	
	alloc_mem_at(t->cr3, 0x30000000, pages, 0x7);

	t->heap_pages = pages;
	t->heap_begin = 0x30000000;
	t->heap_end = 0x30000000 + size;
	
}

// TODO: replace with something better
// 		 allow processes to just choose an address?
// 		 anon mmap style function?
void *task_sbrk(int increment, task_t *t) {
	
	if (!increment) return (void *)t->heap_end;

	if (!t->heap_begin) {
		if (increment < 0) {
			return (void *)-1;
		} else {
			// init heap
			task_init_heap(increment, t);
			return (void *)t->heap_begin;
		}
	} else {
		t->heap_end += increment;
		if (increment < 0) {
			// decrement heap size
			if (t->heap_end < t->heap_begin) return (void *)-1;
			size_t new_sz = t->heap_end - t->heap_begin;
			size_t new_pages = (new_sz + 0x1000 - 1) / 0x1000;
			
			if (new_pages < t->heap_pages) {
				
				size_t pages_delete = t->heap_pages - new_pages;
				for (size_t i = 0; i < pages_delete; i++) {
					uint32_t addr = t->heap_begin + (new_pages + i) * 0x1000;
					
					void *phys = get_phys(t->cr3, (void *)addr);
					pmm_free(phys);
					
					set_cr3(t->cr3);
					unmap_page((void *)addr);
					set_cr3(def_cr3());
				}
				t->heap_pages = new_pages;
			}
			
			
			return (void *)t->heap_end;
		} else {
			// increment heap size
			
			size_t new_sz = t->heap_end - t->heap_begin;
			size_t new_pages = (new_sz + 0x1000 - 1) / 0x1000;
			alloc_mem_at(t->cr3, t->heap_begin + t->heap_pages * 0x1000, new_pages, 0x7);
			t->heap_pages = new_pages;
			
			return (void *)(t->heap_end - increment);
		}
	}
}

static int task_idling = 0;
void task_switch_to(task_t *t) {
	if (!t) {
		task_idling = 1;
		asm volatile ("jmp task_idle");
	} else {
		asm volatile ("jmp task_enter" : : "a"(t->cr3), "b"(&(t->st)) : "memory");
	}
}

void pic_eoi(uint8_t id);

static int task_first = 1;
int task_int_handler(interrupt_cpu_state *state) {
	if (!task_first && !task_idling) {
		task_save_cpu_state(state, sched_get_current());
	} else {
		task_first = 0;
	}

	task_idling = 0;

	pic_eoi(0x20); // ugly

	task_switch_to(sched_schedule_next());

	early_mesg(LEVEL_WARN, "task", "how did we get here");

	return 1;
}
