#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include <stddef.h>

/* process */
void sys_exit(int err_code);
int32_t sys_getpid();

/* wait */
int sys_waitpid(int32_t pid);
int sys_waitipc();

/* sched */
int32_t sys_spawn_new(int32_t parent, int is_privileged);
void sys_make_ready(int32_t pid, uintptr_t entry, uintptr_t stack);

/* memory */
uintptr_t sys_get_phys_from(int32_t pid, uintptr_t virt);
void sys_map_to(int32_t pid, uintptr_t virt, uintptr_t phys);
void sys_unmap_from(int32_t pid, uintptr_t virt);
uintptr_t sys_alloc_phys();
void *sys_sbrk(int increment);

/* IPC */
int sys_ipc_send(int32_t pid, size_t size, void *data);
int sys_ipc_recv(void *data);
void sys_ipc_remove();
int sys_ipc_queue_length();
int32_t sys_ipc_get_sender();

/* misc */
void sys_debug_log(char *message);

#endif
