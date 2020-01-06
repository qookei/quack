#include <stdlib.h>
#include <arch/mm.h>
#include <mm/heap.h>
#include <spinlock.h>
#include <util.h>

static spinlock_t mm_lock = {0};

static uintptr_t top = ARCH_MM_HEAP_BASE;

void *kmalloc(size_t bytes) {
	spinlock_lock(&mm_lock);

	bytes = ((bytes + 7) / 8) * 8; // round up to nearest multiple of 8

	bytes += 16; // 8 bytes for size, another 8 for amount of pages
	size_t pages = (bytes + ARCH_MM_PAGE_SIZE - 1) / ARCH_MM_PAGE_SIZE + 1;
	void *out = (void *)top;

	for (size_t i = 0; i < pages; i++) {
		void *p = arch_mm_alloc_phys(1);
		if (!p) {
			spinlock_release(&mm_lock);
			return NULL;
		}
		arch_mm_map_kernel((void *)top, p, 1, ARCH_MM_FLAG_R | ARCH_MM_FLAG_W, ARCH_MM_CACHE_DEFAULT);
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
	void *newp = kmalloc(s);
	if (old) {
		spinlock_lock(&mm_lock);
		uint64_t size = *(uint64_t *)((uintptr_t)old - 16);
		spinlock_release(&mm_lock);
		memcpy(newp, old, size);
		kfree(old);
	}
	return newp;
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
		void *p = (void *)arch_mm_get_phys_kernel(curr);
		arch_mm_unmap_kernel(curr, 1);
		arch_mm_free_phys(p, 1);
	}

	spinlock_release(&mm_lock);
}

void* operator new(size_t size){
	return kmalloc(size);
}

void* operator new[](size_t size){
	return kmalloc(size);
}

void operator delete(void *p){
	kfree(p);
}

void operator delete[](void *p){
	kfree(p);
}

void operator delete(void *p, long unsigned int size){
	(void)(size);
	kfree(p);
}

void operator delete[](void *p, long unsigned int size){
	(void)(size);
	kfree(p);
}

