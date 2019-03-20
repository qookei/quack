#include "../sys/syscall.h"

int liballoc_lock() {
	return 0;
}

int liballoc_unlock() {
	return 0;
}

void* liballoc_alloc(int pages) {
	return (void *)sys_sbrk(pages * 0x1000);
}

int liballoc_free(void* ptr, int pages) {
	return 1;
}

