/*
 * quack init server
 * */

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <liballoc.h>
#include <string.h>
#include <ctype.h>

#include <syscall.h>
#include <elf.h>
#include <uuid.h>
#include <message.h>

#include <debug_out.h>

#include "multiboot.h"

#define EXEC_PID 2
#define INITFS_PID 3
#define SERVMAN_PID 4

int32_t spawn(int32_t exec_pid, void *data, size_t size) {
	size_t req_size = sizeof(msg_exec_request_t) + size;
	msg_exec_request_t *req = calloc(req_size, 1);
	req->operation = msg_exec_spawn_new;
	req->binary_size = size;
	memcpy(req->binary, data, size);
	req->is_privileged = 1;

	int status = message_send_new(msg_exec_req, req, req_size, EXEC_PID);

	if (status != msg_status_ok) {
		debugf("init: error sending message to exec: %s\n", msg_get_status_name(status));
		return -status;
	}

	free(req);

	message_t *recv;
	status = message_recv(true, &recv, NULL);

	if (status != msg_status_ok) {
		debugf("init: error blocking receiving a message: %s\n", msg_get_status_name(status));
		return -status;
	}

	msg_exec_response_t *resp = (msg_exec_response_t *)recv->data;

	if (resp->status < 0) {
		sys_debug_log("init: failed to spawn process\n");
		return -msg_status_failed;
	}

	int32_t pid = resp->pid;
	free(recv);

	return pid;
}

int32_t launch_from_initrd(const char *path) {
	msg_initfs_request_t req;
	memset(&req, 0, sizeof(req));
	strcpy(req.filename, path);

	int status = message_send_new(msg_initfs_get_file, &req, sizeof(req), INITFS_PID);

	if (status != msg_status_ok) {
		debugf("init: error sending message to initfs: %s\n", msg_get_status_name(status));
		return -status;
	}

	message_t *recv;
	status = message_recv(true, &recv, NULL);

	if (status != msg_status_ok) {
		debugf("init: error recv message from initfs: %s\n", msg_get_status_name(status));
		return -status;
	}

	msg_initfs_response_t *resp = (msg_initfs_response_t *)recv->data;

	if (resp->status < 0) {
		sys_debug_log("init: failed to read file from initrd\n");
		return -msg_status_failed;
	}

	int r = spawn(EXEC_PID, resp->data, resp->size);
	free(recv);

	return r;
}

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
		debugf("init: error sending message to initfs: %s\n", msg_get_status_name(status));
		return -status;
	}

	msg_serv_response_t *resp = (msg_serv_response_t *)recv->data;

	if (resp->status < 0) {
		sys_debug_log("init: failed to read file from initrd\n");
		return -msg_status_failed;
	}

	int r = resp->pid;
	free(recv);

	return r;
}

void _start(void) {
	sys_debug_log("init: welcome to quack\n");

	multiboot_info_t info;
	sys_ipc_recv(&info);
	sys_ipc_remove();

	int32_t servman_pid = launch_from_initrd("servman");
	int32_t vgatty_pid = launch_from_initrd("vgatty");
	int32_t logd_pid = launch_from_initrd("logd");

	global_data_t *gd = GLOBAL_DATA_PTR;
	uint64_t s = gd->timer_ticks + 1000;
	while(gd->timer_ticks < s) asm volatile ("" : : : "memory");

	while(1) {
		char *c = "init: Hello world from init!\n";
		sys_ipc_send(logd_pid, strlen(c) + 1, c);
	}

	sys_debug_log("init: exiting\n");
	sys_exit(0);
}
