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

uint32_t reqres(uint32_t type) {
	uint32_t _val;
	asm ("int $0x30" : "=a"(_val) : "a"(8), "b"(type));
	return _val;
}

void exit() {
	asm ("int $0x30" : : "a"(0));
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

struct vmode {
	uint32_t w, h;
	uint32_t p, b;
};

struct mpack {
	uint32_t x;
	uint32_t y;
	uint32_t b;
};

typedef enum { false, true } bool;

void fillpx(uint8_t *vram, struct vmode video, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t r, uint8_t g, uint8_t b) {
	uint32_t orig_pos = x * (video.b / 8) + y * video.p;
	uint32_t bb = (video.b / 8);
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

void bitmappx(uint16_t *bitmap, uint8_t *vram, struct vmode video, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t r, uint8_t g, uint8_t b) {
	uint32_t orig_pos = x * (video.b / 8) + y * video.p;
	uint32_t bb = (video.b / 8);
	uint32_t mask = (1 << (w));
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
}, fm[9] = {
	0b0000,
	0b0000,
	0b1001,
	0b1111,
	0b1011,
	0b1001,
	0b1001,
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

void _start(void) {
	struct mpack mouse;
	struct vmode video;
	int mouse_handle = open("/dev/mouse", 0),
	    video_handle = open("/dev/videomode", 0);
	read(video_handle, &video, 16);
	close(video_handle);

	uint8_t *vram = (uint8_t *)reqres(RESOURCE_FRAME_BUFFER);

	if ((uint32_t)vram == 0xFFFFFFFF){
		write(1, err, strlen(err));
		close(mouse_handle);
		exit();
	}

	uint32_t oldx			= 0,	menuw			= 200,	app_x			= 300,	fontw	= 4,
		 oldy			= 0,	menuh			= 400,	app_y			= 200,	fonth	= 9,
		 backgroud_r		= 0x28,	backgroud_g		= 0x28, backgroud_b		= 0x28,
		 statusbar_r		= 0x1d,	statusbar_g		= 0x20, statusbar_b		= 0x21,
		 statusbar_text_r	= 0xeb, statusbar_text_g	= 0xdb, statusbar_text_b	= 0xb2,
		 menu_r			= 0x1d,	menu_g			= 0x20,	menu_b			= 0x21,
		 menu_text_r		= 0xeb, menu_text_g		= 0xdb, menu_text_b		= 0xb2,
		 app_bar_active_r	= 0x68, app_bar_active_g	= 0x9d, app_bar_active_b	= 0x6a,
		 app_bar_active_text_r	= 0xeb,	app_bar_active_text_g	= 0xdb,	app_bar_active_text_b	= 0xb2,
		 statusbar		= 18,	wy 			= (statusbar - fonth) / 2;
	bool menu = false, app = false;

	fillpx(vram, video, 0, 0, video.w, video.h, backgroud_r, backgroud_g, backgroud_b);
	fillpx(vram, video, 0, 0, video.w, statusbar, statusbar_r, statusbar_g, statusbar_b);
	bitmappx(fP, vram, video, 5, wy, fontw, fonth, statusbar_text_r, statusbar_text_g, statusbar_text_b);
	bitmappx(fo, vram, video, 10, wy, fontw, fonth, statusbar_text_r, statusbar_text_g, statusbar_text_b);
	bitmappx(fn, vram, video, 15, wy, fontw, fonth, statusbar_text_r, statusbar_text_g, statusbar_text_b);
	bitmappx(fd, vram, video, 20, wy, fontw, fonth, statusbar_text_r, statusbar_text_g, statusbar_text_b);
	bitmappx(ipower, vram, video, video.w - 15, wy, 5, 5, statusbar_text_r, statusbar_text_g, statusbar_text_b);

	while (1) {
		// mouse
		int i = read(mouse_handle, &mouse, 18);
		if (i > 0) {
			if (mouse.b & (1<<2)) { // left mouse button
				if (mouse.y <= statusbar && mouse.x > video.w - 35) {
					write(1, "\ec", 2);
					exit();
				} else if (menu && mouse.y <= 50 && mouse.y >= 30 && mouse.x <= menuw) {
					fillpx(vram, video, 0, statusbar, menuw, menuh, backgroud_r, backgroud_g, backgroud_b);
					fillpx(vram, video, app_x, app_y, 300, statusbar, app_bar_active_r, app_bar_active_g, app_bar_active_b);
					fillpx(vram, video, app_x, app_y + statusbar, 300, 200, 0x00, 0x00, 0x00);
					bitmappx(fd, vram, video, app_x + 5, app_y + 4, fontw, fonth, app_bar_active_text_r, app_bar_active_text_g, app_bar_active_text_b);
					bitmappx(fe, vram, video, app_x + 10, app_y + 4, fontw, fonth, app_bar_active_text_r, app_bar_active_text_g, app_bar_active_text_b);
					bitmappx(fm, vram, video, app_x + 15, app_y + 4, fontw, fonth, app_bar_active_text_r, app_bar_active_text_g, app_bar_active_text_b);
					bitmappx(fo, vram, video, app_x + 20, app_y + 4, fontw, fonth, app_bar_active_text_r, app_bar_active_text_g, app_bar_active_text_b);
					bitmappx(iclose, vram, video, app_x + 290, app_y + 6, 5, 5, app_bar_active_text_r, app_bar_active_text_g, app_bar_active_text_b);
					menu = !menu;
					app = true;
				} else if (app && mouse.x >= app_x && mouse.x < app_x + 282 && mouse.y >= app_y && mouse.y < app_y + 18) {
					fillpx(vram, video, app_x, app_y, 300, 218, backgroud_r, backgroud_g, backgroud_b);

					if (menu) {
						menu = !menu;
						fillpx(vram, video, 0, statusbar, menuw, menuh, backgroud_r, backgroud_g, backgroud_b);
					} if (app_x > 0 || mouse.x - oldx <= 1)
						app_x = app_x + mouse.x - oldx;
					if (app_y > statusbar || mouse.y - oldy <= 1)
						app_y = app_y + mouse.y - oldy;

					fillpx(vram, video, app_x, app_y, 300, statusbar, app_bar_active_r, app_bar_active_g, app_bar_active_b);
					fillpx(vram, video, app_x, app_y + statusbar, 300, 200, 0x00, 0x00, 0x00);
					bitmappx(fd, vram, video, app_x + 5, app_y + 4, fontw, fonth, app_bar_active_text_r, app_bar_active_text_g, app_bar_active_text_b);
					bitmappx(fe, vram, video, app_x + 10, app_y + 4, fontw, fonth, app_bar_active_text_r, app_bar_active_text_g, app_bar_active_text_b);
					bitmappx(fm, vram, video, app_x + 15, app_y + 4, fontw, fonth, app_bar_active_text_r, app_bar_active_text_g, app_bar_active_text_b);
					bitmappx(fo, vram, video, app_x + 20, app_y + 4, fontw, fonth, app_bar_active_text_r, app_bar_active_text_g, app_bar_active_text_b);
					bitmappx(iclose, vram, video, app_x + 290, app_y + 6, 5, 5, app_bar_active_text_r, app_bar_active_text_g, app_bar_active_text_b);
				} else if (app && mouse.y <= app_y + 18 && mouse.y >= app_y && mouse.x <= app_x + 300 && mouse.x >= app_x + 282) {
					fillpx(vram, video, app_x, app_y, 300, 218, backgroud_r, backgroud_g, backgroud_b);
					app_x = 300;
					app_y = 200;
					app = false;
				}
			}

			if (mouse.y >= 18) { // refresh main window
				bitmappx(cursor_tilemap, vram, video, oldx, oldy, 8, 14, backgroud_r, backgroud_g, backgroud_b); // clear mouse track
				if (app && mouse.x >= app_x && mouse.x < app_x + 290) {
					if (mouse.y >= app_y + statusbar && mouse.y < app_y + 200 + statusbar)
						fillpx(vram, video, app_x, app_y + statusbar, 300, 200, 0x00, 0x00, 0x00);
					else if (mouse.y >= app_y && mouse.y < app_y + statusbar) {
						fillpx(vram, video, app_x, app_y, 300, statusbar, app_bar_active_r, app_bar_active_g, app_bar_active_b);
						bitmappx(fd, vram, video, app_x + 5, app_y + 4, fontw, fonth, app_bar_active_text_r, app_bar_active_text_g, app_bar_active_text_b);
						bitmappx(fe, vram, video, app_x + 10, app_y + 4, fontw, fonth, app_bar_active_text_r, app_bar_active_text_g, app_bar_active_text_b);
						bitmappx(fm, vram, video, app_x + 15, app_y + 4, fontw, fonth, app_bar_active_text_r, app_bar_active_text_g, app_bar_active_text_b);
						bitmappx(fo, vram, video, app_x + 20, app_y + 4, fontw, fonth, app_bar_active_text_r, app_bar_active_text_g, app_bar_active_text_b);
						bitmappx(iclose, vram, video, app_x + 290, app_y + 6, 5, 5, app_bar_active_text_r, app_bar_active_text_g, app_bar_active_text_b);
					}
				}
			} else if (mouse.y <= 20) { // refresh statusbar
				int wx = 0;
				bitmappx(cursor_tilemap, vram, video, oldx, oldy, 8, 14, statusbar_r, statusbar_g, statusbar_b); // clear mouse track

				// refresh letters
				bitmappx(fP, vram, video, wx += 5, wy, fontw, fonth, statusbar_text_r, statusbar_text_g, statusbar_text_b);
				bitmappx(fo, vram, video, wx += 5, wy, fontw, fonth, statusbar_text_r, statusbar_text_g, statusbar_text_b);
				bitmappx(fn, vram, video, wx += 5, wy, fontw, fonth, statusbar_text_r, statusbar_text_g, statusbar_text_b);
				bitmappx(fd, vram, video, wx += 5, wy, fontw, fonth, statusbar_text_r, statusbar_text_g, statusbar_text_b);
				bitmappx(ipower, vram, video, video.w - 15, wy, 5, 5, menu_text_r, menu_text_g, menu_text_b);
			}

			if (mouse.y >= 18) {
				if (menu && mouse.x < menuw && mouse.y >= statusbar && mouse.y < menuh + statusbar)
					bitmappx(cursor_tilemap, vram, video, oldx, oldy, 8, 14, menu_r, menu_g, menu_b);
				if (menu && mouse.x < menuw && mouse.y >= 30 && mouse.y < 50) {
					int wx = 5;
					bitmappx(fd, vram, video, wx += 5, 40, fontw, fonth, menu_text_r, menu_text_g, menu_text_b);
					bitmappx(fe, vram, video, wx += 5, 40, fontw, fonth, menu_text_r, menu_text_g, menu_text_b);
					bitmappx(fm, vram, video, wx += 5, 40, fontw, fonth, menu_text_r, menu_text_g, menu_text_b);
					bitmappx(fo, vram, video, wx += 5, 40, fontw, fonth, menu_text_r, menu_text_g, menu_text_b);
				}
			}


			if (mouse.b & (1<<2)) { // left mouse button
				if (mouse.y <= statusbar && mouse.x <= 48) {
					menu = !menu;
					if (menu) {
						int wx = 5;
						fillpx(vram, video, 0, statusbar, menuw, menuh, menu_r, menu_g, menu_b);
						bitmappx(fd, vram, video, wx += 5, 40, fontw, fonth, menu_text_r, menu_text_g, menu_text_b);
						bitmappx(fe, vram, video, wx += 5, 40, fontw, fonth, menu_text_r, menu_text_g, menu_text_b);
						bitmappx(fm, vram, video, wx += 5, 40, fontw, fonth, menu_text_r, menu_text_g, menu_text_b);
						bitmappx(fo, vram, video, wx += 5, 40, fontw, fonth, menu_text_r, menu_text_g, menu_text_b);
					} else {
						fillpx(vram, video, 0, statusbar, menuw, menuh, backgroud_r, backgroud_g, backgroud_b);
						if (app) {
							fillpx(vram, video, app_x, app_y, 300, statusbar, app_bar_active_r, app_bar_active_g, app_bar_active_b);
							fillpx(vram, video, app_x, app_y + statusbar, 300, 200, 0x00, 0x00, 0x00);
							bitmappx(fd, vram, video, app_x + 5, app_y + 4, fontw, fonth, app_bar_active_text_r, app_bar_active_text_g, app_bar_active_text_b);
							bitmappx(fe, vram, video, app_x + 10, app_y + 4, fontw, fonth, app_bar_active_text_r, app_bar_active_text_g, app_bar_active_text_b);
							bitmappx(fm, vram, video, app_x + 15, app_y + 4, fontw, fonth, app_bar_active_text_r, app_bar_active_text_g, app_bar_active_text_b);
							bitmappx(fo, vram, video, app_x + 20, app_y + 4, fontw, fonth, app_bar_active_text_r, app_bar_active_text_g, app_bar_active_text_b);
							bitmappx(iclose, vram, video, app_x + 290, app_y + 6, 5, 5, app_bar_active_text_r, app_bar_active_text_g, app_bar_active_text_b);
						}
					}
				}
			}

			oldx = mouse.x;
			oldy = mouse.y;

			bitmappx(cursor_tilemap, vram, video, mouse.x, mouse.y, 8, 14, 0x00, 0x00, 0x00);
			bitmappx(cursor_border, vram, video, mouse.x, mouse.y, 8, 14, 0xFF, 0xFF, 0xFF);
		}
	}
	exit();
}
