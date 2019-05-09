#include "message.h"

#include "string.h"

static const char *status_names[] = {
	"ok",
	"empty",
	"invalid",
	"failed"
};

const char *msg_get_status_name(int status) {
	return status_names[status];
}

message_t *message_new_uuid(uint32_t type, void *data, size_t size, uuid_t *uuid) {	
	message_t *msg = calloc(sizeof(message_t) + size, 1);
	msg->type = type;
	msg->size = size;

	memcpy(msg->uuid, uuid->uuid, 16);
	memcpy(msg->data, data, size);

	return msg;
}

message_t *message_new(uint32_t type, void *data, size_t size) {
	uuid_t tmp;
	global_data_t *gd = GLOBAL_DATA_PTR;
	uuid_generate(gd->timer_ticks, tmp.uuid);
	
	return message_new_uuid(type, data, size, &tmp);
}

int message_send_new_uuid(uint32_t type, void *data, size_t size, uuid_t *uuid, int32_t pid) {
	message_t *msg = message_new_uuid(type, data, size, uuid);

	int i = message_send(msg, pid);
	free(msg);

	return i;
}

int message_send_new(uint32_t type, void *data, size_t size, int32_t pid) {
	message_t *msg = message_new(type, data, size);

	int i = message_send(msg, pid);
	free(msg);

	return i;
}

int message_send(message_t *msg, int32_t pid) {

	if (!msg)
		return msg_status_invalid;

	int status = sys_ipc_send(pid, sizeof(message_t) + msg->size, msg);

	if (status == -2)
		return msg_status_invalid;

	if (status == -3)
		return msg_status_failed;

	return msg_status_ok;
}

int message_recv(bool blocking, message_t **dst, int32_t *pid) {
	if (blocking)
		sys_wait(WAIT_IPC, 0, NULL, NULL);
	if (!sys_ipc_queue_length())
		return msg_status_empty;

	size_t s = sys_ipc_recv(NULL);
	void *msg = calloc(s, 1);
	
	int stat = sys_ipc_recv(msg);
	if (stat == -1) {
		free(msg);
		return msg_status_failed;
	}

	if (stat != s) {
		free(msg);
		return msg_status_invalid;
	}

	if (dst) *dst = msg;
	if (pid) *pid = sys_ipc_get_sender();

	sys_ipc_remove();
	return msg_status_ok;
}
