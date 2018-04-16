#ifndef TASKING_H
#define TASKING_H

#include <kheap/liballoc.h>
#include <fs/vfs.h>
#include <paging/paging.h>
#include <stddef.h>
#include <stdint.h>
#include <interrupt/isr.h>

#define PROCESS_ALIVE 0
#define PROCESS_ZOMBIE 1
#define PROCESS_DEAD 2

#define SIGTERM 15
#define SIGILL 4

#define WAIT_NONE 0
#define WAIT_PROC 1

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

typedef struct task{
    
    uint32_t cr3;
    uint32_t pid;

    cpu_state_t st;

    task* next;
    task* prev;

    uint32_t waiting_status;
    uint32_t waiting_info;

    // file handles
    file_handle_t *files;

    char pwd[1024];

} task_t;


void tasking_init();
void tasking_setup(const char *);
extern "C" {void tasking_switch();}
void tasking_schedule_next();
void tasking_schedule_after_kill();

task_t *new_task(uint32_t, uint16_t, uint16_t, uint32_t, bool, uint32_t);
uint32_t tasking_fork(interrupt_cpu_state *);
void tasking_waitpid(interrupt_cpu_state *, uint32_t);
int tasking_execve(const char *name, char **argv, char **envp);
void kill_task(uint32_t);
void kill_task_raw(task_t*);


#endif  // TASKING_H