/*
 * quack early exec server
 * */

#include <stdint.h>
#include <stddef.h>

#include "sys/syscall.h"
#include "sys/elf.h"

#define OPERATION_SPAWN_NEW 0x1
#define OPERATION_EXEC 0x2
#define OPERATION_FORK 0x3
#define OPERATION_EXIT 0x4

struct spawn_message {
	int operation;
	int is_privileged;
	size_t binary_size;
	char binary[];
};

struct spawn_response {
	int status;
	int32_t pid;
};

int alloc_mem_at(int32_t pid, uintptr_t virt, size_t pages) {
	while(pages) {
		uintptr_t phys = sys_alloc_phys();
		
		if (!phys)
			return 0;
		
		sys_map_to(pid, virt, phys);
		
		virt += 0x1000;
		pages--;
	}

	return 1;
}

void *memcpy(void *dst, const void *src, size_t len) {
	for (size_t i = 0; i < len; i++)
		((unsigned char *)dst)[i] = ((const unsigned char *)src)[i];
	
	return dst;
}

void proc_memcpy(int32_t pid, uintptr_t src, uintptr_t dst, size_t size) {
	uintptr_t dst_phys;	
	uintptr_t dst_off;
	uintptr_t src_off = 0;

	dst_phys = sys_get_phys_from(pid, dst);

	dst_off = dst & 0xFFF;
	dst_phys = dst_phys & 0xFFFFF000;

	while (size > 0) {
		size_t copy_size = size > 0x1000 ? 0x1000 : size;

		sys_map_to(sys_getpid(), 0x80002000, dst_phys);

		dst += 0x1000;		
		dst_phys = sys_get_phys_from(pid, dst) & 0xFFFFF000;

		if (dst_off + copy_size >= + 0x1000 && dst_phys)
			sys_map_to(sys_getpid(), 0x80003000, dst_phys);

		memcpy((void *)(0x80002000 + dst_off), (void *)(src + src_off), copy_size);

		sys_unmap_from(sys_getpid(), 0x80002000);
		sys_unmap_from(sys_getpid(), 0x80003000);

		size -= copy_size;
		src_off += 0x1000;
	}
}

int32_t exec_spawn_new(int32_t parent, struct spawn_message *msg) {
	void *elf_file = msg->binary;

	if (!elf_check_header((elf_hdr *)elf_file)){
		sys_debug_log("exec: failed to verify header\n");
		return -1;
	}

	if (!sys_is_privileged(parent) && msg->is_privileged) {
		sys_debug_log("exec: nonprivileged process tried to spawn a privileged process");
		return -2;
	}

	int32_t pid = sys_spawn_new(parent, msg->is_privileged);

	elf_hdr *hdr = (elf_hdr *)elf_file;

	for (int i = 0; i < hdr->ph_ent_cnt; i++) {
		elf_program_header *phdr = elf_get_program_header(elf_file, i);

		if (phdr->type != SEGTYPE_LOAD) {
			continue;
		}

		uint32_t sz = (phdr->size_in_mem + 0xFFF) / 0x1000;

		if (!alloc_mem_at(pid, phdr->load_to & 0xFFFFF000, sz)) {
			sys_debug_log("exec: failed to alloc memory\n");
			return -3;
		}

		proc_memcpy(pid, (uintptr_t)elf_file + phdr->data_offset, phdr->load_to, phdr->size_in_mem);
	}
	
	if (!alloc_mem_at(pid, 0xA0000000, 0x4)) {
		sys_debug_log("exec: failed to alloc stack\n");
		return -4;
	}

	sys_make_ready(pid, hdr->entry, 0xA0004000);

	return pid;
}

void respond(int32_t to, int status, int32_t pid) {
	struct spawn_response resp;
	resp.status = status;
	resp.pid = pid;

	sys_ipc_send(to, sizeof(resp), &resp);
}

void handle_ipc_message() {
	char num_buf[2] = {0, 0};

	int32_t sender = sys_ipc_get_sender();
	size_t size = sys_ipc_recv(NULL);
	char buf[size];
	sys_ipc_recv(buf);
	sys_ipc_remove();

	struct spawn_message *msg = (struct spawn_message *)buf;
	
	sys_debug_log("exec: parsing message\n");

	switch (msg->operation) {
	case OPERATION_SPAWN_NEW: {
		sys_debug_log("exec: spawning a new process\n");
		int32_t p = exec_spawn_new(sender, msg);
		if (p < 0) respond(sender, p, -1);
		else respond(sender, 0, p);
		break;
	}
	
	case OPERATION_EXEC:
	case OPERATION_FORK:
	default:
		sys_debug_log("exec: operation not implemented\n");
		respond(sender, -2, -1);
		break;
	
	case OPERATION_EXIT:
		sys_exit(0);
	}

}

void _start(void) {
	sys_debug_log("exec: starting\n");
	sys_debug_log("exec: waiting for messages...\n");

	while (1) {
		sys_waitipc();
		handle_ipc_message();
	}

	sys_debug_log("exec: exiting, somehow...\n");
	sys_exit(-1);
}
