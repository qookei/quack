#include <stdint.h>
#include <stddef.h>

#define RESOURCE_FRAME_BUFFER 0

int read(int handle, void *buffer, size_t count) {
	int _val;
	asm ("int $0x30" : "=a"(_val) : "a"(5), "b"(handle), "c"(buffer), "d"(count));
	return _val;
}

int open(const char *name, uint32_t flags) {
	int _val;
	asm ("int $0x30" : "=a"(_val) : "a"(4), "b"(name), "c"(flags));
	return _val;
}

int write(int handle, void *buffer, size_t count) {
	int _val;
	asm ("int $0x30" : "=a"(_val) : "a"(6), "b"(handle), "c"(buffer), "d"(count));
	return _val;
}

int fork() {
	int _val;
	asm ("mov $1, %%eax; int $0x30; mov %%eax, %0" : "=g"(_val));
	return _val;
}

uint32_t reqres(uint32_t type) {
	uint32_t _val;
	asm ("int $0x30" : "=a"(_val) : "a"(8), "b"(type));
	return _val;
}

void exit() {
	asm ("int $0x30" : : "a"(0));
}

int get_pid() {
	int val;
	asm ("int $0x30" : "=a"(val) : "a"(14));
	return val;
}

int close(int handle) {
	int _val;
	asm ("int $0x30" : "=a"(_val) : "a"(7), "b"(handle));
	return _val;
}

size_t strlen(const char *s) {
	size_t len = 0;
	while(s[len]) {
		len++;
	}
	return len;
}

char *err = "Failed to acquire a frame buffer!\n";

void ipc_wait() {
	asm ("int $0x30" : : "a"(22));
}

void ipc_remove() {
	asm ("int $0x30" : : "a"(20));
}

int ipc_recv(void *dst) {
	int _val;
	asm ("int $0x30" : "=a"(_val) : "a"(19), "b"(dst));
	return _val;
}

int execve(const char *path) {
	int _val;
	asm ("int $0x30" : "=a"(_val) : "a"(2), "b"(path));
	return _val;
}


struct vmode {
	uint32_t w, h;
	uint32_t p, b;
};

struct mpack {
	uint32_t x;
	uint32_t y;
	uint32_t b;
};

void memcpy(void *dst, const void *src, size_t s) {
	while (s--)
		*((char *)dst++) = *((char *)src++);
}

typedef enum { false, true } bool;

void fillpx(uint8_t *vram, struct vmode video, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t rgb) {
	uint8_t r = rgb >> 16,
			g = rgb >> 8 & 0xFF,
			b = rgb & 0xFF;

	uint32_t orig_pos = x * (video.b / 8) + y * video.p,
		 bb = (video.b / 8);
	for (uint32_t yy = 0; yy < h; yy++) {
		if (y + yy >= video.h) break;
		uint32_t xpos = orig_pos;
		for (uint32_t xx = 0; xx < w; xx++) {
			if (x + xx >= video.w) break;
			vram[xpos]     = b;
			vram[xpos + 1] = g;
			vram[xpos + 2] = r;
			xpos += bb;
		}
		orig_pos += video.p;
	}
}

void *sbrk(int increment) {
	uint32_t _val;
	asm ("int $0x30" : "=a"(_val) : "a"(17), "b"(increment));
	return (void *)_val;
}

void bitmappx(uint16_t *bitmap, uint8_t *vram, struct vmode video, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t rgb) {
	uint8_t r = rgb >> 16,
			g = rgb >> 8 & 0xFF,
			b = rgb & 0xFF;

	uint32_t orig_pos = x * (video.b / 8) + y * video.p,
		 bb = (video.b / 8),
		 mask = (1 << (w));
	for (uint32_t yy = 0; yy < h; yy++) {
		if (y + yy >= video.h) break;
		uint32_t xpos = orig_pos;
		for (uint32_t xx = 0; xx <= w; xx++) {
			if (x + xx >= video.w) break;
			if (bitmap[yy] & (mask >> xx)) {
				vram[xpos]     = b;
				vram[xpos + 1] = g;
				vram[xpos + 2] = r;
			}

			xpos += bb;
		}
		orig_pos += video.p;
	}
}

void putpx(uint8_t *vram, struct vmode video, uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b) {
	vram[x * (video.b / 8) + y * video.p]     = r;
	vram[x * (video.b / 8) + y * video.p + 1] = g;
	vram[x * (video.b / 8) + y * video.p + 2] = b;
}

