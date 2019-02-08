/*
 * quack init server
 * */

#include <stdint.h>
#include <stddef.h>

#include "sys/syscall.h"
#include "sys/elf.h"

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

void create_proc_from_elf(int32_t pid, void *elf_file) {
	if(!elf_check_header((elf_hdr *)elf_file)){
		sys_debug_log("init: failed to spawn exec server\n");
		sys_exit(1);
	}

	elf_hdr *hdr = (elf_hdr *)elf_file;

	for(int i=0; i < hdr->ph_ent_cnt; i++) {
		elf_program_header *phdr = elf_get_program_header(elf_file, i);

		if(phdr->type != SEGTYPE_LOAD) {
			continue;
		}

		uint32_t sz = (phdr->size_in_mem + 0xFFF) / 0x1000;

		if (!alloc_mem_at(pid, phdr->load_to & 0xFFFFF000, sz)) {
			sys_debug_log("init: failed to alloc code/data for exec server\n");
			sys_exit(1);
		}

		proc_memcpy(pid, (uintptr_t)elf_file + phdr->data_offset, phdr->load_to, phdr->size_in_mem);
	}
	
	if (!alloc_mem_at(pid, 0xA0000000, 0x4)) {
		sys_debug_log("init: failed to alloc stack for exec server\n");
		sys_exit(1);
	}

	sys_make_ready(pid, hdr->entry, 0xA0004000);
}

void _start(void) {
	char num_buf[2] = {0, 0};

	size_t exec_size = sys_ipc_recv(NULL);
	char exec[exec_size];
	sys_ipc_recv(exec);
	sys_ipc_remove();

	sys_debug_log("init: welcome to quack\n");

	sys_debug_log("init: spawning exec server\n");
	int32_t pid = sys_spawn_new(sys_getpid(), 1);
	create_proc_from_elf(pid, exec);

	sys_debug_log("init: messaging exec server\n");
	char msg[] = "hi!";
	sys_ipc_send(pid, 4, msg);

	sys_debug_log("init: waiting for response\n");
	sys_waitipc();
	int32_t sender = sys_ipc_get_sender();
	size_t size = sys_ipc_recv(NULL);
	char buf[size];
	sys_ipc_recv(buf);

	sys_ipc_remove();

	sys_debug_log("init: received a message from pid ");
	num_buf[0] = '0' + sender;
	sys_debug_log(num_buf);

	sys_debug_log(", size ");
	num_buf[0] = '0' + size;
	sys_debug_log(num_buf);
	sys_debug_log(" bytes");

	sys_debug_log(", contains \"");
	sys_debug_log(buf);
	sys_debug_log("\"\n");

	sys_debug_log("init: exiting\n");
	sys_exit(0);
}
