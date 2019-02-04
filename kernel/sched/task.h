#ifndef TASKING_H
#define TASKING_H

#include <kheap/heap.h>
#include <paging/paging.h>
#include <stddef.h>
#include <stdint.h>
#include <interrupt/isr.h>

#define IPC_MAX_QUEUE 64

#define SIGTERM 15
#define SIGILL 4

#define WAIT_NONE 0
#define WAIT_PROC 1
#define WAIT_IPC 1

typedef struct {

	uint32_t seg;

	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
	uint32_t esi;
	uint32_t edi;
	uint32_t ebp;

	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;
	uint32_t esp;
	uint32_t ss;

} cpu_state_t;

typedef struct ipc_message {
	
	uint32_t size;
	void *data;
	uint32_t sender;
	
} ipc_message_t;

typedef struct task{
	
	uint32_t cr3;
	uint32_t pid;

	cpu_state_t st;

	struct task* next;
	struct task* prev;

	uint32_t waiting_status;
	uint32_t waiting_info;

	uint32_t heap_begin;
	uint32_t heap_end;
	uint32_t heap_pages;

	ipc_message_t **ipc_message_queue;

} task_t;

int task_int_handler(interrupt_cpu_state *);

void task_setup(void *);

void task_switch();

void task_schedule_next();
void task_switch_to(task_t *);

task_t *new_task(uint32_t, uint16_t, uint16_t, uint32_t, int, uint32_t);

uint32_t task_fork(interrupt_cpu_state *, task_t *);

void task_waitpid(interrupt_cpu_state *, uint32_t, task_t *);
void task_waitipc(interrupt_cpu_state *, task_t *);

int task_ipcsend(uint32_t pid, uint32_t size, void *data, uint32_t sender);
uint32_t task_ipcrecv(void **data, task_t *);
void task_ipcremov(task_t *);
uint32_t task_ipcqueuelen(task_t *);
uint32_t task_ipcgetsender(task_t *);

void kill_task(uint32_t);
void kill_task_raw(task_t *);

void *task_sbrk(int, task_t *);

#endif	// TASKING_H
