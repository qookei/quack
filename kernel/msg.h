#ifndef MSG_H
#define MSG_H

#include <stdint.h>
#include <stddef.h>

struct message {
	void *data;
	size_t size;
	int32_t sender;
};

// TODO: make the queue dynamically resize itself
#define MAX_MESSAGES 128

struct message_queue {
	struct message *msgs[MAX_MESSAGES];

	size_t dequeue;
	size_t enqueue;

	size_t elem_count;
};

int msg_queue_push(struct message_queue *queue, struct message *msg);
struct message *msg_queue_peek(struct message_queue *queue);
void msg_queue_pop(struct message_queue *queue);
size_t msg_queue_size(struct message_queue *queue);

#endif
