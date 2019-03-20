/*
 * quack i8042 driver/daemon
 * */

#include <stdint.h>
#include <stddef.h>

#include <global_data.h>

#include <syscall.h>
#include <uuid.h>

#define bittest(var,pos) ((var) & (1 << (pos)))
#define ps2_wait_ready() {while(bittest(inb(0x64), 1));}
#define ps2_wait_response() {while(bittest(inb(0x64), 0));}

void outb(uint16_t port, uint8_t val) {
	asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

uint8_t inb(uint16_t port) {
	uint8_t ret;
	asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

const char lower_normal[] = { '\0', '?', '1', '2', '3', '4', '5', '6',
		'7', '8', '9', '0', '-', '=', '\b', '\t', 'q', 'w', 'e', 'r', 't', 'y',
		'u', 'i', 'o', 'p', '[', ']', '\n', '\0', 'a', 's', 'd', 'f', 'g',
		'h', 'j', 'k', 'l', ';', '\'', '`', '\0', '\\', 'z', 'x', 'c', 'v',
		'b', 'n', 'm', ',', '.', '/', '\0', '\0', '\0', ' '};


int32_t respond_to[8] = {0};

#define DRIVER_EVENT_SUBSCRIBE 0xDEAD0001
#define DRIVER_EVENT_KEY_TYPED 0xDEAD0002
#define MESSAGE_RESPONSE 0xCAFE0001

struct message {
	uint32_t type;
	uint8_t uuid[16];
	uint8_t data[];
};

struct event_subscribe_msg {
	int32_t subscriber;
};

struct msg_response {
	int status;
};

struct event_key_typed {
	uint32_t scancode;
	uint32_t character;
};

void _start(void) {
	char num_buf[2] = {0, 0};

	sys_debug_log("i8042d: starting\n");

	sys_register_handler(0x21);

	sys_enable_ports(0x60, 1);
	sys_enable_ports(0x64, 1);

	struct global_data *global_data = (struct global_data *)0xD0000000;

	while (inb(0x64) & 0x01)
		(void)inb(0x60);

	sys_debug_log("i8042d: entering irq wait loop\n");

	int i = 0;

	while(1) {
		int i = sys_wait(WAIT_IRQ | WAIT_IPC, 0, NULL, NULL);
		if (i == WAIT_IRQ) {
			uint8_t b = inb(0x60);

			char resp_buf[sizeof(struct message) + sizeof(struct event_key_typed)];
			struct message *msg = (struct message *)resp_buf;
			struct event_key_typed *resp = (struct event_key_typed *)msg->data;
			uuid_generate(global_data->timer_ticks, msg->uuid);
			msg->type = DRIVER_EVENT_KEY_TYPED;
			resp->scancode = b;
			resp->character = b < 0x57 ? lower_normal[b] : 0x00;

			for (int i = 0; i < 8; i++) {
				if (respond_to[i]) {
					sys_ipc_send(respond_to[i], sizeof(resp_buf), resp_buf);
				}
			}
		} else {
			int32_t sender = sys_ipc_get_sender();
			size_t size = sys_ipc_recv(NULL);
			char buf[size];
			sys_ipc_recv(buf);
			sys_ipc_remove();

			struct message *m = (struct message *)buf;
			if (m->type == DRIVER_EVENT_SUBSCRIBE) {
				struct event_subscribe_msg *ev = (struct event_subscribe_msg *)m->data;
				int was_set = 0;
				for (int i = 0; i < 8; i++) {
					if (!respond_to[i]) {
						respond_to[i] = ev->subscriber;
						was_set = 1;
						break;
					}
				}

				char resp_buf[sizeof(struct message) + sizeof(struct msg_response)];
				struct message *msg = (struct message *)resp_buf;
				struct msg_response *resp = (struct msg_response *)msg->data;
				uuid_generate(global_data->timer_ticks, msg->uuid);
				msg->type = MESSAGE_RESPONSE;
				resp->status = !was_set ? -1 : 0;

				sys_ipc_send(sender, sizeof(resp_buf), resp_buf);
			}
		}
	}

	sys_debug_log("i8042d: exiting\n");
	sys_exit(0);
}
