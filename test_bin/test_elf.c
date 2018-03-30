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

int execve(const char *path) {
	int _val;
	asm ("int $0x30" : "=a"(_val) : "a"(2), "b"(path));
	return _val;
}

uint32_t reqres(uint32_t type) {
	uint32_t _val;
	asm ("int $0x30" : "=a"(_val) : "a"(8), "b"(type));
	return _val;
}

int fork() {
	int _val;
	asm ("mov $1, %%eax; int $0x30; mov %%eax, %0" : "=g"(_val));
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

void reverse(char *s) {
	int i, j;
	char c;

	for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}

char *itoa(uint32_t n, char *s, int base) {
	int i;
	
	i = 0;
	do {       /* generate digits in reverse order */
		s[i++] = "0123456789ABCDEF"[n % base];   /* get next digit */
	} while ((n /= base) > 0);     /* delete it */
	s[i] = '\0';
	reverse(s);
	return s;
}

void putpx(uint8_t *vram, struct vmode video, uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b) {
	vram[x * (video.b / 8) + y * video.p]     = r;
	vram[x * (video.b / 8) + y * video.p + 1] = g;
	vram[x * (video.b / 8) + y * video.p + 2] = b;
}

void _start(void) {
	struct mpack mouse;
	struct vmode video;
	int d = open("/dev/mouse", 0);
	
	int v = open("/dev/videomode", 0);
	read(v, &video, 16);
	close(v);

	uint8_t *vram = (uint8_t *)reqres(RESOURCE_FRAME_BUFFER);

	if ((uint32_t)vram == 0xFFFFFFFF){
		write(1, err, strlen(err));
		exit();
	}

	write(1, "\ec", 2);

	while(1) {
		int i = read(d, &mouse, 12);
		if (i > 0) {

			if(mouse.b & (1<<2)) {
				putpx(vram, video, mouse.x, mouse.y, mouse.x, 0xFF, mouse.y);
			}
		}
	}
}