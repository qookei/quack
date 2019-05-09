/*
 * quack message logger server
 * */

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <liballoc.h>
#include <string.h>
#include <ctype.h>

#include <syscall.h>
#include <uuid.h>
#include <message.h>

#include <debug_out.h>
#include <global_data.h>

#define SERVMAN_PID 4

global_data_t *gd = GLOBAL_DATA_PTR;

int32_t get_server_by_name(const char *name) {
	msg_serv_get_t req;
	memset(&req, 0, sizeof(req));
	strcpy(req.name, name);
	req.pid = 0;
	req.by_name = 1;

	int status = message_send_new(msg_serv_get, &req, sizeof(req), SERVMAN_PID);

	if (status != msg_status_ok) {
		debugf("init: error sending message to servman: %s\n", msg_get_status_name(status));
		return -status;
	}

	message_t *recv;	
	status = message_recv(true, &recv, NULL);

	if (status != msg_status_ok) {
		debugf("init: failed to recv message from servman: %s\n", msg_get_status_name(status));
		return -status;
	}

	msg_serv_response_t *resp = (msg_serv_response_t *)recv->data;
	if (resp->status < 0) {
		sys_debug_log("init: specified server was not found\n");
		return -msg_status_failed;
	}

	debugf("logd: proc: %s %u %u\n", resp->name, resp->pid, resp->status);

	int r = resp->pid;
	free(recv);

	return r;
}

void server_register_self(const char *name) {
	msg_serv_manage_t req;
	memset(&req, 0, sizeof(req));
	strcpy(req.name, name);
	req.pid = sys_getpid();
	req.type = msg_serv_manage_add;

	int status = message_send_new(msg_serv_manage, &req, sizeof(req), SERVMAN_PID);

	if (status != msg_status_ok) {
		debugf("init: error sending message to servman: %s\n", msg_get_status_name(status));
		return;
	}

	message_t *recv;
	status = message_recv(true, &recv, NULL);

	if (status != msg_status_ok) {
		debugf("init: failed to recv message from servman: %s\n", msg_get_status_name(status));
		return;
	}

	msg_serv_response_t *resp = (msg_serv_response_t *)recv->data;

	if (resp->status < 0) {
		sys_debug_log("init: specified server was not found\n");
		return;
	}

	free(recv);
}

void _start(void) {
	sys_debug_log("logd: initializing\n");

	int32_t vgatty = get_server_by_name("vgatty"); // TODO: make the log destination selectable

	server_register_self("logd");

	debugf("logd: vgatty is at pid %u\n", vgatty);

	while(1) {
		sys_wait(WAIT_IPC, 0, NULL, NULL);
		size_t recv_size = sys_ipc_recv(NULL);
		char *buf = malloc(recv_size);
		sys_ipc_recv(buf);
		sys_ipc_remove();

		buf[recv_size - 1] = 0; // set terminator just in case
		sys_debug_log(buf);

		sys_ipc_send(vgatty, recv_size, buf);

		free(buf);
	}

	sys_debug_log("logd: exiting\n");
	sys_exit(0);
}
