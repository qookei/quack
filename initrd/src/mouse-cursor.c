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
	uint32_t w;
	uint32_t h;
	uint32_t p;
	uint32_t b;
};

struct mpack {
	uint32_t x;
	uint32_t y;
	uint32_t b;
};

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
		for (uint32_t xx = 0; xx < w; xx++) {
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

uint16_t cursor_tilemap[] = {

	0b10000000,
	0b11000000,
	0b11100000,
	0b11110000,
	0b11111000,
	0b11111100,
	0b11111110,
	0b11111111,
	0b11111000,
	0b11011000,
	0b10001100,
	0b00001100,
	0b00000110,
	0b00000110,



};

uint32_t w_x, w_y;

void _start(void) {

	struct mpack mouse;
	struct vmode video;
	int mouse_handle = open("/dev/mouse", 0);
	
	int video_handle = open("/dev/videomode", 0);
	read(video_handle, &video, 16);
	close(video_handle);

	uint8_t *vram = (uint8_t *)reqres(RESOURCE_FRAME_BUFFER);

	write(1, "\ec", 2);

	if ((uint32_t)vram == 0xFFFFFFFF){
		write(1, err, strlen(err));
		close(mouse_handle);
		exit();
	}

	w_x = 0; w_y = 0;

	uint32_t oldx = 0, oldy = 0;

	fillpx(vram, video, 0, 0, video.w, video.h, 0x00, 0x80, 0x80);

	while(1) {
		int i = read(mouse_handle, &mouse, 12);
		
		
		if (i > 0) {

			if(mouse.b & (1<<2)) {

				if (mouse.x >= w_x && mouse.x < w_x + 400 && mouse.y >= w_y && mouse.y < w_y + 200) {
					
					fillpx(vram, video, w_x, w_y, 400, 200, 0x00, 0x80, 0x80);

					w_x = w_x + mouse.x - oldx;
					w_y = w_y + mouse.y - oldy;

					fillpx(vram, video, w_x, w_y, 400, 200, 0xFF, 0xFF, 0xFF);
				}


			}

			if (mouse.x >= w_x && mouse.x < w_x + 400 && mouse.y >= w_y && mouse.y < w_y + 186)
				bitmappx(cursor_tilemap, vram, video, oldx, oldy, 8, 14, 0xFF, 0xFF, 0xFF);
			else
				bitmappx(cursor_tilemap, vram, video, oldx, oldy, 8, 14, 0x00, 0x80, 0x80);
			
			oldx = mouse.x;
			oldy = mouse.y;

		}


		bitmappx(cursor_tilemap, vram, video, mouse.x, mouse.y, 8, 14, 0x80, 0x80, 0x80);
	}
}