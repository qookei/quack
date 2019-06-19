#ifndef ARCH_INFO_H
#define ARCH_INFO_H

#include <stdint.h>

typedef struct {
	uint64_t addr;
	int pitch;

	int width;
	int height;
	int bpp;

	int red_off;
	int green_off;
	int blue_off;

	uint32_t red_size;
	uint32_t green_size;
	uint32_t blue_size;
} arch_video_mode_t;

#define ARCH_INFO_HAS_INITRAMFS		(1 << 1)
#define ARCH_INFO_HAS_VIDEO_MODE	(1 << 2)

typedef struct {
	int flags;

	void *initramfs;
	const char *initramfs_cmd;
	size_t initramfs_size;

	arch_video_mode_t *vid_mode;
} arch_boot_info_t;

#endif
