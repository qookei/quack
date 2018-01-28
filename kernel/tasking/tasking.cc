#include "tasking.h"

process_t *current_proc;
process_t *kernel_proc;
uint32_t __cpid__;


extern "C" {
    bool tasking_enabled = false;
    uint8_t tasking_ticks;
}

extern void kernel_idle();
extern void* memset(void*, int, size_t);


void kernel_thread() {
    tasking_enabled = true;

    kernel_idle();

    while(1);
}

process_t *create_process(char *name, uint32_t loc) {
    process_t *p = (process_t*)kmalloc(sizeof(process_t));
    memset(p, 0, sizeof(process_t));
    p->name = name;
    p->pid = ++__cpid__;
    p->state = PROCESS_ALIVE;
    p->notify = __notify__;
    p->eip = loc;
    p->esp = (uint32_t) kmalloc(4096);
    p->not_executed = true;
    p->waiting_for = NULL;
    // set current cr3 to process cr3
    uint32_t cr3;
    asm volatile("mov %%cr3, %0" : "=r" (cr3));
    p->cr3 = cr3;
    uint32_t *stack = (uint32_t *)(p->esp + 4096);
    p->stack = p->esp;

    *--stack = 0x202;
    *--stack = 0x8;
    *--stack = loc;
    *--stack = 0x0;
    *--stack = 0x0;
    *--stack = 0x0;
    *--stack = 0x0;
    *--stack = 0x0;
    *--stack = 0x0;
    *--stack = p->esp + 4096;
    *--stack = 0x10;
    *--stack = 0x10;
    *--stack = 0x10;
    *--stack = 0x10;
    p->esp = (uint32_t) stack;

    return p;
}

process_t *get_process(uint32_t pid) {
    process_t *current = kernel_proc;
    do {
        if (current->pid == pid)
            return current;
        current = current->next;
    } while(current != kernel_proc);
    return NULL;
}

void print_tasks() {
    bool en = tasking_enabled;
    tasking_enabled = false;
    process_t *current = kernel_proc;
    printf("Running processes: (PID, name, state, * = current)\n");
    do {
		printf("%c[%u] '%s' %u\n", current == current_proc ? '*' : ' ', current->pid, current->name, current->state);
		current = current->next;
    } while (current != kernel_proc);
    tasking_enabled = en;
}

void __init__() {
    current_proc->not_executed = false;
    asm volatile("mov %%eax, %%esp": :"a"(current_proc->esp));
	asm volatile("pop %gs");
	asm volatile("pop %fs");
	asm volatile("pop %es");
	asm volatile("pop %ds");
	asm volatile("pop %ebp");
	asm volatile("pop %edi");
	asm volatile("pop %esi");
	asm volatile("pop %edx");
	asm volatile("pop %ecx");
	asm volatile("pop %ebx");
	asm volatile("pop %eax");
    asm volatile("iret");
}

void preempt_now() {
    if (!tasking_enabled) return;
    asm volatile("int $0x20");
}

void __kill__() {
    if (current_proc->pid != 1) {
        tasking_enabled = false;
        kfree((void *)current_proc->stack);
        kfree(current_proc);
        current_proc->prev->next = current_proc->next;
        current_proc->next->prev = current_proc->prev;
        current_proc->state = PROCESS_DEAD;
        tasking_enabled = true;
        preempt_now();
    } else {
        printf("Kernel died\n");
        asm volatile("cli\n1:\nhlt\njmp 1b");
    }
}

void __notify__(uint32_t sig) {
    switch (sig) {
        case SIGTERM:
            __kill__();
            break;
        
        case SIGILL:
            printf("Process %u caused an SIGILL\n", current_proc->pid);
            __kill__();
    }
}

void init_tasking() {
    kernel_proc = create_process("quack", (uint32_t)kernel_thread);
    kernel_proc->next = kernel_proc;
    kernel_proc->prev = kernel_proc;

    current_proc = kernel_proc;
    __init__();

}

process_t *get_current_process() {
    return current_proc;
}

uint32_t add_process(process_t *proc) {
    bool en = tasking_enabled;
    tasking_enabled = false;
    proc->next = current_proc->next;
    proc->next->prev = proc;
    proc->prev = current_proc;
    current_proc->next = proc;
    tasking_enabled = en;
    return proc->pid;
}

void notify(uint32_t sig) {
    current_proc->notify(sig);
}

void kill(process_t *proc){
	if(get_process(proc->pid) != NULL){
		tasking_enabled = false;
		kfree((void *)proc->stack);
		kfree(proc);
		proc->prev->next = proc->next;
		proc->next->prev = proc->prev;
		proc->state = PROCESS_DEAD;
		tasking_enabled = true;
	}
}

extern "C" {

void preempt() {
    asm volatile("push %eax");
	asm volatile("push %ebx");
	asm volatile("push %ecx");
	asm volatile("push %edx");
	asm volatile("push %esi");
	asm volatile("push %edi");
	asm volatile("push %ebp");
	asm volatile("push %ds");
	asm volatile("push %es");
	asm volatile("push %fs");
	asm volatile("push %gs");
	asm volatile("mov %%esp, %%eax":"=a"(current_proc->esp));
    current_proc = current_proc->next;
	if(current_proc->not_executed){
		__init__();
		return;
	}
	//pop all of next process' registers off of its stack
	asm volatile("mov %%eax, %%esp": :"a"(current_proc->esp));
	asm volatile("pop %gs");
	asm volatile("pop %fs");
	asm volatile("pop %es");
	asm volatile("pop %ds");
	asm volatile("pop %ebp");
	asm volatile("pop %edi");
	asm volatile("pop %esi");
	asm volatile("pop %edx");
	asm volatile("pop %ecx");
	asm volatile("pop %ebx");
    asm volatile("pop %eax");
}

}