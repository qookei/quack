#ifndef TASKING_H
#define TASKING_H

#include "../kheap/heap.h"
#include <stddef.h>
#include <stdint.h>

#define PROCESS_ALIVE 0
#define PROCESS_ZOMBIE 1
#define PROCESS_DEAD 2

#define SIGTERM 15
#define SIGILL 4

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

typedef struct {

    char *name;
    uint32_t cr3;

    cpu_state_t st;

} task_t;


void tasking_init();
extern "C" {void tasking_switch();}

uint32_t new_task(uint32_t, uint16_t, uint16_t, uint32_t, bool);


#endif  // TASKING_H