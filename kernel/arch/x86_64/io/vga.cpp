#include <stdint.h>
#include <stddef.h>

#include <string.h>

#define WIDTH 80
#define HEIGHT 25

#include <io/port.h>
#include <mm/mm.h>

static void copy_line(void *vram, int y1, int y2) {
	for (size_t i = 0; i < WIDTH * 2; i++)
		((unsigned char*)vram)[y1 * WIDTH * 2 + i] = ((unsigned char*)vram)[y2 * WIDTH * 2 + i];
}

static int x, y;
static uint8_t *vram;

void vga_init(void) {
	x = 0;
	y = 0;
	vram = (uint8_t *)(0xB8000 + VIRT_PHYS_BASE);

	outb(0x3D4, 0x0A);
	outb(0x3D5, (inb(0x3D5) & 0xC0) | 0);

	outb(0x3D4, 0x0B);
	outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);

	memset(vram, 0, WIDTH * 2 * HEIGHT);
}

void vga_putch(char c) {
	if (c == '\n') {
		x = 0;
		y++;
		if (y >= HEIGHT) {
			for (size_t j = 0 ; j < HEIGHT - 1; j++)
				copy_line(vram, j, j + 1);
			memset(vram + WIDTH * 2 * (HEIGHT - 1), 0, 0xA0);
			y = HEIGHT - 1;
		}
		return;
	}

	if (c == '\t') {
		for (int i = 0; i < 8; i++) vga_putch(' ');
		return;
	}
	
	vram[(y * WIDTH + x) * 2] = c;
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

	uint16_t pos = y * WIDTH + x;
	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}
