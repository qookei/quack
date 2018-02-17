#include "tasking.h"
#define EMPTY_PID (task_t*)0xFFFFFFFF

task_t **tasks;

int current_task = 0;

extern void* memset(void*, int, size_t);
extern void* memcpy(void*, const void*, size_t);

extern int kprintf(const char*, ...);

const task_regs_t default_regs = {0,0,0,0,0,0,0,0,0,0x1b,0x23,0x23,0x23,0x23,0x23,0x202};

void f() {
    asm volatile("sti");
    while (1) {
        kprintf("this pid %u\n", current_task);
    }
}

void tasking_init() {
    tasks = (task_t **)kmalloc(MAX_TASKS * sizeof(task_t *));
    memset(tasks, 0, MAX_TASKS * sizeof(task_t *));

    tasks[0] = (task_t *)kmalloc(sizeof(task_t));
    tasks[0]->page_directory = def_cr3();
    // tasks[0]->regs.eip = (uint32_t)(&f);
    current_task = 0;
}


int tasking_create(task_t *task) {
    int pid;
    for (pid = 0; pid < MAX_TASKS; pid++)
        if (tasks[pid] == NULL) break;

    tasks[pid] = task;
    return pid;
}


extern "C" {

int tasking_enabled;

extern void tasking_enter(task_regs_t*, uint32_t);

void tasking_switch(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx, uint32_t esi, 
                    uint32_t edi, uint32_t ebp, uint32_t ds, uint32_t es, uint32_t fs, 
                    uint32_t gs, uint32_t eip, uint32_t cs, uint32_t eflags, uint32_t esp, uint32_t ss) {

    set_cr3(def_cr3());

    tasks[current_task]->regs.eax = eax;
    tasks[current_task]->regs.ebx = ebx;
    tasks[current_task]->regs.ecx = ecx;
    tasks[current_task]->regs.edx = edx;
    tasks[current_task]->regs.esi = esi;
    tasks[current_task]->regs.edi = edi;
    tasks[current_task]->regs.ebp = ebp;
    tasks[current_task]->regs.ds = ds;
    tasks[current_task]->regs.es = es;
    tasks[current_task]->regs.fs = fs;
    tasks[current_task]->regs.gs = gs;
    tasks[current_task]->regs.eip = eip;
    tasks[current_task]->regs.cs = cs;
    tasks[current_task]->regs.eflags = eflags;
    tasks[current_task]->regs.esp = esp;
    tasks[current_task]->regs.ss = ss;
    
    current_task++;
    tasking_shedule();
    tasking_enter(&tasks[current_task]->regs, tasks[current_task]->page_directory);
}

}


void tasking_shedule() {
        
    if (tasks[current_task] == NULL) {
        current_task = 0;
    }    

}

void tasking_kill(uint32_t pid) {
    tasks[pid] = NULL;
}