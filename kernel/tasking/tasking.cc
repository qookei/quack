#include "tasking.h"
#include <io/ports.h>
#include <multiboot.h>
#include <trace/stacktrace.h>
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

uint32_t kernel_idle_create(uint32_t addr, uint16_t cs, uint16_t ds, uint32_t pd, uint32_t esp);


uint32_t load_init(const char *init) {

    uint32_t oldcr3;
    getreg("cr3", oldcr3);

    set_cr3(def_cr3());

    struct stat s;
    int i = stat(init, &s);

    uint32_t sz = s.st_size;
    printf("stat %i\n", i);
    printf("size %u\n", sz);


    int f = open(init, O_RDONLY);
    char *buffer = (char *)kmalloc(s.st_size);
    read(f, buffer, s.st_size);
    close(f);

    uint32_t phys = (uint32_t)get_phys(buffer);

    uint32_t pd = create_page_directory(mbootinfo);

    set_cr3(pd);

    kprintf("before cpy\n");

    for (uint32_t i = 0; i <= sz / 0x1000; i++) {

        void* mem = pmm_alloc();

        map_page(mem, (void*)((i*0x1000) + 0x1000), 0x7);

        map_page((void*)(phys & 0xFFFFF000 + (i * 0x1000)), (void*)0xE0000000, 0x3);

        memcpy((void*)((i*0x1000) + 0x1000), (const void*)(0xE0000000 + phys & 0xFFF), 0x1000);

        unmap_page((void *)0xE0000000);
    }

    kprintf("after cpy\n");

    // uint32_t x = loadexec(i);

    set_cr3(oldcr3);

    return pd;    

}

void tasking_init() {
    uint32_t esp;
    getreg("esp", esp);

    task_t *t = (task_t*)kmalloc(sizeof(task_t));
    memset(t, 0, sizeof(task_t));
    t->files = (file_handle_t *)kmalloc(sizeof(file_handle_t) * MAX_FILES);
    memset(t->files, 0, sizeof(file_handle_t) * MAX_FILES);
    current_task = t; // dirty hack here

    
}

void tasking_setup() {
    printf("%u free memory\n", free_pages() * 0x1000);

    uint32_t pd = load_init("/init");
    
    kfree(current_task->files);
    kfree(current_task);
    current_task = NULL;



    new_task(0x1000, 0x1b, 0x23, pd, true);

    // create_proc_from_mod(0);
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
    
    t->files = (file_handle_t *)kmalloc(sizeof(file_handle_t) * MAX_FILES);
    memset(t->files, 0, sizeof(file_handle_t) * MAX_FILES);

    t->files[0].present = 1;
    memcpy(t->files[0].path, "/dev/tty", 9);

    t->files[1].present = 1;
    memcpy(t->files[1].path, "/dev/tty", 9);

    t->files[2].present = 1;
    memcpy(t->files[2].path, "/dev/tty", 9);

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