/*
 * quack vga text mode tty driver
 * */

#include <stdint.h>
#include <stddef.h>

#include <string.h>

#include <syscall.h>
#include <liballoc.h>
#include <message.h>
#include <debug_out.h>

#define SERVMAN_PID 4

#define WIDTH 80
#define HEIGHT 25

void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

uint8_t inb(uint16_t port) {
	uint8_t ret;
	asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

void copy_line(void *vram, int y1, int y2) {
	for (size_t i = 0; i < WIDTH * 2; i++)
		((unsigned char*)vram)[y1 * WIDTH * 2 + i] = ((unsigned char*)vram)[y2 * WIDTH * 2 + i];
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
	int x = 0, y = 0;
	uint8_t *vram;

	sys_debug_log("vgatty: initializing tty interface\n");

	server_register_self("vgatty");

	sys_map_to(sys_getpid(), 0xB8000, 0xB8000);
	vram = (uint8_t *)0xB8000;

	sys_enable_ports(0x3D4, 1);
	sys_enable_ports(0x3D5, 1);

	outb(0x3D4, 0x0A);
	outb(0x3D5, (inb(0x3D5) & 0xC0) | 0);

	outb(0x3D4, 0x0B);
	outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);

	memset(vram, 0, WIDTH * 2 * HEIGHT);

	while(1) {
		sys_wait(WAIT_IPC, 0, NULL, NULL);

		size_t recv_size = sys_ipc_recv(NULL);
		char *buf = malloc(recv_size);
		sys_ipc_recv(buf);
		sys_ipc_remove();

		for (size_t i = 0; i < recv_size; i++) {
			if (!buf[i]) break;

			if (buf[i] == '\n') {
				x = 0;
				y++;
				if (y >= HEIGHT) {
					for (size_t j = 0 ; j < HEIGHT - 1; j++)
						copy_line(vram, j, j + 1);
					memset(vram + WIDTH * 2 * (HEIGHT - 1), 0, 0xA0);
					y = HEIGHT - 1;
				}
				continue;
			}
			
			vram[(y * WIDTH + x) * 2] = buf[i];
			vram[(y * WIDTH + x) * 2 + 1] = 0x7;
			x++;
			if (x >= WIDTH) {
				x = 0;
				y++;
				if (y >= HEIGHT) {
					for (size_t j = 0 ; j < HEIGHT - 1; j++)
						copy_line(vram, j, j + 1);
					memset(vram + WIDTH * 2 * (HEIGHT - 1), 0, 0xA0);
					y = HEIGHT - 1;
				}
			}
		}

		uint16_t pos = y * WIDTH + x;
		outb(0x3D4, 0x0F);
		outb(0x3D5, (uint8_t) (pos & 0xFF));
		outb(0x3D4, 0x0E);
		outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
	
		free(buf);
	}

	sys_debug_log("vgatty: exitting\n");
	sys_exit(0);
}
