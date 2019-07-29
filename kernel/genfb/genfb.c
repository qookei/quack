#include "genfb.h"
#include <arch/info.h>
#include <kmesg.h>
#include <arch/mm.h>
#include <mm/heap.h>
#include <string.h>
#include <spinlock.h>
#include <arch/io.h>

#include "font.h"

static arch_video_mode_t *mode_info = NULL;
static uint32_t *vid_back = NULL;
static uint32_t *vid_front = NULL;

static int disp_w;
static int disp_h;

static int disp_x;
static int disp_y;

static int is_working = 0;

void genfb_init(arch_video_mode_t *mode) {
	if (!mode) {
		kmesg("genfb", "genfb_init called with NULL");
		return;
	}

	if (mode->bpp != 32) {
		kmesg("genfb", "not a 32bpp framebuffer");
		return;
	}

	mode_info = mode;
	disp_w = mode->width / char_width;
	disp_h = mode->height / char_height;
	kmesg("genfb", "initializing a %dx%d character screen", disp_w, disp_h);

	size_t bytes = mode->height * mode->pitch;
	vid_back = kmalloc(bytes);

	memset(vid_back, 0, mode->height * mode->pitch);

	size_t pages = (bytes + ARCH_MM_PAGE_SIZE - 1) / ARCH_MM_PAGE_SIZE;
	arch_mm_map_kernel(-1, (void *)mode->addr, (void *)mode->addr, 
				pages, ARCH_MM_FLAGS_WRITE);
	vid_front = (uint32_t *)mode->addr;

	is_working = 1;
}

static void genfb_putch_internal(char c, int x, int y, uint32_t fg, uint32_t bg) {
	size_t font_off = (size_t)c * char_height * char_width / 8;

	size_t fb_off = (x * 4 + y * mode_info->pitch) / 4;
	for (int y = 0; y < char_height; y++) {
		uint8_t byte = font[font_off + y];
		size_t tmp_fb_off = fb_off;
		for (int x = 0; x < char_width; x++) {
			uint8_t mask = 1 << (8 - x);
			uint32_t col = fg;
			if (!(byte & mask)) col = bg;
			vid_back[tmp_fb_off] = vid_front[tmp_fb_off] = col;
			tmp_fb_off ++;
		}
		fb_off += mode_info->pitch / 4;
	}
}

static void genfb_scroll_up(void) {
	if (!arch_mem_fast_memcpy)
		memcpy(vid_back,
			(void *)((uintptr_t)vid_back + (mode_info->pitch * char_height)),
			mode_info->pitch * (mode_info->height - char_height));
	else
		arch_mem_fast_memcpy(vid_back,
			(void *)((uintptr_t)vid_back + (mode_info->pitch * char_height)),
			mode_info->pitch * (mode_info->height - char_height));

	if (!arch_mem_fast_memset)
		memset((void *)((uintptr_t)vid_back + (mode_info->height - char_height)
			* mode_info->pitch), 0, char_height * mode_info->pitch);
	else
		arch_mem_fast_memset((void *)((uintptr_t)vid_back + (mode_info->height - char_height)
			* mode_info->pitch), 0, char_height * mode_info->pitch);

	if (!arch_mem_fast_memcpy)
		memcpy(vid_front, vid_back, mode_info->height * mode_info->pitch);
	else
		arch_mem_fast_memcpy(vid_front, vid_back, mode_info->height * mode_info->pitch);
}

static spinlock_t lock = {0};

void genfb_putch(char c) {
	if (!is_working)
		return;

	spinlock_lock(&lock);

	if (c == '\n') {
		disp_x = 0;
		disp_y++;
		if (disp_y >= disp_h) {
			genfb_scroll_up();
			disp_y = disp_h - 1;
		}

		spinlock_release(&lock);
		return;
	}

	if (c == '\t') {
		disp_x += 8;
		goto scroll_screen;
	}

	genfb_putch_internal(c, disp_x * char_width, disp_y * char_height,
			0xFFFFFFFF, 0x00000000);
	disp_x++;

scroll_screen:
	if (disp_x >= disp_w) {
		disp_x = 0;
		disp_y++;
		if (disp_y >= disp_h) {
			genfb_scroll_up();
			disp_y = disp_h - 1;
		}
	}

	spinlock_release(&lock);
	return;
}
