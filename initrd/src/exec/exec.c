/*
 * quack early exec server
 * */

#include <stdint.h>
#include <stddef.h>

#include <string.h>

#include <syscall.h>
#include <liballoc.h>
#include <elf.h>

#include <message.h>
#include <debug_out.h>

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
		
		sys_map_to(sys_getpid(), 0xB0000000, phys);
		memset((void *)0xB0000000, 0, 0x1000);
		sys_unmap_from(sys_getpid(), 0xB0000000);

		virt += 0x1000;
		pages--;
	}

	return 1;
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

int32_t exec_spawn_new(int32_t parent, msg_exec_request_t *msg) {
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

		uint32_t sz = ((phdr->load_to & 0xFFF) + phdr->size_in_mem + 0xFFF) / 0x1000;

		if (!alloc_mem_at(pid, phdr->load_to & 0xFFFFF000, sz)) {
			sys_debug_log("exec: failed to alloc memory\n");
			return -3;
		}

		proc_memcpy(pid, (uintptr_t)elf_file + phdr->data_offset, phdr->load_to, phdr->size_in_file);
	}
	
	if (!alloc_mem_at(pid, 0xA0000000, 0x4)) {
		sys_debug_log("exec: failed to alloc stack\n");
		return -4;
	}

	sys_make_ready(pid, hdr->entry, 0xA0004000);

	return pid;
}

void respond(int32_t to, int status, int32_t pid) {
	msg_exec_response_t resp;
	resp.status = status;
	resp.pid = pid;

	message_send_new(msg_exec_resp, &resp, sizeof(resp), to);
}

void handle_ipc_message() {
	int32_t sender;
	message_t *msg;
	message_recv(true, &msg, &sender);

	if (msg->type != msg_exec_req) {
		sys_debug_log("exec: eek...! not a msg_exec_req!\n");
		free(msg);
		return;
	}

	msg_exec_request_t *req = (msg_exec_request_t *)msg->data;

	sys_debug_log("exec: parsing message\n");

	switch (req->operation) {
	case msg_exec_spawn_new: {
		sys_debug_log("exec: spawning a new process\n");
		int32_t p = exec_spawn_new(sender, req);
		if (p < 0) respond(sender, p, -1);
		else respond(sender, 0, p);
		break;
	}

	case msg_exec_fork:
	case msg_exec_exec:
	default:
		sys_debug_log("exec: operation not implemented\n");
		respond(sender, -2, -1);
		break;
	
	case msg_exec_exit:
		sys_exit(0);
	}

	free(msg);
}

void _start(void) {
	sys_debug_log("exec: starting\n");
	sys_debug_log("exec: waiting for messages...\n");

	while (1) {
		handle_ipc_message();
	}

	sys_debug_log("exec: exiting, somehow...\n");
	sys_exit(-1);
}
