#include "devices.h"
#include <multiboot.h>

int dev_videomode_stat(struct stat *destination);
size_t dev_videomode_read(char *buffer, size_t count);
size_t dev_videomode_write(char *buffer, size_t count);

extern multiboot_info_t *mbootinfo;

extern int printf(const char *, ...);

void dev_videomode_init() {

	devfs_device dev;

	dev.stat = dev_videomode_stat;
	dev.read = dev_videomode_read;
	dev.write = dev_videomode_write;

	strcpy(dev.name, "videomode");

	devfs_register_device(&dev);

}

int dev_videomode_stat(struct stat *destination) {
	struct stat s;
	
	s.st_atime = gettime();
	s.st_mtime = gettime();
	s.st_ctime = gettime();
	s.st_mode = S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

	memcpy(destination, &s, sizeof(struct stat));

	return 0;
}

size_t dev_videomode_read(char *buffer, size_t count) {
	char mbuffer[16] = {0};
	((uint32_t*)mbuffer)[0] = mbootinfo->framebuffer_width;
	((uint32_t*)mbuffer)[1] = mbootinfo->framebuffer_height;
	((uint32_t*)mbuffer)[2] = mbootinfo->framebuffer_pitch;
	((uint32_t*)mbuffer)[3] = mbootinfo->framebuffer_bpp;

	size_t s = count;
	if (s > 16) s = 16;

	memcpy(buffer, mbuffer, s);

	return s;
}

size_t dev_videomode_write(char *buffer, size_t count) {
	return EIO;
}
