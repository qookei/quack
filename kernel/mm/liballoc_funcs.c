#include <stdlib.h>
#include <arch/mm.h>

int liballoc_lock() {
	return 0;
}

int liballoc_unlock() {
	return 0;
}

// TODO: use the address determined by the arch
uintptr_t top = 0xA0000000;

void *liballoc_alloc(int pages) {
	uintptr_t alloc_addr = top;

	for (int i = 0; i < pages; i++) {
		void *p = arch_mm_alloc_phys(1);
		if (!p) return NULL;
		arch_mm_map_kernel(-1, (void *)top, p, 1, ARCH_MM_FLAGS_READ | ARCH_MM_FLAGS_WRITE);
		top += ARCH_MM_PAGE_SIZE;
	}

	memset((void *)alloc_addr, 0, pages * 4096);

	return (void *)alloc_addr;
}

int liballoc_free(void *ptr, int pages) {
	for (int i = 0; i < pages; i++) {
		void *curr = (void *)((uintptr_t)ptr + i * ARCH_MM_PAGE_SIZE);
		void *p = (void *)arch_mm_get_phys_kernel(-1, curr);
		arch_mm_unmap_kernel(-1, curr, 1);
		arch_mm_free_phys(p, 1);
	}
	
	return 0;
}
