#include "devfs.h"
#include <tty/tty.h>
#include <kbd/ps2kbd.h>
#include <mouse/ps2mouse.h>
#include <tasking/tasking.h>
#include <string.h>

#define DEVFS_VFS_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)

struct stat _devfs_stat;

extern int printf(const char *, ...);

extern task_t *current_task;
extern mountpoint_t *mountpoints;

void devfs_init() {
	memset(&_devfs_stat, 0, sizeof(struct stat));
	_devfs_stat.st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
	_devfs_stat.st_atime = 0;
	_devfs_stat.st_mtime = 0;
	_devfs_stat.st_ctime = 0;

	mountpoints[0].flags = 0;
	mountpoints[0].present = true;
	memcpy(mountpoints[0].fs, "devfs", 6);
	memcpy(mountpoints[0].path, "/dev/", 6);
	memcpy(mountpoints[0].dev, "?", 2);
}
extern void vesa_text_write(const char* data, size_t size);

size_t devfs_write(const char *path, char *buffer, size_t count) {
	// if (memcmp(path, "stdout", 7) == 0 || memcmp(path, "stderr", 7) == 0) {
	// 	size_t t = count;
	// 	while (t--) {
	// 		tty_putchar(*buffer++);
	// 	}
	// 	return count;
	// }

	if (memcmp(path, "tty", 4) == 0) {
		
		// while (t--) {
		// 	tty_putchar(*buffer++);
		// }
		vesa_text_write(buffer, count);
		return count;
	}

	return EIO;
}

extern multiboot_info_t *mbootinfo;


size_t devfs_read(const char *path, char *buffer, size_t count) {

	if (strcmp(path, "tty") == 0) {	
		char c = readch();
		size_t s = 0;
		while (c != 0 && s < count) {
			buffer[s++] = c;
			c = readch();
		}
		return s;
	}

	if (strcmp(path, "initrd") == 0) {
		multiboot_module_t* p = (multiboot_module_t*) (0xC0000000 + mbootinfo->mods_addr);

		uint32_t sta = p->mod_start;
		uint32_t end = p->mod_end - 1;

		for (uint32_t i = 0; i <= (end - sta) / 0x1000; i++) {
			map_page((void*)((sta & 0xFFFFF000) + (i * 0x1000)), (void*)(0xE0000000 + (i * 0x1000)), 0x3);
		}

		size_t s = count;
		if ((end - sta) + 1 < count) {
			s = (end - sta) + 1;
		}

		memcpy(buffer, (void *)0xE0000000, s);

		for (uint32_t i = 0; i <= (end - sta) / 0x1000; i++) {
			unmap_page((void*)(0xE0000000 + (i * 0x1000)));
		}

		return s;
	}

	if (strcmp(path, "mouse") == 0) {
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

	if (strcmp(path, "videomode") == 0) {
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
	
	return EIO;
}


int devfs_stat(const char *path, struct stat *destination) {
	if (strcmp(path, "tty") == 0) {
		struct stat s;
		
		s.st_atime = 0;
		s.st_mtime = 0;
		s.st_ctime = 0;
		s.st_mode = S_IFCHR | DEVFS_VFS_MODE;

		memcpy(destination, &s, sizeof(struct stat));

		return 0;
	}

	if (strcmp(path, "mouse") == 0) {
		struct stat s;
		
		s.st_atime = 0;
		s.st_mtime = 0;
		s.st_ctime = 0;
		s.st_mode = S_IFCHR | DEVFS_VFS_MODE;

		memcpy(destination, &s, sizeof(struct stat));

		return 0;
	}

	if (strcmp(path, "initrd") == 0) {
		multiboot_module_t* p = (multiboot_module_t*) (0xC0000000 + mbootinfo->mods_addr);

		uint32_t sta = p->mod_start;
		uint32_t end = p->mod_end - 1;

		struct stat s;
		
		s.st_atime = 0;
		s.st_mtime = 0;
		s.st_ctime = 0;
		s.st_size = end - sta + 1;
		s.st_mode = S_IFBLK | DEVFS_VFS_MODE;

		memcpy(destination, &s, sizeof(struct stat));

		return 0;
	}

	if (strcmp(path, "videomode") == 0) {
		struct stat s;
		
		s.st_atime = 0;
		s.st_mtime = 0;
		s.st_ctime = 0;
		s.st_mode = S_IFCHR | DEVFS_VFS_MODE;

		memcpy(destination, &s, sizeof(struct stat));

		return 0;
	}

	return ENOENT;

}