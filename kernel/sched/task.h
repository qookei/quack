#ifndef TASK_H
#define TASK_H

#include <kheap/heap.h>
#include <paging/paging.h>
#include <stddef.h>
#include <stdint.h>
#include <interrupt/isr.h>

#define IPC_MAX_QUEUE 64

typedef int32_t pid_t;

#define SIGTERM 15
#define SIGILL 4
#define SIGSEGV 11

#define WAIT_NONE 0
#define WAIT_PROC 1
#define WAIT_IPC 2
#define WAIT_READY 3
#define WAIT_IRQ 4

#define INITIAL_STACK_SIZE 0x4000

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
	pid_t pid;

	cpu_state_t st;

	struct task *parent;

	uint32_t waiting_status;
	uint32_t waiting_info;

	uint32_t heap_begin;
	uint32_t heap_end;
	uint32_t heap_pages;

	ipc_message_t **ipc_message_queue;
	int is_privileged;

	int pending_irqs;
} task_t;

int task_int_handler(interrupt_cpu_state *);

void task_save_cpu_state(interrupt_cpu_state *state, task_t *t);

void task_init();
void task_setup(void *);

void task_switch();

void task_switch_to(task_t *);

task_t *task_create_new(int is_privileged);

void task_waitpid(interrupt_cpu_state *, pid_t child, task_t *);
void task_waitipc(interrupt_cpu_state *, task_t *);
int task_waitirq(interrupt_cpu_state *, task_t *);
int task_wakeup_irq(task_t *);

int task_ipcsend(task_t *recv, task_t *send, uint32_t size, void *data);
uint32_t task_ipcrecv(void **data, task_t *);
void task_ipcremov(task_t *);
uint32_t task_ipcqueuelen(task_t *);
uint32_t task_ipcgetsender(task_t *);

void task_kill(task_t *, int ret_val, int sig);

void *task_sbrk(int, task_t *);

#endif	// TASK_H
