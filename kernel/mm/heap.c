#include <stdlib.h>
#include <arch/mm.h>
#include <mm/heap.h>
#include <spinlock.h>
#include <util.h>

static spinlock_t mm_lock = {0};

static uintptr_t top = 0x100000000;

void *kmalloc(size_t bytes) {
	spinlock_lock(&mm_lock);

	bytes += 16; // 8 bytes for size, another 8 for amount of pages
	size_t pages = (bytes + ARCH_MM_PAGE_SIZE - 1) / ARCH_MM_PAGE_SIZE + 1;
	void *out = (void *)top;

	for (size_t i = 0; i < pages; i++) {
		void *p = arch_mm_alloc_phys(1);
		if (!p) {
			spinlock_release(&mm_lock);
			return NULL;
		}
		arch_mm_map_kernel(-1, (void *)top, p, 1, ARCH_MM_FLAGS_READ | ARCH_MM_FLAGS_WRITE);
		top += ARCH_MM_PAGE_SIZE;
	}

	top += ARCH_MM_PAGE_SIZE;

	out = (void *)((uintptr_t)out + (pages * ARCH_MM_PAGE_SIZE - bytes));

	((uint64_t *)out)[0] = bytes - 16;
	((uint64_t *)out)[1] = pages;

	spinlock_release(&mm_lock);

	return (void *)((uintptr_t)out + 16);
}

void *kcalloc(size_t bytes, size_t elem) {
	void *out = kmalloc(bytes * elem);
	memset(out, 0, bytes * elem);
	return out;
}

void *krealloc(void *old, size_t s) {
	void *new = kmalloc(s);
	if (old) {
		spinlock_lock(&mm_lock);
		uint64_t size = *(uint64_t *)((uintptr_t)old - 16);
		spinlock_release(&mm_lock);
		memcpy(new, old, size);
		kfree(old);
	}
	return new;
}

void kfree(void *ptr) {
	spinlock_lock(&mm_lock);
	size_t size = *(uint64_t *)((uintptr_t)ptr - 16);
	size_t req_pages = *(uint64_t *)((uintptr_t)ptr - 8);
	void *start = (void *)((uintptr_t)ptr & (~(ARCH_MM_PAGE_SIZE - 1))); // this assumes page size is a multiple of 16

	size += 16; // 8 bytes for size, another 8 for amount of pages
	size_t pages = (size + ARCH_MM_PAGE_SIZE - 1) / ARCH_MM_PAGE_SIZE + 1;

	assert(req_pages == pages);

	for (size_t i = 0; i < pages; i++) {
		void *curr = (void *)((uintptr_t)start + i * ARCH_MM_PAGE_SIZE);
		void *p = (void *)arch_mm_get_phys_kernel(-1, curr);
		arch_mm_unmap_kernel(-1, curr, 1);
		arch_mm_free_phys(p, 1);
	}

	spinlock_release(&mm_lock);
}
