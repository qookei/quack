#include "devices.h"
#include <multiboot.h>
#include <mouse/ps2mouse.h>

int dev_mouse_stat(struct stat *destination);
size_t dev_mouse_read(char *buffer, size_t count);
size_t dev_mouse_write(char *buffer, size_t count);

extern multiboot_info_t *mbootinfo;

extern int printf(const char *, ...);

void dev_mouse_init() {

	devfs_device dev;

	dev.stat = dev_mouse_stat;
	dev.read = dev_mouse_read;
	dev.write = dev_mouse_write;

	strcpy(dev.name, "mouse");

	devfs_register_device(&dev);

}

int dev_mouse_stat(struct stat *destination) {
	struct stat s;
	
	s.st_atime = gettime();
	s.st_mtime = gettime();
	s.st_ctime = gettime();
	s.st_mode = S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

	memcpy(destination, &s, sizeof(struct stat));

	return 0;
}

size_t dev_mouse_read(char *buffer, size_t count) {
	if (!ps2mouse_haschanged()) return 0;
	int32_t x = ps2mouse_get_mouse_x();
	int32_t y = ps2mouse_get_mouse_y();
	uint8_t i = ps2mouse_get_mouse_buttons();

	if (x < 0) x = 0;
	if (y < 0) y = 0;
	
	if (x >= mbootinfo->framebuffer_width)  x = mbootinfo->framebuffer_width - 1;
	if (y >= mbootinfo->framebuffer_height) y = mbootinfo->framebuffer_height - 1;

	char mbuffer[12] = {0};
	((int32_t*)mbuffer)[0] = x;
	((int32_t*)mbuffer)[1] = y;
	((uint32_t*)mbuffer)[2] = i;

	size_t s = count;
	if (s > 12) s = 12;

	memcpy(buffer, mbuffer, s);

	return s;

}

size_t dev_mouse_write(char *buffer, size_t count) {
	return EIO;
}
