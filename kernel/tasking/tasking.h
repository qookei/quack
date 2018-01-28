#ifndef TASKING
#define TASKING

#include "../kheap/heap.h"
#include "../interrupt/isr.h"
#include "../vsprintf.h"
#include <stddef.h>
#include <stdint.h>

#define PROCESS_ALIVE 0
#define PROCESS_ZOMBIE 1
#define PROCESS_DEAD 2

#define SIGTERM 15
#define SIGILL 4

typedef struct process {

    char *name;
    uint32_t pid;
    uint32_t esp;
    uint32_t stack;
    uint32_t eip;
    uint32_t cr3;
    uint32_t state;
    void (*notify)(uint32_t);
    void *waiting_for;
    bool not_executed;
    struct process *next, *prev;

} process_t;


void init_tasking();
void print_tasks();
uint32_t add_process(process_t *);
void __init__();
void preempt_now();
void __kill__();
void __notify__(uint32_t);
process_t *create_process(char *, uint32_t);
process_t *get_process(uint32_t);
process_t *get_current_process();
extern "C" void preempt();
void notify(uint32_t);
void kill(process_t *);


#endif