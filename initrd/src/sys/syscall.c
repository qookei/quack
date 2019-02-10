#include "syscall.h"

/* process */
void sys_exit(int err_code) {
	asm volatile ("int $0x30" : : "b"(err_code), "a"(0));
}

int32_t sys_getpid() {
	int32_t pid;
	asm volatile ("int $0x30" : "=b"(pid) : "a"(1));
	return pid;
}

int sys_is_privileged(int32_t pid) {
	int is_privileged;
	asm volatile ("int $0x30" : "=c"(is_privileged) : "a"(17), "b"(pid));
	return is_privileged;
}

/* wait */
int sys_waitpid(int32_t pid) {
	int stat;
	asm volatile ("int $0x30" : "=b"(stat) : "b"(pid), "a"(2));
	return stat;
}

void sys_waitipc() {
	asm volatile ("int $0x30" : : "a"(3));
}

void sys_waitirq() {
	asm volatile ("int $0x30" : : "a"(21));
}

/* sched */
int32_t sys_spawn_new(int32_t parent, int is_privileged) {
	int32_t child;
	asm volatile ("int $0x30" : "=b"(child) : "a"(4), "b"(parent), "c"(is_privileged));
	return child;
}

void sys_make_ready(int32_t pid, uintptr_t entry, uintptr_t stack) {
	asm volatile ("int $0x30" : : "a"(5), "b"(pid), "c"(entry), "d"(stack));
}

void sys_prioritize(int32_t pid) {
	asm volatile ("int $0x30" : : "a"(18), "b"(pid));
}

/* memory */
uintptr_t sys_get_phys_from(int32_t pid, uintptr_t virt) {
	uintptr_t phys;
	asm volatile ("int $0x30" : "=d"(phys) : "a"(6), "b"(pid), "c"(virt));
	return phys;
}

void sys_map_to(int32_t pid, uintptr_t virt, uintptr_t phys) {
	asm volatile ("int $0x30" : : "a"(7), "b"(pid), "c"(virt), "d"(phys));
}

void sys_unmap_from(int32_t pid, uintptr_t virt) {
	asm volatile ("int $0x30" : : "a"(8), "b"(pid), "c"(virt));
}

uintptr_t sys_alloc_phys() {
	uintptr_t phys;
	asm volatile ("int $0x30" : "=b"(phys) : "a"(9));
	return phys;
}

void *sys_sbrk(int increment) {
	uintptr_t brk;
	asm volatile ("int $0x30" : "=c"(brk) : "a"(10), "b"(increment));
	return (void *)brk;
}

/* IPC */
int sys_ipc_send(int32_t pid, size_t size, void *data) {
	int stat;
	asm volatile ("int $0x30" : "=c"(stat) : "a"(11), "b"(pid), "c"(size), "d"(data));
	return stat;
}

int sys_ipc_recv(void *data) {
	int stat;
	asm volatile ("int $0x30" : "=c"(stat) : "a"(12), "d"(data));
	return stat;
}

void sys_ipc_remove() {
	asm volatile ("int $0x30" : : "a"(13));
}

int sys_ipc_queue_length() {
	int len;
	asm volatile ("int $0x30" : "=b"(len) : "a"(14));
	return len;
}

int32_t sys_ipc_get_sender() {
	int32_t sender;
	asm volatile ("int $0x30" : "=b"(sender) : "a"(15));
	return sender;
}

/* misc */
void sys_debug_log(char *message) {
	char *c = message;
	size_t i = 0;
	while (c[i]) i++;

	asm volatile ("int $0x30" : : "a"(16), "b"(message), "c"(i));
}

/* irq */
void sys_register_handler(int int_no) {
	asm volatile ("int $0x30" : : "a"(19), "b"(int_no));
}

void sys_unregister_handler(int int_no) {
	asm volatile ("int $0x30" : : "a"(20), "b"(int_no));
}
