/*
 * quack init server
 * */

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include "../sys/syscall.h"
#include "../sys/elf.h"
#include "../sys/uuid.h"

#define OPERATION_SPAWN_NEW 0x1
#define OPERATION_EXEC 0x2
#define OPERATION_FORK 0x3
#define OPERATION_EXIT 0x4

char *itoa(uint32_t i, int base, int padding) {
	static char buf[50];
	char *ptr = buf + 49;
	*ptr = '\0';

	do {
		*--ptr = "0123456789ABCDEF"[i % base];
		if (padding)
			padding--;
	} while ((i /= base) != 0);

	while (padding) {
		*--ptr = '0';
		padding--;
	}

	return ptr;

}

void usprintf(char *buf, const char *fmt, ...) {
	va_list arg;
	va_start(arg, fmt);

	uint32_t i;
	char *s;

	while(*fmt) {
		if (*fmt != '%') {
			*buf++ = *fmt;
			fmt++;
			continue;
		}

		fmt++;
		int padding = 0;
		if (*fmt >= '0' && *fmt <= '9')
			padding = *fmt++ - '0';

		switch (*fmt) {
			case 'c': {
				i = va_arg(arg, int);
				*buf++ = i;
				break;
			}

			case 'u': {
				i = va_arg(arg, int);
				char *c = itoa(i, 10, padding);
				while (*c)
					*buf++ = *c++;
				break;
			}

			case 'x': {
				i = va_arg(arg, int);
				char *c = itoa(i, 16, padding);
				while (*c)
					*buf++ = *c++;
				break;
			}

			case 's': {
				s = va_arg(arg, char *);
				while (*s)
					*buf++ = *s++;
				break;
			}

			case '%': {
				*buf++ = '%';
				break;
			}
		}

		fmt++;
	}

	*buf++ = '\0';

	va_end(arg);
}
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

void *memset(void *dst, int val, size_t len) {
	for (size_t i = 0; i < len; i++)
		((unsigned char *)dst)[i] = val;

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

		uint32_t sz = ((phdr->load_to & 0xFFF) + phdr->size_in_mem + 0xFFF) / 0x1000;

		if (!alloc_mem_at(pid, phdr->load_to & 0xFFFFF000, sz)) {
			sys_debug_log("init: failed to alloc code/data for exec server\n");
			sys_exit(1);
		}

		proc_memcpy(pid, (uintptr_t)elf_file + phdr->data_offset, phdr->load_to, phdr->size_in_file);
	}

	if (!alloc_mem_at(pid, 0xA0000000, 0x4)) {
		sys_debug_log("init: failed to alloc stack for exec server\n");
		sys_exit(1);
	}

	sys_make_ready(pid, hdr->entry, 0xA0004000);
}

struct spawn_response resp;

int32_t spawn(int32_t exec_pid, void *data, size_t size) {
	size_t ssize = size + sizeof (struct spawn_message);
	void *tmp_msg = sys_sbrk(ssize);

	struct spawn_message *s_msg = (struct spawn_message *)tmp_msg;
	s_msg->operation = OPERATION_SPAWN_NEW;
	s_msg->is_privileged = 1;
	s_msg->binary_size = size;
	memcpy(s_msg->binary, data, size);
	sys_ipc_send(exec_pid, ssize, tmp_msg);

	sys_wait(WAIT_IPC, 0, NULL, NULL);
	int32_t r = sys_ipc_recv(&resp);
	sys_ipc_remove();

	sys_sbrk(-ssize);

	return resp.pid;
}

char buf[128];
char exec[10240];
char i8042d[10240];
char vgatty[10240];

#define DRIVER_EVENT_SUBSCRIBE 0xDEAD0001
#define DRIVER_EVENT_KEY_TYPED 0xDEAD0002
#define MESSAGE_RESPONSE 0xCAFE0001

struct message {
	uint32_t type;
	uint8_t uuid[16];
	uint8_t data[];
};

struct event_subscribe_msg {
	int32_t subscriber;
};

struct msg_response {
	int status;
};

struct event_key_typed {
	uint32_t scancode;
	uint32_t character;
};

void _start(void) {
	size_t exec_size = sys_ipc_recv(NULL);
	sys_ipc_recv(exec);
	sys_ipc_remove();

	size_t i8042d_size = sys_ipc_recv(NULL);
	sys_ipc_recv(i8042d);
	sys_ipc_remove();

	size_t vgatty_size = sys_ipc_recv(NULL);
	sys_ipc_recv(vgatty);
	sys_ipc_remove();

	sys_map_to(sys_getpid(), 0xB8000, 0xB8000);

	sys_debug_log("init: welcome to quack\n");

	sys_debug_log("init: spawning exec server\n");
	int32_t pid = sys_spawn_new(sys_getpid(), 1);
	create_proc_from_elf(pid, exec);

	sys_debug_log("init: spawning a process with the exec server\n");

	int32_t i8042d_pid = spawn(pid, i8042d, i8042d_size);
	int32_t vgatty_pid = spawn(pid, vgatty, vgatty_size);

	sys_map_timer(0x1000);
	uint64_t *timer = (uint64_t *)0x1000;

	char resp_buf[sizeof(struct message) + sizeof(struct event_subscribe_msg)];
	struct message *msg = (struct message *)resp_buf;
	struct event_subscribe_msg *resp = (struct event_subscribe_msg *)msg->data;
	uuid_generate(*timer, msg->uuid);
	msg->type = DRIVER_EVENT_SUBSCRIBE;
	resp->subscriber = sys_getpid();

	sys_ipc_send(i8042d_pid, sizeof(resp_buf), resp_buf);
	sys_wait(WAIT_IPC, 0, NULL, NULL);

	sys_ipc_remove();

	while(1) {
		sys_wait(WAIT_IPC, 0, NULL, NULL);

		int32_t sender = sys_ipc_get_sender();
		size_t size = sys_ipc_recv(NULL);
		char buf[size];
		sys_ipc_recv(buf);
		sys_ipc_remove();

		struct message *m = (struct message *)buf;
		struct event_key_typed *ev = (struct event_key_typed *)m->data;

		char c[2];
		memset(c, 0, 2);
		c[0] = ev->character;

		sys_ipc_send(vgatty_pid, 2, c);
	}

	sys_debug_log("init: exiting\n");
	sys_exit(0);
}