uint16_t cursor_tilemap[15] = {
	0b10000000,
	0b11000000,
	0b11100000,
	0b11110000,
	0b11111000,
	0b11111100,
	0b11111110,
	0b11111111,
	0b11111100,
	0b11011100,
	0b10001110,
	0b00001110,
	0b00000111,
	0b00000111,
}, cursor_border[15] = {
	0b10000000,
	0b11000000,
	0b10100000,
	0b10010000,
	0b10001000,
	0b10000100,
	0b10000010,
	0b10000111,
	0b10100100,
	0b11010100,
	0b10001010,
	0b00001010,
	0b00000101,
	0b00000111,
}, fP[9] = {
	0b0000,
	0b1110,
	0b1001,
	0b1001,
	0b1110,
	0b1000,
	0b1000,
}, fo[9] = {
	0b0000,
	0b0000,
	0b0110,
	0b1001,
	0b1001,
	0b1001,
	0b0110,
}, fn[9] = {
	0b0000,
	0b0000,
	0b1110,
	0b1001,
	0b1001,
	0b1001,
	0b1001,
}, fd[9] = {
	0b0001,
	0b0001,
	0b0111,
	0b1001,
	0b1001,
	0b1011,
	0b0101,
}, fe[9] = {
	0b0000,
	0b0000,
	0b0110,
	0b1001,
	0b1111,
	0b1000,
	0b0111,
}, fa[9] = {
	0b0000,
	0b0000,
	0b0111,
	0b1001,
	0b1001,
	0b1011,
	0b0101,
}, fb[9] = {
	0b1000,
	0b1000,
	0b1110,
	0b1001,
	0b1001,
	0b1001,
	0b1110,
}, fu[9] = {
	0b0000,
	0b0000,
	0b1001,
	0b1001,
	0b1001,
	0b1001,
	0b0111,
}, ft[9] = {
	0b0000,
	0b0100,
	0b1110,
	0b0100,
	0b0100,
	0b0100,
	0b0010,
}, fA[9] = {
	0b0000,
	0b0110,
	0b1001,
	0b1001,
	0b1111,
	0b1001,
	0b1001,
}, fD[9] = {
	0b0000,
	0b1110,
	0b1001,
	0b1001,
	0b1001,
	0b1001,
	0b1110,
}, fE[9] = {
	0b0000,
	0b1111,
	0b1000,
	0b1110,
	0b1000,
	0b1000,
	0b1111,
}, fV[9] = {
	0b0000,
	0b1001,
	0b1001,
	0b1001,
	0b0110,
	0b0110,
	0b0110,
}, fr[9] = {
	0b0000,
	0b0000,
	0b1011,
	0b1100,
	0b1000,
	0b1000,
	0b1000,
}, fh[9] = {
	0b1000,
	0b1000,
	0b1110,
	0b1001,
	0b1001,
	0b1001,
	0b1001,	
}, f0[9] = {
	0b0000,
	0b0110,
	0b1001,
	0b1011,
	0b1101,
	0b1001,
	0b0110,
}, f1[9] = {
	0b0000,
	0b0100,
	0b1100,
	0b0100,
	0b0100,
	0b0100,
	0b0100,
}, fdot[9] = {
	0b0000,
	0b0000,
	0b0000,
	0b0000,
	0b0000,
	0b0100,
	0b0100,
}, iclose[5] = {
	0b10001,
	0b01010,
	0b00100,
	0b01010,
	0b10001,
}, ipower[5] = {
	0b00100,
	0b10101,
	0b10101,
	0b10001,
	0b01110,
};

int serial_handle;

int itoa(int value, char *sp, int radix) {
	char tmp[16], *tp = tmp;
	unsigned v;
	int i, sign = (radix == 10 && value < 0);

	if (sign)
		v = -value;
	else
		v = (unsigned)value;

	while (v || tp == tmp) {
		i = v % radix;
		v /= radix; // v/=radix uses less CPU clocks than v=v/radix does
		if (i < 10)
			*tp++ = i+'0';
		else
			*tp++ = i + 'a' - 10;
	}
	int len = tp - tmp;

	if (sign) {
		*sp++ = '-';
		len++;
	}

	while (tp > tmp)
		*sp++ = *--tp;

	return len;
}

