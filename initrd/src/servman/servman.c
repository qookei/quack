/*
 * quack server manager
 * */

#include <stdint.h>
#include <stddef.h>

#include <string.h>

#include <syscall.h>
#include <uuid.h>
#include <liballoc.h>

#include <message.h>
#include <debug_out.h>

#define MIN(x, y) ((x) > (y) ? (y) : (x))

struct server {
	int32_t pid;
	char name[32];
};

#define MAX_SERVERS 64

struct server servers[64];

msg_serv_response_t serv_manage(msg_serv_manage_t *m) {
	m->name[31] = 0;

	msg_serv_response_t r;

	debugf("servman: manage: %u %s %u\n", m->type, m->name, m->pid);

	if (m->type == msg_serv_manage_add) {
		int i = 0;
		while (i < MAX_SERVERS && servers[i].pid) i++;
		
		if (i == MAX_SERVERS - 1 && servers[i].pid) {
			r.status = -1;
		} else {
			servers[i].pid = m->pid;
			memcpy(servers[i].name, m->name, 32);
			r.status = 0;
		}
	} else if (m->type == msg_serv_manage_remove) {
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

msg_serv_response_t serv_get(msg_serv_get_t *m) {
	m->name[31] = 0;

	msg_serv_response_t r;

	debugf("servman: get! %s %u\n", m->name, m->pid);

	if (m->pid) {
		int i = 0;
		while (i < MAX_SERVERS && servers[i].pid != m->pid) i++;
		
		if (i == MAX_SERVERS - 1 && servers[i].pid != m->pid) {
			r.status = -1;
		} else {
			r.pid = m->pid;
			memcpy(r.name, servers[i].name, 32);
			r.status = 0;
		}
	} else if (m->name[0]) {
		int i = 0;
		while (i < MAX_SERVERS && strcmp(servers[i].name, m->name)) i++;
		
		if (i == MAX_SERVERS - 1 && strcmp(servers[i].name, m->name)) {
			r.status = -2;
		} else {
			r.pid = servers[i].pid;	
			memcpy(r.name, servers[i].name, 32);
			debugf("servman: found %s at pid %u\n", servers[i].name, servers[i].pid);
			r.status = 0;
		}

	} else {
		sys_debug_log("servman: what? no identifiable information specificed\n");
		r.status = -3;
	}

	return r;
}

void respond_to(int32_t pid, msg_serv_response_t resp) {
	message_send_new(msg_serv_name_resp, &resp, sizeof(msg_serv_response_t), pid);
}

void handle_ipc_message() {
	message_t *msg;
	int32_t sender;
	message_recv(true, &msg, &sender);
		
	sys_debug_log("servman: parsing message\n");

	switch (msg->type) {
	case msg_serv_manage: {
		msg_serv_response_t r = serv_manage((msg_serv_manage_t *)msg->data);
		respond_to(sender, r);
		break;
	}
	
	case msg_serv_get: {
		msg_serv_response_t r = serv_get((msg_serv_get_t *)msg->data);
		respond_to(sender, r);
		break;
	}
	
	default:
		sys_debug_log("servman: what? unsupported operation\n");
		msg_serv_response_t r = {.status = -5};
		respond_to(sender, r);
		break;
	}

	free(msg);
}

void _start(void) {
	sys_debug_log("servman: starting\n");
	sys_debug_log("servman: waiting for messages...\n");

	while (1) {
		handle_ipc_message();
	}

	sys_debug_log("servman: exiting, somehow...\n");
	sys_exit(-1);
}
