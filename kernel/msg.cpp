#include "msg.h"
#include <mm/heap.h>

int msg_queue_push(struct message_queue *queue, struct message *msg) {
	queue->msgs[queue->enqueue] = msg;
	int pos = queue->enqueue;

	queue->elem_count++;

	if (++queue->enqueue >= MAX_MESSAGES)
		queue->enqueue -= MAX_MESSAGES;

	return pos;
}

struct message *msg_queue_peek(struct message_queue *queue) {
	struct message *msg = queue->msgs[queue->dequeue];

	return msg;
}

void msg_queue_pop(struct message_queue *queue) {
	kfree(queue->msgs[queue->dequeue]);
	queue->msgs[queue->dequeue] = NULL;

	queue->elem_count--;

	if (++queue->dequeue >= MAX_MESSAGES)
		queue->dequeue -= MAX_MESSAGES;
}

size_t msg_queue_size(struct message_queue *queue) {
	return queue->elem_count;
}
