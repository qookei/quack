#include "tasking.h"
#include "elf.h"
#include <io/ports.h>
#include <multiboot.h>
#include <trace/stacktrace.h>
#include <panic.h>
#include <mesg.h>
#define CLI() asm volatile("cli");
#define STI() asm volatile("sti");

#define loadcr3(cr3) asm volatile("mov %0, %%eax; mov %%eax, %%cr3" : : "r"(cr3) : "%eax");

#define getreg(x,y) asm volatile ("mov %%" x ", %0" : "=r"(y) : :);

void tasking_destroy_address_space(task_t *t) {
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

unsigned char test_elf[] = {
  0x7f, 0x45, 0x4c, 0x46, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x60, 0x80, 0x04, 0x08, 0x34, 0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x34, 0x00, 0x20, 0x00, 0x01, 0x00, 0x28, 0x00,
  0x03, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x80, 0x04, 0x08, 0x00, 0x80, 0x04, 0x08, 0x62, 0x00, 0x00, 0x00,
  0x62, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xeb, 0xfe, 0x00, 0x2e, 0x73, 0x68, 0x73, 0x74, 0x72, 0x74, 0x61, 0x62,
  0x00, 0x2e, 0x74, 0x65, 0x78, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x0b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
  0x60, 0x80, 0x04, 0x08, 0x60, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x62, 0x00, 0x00, 0x00,
  0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

unsigned int test_elf_len = 236;

task_t* task_head = NULL;
task_t* current_task = NULL;

uint32_t __pid = 1;
void tasking_setup(const char *init_path) {
	elf_loaded r = prepare_elf_for_exec(test_elf, test_elf_len);
	
	if (!r.success_ld) {
		panic("Failed to load init", NULL, 0, 0);
	}

	new_task(r.entry_addr, 0x1b, 0x23, r.page_direc, 1, __pid++);

	current_task = task_head;
}

int64_t ntasks = 0;

void insert(task_t *t) {
	task_t* temp = task_head;
	if(task_head == NULL) {
		task_head = t;
		return;
	}
	while(temp->next != NULL) temp = temp->next;
	temp->next = t;
	t->prev = temp;
}

void kill_task(uint32_t pid) {
	task_t *t = task_head;
	while(t->next != NULL && t->pid != pid) {
		t = t->next;
	}

	if (t->pid != pid)
		return;

	kill_task_raw(t);

}


void kill_task_raw(task_t *t) {

	uint32_t dead_pid = t->pid;
	uint32_t dead_ret = t->st.eax;

	tasking_destroy_address_space(t);

	if (t->prev != NULL)
		t->prev->next = t->next;
	if (t->next != NULL)
		t->next->prev = t->prev;
	kfree(t);
	ntasks--;

	// find all tasks that waited for this

	task_t* temp = task_head;
	while(temp != NULL) {
		if (temp->waiting_status == WAIT_PROC && temp->waiting_info == dead_pid) {
			temp->waiting_status = WAIT_NONE;
			temp->waiting_info = 0;
			temp->st.eax = dead_ret;
		}
		temp = temp->next;
	}
}


extern uint32_t current_pd;

extern const char *tty_path;

task_t *new_task(uint32_t addr, uint16_t cs, uint16_t ds, uint32_t pd, int user, uint32_t pid) {

	task_t *t = (task_t*)kmalloc(sizeof(task_t));

	memset(t, 0, sizeof(task_t));

	uint32_t a;
	getreg("cr3", a);

	uint32_t oldcr3 = a;

	loadcr3(pd); 

	current_pd = pd;

	void* map = pmm_alloc();
	map_page(map, (void*)0xA0000000, user ? 0x7 : 0x3);

	set_cr3(oldcr3);

	t->cr3 = pd;
	
	t->pid = pid;

	t->st.seg = ds;
	t->st.ebx = 0;
	t->st.ebx = 0;
	t->st.ecx = 0;
	t->st.edx = 0;
	t->st.esi = 0;
	t->st.edi = 0;
	t->st.ebp = 0;

	t->st.esp = 0xA0001000;
	t->st.eip = addr;
	t->st.cs = cs;
	t->st.eflags = 0x202;
	t->st.ss = ds;
	
	t->ipc_message_queue = (ipc_message_t **)kmalloc(IPC_MAX_QUEUE * sizeof(ipc_message_t *));
	memset(t->ipc_message_queue, 0, IPC_MAX_QUEUE * sizeof(ipc_message_t *));
	
	insert(t);
	ntasks++;
	return t;

}

uint32_t tasking_fork(interrupt_cpu_state *state) {
	set_cr3(def_cr3());
	task_t *t = (task_t*)kmalloc(sizeof(task_t));
	
	memset(t, 0, sizeof(task_t));

	t->st.eax = 0;
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
	t->st.ebx = state->ebx;
	t->pid = __pid++;

	t->ipc_message_queue = (ipc_message_t **)kmalloc(IPC_MAX_QUEUE * sizeof(ipc_message_t *));
	memset(t->ipc_message_queue, 0, IPC_MAX_QUEUE * sizeof(ipc_message_t *));

	uint32_t new_proc_pd = create_page_directory();
	t->cr3 = new_proc_pd;

	uint32_t pd_addr = current_task->cr3;

	uint32_t kernel_addr = (0xC0000000 >> 22);

	set_cr3(new_proc_pd);

	map_page((void *)(pd_addr & 0xFFFFF000), (void *)0xE0000000, 0x3);
	uint32_t *pd = (uint32_t*)0xE0000000;

	uint32_t at = 0x00000000;

	for (uint32_t i = 0; i < kernel_addr; i++) {
		map_page((void *)(pd[i] & 0xFFFFF000), (void *)0xE0001000, 0x3);
		uint32_t *pt = (uint32_t *)(0xE0001000);
		
		if (!(pd[i] & 0xFFF)) {
			unmap_page((void *)0xE0001000);
			at += 0x400000;
			continue;
		}

		for (uint32_t j = 0; j < 1024; j++) {
		
			uint32_t entry = pt[j];

			if (entry & 0xFFF) {
				map_page((void *)(entry & 0xFFFFF000), (void *)0xE0002000, 0x3);
				void *new_mem = pmm_alloc();
				map_page((void *)new_mem, (void *)at, entry & 0xFFF);
				memcpy((void*)at, (const void*)0xE0002000, 0x1000);
				unmap_page((void *)0xE0002000);
			}

			at += 0x1000;

		}

		unmap_page((void *)0xE0001000);

	}

	unmap_page((void *)0xE0000000);

	set_cr3(def_cr3());

	insert(t);

	ntasks++;


	return t->pid;
}

int tasking_ipcsend(uint32_t pid, uint32_t size, void *data, uint32_t sender) {

	task_t *t = task_head;
	while(t->next != NULL && t->pid != pid) {
		t = t->next;
	}

	if (t->pid != pid)
		return 0;

	uint32_t i = 0;
	
	for (; i < IPC_MAX_QUEUE; i++) {
		if (!t->ipc_message_queue[i])
			break;
	}
	
	if (i == IPC_MAX_QUEUE)
		return 0;

	t->ipc_message_queue[i] = (ipc_message_t *)kmalloc(sizeof(ipc_message_t));
	t->ipc_message_queue[i]->size = size;
	t->ipc_message_queue[i]->data = data;
	t->ipc_message_queue[i]->sender = sender;

	if (t->waiting_status == WAIT_IPC) {
		t->waiting_status = WAIT_NONE;
	}

	return 1;
}

uint32_t tasking_ipcrecv(void **data) {
	if (!current_task->ipc_message_queue[0])
		return -1;

	if (data)	
		*data = current_task->ipc_message_queue[0]->data;
	return current_task->ipc_message_queue[0]->size;
}

void tasking_ipcremov() {
	
	if (!current_task->ipc_message_queue[0])
		return;
	
	kfree(current_task->ipc_message_queue[0]);
	
	uint32_t i = 1;
	
	for (; i < IPC_MAX_QUEUE; i++) {
		if (!current_task->ipc_message_queue[i])
			break;
	}
	
	for (uint32_t j = 1; j < i; j++) {
		current_task->ipc_message_queue[j - 1] = current_task->ipc_message_queue[j];
	}
	
	current_task->ipc_message_queue[i - 1] = NULL;
	
}

uint32_t tasking_ipcgetsender() {
	if (tasking_ipcqueuelen()) {
		return current_task->ipc_message_queue[0]->sender;
	}
	
	return 0xFFFFFFFF;
}

uint32_t tasking_ipcqueuelen() {
	
	for (uint32_t i = 0; i < IPC_MAX_QUEUE; i++) {
		if (!current_task->ipc_message_queue[i])
			return i;
	}
	
	return IPC_MAX_QUEUE;
}

int tasking_execve(const char *name, char **argv, char **envp) {

	// not so TODO, move to a user-space server
	return -1;
/*
	(void)argv;
	(void)envp;

	elf_loaded r = prepare_elf_for_exec(name);

	if (!r.success_ld) {
		return -1;
	}

	task_t* t = current_task;

	tasking_schedule_next();

	if (t->ipc_message_queue) {
		for (uint32_t i = 0; i < IPC_MAX_QUEUE; i++) {
			if (t->ipc_message_queue[i]) {
				void *data2free = t->ipc_message_queue[i]->data;
				kfree(t->ipc_message_queue[i]);
				if (data2free)
					kfree(data2free);
			}
		}
		kfree(t->ipc_message_queue);
	}

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

	t->cr3 = r.page_direc;

	set_cr3(t->cr3);

	void* map = pmm_alloc();
	map_page(map, (void*)0xA0000000, 0x7);

	set_cr3(def_cr3());

	t->st.ebx = 0;
	t->st.ebx = 0;
	t->st.ecx = 0;
	t->st.edx = 0;
	t->st.esi = 0;
	t->st.edi = 0;
	t->st.ebp = 0;

	t->st.esp = 0xA0001000;
	t->st.eip = r.entry_addr;
	t->st.eflags = 0x202;
	
	t->ipc_message_queue = (ipc_message_t **)kmalloc(IPC_MAX_QUEUE * sizeof(ipc_message_t *));
	memset(t->ipc_message_queue, 0, IPC_MAX_QUEUE * sizeof(ipc_message_t *));
		
	return 0;*/
}

void tasking_waitpid(interrupt_cpu_state *state, uint32_t pid) {
	task_t *t = current_task;

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
	t->st.ebx = state->ebx;

	t->waiting_status = WAIT_PROC;
	t->waiting_info = pid;

	task_t *t_tmp = task_head;
	while(t_tmp->next != NULL && t_tmp->pid != pid) {
		t_tmp = t_tmp->next;
	}

	if (t_tmp->pid != pid) {
		t->waiting_status = WAIT_NONE;
		t->waiting_info = 0;
		t->st.eax = -1;
		return;
	}
}

void tasking_waitipc(interrupt_cpu_state *state) {
	task_t *t = current_task;

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
	t->st.ebx = state->ebx;

	t->waiting_status = WAIT_IPC;
}

void tasking_schedule_next() {

	if (ntasks < 1) {
		panic("scheduler: no processes running!", NULL, 0, 0);
	}

	while (1) {
		current_task = current_task->next;
		if (current_task == NULL)
			current_task = task_head;
			
		if (current_task == NULL)
			panic("scheduler: no processes running!", NULL, 0, 0);
	
		if (current_task->waiting_status == WAIT_NONE)
			break;
	}
}

void tasking_init_heap(size_t size) {
	size_t pages = (size + 0x1000 - 1) / 0x1000;
	
	alloc_mem_at(current_task->cr3, 0x30000000, pages, 0x7);

	current_task->heap_pages = pages;
	current_task->heap_begin = 0x30000000;
	current_task->heap_end = 0x30000000 + size;
	
}

void *tasking_sbrk(int increment) {
	
	if (!increment) return (void *)current_task->heap_end;

	if (!current_task->heap_begin) {
		if (increment < 0) {
			return (void *)-1;
		} else {
			// init heap
			tasking_init_heap(increment);
			return (void *)current_task->heap_begin;
		}
	} else {
		current_task->heap_end += increment;
		if (increment < 0) {
			// decrement heap size
			if (current_task->heap_end < current_task->heap_begin) return (void *)-1;
			size_t new_sz = current_task->heap_end - current_task->heap_begin;
			size_t new_pages = (new_sz + 0x1000 - 1) / 0x1000;
			
			if (new_pages < current_task->heap_pages) {
				
				size_t pages_delete = current_task->heap_pages - new_pages;
				for (size_t i = 0; i < pages_delete; i++) {
					uint32_t addr = current_task->heap_begin + (new_pages + i) * 0x1000;
					
					void *phys = get_phys(current_task->cr3, (void *)addr);
					pmm_free(phys);
					
					set_cr3(current_task->cr3);
					unmap_page((void *)addr);
					set_cr3(def_cr3());
				}
				current_task->heap_pages = new_pages;
			}
			
			
			return (void *)current_task->heap_end;
		} else {
			// increment heap size
			
			size_t new_sz = current_task->heap_end - current_task->heap_begin;
			size_t new_pages = (new_sz + 0x1000 - 1) / 0x1000;
			alloc_mem_at(current_task->cr3, current_task->heap_begin + current_task->heap_pages * 0x1000, new_pages, 0x7);
			current_task->heap_pages = new_pages;
			
			return (void *)(current_task->heap_end - increment);
		}
	}
	
}


void tasking_schedule_after_kill() {
	asm volatile ("jmp tasking_enter" : : "a"(current_task->cr3), "b"(&(current_task->st)));
}

// TODO: make tasking handler use the generic interrupt code and not it's own "special snowflake" code

int init = 0;

uint32_t tasking_handler(uint32_t esp) {
	
	set_cr3(def_cr3());

	outb(0x20, 0xA0);

	if (init) {
		memcpy(&(current_task->st), (cpu_state_t*)esp, sizeof(cpu_state_t));
		tasking_schedule_next();
	} else {
		init = 1;
	}

	tasking_schedule_after_kill();
	
	return esp;
}
