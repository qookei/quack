#ifndef SCHED_H
#define SCHED_H

#include <stdint.h>
#include <stddef.h>

#include <sched/task.h>
#include <kheap/heap.h>

#include <string.h>

#define MAX_PROCESSES 65536

typedef struct sched_entry {
	task_t *task;

	struct sched_entry *next;
} sched_entry_t;

void sched_init();

task_t *sched_get_task(pid_t pid);
task_t *sched_get_current();

void sched_wake_up(task_t *t);
void sched_suspend(task_t *t);
int sched_exists(task_t *t);

pid_t sched_task_spawn(task_t *parent, int is_privileged);
void sched_task_make_ready(task_t *, uintptr_t entry, uintptr_t stack);
pid_t sched_find_free_pid();

void sched_kill(pid_t, int ret_val, int sig);

task_t *sched_schedule_next();

#endif
