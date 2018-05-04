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

typedef enum { false, true } bool;

void putpx(uint8_t *vram, struct vmode video, uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b) {
	vram[x * (video.b / 8) + y * video.p]     = r;
	vram[x * (video.b / 8) + y * video.p + 1] = g;
	vram[x * (video.b / 8) + y * video.p + 2] = b;
}
void _start(void) {

	char fP[9][4] = {
				{'*','*','*',' '},
				{'*',' ',' ','*'},
				{'*',' ',' ','*'},
				{'*',' ',' ','*'},
				{'*','*','*',' '},
				{'*',' ',' ',' '},
				{'*',' ',' ',' '},
				{'*',' ',' ',' '},
				{'*',' ',' ',' '},
			 };

	char fo[9][4] = {
				{' ',' ',' ',' '},
				{' ',' ',' ',' '},
				{' ',' ',' ',' '},
				{' ',' ',' ',' '},
				{' ','*','*',' '},
				{'*',' ',' ','*'},
				{'*',' ',' ','*'},
				{'*',' ',' ','*'},
				{' ','*','*',' '},
			 };

	char fn[9][4] = {
				{' ',' ',' ',' '},
				{' ',' ',' ',' '},
				{' ',' ',' ',' '},
				{' ',' ',' ',' '},
				{'*','*','*',' '},
				{'*',' ',' ','*'},
				{'*',' ',' ','*'},
				{'*',' ',' ','*'},
				{'*',' ',' ','*'},
			 };

	char fd[9][4] = {
				{' ',' ',' ','*'},
				{' ',' ',' ','*'},
				{' ',' ',' ','*'},
				{' ',' ',' ','*'},
				{' ','*','*','*'},
				{'*',' ',' ','*'},
				{'*',' ',' ','*'},
				{'*',' ',' ','*'},
				{' ','*','*','*'},
			 };

	struct mpack mouse;
	struct vmode video;
	int mouse_handle = open("/dev/mouse", 0);
	
	int video_handle = open("/dev/videomode", 0);
	read(video_handle, &video, 16);
	close(video_handle);

	uint8_t *vram = (uint8_t *)reqres(RESOURCE_FRAME_BUFFER);

	if ((uint32_t)vram == 0xFFFFFFFF){
		write(1, err, strlen(err));
		close(mouse_handle);
		exit();
	}
	int y = 0;
	int x = 0;
	bool init = true;
	bool update = true;
	//background
	while (1)
		{
			if (y == video.h) {
				break;
			}
			if (x == video.w) {
				y += 1;
				x = 0;
			}
			putpx(vram, video, x, y, 0xFF, 0xFF, 0xFF);
			x += 1;
	}
	while (1) {
		if (init == true || update == true) {
			init = false;
			update = false;
			x = 0;
			y = 0;
			//statusbar
			while (1)
			{
				if (y == 20) {
					break;
				}
				if (x == video.w) {
					y += 1;
					x = 0;
				}
				putpx(vram, video, x, y, 0x00, 0x00, 0x00);
				x += 1;
			}
			x = 0;
			y = 0;
			int wx = 8;
			int wy = 5;
			//statusbar_text P
			while (1)
			{
				if (y == 9) {
					wx += 8;
					x = 0;
					y = 0;
					break;
				}
				if (x == 4) {
					y += 1;
					x = 0;
				}
				if (fP[y][x] == '*') {
					putpx(vram, video, wx + x, wy + y, 0xFF, 0xFF, 0xFF);
				}
				x += 1;
			}
			//statusbar_text o
			while (1)
			{
				if (y == 9) {
					wx += 8;
					x = 0;
					y = 0;
					break;
				}
				if (x == 4) {
					y += 1;
					x = 0;
				}
				if (fo[y][x] == '*') {
					putpx(vram, video, wx + x, wy + y, 0xFF, 0xFF, 0xFF);
				}
				x += 1;
			}
			//statusbar_text n
			while (1)
			{
				if (y == 9) {
					wx += 8;
					x = 0;
					y = 0;
					break;
				}
				if (x == 4) {
					y += 1;
					x = 0;
				}
				if (fn[y][x] == '*') {
					putpx(vram, video, wx + x, wy + y, 0xFF, 0xFF, 0xFF);
				}
				x += 1;
			}
			//statusbar_text d
			while (1)
			{
				if (y == 9) {
					wx += 8;
					x = 0;
					y = 0;
					break;
				}
				if (x == 4) {
					y += 1;
					x = 0;
				}
				if (fd[y][x] == '*') {
					putpx(vram, video, wx + x, wy + y, 0xFF, 0xFF, 0xFF);
				}
				x += 1;
			}
		}
	}
	exit();
}
