#ifndef LIB_FRG_ALLOCATOR_H
#define LIB_FRG_ALLOCATOR_H

#include <mm/heap.h>

struct frg_allocator {
	void *allocate(size_t s) {
		return kmalloc(s);
	}

	void deallocate(void *p, size_t) {
		if(p)
			kfree(p);
	}

	void free(void *p) {
		deallocate(p, 0);
	}

	static frg_allocator &get() {
		static frg_allocator alloc;
		return alloc;
	}
};

#endif //LIB_FRG_ALLOCATOR_H
