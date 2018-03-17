#include "tasking.h"
#include "../io/ports.h"
#include "../multiboot.h"
#define loadcr3(cr3) asm volatile("mov %0, %%eax; mov %%eax, %%cr3" : : "r"(cr3) : "%eax");

#define getreg(x,y) asm volatile ("mov %%" x ", %0" : "=r"(y) : :);

extern int printf(const char* , ...);
extern int kprintf(const char* , ...);
extern void* memset(void*, int, size_t);
extern void* memcpy(void*, const void*, size_t);

extern multiboot_info_t *mbootinfo;

task_t tasks[256];

uint32_t ctask = -1;
uint32_t ntask = 0;

uint8_t arr[] = {
    0xb8, 0x02, 0x00, 0x00, 0x00, 0xbe, 0x41, 0x00, 0x00, 0x00, 0xcd, 0x30, 0xeb, 0xfc
};

uint8_t arr2[] = {
    0xb8, 0x02, 0x00, 0x00, 0x00, 0xbe, 0x42, 0x00, 0x00, 0x00, 0xcd, 0x30, 0xeb, 0xfc
};

extern uint32_t loadexec(uint32_t i);

void tasking_init() {
	uint32_t a;
    // getreg("esp");
    // tasks[0].esp = a;
    // tasks[0].cr3 = def_cr3();
    // ntask++;

    // void* map = pmm_alloc();
    // map_page(map, (void*)0xA0000000, 0x3);

    // uint32_t* stack = (uint32_t*)(0xA0000000 + 0x1000);
    // *--stack = 0x10;
    // *--stack = (0xA0001000);
    // *--stack = 0x202;
    // *--stack = 0x08;
    // *--stack = (uint32_t)f;
    // *--stack = 0x10;
    // *--stack = 0;
    // *--stack = 0;
    // *--stack = 0;
    // *--stack = 0;
    // *--stack = 0;
    // *--stack = 0;
    // *--stack = 0;


    // tasks[1].esp = (uint32_t)stack;
    // tasks[1].cr3 = def_cr3();
    // ntask++;

    void* page = pmm_alloc();

    getreg("cr3", a);

    uint32_t oldcr3 = a;

    uint32_t x = create_page_directory(mbootinfo);
    set_cr3(x);

    map_page(page, (void*)0xA00000, 0x7);

    for(int i = 0; i < 15; i++) {
        ((uint8_t*)0xA00000)[i] = arr[i];
    }

    kprintf("after\n");

    set_cr3(oldcr3);

    new_task(0xA00000, 0x1b, 0x23, x, true);


    getreg("cr3", a);

    oldcr3 = a;

    void* page2 = pmm_alloc();

    uint32_t y = create_page_directory(mbootinfo);
    set_cr3(y);

    map_page(page2, (void*)0xA00000, 0x7);

    for(int i = 0; i < 15; i++) {
        ((uint8_t*)0xA00000)[i] = arr2[i];
    }

    kprintf("after\n");

    set_cr3(oldcr3);

    new_task(0xA00000, 0x1b, 0x23, y, true);




}

extern uint32_t current_pd;



uint32_t new_task(uint32_t addr, uint16_t cs, uint16_t ds, uint32_t pd, bool user) { 

    uint32_t a;
    getreg("cr3", a);

    uint32_t oldcr3 = a;

    loadcr3(pd); 
    uint32_t po = current_pd;

    current_pd = pd;

    void* map = pmm_alloc();
    map_page(map, (void*)0xA0000000, user ? 0x7 : 0x3);

    tasks[ntask].cr3 = pd;
    
    tasks[ntask].st.seg = ds;
    tasks[ntask].st.ebx = 0;
    tasks[ntask].st.ebx = 0;
    tasks[ntask].st.ecx = 0;
    tasks[ntask].st.edx = 0;
    tasks[ntask].st.esi = 0;
    tasks[ntask].st.edi = 0;
    tasks[ntask].st.ebp = 0;

    tasks[ntask].st.esp = 0xA0000000;
    tasks[ntask].st.eip = addr;
    tasks[ntask].st.cs = cs;
    tasks[ntask].st.eflags = 0x202;
    tasks[ntask].st.ss = ds;

    ntask++;

    set_cr3(oldcr3);

    return ntask - 1;
}

extern "C" {

// extern void tasking_enter(void);

uint32_t tasking_handler(uint32_t esp) {
    
    set_cr3(def_cr3());

    outb(0x20, 0xA0);

    if (ctask == -1) {
        ctask = 0;
        asm volatile ("mov %0, %%eax; mov %1, %%ebx; jmp tasking_enter" : : "r"(tasks[ctask].cr3), "r"(&tasks[ctask].st) : "%eax", "%ebx");
    }

    if (ntask < 1) return esp;

    memcpy(&tasks[ctask].st, (cpu_state_t*)esp, sizeof(cpu_state_t));

    // printf("esp: %08x\n", tasks[ctask].st.esp);
    // printf("esp: %08x\n", tasks[ctask].st.esp);
    // printf("esp: %08x\n", tasks[ctask].st.esp);
    // printf("esp: %08x\n", tasks[ctask].st.esp);

    ctask++;
    if (ctask >= ntask) ctask = 0;

    //printf("ctask: %u\n", ctask);

    asm volatile ("mov %0, %%eax; mov %1, %%ebx; jmp tasking_enter" : : "r"(tasks[ctask].cr3), "r"(&tasks[ctask].st) : "%eax", "%ebx");

    // never get here
    return esp;
}

}