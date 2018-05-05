#include <stdlib.h>
#include <paging/paging.h>

extern int printf(const char *, ...);

extern "C" {

int liballoc_lock() {
	// asm volatile ("cli");

	return 0;
}

int liballoc_unlock() {
	// asm volatile ("sti");

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
	// TODO
	//printf("free the fucking memory you idiot!!!!\n");
	for (int n = 0; n < pages; n++) {
		void *a = get_phys(def_cr3(), ptr);
		unmap_page(ptr);
		pmm_free(a);
		//printf("%p %p\n", a, ptr);
		ptr = (void *)((uint32_t)ptr + 0x1000);
	}
}

}

