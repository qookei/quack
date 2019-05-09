#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "../sys/global_data.h"
#include "../sys/syscall.h"
#include "uuid.h"
#include "liballoc.h"

enum msg_type {
	msg_invalid = 0,
	msg_generic_mesg,
	msg_generic_resp,
	msg_generic_status,

	msg_driver_event_subscribe,
	msg_driver_event_input_device,

	msg_initfs_get_file,
	msg_initfs_file_resp,

	msg_serv_manage,
	msg_serv_get,
	msg_serv_name_resp,

	msg_exec_req,
	msg_exec_resp,

	msg_logd_log,
};

enum msg_status {
	msg_status_ok = 0,
	msg_status_empty,
	msg_status_invalid,
	msg_status_failed,
};

const char *msg_get_status_name(int status);

typedef struct {
	uint8_t uuid[16];
} uuid_t;

typedef struct {
	uint32_t type;
	uint8_t uuid[16];
	uint64_t size;
	uint8_t data[];
} message_t;

// ----------

typedef struct {
	int status;
} msg_generic_status_t;

// ----------

enum input_event_types {
	input_event_key = 1,
	input_event_button,
	input_event_mouse_move,
};

enum input_event_modes {
	input_event_axis_x = 1,
	input_event_axis_y,
	input_event_up,
	input_event_down,
};

typedef struct {
	int32_t subscriber;
} msg_driver_event_subscribe_t;

typedef struct {
	uint64_t time;
	uint8_t type;
	uint32_t mode;
	uint64_t value;
} input_event_t;

typedef struct {
	size_t n_events;
	input_event_t events[];
} msg_driver_event_user_input_t;

// -----------

typedef struct {
	char filename[128];
} msg_initfs_request_t;

typedef struct {
	int status;
	size_t size;
	uint8_t data[];
} msg_initfs_response_t;

// -----------

enum msg_exec_operations {
	msg_exec_spawn_new,
	msg_exec_fork,
	msg_exec_exec,
	msg_exec_exit,
};

typedef struct {
	uint32_t operation;
	int is_privileged;
	size_t binary_size;
	uint8_t binary[];
} msg_exec_request_t;

typedef struct {
	int status;
	int32_t pid;
} msg_exec_response_t;

// -----------

enum msg_serv_manage_opers {
	msg_serv_manage_add = 1,
	msg_serv_manage_remove,
};

typedef struct {
	int type;
	int32_t pid;
	char name[32];
} msg_serv_manage_t;

typedef struct {
	int by_name;
	int32_t pid;
	char name[32];
} msg_serv_get_t;

typedef struct {
	int status;
	int32_t pid;
	char name[32];
} msg_serv_response_t;

message_t *message_new_uuid(uint32_t type, void *data, size_t size, uuid_t *uuid);
message_t *message_new(uint32_t type, void *data, size_t size);

int message_send_new_uuid(uint32_t type, void *data, size_t size, uuid_t *uuid, int32_t pid);
int message_send_new(uint32_t type, void *data, size_t size, int32_t pid);

int message_send(message_t *msg, int32_t pid);
int message_recv(bool blocking, message_t **msg, int32_t *pid);

#endif
