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

char *msg1 = "Usermode paint!\n";
char *msg2 = "This is C code running in userspace!\n";

char *msg3 = "Failed to acquire a frame buffer!\n";

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

void _start(void) {
	write(1, msg1, strlen(msg1));
	write(1, msg2, strlen(msg2));

	struct mpack mouse;
	struct vmode video;
	int d = open("/dev/mouse", 0);
	
	int v = open("/dev/videomode", 0);
	read(v, &video, 16);
	close(v);

	uint8_t *vram = (uint8_t *)reqres(RESOURCE_FRAME_BUFFER);

	if ((uint32_t)vram == 0xFFFFFFFF){
		write(1, msg3, strlen(msg3));
		exit();
	}

	write(1, "\ec", 2);

	int u = 1;
	if (!u) {
		// execve("/bin/test2");
	} else {
		while(1) {
			int i = read(d, &mouse, 12);
			if (i > 0) {

				// char buf1[6] = {0x20};
				// char buf2[6] = {0x20};
				// itoa(mouse.x, buf1, 10);
				// itoa(mouse.y, buf2, 10);

				// // write(1, "\ec", 2);
				// write(1, "\e[H", 3);
				// write(1, "Mouse at ", 9);
				// write(1, buf1, strlen(buf1));
				// write(1, ", ", 2);
				// write(1, buf2, strlen(buf2));
				// write(1, "                       \n", 24);

				// if(mouse.b & (1<<2)) {
					vram[mouse.x * (video.b / 8) + mouse.y * video.p] = 0xFF;
					vram[mouse.x * (video.b / 8) + mouse.y * video.p + 1] = 0xFF;
					vram[mouse.x * (video.b / 8) + mouse.y * video.p + 2] = 0xFF;
					vram[mouse.x * (video.b / 8) + mouse.y * video.p + 3] = 0xFF;	
				// }
			}
		}
	}

}