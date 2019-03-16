/*
 * quack server manager
 * */

#include <stdint.h>
#include <stddef.h>

#include "../sys/syscall.h"
#include "../sys/uuid.h"

#define SERV_MANAGE 0xF00F0001
#define SERV_GET 0xF00F0002
#define SERV_RESPONSE 0xF00F0003

#define SERV_MANAGE_ADD 0x1
#define SERV_MANAGE_REMOVE 0x2

#define MIN(x, y) ((x) > (y) ? (y) : (x))

struct message {
	uint32_t type;
	uint8_t uuid[16];
	uint8_t data[];
};

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

struct server {
	int32_t pid;
	char name[32];
};

#define MAX_SERVERS 64

struct server servers[64];

void *memset(void *dst, int n, size_t len) {
	for (size_t i = 0; i < len; i++)
		((unsigned char *)dst)[i] = n;

	return dst;
}

void *memcpy(void *dst, const void *src, size_t len) {
	for (size_t i = 0; i < len; i++)
		((unsigned char *)dst)[i] = ((const unsigned char *)src)[i];

	return dst;
}

size_t strlen(const char *s) {
	size_t l = 0;
	while (s[l]) l++;
	return l;
}

struct msg_serv_resp serv_manage(struct msg_serv_manage *m) {
	m->name[31] = 0;

	struct msg_serv_resp r;

	if (m->type == SERV_MANAGE_ADD) {
		int i = 0;
		while (i < MAX_SERVERS && servers[i].pid) i++;
		
		if (i == MAX_SERVERS - 1 && servers[i].pid) {
			r.status = -1;
		} else {
			servers[i].pid = m->pid;
			memcpy(servers[i].name, m->name, MIN(strlen(m->name) + 1, 32));
			r.status = 0;
		}
	} else if (m->type == SERV_MANAGE_REMOVE) {
		int i = 0;
		while (i < MAX_SERVERS && servers[i].pid != m->pid) i++;
		
		if (i == MAX_SERVERS - 1 && servers[i].pid) {
			r.status = -2;
		} else {
			servers[i].pid = 0;
			servers[i].name[0] = 0;
			r.status = 0;
		}

	} else {
		sys_debug_log("servman: what? unsupported manage operation\n");
		r.status = -3;
	}

	return r;
}

int strcmp(const char *str1, const char *str2) {
	size_t i = 0;
	while (str1[i] && str1[i] == str2[i]) i++;

	return str1[i] - str2[i];
}

struct msg_serv_resp serv_get(struct msg_serv_get *m) {
	m->name[31] = 0;

	struct msg_serv_resp r;

	if (m->pid) {
		int i = 0;
		while (i < MAX_SERVERS && servers[i].pid != m->pid) i++;
		
		if (i == MAX_SERVERS - 1 && servers[i].pid != m->pid) {
			r.status = -1;
		} else {
			r.pid = m->pid;
			memcpy(r.name, servers[i].name, MIN(strlen(servers[i].name) + 1, 32));
			r.status = 0;
		}
	} else if (m->name[0]) {
		int i = 0;
		while (i < MAX_SERVERS && strcmp(servers[i].name, m->name)) i++;
		
		if (i == MAX_SERVERS - 1 && strcmp(servers[i].name, m->name)) {
			r.status = -2;
		} else {
			r.pid = servers[i].pid;	
			memcpy(r.name, servers[i].name, MIN(strlen(servers[i].name) + 1, 32));
			r.status = 0;
		}

	} else {
		sys_debug_log("servman: what? no identifiable information specificed\n");
		r.status = -3;
	}

	return r;
}

void respond_to(int32_t pid, struct msg_serv_resp resp) {
	char buf[sizeof(struct message) + sizeof(struct msg_serv_resp)];
	struct message *m = (struct message *)buf;
	m->type = SERV_RESPONSE;
	uuid_generate(0, m->uuid); // TODO
	memcpy(m->data, &resp, sizeof(resp));

	sys_ipc_send(pid, sizeof(buf), buf);
}

void handle_ipc_message() {
	int32_t sender = sys_ipc_get_sender();
	size_t size = sys_ipc_recv(NULL);

	if ((size > sizeof(struct message) + sizeof(struct msg_serv_manage)) ||
		(size > sizeof(struct message) + sizeof(struct msg_serv_get))) {

		sys_debug_log("servman: message too large\n");
		sys_ipc_remove();

		struct msg_serv_resp r = {.status = -4};
		respond_to(sender, r);
		return;
	}

	char buf[size];
	sys_ipc_recv(buf);
	sys_ipc_remove();

	struct message *msg = (struct message *)buf;
	
	sys_debug_log("servman: parsing message\n");

	switch (msg->type) {
	case SERV_MANAGE: {
		struct msg_serv_resp r = serv_manage((struct msg_serv_manage *)msg->data);
		respond_to(sender, r);
		break;
	}
	
	case SERV_GET: {
		struct msg_serv_resp r = serv_get((struct msg_serv_get *)msg->data);
		respond_to(sender, r);
		break;
	}
	
	default:
		sys_debug_log("servman: what? unsupported operation\n");
		struct msg_serv_resp r = {.status = -5};
		respond_to(sender, r);
		break;
	}

}

void _start(void) {
	sys_debug_log("servman: starting\n");
	sys_debug_log("servman: waiting for messages...\n");

	while (1) {
		sys_wait(WAIT_IPC, 0, NULL, NULL);
		handle_ipc_message();
	}

	sys_debug_log("servman: exiting, somehow...\n");
	sys_exit(-1);
}