void _start(void) {
	struct mpack mouse;
	struct vmode video;
	int mouse_handle = open("/dev/mouse", 0),
	    video_handle = open("/dev/videomode", 0);
	serial_handle = open("/dev/serial", 0);
	read(video_handle, &video, 16);
	close(video_handle);

	uint8_t *vram = (uint8_t *)reqres(RESOURCE_FRAME_BUFFER);

	if ((uint32_t)vram == 0xFFFFFFFF){
		write(1, err, strlen(err));
		close(mouse_handle);
		exit();
	}

	uint32_t statusbar_rgb		= 0x1d2021, statusbar_text_rgb		= 0xebdbb2,
			 menu_rgb			= 0x1d2021, menu_text_rgb			= 0xebdbb2,
			 app_bar_active_rgb	= 0x303030, app_bar_active_text_rgb = 0xc0c0c0,
			 app_rgb			= 0x212121, app_text_rgb			= 0xd9d9d9,
			 background_rgb		= 0x151515;
	uint16_t oldx = 0, fontw = 4, menuw = 200, app_x = 300, statusbar = 18,
			 oldy = 0, fonth = 9, menuh = 400, app_y = 200, wy		 = (statusbar - fonth) / 2;

	bool menu = false, app = false;

	void letterdraw(char *t, int wx, int wya, uint32_t rgb) {
		size_t len = 0;
		while(t[len])
			len++;
		for (int i = 0; i < len; i++) {
			if (t[i] == 'A')
				bitmappx(fA, vram, video, wx += 5, wya, fontw, fonth, rgb);
			else if (t[i] == 'D')
				bitmappx(fD, vram, video, wx += 5, wya, fontw, fonth, rgb);
			else if (t[i] == 'E')
				bitmappx(fE, vram, video, wx += 5, wya, fontw, fonth, rgb);
			else if (t[i] == 'P')
				bitmappx(fP, vram, video, wx += 5, wya, fontw, fonth, rgb);
			else if (t[i] == 'V')
				bitmappx(fV, vram, video, wx += 5, wya, fontw, fonth, rgb);
			else if (t[i] == 'a')
				bitmappx(fa, vram, video, wx += 5, wya, fontw, fonth, rgb);
			else if (t[i] == 'b')
				bitmappx(fb, vram, video, wx += 5, wya, fontw, fonth, rgb);
			else if (t[i] == 'd')
				bitmappx(fd, vram, video, wx += 5, wya, fontw, fonth, rgb);
			else if (t[i] == 'e')
				bitmappx(fe, vram, video, wx += 5, wya, fontw, fonth, rgb);
			else if (t[i] == 'o')
				bitmappx(fo, vram, video, wx += 5, wya, fontw, fonth, rgb);
			else if (t[i] == 'u')
				bitmappx(fu, vram, video, wx += 5, wya, fontw, fonth, rgb);
			else if (t[i] == 't')
				bitmappx(ft, vram, video, wx += 5, wya, fontw, fonth, rgb);
			else if (t[i] == 'h')
				bitmappx(fh, vram, video, wx += 5, wya, fontw, fonth, rgb);
			else if (t[i] == 'n')
				bitmappx(fn, vram, video, wx += 5, wya, fontw, fonth, rgb);
			else if (t[i] == 'r')
				bitmappx(fr, vram, video, wx += 5, wya, fontw, fonth, rgb);
			else if (t[i] == 'h')
				bitmappx(fh, vram, video, wx += 5, wya, fontw, fonth, rgb);
			else if (t[i] == '0')
				bitmappx(f0, vram, video, wx += 5, wya, fontw, fonth, rgb);
			else if (t[i] == '1')
				bitmappx(f1, vram, video, wx += 5, wya, fontw, fonth, rgb);
			else if (t[i] == '.')
				bitmappx(fdot, vram, video, wx += 5, wya, fontw, fonth, rgb);
			else if (t[i] == ' ')
				wx += 5;
		}
	}

	void demodraw(int a) {
		
		if (a == 0 || a == 2) {
			int wya = app_y + statusbar + 10,
				wx = app_x + 5;
			fillpx(vram, video, app_x, app_y + statusbar, 300, 200, app_rgb);

			letterdraw("About the Pond DE", wx, wya, app_text_rgb);

			wya += fonth + 2;
			wx = app_x + 5;

			letterdraw("Ver 0.1", wx, wya, app_text_rgb);
		}
		if (a == 1 || a == 2) {
			int wx = app_x,
			    wya = wy + app_y;
			fillpx(vram, video, app_x, app_y, 300, statusbar, app_bar_active_rgb);
			letterdraw("about", wx, wya, app_bar_active_text_rgb);
			bitmappx(iclose, vram, video, app_x + 290, app_y + 6, 5, 5, app_bar_active_text_rgb);
		}
		if (a == 3)
			fillpx(vram, video, app_x, app_y, 300, 218, background_rgb);
	}

	void paneldraw(int a) {
		if (a == 0 || a == 3)
			fillpx(vram,video, 0, 0, video.w, statusbar, statusbar_rgb);
		if (a == 1 || a == 3 || a == 4) {
			int wx = 0;
			letterdraw("Pond", wx, wy, statusbar_text_rgb);
		}
		if (a == 2 || a == 3 || a == 4)
			bitmappx(ipower, vram, video, video.w - 15, wy, 5, 5, menu_text_rgb);
	}

	void menudraw(int a) {
		if (a == 0 || a == 2)
			fillpx(vram, video, 0, statusbar, menuw, menuh, menu_rgb);
		if (a == 1 || a == 2) {
			int wx = 0;
			letterdraw("about", wx, 40, menu_text_rgb);
		}
		if (a == 3)
			fillpx(vram, video, 0, statusbar, menuw, menuh, background_rgb);
	}

	fillpx(vram, video, 0, 0, video.w, video.h, background_rgb);
	paneldraw(3);

	while (1) {
		// mouse
		int i = read(mouse_handle, &mouse, 18);
		if (i > 0) {
			if (mouse.b & 1) { // left mouse button
				if (mouse.y <= statusbar && mouse.x > video.w - 35)
					exit();
				else if (menu && mouse.y <= 50 && mouse.y >= 30 && mouse.x <= menuw) {
					menudraw(3);
					demodraw(2);
					menu = !menu;
					app = true;
				} else if (app && mouse.x >= app_x && mouse.x < app_x + 282 && mouse.y >= app_y && mouse.y < app_y + 18) {
					demodraw(3);

					if (menu) {
						menu = !menu;
						menudraw(3);
					} if (app_x > 0 || mouse.x - oldx <= 1)
						app_x = app_x + mouse.x - oldx;
					if (app_y > statusbar || mouse.y - oldy <= 1)
						app_y = app_y + mouse.y - oldy;

					demodraw(2);
				} else if (app && mouse.y <= app_y + 18 && mouse.y >= app_y && mouse.x <= app_x + 300 && mouse.x >= app_x + 282) {
					demodraw(3);
					app_x = 300;
					app_y = 200;
					app = false;
				}

				if (mouse.y <= statusbar && mouse.x <= 48) {
					menu = !menu;
					if (menu)
						menudraw(2);
					else {
						menudraw(3);
						if (app)
							demodraw(2);
					}
				}
			}

			if (mouse.x != oldx || mouse.y != oldy) {
				if (mouse.y >= 18) { // refresh main window
					bitmappx(cursor_tilemap, vram, video, oldx, oldy, 8, 14, background_rgb); // clear mouse track
					if (app && mouse.x >= app_x && mouse.x < app_x + 290) {
						if (mouse.y >= app_y + statusbar && mouse.y < app_y + 200 + statusbar)
							demodraw(0);
						else if (mouse.y >= app_y && mouse.y < app_y + statusbar)
							demodraw(1);
					}
					if (menu) {
						if (mouse.x < menuw && mouse.y >= statusbar && mouse.y < menuh + statusbar)
							bitmappx(cursor_tilemap, vram, video, oldx, oldy, 8, 14, menu_rgb);
						if (mouse.x < menuw && mouse.y >= 30 && mouse.y < 50)
							menudraw(1);
					}
				} else if (mouse.y <= 20) { // refresh statusbar
					bitmappx(cursor_tilemap, vram, video, oldx, oldy, 8, 14, statusbar_rgb);
					if (mouse.x < 48)
						paneldraw(1);
					else if (mouse.x > video.w - 35)
						paneldraw(2);
				}
			}

			oldx = mouse.x;
			oldy = mouse.y;

			bitmappx(cursor_tilemap, vram, video, mouse.x, mouse.y, 8, 14, 0x000000);
			bitmappx(cursor_border, vram, video, mouse.x, mouse.y, 8, 14, 0xFFFFFF);
		}
	}
	exit();
}
