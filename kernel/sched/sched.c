#include "sched.h"
#include <mesg.h>

task_t **tasks = NULL;

sched_entry_t *sched_queue = NULL;
sched_entry_t *current_process = NULL;

void sched_queue_insert(task_t *task) {
	if (!sched_queue) {
		sched_queue = kmalloc(sizeof(sched_entry_t));
		sched_queue->next = NULL;
		sched_queue->task = task;

		current_process = sched_queue;
		return;
	}

	sched_entry_t *temp = sched_queue;
	while (temp->next) {
		if (temp->task == task || temp->next->task == task)
			return; // already awake
		temp = temp->next;
	}

	sched_entry_t *ent = kmalloc(sizeof(sched_entry_t));
	ent->next = NULL;
	ent->task = task;

	temp->next = ent;
}

void sched_queue_remove(task_t *task) {
	sched_entry_t *temp = sched_queue;
	sched_entry_t *prev = NULL;
	while(temp && temp->task != task) {
		prev = temp;
		temp = temp->next;
	}

	if (!temp || temp->task != task) {
		early_mesg(LEVEL_WARN, "sched",
					"trying to suspend a process that's not running");

		return;
	}

	if (temp == sched_queue) {
		sched_queue = temp->next;
	} else {
		prev->next = temp->next;
	}

	kfree(temp);
}

void sched_init() {
	tasks = kmalloc(sizeof(task_t *) * MAX_PROCESSES);
	memset(tasks, 0, sizeof(task_t *) * MAX_PROCESSES);

	task_init();
}

void sched_wake_up(task_t *t) {
	sched_queue_insert(t);
}

void sched_suspend(task_t *t) {
	sched_queue_remove(t);
}

int sched_exists(task_t *t) {
	for (size_t i = 0; i < MAX_PROCESSES; i++)
		if (tasks[i] == t) return 1;

	return 0;
}

pid_t sched_task_spawn(task_t *parent, int is_privileged) {
	pid_t pid = sched_find_free_pid();

	task_t *t = task_create_new(is_privileged);

	t->parent = parent;
	t->pid = pid;
	t->waiting_status = WAIT_READY;

	tasks[pid] = t;

	return pid;
}

void sched_task_make_ready(task_t *t, uintptr_t entry, uintptr_t stack) {
	if (t->waiting_status != WAIT_READY)
		return;

	t->waiting_status = WAIT_NONE;
	t->st.eip = entry;
	t->st.esp = stack;

	sched_queue_insert(t);
}

void sched_kill(pid_t pid, int ret_val, int sig) {
	task_t *t = sched_get_task(pid);

	task_kill(t, ret_val, sig);

	sched_queue_remove(t);
	task_switch_to(sched_schedule_next());
}

task_t *sched_get_task(pid_t pid) {
	if (pid >= MAX_PROCESSES)
		return NULL;

	return tasks[pid];
}

pid_t sched_find_free_pid() {
	pid_t pid = 0;
	while (sched_get_task(pid))
		pid++;

	if (pid >= MAX_PROCESSES)
		pid = -1;

	return pid;
}

task_t *sched_get_current() {
	if (current_process)
		return current_process->task;

	return NULL;
}

task_t *sched_schedule_next() {
	if (!sched_queue)
		return NULL;		// nothing ready to run

	if (!current_process)
		current_process = sched_queue;

	current_process = current_process->next;

	if (!current_process)
		current_process = sched_queue;

	return current_process->task;
}
