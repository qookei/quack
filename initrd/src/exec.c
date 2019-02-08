/*
 * quack early exec server
 * */

#include <stdint.h>
#include <stddef.h>

#include "sys/syscall.h"

void _start(void) {
	char num_buf[2] = {0, 0};

	sys_debug_log("exec: starting\n");
	sys_debug_log("exec: waiting for messages...\n");

	sys_waitipc();

	int32_t sender = sys_ipc_get_sender();
	size_t size = sys_ipc_recv(NULL);
	char buf[size];
	sys_ipc_recv(buf);

	sys_ipc_remove();

	sys_debug_log("exec: received a message from pid ");
	num_buf[0] = '0' + sender;
	sys_debug_log(num_buf);

	sys_debug_log(", size ");
	num_buf[0] = '0' + size;
	sys_debug_log(num_buf);
	sys_debug_log(" bytes");

	sys_debug_log(", contains \"");
	sys_debug_log(buf);
	sys_debug_log("\"\n");

	sys_debug_log("exec: sending back a message\n");
	char msg[] = "hello!";
	sys_ipc_send(sender, 7, msg);

	sys_debug_log("exec: exiting\n");
	sys_exit(0);
}
