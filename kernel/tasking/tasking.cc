#include "tasking.h"
#include "../io/ports.h"
#include "../multiboot.h"
#include "../trace/stacktrace.h"
#define CLI() asm volatile("cli");
#define STI() asm volatile("sti");

#define loadcr3(cr3) asm volatile("mov %0, %%eax; mov %%eax, %%cr3" : : "r"(cr3) : "%eax");

#define getreg(x,y) asm volatile ("mov %%" x ", %0" : "=r"(y) : :);

extern int printf(const char* , ...);
extern int kprintf(const char* , ...);
extern void* memset(void*, int, size_t);
extern void* memcpy(void*, const void*, size_t);

extern multiboot_info_t *mbootinfo;

task_t* task_head = NULL;
task_t* current_task = NULL;


extern uint32_t loadexec(uint32_t i);

void create_proc_from_mod(uint32_t i) {

    uint32_t oldcr3;
    getreg("cr3", oldcr3);

    uint32_t x = loadexec(i);

    new_task(0x1000, 0x1b, 0x23, x, true);

}

void tasking_init() {
    create_proc_from_mod(0);
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

    if (t->prev != NULL)
        t->prev->next = t->next;
    if (t->next != NULL)
        t->next->prev = t->prev;
    kfree(t);
    ntasks--;

}

extern uint32_t current_pd;

uint32_t pid = 1;


uint32_t new_task(uint32_t addr, uint16_t cs, uint16_t ds, uint32_t pd, bool user) {

    task_t *t = (task_t*)kmalloc(sizeof(task_t));

    memset(t, 0, sizeof(task_t));

    uint32_t a;
    getreg("cr3", a);

    uint32_t oldcr3 = a;

    loadcr3(pd); 
    uint32_t po = current_pd;

    current_pd = pd;

    void* map = pmm_alloc();
    map_page(map, (void*)0xA0000000, user ? 0x7 : 0x3);

    set_cr3(oldcr3);

    t->cr3 = pd;
    
    t->pid = pid++;

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
    
    insert(t);
    ntasks++;
    return t->pid;

}

uint32_t tasking_fork() {
    // fork the process
    // nice
    
}

void tasking_schedule_next() {

    if (ntasks < 1) {
        printf("\e[1m");
        printf("\e[47m");
        printf("\e[31m");
        printf("Kernel panic!\n");
        printf("Scheduler has nothing to do(no processes running)! Halting!\n");
        CLI();
        while(1) asm volatile("hlt");
    }

    current_task = current_task->next;
    if (current_task == NULL)
        current_task = task_head;
}

extern "C" {

bool init = false;

uint32_t tasking_handler(uint32_t esp) {
    
    set_cr3(def_cr3());

    outb(0x20, 0xA0);

    if (init) {
        memcpy(&(current_task->st), (cpu_state_t*)esp, sizeof(cpu_state_t));
        tasking_schedule_next();
    } else {
        init = true;
    }

    asm volatile ("mov %0, %%eax; mov %1, %%ebx; jmp tasking_enter" : : "r"(current_task->cr3), "r"(&(current_task->st)) : "%eax", "%ebx");

    return esp;
}

}