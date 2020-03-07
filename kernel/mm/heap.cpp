#include <stdlib.h>
#include <arch/mm.h>
#include <mm/heap.h>
#include <spinlock.h>
#include <frg/mutex.hpp>
#include <util.h>

static spinlock mm_lock;

static uintptr_t top = vm_heap_base;

void *kmalloc(size_t bytes) {
	frg::unique_lock guard{mm_lock};

	bytes = ((bytes + 7) / 8) * 8; // round up to nearest multiple of 8

	bytes += 16; // 8 bytes for size, another 8 for amount of pages
	size_t pages = (bytes + vm_page_size - 1) / vm_page_size + 1;
	void *out = (void *)top;

	for (size_t i = 0; i < pages; i++) {
		void *p = arch_mm_alloc_phys(1);
		if (!p) {
			return NULL;
		}
		arch_mm_map_kernel((void *)top, p, 1, vm_perm::rw, vm_cache::def);
		top += vm_page_size;
	}

	top += vm_page_size;

	out = (void *)((uintptr_t)out + (pages * vm_page_size - bytes));

	((uint64_t *)out)[0] = bytes - 16;
	((uint64_t *)out)[1] = pages;

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
		frg::unique_lock guard{mm_lock};
		uint64_t size = *(uint64_t *)((uintptr_t)old - 16);
		guard.unlock();
		memcpy(newp, old, size);
		kfree(old);
	}
	return newp;
}

void kfree(void *ptr) {
	frg::unique_lock guard{mm_lock};
	size_t size = *(uint64_t *)((uintptr_t)ptr - 16);
	size_t req_pages = *(uint64_t *)((uintptr_t)ptr - 8);
	void *start = (void *)((uintptr_t)ptr & (~(vm_page_size - 1))); // this assumes page size is a multiple of 16

	size += 16; // 8 bytes for size, another 8 for amount of pages
	size_t pages = (size + vm_page_size - 1) / vm_page_size + 1;

	assert(req_pages == pages);

	for (size_t i = 0; i < pages; i++) {
		void *curr = (void *)((uintptr_t)start + i * vm_page_size);
		void *p = (void *)arch_mm_get_phys_kernel(curr);
		arch_mm_unmap_kernel(curr, 1);
		arch_mm_free_phys(p, 1);
	}
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

