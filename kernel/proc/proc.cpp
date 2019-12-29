#include "proc.h"
#include <mm/heap.h>
#include <string.h>

int proc_send_no_copy_msg(int32_t from, int32_t to, void *data, size_t size) {
	struct thread *t = sched_get_thread(to);
	if (!t) {
		kmesg("proc", "failed to get thread with tid %d", to);
		return -1;
	}

	struct message *msg = kmalloc(sizeof(struct message));
	msg->data = data;
	msg->size = size;
	msg->sender = from;

	msg_queue_push(t->msg_queue, msg);

	return 0;
}

int proc_send_msg(int32_t from, int32_t to, void *data, size_t size) {
	void *tmp = kmalloc(size);
	memcpy(tmp, data, size);
	int result = proc_send_no_copy_msg(from, to, tmp, size);

	if (result < 0)
		kfree(tmp);

	return result;
}

struct message *proc_peek_msg(int32_t tid) {
	struct thread *t = sched_get_thread(tid);
	if (!t) {
		return NULL;
	}

	struct message *m = msg_queue_peek(t->msg_queue);

	return m;
}

int proc_pop_msg(int32_t tid) {
	struct thread *t = sched_get_thread(tid);
	if (!t) {
		return -1;
	}

	msg_queue_pop(t->msg_queue);

	return 0;
}

size_t proc_msg_count(int32_t tid) {
	struct thread *t = sched_get_thread(tid);
	if (!t) {
		return -1;
	}

	size_t size = msg_queue_size(t->msg_queue);

	return size;
}
