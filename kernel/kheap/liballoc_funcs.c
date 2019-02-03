#include <stdlib.h>
#include <paging/paging.h>

extern int kprintf(const char *, ...);

int liballoc_lock() {
	return 0;
}

int liballoc_unlock() {
	return 0;
}

uint32_t top = 0x1000;

void* liballoc_alloc(int pages){

	uint32_t alloc_addr = top;

	for (int i = 0; i < pages; i++) {
		void *p = pmm_alloc();
		map_page(p, (void *)top, 0x3);
		top += 0x1000;
	}

	return (void *)alloc_addr;
}

int liballoc_free(void* ptr, int pages) {
	for (int i = 0; i < pages; i++) {
		void *curr = (void *)((uint32_t)ptr + i * 0x1000);
		void *p = get_phys(def_cr3(), curr);
		unmap_page(curr);
		pmm_free(p);
	}
	
	return 0;
}
