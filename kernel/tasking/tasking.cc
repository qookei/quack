#include "tasking.h"
#include "../io/ports.h"
#include "../multiboot.h"
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
    create_proc_from_mod(1);
    create_proc_from_mod(0);
    create_proc_from_mod(1);
    create_proc_from_mod(1);
    create_proc_from_mod(1);
    create_proc_from_mod(1);
    create_proc_from_mod(1);
    // create_proc_from_mod(1);
    current_task = task_head;
}

task_t *new_node() {
    task_t *t = (task_t*)kmalloc(sizeof(task_t));
    memset(t, 0, sizeof(task_t));
    return t;
}

void insert(task_t *t) {
    task_t* temp = task_head;
    if(task_head == NULL) {
        task_head = t;
        return;
    }
    while(temp->next != NULL) temp = temp->next; // Go To last Node
    temp->next = t;
    t->prev = temp;
}

void kill_task(uint32_t pid) {
    task_t *t = task_head;
    while(t->next != NULL && t->pid != pid) {
        t = t->next;
    }

    // probably something to do wiith null here
    kprintf("kill1\n");
    if (t->prev != NULL)
        t->prev->next = t->next;
    kprintf("kill2\n");
    if (t->next != NULL)
        t->next->prev = t->prev;
    kprintf("kill3\n");
    kfree(t);
}

void kill_task_raw(task_t *t) {
    if (t->prev != NULL)
    t->prev->next = t->next;
    if (t->next != NULL)
    t->next->prev = t->prev;
    kfree(t);
}

extern uint32_t current_pd;

uint32_t pid;


uint32_t new_task(uint32_t addr, uint16_t cs, uint16_t ds, uint32_t pd, bool user) {

    task_t *t = (task_t*)kmalloc(sizeof(task_t));

    printf("----- %p\n", t);

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

}

void tasking_schedule_next() {
    current_task = current_task->next;
    if (current_task == NULL)
        current_task = task_head;
    // kprintf("current pid: %u\n", current_task->pid);
}

extern "C" {

// extern void tasking_enter(void);

uint32_t counter;

uint32_t tasking_handler(uint32_t esp) {
    
    set_cr3(def_cr3());

    outb(0x20, 0xA0);

    memcpy(&(current_task->st), (cpu_state_t*)esp, sizeof(cpu_state_t));

    tasking_schedule_next();

    kprintf("counter: %u\ncurrent pid: %u\ncurrent task ptr: %08p\n", counter++, current_task->pid, current_task);
    // printf("counter: %u\n", counter-1);

    asm volatile ("mov %0, %%eax; mov %1, %%ebx; jmp tasking_enter" : : "r"(current_task->cr3), "r"(&(current_task->st)) : "%eax", "%ebx");

    return esp;
}

}