#include "pmm.h"

#include <mm/mm.h>

#include <lib/string.c>				// XXX: remove this

#define PMM_MEMORY_BASE 0x1000000

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
#define ROUND_DOWN(N,S) ((N / S) * S)
#define OVERLAPS(a, as, b, bs) ((a) >= (b) && (a + as) <= (b + bs)) // check if a is inside b

uint64_t *pmm_bitmap = NULL;
uint64_t pmm_initial_bitmap[] = {0x0000000000000000};

size_t pmm_bitmap_len = 64;

static int pmm_bit_read(size_t idx) {
	size_t off = idx / 64;
	size_t mask = (1 << (idx % 64));

	return (pmm_bitmap[off] & mask) > 0;
}

static void pmm_bit_write(size_t idx, int bit, size_t count) {
	for (; count; count--, idx++) {
		size_t off = idx / 64;
		size_t mask = (1 << (idx % 64));

		if (bit)
			pmm_bitmap[off] |= mask;
		else
			pmm_bitmap[off] &= ~mask; 
	}
}

static int pmm_bitmap_is_free(size_t idx, size_t count) {
	for (; count; idx++, count--) {
		if (pmm_bit_read(idx))
			return 0;
	}

	return 1;
}

void logf(const char *, ...);			// XXX: remove

static size_t pmm_free_pages = 0;
static size_t pmm_total_pages = 0;

static uintptr_t pmm_find_avail_memory_top(multiboot_memory_map_t *mmap, size_t mmap_len) {
	uintptr_t top = 0;
	for (size_t i = 0; i < mmap_len; i++) {
		if (mmap[i].type == MULTIBOOT_MEMORY_AVAILABLE) {
			if (mmap[i].addr + mmap[i].len > top)
				top = mmap[i].addr + mmap[i].len;
		}
	}

	if (!top) {
		logf("pmm: failed to find top of available memory\n");
	} else {
		uint32_t top_high = top >> 32;
		uint32_t top_low = top & 0xFFFFFFFF;
		logf("pmm: top of available memory is %8x%8x\n", top_high, top_low);
	}

	return top;
}

void pmm_init(multiboot_memory_map_t *mmap, size_t mmap_len) {
	pmm_bitmap = pmm_initial_bitmap;

	uintptr_t mem_top = pmm_find_avail_memory_top(mmap, mmap_len);
	uint32_t mem_pages = (mem_top + PAGE_SIZE - 1) / PAGE_SIZE;
	uint32_t bitmap_bytes = (mem_pages + 7) / 8;
	logf("pmm: keeping track of %u pages, which will cost us %u bytes\n", mem_pages, bitmap_bytes);

	while (mem_pages > pmm_bitmap_len) {
		logf("pmm: allocating a larger bitmap\n");

		size_t can_alloc = (bitmap_bytes + PAGE_SIZE - 1) / PAGE_SIZE;
		if (can_alloc > pmm_bitmap_len) can_alloc = pmm_bitmap_len;

		void *new_bm = (void *)((size_t)pmm_alloc(can_alloc) + PMM_MEMORY_BASE);
		void *old_bm = pmm_bitmap;
		pmm_bitmap = (void *)((uintptr_t)new_bm + VIRT_PHYS_BASE);
		memcpy(pmm_bitmap, old_bm, pmm_bitmap_len / 8);
	
		if (old_bm != pmm_initial_bitmap)
			pmm_free((void *)((uintptr_t)old_bm - VIRT_PHYS_BASE), pmm_bitmap_len / 8 / PAGE_SIZE);
	
		pmm_bitmap_len = can_alloc * PAGE_SIZE * 8;
	}

	logf("pmm: allocation done\n");
	logf("pmm: bitmap length is %u, which is %u bytes\n", (uint32_t)pmm_bitmap_len, (uint32_t)(pmm_bitmap_len / 8));

	size_t bitmap_phys = (size_t)pmm_bitmap - VIRT_PHYS_BASE;

	logf("pmm: bitmap is at %8x\n", (uint32_t)bitmap_phys);

	memset(pmm_bitmap, 0xFF, pmm_bitmap_len / 8);

	for (size_t i = 0; i < mmap_len; i++) {
		if (mmap[i].type == MULTIBOOT_MEMORY_AVAILABLE) {
			uintptr_t start = ROUND_UP(mmap[i].addr, PAGE_SIZE);
			size_t len = ROUND_DOWN(mmap[i].len, PAGE_SIZE);
			size_t count = len / PAGE_SIZE;

			logf("pmm: checking region %8x -- %8x - %u - %u\n", (uint32_t)start, (uint32_t)start + (uint32_t)len, (uint32_t)len, (uint32_t)count);

			if (!len) {
				logf("pmm: discarded, less than one page\n");
				continue;
			}

			if (start + len < PMM_MEMORY_BASE) {
				logf("pmm: discarded, below memory base\n");
				continue;
			}

			if (start < PMM_MEMORY_BASE) {
				logf("pmm: adjusting start addr\n");
				start = PMM_MEMORY_BASE;
				len -= start - PMM_MEMORY_BASE;
				count = len / PAGE_SIZE;
			}


			if (OVERLAPS(bitmap_phys, pmm_bitmap_len / 8, start, len)) {
				logf("pmm: adjusting, overlaps bitmap\n");
				
				if ((size_t)start < bitmap_phys)
					pmm_free((void *)start, ((size_t)start - bitmap_phys) /PAGE_SIZE);

				start = bitmap_phys + pmm_bitmap_len / 8;
				len -= start - PMM_MEMORY_BASE;
				count = len / PAGE_SIZE;
			}

			pmm_free((void *)start, count);
		}
	}

	pmm_total_pages = pmm_free_pages;	// all available pages are free

	logf("pmm: done setting up, %u pages free\n", (uint32_t)pmm_free_pages);

}

void *pmm_alloc(size_t count) {
	size_t idx = 0;
	while(idx < pmm_bitmap_len) {
		if (!pmm_bitmap_is_free(idx, count)) continue;
		pmm_bit_write(idx, 1, count);
		if (pmm_total_pages)
			pmm_free_pages -= count;
		return (void *)(idx * PAGE_SIZE);
	}

	return NULL;
}

void pmm_free(void *mem, size_t count) {
	size_t idx = (size_t)mem / PAGE_SIZE;
	pmm_bit_write(idx, 0, count);
	pmm_free_pages += count;
}