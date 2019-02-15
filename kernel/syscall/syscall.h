#ifndef SYSCALL
#define SYSCALL

#include <interrupt/isr.h>
#include <sched/sched.h>
#include <paging/paging.h>

void syscall_init();

void exit_handler(uintptr_t *, uintptr_t *, uintptr_t *, void *);
void getpid_handler(uintptr_t *, uintptr_t *, uintptr_t *, void *);
void sched_spawn_new_handler(uintptr_t *, uintptr_t *, uintptr_t *, void *);
void sched_make_ready_handler(uintptr_t *, uintptr_t *, uintptr_t *, void *);
void get_phys_from_handler(uintptr_t *, uintptr_t *, uintptr_t *, void *);
void map_mem_to_handler(uintptr_t *, uintptr_t *, uintptr_t *, void *);
void unmap_mem_from_handler(uintptr_t *, uintptr_t *, uintptr_t *, void *);
void alloc_mem_phys_handler(uintptr_t *, uintptr_t *, uintptr_t *, void *);
void sbrk_handler(uintptr_t *, uintptr_t *, uintptr_t *, void *);
void ipc_send_handler(uintptr_t *, uintptr_t *, uintptr_t *, void *);
void ipc_recv_handler(uintptr_t *, uintptr_t *, uintptr_t *, void *);
void ipc_remove_handler(uintptr_t *, uintptr_t *, uintptr_t *, void *);
void ipc_queue_length_handler(uintptr_t *, uintptr_t *, uintptr_t *, void *);
void ipc_get_sender_handler(uintptr_t *, uintptr_t *, uintptr_t *, void *);
void debug_log_handler(uintptr_t *, uintptr_t *, uintptr_t *, void *);
void is_privileged_handler(uintptr_t *, uintptr_t *, uintptr_t *, void *);
void sched_prioritize_handler(uintptr_t *, uintptr_t *, uintptr_t *, void *);
void register_handler_handler(uintptr_t *, uintptr_t *, uintptr_t *, void *);
void unregister_handler_handler(uintptr_t *, uintptr_t *, uintptr_t *, void *);
void map_timer_handler(uintptr_t *, uintptr_t *, uintptr_t *, void *);
void wait_handler(uintptr_t *, uintptr_t *, uintptr_t *, void *);
void dummy_handler(uintptr_t *, uintptr_t *, uintptr_t *, void *);

int verify_addr(uint32_t pd, uint32_t addr, uint32_t len, uint32_t flags);
int copy_from_user(void *dst, void *src, size_t len);
int copy_to_user(void *dst, void *src, size_t len);

#endif
