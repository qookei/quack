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

void *memset(void *dst, int val, size_t len) {
	for (size_t i = 0; i < len; i++)
		((unsigned char *)dst)[i] = val;

	return dst;
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

void *memcpy(void *dst, const void *src, size_t len) {
	for (size_t i = 0; i < len; i++)
		((unsigned char *)dst)[i] = ((const unsigned char *)src)[i];

	return dst;
}

size_t strlen(const char *s) {
	size_t l = 0;
	while(s[l]) l++;
	return l;
}

#define SERV_MANAGE 0xF00F0001
#define SERV_GET 0xF00F0002
#define SERV_RESPONSE 0xF00F0003

#define SERV_MANAGE_ADD 0x1
#define SERV_MANAGE_REMOVE 0x2

struct msg_serv_manage {
	int type;
	int32_t pid;
	char name[32];
};

struct msg_serv_get {
	int32_t pid;
	int by_name;
	char name[32];
};

struct msg_serv_resp {
	int status;
	int32_t pid;
	char name[32];
};

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

struct msg_fs_file_resp {
	int status;
	size_t size;
	uint8_t data[];
};

char rbuf[20480];

#define FS_REQUEST_FILE 0xBEEF0001
#define FS_FILE_RESPONSE 0xCAFE0002

#define EXEC_PID 2
#define INITFS_PID 3

int32_t launch_from_initrd(const char *path) {
	char buf[sizeof(struct message) + strlen(path) + 1];
	struct message *m = (struct message *)buf;
	m->type = FS_REQUEST_FILE;
	uuid_generate(0, m->uuid);
	memcpy(m->data, path, strlen(path) + 1);
	
	sys_ipc_send(INITFS_PID, sizeof(buf), buf);
	sys_wait(WAIT_IPC, 0, NULL, NULL);

	size_t size = sys_ipc_recv(NULL);
	sys_ipc_recv(rbuf);
	sys_ipc_remove();

	struct message *rm = (struct message *)rbuf;
	struct msg_fs_file_resp *resp = (struct msg_fs_file_resp *)rm->data;

	if (resp->status < 0) {
		sys_debug_log("init: failed to read file from initrd\n");
		return -1;
	}

	return spawn(EXEC_PID, resp->data, resp->size);
}

int32_t get_server_by_name(const char *name) {
	char sbuf[sizeof(struct message) + sizeof(struct msg_serv_get)];
	struct message *msg = (struct message *)sbuf;
	struct msg_serv_get *get = (struct msg_serv_get *)msg->data;
	uuid_generate(0, msg->uuid);
	msg->type = SERV_GET;
	memcpy(get->name, name, strlen(name) + 1);
	get->pid = 0;

	sys_ipc_send(4, sizeof(sbuf), sbuf);
	sys_wait(WAIT_IPC, 0, NULL, NULL);

	size_t size = sys_ipc_recv(NULL);
	char rbuf[size];
	sys_ipc_recv(rbuf);
	sys_ipc_remove();

	struct message *m = (struct message *)rbuf;
	struct msg_serv_resp *resp = (struct msg_serv_resp *)m->data;

	return resp->pid;
}

void _start(void) {
	sys_debug_log("init: welcome to quack\n");

	int32_t servman_pid = launch_from_initrd("servman");
	int32_t vgatty_pid = launch_from_initrd("vgatty");

	char resp_buf[sizeof(struct message) + sizeof(struct msg_serv_manage)];
	struct message *msg = (struct message *)resp_buf;
	struct msg_serv_manage *resp = (struct msg_serv_manage *)msg->data;
	uuid_generate(0, msg->uuid);
	msg->type = SERV_MANAGE;
	resp->type = SERV_MANAGE_ADD;
	memcpy(resp->name, "testing", 8);
	resp->pid = 0xDEAD;

	sys_ipc_send(servman_pid, sizeof(resp_buf), resp_buf);
	sys_wait(WAIT_IPC, 0, NULL, NULL);

	sys_ipc_remove();

	int32_t id = get_server_by_name("testing");

	char tmp[256];
	usprintf(tmp, "init: pid of testing is %u\n", id);
	sys_debug_log(tmp);

	sys_debug_log("init: exiting\n");
	sys_exit(0);
}
