/*
 * quack vga text mode tty driver
 * */

#include <stdint.h>
#include <stddef.h>

#include "sys/syscall.h"

#define WIDTH 80
#define HEIGHT 25

void *memset(void *arr, int val, size_t len) {
	for (size_t i = 0; i < len; i++)
		((unsigned char*)arr)[i] = (unsigned char)val;
	return arr;
}

void *memcpy(void *dest, const void *src, size_t len) {
	for (size_t i = 0; i < len; i++)
		((unsigned char*)dest)[i] = ((unsigned char*)src)[i];
	return dest;
}

void *memmove(void *dest, const void *src, size_t len) {
	unsigned char cpy[len];
	memcpy(cpy, src, len);
	return memcpy(dest, cpy, len);
}

void copy_line(void *vram, int y1, int y2) {
	for (size_t i = 0; i < WIDTH * 2; i++)
		((unsigned char*)vram)[y1 * WIDTH * 2 + i] = ((unsigned char*)vram)[y2 * WIDTH * 2 + i];
}

void _start(void) {
	int x = 0, y = 0;
	uint8_t *vram;

	sys_debug_log("vgatty: initializing tty interface\n");

	sys_map_to(sys_getpid(), 0xB8000, 0xB8000);
	vram = (uint8_t *)0xB8000;

	while(1) {
		sys_waitipc();

		size_t recv_size = sys_ipc_recv(NULL);
		char buf[recv_size];
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
	}

	sys_debug_log("vgatty: exitting\n");
	sys_exit(0);
}
