#ifndef TASKING
#define TASKING

#include "../kheap/heap.h"
#include "../interrupt/isr.h"
#include "../vsprintf.h"
#include <stddef.h>
#include <stdint.h>

#define MAX_TASKS 0xF

#define SIGABRT                 0
#define SIGFPE                  1
#define SIGILL                  2
#define SIGINT                  3
#define SIGSEGV                 4
#define SIGTERM                 5

#define SIG_ERR                 0xffffffff

typedef struct {

    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t eip;
    uint32_t cs;
    uint32_t ds;
    uint32_t es;
    uint32_t fs;
    uint32_t gs;
    uint32_t ss;
    uint32_t eflags;

} task_regs_t;

typedef struct {

    int status;
    int parent;

    int return_value;

    uint32_t page_directory;

    task_regs_t regs;

    // size_t heap_base;
    // size_t heap_size;

    // uint32_t sigabrt_hnd;
    // uint32_t sigfpe_hnd;
    // uint32_t sigill_hnd;
    // uint32_t sigint_hnd;
    // uint32_t sigsegv_hnd;
    // uint32_t sigterm_hnd;

} task_t;

extern task_t **tasks;
extern int current_task;
extern "C" {extern int tasking_enabled;}

void tasking_init();
int tasking_create(task_t*);
void tasking_shedule();
void tasking_kill(uint32_t);

#endif