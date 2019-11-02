#include "pmm.h"
#include <mm/mm.h>
#include <string.h>
#include <kmesg.h>

#include <arch/mm.h>

#define PMM_MEMORY_BASE 0x2000000

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
#define ROUND_DOWN(N,S) ((N / S) * S)
#define OVERLAPS(a, as, b, bs) ((a) >= (b) && (a + as) <= (b + bs)) // check if a is inside b

uint64_t *pmm_bitmap = NULL;

size_t pmm_bitmap_len = 64;

static int pmm_bit_read(size_t idx) {
	size_t off = idx / 64;
	size_t mask = (1UL << (idx % 64UL));

	return (pmm_bitmap[off] & mask) == mask;
}

static void pmm_bit_write(size_t idx, int bit, size_t count) {
	for (; count; count--, idx++) {
		size_t off = idx / 64;
		size_t mask = (1UL << (idx % 64UL));

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
		kmesg("pmm", "failed to find top of available memory");
	} else {
		kmesg("pmm", "top of available memory is %016lx", top);
	}

	return top;
}

void pmm_init(multiboot_memory_map_t *mmap, size_t mmap_len) {
	uintptr_t mem_top = pmm_find_avail_memory_top(mmap, mmap_len);
	uint32_t mem_pages = (mem_top + PAGE_SIZE - 1) / PAGE_SIZE;

	pmm_bitmap = (uint64_t *)(PMM_MEMORY_BASE + VIRT_PHYS_BASE);
	pmm_bitmap_len = mem_pages;

	size_t bitmap_phys = (size_t)pmm_bitmap - VIRT_PHYS_BASE;

	memset(pmm_bitmap, 0xFF, pmm_bitmap_len / 8);

	for (size_t i = 0; i < mmap_len; i++) {
		if (mmap[i].type == MULTIBOOT_MEMORY_AVAILABLE) {
			uintptr_t start = ROUND_UP(mmap[i].addr, PAGE_SIZE);
			size_t len = ROUND_DOWN(mmap[i].len, PAGE_SIZE);
			size_t count = len / PAGE_SIZE;

			if (!len) continue;
			if (start + len < PMM_MEMORY_BASE) continue;

			if (start < PMM_MEMORY_BASE) {
				len -= PMM_MEMORY_BASE - start;
				start = PMM_MEMORY_BASE;
				count = len / PAGE_SIZE;
			}


			if (OVERLAPS(bitmap_phys, pmm_bitmap_len / 8, start, len)) {
				if (start < bitmap_phys)
					pmm_free((void *)start, (start - bitmap_phys) / PAGE_SIZE);

				start = bitmap_phys + pmm_bitmap_len / 8;
				len -= pmm_bitmap_len / 8;
				count = len / PAGE_SIZE;
			}

			pmm_free((void *)start, count);
		}
	}

	pmm_total_pages = pmm_free_pages;	// all available pages are free

	pmm_bit_write(bitmap_phys / PAGE_SIZE, 1, (pmm_bitmap_len / 8 + PAGE_SIZE - 1) / PAGE_SIZE);

	pmm_alloc((pmm_bitmap_len / 8 + PAGE_SIZE - 1) / PAGE_SIZE);

	kmesg("pmm", "done setting up, %lu pages free", pmm_free_pages);
}

void *pmm_alloc_ex(size_t count, size_t alignment, uintptr_t upper) {
	size_t idx = PMM_MEMORY_BASE / PAGE_SIZE, max_idx = 0;
	
	if (!upper)
		max_idx = pmm_bitmap_len;
	else
		max_idx = pmm_bitmap_len < (upper / PAGE_SIZE) ? pmm_bitmap_len : (upper / PAGE_SIZE);
	
	while(idx < max_idx) {
		if (!pmm_bitmap_is_free(idx, count)) {
			idx += alignment;
			continue;
		}
		pmm_bit_write(idx, 1, count);
		if (pmm_total_pages)
			pmm_free_pages -= count;
		return (void *)(idx * PAGE_SIZE);
	}
	
	return NULL;

}

void *pmm_alloc(size_t count) {
	return pmm_alloc_ex(count, 1, 0);
}

void pmm_free(void *mem, size_t count) {
	size_t idx = (size_t)mem / PAGE_SIZE;
	pmm_bit_write(idx, 0, count);
	pmm_free_pages += count;
}

// arch functions

void *arch_mm_alloc_phys(size_t blocks) {
	return pmm_alloc(blocks);
}

int arch_mm_free_phys(void *mem, size_t blocks) {
	pmm_free(mem, blocks);
	return 1;
}
