#ifndef PROC_H
#define PROC_H

#include "scheduler.h"
#include <msg.h>

// TODO: add support for multiple message queues

int proc_send_no_copy_msg(int32_t from, int32_t to, void *data, size_t size);
int proc_send_msg(int32_t from, int32_t to, void *data, size_t size);

struct message *proc_peek_msg(int32_t tid);
int proc_pop_msg(int32_t tid);
size_t proc_msg_count(int32_t tid);

#endif
